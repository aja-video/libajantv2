/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubtypes.h
	@brief		Declares data types and structures used in NTV2 "nub" packets.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/

#ifndef __NTV2NUBTYPES_H
#define __NTV2NUBTYPES_H

#include "ntv2endian.h"
#include <vector>

#define NTV2NUBPORT		7575	//	Default port we listen on

#if !defined(NTV2_DEPRECATE_16_3)
	//	In SDK 16.3 or later, client/server RPC implementations are plugins, each with their own protocols/versioning
	typedef ULWord NTV2NubProtocolVersion;
#endif	//	!defined(NTV2_DEPRECATE_16_3)

namespace ntv2nub
{
	const bool kDisableByteSwap (true);
	const bool kEnableByteSwapIfNeeded (false);

	inline void PUSHU8(const uint8_t inVal, std::vector<uint8_t> & inArr)
	{
		inArr.push_back(inVal);
	}

	inline void PUSHU16(const uint16_t inVal, std::vector<uint8_t> & inArr, const bool dontSwap = false)
	{
		const uint16_t _u16 ((NTV2HostIsBigEndian || dontSwap) ? inVal : NTV2EndianSwap16HtoB(inVal));
		const UByte * _pU16 (reinterpret_cast<const UByte*>(&_u16));
		inArr.push_back(_pU16[0]); inArr.push_back(_pU16[1]);
	}

	inline void PUSHU32(const uint32_t inVal, std::vector<uint8_t> & inArr, const bool dontSwap = false)
	{
		const uint32_t _u32 ((NTV2HostIsBigEndian || dontSwap) ? inVal : NTV2EndianSwap32HtoB(inVal));
		const UByte * _pU32 (reinterpret_cast<const UByte*>(&_u32));
		inArr.push_back(_pU32[0]); inArr.push_back(_pU32[1]);
		inArr.push_back(_pU32[2]); inArr.push_back(_pU32[3]);
	}

	inline void PUSHU64(const uint64_t inVal, std::vector<uint8_t> & inArr, const bool dontSwap = false)
	{
		const uint64_t _u64 ((NTV2HostIsBigEndian || dontSwap) ? inVal : NTV2EndianSwap64HtoB(inVal));
		const UByte * _pU64 (reinterpret_cast<const UByte*>(&_u64));
		inArr.push_back(_pU64[0]); inArr.push_back(_pU64[1]);
		inArr.push_back(_pU64[2]); inArr.push_back(_pU64[3]);
		inArr.push_back(_pU64[4]); inArr.push_back(_pU64[5]);
		inArr.push_back(_pU64[6]); inArr.push_back(_pU64[7]);
	}

	inline void POPU8 (uint8_t & outVal, const std::vector<uint8_t> & inArr, std::size_t & inOutNdx)
	{
		outVal = inArr.at(inOutNdx++);
	}

	inline void POPU16 (uint16_t & outVal, const std::vector<uint8_t> & inArr, std::size_t & inOutNdx, const bool dontSwap = false)
	{
		uint16_t _u16(0);
		UByte * _pU8(reinterpret_cast<UByte*>(&_u16));
		_pU8[0] = inArr.at(inOutNdx++); _pU8[1] = inArr.at(inOutNdx++);
		outVal = (NTV2HostIsBigEndian || dontSwap) ? _u16 : NTV2EndianSwap16BtoH(_u16);
	}

	inline void POPU32 (uint32_t & outVal, const std::vector<uint8_t> & inArr, std::size_t & inOutNdx, const bool dontSwap = false)
	{
		uint32_t _u32(0);
		UByte * _pU8(reinterpret_cast<UByte*>(&_u32));
		_pU8[0] = inArr.at(inOutNdx++); _pU8[1] = inArr.at(inOutNdx++);
		_pU8[2] = inArr.at(inOutNdx++); _pU8[3] = inArr.at(inOutNdx++);
		outVal = (NTV2HostIsBigEndian || dontSwap) ? _u32 : NTV2EndianSwap32BtoH(_u32);
	}

	inline void POPU64 (uint64_t & outVal, const std::vector<uint8_t> & inArr, std::size_t & inOutNdx, const bool dontSwap = false)
	{
		uint32_t _u64(0);
		UByte * _pU8(reinterpret_cast<UByte*>(&_u64));
		_pU8[0] = inArr.at(inOutNdx++); _pU8[1] = inArr.at(inOutNdx++);
		_pU8[2] = inArr.at(inOutNdx++); _pU8[3] = inArr.at(inOutNdx++);
		_pU8[4] = inArr.at(inOutNdx++); _pU8[5] = inArr.at(inOutNdx++);
		_pU8[6] = inArr.at(inOutNdx++); _pU8[7] = inArr.at(inOutNdx++);
		outVal = (NTV2HostIsBigEndian || dontSwap) ? _u64 : NTV2EndianSwap64BtoH(_u64);
	}

}	//	namespace ntv2nub

#endif	//	__NTV2NUBTYPES_H
