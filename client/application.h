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
    void onSslErrors();

  private:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void send(const protocol::Message &message);
    void onMessageReceived(const protocol::Message &message);

    QSslSocket socket;
    std::vector<char> sendBuffer;
  };
} // ns protobuf_client_example
