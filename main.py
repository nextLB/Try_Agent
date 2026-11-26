from openai import OpenAI


# Config
API_KEY = "sk-ws-H.XDDDDLP.ygf2.MEYCIQChE7IwGbf4eXeJEhgQGpGCEJ0_ZSewWbY0LvwrJHy4XAIhAJpfs1eriYZc_vpYZhgPhbum6iVCEYqhaaZ_uxGiuxoN"
client = OpenAI(
    api_key=API_KEY,
    base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
)
MODEL = "qwen-plus"
# 上下文存储加系统级设定
MESSAGES = [{"role": "system", "content": "你是一个简洁友好的助手，用中文回答。"}]



# 流式输出
def streaming_output(resp):
    answer_parts = []
    usage = None
    print("助手: ", end="", flush=True)
    for chunk in resp:
        # 最后一个 chunk 只有 usage，没有 choices，要判断一下
        if not chunk.choices:
            usage = chunk.usage
            continue
        delta = chunk.choices[0].delta
        piece = delta.content or ""   # 首个 chunk 可能只有 role 没有 content
        if piece:
            answer_parts.append(piece)
            print(piece, end="", flush=True)  # 边收边打，不换行
    print("\n")  # 整段输出完后换行

    return answer_parts, usage



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


        # 送入模型，执行推理获取返回结果
        try:
            resp = client.chat.completions.create(
                model=MODEL,
                messages=MESSAGES,
                temperature=0.7,
                stream=True,
                # 流式模式下 usage 默认不在 chunk 里返回，
                # 加上这个选项后，最后一个 chunk 会携带 token 统计
                stream_options={"include_usage": True},
            )
        except Exception as e:
            print(f"[出错] {e}\n")
            MESSAGES.pop()  # 失败就把这句 user 撤掉，避免污染上下文
            continue


        # 对于获取到的返回结果进行流式输出
        answer_parts, usage = streaming_output(resp)


        # 将返回的消息加入到上下文中进行后续使用
        answer = "".join(answer_parts)
        MESSAGES.append({"role": "assistant", "content": answer})


        # 输出一下本次的用量
        if usage:
            print(f"  [tokens] prompt={usage.prompt_tokens} "
                f"completion={usage.completion_tokens}\n")






if __name__ == "__main__":
    main()




