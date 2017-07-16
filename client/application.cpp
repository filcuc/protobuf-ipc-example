#include "application.h"
#include "message.pb.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>

#include <iostream>

namespace protobuf_client_example {

  namespace {
    const uint32_t MAGIC_NUMBER = 12345;

    void encodeToBigEndian(char* ptr, uint32_t data) {
      ptr[0] = static_cast<char>((data & 0xFF000000) >> 24); 
      ptr[1] = static_cast<char>((data & 0x00FF0000) >> 16); 
      ptr[2] = static_cast<char>((data & 0x0000FF00) >> 8); 
      ptr[3] = static_cast<char>((data & 0x000000FF));
    }

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
  }

  Application::Application() {
    m_recvBuffer.reserve(1024);
    m_sendBuffer.resize(1024);
    QObject::connect(&m_socket, &QSslSocket::encrypted, this, &Application::onConnected);
    QObject::connect(&m_socket, &QSslSocket::disconnected, this, &Application::onDisconnected);
    QObject::connect(&m_socket, &QSslSocket::readyRead, this, &Application::onReadyRead);
    QObject::connect(&m_socket, static_cast<void(QSslSocket::*)(const QList<QSslError>&)>(&QSslSocket::sslErrors), this, &Application::onSslErrors);
  }

  void Application::start(const QString &hostname, int port) {
    qInfo("Application started");
    qInfo("Connecting to %s:%d", qUtf8Printable(hostname), port);
    m_port = port;
    m_hostname = hostname;
    m_socket.setPeerVerifyMode(QSslSocket::QueryPeer);
    m_socket.connectToHostEncrypted(hostname, port);
  }

  void Application::onConnected() {
    qDebug("Socket connected");
    protocol::Message message;
    message.set_type(protocol::Message_Type_GET_EVENTS_REQUEST);
    protocol::GetEventRequest* request =  message.mutable_geteventrequest();
    send(message);
  }

  void Application::onDisconnected() {
    qInfo("Socket disconnected, closing the application");
    if (m_socket.error() != QSslSocket::UnknownSocketError)
        qWarning("Socket error is %s", qUtf8Printable(m_socket.errorString()));
    qApp->quit();
  }

  void Application::onReadyRead()
  {
      m_recvBuffer.append(m_socket.readAll());
      while (true) {
          if (m_recvBuffer.size() < 4)
              return;
          uint32_t magic_number = decodeFromBigEndian(m_recvBuffer.data());
          if (m_recvBuffer.size() < 8)
              return;
          uint32_t message_size = decodeFromBigEndian(m_recvBuffer.data() + 4);
          if (m_recvBuffer.size() < (8 + message_size))
              return;
          protocol::Message message;
          message.ParseFromArray(m_recvBuffer.data() + 8, message_size);
          m_recvBuffer = m_recvBuffer.mid(8 + message_size);
          onMessageReceived(message);
      }
  }

  void Application::send(const protocol::Message &message) {
    const int message_size = 2 * sizeof(uint32_t) + message.ByteSize();
    m_sendBuffer.resize(message_size);
    encodeToBigEndian(&m_sendBuffer[0], MAGIC_NUMBER);
    encodeToBigEndian(&m_sendBuffer[4], message.ByteSize());
    const bool ok = message.SerializeToArray(&m_sendBuffer[8], m_sendBuffer.size() - 8);
    if (ok) {
      qDebug("Sending a message through the socket");
      m_socket.write(m_sendBuffer.data(), m_sendBuffer.size());
    }
  }

  void Application::onMessageReceived(const protocol::Message &message)
  {
      switch (message.type())
      {
      case protocol::Message_Type_GET_EVENTS_REPLY:
          qDebug("GetEventsReply received");
          break;
      default:
          qWarning("Unknown message received");
      }
  }

  void Application::onSslErrors(const QList<QSslError> &errors)
  {
      for (const QSslError &error : errors) {
          qInfo("****************************************");
          qInfo("Ssl error connecting to %s:%d", qUtf8Printable(m_hostname), m_port);
          qInfo("Error: %s", qUtf8Printable(error.errorString()));
          qInfo("****************************************");

          QSslCertificate cert = m_socket.peerCertificate();

          std::string action;
          while (true) {
            if (action == "y") {
                m_socket.ignoreSslErrors({error});
                break;
            }
            if (action == "n") {
                m_socket.disconnectFromHost();
                return;
            }
            if (action == "c") {
                displayCertificateInformations(cert);
                action = std::string();
            }
            if (action == "s") {
                displayCertificateIssuerInformations(cert);
                action = std::string();
            }
            if (!action.empty())
              qInfo("Please input \"y\", \"n\", \"c\", \"s\"");
            qInfo() << "";
            qInfo("Do you wish to continue?");
            qInfo("\"y\" Yes | \"n\" No | \"c\" Cert Info | \"s\" Issuer Info");
            std::getline(std::cin, action);
          }
      }
  }

  void Application::displayCertificateInformations(const QSslCertificate &cert)
  {
      qInfo() << "Common Name:" << cert.subjectInfo(QSslCertificate::CommonName);
      qInfo() << "Organization:" << cert.subjectInfo(QSslCertificate::Organization);
      qInfo() << "Unit:" << cert.subjectInfo(QSslCertificate::OrganizationalUnitName);
      qInfo() << "Country:" << cert.subjectInfo(QSslCertificate::CountryName);
      qInfo() << "State|Province:" << cert.subjectInfo(QSslCertificate::StateOrProvinceName);
      qInfo() << "Locality:" << cert.subjectInfo(QSslCertificate::LocalityName);
      qInfo() << "Email:" << cert.subjectInfo(QSslCertificate::EmailAddress);
      qInfo() << "SerialNumber:" << cert.subjectInfo(QSslCertificate::SerialNumber);
      qInfo() << "SelfSigned:" << cert.isSelfSigned();
  }

  void Application::displayCertificateIssuerInformations(const QSslCertificate &cert)
  {
      qInfo() << "Common Name:" << cert.issuerInfo(QSslCertificate::CommonName);
      qInfo() << "Organization:" << cert.issuerInfo(QSslCertificate::Organization);
      qInfo() << "Unit:" << cert.issuerInfo(QSslCertificate::OrganizationalUnitName);
      qInfo() << "Country:" << cert.issuerInfo(QSslCertificate::CountryName);
      qInfo() << "State|Province:" << cert.issuerInfo(QSslCertificate::StateOrProvinceName);
      qInfo() << "Locality:" << cert.issuerInfo(QSslCertificate::LocalityName);
      qInfo() << "Email:" << cert.issuerInfo(QSslCertificate::EmailAddress);
      qInfo() << "SerialNumber:" << cert.issuerInfo(QSslCertificate::SerialNumber);
  }
}
