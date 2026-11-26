
"""
tools.py

Agent 工具集合

包含：
1. get_current_time
2. calculate
3. list_directory
4. get_file_info
5. search_files
"""

import os
import ast
import operator
from pathlib import Path
from datetime import datetime

from tools_registry import register_tool


# ============================================================
# 1. 获取当前时间
# ============================================================

@register_tool(
    name="get_current_time",
    description="获取当前系统的日期和时间。",
    parameters={
        "type": "object",
        "properties": {},
        "required": []
    }
)
def get_current_time():

    now = datetime.now()

    return {
        "success": True,
        "datetime": now.strftime("%Y-%m-%d %H:%M:%S"),
        "date": now.strftime("%Y-%m-%d"),
        "time": now.strftime("%H:%M:%S"),
        "weekday": now.strftime("%A")
    }


# ============================================================
# 2. 数学计算
# ============================================================

# 允许使用的数学运算符
_ALLOWED_OPERATORS = {
    ast.Add: operator.add,
    ast.Sub: operator.sub,
    ast.Mult: operator.mul,
    ast.Div: operator.truediv,
    ast.FloorDiv: operator.floordiv,
    ast.Mod: operator.mod,
    ast.Pow: operator.pow,
    ast.USub: operator.neg,
    ast.UAdd: operator.pos
}


def _calculate_node(node):

    if isinstance(node, ast.Constant):

        if isinstance(node.value, bool) or not isinstance(
            node.value, (int, float)
        ):
            raise ValueError("只允许数字")

        return node.value

    if isinstance(node, ast.BinOp):

        operator_type = type(node.op)

        if operator_type not in _ALLOWED_OPERATORS:
            raise ValueError("不支持该运算符")

        left = _calculate_node(node.left)
        right = _calculate_node(node.right)

        # 限制幂运算，防止资源消耗过大
        if isinstance(node.op, ast.Pow) and abs(right) > 1000:
            raise ValueError("指数过大")

        result = _ALLOWED_OPERATORS[operator_type](
            left, right
        )

        if isinstance(result, (int, float)):
            if abs(result) > 1e100:
                raise ValueError("计算结果过大")

        return result

    if isinstance(node, ast.UnaryOp):

        operator_type = type(node.op)

        if operator_type not in _ALLOWED_OPERATORS:
            raise ValueError("不支持该运算符")

        return _ALLOWED_OPERATORS[operator_type](
            _calculate_node(node.operand)
        )

    raise ValueError("表达式包含不允许的内容")


@register_tool(
    name="calculate",
    description="执行基础数学表达式计算，例如加减乘除、取余和幂运算。",
    parameters={
        "type": "object",
        "properties": {
            "expression": {
                "type": "string",
                "description": "需要计算的数学表达式，例如 (10+20)*3"
            }
        },
        "required": ["expression"]
    }
)
def calculate(expression: str):

    try:

        if len(expression) > 200:
            raise ValueError("表达式过长")

        tree = ast.parse(expression, mode="eval")

        result = _calculate_node(tree.body)

        return {
            "success": True,
            "expression": expression,
            "result": result
        }

    except Exception as e:

        return {
            "success": False,
            "expression": expression,
            "error": str(e)
        }


# ============================================================
# 3. 查看目录
# ============================================================

@register_tool(
    name="list_directory",
    description="查看本地指定目录下的文件和子目录。",
    parameters={
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "description": "需要查看的本地目录路径"
            },
            "max_items": {
                "type": "integer",
                "description": "最多返回多少个项目，默认100"
            }
        },
        "required": ["path"]
    }
)
def list_directory(path, max_items=100):

    directory = Path(path).expanduser()

    if not directory.exists():
        return {
            "success": False,
            "error": "目录不存在",
            "path": str(directory)
        }

    if not directory.is_dir():
        return {
            "success": False,
            "error": "指定路径不是目录",
            "path": str(directory)
        }

    max_items = max(1, min(int(max_items), 500))

    items = []

    with os.scandir(directory) as entries:

        for entry in entries:

            if len(items) >= max_items:
                break

            try:

                if entry.is_dir(follow_symlinks=False):
                    item_type = "directory"
                    size = None

                elif entry.is_file(follow_symlinks=False):
                    item_type = "file"
                    size = entry.stat(
                        follow_symlinks=False
                    ).st_size

                else:
                    item_type = "other"
                    size = None

                items.append({
                    "name": entry.name,
                    "path": entry.path,
                    "type": item_type,
                    "size_bytes": size
                })

            except OSError:
                continue

    items.sort(
        key=lambda x: (
            x["type"] != "directory",
            x["name"].lower()
        )
    )

    return {
        "success": True,
        "path": str(directory.resolve()),
        "total_returned": len(items),
        "items": items
    }


# ============================================================
# 4. 获取文件信息
# ============================================================

@register_tool(
    name="get_file_info",
    description="获取本地文件或文件夹的详细信息。",
    parameters={
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "description": "文件或文件夹的完整路径"
            }
        },
        "required": ["path"]
    }
)
def get_file_info(path):

    file_path = Path(path).expanduser()

    if not file_path.exists():
        return {
            "success": False,
            "error": "路径不存在",
            "path": str(file_path)
        }

    stat = file_path.stat()
    is_directory = file_path.is_dir()

    return {
        "success": True,
        "name": file_path.name,
        "path": str(file_path.resolve()),
        "type": "directory" if is_directory else "file",
        "extension": file_path.suffix.lower() if not is_directory else None,
        "size_bytes": stat.st_size,
        "size_mb": round(stat.st_size / (1024 * 1024), 3),
        "created_time": datetime.fromtimestamp(stat.st_ctime).isoformat(),
        "modified_time": datetime.fromtimestamp(stat.st_mtime).isoformat()
    }


# ============================================================
# 5. 搜索文件
# ============================================================

@register_tool(
    name="search_files",
    description="在指定目录中搜索文件，支持文件名关键字和扩展名。",
    parameters={
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "description": "搜索起始目录"
            },
            "keyword": {
                "type": "string",
                "description": "文件名关键字，可以为空"
            },
            "extension": {
                "type": "string",
                "description": "扩展名，例如 jpg、png、pdf"
            },
            "recursive": {
                "type": "boolean",
                "description": "是否递归搜索子目录"
            },
            "max_results": {
                "type": "integer",
                "description": "最多返回多少个结果"
            }
        },
        "required": ["path"]
    }
)
def search_files(
    path,
    keyword="",
    extension="",
    recursive=True,
    max_results=100
):

    root = Path(path).expanduser()

    if not root.exists():
        return {
            "success": False,
            "error": "搜索目录不存在"
        }

    if not root.is_dir():
        return {
            "success": False,
            "error": "搜索路径不是目录"
        }

    keyword = str(keyword).strip().lower()
    extension = str(extension).strip().lower()

    if extension and not extension.startswith("."):
        extension = "." + extension

    max_results = max(1, min(int(max_results), 1000))

    iterator = root.rglob("*") if recursive else root.iterdir()

    results = []

    for file_path in iterator:

        try:

            if not file_path.is_file():
                continue

            if keyword and keyword not in file_path.name.lower():
                continue

            if extension and file_path.suffix.lower() != extension:
                continue

            stat = file_path.stat()

            results.append({
                "name": file_path.name,
                "path": str(file_path.resolve()),
                "extension": file_path.suffix.lower(),
                "size_bytes": stat.st_size
            })

            if len(results) >= max_results:
                break

        except OSError:
            continue

    return {
        "success": True,
        "search_path": str(root.resolve()),
        "total_returned": len(results),
        "results": results
    }
