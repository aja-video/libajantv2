/**
	@file		ntv2qtpreview/main.cpp
	@brief		Demonstration application that uses AutoCirculate to capture frames from a single
				SDI input and display the captured video in a Qt window, with or without audio.
	@copyright	Copyright (C) 2013 AJA Video Systems, Inc.	All rights reserved.
**/

#include "ntv2qtpreview.h"

#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

int main (int argc, char * argv [])
{
	QApplication	app (argc, argv);
	NTV2QtPreview	window;

	window.setWindowTitle ("NTV2 Qt Preview");
	window.show ();

	return app.exec ();

}	//	main
