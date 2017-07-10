#pragma once

#include <QObject>
#include <QTcpSocket>

class QString;

namespace protobuf_client_example {

  namespace protocol { class Message; }

  class Application : public QObject
  {
    Q_OBJECT

  public:
    Application();

    void start(const QString &url, int port);

  private:
    void onConnected();
    void onDisconnected();
    void send(const protocol::Message &message);

    QTcpSocket socket;
    std::vector<char> sendBuffer;
  };
} // ns protobuf_client_example
