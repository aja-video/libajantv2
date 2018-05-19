/**
	@file		ntv2qtmultiinput/main.cpp
	@brief		Demonstration application that uses AutoCirculate to capture frames from 1, 2, 3,
				or 4 SDI inputs and display the captured video in a Qt window, with or without audio.
	@copyright	Copyright (C) 2013 AJA Video Systems, Inc.	All rights reserved.
**/

#include "ntv2qtmultiinput.h"
#include "ajabase/system/debug.h"
#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	#include <QtWidgets>
#else
	#include <QtGui>
#endif


int main (int argc, char * argv [])
{
	QApplication		app (argc, argv);
	AJADebug::Open();

	NTV2QtMultiInput	window;
	window.setWindowTitle ("NTV2 Qt Multi-Input");
	window.show ();

	return app.exec ();

}	//	main
