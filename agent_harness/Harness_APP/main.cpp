#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMainWindow>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
    // 初始化应用程序深色主题
    void applyDarkTheme(QApplication &app)
    {
        app.setStyle(QStyleFactory::create("Fusion"));

        QPalette palette;
        palette.setColor(QPalette::Window, QColor("#0B1016"));
        palette.setColor(QPalette::WindowText, QColor("#E6EDF3"));
        palette.setColor(QPalette::Base, QColor("#111923"));
        palette.setColor(QPalette::AlternateBase, QColor("#17212C"));
        palette.setColor(QPalette::ToolTipBase, QColor("#18232E"));
        palette.setColor(QPalette::ToolTipText, QColor("#E6EDF3"));
        palette.setColor(QPalette::Text, QColor("#DCE5EE"));
        palette.setColor(QPalette::Button, QColor("#1A2632"));
        palette.setColor(QPalette::ButtonText, QColor("#E6EDF3"));
        palette.setColor(QPalette::BrightText, QColor("#FF6B7A"));
        palette.setColor(QPalette::Highlight, QColor("#54CDB7"));
        palette.setColor(QPalette::HighlightedText, QColor("#071411"));
        palette.setColor(QPalette::Link, QColor("#69D8C4"));
        palette.setColor(QPalette::PlaceholderText, QColor("#778595"));
        app.setPalette(palette);

        QFont appFont("Segoe UI");
        appFont.setPointSize(10);
        app.setFont(appFont);

        app.setStyleSheet(R"(
            QToolTip {
                color: #E6EDF3;
                background-color: #18232E;
                border: 1px solid #344454;
                padding: 5px 8px;
                border-radius: 6px;
            }

            QMainWindow#mainWindow,
            QWidget#rootSurface {
                background-color: #0B1016;
            }

            QWidget#homeCanvas {
                background: qlineargradient(
                    x1: 0, y1: 0, x2: 1, y2: 1,
                    stop: 0 #141E29,
                    stop: 1 #101720
                );
                border: 1px solid #273442;
                border-radius: 14px;
            }

            QWidget#composerPanel {
                background-color: #111923;
                border: 1px solid #344454;
                border-radius: 14px;
            }

            QPlainTextEdit#messageInput {
                color: #DCE5EE;
                background-color: transparent;
                border: none;
                selection-background-color: #276B60;
                padding: 4px;
            }

            QPushButton#sendButton {
                color: #071411;
                background-color: #54CDB7;
                border: none;
                border-radius: 8px;
                padding: 9px 22px;
                font-weight: 600;
            }

            QPushButton#sendButton:hover {
                background-color: #69D8C4;
            }

            QPushButton#sendButton:pressed {
                background-color: #3EAD9A;
            }

            QPushButton#sendButton:disabled {
                color: #718078;
                background-color: #293B3A;
            }
        )");
    }


    // 处理一条完整的 Python JSON 事件
    void handlePythonEvent(const QByteArray &line,
                           QPushButton *sendButton,
                           QString &assistantResponse)
    {
        if (line.trimmed().isEmpty()) {
            return;
        }

        QJsonParseError parseError;

        const QJsonDocument document =
            QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError ||
            !document.isObject()) {

            qWarning().noquote()
                << "[Harness] Python 返回了无效 JSON 事件："
                << parseError.errorString();

            // 输出原始 HEX，方便检查编码和传输数据
            qWarning().noquote()
                << "[Harness] 原始数据 HEX："
                << line.toHex(' ');

            return;
        }

        const QJsonObject event = document.object();

        const QString eventType =
            event.value("type").toString();

        // Python 后端初始化状态
        if (eventType == "status") {

            const bool ready =
                event.value("ready").toBool(false);

            const QString message =
                event.value("message").toString();

            const QString model =
                event.value("model").toString();

            sendButton->setEnabled(ready);

            if (ready) {
                qInfo().noquote()
                    << "[Harness] Python 后端已就绪，模型："
                    << model;
            }
            else {
                qWarning().noquote()
                    << "[Harness] Python 后端初始化失败："
                    << message;
            }
        }

        // 模型开始生成回答
        else if (eventType == "response_started") {

            assistantResponse.clear();

            sendButton->setEnabled(false);

            qInfo().noquote()
                << "[Harness] 模型开始回复：";
        }

        // 接收模型流式文本片段
        else if (eventType == "response_delta") {

            assistantResponse +=
                event.value("text").toString();
        }

        // 模型回答完成
        else if (eventType == "response_done") {

            // 优先使用事件携带的完整回答
            QString finalText =
                event.value("text").toString();

            if (finalText.isEmpty()) {
                finalText = assistantResponse;
            }

            qInfo().noquote()
                << "[Harness] 模型回复：\n"
                << finalText;

            // 获取 Token 使用量
            const QJsonObject usage =
                event.value("usage").toObject();

            if (!usage.isEmpty()) {

                qInfo().noquote()
                    << "[Tokens] prompt="
                    << usage.value("prompt_tokens").toInt()
                    << " completion="
                    << usage.value("completion_tokens").toInt()
                    << " total="
                    << usage.value("total_tokens").toInt();
            }

            assistantResponse.clear();

            sendButton->setEnabled(true);
        }

        // Python 或模型请求发生错误
        else if (eventType == "error") {

            qWarning().noquote()
                << "[Harness] Python/API 错误："
                << event.value("message").toString();

            assistantResponse.clear();

            sendButton->setEnabled(true);
        }

        // 其他暂未处理的事件
        else {

            qInfo().noquote()
                << "[Harness] 收到其他事件："
                << eventType;
        }
    }


    // 将 stdout 缓冲区中的完整 JSONL 行逐条取出
    void processStdoutBuffer(QByteArray &buffer,
                             QPushButton *sendButton,
                             QString &assistantResponse,
                             bool flushIncompleteLine = false)
    {
        while (true) {

            const qsizetype newlineIndex =
                buffer.indexOf('\n');

            if (newlineIndex < 0) {
                break;
            }

            const QByteArray line =
                buffer.left(newlineIndex).trimmed();

            buffer.remove(0, newlineIndex + 1);

            handlePythonEvent(
                line,
                sendButton,
                assistantResponse
            );
        }

        // 子进程退出时处理可能没有换行符结尾的最后一段数据
        if (flushIncompleteLine && !buffer.trimmed().isEmpty()) {

            const QByteArray lastLine =
                buffer.trimmed();

            buffer.clear();

            handlePythonEvent(
                lastLine,
                sendButton,
                assistantResponse
            );
        }
    }
}


