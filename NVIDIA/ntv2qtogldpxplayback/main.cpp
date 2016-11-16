#include "ntv2qtogldpxplayback.h"
#include <QtGui/QApplication>


int main(int argc, char *argv[])
{
	const char *arg1;


	QApplication a(argc, argv);

	if ( argc > 1 )
		arg1 = argv[1];
	else
		arg1 = NULL;

	NTV2QtOglDPXPlayback w;
	w.show();
	return a.exec();
}
