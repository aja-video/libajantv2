#include "ntv2dpxplayer.h"
#include <QtGui/QApplication>
#include "ajainternal/gui/ajaapplication.h"

int main(int argc, char *argv[])
{
	const char *arg1;

	AjaApplication a(argc, argv, AjaStyleLookSlate);

	if ( argc > 1 )
		arg1 = argv[1];
	else
		arg1 = NULL;

	NTV2DPXPlayer w(0, 0, arg1);
	w.show();
	return a.exec();
}
