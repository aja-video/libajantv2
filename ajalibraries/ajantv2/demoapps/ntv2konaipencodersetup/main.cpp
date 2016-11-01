#include <QCoreApplication>
#include <QCommandLineParser>
#include "konaipencodersetup.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Kona IP Encoder Setup");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("InputJsonFile", QCoreApplication::translate("main", "Json File to Open."));

    parser.process(a);
    const QStringList args = parser.positionalArguments();
    if ( args.size() == 1 )
    {
        CKonaIpEncoderJsonReader readJson;
        readJson.openJson(args.at(0));
        CKonaIPEncoderSetup ipBoardSetup;
        ipBoardSetup.setupBoard((char *)"0",readJson.getKonaIParams());
    }
    else
        parser.showHelp();

}
