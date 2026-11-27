"""
一键打开钉钉日历（相对坐标版 · 零 pyautogui 依赖）
"""

import ctypes
import time
import sys
from ctypes import wintypes

try:
    ctypes.windll.user32.SetProcessDpiAwarenessContext(-4)
except Exception:
    pass

import psutil
import pygetwindow as gw

# ================= 配置区 =================
DINGTALK_EXE = "dingtalk.exe"
REL_X = 74
REL_Y = 251
# ==========================================


def click_at(x, y):
    """用 Win32 API 模拟鼠标左键点击，不依赖 pyautogui"""
    ctypes.windll.user32.SetCursorPos(int(x), int(y))
    time.sleep(0.05)
    ctypes.windll.user32.mouse_event(0x0002, 0, 0, 0, 0)  # 左键按下
    time.sleep(0.05)
    ctypes.windll.user32.mouse_event(0x0004, 0, 0, 0, 0)  # 左键抬起


def is_dingtalk_running():
    for p in psutil.process_iter(["name"]):
        try:
            if (p.info["name"] or "").lower() == DINGTALK_EXE:
                return True
        except Exception:
            continue
    return False


def find_dingtalk_window():
    best = None
    for w in gw.getAllWindows():
        try:
            if not w.visible or not w.title:
                continue
            title = w.title.lower()
            if "钉钉" not in title and "dingtalk" not in title:
                continue
            if any(b in title for b in ("edge", "chrome", "firefox")):
                continue
            if w.width < 300 or w.height < 300:
                continue
            if best is None or w.width * w.height > best.width * best.height:
                best = w
        except Exception:
            continue
    return best


def activate_window(win):
    if win.isMinimized:
        win.restore()
        time.sleep(0.3)
    try:
        win.activate()
    except Exception:
        # 兜底：点击标题栏把窗口拉起来
        click_at(win.left + win.width // 2, win.top + 10)
    time.sleep(0.8)


def open_calendar():
    if not is_dingtalk_running():
        return {"ok": False, "error": "钉钉未启动，请先打开并登录钉钉客户端"}

    win = find_dingtalk_window()
    if not win:
        return {"ok": False, "error": "未找到钉钉主窗口"}

    activate_window(win)

    abs_x = win.left + REL_X
    abs_y = win.top + REL_Y
    print(f"[点击] 窗口=({win.left},{win.top}) 相对=({REL_X},{REL_Y}) 绝对=({abs_x},{abs_y})")
    click_at(abs_x, abs_y)
    time.sleep(0.5)
    return {"ok": True, "message": "已点击钉钉日历入口"}


def main():
    print("[Agent] 正在检测钉钉并打开日历...")
    result = open_calendar()
    print(result)
    if not result["ok"]:
        sys.exit(1)


if __name__ == "__main__":
    main()