/* SPDX-License-Identifier: MIT */
/**
	@file		variant.h
	@brief		Declares the AJAVariant class.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_VARIANT_H
#define AJA_VARIANT_H

#include "ajabase/common/public.h"

#include <vector>

typedef enum {
	AJA_VARIANT_NONE,
	AJA_VARIANT_BOOL,
	AJA_VARIANT_FLOAT,
	AJA_VARIANT_DOUBLE,
	AJA_VARIANT_INT8,
	AJA_VARIANT_UINT8,
	AJA_VARIANT_INT16,
	AJA_VARIANT_UINT16,
	AJA_VARIANT_INT32,
	AJA_VARIANT_UINT32,
	AJA_VARIANT_INT64,
	AJA_VARIANT_UINT64,
	AJA_VARIANT_STRING,
} AJAVariantType;

/** \class AJAVariant variant.h
 *	\brief A simple Variant class.
 */
class AJA_EXPORT AJAVariant {
public:
	AJAVariant();
    explicit AJAVariant(AJAVariantType type);
    explicit AJAVariant(bool value);
    explicit AJAVariant(float value);
    explicit AJAVariant(double value);
    explicit AJAVariant(int8_t value);
    explicit AJAVariant(uint8_t value);
    explicit AJAVariant(int16_t value);
    explicit AJAVariant(uint16_t value);
    explicit AJAVariant(int32_t value);
    explicit AJAVariant(uint32_t value);
    explicit AJAVariant(int64_t value);
    explicit AJAVariant(uint64_t value);
    explicit AJAVariant(const char* value);
    explicit AJAVariant(const char* value, size_t length);
    explicit AJAVariant(const std::string& value);

    AJAVariant(const AJAVariant& other);
    ~AJAVariant() {}

    void operator=(const AJAVariant& other);
	
    // Type conversion operators
    operator bool() const;
    operator float() const;
    operator double() const;
    operator int8_t() const;
    operator uint8_t() const;
    operator int16_t() const;
    operator uint16_t() const;
    operator int32_t() const;
    operator uint32_t() const;
    operator int64_t() const;
    operator uint64_t() const;
    operator std::string() const;

	/**
	 *	Get the type of this variant.
	 *	@return	The type of this variant.
	 */
    AJAVariantType GetType() const { return mType; }

	/**
	 *	Get the bool value from this variant.
	 *	@return	The internal union bool value.
	 */
    bool GetBool() const;
	/**
	 *	Get the float value from this variant.
	 *	@return	The internal union float value.
	 */
    float GetFloat() const;
	/**
	 *	Get the double value from this variant.
	 *	@return	The internal union double value.
	 */
    double GetDouble() const;
	/**
	 *	Get the int8_t value from this variant.
	 *	@return	The internal union int8_t value.
	 */
    int8_t GetInt8() const;
	/**
	 *	Get the uint8_t value from this variant.
	 *	@return	The internal union uint8_t value.
	 */
    uint8_t GetUInt8() const;
	/**
	 *	Get the int16_t value from this variant.
	 *	@return	The internal union int16_t value.
	 */
    int16_t GetInt16() const;
	/**
	 *	Get the uint16_t value from this variant.
	 *	@return	The internal union uint16_t value.
	 */
    uint16_t GetUInt16() const;
	/**
	 *	Get the int32_t value from this variant.
	 *	@return	The internal union int32_t value.
	 */
    int32_t GetInt32() const;
	/**
	 *	Get the uint32_t value from this variant.
	 *	@return	The internal union uint32_t value.
	 */
    uint32_t GetUInt32() const;
	/**
	 *	Get the int64_t value from this variant.
	 *	@return	The internal union int64_t value.
	 */
    int64_t GetInt64() const;
	/**
	 *	Get the uint64_t value from this variant.
	 *	@return	The internal union uint64_t value.
	 */
    uint64_t GetUInt64() const;
	/**
	 *	Get the std::string value from this variant.
	 *	@return	The internal union std::string value.
	 */
    const std::string& GetString() const;

