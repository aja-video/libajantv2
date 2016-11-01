#include <QCoreApplication>
#include <QCommandLineParser>
#include "konaipencodersetup.h"

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
        CKonaIpEncoderJsonReader readJson;
        readJson.openJson(args.at(0));
        CKonaIPEncoderSetup ipBoardSetup;
        ipBoardSetup.setupBoard(devStr.c_str(),readJson.getKonaIParams());
    }
    else
        parser.showHelp();

}
