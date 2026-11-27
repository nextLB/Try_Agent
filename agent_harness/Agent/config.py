
from pathlib import Path


AGENT_DIR = Path(__file__).resolve().parent

LOG_DIR = AGENT_DIR / "logs"

LOG_FILE = LOG_DIR / "agent.log"

HOST = "127.0.0.1"

PORT = 9000


# ============================================================
# 大模型 API 配置
# ============================================================

# API 接口地址
BASE_URL = "https://dashscope.aliyuncs.com/compatible-mode/v1"

# 模型名称
MODEL = "qwen4-max"
