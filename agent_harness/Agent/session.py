import threading
from typing import Any

from openai import OpenAI

from config import BASE_URL, MODEL
from utils_function import (
    create_user_message,
    logger,
    send_event,
    text_streaming_output,
)

SYSTEM_PROMPT = "你是一个简洁、友好的助手，请使用中文回答。"


class UserSession:
    def __init__(self, user_id: int, sk_key: str, socket_obj: Any):
        self.user_id = user_id
        self.sk_key = sk_key
        self.socket = socket_obj
        self.lock = threading.Lock()
        self.alive = True
        self.messages = [
            {
                "role": "system",
                "content": SYSTEM_PROMPT,
            }
        ]
        self.client = OpenAI(
            api_key=sk_key,
            base_url=BASE_URL,
        )

    def is_alive(self) -> bool:
        return self.alive

    def send(self, event: dict) -> bool:
        if not self.alive:
            return False

        payload = dict(event)
        payload["user_id"] = self.user_id

        with self.lock:
            try:
                send_event(self.socket, payload)
                return True
            except Exception:
                self.alive = False
                return False

    def send_delta(self, content: str) -> bool:
        return self.send({
            "type": "response_delta",
            "text": content,
        })

    def handle_chat(self, text: str) -> None:
        if not self.alive:
            return

        user_message = create_user_message(text)
        self.messages.append(user_message)

        if not self.send({"type": "response_started"}):
            self.messages.pop()
            return

        try:
            stream = self.client.chat.completions.create(
                model=MODEL,
                messages=self.messages,
                temperature=0.7,
                stream=True,
                stream_options={"include_usage": True},
            )

            assistant_parts, usage_data = text_streaming_output(
                stream,
                self.send_delta,
                self.is_alive,
            )

            assistant_text = "".join(assistant_parts)
            self.messages.append({
                "role": "assistant",
                "content": assistant_text,
            })

            logger.info(
                f"user_id={self.user_id} 模型请求完成，"
                f"回答字符数={len(assistant_text)}，usage={usage_data}"
            )

            self.send({
                "type": "response_done",
                "text": assistant_text,
                "usage": usage_data,
            })

        except Exception as exc:
            logger.exception(
                f"user_id={self.user_id} 模型 API 请求失败"
            )

            if self.messages and self.messages[-1] is user_message:
                self.messages.pop()

            self.send({
                "type": "error",
                "message": str(exc),
            })

    def close(self) -> None:
        self.alive = False
        self.messages.clear()

        try:
            self.client.close()
        except Exception:
            pass
