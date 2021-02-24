/* SPDX-License-Identifier: MIT */
/*
  This software is provided by AJA Video, Inc. "AS IS"
  with no express or implied warranties.
*/

#ifndef _NTV2ERRORLIST_H
#define _NTV2ERRORLIST_H

#include <string>
#include <list>

class CNTV2ErrorList;

class CNTV2ErrorList
{
public:
	CNTV2ErrorList();
	virtual ~CNTV2ErrorList();
	
	void Error(const std::string& message);
	void Acquire(CNTV2ErrorList& errorList);
	
	std::string GetErrorMessage() const;
	void Clear();

private:
	std::list<std::string> mErrors;
};

#endif

