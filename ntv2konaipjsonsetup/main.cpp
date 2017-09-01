#include <QCoreApplication>
#include <QCommandLineParser>
#include "konaipjsonsetup.h"
#include "QDebug"
#include "keywords.h"

int main(int argc, char *argv[])
{
// This will tell the application at runtime to look in the 'qtlibs' directory
// for the QPA platform library, on Linux something like libqxcb.so
#if defined(AJA_BUNDLE_QT_LIBS_FOR_SDK)
     qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", QByteArray("qtlibs"));
#endif

    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Kona IP Json Setup");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption deviceOption("d", "which device to use", "device");
    parser.addOption(deviceOption);
    const QCommandLineOption boardOption("b", "which device to use", "board");
    parser.addOption(boardOption);
    const QCommandLineOption keywordsOption("k", "list supported JSON keywords");
    parser.addOption(keywordsOption);
    parser.addPositionalArgument("InputJsonFile", QCoreApplication::translate("main", "Json File to Open."));

    parser.process(a);

    if (parser.isSet(keywordsOption))
    {
        std::cout << keywordList.toStdString() << std::endl;
        exit(0);
    }


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
            std::cout << "Failed to parse JSON" << std::endl;
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
