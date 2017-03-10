#include <QCoreApplication>
#include <QCommandLineParser>
#include "konaipjsonsetup.h"
#include "QDebug"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Kona IP Json Setup");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption deviceOption("d", "which device to use", "device");
    parser.addOption(deviceOption);
    const QCommandLineOption boardOption("b", "which device to use", "board");
    parser.addOption(boardOption);
    parser.addPositionalArgument("InputJsonFile", QCoreApplication::translate("main", "Json File to Open."));

    parser.process(a);

    QString device = "0";
    if (parser.isSet(deviceOption))
    {
         device = parser.value(deviceOption);
    }

    if (parser.isSet(boardOption))
    {
         device = parser.value(boardOption);
    }
    std::string devStr = device.toUtf8().constData();

    const QStringList args = parser.positionalArguments();
    if ( args.size() == 1 )
    {
        qDebug() << args.at(0);

        CKonaIpJsonSetup jsonSetup;
        bool rv = jsonSetup.openJson(args.at(0));
        if (!rv)
        {
            cout << "Failed to parse JSON" << endl;
            exit (-1);
        }
        jsonSetup.setupBoard(devStr);
    }
    else
    {
        parser.showHelp();
        exit (-2);
    }
    exit(0);
}
