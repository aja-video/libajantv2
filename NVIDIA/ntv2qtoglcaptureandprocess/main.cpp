#include "ntv2qtoglcaptureandprocess.h"
#include "ajabase/common/options_popt.h"
#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif
#if defined(AJA_LINUX)
    #include <X11/Xlib.h>
#endif

int main(int argc, char *argv[])
{
	uint32_t	boardNumber     (0);                //  Which board to use

#if defined(AJA_LINUX)
	poptContext	optionsContext;

	//  Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",   'b',    POPT_ARG_INT,   &boardNumber,   0,  "which board to use",   "number of the board"  },
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//  Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, (const char**)argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

    XInitThreads();	// Drawing is done from worker threads, so active thread locking in X
#endif

	QApplication a(argc, argv);

	NTV2QtOglCaptureAndProcess w;

	w.boardChanged(boardNumber);
	w.Init();

	w.show();
	return a.exec();
}
