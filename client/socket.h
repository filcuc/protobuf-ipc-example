#pragma once

#include <QAbstractSocket>
#include <QObject>

#include <functional>

// Forward declarations
class QSslError;

namespace protobuf_client_example {

// Forward declarations
namespace details { class SocketImpl; }
namespace protocol { class Message; }

/// Fucntion callback for ssl errors. Return true for ignoring the error, false otherwise
using SocketErrorHandler = std::function<bool(const QSslError &error)>;

/// Threaded and asynchronous socket implementation
class Socket : public QObject
{
    Q_OBJECT

public:
    /// Constructor
    Socket(SocketErrorHandler errorHandler = [](const QSslError &){ return true; });

    /// Return the last connection hostname
    QString hostName() const { return m_hostName; }

    /// Return the last connection port
    int port() const { return m_port; }

public slots:
    /// Connect to the given hostname and port. This is invoked asynchronously
    void connectToHost(const QString &hostname, int port);

    /// Disconnect from the host. This is invoked asynchronously
    void disconnectFromHost();

    /// Sends a message through this Socket. This is invoked asynchronously
    void sendMessage(const protobuf_client_example::protocol::Message &message);

    /// Private handler
    bool handleError(const QSslError &error);

signals:
    /// Emitted when the socket connnected
    void connected();

    /// Emitted when the socket disconnected
    void disconnected(QAbstractSocket::SocketError error, const QString &errorString);

    /// Emitted when a new message is received
    void messageReceived(const protobuf_client_example::protocol::Message &message);

private:
    details::SocketImpl *m_socket;
    SocketErrorHandler m_errorHandler;
    QString m_hostName;
    int m_port;
};

}
