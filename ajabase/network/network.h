/* SPDX-License-Identifier: MIT */
/**
	@file		network.h
	@brief		Declares the AJANetwork class.
	@copyright	(C) 2011-2023 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_NETWORK_H
#define AJA_NETWORK_H

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/common/public.h"
#include "ajabase/system/system.h"
#include "ajabase/system/lock.h"
#include <string>
#include <map>

#if defined(AJA_LINUX) || defined(AJA_MAC)
	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <poll.h>
	#include <sys/socket.h>
#elif defined(AJA_WINDOWS)
	typedef int socklen_t;
  typedef uint32_t in_addr_t;
#endif

/////////////////////////////
// Typedefs
/////////////////////////////
#if defined(AJA_BAREMETAL)
typedef uint32_t in_addr_t;
struct in_addr {
  in_addr_t       s_addr;
};
typedef unsigned short sa_family_t;
struct sockaddr_in {
  sa_family_t     sin_family;
  in_port_t       sin_port;
  struct in_addr  sin_addr;
};
typedef uint32_t socklen_t;
#endif

/////////////////////////////
// Declarations
/////////////////////////////
/**
 *	Class which handles generic network functions in a portable manner
 */
class AJA_EXPORT AJANetwork
{
	public:
		AJANetwork(void);
		virtual ~AJANetwork(void);

    static in_addr_t aja_inet_addr(const char *cp);
    static char *aja_inet_ntoa(struct in_addr in);
};


#endif // AJA_NETWORK_H
