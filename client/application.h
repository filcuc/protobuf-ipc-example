#pragma once

#include <QObject>
#include <QSslSocket>

class QString;

namespace protobuf_client_example {
class Socket;
namespace protocol { class Message; }

/// This is the whole application controller
class Application : public QObject
{
    Q_OBJECT

public:
    /// Constructor
    Application();

    /// Connect to the given host and port
    void connectToHost(const QString &url, int port);

private:
    /// Called on Ssl errors
    bool onSslError(const QSslError &error);

    /// Called on socket connected
    void onConnected();

    /// Called on message received
    void onMessageReceived(const protobuf_client_example::protocol::Message &message);

    /// Called on socket disconnected
    void onDisconnected(QAbstractSocket::SocketError error, const QString &errorString);

    Socket* m_socket;
};
} // ns protobuf_client_example
