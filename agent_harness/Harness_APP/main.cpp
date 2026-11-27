
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdio>

// 导入自定义模块
#include "ui_theme.h"
#include "python_event_handler.h"
#include "python_stdout_parser.h"


// ============================================================
// 1. 程序入口
// ============================================================

int main(int argc, char *argv[])
{
    // --------------------------------------------------------
    // 1.1 创建 Qt 应用程序
    // --------------------------------------------------------

    QApplication app(argc, argv);

    app.setApplicationName("Harness");
    app.setOrganizationName("PersonalProjects");


    // --------------------------------------------------------
    // 1.2 初始化 UI 主题
    // --------------------------------------------------------

    applyDarkTheme(app);


    // ========================================================
    // 2. 创建主窗口
    // ========================================================

    QMainWindow window;

    window.setObjectName("mainWindow");
    window.setWindowTitle("Harness");
    window.resize(1280, 800);
    window.setMinimumSize(900, 600);


    // --------------------------------------------------------
    // 2.1 创建根容器
    // --------------------------------------------------------

    auto *rootSurface = new QWidget(&window);

    rootSurface->setObjectName("rootSurface");

    rootSurface->setAttribute(
        Qt::WA_StyledBackground,
        true
    );

    auto *rootLayout = new QVBoxLayout(rootSurface);

    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(0);


    // --------------------------------------------------------
    // 2.2 创建首页中央区域
    // --------------------------------------------------------

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


    // --------------------------------------------------------
    // 2.3 创建消息输入面板
    // --------------------------------------------------------

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


    // --------------------------------------------------------
    // 2.4 创建多行文本输入框
    // --------------------------------------------------------

    auto *messageInput = new QPlainTextEdit(composerPanel);

    messageInput->setObjectName("messageInput");
    messageInput->setPlaceholderText("输入消息……");
    messageInput->setFixedHeight(82);
    messageInput->setTabChangesFocus(false);

    composerLayout->addWidget(messageInput);


    // --------------------------------------------------------
    // 2.5 创建发送按钮
    // --------------------------------------------------------

    auto *buttonLayout = new QHBoxLayout();

    buttonLayout->addStretch(1);

    auto *sendButton = new QPushButton("发送", composerPanel);

    sendButton->setObjectName("sendButton");
    sendButton->setCursor(Qt::PointingHandCursor);
    sendButton->setMinimumWidth(88);
    sendButton->setFixedHeight(36);

    sendButton->setEnabled(false);

    buttonLayout->addWidget(sendButton);

    composerLayout->addLayout(buttonLayout);


    // --------------------------------------------------------
    // 2.6 输入面板居中
    // --------------------------------------------------------

    auto *composerRow = new QHBoxLayout();

    composerRow->addStretch(1);
    composerRow->addWidget(composerPanel);
    composerRow->addStretch(1);

    homeLayout->addLayout(composerRow);

    rootLayout->addWidget(homeCanvas);

    window.setCentralWidget(rootSurface);


    // ========================================================
    // 3. 定位 Python Agent 项目
    // ========================================================

    const QString appDir =
        QCoreApplication::applicationDirPath();

    const QString agentDir = QDir::cleanPath(
        QDir(appDir).absoluteFilePath("../../Agent")
    );

    const QString pythonScript = QDir::cleanPath(
        QDir(agentDir).filePath("main.py")
    );


    // ========================================================
    // 4. 创建 Python 子进程
    // ========================================================

    auto *py = new QProcess(&app);

    py->setWorkingDirectory(agentDir);

    py->setProcessChannelMode(
        QProcess::SeparateChannels
    );


    // --------------------------------------------------------
    // 4.1 设置 Python UTF-8 环境
    // --------------------------------------------------------

    QProcessEnvironment processEnvironment =
        QProcessEnvironment::systemEnvironment();

    processEnvironment.insert("PYTHONUTF8", "1");
    processEnvironment.insert("PYTHONIOENCODING", "utf-8");

    py->setProcessEnvironment(processEnvironment);


    // --------------------------------------------------------
    // 4.2 Python 数据缓冲区和 Agent 响应状态
    // --------------------------------------------------------

    QByteArray pythonStdoutBuffer;

    QString currentAssistantResponse;

    int currentDeltaCount = 0;


    // ========================================================
    // 5. 接收 Python stdout
    // ========================================================

    QObject::connect(
        py,
        &QProcess::readyReadStandardOutput,
        &window,
        [py,
         sendButton,
         &pythonStdoutBuffer,
         &currentAssistantResponse,
         &currentDeltaCount]() {

            // 读取 Python 当前可用的 stdout 数据
            pythonStdoutBuffer +=
                py->readAllStandardOutput();


            // 交给 JSONL 数据解析模块
            processStdoutBuffer(
                pythonStdoutBuffer,
                sendButton,
                currentAssistantResponse,
                currentDeltaCount
            );
        }
    );


    // ========================================================
    // 6. 接收 Python stderr
    // ========================================================

    QObject::connect(
        py,
        &QProcess::readyReadStandardError,
        &window,
        [py]() {

            const QByteArray output =
                py->readAllStandardError();

            if (!output.isEmpty()) {

                // 当前仅用于终端调试
                const QByteArray prefix =
                    "[Python stderr] ";

                fwrite(
                    prefix.constData(),
                    1,
                    static_cast<size_t>(prefix.size()),
                    stderr
                );

                fwrite(
                    output.constData(),
                    1,
                    static_cast<size_t>(output.size()),
                    stderr
                );

                fflush(stderr);
            }
        }
    );


    // ========================================================
    // 7. Python 进程生命周期管理
    // ========================================================

    // --------------------------------------------------------
    // 7.1 Python 进程启动成功
    // --------------------------------------------------------

    QObject::connect(
        py,
        &QProcess::started,
        &window,
        []() {

            fprintf(
                stdout,
                "[Harness] Python process started.\n"
            );

            fflush(stdout);
        }
    );


    // --------------------------------------------------------
    // 7.2 Python 进程发生错误
    // --------------------------------------------------------

    QObject::connect(
        py,
        &QProcess::errorOccurred,
        &window,
        [py, sendButton](QProcess::ProcessError) {

            sendButton->setEnabled(false);

            const QByteArray errorData =
                py->errorString().toUtf8();

            fprintf(
                stderr,
                "[Harness] Python process error: "
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
    );


    // --------------------------------------------------------
    // 7.3 Python 进程退出
    // --------------------------------------------------------

    QObject::connect(
        py,
        &QProcess::finished,
        &window,
        [py,
         sendButton,
         &pythonStdoutBuffer,
         &currentAssistantResponse,
         &currentDeltaCount](
            int exitCode,
            QProcess::ExitStatus exitStatus
        ) {

            // 读取最后剩余的 stdout
            pythonStdoutBuffer +=
                py->readAllStandardOutput();


            // 处理缓冲区中的残余 JSON 数据
            processStdoutBuffer(
                pythonStdoutBuffer,
                sendButton,
                currentAssistantResponse,
                currentDeltaCount,
                true
            );


            // 读取最后剩余的 stderr
            const QByteArray errorOutput =
                py->readAllStandardError();

            if (!errorOutput.isEmpty()) {

                fwrite(
                    errorOutput.constData(),
                    1,
                    static_cast<size_t>(errorOutput.size()),
                    stderr
                );

                fflush(stderr);
            }


            sendButton->setEnabled(false);


            fprintf(
                stdout,
                "[Harness] Python process exited. "
                "exitCode=%d normalExit=%s\n",
                exitCode,
                exitStatus == QProcess::NormalExit
                    ? "true"
                    : "false"
            );

            fflush(stdout);
        }
    );


    // ========================================================
    // 8. 查找并启动 Python 解释器
    // ========================================================

    const QFileInfo scriptInfo(pythonScript);

    const QString pythonProgram =
        QStandardPaths::findExecutable("python");


    if (!scriptInfo.exists() || !scriptInfo.isFile()) {

        fprintf(
            stderr,
            "[Harness] Python script not found.\n"
        );

        fflush(stderr);
    }

    else if (pythonProgram.isEmpty()) {

        fprintf(
            stderr,
            "[Harness] Python interpreter not found.\n"
        );

        fflush(stderr);
    }

    else {

        // 设置 Python 解释器
        py->setProgram(pythonProgram);

        // -u：禁用 Python stdout/stderr 缓冲
        py->setArguments({
            "-u",
            pythonScript
        });


        fprintf(
            stdout,
            "[Harness] Python interpreter: %s\n",
            pythonProgram.toUtf8().constData()
        );

        fprintf(
            stdout,
            "[Harness] Python script: %s\n",
            pythonScript.toUtf8().constData()
        );

        fprintf(
            stdout,
            "[Harness] Agent directory: %s\n",
            agentDir.toUtf8().constData()
        );

        fflush(stdout);


        // 启动 Python 子进程
        py->start();
    }


    // ========================================================
    // 9. Qt → Python：发送 JSONL 消息
    // ========================================================

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


            // 检查 Python 进程是否正常运行
            if (py->state() != QProcess::Running) {

                fprintf(
                    stderr,
                    "[Harness] Python process is not running.\n"
                );

                fflush(stderr);

                return;
            }


            // ------------------------------------------------
            // 9.1 创建 JSON 对象
            // ------------------------------------------------

            QJsonObject messageObject;

            messageObject.insert("text", message);


            // ------------------------------------------------
            // 9.2 序列化 JSON
            // ------------------------------------------------

            const QByteArray jsonData =
                QJsonDocument(messageObject).toJson(
                    QJsonDocument::Compact
                );


            // ------------------------------------------------
            // 9.3 JSONL：每条消息以换行符结尾
            // ------------------------------------------------

            const QByteArray payload =
                jsonData + '\n';


            // ------------------------------------------------
            // 9.4 写入 Python stdin
            // ------------------------------------------------

            const qint64 bytesWritten =
                py->write(payload);


            if (bytesWritten < 0) {

                fprintf(
                    stderr,
                    "[Harness] Failed to write to Python.\n"
                );

                fflush(stderr);

                return;
            }


            // ------------------------------------------------
            // 9.5 更新界面状态
            // ------------------------------------------------

            fprintf(
                stdout,
                "[Harness] Message sent to Python.\n"
            );

            fflush(stdout);

            messageInput->clear();

            messageInput->setFocus();

            sendButton->setEnabled(false);
        }
    );


    // ========================================================
    // 10. Qt 退出时关闭 Python 子进程
    // ========================================================

    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        py,
        [py]() {

            if (py->state() != QProcess::NotRunning) {

                // 关闭 Python stdin
                py->closeWriteChannel();


                // 等待 Python 正常退出
                if (!py->waitForFinished(2000)) {

                    // 超时则尝试终止
                    py->terminate();

                    if (!py->waitForFinished(1000)) {

                        // 最后强制结束
                        py->kill();

                        py->waitForFinished(1000);
                    }
                }
            }
        }
    );


    // ========================================================
    // 11. 显示窗口并进入事件循环
    // ========================================================

    window.show();

    return app.exec();
}
