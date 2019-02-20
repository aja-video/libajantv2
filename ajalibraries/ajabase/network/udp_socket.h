/**
	@file		udp_socket.h
	@copyright	Copyright (C) 2011-2019 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJAUDPSocket class.
**/

#ifndef AJA_UDP_SOCKET_H
#define AJA_UDP_SOCKET_H

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/network/ip_socket.h"


/////////////////////////////
// Declarations
/////////////////////////////
class AJA_EXPORT AJAUDPSocket : public AJAIPSocket
{
	public:
		AJAUDPSocket(void);
		virtual ~AJAUDPSocket(void);

        AJAStatus Open(const std::string& ipAddress, uint16_t port);

		uint32_t Poll(
					uint8_t*            pData,
					uint32_t            dataLength,
					struct sockaddr_in& client,
					int                 timeout);
		uint32_t Read(uint8_t* pData, uint32_t dataLength, struct sockaddr_in& client);
		uint32_t Write(
					const uint8_t*      pData,
					uint32_t            dataLength,
					struct sockaddr_in& targetAddress);


	private:
};

#endif	//	AJA_UDP_SOCKET_H
