
#ifndef PYTHON_EVENT_HANDLER_H
#define PYTHON_EVENT_HANDLER_H

#include <QByteArray>
#include <QString>

class QLabel;
class QLineEdit;
class QPushButton;
class QTextEdit;


struct BackendUiState {
    QPushButton *connectButton = nullptr;
    QPushButton *disconnectButton = nullptr;
    QPushButton *sendButton = nullptr;
    QLabel *userIdLabel = nullptr;
    QLineEdit *skKeyInput = nullptr;
    int userId = 0;
    QString skKey;
    bool connected = false;
};


void appendChatUserMessage(QTextEdit *chatView, const QString &text);


// 处理一条完整的后端 JSON 事件
void handlePythonEvent(
    const QByteArray &line,
    QTextEdit *chatView,
    BackendUiState &state
);

#endif // PYTHON_EVENT_HANDLER_H
