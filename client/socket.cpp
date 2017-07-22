#include "socket.h"

#include "message.pb.h"

#include <QByteArray>
#include <QSslSocket>
#include <QThread>

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace protobuf_client_example
{

namespace details
{

const uint32_t MAGIC_NUMBER = 12345;
const int BUFFER_SIZE = 1024;

/// Encode a uint32 to big endian encoding
void encodeToBigEndian(char* ptr, uint32_t data) {
    ptr[0] = static_cast<char>((data & 0xFF000000) >> 24);
    ptr[1] = static_cast<char>((data & 0x00FF0000) >> 16);
    ptr[2] = static_cast<char>((data & 0x0000FF00) >> 8);
    ptr[3] = static_cast<char>((data & 0x000000FF));
}

/// Decode a big endian to current uint32 endianess
uint32_t decodeFromBigEndian(char* ptr) {
    uint32_t data = static_cast<uint32_t>(ptr[0]);
    data = data << 8;
    data += static_cast<uint32_t>(ptr[1]);
    data = data << 8;
    data += static_cast<uint32_t>(ptr[2]);
    data = data << 8;
    data += static_cast<uint32_t>(ptr[3]);
    return data;
}

/// This class wraps a QSslSocket and it's meant to run in its own
/// thread
class SocketImpl : public QObject
{
    Q_OBJECT

public:
    SocketImpl(Socket* errorHandler, QObject *parent = nullptr)
        : QObject(parent)
        , m_socket(new QSslSocket(this))
        , m_errorHandler(errorHandler)
    {
        m_socket->setPeerVerifyMode(QSslSocket::QueryPeer);
        m_recvBuffer.reserve(BUFFER_SIZE);
        m_sendBuffer.resize(BUFFER_SIZE);
        QObject::connect(m_socket, &QSslSocket::encrypted, this, &SocketImpl::connected);
        QObject::connect(m_socket, &QSslSocket::disconnected, this, &SocketImpl::onDisconected);
        QObject::connect(m_socket, static_cast<void(QSslSocket::*)(const QList<QSslError>&)>(&QSslSocket::sslErrors), this, &SocketImpl::onSslErrors);
        QObject::connect(m_socket, &QSslSocket::readyRead, this, &SocketImpl::onReadyRead);
    }

public slots:
    void connectToHost(const QString &hostname, int port)
    {
        m_socket->connectToHostEncrypted(hostname, port);
    }

    void disconectFromHost()
    {
        m_socket->disconnectFromHost();
    }

    void onSslErrors(const QList<QSslError> &errors)
    {
        QList<QSslError> toIgnore;
        for (const QSslError &e : errors) {
            bool result = true;
            QMetaObject::invokeMethod(m_errorHandler, "handleError", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result), Q_ARG(QSslError, e));
            if (!result) {
                disconectFromHost();
                return;
            }
            toIgnore.push_back(e);
        }
        m_socket->ignoreSslErrors(toIgnore);
    }

    void sendMessage(const protobuf_client_example::protocol::Message &message)
    {
        const int message_size = 2 * sizeof(uint32_t) + message.ByteSize();
        m_sendBuffer.resize(message_size);
        encodeToBigEndian(&m_sendBuffer[0], MAGIC_NUMBER);
        encodeToBigEndian(&m_sendBuffer[4], message.ByteSize());
        const bool ok = message.SerializeToArray(&m_sendBuffer[8], m_sendBuffer.size() - 8);
        if (ok) {
            qDebug("Sending a message through the socket");
            m_socket->write(m_sendBuffer.data(), m_sendBuffer.size());
        }
    }

    void onReadyRead()
    {
        m_recvBuffer.append(m_socket->readAll());
        while (true) {
            if (m_recvBuffer.size() < 4)
                return;
            uint32_t magic_number = decodeFromBigEndian(m_recvBuffer.data());
            if (m_recvBuffer.size() < 8)
                return;
            uint32_t message_size = decodeFromBigEndian(m_recvBuffer.data() + 4);
            if (m_recvBuffer.size() < (8 + message_size))
                return;
            protobuf_client_example::protocol::Message message;
            message.ParseFromArray(m_recvBuffer.data() + 8, message_size);
            m_recvBuffer = m_recvBuffer.mid(8 + message_size);
            emit messageReceived(message);
        }
    }

    void onDisconected()
    {
        emit disconnected(m_socket->error(), m_socket->errorString());
    }

signals:
    void connected();
    void disconnected(QAbstractSocket::SocketError error, const QString& errorString);
    void messageReceived(const protobuf_client_example::protocol::Message &message);

private:
    QSslSocket *m_socket;
    Socket *m_errorHandler;
    QByteArray m_recvBuffer;
    std::vector<char> m_sendBuffer;
};

} // namespace protobuf_client_example::details

Socket::Socket(SocketErrorHandler errorHandler)
    : m_socket(new details::SocketImpl(this))
    , m_errorHandler(std::move(errorHandler))
    , m_hostName(QString())
    , m_port(0)
{
    QThread* m_thread = new QThread(this);
    QObject::connect(m_thread, &QThread::finished, m_socket, &details::SocketImpl::deleteLater);
    m_socket->moveToThread(m_thread);
    QObject::connect(m_socket, &details::SocketImpl::connected, this, &Socket::connected, Qt::QueuedConnection);
    QObject::connect(m_socket, &details::SocketImpl::disconnected, this, &Socket::disconnected, Qt::QueuedConnection);
    QObject::connect(m_socket, &details::SocketImpl::messageReceived, this, &Socket::messageReceived, Qt::QueuedConnection);
    m_thread->start();
}

void Socket::connectToHost(const QString &hostname, int port)
{
    m_hostName = hostname;
    m_port = port;
    QMetaObject::invokeMethod(m_socket, "connectToHost", Qt::QueuedConnection, Q_ARG(QString, hostname), Q_ARG(int, port));
}

void Socket::disconnectFromHost()
{
    QMetaObject::invokeMethod(m_socket, "disconnectFromHost", Qt::QueuedConnection);
}

bool Socket::handleError(const QSslError &error)
{
    return m_errorHandler(error);
}

void Socket::sendMessage(const protobuf_client_example::protocol::Message &message)
{
    QMetaObject::invokeMethod(m_socket, "sendMessage", Qt::QueuedConnection, Q_ARG(protobuf_client_example::protocol::Message, message));
}

} // namespace protobuf_client_example

#include "socket.moc"
