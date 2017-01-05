/**
	@file		atomic.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJAAtomic class.
**/

#ifndef AJA_ATOMIC_H
#define AJA_ATOMIC_H

#include "ajabase/common/public.h"

/** 
 *	Collection of system independent atomic functions.
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJAAtomic
{
public:

	AJAAtomic();
	virtual ~AJAAtomic();

	/**
	 *	Exchange the pointer value with the target.
	 *
	 *	@param[in,out]	pTarget	The target of the exchange.
	 *	@param[in]		pValue	The value to exchange with the target.
	 *	@return					The value of the target before the exchange.
	 */
	static void* Exchange(void* volatile* pTarget, void* pValue);

	/**
	 *	Exchange the integer value with the target.
	 *
	 *	@param[in,out]	pTarget	The target of the exchange.
	 *	@param[in]		value	The value to exchange with the target.
	 *	@return					The value of the target before the exchange.
	 */
	static int32_t Exchange(int32_t volatile* pTarget, int32_t value);

	/**
	 *	Increment the integer target.
	 *
	 *	@param[in, out]	pTarget	The target to increment.
	 *	@return					The target value post increment.
	 */
	static int32_t Increment(int32_t volatile* pTarget);

	/**
	 *	Decrement the integer target.
	 *
	 *	@param[in,out]	pTarget	The target to decrement.
	 *	@return					The target value post decrement.
	 */
	static int32_t Decrement(int32_t volatile* pTarget);

	/**
	 *	Exchange unsigned integer value and target.
	 *
	 *	@param[in,out]	pTarget	The target of the exchange.
	 *	@param[in]		value	The value to exchange with the target.
	 *	@return					The value of the target before the exchange.
	 */
	static uint32_t Exchange(uint32_t volatile* pTarget, uint32_t value);

	/**
	 *	Increment the unsigned integer target.
	 *
	 *	@param[in,out]	pTarget	The target to increment.
	 *	@return					The target value post increment.
	 */
	static uint32_t Increment(uint32_t volatile* pTarget);

	/**
	 *	Decrement the unsigned integer target.
	 *
	 *	@param[in,out]	pTarget	The target to decrement.
	 *	@return					The target value post decrement.
	 */
	static uint32_t Decrement(uint32_t volatile* pTarget);
};

#endif	//	AJAAtomic
