
#ifndef PYTHON_EVENT_HANDLER_H
#define PYTHON_EVENT_HANDLER_H

#include <QByteArray>
#include <QString>

class QPushButton;


// 处理一条完整的 Python JSON 事件
void handlePythonEvent(
    const QByteArray &line,
    QPushButton *sendButton,
    QString &assistantResponse,
    int &deltaCount
);

#endif // PYTHON_EVENT_HANDLER_H
