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

/**
 *	\brief The data types that AJAVariant supports.
 */
typedef enum
{
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
class AJA_EXPORT AJAVariant
{
public:
	explicit AJAVariant (const AJAVariantType type = AJA_VARIANT_INT32);
	explicit inline AJAVariant (const bool value)		{mType = AJA_VARIANT_BOOL;		mUnion.mBooleanValue = value;}
	explicit inline AJAVariant (const float value)		{mType = AJA_VARIANT_FLOAT;		mUnion.mFloatValue   = value;}
	explicit inline AJAVariant (const double value)		{mType = AJA_VARIANT_DOUBLE;	mUnion.mDoubleValue  = value;}
	explicit inline AJAVariant (const int8_t value)		{mType = AJA_VARIANT_INT8;		mUnion.mInt8Value    = value;}
	explicit inline AJAVariant (const uint8_t value)	{mType = AJA_VARIANT_UINT8;		mUnion.mUInt8Value   = value;}
	explicit inline AJAVariant (const int16_t value)	{mType = AJA_VARIANT_INT16;		mUnion.mInt16Value   = value;}
	explicit inline AJAVariant (const uint16_t value)	{mType = AJA_VARIANT_UINT16;	mUnion.mUInt16Value  = value;}
	explicit inline AJAVariant (const int32_t value)	{mType = AJA_VARIANT_INT32;		mUnion.mInt32Value   = value;}
	explicit inline AJAVariant (const uint32_t value)	{mType = AJA_VARIANT_UINT32;	mUnion.mUInt32Value  = value;}
	explicit inline AJAVariant (const int64_t value)	{mType = AJA_VARIANT_INT64;		mUnion.mInt64Value   = value;}
	explicit inline AJAVariant (const uint64_t value)	{mType = AJA_VARIANT_UINT64;	mUnion.mUInt64Value  = value;}
	explicit inline AJAVariant (const char* pStr)		{mType = AJA_VARIANT_STRING;	mStringValue = pStr;}
	explicit inline AJAVariant (const char* pStr, size_t len)	: mType(AJA_VARIANT_STRING), mStringValue(pStr, len) {}
	explicit inline AJAVariant (const std::string & value)		: mType(AJA_VARIANT_STRING), mStringValue(value) {}

	inline AJAVariant (const AJAVariant & other)	{operator=(other);}

	~AJAVariant() {}

	/**
	 *	Assigns the value and type of another AJAVariant to me.
	 *	@param[in]	other	The AJAVariant to assign to me.
	 *	@return	A non-const reference to me.
	 */
    AJAVariant & operator = (const AJAVariant & other);

	/**
	 *	Assigns a scalar value of any type to me.
	 *	@param[in]	inRHS	The scalar value to assign to me.
	 *	@return	A non-const reference to me.
	 */
	template<typename T> AJAVariant &	operator = (const T inRHS)
										{
											const AJAVariant v(inRHS);
											mType = v.mType;
											mUnion = v.mUnion;
											mStringValue = v.mStringValue;
											return *this;
										}

	//	Type conversion operators
	inline operator bool() const		{return AsBool();}
	inline operator float() const		{return AsFloat();}
	inline operator double() const		{return AsDouble();}
	inline operator int8_t() const		{return AsInt8();}
	inline operator uint8_t() const		{return AsUInt8();}
	inline operator int16_t() const		{return AsInt16();}
	inline operator uint16_t() const	{return AsUInt16();}
	inline operator int32_t() const		{return AsInt32();}
	inline operator uint32_t() const	{return AsUInt32();}
	inline operator int64_t() const		{return AsInt64();}
	inline operator uint64_t() const	{return AsUInt64();}
	inline operator std::string() const	{return AsString();}

	inline AJAVariantType GetType() const	{return mType;}	///< @returns	My data type

	bool IsNumeric() const;		///< @returns	True if I have a numeric type
	bool IsUnsigned() const;	///< @returns	True if I have an unsigned numeric type;
	bool IsSigned() const;		///< @returns	True if I have a signed numeric type

	/**
	 *	Get the bool value from this variant.
	 *	@return	My bool value.
	 */
	inline bool GetBool() const				{return mUnion.mBooleanValue;}

	/**
	 *	Get the float value from this variant.
	 *	@return	My float value.
	 */
	inline float GetFloat() const			{return mUnion.mFloatValue;}

	/**
	 *	Get the double value from this variant.
	 *	@return	My double value.
	 */
	inline double GetDouble() const			{return mUnion.mDoubleValue;}

	/**
	 *	Get the int8_t value from this variant.
	 *	@return	My int8_t value.
	 */
	inline int8_t GetInt8() const			{return mUnion.mInt8Value;}

	/**
	 *	Get the uint8_t value from this variant.
	 *	@return	My uint8_t value.
	 */
	inline uint8_t GetUInt8() const			{return mUnion.mUInt8Value;}

	/**
	 *	Get the int16_t value from this variant.
	 *	@return	My int16_t value.
	 */
	inline int16_t GetInt16() const			{return mUnion.mInt16Value;}

	/**
	 *	Get the uint16_t value from this variant.
	 *	@return	My uint16_t value.
	 */
	inline uint16_t GetUInt16() const		{return mUnion.mUInt16Value;}

	/**
	 *	Get the int32_t value from this variant.
	 *	@return	My int32_t value.
	 */
	inline int32_t GetInt32() const			{return mUnion.mInt32Value;}

	/**
	 *	Get the uint32_t value from this variant.
	 *	@return	My uint32_t value.
	 */
	inline uint32_t GetUInt32() const		{return mUnion.mUInt32Value;}

	/**
	 *	Get the int64_t value from this variant.
	 *	@return	My int64_t value.
	 */
	inline int64_t GetInt64() const			{return mUnion.mInt64Value;}

	/**
	 *	Get the uint64_t value from this variant.
	 *	@return	My uint64_t value.
	 */
	inline uint64_t GetUInt64() const		{return mUnion.mUInt64Value;}

	/**
	 *	Get the std::string value from this variant.
	 *	@return	My std::string value.
	 */
	inline const std::string & GetString() const	{return mStringValue;}

	/**
	 *	Sets my value from the specified bool.
	 *	@param[in]	value	The bool value to set.
	 */
    inline void SetBool(bool value)	{mUnion.mBooleanValue = value;	mType = AJA_VARIANT_BOOL;}
	/**
	 *	Sets my value from the specified float.
	 *	@param[in]	value	The float value to set.
	 */
    inline void SetFloat(float value)	{mUnion.mFloatValue = value;	mType = AJA_VARIANT_FLOAT;}
	/**
	 *	Sets my value from the specified double.
	 *	@param[in]	value	The double value to set.
	 */
    inline void SetDouble(double value)	{mUnion.mDoubleValue = value;	mType = AJA_VARIANT_DOUBLE;}
	/**
	 *	Sets my value from the specified int8_t.
	 *	@param[in]	value	The int8_t value to set.
	 */
    inline void SetInt8(int8_t value)	{mUnion.mInt8Value = value;	mType = AJA_VARIANT_INT8;}
	/**
	 *	Sets my value from the specified uint8_t.
	 *	@param[in]	value	The uint8_t value to set.
	 */
    inline void SetUInt8(uint8_t value)	{mUnion.mUInt8Value = value;	mType = AJA_VARIANT_UINT8;}
	/**
	 *	Sets my value from the specified int16_t.
	 *	@param[in]	value	The int16_t value to set.
	 */
    inline void SetInt16(int16_t value)	{mUnion.mInt16Value = value;	mType = AJA_VARIANT_INT16;}
	/**
	 *	Sets my value from the specified uint16_t.
	 *	@param[in]	value	The uint16_t value to set.
	 */
    inline void SetUInt16(uint16_t value)	{mUnion.mUInt16Value = value;	mType = AJA_VARIANT_UINT16;}
	/**
	 *	Sets my value from the specified int32_t.
	 *	@param[in]	value	The int32_t value to set.
	 */
    inline void SetInt32(int32_t value)	{mUnion.mInt32Value = value;	mType = AJA_VARIANT_INT32;}
	/**
	 *	Sets my value from the specified uint32_t.
	 *	@param[in]	value	The uint32_t value to set.
	 */
    inline void SetUInt32(uint32_t value)	{mUnion.mUInt32Value = value;	mType = AJA_VARIANT_UINT32;}
	/**
	 *	Sets my value from the specified int64_t.
	 *	@param[in]	value	The int64_t value to set.
	 */
    inline void SetInt64(int64_t value)	{mUnion.mInt64Value = value;	mType = AJA_VARIANT_INT64;}
	/**
	 *	Sets my value from the specified uint64_t.
	 *	@param[in]	value	The uint64_t value to set.
	 */
    inline void SetUInt64(uint64_t value)	{mUnion.mUInt64Value = value;	mType = AJA_VARIANT_UINT64;}
	/**
	 *	Sets my value from the specified c-string.
	 *	@param[in]	value	The c-string value to set.
	 */
    inline void SetString(const char* value)	{mStringValue = std::string(value);	mType = AJA_VARIANT_STRING;}
	/**
	 *	Sets my value from the given c-style string.
	 *	@param[in]	pStr	Points to the character buffer to copy from. Cannot be nullptr.
     *	@param[in]	len		The number of bytes to copy from the string value.
	 */
    inline void SetString(const char* pStr, const size_t len)	{mStringValue = std::string(pStr, len);	mType = AJA_VARIANT_STRING;}
	/**
	 *	Sets my value from the given std::string.
	 *	@param[in]	value	The std::string value to set.
	 */
    inline void SetString(const std::string & value)	{mStringValue = value;	mType = AJA_VARIANT_STRING;}

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
	union
	{
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
	} mUnion;
	std::string mStringValue;
};	//	AJAVariant

typedef std::vector<AJAVariant> AJAVariantList;

#endif	//	AJA_VARIANT_H
