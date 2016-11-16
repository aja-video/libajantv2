/**
	@file		ntv2qtplaycorder/main.cpp  
	@brief		Demonstration application that records or plays frames to/from disk in DPX format.
	@copyright	Copyright (C) 2013 AJA Video Systems, Inc.  All rights reserved. 
**/

#include "ntv2qtplaycorder.h"
#include <QtGui/QApplication>

int main (int argc, char * argv [])
{
	QApplication		app (argc, argv);
	NTV2QtPlaycorder	window;

	window.setWindowTitle ("NTV2 Qt Playcorder");
	window.show ();

	return app.exec ();

}	//	main

