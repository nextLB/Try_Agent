
#include "python_stdout_parser.h"

#include "python_event_handler.h"

#include <QPushButton>


// ============================================================
// 解析 Python stdout 中的 JSONL 数据
// ============================================================

void processStdoutBuffer(
    QByteArray &buffer,
    QPushButton *sendButton,
    QString &assistantResponse,
    int &deltaCount,
    bool flushIncompleteLine
)
{
    // --------------------------------------------------------
    // 1. 持续查找换行符
    // --------------------------------------------------------

    while (true) {

        const qsizetype newlineIndex =
            buffer.indexOf('\n');

        // 当前缓冲区还没有完整的一行 JSON
        if (newlineIndex < 0) {
            break;
        }


        // ----------------------------------------------------
        // 2. 提取一条完整 JSONL 数据
        // ----------------------------------------------------

        const QByteArray line =
            buffer.left(newlineIndex).trimmed();


        // 从缓冲区删除已经读取的数据
        buffer.remove(0, newlineIndex + 1);


        // ----------------------------------------------------
        // 3. 将完整数据交给事件处理模块
        // ----------------------------------------------------

        handlePythonEvent(
            line,
            sendButton,
            assistantResponse,
            deltaCount
        );
    }


    // --------------------------------------------------------
    // 4. 处理 Python 进程退出时的残余数据
    // --------------------------------------------------------

    if (flushIncompleteLine &&
        !buffer.trimmed().isEmpty()) {

        const QByteArray lastLine =
            buffer.trimmed();

        buffer.clear();


        handlePythonEvent(
            lastLine,
            sendButton,
            assistantResponse,
            deltaCount
        );
    }
}
