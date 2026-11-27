import json
import sys


def main() -> None:
    # 使用 UTF-8 处理 Windows 子进程管道中的中文文本
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")

    print("[Python] main.py 已启动，等待 Qt 消息……", flush=True)

    # 按行读取 Qt 通过 stdin 发送的 JSON 数据
    for raw_line in sys.stdin.buffer:
        line = raw_line.decode("utf-8").strip()
        if not line:
            continue

        try:
            message = json.loads(line)
            print(
                "[Python] 收到 JSON："
                + json.dumps(message, ensure_ascii=False),
                flush=True,
            )
        except json.JSONDecodeError as exc:
            print(f"[Python] JSON 解析失败：{exc}", file=sys.stderr, flush=True)

    print("[Python] stdin 已关闭，Python 子进程退出。", flush=True)


if __name__ == "__main__":
    main()
