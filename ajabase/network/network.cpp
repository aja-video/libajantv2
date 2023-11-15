/* SPDX-License-Identifier: MIT */
/**
	@file		network.cpp
	@brief		Implements the AJANetwork class.
	@copyright	(C) 2011-2023 AJA Video Systems, Inc.  All rights reserved.
**/

/////////////////////////////
// Includes
/////////////////////////////
#include "network.h"

AJANetwork::AJANetwork(void)
{
}

AJANetwork::~AJANetwork(void)
{
}

in_addr_t AJANetwork::aja_inet_addr(const char *cp)
{
#if !defined(AJA_BAREMETAL)
  return inet_addr(cp);
#else
  in_addr_t t;
  return t;
#endif
}

char *AJANetwork::aja_inet_ntoa(struct in_addr in)
{
#if !defined(AJA_BAREMETAL)
  return inet_ntoa(in);
#else
  return "";
#endif
}

