/**
	@file		cli_utils.cpp
	@copyright	Copyright (C) 2011-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implementation of GetNumber, GetString and GetCharAsInt functions.
**/

#include "ajabase/common/cli_utils.h"
#include <stdio.h>
#include <sstream>

using namespace::std;


bool GetNumber(const string& prompt, uint32_t& value)
{
	char input[32];

	cout << prompt;
	cin.getline(input, 32);
	string str(input);

	return (0 != (bool)(istringstream(str) >> value));
}


bool GetString(const string& prompt, string& value)
{
	char input[32];

	cout << prompt;
	cin.getline(input, 32);
	value = input;
	return (0 != value.length());
}


bool GetCharAsInt(const string& prompt, int& value)
{
	string input;

	if (true == GetString(prompt, input))
	{
		value = (int) input[0];
		return (true);
	}
	return (false);
}
