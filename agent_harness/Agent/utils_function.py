import json
from typing import Any, Dict, Optional
import sys
import json
import logging
from logging.handlers import RotatingFileHandler
from config import LOG_DIR, LOG_FILE


# ============================================================
# 日志初始化
# ============================================================

def setup_logger() -> logging.Logger:
    """初始化滚动日志文件，不向 stdout 或 stderr 输出普通日志。"""
    LOG_DIR.mkdir(parents=True, exist_ok=True)

    agent_logger = logging.getLogger("HarnessAgent")
    agent_logger.setLevel(logging.INFO)
    agent_logger.propagate = False

    if not agent_logger.handlers:
        handler = RotatingFileHandler(
            LOG_FILE,
            maxBytes=5 * 1024 * 1024,
            backupCount=3,
            encoding="utf-8",
        )
        handler.setFormatter(
            logging.Formatter(
                "%(asctime)s [%(levelname)s] %(message)s",
                datefmt="%Y-%m-%d %H:%M:%S",
            )
        )
        agent_logger.addHandler(handler)

    return agent_logger


logger = setup_logger()



# ============================================================
# stdout 通信：只发送 ASCII-only JSONL
# ============================================================

def emit_event(event: Dict[str, Any]) -> None:
    """向 Qt stdout 通道发送一条 JSON 事件，每条事件以换行符结束。

    ensure_ascii=True 会把中文转换为 Unicode 转义序列，保证输出字节为 ASCII，
    避免 Windows 控制台代码页影响 QProcess 接收到的 JSON 数据。
    Qt 的 QJsonDocument 解析后会自动还原中文。
    """
    payload = json.dumps(
        event,
        ensure_ascii=True,
        separators=(",", ":"),
    )

    sys.stdout.buffer.write(payload.encode("ascii") + b"\n")
    sys.stdout.buffer.flush()



def parse_user_message(raw_line: bytes) -> str | None:
    """
    解析 C++ 发送的 JSONL 消息，提取用户文本。

    Args:
        raw_line: 原始 JSON 字节数据。

    Returns:
        str: 有效的用户输入文本。
        None: JSON 解析失败、格式错误或文本为空。
    """

    line = raw_line.decode("utf-8", errors="replace").strip()

    if not line:
        return None

    try:
        message = json.loads(line)
    except json.JSONDecodeError:
        return None

    if not isinstance(message, dict):
        return None

    user_text = message.get("text")

    if not isinstance(user_text, str):
        return None

    user_text = user_text.strip()

    if not user_text:
        return None

    return user_text


def create_user_message(user_text: str) -> dict:
    """
    创建标准格式的用户消息。

    Args:
        user_text: 用户输入的文本。

    Returns:
        dict: 标准格式的用户消息。
    """

    user_message = {
        "role": "user",
        "content": user_text,
    }

    return user_message


# ============================================================
# Token 使用量处理
# ============================================================

def get_usage_data(usage: Any) -> Optional[Dict[str, Any]]:
    """提取流式响应中的 Token 使用量。"""
    if usage is None:
        return None

    return {
        "prompt_tokens": getattr(usage, "prompt_tokens", None),
        "completion_tokens": getattr(usage, "completion_tokens", None),
        "total_tokens": getattr(usage, "total_tokens", None),
    }



def text_streaming_output(stream: object) -> tuple[list[str], dict | None]:
    """
    接收模型流式输出，提取文本并实时发送给 Qt。

    Args:
        stream: 模型 API 返回的流式数据对象。

    Returns:
        assistant_parts: 模型返回的文本片段列表。
        usage_data: Token 使用量信息。
    """

    assistant_parts = []
    usage_data = None

    # 逐块读取模型返回内容
    for chunk in stream:

        # 获取 Token 使用量
        if getattr(chunk, "usage", None) is not None:
            usage_data = get_usage_data(chunk.usage)

        # 最后一个流式数据块可能只包含 usage
        if not chunk.choices:
            continue

        delta = chunk.choices[0].delta
        content = getattr(delta, "content", None)

        if content:
            assistant_parts.append(content)

            # 实时发送文本片段给 Qt
            emit_event({
                "type": "response_delta",
                "text": content,
            })

    return assistant_parts, usage_data



