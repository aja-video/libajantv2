/**
	@file		ntv2qtrawcapture/main.cpp
	@brief		Demonstration application that uses AutoCirculate to capture frames in raw format
				from a CION camera via SDI, display the captured video in a Qt window, and write
				the frames to a storage medium as DNG files.
	@copyright	Copyright (C) 2014 AJA Video Systems, Inc.	All rights reserved.
**/


//	Includes
#include "ntv2qtrawcapture.h"


#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	#include <QtWidgets>
#else
	#include <QtGui>
#endif


int main (int argc, char * argv [])
{
	QApplication		app (argc, argv);
	NTV2QtRawCapture	window;

	window.setWindowTitle ("NTV2 Qt Raw Capture");
	window.show ();

	return app.exec ();

}	//	main

