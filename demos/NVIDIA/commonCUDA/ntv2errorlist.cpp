/* SPDX-License-Identifier: MIT */
/*
  This software is provided by AJA Video, Inc. "AS IS"
  with no express or implied warranties.
*/

#include "ntv2errorlist.h"

CNTV2ErrorList::CNTV2ErrorList()
{
	
}

CNTV2ErrorList::~CNTV2ErrorList()
{
	
}

void CNTV2ErrorList::Error(const std::string& message)
{
	mErrors.push_back(message);
}

void CNTV2ErrorList::Acquire(CNTV2ErrorList& errorList)
{
	mErrors.insert(mErrors.end(),
		errorList.mErrors.begin(), errorList.mErrors.end());
	
	errorList.mErrors.clear();
}

std::string CNTV2ErrorList::GetErrorMessage() const
{
	std::string result;
	for(std::list<std::string>::const_iterator itr = mErrors.begin();
		itr != mErrors.end();
		itr++)
	{
		result += "error: " + (*itr) + "\n";
	}
	
	return result;
}

void CNTV2ErrorList::Clear()
{
	mErrors.clear();
}

