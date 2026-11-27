import heapq
import socket
import sys
import threading

from config import HOST, PORT, MODEL
from session import UserSession
from utils_function import logger, parse_request, send_event


class BackendServer:
    def __init__(self, host: str = HOST, port: int = PORT):
        self.host = host
        self.port = port
        self.sessions = {}
        self.allocated = set()
        self.free_ids = []
        self.next_id = 1
        self.lock = threading.Lock()
        self.running = False
        self.server_socket = None

    def allocate_id(self) -> int:
        with self.lock:
            if self.free_ids:
                user_id = heapq.heappop(self.free_ids)
            else:
                user_id = self.next_id
                self.next_id += 1

            self.allocated.add(user_id)
            return user_id

    def release_id(self, user_id: int) -> None:
        with self.lock:
            if user_id not in self.allocated:
                return

            self.allocated.remove(user_id)
            self.sessions.pop(user_id, None)
            heapq.heappush(self.free_ids, user_id)

    def send_raw(self, conn: socket.socket, event: dict) -> bool:
        try:
            send_event(conn, event)
            return True
        except Exception:
            return False

    def stop(self) -> None:
        self.running = False

        if self.server_socket is not None:
            try:
                self.server_socket.close()
            except OSError:
                pass

    def watch_stdin(self) -> None:
        for line in sys.stdin:
            if line.strip().lower() in ("stop", "quit", "exit"):
                self.stop()
                break

    def serve_forever(self) -> None:
        self.server_socket = socket.socket(
            socket.AF_INET,
            socket.SOCK_STREAM,
        )
        self.server_socket.setsockopt(
            socket.SOL_SOCKET,
            socket.SO_REUSEADDR,
            1,
        )
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(128)
        self.server_socket.settimeout(0.5)
        self.running = True

        logger.info(f"TCP 后端已启动，监听 {self.host}:{self.port}")

        watcher = threading.Thread(
            target=self.watch_stdin,
            daemon=True,
        )
        watcher.start()

        try:
            while self.running:
                try:
                    conn, addr = self.server_socket.accept()
                except socket.timeout:
                    continue
                except OSError:
                    if not self.running:
                        break
                    logger.exception("监听套接字发生异常")
                    break

                thread = threading.Thread(
                    target=self.handle_connection,
                    args=(conn, addr),
                    daemon=True,
                )
                thread.start()

        except KeyboardInterrupt:
            logger.info("收到键盘中断，后端准备退出")

        finally:
            self.stop()
            logger.info("TCP 后端监听循环已退出")

    def handle_connection(self, conn: socket.socket, addr: tuple) -> None:
        session = None
        buffer = b""

        try:
            while True:
                data = conn.recv(4096)

                if not data:
                    break

                buffer += data

                while b"\n" in buffer:
                    line, buffer = buffer.split(b"\n", 1)

                    if not line.strip():
                        continue

                    request = parse_request(line)

                    if request is None:
                        continue

                    request_type = request.get("type")

                    if request_type == "connect":
                        session = self.handle_connect(
                            conn,
                            addr,
                            request,
                            session,
                        )

                    elif request_type == "chat":
                        self.handle_chat(conn, request, session)

                    elif request_type == "disconnect":
                        self.handle_disconnect(conn, session)
                        session = None
                        break

                    else:
                        self.send_raw(
                            conn,
                            {
                                "type": "error",
                                "message": "未知请求类型",
                            },
                        )

        except OSError:
            pass

        except Exception:
            logger.exception("处理客户端连接时发生异常")

        finally:
            if session is not None:
                user_id = session.user_id
                session.close()
                self.release_id(user_id)
                logger.info(
                    f"user_id={user_id} 连接断开，ID 已回收"
                )

            conn.close()

    def handle_connect(
        self,
        conn: socket.socket,
        addr: tuple,
        request: dict,
        session: UserSession | None,
    ) -> UserSession | None:
        if session is not None:
            self.send_raw(
                conn,
                {
                    "type": "connect_failed",
                    "message": "当前连接已经分配过 ID",
                },
            )
            return session

        sk_key = request.get("sk-key")

        if not isinstance(sk_key, str) or not sk_key.strip():
            self.send_raw(
                conn,
                {
                    "type": "connect_failed",
                    "message": "sk-key 为空",
                },
            )
            return None

        model = request.get("model")

        if not isinstance(model, str) or not model.strip():
            model = MODEL
        else:
            model = model.strip()

        user_id = None

        try:
            user_id = self.allocate_id()
            new_session = UserSession(
                user_id,
                sk_key.strip(),
                conn,
                model,
            )
        except Exception as exc:
            logger.exception("创建用户会话失败")

            if user_id is not None:
                self.release_id(user_id)

            self.send_raw(
                conn,
                {
                    "type": "connect_failed",
                    "message": str(exc),
                },
            )
            return None

        with self.lock:
            self.sessions[user_id] = new_session

        logger.info(
            f"user_id={user_id} model={model} 已连接，addr={addr}"
        )

        self.send_raw(
            conn,
            {
                "type": "connected",
                "user_id": user_id,
                "model": model,
            },
        )

        return new_session

    def handle_chat(
        self,
        conn: socket.socket,
        request: dict,
        session: UserSession | None,
    ) -> None:
        if session is None:
            self.send_raw(
                conn,
                {
                    "type": "error",
                    "message": "尚未建立连接",
                },
            )
            return

        user_id = request.get("user_id")
        sk_key = request.get("sk-key")

        if user_id != session.user_id or sk_key != session.sk_key:
            self.send_raw(
                conn,
                {
                    "type": "error",
                    "user_id": session.user_id,
                    "message": "身份校验失败",
                },
            )
            return

        text = request.get("text")

        if not isinstance(text, str) or not text.strip():
            self.send_raw(
                conn,
                {
                    "type": "error",
                    "user_id": session.user_id,
                    "message": "text 为空",
                },
            )
            return

        session.handle_chat(text.strip())

    def handle_disconnect(
        self,
        conn: socket.socket,
        session: UserSession | None,
    ) -> None:
        if session is None:
            return

        user_id = session.user_id

        self.send_raw(
            conn,
            {
                "type": "disconnected",
                "user_id": user_id,
            },
        )

        session.close()
        self.release_id(user_id)

        logger.info(f"user_id={user_id} 主动断开，ID 已回收")
