#include "backend_controller.h"

#include <QAbstractSocket>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QTextEdit>

#include <cstdio>

#include "python_stdout_parser.h"


BackendController::BackendController(
    const QString &host,
    quint16 port,
    QObject *parent
)
    : QObject(parent),
      m_socket(new QTcpSocket(this)),
      m_host(host),
      m_port(port)
{
    QObject::connect(
        m_socket,
        &QTcpSocket::connected,
        this,
        &BackendController::onConnected
    );

    QObject::connect(
        m_socket,
        &QTcpSocket::readyRead,
        this,
        &BackendController::onReadyRead
    );

    QObject::connect(
        m_socket,
        &QTcpSocket::disconnected,
        this,
        &BackendController::onDisconnected
    );

    QObject::connect(
        m_socket,
        &QTcpSocket::errorOccurred,
        this,
        &BackendController::onSocketError
    );
}


void BackendController::bindUi(const BackendUiRefs &refs)
{
    m_ui = refs;

    m_state.connectButton = refs.connectButton;
    m_state.disconnectButton = refs.disconnectButton;
    m_state.sendButton = refs.sendButton;
    m_state.userIdLabel = refs.userIdLabel;
    m_state.skKeyInput = refs.skKeyInput;

    QObject::connect(
        refs.connectButton,
        &QPushButton::clicked,
        this,
        &BackendController::onConnectClicked
    );

    QObject::connect(
        refs.sendButton,
        &QPushButton::clicked,
        this,
        &BackendController::onSendClicked
    );

    QObject::connect(
        refs.disconnectButton,
        &QPushButton::clicked,
        this,
        &BackendController::onDisconnectClicked
    );
}


void BackendController::disconnectFromHost()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}


void BackendController::onConnectClicked()
{
    const QString skKey = m_ui.skKeyInput->text().trimmed();
    const QString model = m_ui.modelCombo->currentText().trimmed();

    if (skKey.isEmpty()) {
        m_ui.skKeyInput->setFocus();
        return;
    }

    m_state.skKey = skKey;
    m_pendingSkKey = skKey;
    m_pendingModel = model;

    m_socket->abort();
    m_buffer.clear();

    m_ui.connectButton->setEnabled(false);
    m_ui.disconnectButton->setEnabled(false);
    m_ui.sendButton->setEnabled(false);
    m_ui.userIdLabel->setText("连接中");

    m_socket->connectToHost(m_host, m_port);

    fprintf(stdout, "[Harness] Connecting to backend...\n");

    fflush(stdout);
}


void BackendController::onConnected()
{
    QJsonObject request;

    request.insert("type", "connect");
    request.insert("sk-key", m_pendingSkKey);
    request.insert("model", m_pendingModel);

    sendRequest(request);
}


void BackendController::onReadyRead()
{
    m_buffer += m_socket->readAll();

    processStdoutBuffer(
        m_buffer,
        m_ui.chatView,
        m_state
    );
}


void BackendController::onDisconnected()
{
    m_state.connected = false;
    m_state.userId = 0;
    m_state.skKey.clear();
    m_state.model.clear();

    setUiDisconnected();

    fprintf(stdout, "[Harness] Backend disconnected\n");

    fflush(stdout);
}


void BackendController::onSocketError()
{
    setUiDisconnected();

    fprintf(stderr, "[Harness] Backend connection error\n");

    fflush(stderr);
}


void BackendController::onSendClicked()
{
    const QString message =
        m_ui.messageInput->toPlainText().trimmed();

    if (message.isEmpty()) {
        m_ui.messageInput->setFocus();
        return;
    }

    if (!m_state.connected ||
        m_socket->state() != QAbstractSocket::ConnectedState) {

        fprintf(
            stderr,
            "[Harness] Backend is not connected.\n"
        );

        fflush(stderr);

        return;
    }

    QJsonObject request;

    request.insert("type", "chat");
    request.insert("user_id", m_state.userId);
    request.insert("sk-key", m_state.skKey);
    request.insert("text", message);

    sendRequest(request);

    appendChatUserMessage(m_ui.chatView, message);

    m_ui.messageInput->clear();
    m_ui.messageInput->setFocus();

    m_ui.sendButton->setEnabled(false);
}


void BackendController::onDisconnectClicked()
{
    if (!m_state.connected) {
        return;
    }

    QJsonObject request;

    request.insert("type", "disconnect");
    request.insert("user_id", m_state.userId);
    request.insert("sk-key", m_state.skKey);

    sendRequest(request);

    m_ui.disconnectButton->setEnabled(false);
    m_ui.sendButton->setEnabled(false);
}


void BackendController::sendRequest(const QJsonObject &request)
{
    const QByteArray payload =
        QJsonDocument(request).toJson(QJsonDocument::Compact)
        + '\n';

    m_socket->write(payload);
    m_socket->flush();
}


void BackendController::setUiDisconnected()
{
    m_state.connected = false;
    m_state.userId = 0;
    m_state.skKey.clear();
    m_state.model.clear();

    m_ui.connectButton->setEnabled(true);
    m_ui.disconnectButton->setEnabled(false);
    m_ui.sendButton->setEnabled(false);
    m_ui.userIdLabel->setText("未连接");
}
