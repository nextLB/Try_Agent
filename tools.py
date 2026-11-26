from datetime import datetime
from tools_registry import register_tool


# 获取当前时间
@register_tool(
    name="get_current_time",
    description="获取当前计算机的本地日期和时间"
)
def get_current_time():

    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


# 数学计算器
@register_tool(
    name="calculate",
    description="计算数学表达式，支持加减乘除、乘方"
)
def calculate(expression: str):

    import ast
    import operator

    operators = {
        ast.Add: operator.add,
        ast.Sub: operator.sub,
        ast.Mult: operator.mul,
        ast.Div: operator.truediv,
        ast.Pow: operator.pow,
    }

    def evaluate(node):

        if isinstance(node, ast.Constant):
            if type(node.value) not in (int, float):
                raise ValueError("只允许数字")
            return node.value

        if isinstance(node, ast.BinOp):

            op = operators.get(type(node.op))

            if op is None:
                raise ValueError("不支持的运算符")

            left = evaluate(node.left)
            right = evaluate(node.right)

            if isinstance(node.op, ast.Pow) and abs(right) > 100:
                raise ValueError("指数过大")

            return op(left, right)

        raise ValueError("不支持的表达式")

    tree = ast.parse(expression, mode="eval")

    return evaluate(tree.body)


