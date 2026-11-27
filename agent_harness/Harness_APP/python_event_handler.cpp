
#include "python_event_handler.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPushButton>

#include <cstdio>


// ============================================================
// 处理一条完整的 Python JSON 事件
// ============================================================

void handlePythonEvent(
    const QByteArray &line,
    QPushButton *sendButton,
    QString &assistantResponse,
    int &deltaCount
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


    // JSON 解析失败
    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject()) {

        fprintf(
            stderr,
            "[Harness] Invalid Python JSON event: %s\n",
            parseError.errorString().toUtf8().constData()
        );

        fprintf(
            stderr,
            "[Harness] Raw JSON HEX: %s\n",
            line.toHex(' ').constData()
        );

        fflush(stderr);

        return;
    }


    const QJsonObject event = document.object();

    const QString eventType =
        event.value("type").toString();


    // --------------------------------------------------------
    // 2. Python 后端初始化状态
    // --------------------------------------------------------

    if (eventType == "status") {

        const bool ready =
            event.value("ready").toBool(false);

        const QString message =
            event.value("message").toString();

        const QString model =
            event.value("model").toString();

        sendButton->setEnabled(ready);

        if (ready) {

            fprintf(
                stdout,
                "[Harness] Python backend ready, model: %s\n",
                model.toUtf8().constData()
            );

            fflush(stdout);
        }
        else {

            const QByteArray errorData = message.toUtf8();

            fprintf(
                stderr,
                "[Harness] Python backend initialization failed: "
            );

            fwrite(
                errorData.constData(),
                1,
                static_cast<size_t>(errorData.size()),
                stderr
            );

            fprintf(stderr, "\n");

            fflush(stderr);
        }
    }


    // --------------------------------------------------------
    // 3. 模型开始生成回答
    // --------------------------------------------------------

    else if (eventType == "response_started") {

        assistantResponse.clear();

        deltaCount = 0;

        sendButton->setEnabled(false);

        fprintf(
            stdout,
            "\n[Harness] Model response started:\n"
        );

        fflush(stdout);
    }


    // --------------------------------------------------------
    // 4. 接收模型流式文本片段
    // --------------------------------------------------------

    else if (eventType == "response_delta") {

        const QString deltaText =
            event.value("text").toString();

        if (!deltaText.isEmpty()) {

            assistantResponse += deltaText;

            ++deltaCount;

            const QByteArray utf8Data =
                deltaText.toUtf8();

            // 实时输出模型文本
            fwrite(
                utf8Data.constData(),
                1,
                static_cast<size_t>(utf8Data.size()),
                stdout
            );

            fflush(stdout);
        }
    }


    // --------------------------------------------------------
    // 5. 模型回答完成
    // --------------------------------------------------------

    else if (eventType == "response_done") {

        const QJsonObject usage =
            event.value("usage").toObject();


        fprintf(stdout, "\n");

        fprintf(
            stdout,
            "[Harness] Model response completed\n"
        );


        // 输出 Token 使用情况
        if (!usage.isEmpty()) {

            const int promptTokens =
                usage.value("prompt_tokens").toInt();

            const int completionTokens =
                usage.value("completion_tokens").toInt();

            const int totalTokens =
                usage.value("total_tokens").toInt();

            fprintf(
                stdout,
                "[Tokens] prompt=%d completion=%d total=%d\n",
                promptTokens,
                completionTokens,
                totalTokens
            );
        }


        fprintf(
            stdout,
            "[Harness] Received Delta count: %d\n",
            deltaCount
        );

        fflush(stdout);


        assistantResponse.clear();

        sendButton->setEnabled(true);
    }


    // --------------------------------------------------------
    // 6. Python 或模型请求发生错误
    // --------------------------------------------------------

    else if (eventType == "error") {

        const QString errorMessage =
            event.value("message").toString();

        const QByteArray errorData =
            errorMessage.toUtf8();

        fprintf(
            stderr,
            "\n[Harness] Python/API error: "
        );

        fwrite(
            errorData.constData(),
            1,
            static_cast<size_t>(errorData.size()),
            stderr
        );

        fprintf(stderr, "\n");

        fflush(stderr);


        assistantResponse.clear();

        sendButton->setEnabled(true);
    }


    // --------------------------------------------------------
    // 7. 其他暂未处理的事件
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
