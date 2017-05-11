#include <QCoreApplication>
#include <QCommandLineParser>
#include "konaipj2ksetup.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Kona IP J2K Setup");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption deviceOption("d", "which device to use", "device");
    parser.addOption(deviceOption);
    const QCommandLineOption boardOption("b", "which device to use", "board");
    parser.addOption(boardOption);
    const QCommandLineOption listOption("l", "list video formats", "");
    parser.addOption(listOption);

    parser.addPositionalArgument("InputJsonFile", QCoreApplication::translate("main", "Json File to Open."));

    parser.process(a);

    if (parser.isSet(listOption))
    {
        CKonaIpJ2kJsonReader readJson;
        readJson.printVideoFormatMap();
    }
    else
    {
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
            CKonaIpJ2kJsonReader readJson;
            readJson.openJson(args.at(0));
            CKonaIpEncoderSetup ipEncoderSetup;
            ipEncoderSetup.setupBoard(devStr.c_str(), readJson.getKonaIpJ2kParams ());
            CKonaIpDecoderSetup ipDecoderSetup;
            ipDecoderSetup.setupBoard(devStr.c_str(), readJson.getKonaIpJ2kParams ());
        }
        else
            parser.showHelp();
    }
}
