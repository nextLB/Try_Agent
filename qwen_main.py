from openai import OpenAI
from tools_registry import get_tools, execute_tool, list_tools
import tools
import json


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
# 上下文存储加系统级设定
MESSAGES = [{"role": "system", "content": "你是一个简洁友好的助手，用中文回答。"}]
TASK_COMPLETED = False

# =========================================================== #
# =========================================================== #
# =========================================================== #






# =========================================================== #
# =========================================================== #
# =========================================================== #


# API调用模型的后处理
# 包含：流式文本输出、工具调用信息收集、Token用量统计
def API_model_after_handle(resp):

    # 保存模型返回的文本片段
    answer_parts = []

    # 保存模型返回的工具调用信息
    tool_calls = {}

    # 保存 Token 用量
    usage = None

    # 记录是否已经输出过助手前缀
    assistant_prefix_printed = False

    # 遍历流式响应
    for chunk in resp:

        # ==========================================
        # 1. 获取 Token 用量
        # ==========================================
        if not chunk.choices:
            usage = chunk.usage
            continue

        # 获取当前 Chunk 的增量数据
        delta = chunk.choices[0].delta

        # ==========================================
        # 2. 处理普通文本
        # ==========================================
        piece = delta.content or ""

        if piece:

            # 只有第一次收到文本时才打印助手前缀
            if not assistant_prefix_printed:
                print("助手: ", end="", flush=True)
                assistant_prefix_printed = True

            # 保存文本片段
            answer_parts.append(piece)

            # 实时输出文本
            print(piece, end="", flush=True)

        # ==========================================
        # 3. 处理工具调用信息
        # ==========================================
        if delta.tool_calls:

            for tool_call in delta.tool_calls:

                # 获取当前工具调用的索引
                index = tool_call.index

                # 第一次接收到该工具调用时初始化
                if index not in tool_calls:

                    tool_calls[index] = {
                        "id": "",
                        "type": "function",
                        "function": {
                            "name": "",
                            "arguments": ""
                        }
                    }

                current_call = tool_calls[index]

                # ----------------------------------
                # 拼接工具调用 ID
                # ----------------------------------
                if tool_call.id:
                    current_call["id"] += tool_call.id

                # ----------------------------------
                # 拼接工具调用类型
                # ----------------------------------
                if tool_call.type:
                    current_call["type"] = tool_call.type

                # ----------------------------------
                # 拼接函数名称和参数
                # ----------------------------------
                if tool_call.function:

                    if tool_call.function.name:
                        current_call["function"]["name"] += (
                            tool_call.function.name
                        )

                    if tool_call.function.arguments:
                        current_call["function"]["arguments"] += (
                            tool_call.function.arguments
                        )

    # ==========================================
    # 4. 整理输出
    # ==========================================

    # 如果输出过文本，结束当前输出行
    if assistant_prefix_printed:
        print("\n")

    # 按工具调用索引排序
    tool_call_list = [
        tool_calls[index]
        for index in sorted(tool_calls.keys())
    ]

    # ==========================================
    # 5. 返回处理结果
    # ==========================================
    return answer_parts, tool_call_list, usage


# 工具处理函数
def handle_tool_calls(tool_calls):

    tool_results = []

    for tool_call in tool_calls:

        tool_name = tool_call["function"]["name"]

        tool_arguments = tool_call["function"]["arguments"]

        tool_call_id = tool_call["id"]

        print(f"\n[调用工具] {tool_name}")
        print(f"[工具参数] {tool_arguments}")

        result = execute_tool(
            name=tool_name,
            arguments=tool_arguments
        )

        print(f"[工具结果] {result}")

        tool_results.append({
            "role": "tool",
            "tool_call_id": tool_call_id,
            "content": json.dumps(result, ensure_ascii=False)
        })

    return tool_results

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
        user_input = input("你: ").strip()
        if not user_input:
            continue
        if user_input.lower() in ("exit", "quit"):
            print("再见。")
            break


        # 放到上下文里面，后续一并送入模型
        MESSAGES.append({"role": "user", "content": user_input})

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




