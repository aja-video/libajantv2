/* SPDX-License-Identifier: MIT */
/**
	@file		variant.cpp
	@brief		Implements the AJAVariant class.
	@copyright	(C) 2010-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/common/variant.h"
#include "ajabase/common/common.h"

#ifdef AJA_USE_CPLUSPLUS11
    #include <cstdlib>
#else
    #include <stdlib.h>
#endif


AJAVariant::AJAVariant(AJAVariantType type)
	: mType(type)
{
    initialize();
}

void AJAVariant::initialize()
{
	switch(mType) {
		case AJA_VARIANT_BOOL:		mUnion.mBooleanValue = false;	break;
		case AJA_VARIANT_FLOAT:		mUnion.mFloatValue = 0.f;		break;
		case AJA_VARIANT_DOUBLE:	mUnion.mDoubleValue = 0.0;		break;
		case AJA_VARIANT_INT8:		mUnion.mInt8Value = 0;			break;
		case AJA_VARIANT_UINT8:		mUnion.mUInt8Value = 0;			break;
		case AJA_VARIANT_INT16:		mUnion.mInt16Value = 0;			break;
		case AJA_VARIANT_UINT16:	mUnion.mUInt16Value = 0;		break;
		case AJA_VARIANT_INT32:		mUnion.mInt32Value = 0;			break;
		case AJA_VARIANT_UINT32:	mUnion.mUInt32Value = 0;		break;
		case AJA_VARIANT_INT64:		mUnion.mInt64Value = 0;			break;
		case AJA_VARIANT_UINT64:	mUnion.mUInt64Value = 0;		break;
		case AJA_VARIANT_STRING:	mStringValue = std::string();	break;
		default:
		case AJA_VARIANT_NONE:		break;
	}
}

AJAVariant & AJAVariant::operator = (const AJAVariant & rhs)
{
	if (&rhs == this)
		return *this;
	mType = rhs.mType;
	switch (mType)
	{
		case AJA_VARIANT_BOOL:		mUnion.mBooleanValue = rhs.mUnion.mBooleanValue;	break;
		case AJA_VARIANT_FLOAT:		mUnion.mFloatValue = rhs.mUnion.mFloatValue;		break;
		case AJA_VARIANT_DOUBLE:	mUnion.mDoubleValue = rhs.mUnion.mDoubleValue;		break;
		case AJA_VARIANT_INT8:		mUnion.mInt8Value = rhs.mUnion.mInt8Value;			break;
		case AJA_VARIANT_UINT8:		mUnion.mUInt8Value = rhs.mUnion.mUInt8Value;		break;
		case AJA_VARIANT_INT16:		mUnion.mInt16Value = rhs.mUnion.mInt16Value;		break;
		case AJA_VARIANT_UINT16:	mUnion.mUInt16Value = rhs.mUnion.mUInt16Value;		break;
		case AJA_VARIANT_INT32:		mUnion.mInt32Value = rhs.mUnion.mInt32Value;		break;
		case AJA_VARIANT_UINT32:	mUnion.mUInt32Value = rhs.mUnion.mUInt32Value;		break;
		case AJA_VARIANT_INT64:		mUnion.mInt64Value = rhs.mUnion.mInt64Value;		break;
		case AJA_VARIANT_UINT64:	mUnion.mUInt64Value = rhs.mUnion.mUInt64Value;		break;
		case AJA_VARIANT_STRING:	mStringValue = rhs.mStringValue;					break;
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return *this;
}


// Type conversion methods
bool AJAVariant::AsBool() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue;
		case AJA_VARIANT_FLOAT:		return mUnion.mFloatValue ? true : false;
		case AJA_VARIANT_DOUBLE:	return mUnion.mDoubleValue ? true : false;
		case AJA_VARIANT_INT8:		return mUnion.mInt8Value ? true : false;
		case AJA_VARIANT_UINT8:		return mUnion.mUInt8Value ? true : false;
		case AJA_VARIANT_INT16:		return mUnion.mInt16Value ? true : false;
		case AJA_VARIANT_UINT16:	return mUnion.mUInt16Value ? true : false;
		case AJA_VARIANT_INT32:		return mUnion.mInt32Value ? true : false;
		case AJA_VARIANT_UINT32:	return mUnion.mUInt32Value ? true : false;
		case AJA_VARIANT_INT64:		return mUnion.mInt64Value ? true : false;
		case AJA_VARIANT_UINT64:	return mUnion.mUInt64Value ? true : false;
		case AJA_VARIANT_STRING:
		{
			if (mStringValue.empty())
				return false;
			std::string tmp(mStringValue);
			aja::lower(tmp);
			if (tmp == "true" || tmp == "1" || tmp == "y")
				return true;
			return false;
		}
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return false;
}

float AJAVariant::AsFloat() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1.f : 0.f;
		case AJA_VARIANT_FLOAT:		return mUnion.mFloatValue;
		case AJA_VARIANT_DOUBLE:	return float(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return float(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return float(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return float(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return float(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return float(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return float(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return float(mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return float(mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue.empty() ? 0.f
#ifdef AJA_USE_CPLUSPLUS11
																: std::strtof(mStringValue.data(), NULL);
#else
																: float(atof(mStringValue.data()));
#endif
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return 0.f;
}

double AJAVariant::AsDouble() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1.0 : 0.0;
		case AJA_VARIANT_FLOAT:		return mUnion.mFloatValue;
		case AJA_VARIANT_DOUBLE:	return mUnion.mDoubleValue;
		case AJA_VARIANT_INT8:		return double(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return double(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return double(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return double(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return double(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return double(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return double(mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return double(mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue.empty() ? 0.0 : strtod(mStringValue.data(), NULL);
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return 0.0;
}

int8_t AJAVariant::AsInt8() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1 : 0;
		case AJA_VARIANT_FLOAT:		return int8_t(mUnion.mFloatValue);
		case AJA_VARIANT_DOUBLE:	return int8_t(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return mUnion.mInt8Value;
		case AJA_VARIANT_UINT8:		return int8_t(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return int8_t(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return int8_t(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return int8_t(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return int8_t(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return int8_t(mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return int8_t(mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue.empty() ? 0 : int8_t(strtol(mStringValue.data(), NULL, 0));
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return 0;
}

uint8_t AJAVariant::AsUInt8() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1 : 0;
		case AJA_VARIANT_FLOAT:		return uint8_t(mUnion.mFloatValue);
		case AJA_VARIANT_DOUBLE:	return uint8_t(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return uint8_t(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return mUnion.mUInt8Value;
		case AJA_VARIANT_INT16:		return uint8_t(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return uint8_t(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return uint8_t(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return uint8_t(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return uint8_t(mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return uint8_t(mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue.empty() ? 0 : uint8_t(strtol(mStringValue.data(), NULL, 0));
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return 0;
}

int16_t AJAVariant::AsInt16() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1 : 0;
		case AJA_VARIANT_FLOAT:		return int16_t(mUnion.mFloatValue);
		case AJA_VARIANT_DOUBLE:	return int16_t(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return int16_t(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return int16_t(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return mUnion.mInt16Value;
		case AJA_VARIANT_UINT16:	return int16_t(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return int16_t(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return int16_t(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return int16_t(mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return int16_t(mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue.empty() ? 0 : int16_t(strtol(mStringValue.data(), NULL, 0));
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return 0;
}

uint16_t AJAVariant::AsUInt16() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1 : 0;
		case AJA_VARIANT_FLOAT:		return uint16_t(mUnion.mFloatValue);
		case AJA_VARIANT_DOUBLE:	return uint16_t(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return uint16_t(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return uint16_t(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return uint16_t(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return mUnion.mUInt16Value;
		case AJA_VARIANT_INT32:		return uint16_t(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return uint16_t(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return uint16_t(mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return uint16_t(mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue.empty() ? 0 : uint16_t(strtol(mStringValue.data(), NULL, 0));
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return 0;
}

int32_t AJAVariant::AsInt32() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1 : 0;
		case AJA_VARIANT_FLOAT:		return int32_t(mUnion.mFloatValue);
		case AJA_VARIANT_DOUBLE:	return int32_t(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return int32_t(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return int32_t(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return int32_t(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return int32_t(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return mUnion.mInt32Value;
		case AJA_VARIANT_UINT32:	return int32_t(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return int32_t(mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return int32_t(mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue.empty() ? 0 : int32_t(strtol(mStringValue.data(), NULL, 0));
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return 0;
}

uint32_t AJAVariant::AsUInt32() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1 : 0;
		case AJA_VARIANT_FLOAT:		return uint32_t(mUnion.mFloatValue);
		case AJA_VARIANT_DOUBLE:	return uint32_t(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return uint32_t(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return uint32_t(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return uint32_t(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return uint32_t(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return uint32_t(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return mUnion.mUInt32Value;
		case AJA_VARIANT_INT64:		return uint32_t(mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return uint32_t(mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue.empty() ? 0 : strtoul(mStringValue.data(), NULL, 0);
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return 0;
}

int64_t AJAVariant::AsInt64() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1 : 0;
		case AJA_VARIANT_FLOAT:		return int64_t(mUnion.mFloatValue);
		case AJA_VARIANT_DOUBLE:	return int64_t(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return int64_t(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return int64_t(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return int64_t(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return int64_t(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return int64_t(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return int64_t(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return mUnion.mInt64Value;
		case AJA_VARIANT_UINT64:	return int64_t(mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue.empty() ? 0
#ifdef AJA_USE_CPLUSPLUS11
																: std::strtoll(mStringValue.data(), NULL, 0);
#else
																: atol(mStringValue.data());
#endif
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return 0;
}

uint64_t AJAVariant::AsUInt64() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? 1 : 0;
		case AJA_VARIANT_FLOAT:		return uint64_t(mUnion.mFloatValue);
		case AJA_VARIANT_DOUBLE:	return uint64_t(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return uint64_t(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return uint64_t(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return uint64_t(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return uint64_t(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return uint64_t(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return uint64_t(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return uint64_t(mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return mUnion.mUInt64Value;
		case AJA_VARIANT_STRING:
		{
			if (mStringValue.empty())
				return 0;
#ifdef AJA_USE_CPLUSPLUS11
			return std::strtoull(mStringValue.data(), NULL, 0);
#else
			char* endptr;
			const char* data = mStringValue.data();
#if defined(_MSC_VER) && _MSC_VER < 1900
			unsigned long value = _strtoui64(data, &endptr, 10);
#else
			unsigned long value = strtoull(data, &endptr, 10);
#endif
			if (data == endptr)
				return 0;
			return value;
#endif
		}
		default:
		case AJA_VARIANT_NONE:			break;
	}
	return 0;
}

std::string AJAVariant::AsString() const
{
	switch(mType)
	{
		case AJA_VARIANT_BOOL:		return mUnion.mBooleanValue ? "true" : "false";
		case AJA_VARIANT_FLOAT:		return aja::to_string(mUnion.mFloatValue);
		case AJA_VARIANT_DOUBLE:	return aja::to_string(mUnion.mDoubleValue);
		case AJA_VARIANT_INT8:		return aja::to_string(mUnion.mInt8Value);
		case AJA_VARIANT_UINT8:		return aja::to_string(mUnion.mUInt8Value);
		case AJA_VARIANT_INT16:		return aja::to_string(mUnion.mInt16Value);
		case AJA_VARIANT_UINT16:	return aja::to_string(mUnion.mUInt16Value);
		case AJA_VARIANT_INT32:		return aja::to_string(mUnion.mInt32Value);
		case AJA_VARIANT_UINT32:	return aja::to_string(mUnion.mUInt32Value);
		case AJA_VARIANT_INT64:		return aja::to_string((long long)mUnion.mInt64Value);
		case AJA_VARIANT_UINT64:	return aja::to_string((unsigned long long)mUnion.mUInt64Value);
		case AJA_VARIANT_STRING:	return mStringValue;
		default:
		case AJA_VARIANT_NONE:		break;
	}
	return std::string();
}

bool AJAVariant::IsNumeric() const
{
	switch(mType)
	{
		case AJA_VARIANT_FLOAT:
		case AJA_VARIANT_DOUBLE:
		case AJA_VARIANT_INT8:
		case AJA_VARIANT_UINT8:
		case AJA_VARIANT_INT16:
		case AJA_VARIANT_UINT16:
		case AJA_VARIANT_INT32:
		case AJA_VARIANT_UINT32:
		case AJA_VARIANT_INT64:
		case AJA_VARIANT_UINT64:
			return true;
		default:
			break;
	}
	return false;
}

bool AJAVariant::IsUnsigned() const
{
	switch(mType)
	{
		case AJA_VARIANT_UINT8:
		case AJA_VARIANT_UINT16:
		case AJA_VARIANT_UINT32:
		case AJA_VARIANT_UINT64:
			return true;
		default:
			break;
	}
	return false;
}

bool AJAVariant::IsSigned() const
{
	switch(mType)
	{
		case AJA_VARIANT_FLOAT:
		case AJA_VARIANT_DOUBLE:
		case AJA_VARIANT_INT8:
		case AJA_VARIANT_INT16:
		case AJA_VARIANT_INT32:
		case AJA_VARIANT_INT64:
			return true;
		default:
			break;
	}
	return false;
}
