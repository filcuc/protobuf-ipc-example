#include "global.h"
#include "application.h"

#include <QCoreApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("Client");
    app.setApplicationVersion("0.0.1");

    QCommandLineOption portOption {"port", "Connect port", "number", "56323"};
    QCommandLineOption hostnameOption {"hostname", "Connect hostname", "hostname", "localhost"};

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(portOption);
    parser.addOption(hostnameOption);

    parser.process(app);

    const QString hostname = parser.value(hostnameOption);
    bool ok = false;
    const int port = parser.value(portOption).toInt(&ok);
    if (!ok)
        return -1;

    protobuf_client_example::Application a;
    a.connectToHost(hostname, port);
    return app.exec();
}
