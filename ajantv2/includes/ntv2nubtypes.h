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

#if !defined(NTV2_DEPRECATE_16_3)
	//	In SDK 16.3 or later, client/server RPC implementations are plugins, each with their own protocols/versioning
	typedef ULWord NTV2NubProtocolVersion;
#endif	//	!defined(NTV2_DEPRECATE_16_3)

typedef std::vector<uint8_t>	RPCBlob;

namespace ntv2nub
{
	const bool kDisableByteSwap (true);
	const bool kEnableByteSwapIfNeeded (false);

	/**
		@name	PUSH Functions
		@brief	These functions are used to encode 8/16/32/64-bit fields of NTV2 objects/structs into a byte sequence to send to a remote machine.
	**/
	///@{

	/**
		@brief		Pushes an 8-bit value onto the given byte sequence.
		@param[in]	val		The 8-bit value to be appeneded to the byte sequence.
		@param		blob	A non-const reference to the byte sequence to be appended.
	**/
	inline void PUSHU8(const uint8_t val, RPCBlob & blob)
	{
		blob.push_back(val);
	}

	/**
		@brief		Pushes a 16-bit value onto the given byte sequence.
		@param[in]	val			The 16-bit value to be appeneded to the byte sequence.
		@param		blob		A non-const reference to the byte sequence to be appended.
		@param[in]	dontSwap	If false, the default, the value is byte-swapped prior to appending, if needed;
								otherwise it's appended in native byte order.
	**/
	inline void PUSHU16(const uint16_t val, RPCBlob & blob, const bool dontSwap = false)
	{
		const uint16_t _u16 ((NTV2HostIsBigEndian || dontSwap) ? val : NTV2EndianSwap16HtoB(val));
		const UByte * _pU16 (reinterpret_cast<const UByte*>(&_u16));
		blob.push_back(_pU16[0]); blob.push_back(_pU16[1]);
	}

	/**
		@brief		Pushes a 32-bit value onto the given byte sequence.
		@param[in]	val			The 32-bit value to be appeneded to the byte sequence.
		@param		blob		A non-const reference to the byte sequence to be appended.
		@param[in]	dontSwap	If false, the default, the value is byte-swapped prior to appending;
								otherwise it is appended using the native byte-ordering.
	**/
	inline void PUSHU32(const uint32_t val, RPCBlob & blob, const bool dontSwap = false)
	{
		const uint32_t _u32 ((NTV2HostIsBigEndian || dontSwap) ? val : NTV2EndianSwap32HtoB(val));
		const UByte * _pU32 (reinterpret_cast<const UByte*>(&_u32));
		blob.push_back(_pU32[0]); blob.push_back(_pU32[1]);
		blob.push_back(_pU32[2]); blob.push_back(_pU32[3]);
	}

	/**
		@brief		Pushes a 64-bit value onto the given byte sequence.
		@param[in]	val			The 64-bit value to be appeneded to the byte sequence.
		@param		blob		A non-const reference to the byte sequence to be appended.
		@param[in]	dontSwap	If false, the default, the value is byte-swapped prior to appending;
								otherwise it is appended using the native byte-ordering.
	**/
	inline void PUSHU64(const uint64_t val, RPCBlob & blob, const bool dontSwap = false)
	{
		const uint64_t _u64 ((NTV2HostIsBigEndian || dontSwap) ? val : NTV2EndianSwap64HtoB(val));
		const UByte * _pU64 (reinterpret_cast<const UByte*>(&_u64));
		blob.push_back(_pU64[0]); blob.push_back(_pU64[1]);
		blob.push_back(_pU64[2]); blob.push_back(_pU64[3]);
		blob.push_back(_pU64[4]); blob.push_back(_pU64[5]);
		blob.push_back(_pU64[6]); blob.push_back(_pU64[7]);
	}
	///@}

	/**
		@name	POP Functions
		@brief	These functions are used to decode NTV2 objects/structs from a received byte sequence.
	**/
	///@{

