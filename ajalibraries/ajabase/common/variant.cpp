/* SPDX-License-Identifier: MIT */
/**
	@file		variant.cpp
	@brief		Implements the AJAVariant class.
	@copyright	(C) 2010-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/common/variant.h"
#include "ajabase/common/common.h"

#include <cstdlib>

AJAVariant::AJAVariant()
: mType(AJA_VARIANT_INT32), mInt32Value(0)
{
}

AJAVariant::AJAVariant(AJAVariantType type)
: mType(type)
{
    initialize();
}

void AJAVariant::initialize()
{
    switch(mType) {
        case AJA_VARIANT_BOOL:
            mBooleanValue = false;
            break;
        case AJA_VARIANT_FLOAT:
            mFloatValue = 0.f;
            break;
        case AJA_VARIANT_DOUBLE:
            mDoubleValue = 0.0;
            break;
        case AJA_VARIANT_INT8:
            mInt8Value = 0;
            break;
        case AJA_VARIANT_UINT8:
            mUInt8Value = 0;
            break;
        case AJA_VARIANT_INT16:
            mInt16Value = 0;
            break;
        case AJA_VARIANT_UINT16:
            mUInt16Value = 0;
            break;
        case AJA_VARIANT_INT32:
            mInt32Value = 0;
            break;
        case AJA_VARIANT_UINT32:
            mUInt32Value = 0;
            break;
        case AJA_VARIANT_INT64:
            mInt64Value = 0;
            break;
        case AJA_VARIANT_UINT64:
            mUInt64Value = 0;
            break;
        case AJA_VARIANT_STRING:
            mStringValue = std::string();
            break;
        default:
        case AJA_VARIANT_NONE:
            break;
    }
}

AJAVariant::AJAVariant(bool value)
: mType(AJA_VARIANT_BOOL), mBooleanValue(value) {}
AJAVariant::AJAVariant(float value)
: mType(AJA_VARIANT_FLOAT), mFloatValue(value) {}
AJAVariant::AJAVariant(double value)
: mType(AJA_VARIANT_DOUBLE), mDoubleValue(value) {}
AJAVariant::AJAVariant(int8_t value)
: mType(AJA_VARIANT_INT8), mInt8Value(value) {}
AJAVariant::AJAVariant(uint8_t value)
: mType(AJA_VARIANT_UINT8), mUInt8Value(value) {}
AJAVariant::AJAVariant(int16_t value)
: mType(AJA_VARIANT_INT16), mInt16Value(value) {}
AJAVariant::AJAVariant(uint16_t value)
: mType(AJA_VARIANT_UINT16), mUInt16Value(value) {}
AJAVariant::AJAVariant(int32_t value)
: mType(AJA_VARIANT_INT32), mInt32Value(value) {}
AJAVariant::AJAVariant(uint32_t value)
: mType(AJA_VARIANT_UINT32), mUInt32Value(value) {}
AJAVariant::AJAVariant(int64_t value)
: mType(AJA_VARIANT_INT64), mInt64Value(value) {}
AJAVariant::AJAVariant(uint64_t value)
: mType(AJA_VARIANT_UINT64), mUInt64Value(value) {}
AJAVariant::AJAVariant(const char* value)
: mType(AJA_VARIANT_STRING), mStringValue(value) {}
AJAVariant::AJAVariant(const char* value, std::size_t length)
: mType(AJA_VARIANT_STRING), mStringValue(value, length) {}
AJAVariant::AJAVariant(const std::string& value)
: mType(AJA_VARIANT_STRING), mStringValue(value) {}

// copy ctor
AJAVariant::AJAVariant(const AJAVariant& other) {
    operator=(other);
}

void AJAVariant::operator=(const AJAVariant& other) {
    mType = other.mType;
    switch (mType) {
        case AJA_VARIANT_BOOL:
            mBooleanValue = other.mBooleanValue;
            break;
        case AJA_VARIANT_FLOAT:
            mFloatValue = other.mFloatValue;
            break;
        case AJA_VARIANT_DOUBLE:
            mDoubleValue = other.mDoubleValue;
            break;
        case AJA_VARIANT_INT8:
            mInt8Value = other.mInt8Value;
            break;
        case AJA_VARIANT_UINT8:
            mUInt8Value = other.mUInt8Value;
            break;
        case AJA_VARIANT_INT16:
            mInt16Value = other.mInt16Value;
            break;
        case AJA_VARIANT_UINT16:
            mUInt16Value = other.mUInt16Value;
            break;
        case AJA_VARIANT_INT32:
            mInt32Value = other.mInt32Value;
            break;
        case AJA_VARIANT_UINT32:
            mUInt32Value = other.mUInt32Value;
            break;
        case AJA_VARIANT_INT64:
            mInt64Value = other.mInt64Value;
            break;
        case AJA_VARIANT_UINT64:
            mUInt64Value = other.mUInt64Value;
            break;
        case AJA_VARIANT_STRING:
            mStringValue = std::string(other.mStringValue);
            break;
        default:
        case AJA_VARIANT_NONE:
            break;
    }
}

// Type conversion operators
AJAVariant::operator bool() const
{
    return AsBool();
}
AJAVariant::operator float() const
{
    return AsFloat();
}
AJAVariant::operator double() const
{
    return AsDouble();
}
AJAVariant::operator int8_t() const
{
    return AsInt8();
}
AJAVariant::operator uint8_t() const
{
    return AsUInt8();
}
AJAVariant::operator int16_t() const
{
    return AsInt16();
}
AJAVariant::operator uint16_t() const
{
    return AsUInt16();
}
AJAVariant::operator int32_t() const
{
    return AsInt32();
}
AJAVariant::operator uint32_t() const
{
    return AsUInt32();
}
AJAVariant::operator int64_t() const
{
    return AsInt64();
}
AJAVariant::operator uint64_t() const
{
    return AsUInt64();
}
AJAVariant::operator std::string() const
{
    return AsString();
}

// Getters
bool AJAVariant::GetBool() const {
    return mBooleanValue;
}
float AJAVariant::GetFloat() const {
    return mFloatValue;
}
double AJAVariant::GetDouble() const {
    return mDoubleValue;
}
int8_t AJAVariant::GetInt8() const {
    return mInt8Value;
}
uint8_t AJAVariant::GetUInt8() const {
    return mUInt8Value;
}
int16_t AJAVariant::GetInt16() const {
    return mInt16Value;
}
uint16_t AJAVariant::GetUInt16() const {
    return mUInt16Value;
}
int32_t AJAVariant::GetInt32() const {
    return mInt32Value;
}
uint32_t AJAVariant::GetUInt32() const {
    return mUInt32Value;
}
int64_t AJAVariant::GetInt64() const {
    return mInt64Value;
}
uint64_t AJAVariant::GetUInt64() const {
    return mUInt64Value;
}
const std::string& AJAVariant::GetString() const {
    return mStringValue;
}

// Setters
void AJAVariant::SetBool(bool value) {
    mBooleanValue = value;
    mType = AJA_VARIANT_BOOL;
}
void AJAVariant::SetFloat(float value) {
    mFloatValue = value;
    mType = AJA_VARIANT_FLOAT;
}
void AJAVariant::SetDouble(double value) {
    mDoubleValue = value;
    mType = AJA_VARIANT_DOUBLE;
}
void AJAVariant::SetInt8(int8_t value) {
    mInt8Value = value;
    mType = AJA_VARIANT_INT8;
}
void AJAVariant::SetUInt8(uint8_t value) {
    mUInt8Value = value;
    mType = AJA_VARIANT_UINT8;
}
void AJAVariant::SetInt16(int16_t value) {
    mInt16Value = value;
    mType = AJA_VARIANT_INT16;
}
void AJAVariant::SetUInt16(uint16_t value) {
    mUInt16Value = value;
    mType = AJA_VARIANT_UINT16;
}
void AJAVariant::SetInt32(int32_t value) {
    mInt32Value = value;
    mType = AJA_VARIANT_INT32;
}
void AJAVariant::SetUInt32(uint32_t value) {
    mUInt32Value = value;
    mType = AJA_VARIANT_UINT32;
}
void AJAVariant::SetInt64(int64_t value) {
    mInt64Value = value;
    mType = AJA_VARIANT_INT64;
}
void AJAVariant::SetUInt64(uint64_t value) {
    mUInt64Value = value;
    mType = AJA_VARIANT_UINT64;
}
void AJAVariant::SetString(const char* value) {
    mStringValue = std::string(value);
    mType = AJA_VARIANT_STRING;
}
void AJAVariant::SetString(const char* value, std::size_t length) {
    mStringValue = std::string(value, length);
    mType = AJA_VARIANT_STRING;
}
void AJAVariant::SetString(const std::string& value) {
    mStringValue = value;
    mType = AJA_VARIANT_STRING;
}

// Type conversion methods
bool AJAVariant::AsBool() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue;
        case AJA_VARIANT_FLOAT:
            return mFloatValue ? true : false;
        case AJA_VARIANT_DOUBLE:
            return mDoubleValue ? true : false;
        case AJA_VARIANT_INT8:
            return mInt8Value ? true : false;
        case AJA_VARIANT_UINT8:
            return mUInt8Value ? true : false;
        case AJA_VARIANT_INT16:
            return mInt16Value ? true : false;
        case AJA_VARIANT_UINT16:
            return mUInt16Value ? true : false;
        case AJA_VARIANT_INT32:
            return mInt32Value ? true : false;
        case AJA_VARIANT_UINT32:
            return mUInt32Value ? true : false;
        case AJA_VARIANT_INT64:
            return mInt64Value ? true : false;
        case AJA_VARIANT_UINT64:
            return mUInt64Value ? true : false;
        case AJA_VARIANT_STRING:
        {
            if (!mStringValue.empty()) {
                std::string tmp = std::string(mStringValue);
                std::string lower = aja::lower(tmp);
                if (tmp == "true")
                    return true;
                else
                    return false;
            }
            else
                return false;
        }
        default:
        case AJA_VARIANT_NONE:
            return false;
    }

    return false;
}

float AJAVariant::AsFloat() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1.f : 0.f;
        case AJA_VARIANT_FLOAT:
            return mFloatValue;
        case AJA_VARIANT_DOUBLE:
            return static_cast<float>(mDoubleValue);
        case AJA_VARIANT_INT8:
            return static_cast<float>(mInt8Value);
        case AJA_VARIANT_UINT8:
            return static_cast<float>(mUInt8Value);
        case AJA_VARIANT_INT16:
            return static_cast<float>(mInt16Value);
        case AJA_VARIANT_UINT16:
            return static_cast<float>(mUInt16Value);
        case AJA_VARIANT_INT32:
            return static_cast<float>(mInt32Value);
        case AJA_VARIANT_UINT32:
            return static_cast<float>(mUInt32Value);
        case AJA_VARIANT_INT64:
            return static_cast<float>(mInt64Value);
        case AJA_VARIANT_UINT64:
            return static_cast<float>(mUInt64Value);
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0.f;
            } else {
                return strtof(mStringValue.data(), NULL);
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0.f;
    }

    return 0.f;
}
double AJAVariant::AsDouble() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1.0 : 0.0;
        case AJA_VARIANT_FLOAT:
            return mFloatValue;
        case AJA_VARIANT_DOUBLE:
            return mDoubleValue;
        case AJA_VARIANT_INT8:
            return static_cast<double>(mInt8Value);
        case AJA_VARIANT_UINT8:
            return static_cast<double>(mUInt8Value);
        case AJA_VARIANT_INT16:
            return static_cast<double>(mInt16Value);
        case AJA_VARIANT_UINT16:
            return static_cast<double>(mUInt16Value);
        case AJA_VARIANT_INT32:
            return static_cast<double>(mInt32Value);
        case AJA_VARIANT_UINT32:
            return static_cast<double>(mUInt32Value);
        case AJA_VARIANT_INT64:
            return static_cast<double>(mInt64Value);
        case AJA_VARIANT_UINT64:
            return static_cast<double>(mUInt64Value);
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0.0;
            } else {
                return strtod(mStringValue.data(), NULL);
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0.0;
    }

    return 0.0;
}
int8_t AJAVariant::AsInt8() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1 : 0;
        case AJA_VARIANT_FLOAT:
            return static_cast<int8_t>(mFloatValue);
        case AJA_VARIANT_DOUBLE:
            return static_cast<int8_t>(mDoubleValue);
        case AJA_VARIANT_INT8:
            return mInt8Value;
        case AJA_VARIANT_UINT8:
            return static_cast<int8_t>(mUInt8Value);
        case AJA_VARIANT_INT16:
            return static_cast<int8_t>(mInt16Value);
        case AJA_VARIANT_UINT16:
            return static_cast<int8_t>(mUInt16Value);
        case AJA_VARIANT_INT32:
            return static_cast<int8_t>(mInt32Value);
        case AJA_VARIANT_UINT32:
            return static_cast<int8_t>(mUInt32Value);
        case AJA_VARIANT_INT64:
            return static_cast<int8_t>(mInt64Value);
        case AJA_VARIANT_UINT64:
            return static_cast<int8_t>(mUInt64Value);
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0;
            } else {
                return static_cast<int8_t>(strtol(mStringValue.data(), NULL, 0));
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0;
    }

    return 0;
}
uint8_t AJAVariant::AsUInt8() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1 : 0;
        case AJA_VARIANT_FLOAT:
            return static_cast<uint8_t>(mFloatValue);
        case AJA_VARIANT_DOUBLE:
            return static_cast<uint8_t>(mDoubleValue);
        case AJA_VARIANT_INT8:
            return static_cast<uint8_t>(mInt8Value);
        case AJA_VARIANT_UINT8:
            return mUInt8Value;
        case AJA_VARIANT_INT16:
            return static_cast<uint8_t>(mInt16Value);
        case AJA_VARIANT_UINT16:
            return static_cast<uint8_t>(mUInt16Value);
        case AJA_VARIANT_INT32:
            return static_cast<uint8_t>(mInt32Value);
        case AJA_VARIANT_UINT32:
            return static_cast<uint8_t>(mUInt32Value);
        case AJA_VARIANT_INT64:
            return static_cast<uint8_t>(mInt64Value);
        case AJA_VARIANT_UINT64:
            return static_cast<uint8_t>(mUInt64Value);
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0;
            } else {
                return static_cast<uint8_t>(strtol(mStringValue.data(), NULL, 0));
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0;
    }

    return 0;
}
int16_t AJAVariant::AsInt16() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1 : 0;
        case AJA_VARIANT_FLOAT:
            return static_cast<int16_t>(mFloatValue);
        case AJA_VARIANT_DOUBLE:
            return static_cast<int16_t>(mDoubleValue);
        case AJA_VARIANT_INT8:
            return static_cast<int16_t>(mInt8Value);
        case AJA_VARIANT_UINT8:
            return static_cast<int16_t>(mUInt8Value);
        case AJA_VARIANT_INT16:
            return mInt16Value;
        case AJA_VARIANT_UINT16:
            return static_cast<int16_t>(mUInt16Value);
        case AJA_VARIANT_INT32:
            return static_cast<int16_t>(mInt32Value);
        case AJA_VARIANT_UINT32:
            return static_cast<int16_t>(mUInt32Value);
        case AJA_VARIANT_INT64:
            return static_cast<int16_t>(mInt64Value);
        case AJA_VARIANT_UINT64:
            return static_cast<int16_t>(mUInt64Value);
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0;
            } else {
                return static_cast<int16_t>(strtol(mStringValue.data(), NULL, 0));
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0;
    }

    return 0;
}
uint16_t AJAVariant::AsUInt16() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1 : 0;
        case AJA_VARIANT_FLOAT:
            return static_cast<uint16_t>(mFloatValue);
        case AJA_VARIANT_DOUBLE:
            return static_cast<uint16_t>(mDoubleValue);
        case AJA_VARIANT_INT8:
            return static_cast<uint16_t>(mInt8Value);
        case AJA_VARIANT_UINT8:
            return static_cast<uint16_t>(mUInt8Value);
        case AJA_VARIANT_INT16:
            return static_cast<uint16_t>(mInt16Value);
        case AJA_VARIANT_UINT16:
            return mUInt16Value;
        case AJA_VARIANT_INT32:
            return static_cast<uint16_t>(mInt32Value);
        case AJA_VARIANT_UINT32:
            return static_cast<uint16_t>(mUInt32Value);
        case AJA_VARIANT_INT64:
            return static_cast<uint16_t>(mInt64Value);
        case AJA_VARIANT_UINT64:
            return static_cast<uint16_t>(mUInt64Value);
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0;
            } else {
                return static_cast<uint16_t>(strtol(mStringValue.data(), NULL, 0));
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0;
    }

    return 0;
}
int32_t AJAVariant::AsInt32() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1 : 0;
        case AJA_VARIANT_FLOAT:
            return static_cast<int32_t>(mFloatValue);
        case AJA_VARIANT_DOUBLE:
            return static_cast<int32_t>(mDoubleValue);
        case AJA_VARIANT_INT8:
            return static_cast<int32_t>(mInt8Value);
        case AJA_VARIANT_UINT8:
            return static_cast<int32_t>(mUInt8Value);
        case AJA_VARIANT_INT16:
            return static_cast<int32_t>(mInt16Value);
        case AJA_VARIANT_UINT16:
            return static_cast<int32_t>(mUInt16Value);
        case AJA_VARIANT_INT32:
            return mInt32Value;
        case AJA_VARIANT_UINT32:
            return static_cast<int32_t>(mUInt32Value);
        case AJA_VARIANT_INT64:
            return static_cast<int32_t>(mInt64Value);
        case AJA_VARIANT_UINT64:
            return static_cast<int32_t>(mUInt64Value);
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0;
            } else {
                return static_cast<int32_t>(strtol(mStringValue.data(), NULL, 0));
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0;
    }

    return 0;
}
uint32_t AJAVariant::AsUInt32() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1 : 0;
        case AJA_VARIANT_FLOAT:
            return static_cast<uint32_t>(mFloatValue);
        case AJA_VARIANT_DOUBLE:
            return static_cast<uint32_t>(mDoubleValue);
        case AJA_VARIANT_INT8:
            return static_cast<uint32_t>(mInt8Value);
        case AJA_VARIANT_UINT8:
            return static_cast<uint32_t>(mUInt8Value);
        case AJA_VARIANT_INT16:
            return static_cast<uint32_t>(mInt16Value);
        case AJA_VARIANT_UINT16:
            return static_cast<uint32_t>(mUInt16Value);
        case AJA_VARIANT_INT32:
            return static_cast<uint32_t>(mInt32Value);
        case AJA_VARIANT_UINT32:
            return mUInt32Value;
        case AJA_VARIANT_INT64:
            return static_cast<uint32_t>(mInt64Value);
        case AJA_VARIANT_UINT64:
            return static_cast<uint32_t>(mUInt64Value);
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0;
            } else {
                return strtoul(mStringValue.data(), NULL, 0);
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0;
    }

    return 0;
}
int64_t AJAVariant::AsInt64() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1 : 0;
        case AJA_VARIANT_FLOAT:
            return static_cast<int64_t>(mFloatValue);
        case AJA_VARIANT_DOUBLE:
            return static_cast<int64_t>(mDoubleValue);
        case AJA_VARIANT_INT8:
            return static_cast<int64_t>(mInt8Value);
        case AJA_VARIANT_UINT8:
            return static_cast<int64_t>(mUInt8Value);
        case AJA_VARIANT_INT16:
            return static_cast<int64_t>(mInt16Value);
        case AJA_VARIANT_UINT16:
            return static_cast<int64_t>(mUInt16Value);
        case AJA_VARIANT_INT32:
            return static_cast<int64_t>(mInt32Value);
        case AJA_VARIANT_UINT32:
            return static_cast<int64_t>(mUInt32Value);
        case AJA_VARIANT_INT64:
            return mInt64Value;
        case AJA_VARIANT_UINT64:
            return static_cast<int64_t>(mUInt64Value);
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0;
            } else {
                return strtoll(mStringValue.data(), NULL, 0);
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0;
    }

    return 0;
}
uint64_t AJAVariant::AsUInt64() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? 1 : 0;
        case AJA_VARIANT_FLOAT:
            return static_cast<uint64_t>(mFloatValue);
        case AJA_VARIANT_DOUBLE:
            return static_cast<uint64_t>(mDoubleValue);
        case AJA_VARIANT_INT8:
            return static_cast<uint64_t>(mInt8Value);
        case AJA_VARIANT_UINT8:
            return static_cast<uint64_t>(mUInt8Value);
        case AJA_VARIANT_INT16:
            return static_cast<uint64_t>(mInt16Value);
        case AJA_VARIANT_UINT16:
            return static_cast<uint64_t>(mUInt16Value);
        case AJA_VARIANT_INT32:
            return static_cast<uint64_t>(mInt32Value);
        case AJA_VARIANT_UINT32:
            return static_cast<uint64_t>(mUInt32Value);
        case AJA_VARIANT_INT64:
            return static_cast<uint64_t>(mInt64Value);
        case AJA_VARIANT_UINT64:
            return mUInt64Value;
        case AJA_VARIANT_STRING:
        {
            if (mStringValue.empty()) {
                return 0;
            } else {
                return strtoull(mStringValue.data(), NULL, 0);
            }
        }
        default:
        case AJA_VARIANT_NONE:
            return 0;
    }

    return 0;
}
std::string AJAVariant::AsString() const {
    switch(mType) {
        case AJA_VARIANT_BOOL:
            return mBooleanValue ? "true" : "false";
        case AJA_VARIANT_FLOAT:
            return aja::to_string(mFloatValue);
        case AJA_VARIANT_DOUBLE:
            return aja::to_string(mDoubleValue);
        case AJA_VARIANT_INT8:
            return aja::to_string(mInt8Value);
        case AJA_VARIANT_UINT8:
            return aja::to_string(mUInt8Value);
        case AJA_VARIANT_INT16:
            return aja::to_string(mInt16Value);
        case AJA_VARIANT_UINT16:
            return aja::to_string(mUInt16Value);
        case AJA_VARIANT_INT32:
            return aja::to_string(mInt32Value);
        case AJA_VARIANT_UINT32:
            return aja::to_string(mUInt32Value);
        case AJA_VARIANT_INT64:
            return aja::to_string((long long)mInt64Value);
        case AJA_VARIANT_UINT64:
            return aja::to_string((unsigned long long)mUInt64Value);
        case AJA_VARIANT_STRING:
            return mStringValue;
        default:
        case AJA_VARIANT_NONE:
            return std::string();
    }

    return std::string();
}
