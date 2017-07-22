#include "application.h"
#include "message.pb.h"
#include "socket.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>

#include <iostream>

namespace
{

void displayCertificateInformations(const QSslCertificate &cert)
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

void displayCertificateIssuerInformations(const QSslCertificate &cert)
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

} // anonymous namespace

namespace protobuf_client_example
{

Application::Application()
    : m_socket(new Socket([&](const QSslError &error){ return onSslError(error); }))
{
    QObject::connect(m_socket, &Socket::connected, this, &Application::onConnected);
    QObject::connect(m_socket, &Socket::disconnected, this, &Application::onDisconnected);
    QObject::connect(m_socket, &Socket::messageReceived, this, &Application::onMessageReceived);
}

void Application::connectToHost(const QString &hostname, int port) {
    qInfo("Application started");
    qInfo("Connecting to %s:%d", qUtf8Printable(hostname), port);
    m_socket->connectToHost(hostname, port);
}

void Application::onConnected() {
    qDebug("Socket connected");
    protobuf_client_example::protocol::Message message;
    message.set_type(protobuf_client_example::protocol::Message_Type_GET_EVENTS_REQUEST);
    protocol::GetEventRequest* request =  message.mutable_geteventrequest();
    m_socket->sendMessage(message);
}

void Application::onDisconnected(QAbstractSocket::SocketError error, const QString &errorString) {
    qInfo("Socket disconnected, closing the application");
    if (error != QSslSocket::UnknownSocketError)
        qWarning("Socket error is %s", qUtf8Printable(errorString));
    qApp->quit();
}

void Application::onMessageReceived(const protobuf_client_example::protocol::Message &message)
{
    switch (message.type())
    {
    case protocol::Message_Type_GET_EVENTS_REPLY:
        qDebug("GetEventsReply received");
        qDebug("Quitting the application");
        qApp->quit();
        break;
    default:
        qWarning("Unknown message received");
    }
}

bool Application::onSslError(const QSslError &error)
{
    qInfo("****************************************");
    qInfo("Ssl error connecting to %s:%d", qUtf8Printable(m_socket->hostName()), m_socket->port());
    qInfo("Error: %s", qUtf8Printable(error.errorString()));
    qInfo("****************************************");

    QSslCertificate cert = error.certificate();

    std::string action;
    while (true) {
        if (action == "y")
            return true;
        else if (action == "n")
            return false;
        else if (action == "c") {
            ::displayCertificateInformations(cert);
            action = std::string();
        }
        else if (action == "s") {
            ::displayCertificateIssuerInformations(cert);
            action = std::string();
        }
        if (!action.empty())
            qInfo("Please input \"y\", \"n\", \"c\", \"s\"");
        qInfo() << "";
        qInfo("Do you wish to continue?");
        qInfo("\"y\" Yes | \"n\" No | \"c\" Cert Info | \"s\" Issuer Info");
        std::getline(std::cin, action);
    }

    return false;
}

} // namespace protobuf_client_example
