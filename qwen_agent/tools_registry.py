
"""
tools_registry.py

Agent 工具注册中心

功能：
1. 注册工具函数
2. 保存工具 Schema
3. 向大模型提供工具定义
4. 根据模型返回的工具名称执行对应函数

不使用 Class，全部使用普通函数。
"""

import json


# ============================================================
# 1. 工具注册表
# ============================================================

TOOL_MAP = {}

TOOL_SCHEMAS = []


# ============================================================
# 2. 注册工具
# ============================================================

def register_tool(name, description, parameters):
    """
    工具注册装饰器。

    参数：
        name: 工具名称
        description: 工具描述
        parameters: OpenAI Function Calling 参数 Schema

    使用示例：

        @register_tool(
            name="example",
            description="示例工具",
            parameters={...}
        )
        def example(...):
            ...
    """

    def decorator(func):

        # 注册 Python 函数
        TOOL_MAP[name] = func

        # 注册工具 Schema
        tool_schema = {
            "type": "function",
            "function": {
                "name": name,
                "description": description,
                "parameters": parameters
            }
        }

        TOOL_SCHEMAS.append(tool_schema)

        print(f"[Registry] 注册工具: {name}")

        return func

    return decorator


# ============================================================
# 3. 获取全部工具
# ============================================================

def get_tools():
    """
    获取全部工具的 Schema。

    这个结果会传给大模型的 tools 参数。
    """

    return TOOL_SCHEMAS


# ============================================================
# 4. 获取工具名称
# ============================================================

def list_tools():
    """
    返回当前已经注册的工具名称。
    """

    return list(TOOL_MAP.keys())


# ============================================================
# 5. 执行工具
# ============================================================

def execute_tool(name, arguments):
    """
    根据工具名称执行对应 Python 函数。

    参数：
        name: 工具名称
        arguments: JSON 字符串或者 dict

    返回：
        Python 函数执行结果
    """

    # 判断工具是否存在
    if name not in TOOL_MAP:
        return {
            "success": False,
            "error": f"工具不存在: {name}"
        }

    try:

        # 如果 arguments 是 JSON 字符串，转换为字典
        if isinstance(arguments, str):
            arguments = json.loads(arguments)

        if not isinstance(arguments, dict):
            raise TypeError("工具参数必须是 JSON 对象")

        # 获取对应 Python 函数
        func = TOOL_MAP[name]

        # 执行函数
        result = func(**arguments)

        return result

    except Exception as e:

        return {
            "success": False,
            "error": str(e),
            "tool": name
        }
