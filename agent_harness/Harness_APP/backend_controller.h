#ifndef BACKEND_CONTROLLER_H
#define BACKEND_CONTROLLER_H

#include <QByteArray>
#include <QObject>
#include <QString>

#include "python_event_handler.h"

class QComboBox;
class QJsonObject;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTcpSocket;
class QTextEdit;


struct BackendUiRefs {
    QComboBox *modelCombo = nullptr;
    QLineEdit *skKeyInput = nullptr;
    QPlainTextEdit *messageInput = nullptr;
    QPushButton *connectButton = nullptr;
    QPushButton *disconnectButton = nullptr;
    QPushButton *sendButton = nullptr;
    QLabel *userIdLabel = nullptr;
    QTextEdit *chatView = nullptr;
};


class BackendController : public QObject {
    Q_OBJECT

public:
    explicit BackendController(
        const QString &host,
        quint16 port,
        QObject *parent = nullptr
    );

    void bindUi(const BackendUiRefs &refs);

public slots:
    void disconnectFromHost();

private slots:
    void onConnectClicked();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onSocketError();
    void onSendClicked();
    void onDisconnectClicked();

private:
    void sendRequest(const QJsonObject &request);
    void setUiDisconnected();

private:
    QTcpSocket *m_socket = nullptr;
    BackendUiRefs m_ui;
    BackendUiState m_state;
    QByteArray m_buffer;
    QString m_pendingSkKey;
    QString m_pendingModel;
    QString m_host;
    quint16 m_port = 0;
};

#endif // BACKEND_CONTROLLER_H
