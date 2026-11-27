
from pathlib import Path


AGENT_DIR = Path(__file__).resolve().parent

LOG_DIR = AGENT_DIR / "logs"

LOG_FILE = LOG_DIR / "agent.log"



# ============================================================
# 大模型 API 配置
# ============================================================

# API 密钥
API_KEY = "sk-ws-H.XDDDDLP.ygf2.MEYCIQChE7IwGbf4eXeJEhgQGpGCEJ0_ZSewWbY0LvwrJHy4XAIhAJpfs1eriYZc_vpYZhgPhbum6iVCEYqhaaZ_uxGiuxoN"

# API 接口地址
BASE_URL = "https://dashscope.aliyuncs.com/compatible-mode/v1"

# 模型名称
MODEL = "qwen4-max"
