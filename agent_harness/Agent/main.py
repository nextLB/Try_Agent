"""Harness Python backend.

进程通信约定：
- stdin：Qt 每行发送一个 UTF-8 JSON 请求。
- stdout：Python 每行返回一个 ASCII-only JSON 事件（JSONL）。
- stderr：保留给 Python 未捕获异常等底层输出。
- logs/agent.log：保存普通运行日志和详细异常。
"""

import json
import logging
import sys
from logging.handlers import RotatingFileHandler
from pathlib import Path
from typing import Any, Dict, Optional


# ============================================================
# 路径与日志初始化
# ============================================================

AGENT_DIR = Path(__file__).resolve().parent
LOG_DIR = AGENT_DIR / "logs"
LOG_FILE = LOG_DIR / "agent.log"


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
# API 配置加载
# ============================================================

try:
    # 从 Agent/config.py 读取 API_KEY、BASE_URL 和 MODEL
    from config import API_KEY, BASE_URL, MODEL

    CONFIG_IMPORT_ERROR: Optional[str] = None
except Exception as exc:
    API_KEY = ""
    BASE_URL = "https://dashscope.aliyuncs.com/compatible-mode/v1"
    MODEL = "qwen4-max"
    CONFIG_IMPORT_ERROR = str(exc)
    logger.exception("导入 config.py 失败")


# ============================================================
# 模型客户端与对话上下文
# ============================================================

client: Any = None

MESSAGES = [
    {
        "role": "system",
        "content": "你是一个简洁、友好的助手，请使用中文回答。",
    }
]


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


# ============================================================
# 日志：写入文件
# ============================================================

def log(message: str) -> None:
    """将普通运行日志写入 logs/agent.log。"""
    logger.info(message)


# ============================================================
# 初始化大模型客户端
# ============================================================

def initialize_client() -> bool:
    """初始化 OpenAI 兼容客户端；成功返回 True，失败返回 False。"""
    global client

    if CONFIG_IMPORT_ERROR is not None:
        log(f"config.py 导入失败：{CONFIG_IMPORT_ERROR}")
        return False

    if not str(API_KEY).strip():
        log("API_KEY 未配置，请检查 Agent/config.py")
        return False

    if not str(BASE_URL).strip():
        log("BASE_URL 未配置，请检查 Agent/config.py")
        return False

    try:
        # 延迟导入，确保 openai 未安装时也能向 Qt 发送初始化失败状态
        from openai import OpenAI

        client = OpenAI(
            api_key=str(API_KEY).strip(),
            base_url=str(BASE_URL).strip(),
        )

        log(f"OpenAI 兼容客户端初始化成功，model={MODEL}")
        return True

    except Exception:
        logger.exception("OpenAI 兼容客户端初始化失败")
        client = None
        return False


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


# ============================================================
# 文本请求处理
# ============================================================

