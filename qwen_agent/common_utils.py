
from tools_registry import execute_tool
import json
import base64
import mimetypes
from pathlib import Path




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




# 将本地文件编码为 Base64
def encode_file_base64(file_path):

    path = Path(file_path).expanduser()

    # 检查文件是否存在
    if not path.is_file():
        raise FileNotFoundError(f"文件不存在：{path}")

    # 读取文件并编码
    file_data = path.read_bytes()

    return path, base64.b64encode(file_data).decode("ascii")


# 构建用户消息
def build_user_message(text=None, image=None, audio=None, video=None):

    content = []

    # ==========================================
    # 1. 文本
    # ==========================================
    if text:
        content.append({
            "type": "text",
            "text": text
        })

    # ==========================================
    # 2. 图片
    # ==========================================
    if image:

        path, encoded_data = encode_file_base64(image)

        mime_type = mimetypes.guess_type(path.name)[0]

        if not mime_type or not mime_type.startswith("image/"):
            raise ValueError("无法识别图片格式")

        content.append({
            "type": "image_url",
            "image_url": {
                "url": f"data:{mime_type};base64,{encoded_data}"
            }
        })

    # ==========================================
    # 3. 音频
    # ==========================================
    if audio:

        path, encoded_data = encode_file_base64(audio)

        audio_format = path.suffix.lstrip(".").lower()

        if not audio_format:
            raise ValueError("无法识别音频文件格式")

        content.append({
            "type": "input_audio",
            "input_audio": {
                "data": f"data:;base64,{encoded_data}",
                "format": audio_format
            }
        })

    # ==========================================
    # 4. 视频
    # ==========================================
    if video:

        path, encoded_data = encode_file_base64(video)

        # 百炼文档规定本地视频 Base64 编码后小于 10MB
        if len(encoded_data) >= 10 * 1024 * 1024:
            raise ValueError(
                "本地视频 Base64 数据不能达到 10MB，"
                "请使用可访问的视频 URL"
            )

        content.append({
            "type": "video_url",
            "video_url": {
                "url": f"data:;base64,{encoded_data}"
            }
        })

    # 没有任何有效输入
    if not content:
        return None

    # 返回用户消息
    return {
        "role": "user",
        "content": content
    }


