
#ifndef PYTHON_STDOUT_PARSER_H
#define PYTHON_STDOUT_PARSER_H

#include <QByteArray>

#include "python_event_handler.h"


// 解析后端 JSONL 数据
void processStdoutBuffer(
    QByteArray &buffer,
    QTextEdit *chatView,
    BackendUiState &state,
    bool flushIncompleteLine = false
);

#endif // PYTHON_STDOUT_PARSER_H
