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
#include <QMainWindow>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProcess>
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
}

int main(int argc, char *argv[])
{
    // 创建 Qt 应用程序对象
    QApplication app(argc, argv);
    app.setApplicationName("Harness");
    app.setOrganizationName("PersonalProjects");

    // 初始化全局深色主题
    applyDarkTheme(app);

    // 创建主窗口并设置标题和尺寸
    QMainWindow window;
    window.setObjectName("mainWindow");
    window.setWindowTitle("Harness");
    window.resize(1280, 800);
    window.setMinimumSize(900, 600);

    // 创建主窗口根容器和外层布局
    auto *rootSurface = new QWidget(&window);
    rootSurface->setObjectName("rootSurface");
    rootSurface->setAttribute(Qt::WA_StyledBackground, true);

    auto *rootLayout = new QVBoxLayout(rootSurface);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(0);

    // 创建首页中央区域和垂直布局
    auto *homeCanvas = new QWidget(rootSurface);
    homeCanvas->setObjectName("homeCanvas");
    homeCanvas->setAttribute(Qt::WA_StyledBackground, true);

    auto *homeLayout = new QVBoxLayout(homeCanvas);
    homeLayout->setContentsMargins(28, 28, 28, 24);
    homeLayout->setSpacing(0);
    homeLayout->addStretch(1);

    // 创建居中的消息输入面板
    auto *composerPanel = new QWidget(homeCanvas);
    composerPanel->setObjectName("composerPanel");
    composerPanel->setAttribute(Qt::WA_StyledBackground, true);
    composerPanel->setMaximumWidth(900);
    composerPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

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

    // 创建输入面板底部按钮布局
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);

    // 创建发送按钮
    auto *sendButton = new QPushButton("发送", composerPanel);
    sendButton->setObjectName("sendButton");
    sendButton->setCursor(Qt::PointingHandCursor);
    sendButton->setMinimumWidth(88);
    sendButton->setFixedHeight(36);
    sendButton->setEnabled(false);
    buttonLayout->addWidget(sendButton);
    composerLayout->addLayout(buttonLayout);

    // 将输入面板放置在页面底部并保持水平居中
    auto *composerRow = new QHBoxLayout();
    composerRow->addStretch(1);
    composerRow->addWidget(composerPanel);
    composerRow->addStretch(1);
    homeLayout->addLayout(composerRow);

    // 将首页区域放入主窗口根布局
    rootLayout->addWidget(homeCanvas);
    window.setCentralWidget(rootSurface);

    // 根据当前目录结构定位同级 Agent 文件夹中的 main.py
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString agentDir = QDir::cleanPath(QDir(appDir).absoluteFilePath("../../Agent"));
    const QString pythonScript = QDir::cleanPath(QDir(agentDir).filePath("main.py"));

    // 创建 Python 子进程
    auto *py = new QProcess(&app);
    py->setWorkingDirectory(agentDir);

    // 接收 Python 标准输出并转发到当前终端
    QObject::connect(py, &QProcess::readyReadStandardOutput, &window, [py]() {
        const QByteArray output = py->readAllStandardOutput();
        if (!output.isEmpty()) {
            qInfo().noquote().nospace() << QString::fromUtf8(output).trimmed();
        }
    });

    // 接收 Python 错误输出并转发到当前终端
    QObject::connect(py, &QProcess::readyReadStandardError, &window, [py]() {
        const QByteArray output = py->readAllStandardError();
        if (!output.isEmpty()) {
            qWarning().noquote().nospace() << QString::fromUtf8(output).trimmed();
        }
    });

    // Python 解释器启动后开放发送按钮
    QObject::connect(py, &QProcess::started, &window, [sendButton]() {
        qInfo().noquote() << "[Harness] Python 解释器已启动，发送通道就绪";
        sendButton->setEnabled(true);
    });

    // 输出 Python 子进程启动或运行错误
    QObject::connect(py, &QProcess::errorOccurred, &window,
                     [py, sendButton](QProcess::ProcessError) {
        sendButton->setEnabled(false);
        qWarning().noquote() << "[Harness] Python 子进程错误：" << py->errorString();
    });

    // Python 进程退出后停止发送
    QObject::connect(py, &QProcess::finished, &window,
                     [sendButton](int exitCode, QProcess::ExitStatus exitStatus) {
        sendButton->setEnabled(false);
        qInfo().noquote() << "[Harness] Python 子进程已退出，exitCode="
                          << exitCode << "，正常退出="
                          << (exitStatus == QProcess::NormalExit);
    });

    // 检查 Python 脚本和解释器是否存在
    const QFileInfo scriptInfo(pythonScript);
    const QString pythonProgram = QStandardPaths::findExecutable("python");

    if (!scriptInfo.exists() || !scriptInfo.isFile()) {
        qCritical().noquote() << "[Harness] 找不到 Python 脚本：" << pythonScript;
        qCritical().noquote() << "[Harness] 请检查 Agent/main.py 是否存在。";
    } else if (pythonProgram.isEmpty()) {
        qCritical().noquote() << "[Harness] 找不到 python.exe，请检查当前终端的 PATH/Conda 环境。";
    } else {
        // 设置 Python 解释器、脚本路径和工作目录
        py->setProgram(pythonProgram);
        py->setArguments({"-u", pythonScript});

        // 输出启动信息并启动 Python 子进程
        qInfo().noquote() << "[Harness] Python 解释器：" << pythonProgram;
        qInfo().noquote() << "[Harness] Python 脚本：" << pythonScript;
        py->start();
    }

    // 点击发送后将 JSON 行写入 Python 子进程的标准输入
    QObject::connect(sendButton, &QPushButton::clicked, &window,
                     [messageInput, py]() {
        const QString message = messageInput->toPlainText().trimmed();
        if (message.isEmpty()) {
            messageInput->setFocus();
            return;
        }

        // 创建 JSON 对象并序列化为 UTF-8
        QJsonObject messageObject;
        messageObject.insert("text", message);
        const QByteArray jsonData =
            QJsonDocument(messageObject).toJson(QJsonDocument::Compact);

        // 检查 Python 子进程是否正在运行
        if (py->state() != QProcess::Running) {
            qWarning().noquote() << "[Harness] Python 子进程尚未运行：" << py->errorString();
            return;
        }

        // 在 JSON 末尾添加换行符，作为一条消息的结束标记
        const QByteArray payload = jsonData + '\n';

        // 将 JSON 数据写入 Python 的 stdin
        const qint64 bytesWritten = py->write(payload);
        if (bytesWritten < 0) {
            qWarning().noquote() << "[Harness] 写入 Python 失败：" << py->errorString();
            return;
        }

        // 输出发送日志并清空输入框
        qInfo().noquote() << "[Harness] 已发送 JSON：" << QString::fromUtf8(jsonData);
        messageInput->clear();
        messageInput->setFocus();
    });

    // 退出 Qt 应用时关闭 Python stdin，让 Python 循环正常结束
    QObject::connect(&app, &QCoreApplication::aboutToQuit, py, [py]() {
        if (py->state() != QProcess::NotRunning) {
            py->closeWriteChannel();
            py->waitForFinished(1000);
            if (py->state() != QProcess::NotRunning) {
                py->terminate();
                if (!py->waitForFinished(1000)) {
                    py->kill();
                    py->waitForFinished(1000);
                }
            }
        }
    });

    // 显示主窗口并进入 Qt 事件循环
    window.show();
    return app.exec();
}