def handle_text_request(
    user_text: str,
    request_id: Optional[str] = None,
) -> None:
    """调用模型处理一条文本请求，并通过 stdout 流式返回 JSON 事件。"""
    if client is None:
        emit_event(
            {
                "type": "error",
                "request_id": request_id,
                "message": "模型客户端尚未初始化，请检查 config.py 和 logs/agent.log。",
            }
        )
        return

    user_message = {
        "role": "user",
        "content": user_text,
    }
    MESSAGES.append(user_message)

    assistant_parts = []
    usage_data = None

    log(f"收到文本请求，字符数={len(user_text)}")

    # 通知 Qt 本轮请求开始
    emit_event(
        {
            "type": "response_started",
            "request_id": request_id,
        }
    )

    try:
        # 调用 DashScope OpenAI 兼容接口，开启流式输出
        stream = client.chat.completions.create(
            model=MODEL,
            messages=MESSAGES,
            temperature=0.7,
            stream=True,
            stream_options={"include_usage": True},
        )

        # 逐块读取模型返回内容
        for chunk in stream:
            # 最后一个流式数据块可能只包含 usage，choices 为空
            if getattr(chunk, "usage", None) is not None:
                usage_data = get_usage_data(chunk.usage)

            if not chunk.choices:
                continue

            delta = chunk.choices[0].delta
            content = getattr(delta, "content", None)

            if content:
                assistant_parts.append(content)

                # 每收到一段模型文本，就通过 JSONL 发送给 Qt
                emit_event(
                    {
                        "type": "response_delta",
                        "request_id": request_id,
                        "text": content,
                    }
                )

        # 整合完整回答并保存上下文
        assistant_text = "".join(assistant_parts)
        MESSAGES.append(
            {
                "role": "assistant",
                "content": assistant_text,
            }
        )

        log(
            f"模型请求完成，回答字符数={len(assistant_text)}，"
            f"usage={usage_data}"
        )

        # 通知 Qt 本轮回答结束
        emit_event(
            {
                "type": "response_done",
                "request_id": request_id,
                "text": assistant_text,
                "usage": usage_data,
            }
        )

    except Exception as exc:
        # 详细异常与 traceback 写入日志文件
        logger.exception("模型 API 请求失败")

        # 请求失败时移除本轮未完成的用户消息，避免上下文不完整
        if MESSAGES and MESSAGES[-1] is user_message:
            MESSAGES.pop()

        # 将业务错误作为标准 JSON 事件返回 Qt
        emit_event(
            {
                "type": "error",
                "request_id": request_id,
                "message": str(exc),
            }
        )


# ============================================================
# JSONL 输入处理：stdin 一行对应一条 Qt 请求
# ============================================================

def handle_input_line(raw_line: bytes) -> None:
    """解析 Qt 发送的一行 UTF-8 JSON，并分发文本请求。"""
    line = raw_line.decode("utf-8", errors="replace").strip()
    if not line:
        return

    try:
        message = json.loads(line)
    except json.JSONDecodeError as exc:
        logger.warning("收到无效 JSON：%s", exc)
        emit_event(
            {
                "type": "error",
                "message": f"无效 JSON：{exc.msg}",
            }
        )
        return

    if not isinstance(message, dict):
        emit_event(
            {
                "type": "error",
                "message": "请求格式错误：JSON 顶层必须是对象。",
            }
        )
        return

    request_id = message.get("request_id")
    if request_id is not None:
        request_id = str(request_id)

    # 当前版本先处理 text 字段
    user_text = message.get("text")

    if not isinstance(user_text, str):
        emit_event(
            {
                "type": "error",
                "request_id": request_id,
                "message": "请求缺少有效的 text 字段。",
            }
        )
        return

    if not user_text.strip():
        emit_event(
            {
                "type": "error",
                "request_id": request_id,
                "message": "text 字段不能为空。",
            }
        )
        return

    handle_text_request(user_text, request_id)


# ============================================================
# Python 主循环
# ============================================================

def main() -> int:
    """持续读取 Qt stdin，直到 Qt 关闭输入通道或进程退出。"""
    log("Python Agent 后端启动")
    log(f"模型配置：{MODEL}")
    log(f"日志文件：{LOG_FILE}")

    # 初始化客户端，并向 Qt 发送后端就绪状态
    # ready=True 仅代表客户端对象创建成功，不代表 API Key 已通过远端验证
    backend_ready = initialize_client()

    if backend_ready:
        emit_event(
            {
                "type": "status",
                "ready": True,
                "message": "Python 后端已就绪",
                "model": MODEL,
            }
        )
        log("已向 Qt 发送后端就绪状态")
    else:
        emit_event(
            {
                "type": "status",
                "ready": False,
                "message": "模型客户端初始化失败，请检查 config.py、openai 依赖和 logs/agent.log。",
                "model": MODEL,
            }
        )
        log("已向 Qt 发送后端未就绪状态")

    # 持续接收 Qt 发来的 JSONL 请求
    try:
        for raw_line in sys.stdin.buffer:
            handle_input_line(raw_line)

    except KeyboardInterrupt:
        log("收到键盘中断，准备退出")

    except Exception:
        logger.exception("Python 主循环发生未处理异常")
        return 1

    log("stdin 已关闭，Python Agent 后端退出")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
