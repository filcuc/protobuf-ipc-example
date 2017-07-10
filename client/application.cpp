#include "application.h"
#include "message.pb.h"

#include <QCoreApplication>
#include <QDebug>

namespace protobuf_client_example {

  namespace {
    const uint32_t MAGIC_NUMBER = 12345;

    void encode(char* ptr, uint32_t data) {
      ptr[0] = static_cast<char>((data & 0xFF000000) >> 24); 
      ptr[1] = static_cast<char>((data & 0x00FF0000) >> 16); 
      ptr[2] = static_cast<char>((data & 0x0000FF00) >> 8); 
      ptr[3] = static_cast<char>((data & 0x000000FF));
    }
  }

  Application::Application() {
    sendBuffer.resize(1024);
    QObject::connect(&socket, &QTcpSocket::connected, this, &Application::onConnected);
    QObject::connect(&socket, &QTcpSocket::disconnected, this, &Application::onDisconnected);
  }

  void Application::start(const QString &url, int port) {
    qDebug() << "Application start";
    socket.connectToHost(url, port);
  }

  void Application::onConnected() {
    qDebug() << "Socket connected sending first message";
    protocol::Message message;
    message.set_type(protocol::Message_Type_GET_EVENTS_REQUEST);
    protocol::GetEventRequest* request =  message.mutable_geteventrequest();
    send(message);
  }

  void Application::onDisconnected() {
    qDebug() << "Socket disconnected, closing the application";
    qApp->quit();
  }

  void Application::send(const protocol::Message &message) {
    const int message_size = 2 * sizeof(uint32_t) + message.ByteSize();
    sendBuffer.resize(message_size);
    encode(&sendBuffer[0], MAGIC_NUMBER);
    encode(&sendBuffer[4], message.ByteSize());
    const bool ok = message.SerializeToArray(&sendBuffer[8], sendBuffer.size() - 8);
    if (ok) {
      qDebug() << "Sending a message through the socket";
      socket.write(sendBuffer.data(), sendBuffer.size());
    }
  }
}
