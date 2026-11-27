
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QStringList>
#include <QTextEdit>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdio>

#include "ui_theme.h"
#include "python_event_handler.h"
#include "python_stdout_parser.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Harness");
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
    window.setWindowTitle("Harness");

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

    auto *titleLabel = new QLabel("Harness", titleBar);

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

    auto *backendSocket = new QTcpSocket(&app);

    BackendUiState backendState;

    backendState.connectButton = connectButton;
    backendState.disconnectButton = disconnectButton;
    backendState.sendButton = sendButton;
    backendState.userIdLabel = userIdLabel;
    backendState.skKeyInput = skKeyInput;

    QByteArray backendBuffer;
    QString pendingSkKey;

    auto sendRequest = [backendSocket](const QJsonObject &request) {
        const QByteArray payload =
            QJsonDocument(request).toJson(QJsonDocument::Compact)
            + '\n';

        backendSocket->write(payload);
        backendSocket->flush();
    };

    QObject::connect(
        connectButton,
        &QPushButton::clicked,
        &window,
        [backendSocket,
         skKeyInput,
         connectButton,
         disconnectButton,
         sendButton,
         userIdLabel,
         chatView,
         sendRequest,
         serverHost,
         serverPort,
         &backendState,
         &backendBuffer,
         &pendingSkKey]() {

            const QString skKey = skKeyInput->text().trimmed();

            if (skKey.isEmpty()) {
                skKeyInput->setFocus();
                return;
            }

            backendState.skKey = skKey;
            pendingSkKey = skKey;

            backendSocket->abort();
            backendBuffer.clear();

            connectButton->setEnabled(false);
            disconnectButton->setEnabled(false);
            sendButton->setEnabled(false);
            userIdLabel->setText("连接中");

            backendSocket->connectToHost(serverHost, serverPort);

            fprintf(stdout, "[Harness] Connecting to backend...\n");

            fflush(stdout);
        }
    );

    QObject::connect(
        backendSocket,
        &QTcpSocket::connected,
        &window,
        [sendRequest, &pendingSkKey]() {

            QJsonObject request;

            request.insert("type", "connect");
            request.insert("sk-key", pendingSkKey);

            sendRequest(request);
        }
    );

    QObject::connect(
        backendSocket,
        &QTcpSocket::readyRead,
        &window,
        [backendSocket, chatView, &backendBuffer, &backendState]() {

            backendBuffer += backendSocket->readAll();

            processStdoutBuffer(
                backendBuffer,
                chatView,
                backendState
            );
        }
    );

    QObject::connect(
        backendSocket,
        &QTcpSocket::disconnected,
        &window,
        [connectButton,
         disconnectButton,
         sendButton,
         userIdLabel,
         &backendState]() {

            backendState.connected = false;
            backendState.userId = 0;
            backendState.skKey.clear();

            connectButton->setEnabled(true);
            disconnectButton->setEnabled(false);
            sendButton->setEnabled(false);
            userIdLabel->setText("未连接");

            fprintf(stdout, "[Harness] Backend disconnected\n");

            fflush(stdout);
        }
    );

    QObject::connect(
        backendSocket,
        &QTcpSocket::errorOccurred,
        &window,
        [connectButton,
         disconnectButton,
         sendButton,
         userIdLabel]() {

            connectButton->setEnabled(true);
            disconnectButton->setEnabled(false);
            sendButton->setEnabled(false);
            userIdLabel->setText("未连接");

            fprintf(stderr, "[Harness] Backend connection error\n");

            fflush(stderr);
        }
    );

    QObject::connect(
        sendButton,
        &QPushButton::clicked,
        &window,
        [messageInput,
         backendSocket,
         sendButton,
         chatView,
         sendRequest,
         &backendState]() {

            const QString message =
                messageInput->toPlainText().trimmed();

            if (message.isEmpty()) {
                messageInput->setFocus();
                return;
            }

            if (!backendState.connected ||
                backendSocket->state()
                    != QAbstractSocket::ConnectedState) {

                fprintf(
                    stderr,
                    "[Harness] Backend is not connected.\n"
                );

                fflush(stderr);

                return;
            }

            QJsonObject request;

            request.insert("type", "chat");
            request.insert("user_id", backendState.userId);
            request.insert("sk-key", backendState.skKey);
            request.insert("text", message);

            sendRequest(request);

            appendChatUserMessage(chatView, message);

            messageInput->clear();
            messageInput->setFocus();

            sendButton->setEnabled(false);
        }
    );

    QObject::connect(
        disconnectButton,
        &QPushButton::clicked,
        &window,
        [backendSocket,
         sendButton,
         disconnectButton,
         sendRequest,
         &backendState]() {

            if (!backendState.connected) {
                return;
            }

            QJsonObject request;

            request.insert("type", "disconnect");
            request.insert("user_id", backendState.userId);
            request.insert("sk-key", backendState.skKey);

            sendRequest(request);

            disconnectButton->setEnabled(false);
            sendButton->setEnabled(false);
        }
    );

    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        backendSocket,
        [backendSocket]() {
            backendSocket->disconnectFromHost();
        }
    );

    window.show();

    return app.exec();
}
