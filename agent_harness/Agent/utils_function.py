import json
from typing import Any, Callable, Dict, Optional
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
# socket 通信：只发送 ASCII-only JSONL
# ============================================================

def send_event(socket_obj: Any, event: Dict[str, Any]) -> None:
    payload = json.dumps(
        event,
        ensure_ascii=True,
        separators=(",", ":"),
    )

    socket_obj.sendall(payload.encode("ascii") + b"\n")


def parse_request(raw_line: bytes) -> Optional[Dict[str, Any]]:
    line = raw_line.decode("utf-8", errors="replace").strip()

    if not line:
        return None

    try:
        message = json.loads(line)
    except json.JSONDecodeError:
        return None

    if not isinstance(message, dict):
        return None

    event_type = message.get("type")

    if not isinstance(event_type, str) or not event_type:
        return None

    return message


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


def text_streaming_output(
    stream: object,
    send_delta: Callable[[str], bool],
    is_alive: Callable[[], bool],
) -> tuple[list[str], dict | None]:
    """
    接收模型流式输出，提取文本并实时发送给对应连接。

    Args:
        stream: 模型 API 返回的流式数据对象。
        send_delta: 发送文本片段的回调，返回 False 表示连接已断开。
        is_alive: 查询连接是否仍然有效的回调。

    Returns:
        assistant_parts: 模型返回的文本片段列表。
        usage_data: Token 使用量信息。
    """

    assistant_parts = []
    usage_data = None

    for chunk in stream:

        if not is_alive():
            break

        if getattr(chunk, "usage", None) is not None:
            usage_data = get_usage_data(chunk.usage)

        if not chunk.choices:
            continue

        delta = chunk.choices[0].delta
        content = getattr(delta, "content", None)

        if content:
            assistant_parts.append(content)

            if not send_delta(content):
                break

    return assistant_parts, usage_data
