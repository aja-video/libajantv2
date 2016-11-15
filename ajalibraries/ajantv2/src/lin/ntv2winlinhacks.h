/* 
   	Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 AJA Video Systems, Inc. 
	Proprietary and Confidential information.  All rights reserved

	A bunch of macros to make windows API calls work in Linux without painful changes

 */

#ifndef NTV2WINLINHACKS_H
#define NTV2WINLINHACKS_H

#if defined(AJALinux)

#if !defined(AJA_LINUX)  // Sleep is in ajabase
#define Sleep(x) usleep(x*1000)
#endif
#define odprintf printf
#define MessageBox(a,b,c,d) fprintf(stderr,"%s\n", b);
#define OutputDebugString(x) fprintf(stderr,"%s\n",x)


#endif // defined(AJALinux)

#endif // NTV2WINLINHACKS_H
