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


CNTV2BitManager::CNTV2BitManager()
{
}

void CNTV2BitManager::~CNTV2BitManager()
{
	Clear();
}

bool CNTV2BitManager::AddFile (const std::string & inBitfilePath)
{
	CNTV2Bitfile Bitfile;
	NTV2BitfileInfo Info;
	
	if (!Bitfile.Open(inBitfilePath)) return false;

	Info.bitfilePath = inBitfilePath;
}

bool CNTV2BitManager::AddDirectory (const std::string & inDirectory)
{
}

void CNTV2BitManager::Clear (void)
{
}

size_t CNTV2BitManager::GetNumBitfiles (void)
{
}