	/**
		@brief		Pops an 8-bit value off the given byte sequence at the given zero-based index position.
		@param[out]	outVal		Receives the 8-bit value popped from the byte sequence.
		@param[in]	blob		The source byte sequence.
		@param[out]	inOutNdx	On entry, contains the zero-based index position in the byte sequence from
								which the one byte comprising the value will be popped. On exit, contains the
								index position immediately past the popped byte value.
	**/
	inline void POPU8 (uint8_t & outVal, const RPCBlob & blob, std::size_t & inOutNdx)
	{
		outVal = blob.at(inOutNdx++);
	}

	/**
		@brief		Pops a 16-bit value off the given byte sequence at the given zero-based index position.
		@param[out]	outVal		Receives the 16-bit value popped from the byte sequence.
		@param[in]	blob		The source byte sequence.
		@param[out]	inOutNdx	On entry, contains the zero-based index position in the byte sequence from
								which the 2 bytes comprising the value will be popped. On exit, contains the
								index position immediately past the popped 2-byte value.
		@param[in]	dontSwap	If false, the default, the value is byte-swapped after popping;
								otherwise the value remains in native byte order.
	**/
	inline void POPU16 (uint16_t & outVal, const RPCBlob & blob, std::size_t & inOutNdx, const bool dontSwap = false)
	{
		uint16_t _u16(0);
		UByte * _pU8(reinterpret_cast<UByte*>(&_u16));
		_pU8[0] = blob.at(inOutNdx++); _pU8[1] = blob.at(inOutNdx++);
		outVal = (NTV2HostIsBigEndian || dontSwap) ? _u16 : NTV2EndianSwap16BtoH(_u16);
	}

	/**
		@brief		Pops a 32-bit value off the given byte sequence at the given zero-based index position.
		@param[out]	outVal		Receives the 32-bit value popped from the byte sequence.
		@param[in]	blob		The source byte sequence.
		@param[out]	inOutNdx	On entry, contains the zero-based index position in the byte sequence from
								which the 4 bytes comprising the value will be popped. On exit, contains the
								index position immediately past the popped 4-byte value.
		@param[in]	dontSwap	If false, the default, the value is byte-swapped after popping;
								otherwise the value remains in native byte order.
	**/
	inline void POPU32 (uint32_t & outVal, const RPCBlob & blob, std::size_t & inOutNdx, const bool dontSwap = false)
	{
		uint32_t _u32(0);
		UByte * _pU8(reinterpret_cast<UByte*>(&_u32));
		_pU8[0] = blob.at(inOutNdx++); _pU8[1] = blob.at(inOutNdx++);
		_pU8[2] = blob.at(inOutNdx++); _pU8[3] = blob.at(inOutNdx++);
		outVal = (NTV2HostIsBigEndian || dontSwap) ? _u32 : NTV2EndianSwap32BtoH(_u32);
	}

	/**
		@brief		Pops a 64-bit value off the given byte sequence at the given zero-based index position.
		@param[out]	outVal		Receives the 64-bit value popped from the byte sequence.
		@param[in]	blob		The source byte sequence.
		@param[out]	inOutNdx	On entry, contains the zero-based index position in the byte sequence from
								which the 8 bytes comprising the value will be popped. On exit, contains the
								index position immediately past the popped 8-byte value.
		@param[in]	dontSwap	If false, the default, the value is byte-swapped after popping;
								otherwise the value remains in native byte order.
	**/
	inline void POPU64 (uint64_t & outVal, const RPCBlob & blob, std::size_t & inOutNdx, const bool dontSwap = false)
	{
		uint64_t _u64(0);
		UByte * _pU8(reinterpret_cast<UByte*>(&_u64));
		_pU8[0] = blob.at(inOutNdx++); _pU8[1] = blob.at(inOutNdx++);
		_pU8[2] = blob.at(inOutNdx++); _pU8[3] = blob.at(inOutNdx++);
		_pU8[4] = blob.at(inOutNdx++); _pU8[5] = blob.at(inOutNdx++);
		_pU8[6] = blob.at(inOutNdx++); _pU8[7] = blob.at(inOutNdx++);
		outVal = (NTV2HostIsBigEndian || dontSwap) ? _u64 : NTV2EndianSwap64BtoH(_u64);
	}
	///@}

}	//	namespace ntv2nub

#endif	//	__NTV2NUBTYPES_H
