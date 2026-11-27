
#ifndef PYTHON_STDOUT_PARSER_H
#define PYTHON_STDOUT_PARSER_H

#include <QByteArray>
#include <QString>

class QPushButton;


// 解析 Python stdout 中的 JSONL 数据
void processStdoutBuffer(
    QByteArray &buffer,
    QPushButton *sendButton,
    QString &assistantResponse,
    int &deltaCount,
    bool flushIncompleteLine = false
);

#endif // PYTHON_STDOUT_PARSER_H
