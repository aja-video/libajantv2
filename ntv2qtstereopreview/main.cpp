/**
	@file		main.cpp
	@brief		Demonstration application that uses AutoCirculate to capture 3D video and display
				the captured video in a Qt window, with or without audio.
	@copyright	(C) 2013-2020 AJA Video Systems, Inc.	All rights reserved.
**/


#include "ntv2qtstereopreview.h"
#include <QApplication>


int main (int argc, char * argv [])
{
	QApplication		app (argc, argv);
	NTV2QtStereoPreview	window;

	window.setWindowTitle ("NTV2 Qt Stereo Preview");
	window.show ();

	return app.exec ();

}	//	main
