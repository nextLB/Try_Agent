
#include "python_event_handler.h"

#include <QColor>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>

#include <cstdio>


static void insertChatText(
    QTextEdit *chatView,
    const QString &text,
    const QString &color,
    int fontWeight
)
{
    QTextCursor cursor(chatView->document());

    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;

    format.setForeground(QColor(color));
    format.setFontWeight(fontWeight);

    cursor.insertText(text, format);

    chatView->setTextCursor(cursor);
    chatView->ensureCursorVisible();
}


static void finishChatAssistant(QTextEdit *chatView)
{
    if (!chatView) {
        return;
    }

    if (chatView->property("assistantStarted").toBool()) {
        insertChatText(chatView, "\n\n", "#DCE5EE", QFont::Normal);
        chatView->setProperty("assistantStarted", false);
    }
}


static void appendChatAssistantDelta(
    QTextEdit *chatView,
    const QString &text
)
{
    if (!chatView || text.isEmpty()) {
        return;
    }

    if (!chatView->property("assistantStarted").toBool()) {
        chatView->setProperty("assistantStarted", true);
        insertChatText(chatView, "助手：", "#54CDB7", QFont::Bold);
    }

    insertChatText(chatView, text, "#DCE5EE", QFont::Normal);
}


static void appendChatError(
    QTextEdit *chatView,
    const QString &message
)
{
    if (!chatView) {
        return;
    }

    finishChatAssistant(chatView);

    insertChatText(
        chatView,
        "失败：" + message + "\n\n",
        "#FF6B7A",
        QFont::Normal
    );
}


static void appendChatNotice(
    QTextEdit *chatView,
    const QString &text,
    const QString &color
)
{
    if (!chatView) {
        return;
    }

    finishChatAssistant(chatView);

    insertChatText(chatView, text + "\n\n", color, QFont::Normal);
}


static void setDisconnectedUi(
    QTextEdit *chatView,
    BackendUiState &state,
    const QString &notice
)
{
    state.connected = false;
    state.userId = 0;
    state.skKey.clear();
    state.model.clear();

    if (state.connectButton) {
        state.connectButton->setEnabled(true);
    }

    if (state.disconnectButton) {
        state.disconnectButton->setEnabled(false);
    }

    if (state.sendButton) {
        state.sendButton->setEnabled(false);
    }

    if (state.userIdLabel) {
        state.userIdLabel->setText("未连接");
    }

    appendChatNotice(chatView, notice, "#778595");
}


void appendChatUserMessage(QTextEdit *chatView, const QString &text)
{
    if (!chatView || text.trimmed().isEmpty()) {
        return;
    }

    chatView->setProperty("assistantStarted", false);

    insertChatText(chatView, "你：", "#69D8C4", QFont::Bold);
    insertChatText(chatView, text + "\n\n", "#E6EDF3", QFont::Normal);
}


// ============================================================
// 处理一条完整的后端 JSON 事件
// ============================================================

void handlePythonEvent(
    const QByteArray &line,
    QTextEdit *chatView,
    BackendUiState &state
)
{
    if (line.trimmed().isEmpty()) {
        return;
    }


    // --------------------------------------------------------
    // 1. JSON 数据解析
    // --------------------------------------------------------

    QJsonParseError parseError;

    const QJsonDocument document =
        QJsonDocument::fromJson(line, &parseError);


    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject()) {

        fprintf(
            stderr,
            "[Harness] Invalid backend JSON event: %s\n",
            parseError.errorString().toUtf8().constData()
        );

        fflush(stderr);

        return;
    }


    const QJsonObject event = document.object();

    const QString eventType =
        event.value("type").toString();


    // --------------------------------------------------------
    // 2. 连接建立
    // --------------------------------------------------------

    if (eventType == "connected") {

        state.connected = true;
        state.userId = event.value("user_id").toInt(0);
        state.model = event.value("model").toString();

        if (state.connectButton) {
            state.connectButton->setEnabled(false);
        }

        if (state.disconnectButton) {
            state.disconnectButton->setEnabled(true);
        }

        if (state.sendButton) {
            state.sendButton->setEnabled(true);
        }

        if (state.userIdLabel) {
            state.userIdLabel->setText(
                QString("已连接 ID: %1 · %2")
                    .arg(state.userId)
                    .arg(state.model)
            );
        }

        appendChatNotice(
            chatView,
            QString("已连接，ID=%1，模型=%2")
                .arg(state.userId)
                .arg(state.model),
            "#69D8C4"
        );

        fprintf(
            stdout,
            "[Harness] Connected, user_id=%d\n",
            state.userId
        );

        fflush(stdout);
    }


    // --------------------------------------------------------
    // 3. 连接失败
    // --------------------------------------------------------

    else if (eventType == "connect_failed") {

        const QString message =
            event.value("message").toString();

        setDisconnectedUi(
            chatView,
            state,
            "连接失败：" + message
        );

        fprintf(stderr, "[Harness] Connect failed\n");

        fflush(stderr);
    }


    // --------------------------------------------------------
    // 4. 连接断开
    // --------------------------------------------------------

    else if (eventType == "disconnected") {

        setDisconnectedUi(chatView, state, "已断开连接");

        fprintf(stdout, "[Harness] Disconnected\n");

        fflush(stdout);
    }


    // --------------------------------------------------------
    // 5. 模型开始生成回答
    // --------------------------------------------------------

    else if (eventType == "response_started") {

        if (state.sendButton) {
            state.sendButton->setEnabled(false);
        }

        if (chatView) {
            chatView->setProperty("assistantStarted", false);
        }
    }


    // --------------------------------------------------------
    // 6. 接收模型流式文本片段
    // --------------------------------------------------------

    else if (eventType == "response_delta") {

        const int eventUserId =
            event.value("user_id").toInt(-1);

        if (eventUserId != state.userId) {
            return;
        }

        appendChatAssistantDelta(
            chatView,
            event.value("text").toString()
        );
    }


    // --------------------------------------------------------
    // 7. 模型回答完成
    // --------------------------------------------------------

    else if (eventType == "response_done") {

        finishChatAssistant(chatView);

        fprintf(stdout, "[Harness] Response succeeded\n");

        fflush(stdout);

        if (state.connected && state.sendButton) {
            state.sendButton->setEnabled(true);
        }
    }


    // --------------------------------------------------------
    // 8. 后端或模型请求发生错误
    // --------------------------------------------------------

    else if (eventType == "error") {

        appendChatError(
            chatView,
            event.value("message").toString()
        );

        fprintf(stderr, "[Harness] Response failed\n");

        fflush(stderr);

        if (state.connected && state.sendButton) {
            state.sendButton->setEnabled(true);
        }
    }


    // --------------------------------------------------------
    // 9. 其他暂未处理的事件
    // --------------------------------------------------------

    else {

        fprintf(
            stdout,
            "[Harness] Unknown event: %s\n",
            eventType.toUtf8().constData()
        );

        fflush(stdout);
    }
}
