
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "backend_controller.h"
#include "ui_theme.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Agent Harness");
    app.setOrganizationName("PersonalProjects");

    applyDarkTheme(app);

    const QString appDir =
        QCoreApplication::applicationDirPath();

    const QStringList iconCandidates = {
        QDir(appDir).filePath("app_icon.png"),
        QDir::cleanPath(
            QDir(appDir).absoluteFilePath("../resources/app_icon.png")
        ),
        QDir::cleanPath(
            QDir(appDir).absoluteFilePath(
                "../../Harness_APP/resources/app_icon.png"
            )
        ),
        QDir::cleanPath(
            QDir(appDir).absoluteFilePath(
                "../Harness_APP/resources/app_icon.png"
            )
        ),
        QDir::cleanPath(
            QDir(appDir).absoluteFilePath(
                "../agent_harness/Harness_APP/resources/app_icon.png"
            )
        ),
    };

    QString appIconPath;

    for (const QString &candidate : iconCandidates) {
        if (QFileInfo::exists(candidate)) {
            appIconPath = candidate;
            break;
        }
    }

    QMainWindow window;

    window.setObjectName("mainWindow");
    window.setWindowTitle("Agent Harness");

    if (!appIconPath.isEmpty()) {
        window.setWindowIcon(QIcon(appIconPath));
    }

    window.resize(1280, 800);
    window.setMinimumSize(900, 600);

    auto *rootSurface = new QWidget(&window);

    rootSurface->setObjectName("rootSurface");
    rootSurface->setAttribute(Qt::WA_StyledBackground, true);

    auto *rootLayout = new QVBoxLayout(rootSurface);

    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(12);

    auto *titleBar = new QWidget(rootSurface);

    titleBar->setObjectName("titleBar");
    titleBar->setAttribute(Qt::WA_StyledBackground, true);
    titleBar->setFixedHeight(48);

    auto *titleBarLayout = new QHBoxLayout(titleBar);

    titleBarLayout->setContentsMargins(4, 0, 4, 0);
    titleBarLayout->setSpacing(10);

    auto *titleIconLabel = new QLabel(titleBar);

    titleIconLabel->setObjectName("titleIconLabel");
    titleIconLabel->setFixedSize(28, 28);

    if (!appIconPath.isEmpty()) {
        titleIconLabel->setPixmap(
            QPixmap(appIconPath).scaled(
                28,
                28,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            )
        );
    }

    auto *titleLabel = new QLabel("Agent Harness", titleBar);

    titleLabel->setObjectName("appTitleLabel");

    titleBarLayout->addStretch(1);
    titleBarLayout->addWidget(titleIconLabel);
    titleBarLayout->addWidget(titleLabel);
    titleBarLayout->addStretch(1);

    rootLayout->addWidget(titleBar);

    auto *connectionPanel = new QWidget(rootSurface);

    connectionPanel->setObjectName("connectionPanel");
    connectionPanel->setAttribute(Qt::WA_StyledBackground, true);

    auto *connectionLayout = new QHBoxLayout(connectionPanel);

    connectionLayout->setContentsMargins(10, 8, 10, 8);
    connectionLayout->setSpacing(10);

    auto *modelCombo = new QComboBox(connectionPanel);

    modelCombo->setObjectName("modelCombo");
    modelCombo->setEditable(true);
    modelCombo->addItem("");
    modelCombo->addItem("qwen4-max");
    modelCombo->addItem("qwen-plus");
    modelCombo->addItem("qwen3.8-omni-flash");
    modelCombo->setCurrentIndex(0);
    modelCombo->lineEdit()->setPlaceholderText(
        "模型（默认 qwen4-max）"
    );
    modelCombo->setFixedHeight(34);
    modelCombo->setMinimumWidth(180);

    auto *skKeyInput = new QLineEdit(connectionPanel);

    skKeyInput->setObjectName("skKeyInput");
    skKeyInput->setPlaceholderText("sk-key");
    skKeyInput->setEchoMode(QLineEdit::Password);
    skKeyInput->setFixedHeight(34);

    auto *connectButton = new QPushButton("连接", connectionPanel);

    connectButton->setObjectName("connectButton");
    connectButton->setFixedHeight(34);
    connectButton->setMinimumWidth(80);
    connectButton->setCursor(Qt::PointingHandCursor);

    auto *disconnectButton = new QPushButton("断开", connectionPanel);

    disconnectButton->setObjectName("disconnectButton");
    disconnectButton->setFixedHeight(34);
    disconnectButton->setMinimumWidth(80);
    disconnectButton->setCursor(Qt::PointingHandCursor);
    disconnectButton->setEnabled(false);

    auto *userIdLabel = new QLabel("未连接", connectionPanel);

    userIdLabel->setObjectName("userIdLabel");

    connectionLayout->addWidget(modelCombo);
    connectionLayout->addWidget(skKeyInput);
    connectionLayout->addWidget(connectButton);
    connectionLayout->addWidget(disconnectButton);
    connectionLayout->addStretch(1);
    connectionLayout->addWidget(userIdLabel);

    rootLayout->addWidget(connectionPanel);

    auto *homeCanvas = new QWidget(rootSurface);

    homeCanvas->setObjectName("homeCanvas");
    homeCanvas->setAttribute(Qt::WA_StyledBackground, true);

    auto *homeLayout = new QVBoxLayout(homeCanvas);

    homeLayout->setContentsMargins(28, 28, 28, 24);
    homeLayout->setSpacing(14);

    auto *chatView = new QTextEdit(homeCanvas);

    chatView->setObjectName("chatView");
    chatView->setReadOnly(true);
    chatView->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
    );

    homeLayout->addWidget(chatView);

    auto *composerPanel = new QWidget(homeCanvas);

    composerPanel->setObjectName("composerPanel");
    composerPanel->setAttribute(Qt::WA_StyledBackground, true);
    composerPanel->setMaximumWidth(1400);
    composerPanel->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
    );

    auto *composerLayout = new QVBoxLayout(composerPanel);

    composerLayout->setContentsMargins(16, 12, 16, 12);
    composerLayout->setSpacing(10);

    auto *messageInput = new QPlainTextEdit(composerPanel);

    messageInput->setObjectName("messageInput");
    messageInput->setPlaceholderText("输入消息……");
    messageInput->setFixedHeight(82);
    messageInput->setTabChangesFocus(false);

    composerLayout->addWidget(messageInput);

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

    auto *composerRow = new QHBoxLayout();

    composerRow->addStretch(1);
    composerRow->addWidget(composerPanel);
    composerRow->addStretch(1);

    homeLayout->addLayout(composerRow);
    homeLayout->setStretch(0, 1);

    rootLayout->addWidget(homeCanvas);
    rootLayout->setStretch(2, 1);

    window.setCentralWidget(rootSurface);

    const QString serverHost = "127.0.0.1";
    const quint16 serverPort = 9000;

    BackendController controller(serverHost, serverPort, &window);

    controller.bindUi({
        modelCombo,
        skKeyInput,
        messageInput,
        connectButton,
        disconnectButton,
        sendButton,
        userIdLabel,
        chatView,
    });

    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        &controller,
        &BackendController::disconnectFromHost
    );

    window.show();

    return app.exec();
}
