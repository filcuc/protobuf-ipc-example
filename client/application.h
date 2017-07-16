#pragma once

#include <QObject>
#include <QSslSocket>

class QString;

namespace protobuf_client_example {

  namespace protocol { class Message; }

  class Application : public QObject
  {
    Q_OBJECT

  public:
    Application();

    void start(const QString &url, int port);

  private slots:
    void onSslErrors(const QList<QSslError> &errors);

  private:
    void displayCertificateInformations(const QSslCertificate &certificate);
    void displayCertificateIssuerInformations(const QSslCertificate &certificate);

    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void send(const protocol::Message &message);
    void onMessageReceived(const protocol::Message &message);

    int m_port;
    QString m_hostname;
    QSslSocket m_socket;
    QByteArray m_recvBuffer;
    std::vector<char> m_sendBuffer;
  };
} // ns protobuf_client_example
