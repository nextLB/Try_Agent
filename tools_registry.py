import inspect


# 保存实际 Python 函数
TOOL_MAP = {}

# 保存提供给大模型的工具 Schema
TOOL_SCHEMAS = []


# 注册函数
def register_tool(name, description):

    def decorator(func):

        # 注册 Python 函数
        TOOL_MAP[name] = func

        # 获取函数参数
        signature = inspect.signature(func)

        properties = {}
        required = []

        type_mapping = {
            str: "string",
            int: "integer",
            float: "number",
            bool: "boolean"
        }

        for param_name, param in signature.parameters.items():

            param_type = param.annotation

            if param_type not in type_mapping:
                raise TypeError(
                    f"不支持的参数类型: {param_name}"
                )

            properties[param_name] = {
                "type": type_mapping[param_type],
                "description": param_name
            }

            if param.default is inspect.Parameter.empty:
                required.append(param_name)

        # 生成 Function Calling Schema
        schema = {
            "type": "function",
            "function": {
                "name": name,
                "description": description,
                "parameters": {
                    "type": "object",
                    "properties": properties,
                    "required": required,
                    "additionalProperties": False
                }
            }
        }

        TOOL_SCHEMAS.append(schema)

        print(f"[Registry] 注册工具: {name}")

        return func

    return decorator


# 获取工具Schema
def get_tools():

    return TOOL_SCHEMAS


# 执行工具
def execute_tool(name, arguments):

    import json

    # 检查工具是否存在
    function = TOOL_MAP.get(name)

    if function is None:
        return {
            "ok": False,
            "error": f"未知工具: {name}"
        }

    try:

        # JSON 字符串转换成字典
        if isinstance(arguments, str):
            arguments = json.loads(arguments)

        if not isinstance(arguments, dict):
            raise TypeError("工具参数必须是 JSON 对象")

        # 检查参数是否合法
        inspect.signature(function).bind(**arguments)

        # 执行实际 Python 函数
        result = function(**arguments)

        return {
            "ok": True,
            "result": result
        }

    except Exception as e:

        return {
            "ok": False,
            "error": str(e)
        }


# 查看注册工具
def list_tools():

    return list(TOOL_MAP.keys())