	/**
	 *	Set the value of this variant from the specified bool.
	 *	@param[in]	value					The bool value to set.
	 */
    void SetBool(bool value);
	/**
	 *	Set the value of this variant from the specified float.
	 *	@param[in]	value					The float value to set.
	 */
    void SetFloat(float value);
	/**
	 *	Set the value of this variant from the specified double.
	 *	@param[in]	value					The double value to set.
	 */
    void SetDouble(double value);
	/**
	 *	Set the value of this variant from the specified int8_t.
	 *	@param[in]	value					The int8_t value to set.
	 */
    void SetInt8(int8_t value);
	/**
	 *	Set the value of this variant from the specified uint8_t.
	 *	@param[in]	value					The uint8_t value to set.
	 */
    void SetUInt8(uint8_t value);
	/**
	 *	Set the value of this variant from the specified int16_t.
	 *	@param[in]	value					The int16_t value to set.
	 */
    void SetInt16(int16_t value);
	/**
	 *	Set the value of this variant from the specified uint16_t.
	 *	@param[in]	value					The uint16_t value to set.
	 */
    void SetUInt16(uint16_t value);
	/**
	 *	Set the value of this variant from the specified int32_t.
	 *	@param[in]	value					The int32_t value to set.
	 */
    void SetInt32(int32_t value);
	/**
	 *	Set the value of this variant from the specified uint32_t.
	 *	@param[in]	value					The uint32_t value to set.
	 */
    void SetUInt32(uint32_t value);
	/**
	 *	Set the value of this variant from the specified int64_t.
	 *	@param[in]	value					The int64_t value to set.
	 */
    void SetInt64(int64_t value);
	/**
	 *	Set the value of this variant from the specified uint64_t.
	 *	@param[in]	value					The uint64_t value to set.
	 */
    void SetUInt64(uint64_t value);
	/**
	 *	Set the value of this variant from the specified c-string.
	 *	@param[in]	value					The c-string value to set.
	 */
    void SetString(const char* value);
	/**
	 *	Set the value of this variant from the specified c-string.
	 *	@param[in]	value					The c-string value to set.
     *	@param[in]	value					The size of the string value.
	 */
    void SetString(const char* value, size_t length);
	/**
	 *	Set the value of this variant from the specified std::string.
	 *	@param[in]	value					The std::string value to set.
	 */
    void SetString(const std::string& value);

    // Type conversion methods
	/**
	 *	Get the value of this variant as bool.
	 *	@return	The bool value of this variant, if the type is AJA_VARIANT_BOOL, otherwise type-convert and return the current type as bool.
	 */
    bool AsBool() const;
	/**
	 *	Get the value of this variant as float.
	 *	@return	The float value of this variant, if the type is AJA_VARIANT_FLOAT, otherwise type-convert and return the current type as float.
	 */
    float AsFloat() const;
	/**
	 *	Get the value of this variant as double.
	 *	@return	The double value of this variant, if the type is AJA_VARIANT_DOUBLE, otherwise type-convert and return the current type as double.
	 */
    double AsDouble() const;
	/**
	 *	Get the value of this variant as int8_t.
	 *	@return	The int8_t value of this variant, if the type is AJA_VARIANT_INT8, otherwise type-convert and return the current type as int8_t.
	 */
    int8_t AsInt8() const;
	/**
	 *	Get the value of this variant as uint8_t.
	 *	@return	The uint8_t value of this variant, if the type is AJA_VARIANT_UINT8, otherwise type-convert and return the current type as uint8_t.
	 */
    uint8_t AsUInt8() const;
	/**
	 *	Get the value of this variant as int16_t.
	 *	@return	The int16_t value of this variant, if the type is AJA_VARIANT_INT16, otherwise type-convert and return the current type as int16_t.
	 */
    int16_t AsInt16() const;
	/**
	 *	Get the value of this variant as uint16_t.
	 *	@return	The uint16_t value of this variant, if the type is AJA_VARIANT_UINT16, otherwise type-convert and return the current type as uint16_t.
	 */
    uint16_t AsUInt16() const;
	/**
	 *	Get the value of this variant as int32_t.
	 *	@return	The int32_t value of this variant, if the type is AJA_VARIANT_INT32, otherwise type-convert and return the current type as int32_t.
	 */
    int32_t AsInt32() const;
	/**
	 *	Get the value of this variant as uint32_t.
	 *	@return	The uint32_t value of this variant, if the type is AJA_VARIANT_UINT32, otherwise type-convert and return the current type as uint32_t.
	 */
    uint32_t AsUInt32() const;
	/**
	 *	Get the value of this variant as int64_t.
	 *	@return	The int64_t value of this variant, if the type is AJA_VARIANT_INT64, otherwise type-convert and return the current type as int64_t.
	 */
    int64_t AsInt64() const;
	/**
	 *	Get the value of this variant as uint64_t.
	 *	@return	The uint64_t value of this variant, if the type is AJA_VARIANT_UINT64, otherwise type-convert and return the current type as uint64_t.
	 */
    uint64_t AsUInt64() const;
	/**
	 *	Get the value of this variant as std::string.
	 *	@return	The std::string value of this variant, if the type is AJA_VARIANT_STRING, otherwise type-convert and return the current type as std::string.
	 */
    std::string AsString() const;

private:
	void initialize();

    AJAVariantType mType;
    union {
        bool mBooleanValue;
        float mFloatValue;
        double mDoubleValue;
        int8_t mInt8Value;
        uint8_t mUInt8Value;
        int16_t mInt16Value;
        uint16_t mUInt16Value;
        int32_t mInt32Value;
        uint32_t mUInt32Value;
        int64_t mInt64Value;
        uint64_t mUInt64Value;
    };
	std::string mStringValue;
};

typedef std::vector<AJAVariant> AJAVariantList;

#endif
