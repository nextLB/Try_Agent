"""Harness Python backend.

通信约定：
- 后端常驻监听 TCP 端口，等待多个 App 连接。
- 每条连接对应一个用户会话，使用换行分隔的 ASCII-only JSONL 收发消息。
- sk-key 由客户端在 connect/chat 请求中提供，后端不保存密钥。
- logs/agent.log 保存普通运行日志和详细异常。
"""

from config import HOST, PORT, MODEL, LOG_FILE
from server import BackendServer
from utils_function import logger


def main() -> int:
    logger.info("Harness Python 后端启动")
    logger.info(f"监听地址：{HOST}:{PORT}")
    logger.info(f"模型配置：{MODEL}")
    logger.info(f"日志文件：{LOG_FILE}")

    server = BackendServer(HOST, PORT)
    server.serve_forever()

    logger.info("Harness Python 后端退出")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
