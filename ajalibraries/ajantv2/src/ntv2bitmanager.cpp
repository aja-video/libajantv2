/**
	@file		ntv2bitmanager.cpp
	@brief		Implementation of CNTV2BitManager class.
	@copyright	(C) 2019 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
**/
#include "ntv2bitmanager.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include <iostream>
#include <sys/stat.h>
#include <assert.h>
#if defined (AJALinux) || defined (AJAMac)
	#include <arpa/inet.h>
#endif
#include <map>

using namespace std;