int main(int argc, char *argv[])
{
    // 创建 Qt 应用程序并初始化主题
    QApplication app(argc, argv);

    app.setApplicationName("Harness");
    app.setOrganizationName("PersonalProjects");

    applyDarkTheme(app);


    // 创建主窗口
    QMainWindow window;

    window.setObjectName("mainWindow");
    window.setWindowTitle("Harness");
    window.resize(1280, 800);
    window.setMinimumSize(900, 600);


    // 创建主窗口根容器和外层布局
    auto *rootSurface = new QWidget(&window);

    rootSurface->setObjectName("rootSurface");

    rootSurface->setAttribute(
        Qt::WA_StyledBackground,
        true
    );

    auto *rootLayout = new QVBoxLayout(rootSurface);

    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(0);


    // 创建首页中央区域和垂直布局
    auto *homeCanvas = new QWidget(rootSurface);

    homeCanvas->setObjectName("homeCanvas");

    homeCanvas->setAttribute(
        Qt::WA_StyledBackground,
        true
    );

    auto *homeLayout = new QVBoxLayout(homeCanvas);

    homeLayout->setContentsMargins(28, 28, 28, 24);
    homeLayout->setSpacing(0);

    homeLayout->addStretch(1);


    // 创建消息输入面板
    auto *composerPanel = new QWidget(homeCanvas);

    composerPanel->setObjectName("composerPanel");

    composerPanel->setAttribute(
        Qt::WA_StyledBackground,
        true
    );

    composerPanel->setMaximumWidth(900);

    composerPanel->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
    );

    auto *composerLayout = new QVBoxLayout(composerPanel);

    composerLayout->setContentsMargins(16, 12, 16, 12);
    composerLayout->setSpacing(10);


    // 创建多行文本输入框
    auto *messageInput = new QPlainTextEdit(composerPanel);

    messageInput->setObjectName("messageInput");
    messageInput->setPlaceholderText("输入消息……");
    messageInput->setFixedHeight(82);
    messageInput->setTabChangesFocus(false);

    composerLayout->addWidget(messageInput);


    // 创建发送按钮
    auto *buttonLayout = new QHBoxLayout();

    buttonLayout->addStretch(1);

    auto *sendButton = new QPushButton("发送", composerPanel);

    sendButton->setObjectName("sendButton");
    sendButton->setCursor(Qt::PointingHandCursor);
    sendButton->setMinimumWidth(88);
    sendButton->setFixedHeight(36);

    // Python 后端初始化完成后才允许发送
    sendButton->setEnabled(false);

    buttonLayout->addWidget(sendButton);

    composerLayout->addLayout(buttonLayout);


    // 将输入面板放置在页面底部并保持居中
    auto *composerRow = new QHBoxLayout();

    composerRow->addStretch(1);
    composerRow->addWidget(composerPanel);
    composerRow->addStretch(1);

    homeLayout->addLayout(composerRow);


    // 将首页区域设置为主窗口中央内容
    rootLayout->addWidget(homeCanvas);

    window.setCentralWidget(rootSurface);


    // 根据 main.exe 的位置定位同级项目中的 Agent/main.py
    const QString appDir =
        QCoreApplication::applicationDirPath();

    const QString agentDir = QDir::cleanPath(
        QDir(appDir).absoluteFilePath("../../Agent")
    );

    const QString pythonScript = QDir::cleanPath(
        QDir(agentDir).filePath("main.py")
    );


    // 创建 Python 子进程
    auto *py = new QProcess(&app);

    py->setWorkingDirectory(agentDir);

    py->setProcessChannelMode(
        QProcess::SeparateChannels
    );


    // 设置 Python 子进程的 UTF-8 环境
    QProcessEnvironment processEnvironment =
        QProcessEnvironment::systemEnvironment();

    processEnvironment.insert("PYTHONUTF8", "1");
    processEnvironment.insert("PYTHONIOENCODING", "utf-8");

    py->setProcessEnvironment(processEnvironment);


    // 保存 Python stdout 未接收完整的一行数据及模型回答
    QByteArray pythonStdoutBuffer;

    QString currentAssistantResponse;


    // 接收 Python stdout，并按 JSONL 换行边界解析
    QObject::connect(
        py,
        &QProcess::readyReadStandardOutput,
        &window,
        [py,
         sendButton,
         &pythonStdoutBuffer,
         &currentAssistantResponse]() {

            pythonStdoutBuffer +=
                py->readAllStandardOutput();

            processStdoutBuffer(
                pythonStdoutBuffer,
                sendButton,
                currentAssistantResponse
            );
        }
    );


    // 接收 Python stderr，显示底层异常信息
    QObject::connect(
        py,
        &QProcess::readyReadStandardError,
        &window,
        [py]() {

            const QByteArray output =
                py->readAllStandardError();

            if (!output.isEmpty()) {

                qWarning().noquote().nospace()
                    << "[Python stderr] "
                    << QString::fromUtf8(output).trimmed();
            }
        }
    );


    // Python 解释器启动后等待后端初始化状态
    QObject::connect(
        py,
        &QProcess::started,
        &window,
        []() {

            qInfo().noquote()
                << "[Harness] Python 进程已启动，等待后端初始化……";
        }
    );


    // 输出 Python 子进程启动或运行错误
    QObject::connect(
        py,
        &QProcess::errorOccurred,
        &window,
        [py, sendButton](QProcess::ProcessError) {

            sendButton->setEnabled(false);

            qWarning().noquote()
                << "[Harness] Python 子进程错误："
                << py->errorString();
        }
    );


    // Python 退出时处理剩余输出并停止发送
    QObject::connect(
        py,
        &QProcess::finished,
        &window,
        [py,
         sendButton,
         &pythonStdoutBuffer,
         &currentAssistantResponse](
            int exitCode,
            QProcess::ExitStatus exitStatus
        ) {

            pythonStdoutBuffer +=
                py->readAllStandardOutput();

            processStdoutBuffer(
                pythonStdoutBuffer,
                sendButton,
                currentAssistantResponse,
                true
            );

            const QByteArray errorOutput =
                py->readAllStandardError();

            if (!errorOutput.isEmpty()) {

                qWarning().noquote().nospace()
                    << "[Python stderr] "
                    << QString::fromUtf8(errorOutput).trimmed();
            }

            sendButton->setEnabled(false);

            qInfo().noquote()
                << "[Harness] Python 子进程已退出，exitCode="
                << exitCode
                << "，正常退出="
                << (exitStatus == QProcess::NormalExit);
        }
    );


    // 检查 Python 脚本和当前 Conda 环境中的解释器
    const QFileInfo scriptInfo(pythonScript);

    const QString pythonProgram =
        QStandardPaths::findExecutable("python");


    if (!scriptInfo.exists() || !scriptInfo.isFile()) {

        qCritical().noquote()
            << "[Harness] 找不到 Python 脚本："
            << pythonScript;
    }

    else if (pythonProgram.isEmpty()) {

        qCritical().noquote()
            << "[Harness] 找不到 python.exe，请先激活 Agent Conda 环境。";
    }

    else {

        // 设置 Python 解释器、脚本路径和工作目录
        py->setProgram(pythonProgram);

        py->setArguments({
            "-u",
            pythonScript
        });

        // 输出启动信息并启动 Python 子进程
        qInfo().noquote()
            << "[Harness] Python 解释器："
            << pythonProgram;

        qInfo().noquote()
            << "[Harness] Python 脚本："
            << pythonScript;

        qInfo().noquote()
            << "[Harness] Agent 工作目录："
            << agentDir;

        py->start();
    }


    // 点击发送后将 UTF-8 JSON 行写入 Python stdin
    QObject::connect(
        sendButton,
        &QPushButton::clicked,
        &window,
        [messageInput, py, sendButton]() {

            const QString message =
                messageInput->toPlainText().trimmed();

            if (message.isEmpty()) {

                messageInput->setFocus();
                return;
            }

            if (py->state() != QProcess::Running) {

                qWarning().noquote()
                    << "[Harness] Python 子进程尚未运行。";

                return;
            }

            // 创建 JSON 消息并序列化为 UTF-8 字节
            QJsonObject messageObject;

            messageObject.insert("text", message);

            const QByteArray jsonData =
                QJsonDocument(messageObject).toJson(
                    QJsonDocument::Compact
                );

            // 添加换行符，作为一条 JSONL 消息的结束标记
            const QByteArray payload = jsonData + '\n';

            // 将 JSON 数据写入 Python stdin
            const qint64 bytesWritten =
                py->write(payload);

            if (bytesWritten < 0) {

                qWarning().noquote()
                    << "[Harness] 写入 Python 失败："
                    << py->errorString();

                return;
            }

            // 输出发送日志并等待当前模型请求完成
            qInfo().noquote()
                << "[Harness] 已发送 JSON："
                << QString::fromUtf8(jsonData);

            messageInput->clear();
            messageInput->setFocus();

            sendButton->setEnabled(false);
        }
    );


    // 退出 Qt 应用时关闭 Python stdin 并回收子进程
    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        py,
        [py]() {

            if (py->state() != QProcess::NotRunning) {

                py->closeWriteChannel();

                if (!py->waitForFinished(2000)) {

                    py->terminate();

                    if (!py->waitForFinished(1000)) {

                        py->kill();
                        py->waitForFinished(1000);
                    }
                }
            }
        }
    );


    // 显示主窗口并进入 Qt 事件循环
    window.show();

    return app.exec();
}