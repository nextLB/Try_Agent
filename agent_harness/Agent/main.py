"""Harness Python backend.

进程通信约定：
- stdin：Qt 每行发送一个 UTF-8 JSON 请求。
- stdout：Python 每行返回一个 ASCII-only JSON 事件（JSONL）。
- stderr：保留给 Python 未捕获异常等底层输出。
- logs/agent.log：保存普通运行日志和详细异常。
"""

import sys
from typing import Any
from utils_function import parse_user_message, create_user_message, text_streaming_output, emit_event, logger
from config import API_KEY, BASE_URL, MODEL, LOG_FILE
from openai import OpenAI




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


    if not str(API_KEY).strip():
        log("API_KEY 未配置，请检查 Agent/config.py")
        return False

    if not str(BASE_URL).strip():
        log("BASE_URL 未配置，请检查 Agent/config.py")
        return False

    try:

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
# 文本请求处理
# ============================================================

def handle_text_request(
    user_text: str
) -> None:
    """调用模型处理一条文本请求，并通过 stdout 流式返回 JSON 事件。"""

    user_message = create_user_message(user_text)

    MESSAGES.append(user_message)

    try:
        # 调用 DashScope OpenAI 兼容接口，开启流式输出
        stream = client.chat.completions.create(
            model=MODEL,
            messages=MESSAGES,
            temperature=0.7,
            stream=True,
            stream_options={"include_usage": True},
        )

        # 接收模型流式输出
        assistant_parts, usage_data = text_streaming_output(stream)

        # 整合完整回答
        assistant_text = "".join(assistant_parts)

        # 保存助手回答到上下文
        MESSAGES.append({
            "role": "assistant",
            "content": assistant_text,
        })

        log(
            f"模型请求完成，回答字符数={len(assistant_text)}，"
            f"usage={usage_data}"
        )

        # 通知 Qt 本轮回答结束
        emit_event({
            "type": "response_done",
            "text": assistant_text,
            "usage": usage_data,
        })

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
                "message": str(exc),
            }
        )







# ============================================================
# JSONL 输入处理：stdin 一行对应一条 Qt 请求
# ============================================================

def handle_input_line(raw_line: bytes) -> None:
    """解析 Qt 发送的一行 UTF-8 JSON，并分发文本请求。"""
    user_text = parse_user_message(raw_line)

    handle_text_request(user_text)










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



