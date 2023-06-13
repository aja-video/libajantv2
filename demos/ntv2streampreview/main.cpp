/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2streampreview/main.cpp
	@brief		Demonstration application that uses AutoCirculate to capture frames from a single
				SDI input and display the captured video in a Qt window, with or without audio.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.	All rights reserved.
**/

#include "ntv2streampreview.h"
#include "ajabase/system/debug.h"
#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

int main (int argc, char * argv [])
{
	QApplication	app (argc, argv);
	AJADebug::Open();

	NTV2StreamPreview	window;
	window.setWindowTitle ("NTV2 Stream Preview");
	window.show ();

	return app.exec ();

}	//	main
