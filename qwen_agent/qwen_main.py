from openai import OpenAI
from tools_registry import get_tools, execute_tool, list_tools
from common_utils import (
    API_model_after_handle,
    handle_tool_calls,
    build_user_message
)
import tools

# =========================================================== #
# =========================================================== #
# =========================================================== #

# Config
API_KEY = "sk-ws-H.XDDDDLP.ygf2.MEYCIQChE7IwGbf4eXeJEhgQGpGCEJ0_ZSewWbY0LvwrJHy4XAIhAJpfs1eriYZc_vpYZhgPhbum6iVCEYqhaaZ_uxGiuxoN"
client = OpenAI(
    api_key=API_KEY,
    base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
)
# MODEL = "qwen-plus"     # 语言模型
MODEL = "qwen3.8-omni-flash"    # 多模态模型
# 上下文存储 (后续也可加系统设定)
MESSAGES = []
TASK_COMPLETED = False

# =========================================================== #
# =========================================================== #
# =========================================================== #






# =========================================================== #
# =========================================================== #
# =========================================================== #
# 获取用户输入
def get_user_message():

    # ==========================================
    # qwen-plus 纯文本模型
    # ==========================================
    if MODEL == "qwen-plus":

        print("\n当前模型：纯文本模式")
        user_input = input("请输入文本：").strip()

        if user_input.lower() in ("exit", "quit"):
            return "EXIT"

        if not user_input:
            return None

        return {
            "role": "user",
            "content": user_input
        }

    # ==========================================
    # qwen3.8-omni-flash 多模态模型
    # ==========================================
    elif MODEL == "qwen3.8-omni-flash":

        print("\n当前模型: 多模态模式")

        text = input("请输入文本（可为空）：").strip()

        if text.lower() in ("exit", "quit"):
            return "EXIT"

        image = input("请输入图片路径（可为空）：").strip()

        audio = input("请输入音频路径（可为空）：").strip()

        video = input("请输入视频路径（可为空）：").strip()

        try:

            message = build_user_message(
                text=text,
                image=image,
                audio=audio,
                video=video
            )

            if message is None:
                print("\n[提示] 至少需要输入一种内容，请重新输入。\n")
                return None

            return message

        except Exception as e:

            print(f"\n[输入处理出错] {e}\n")
            return None

        


# =========================================================== #
# =========================================================== #
# =========================================================== #






# =========================================================== #
# =========================================================== #
# =========================================================== #



# 主函数
def main():

    print("输入内容开始聊天，输入 exit 或 quit 退出。\n")

    # Agent Loop
    while True:
        # 获取用户输入
        user_message = get_user_message()

        if user_message is None:
            continue

        # 处理退出指令
        if user_message == "EXIT":
            print("再见。")
            break


        # 放到上下文里面，后续一并送入模型
        MESSAGES.append(user_message)

        # 重置标志
        TASK_COMPLETED = False

        # 当本地调用处理任务还未结束的话就继续给到模型
        while not TASK_COMPLETED:

            # 送入模型，执行推理获取返回结果
            try:
                resp = client.chat.completions.create(
                    model=MODEL,
                    messages=MESSAGES,
                    temperature=0.7,
                    tools=get_tools(),
                    tool_choice="auto",
                    stream=True,
                    stream_options={"include_usage": True},
                )
            except Exception as e:
                print(f"\n[模型请求出错] {e}\n")
                TASK_COMPLETED = True
                continue


            # 对于获取到的返回结果进行流式输出
            answer_parts, tool_calls, usage = API_model_after_handle(resp)


            # Token 统计
            if usage:
                print(
                    f"[tokens] prompt={usage.prompt_tokens} "
                    f"completion={usage.completion_tokens}\n"
                )



            # ==========================================
            # 情况一：模型要求调用工具
            # ==========================================
            if tool_calls:

                # 保存模型工具调用请求
                MESSAGES.append({
                    "role": "assistant",
                    "content": "".join(answer_parts) or None,
                    "tool_calls": tool_calls
                })

                # 执行工具
                tool_results = handle_tool_calls(tool_calls)

                # 保存工具执行结果
                MESSAGES.extend(tool_results)

                # 当前任务尚未完成，继续 Agent Loop
                TASK_COMPLETED = False

            # ==========================================
            # 情况二：模型没有要求调用工具
            # ==========================================
            else:

                answer = "".join(answer_parts)

                # 保存模型最终回答
                MESSAGES.append({
                    "role": "assistant",
                    "content": answer
                })

                # 输出最终回答
                # 注意：如果 streaming_output 已经打印了文本，
                # 这里不需要再重复 print。

                # 当前任务完成
                TASK_COMPLETED = True


# =========================================================== #
# =========================================================== #
# =========================================================== #



if __name__ == "__main__":
    main()




