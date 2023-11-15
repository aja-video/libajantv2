/* SPDX-License-Identifier: MIT */
/**
	@file		ip_socket.cpp
	@brief		Implements the AJAIPSocket class.
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
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

int AJANetwork::aja_inet_aton(const char *cp, struct in_addr *inp)
{
#if !defined(AJA_BAREMETAL)
  return inet_aton(cp, inp);
#else
  return 0;
#endif
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

in_addr_t AJANetwork::aja_inet_network(const char *cp)
{
#if !defined(AJA_BAREMETAL)
  return inet_network(cp);
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

struct in_addr AJANetwork::aja_inet_makeaddr(int net, int host)
{
#if !defined(AJA_BAREMETAL)
  return inet_makeaddr(net, host);
#else
  struct in_addr t;
  return t;
#endif
}

in_addr_t AJANetwork::aja_inet_lnaof(struct in_addr in)
{
#if !defined(AJA_BAREMETAL)
  return inet_lnaof(in);
#else
  in_addr_t t;
  return t;
#endif
}

in_addr_t AJANetwork::aja_inet_netof(struct in_addr in)
{
#if !defined(AJA_BAREMETAL)
  return inet_netof(in);
#else
  in_addr_t t;
  return t;
#endif
}

