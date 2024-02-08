/* SPDX-License-Identifier: MIT */
/**
	@file		ajamovingavg.h
	@brief		Declares the AJAMovingAvg class.
	@copyright	(C) 2021-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_MOVINGAVG_H
#define AJA_MOVINGAVG_H

//#include "ajabase/common/public.h"
#include "ajabase/system/lock.h"
#include <climits>
#include <cfloat>
#include <deque>
#include <ostream>
#include <typeinfo>
#if defined(AJA_WINDOWS)
	#pragma warning(disable:4056)
#endif


/**
 *	Class that maintains a moving average of a fixed number of T samples.
 *	Defaults to a maximum of 10 samples.
 *	@ingroup AJAGroupSystem
 */
template <typename T> class AJAMovingAvg
{
	public:
		static const size_t kDefaultMaxNumSamples = 10;

	public:
		/**
		 *	Constructs me with the given sample capacity.
		 *
		 *	@param[in]	inMaxNumSamples		Specifies the maximum number of samples to be stored.
		 */
		inline explicit			AJAMovingAvg (const size_t inMaxNumSamples = kDefaultMaxNumSamples)
		{
			reset(inMaxNumSamples);
		}

		/**
		 *	Stores the given sample value. If my sample capacity is exceeded, my oldest sample is dropped.
		 *
		 *	@param[in]	inValue		Specifies the new sample value.
		 */
		inline void				addSample (const T inValue)
		{	AJAAutoLock tmp(&mLock);
			mValues.push_front(inValue);
			if (mValues.size() > mSampleCapacity)
				mValues.pop_back();
			mTotNumSamples++;
			if (inValue < mMinValue)
				mMinValue = inValue;
			if (inValue > mMaxValue)
				mMaxValue = inValue;
		}

		/**
		 *	Resets me with the given maximum number of samples.
		 *
		 *	@param[in]	inMaxNumSamples		Specifies my new sample capacity.
		 */
		inline void				reset (const size_t inMaxNumSamples = kDefaultMaxNumSamples)
		{	AJAAutoLock tmp(&mLock);
			mValues.clear();
			mSampleCapacity = inMaxNumSamples;
			mTotNumSamples = 0;
			mMinValue = largestPossibleValue();
			mMaxValue = smallestPossibleValue();
		}

		/**
		 *	@returns the current number of samples I'm storing.
		 */
		inline size_t			numStoredSamples (void) const	{AJAAutoLock tmp(&mLock);  return mValues.size();}

		/**
		 *	@returns the maximum number of samples I will store.
		 */
		inline size_t			sampleCapacity (void) const		{AJAAutoLock tmp(&mLock);  return mSampleCapacity;}

		/**
		 *	@returns the total number of samples (i.e. number of addSample calls made).
		 */
		inline size_t			totalSamples (void) const		{AJAAutoLock tmp(&mLock);  return mTotNumSamples;}

		/**
		 *	@returns true if I'm valid.
		 */
		inline bool				isValid (void) const			{AJAAutoLock tmp(&mLock);  return totalSamples() > 0;}

		/**
		 *	@returns true if I'm empty.
		 */
		inline bool				isEmpty (void) const			{AJAAutoLock tmp(&mLock);  return mValues.empty();}

		/**
		 *	@returns my recent average.
		 */
		inline T				average (void) const
		{	AJAAutoLock tmp(&mLock);
			if (isEmpty())
				return 0;
			return sum() / T(numStoredSamples());
		}

		/**
		 *	@returns my recent average as a double-precision floating-point value.
		 */
		inline double			averageF (void) const
		{	AJAAutoLock tmp(&mLock);
			if (isEmpty())
				return 0.0;
			return double(sum()) / double(numStoredSamples());
		}

		/**
		 *	@returns the minimum value ever seen by addSample.
		 */
		inline T				minimum (void) const			{AJAAutoLock tmp(&mLock);  return mMinValue;}

		/**
		 *	@returns the minimum value from my stored samples.
		 */
		inline T				recentMinimum (void) const
		{
			T result(largestPossibleValue());
			AJAAutoLock tmp(&mLock);
			for (auto it(mValues.begin());  it != mValues.end();  ++it)
				if (*it < result)
					result = *it;
			return result;
		}

		/**
		 *	@returns the maximum value ever seen by addSample.
		 */
		inline T				maximum (void) const			{AJAAutoLock tmp(&mLock);  return mMaxValue;}

		/**
		 *	@returns the maximum value from my stored samples.
		 */
		inline T				recentMaximum (void) const
		{
			T result(smallestPossibleValue());
			AJAAutoLock tmp(&mLock);
			for (auto it(mValues.begin());  it != mValues.end();  ++it)
				if (*it > result)
					result = *it;
			return result;
		}

		/**
		 *	Writes my current state in a human-readable form into the given output stream.
		 *
		 *	@param		oss				The output stream to write into.
		 *	@param[in]	inDetailed		Specify true to print detailed information about my state;
		 *								otherwise just print my average value as a floating-point value.
		 *								Defaults to false.
		 */
		inline std::ostream &	Print (std::ostream & oss, const bool inDetailed = false) const
		{
			AJAAutoLock tmp(&mLock);
			if (isValid())
			{
				if (inDetailed)
					oss << averageF() << " (avg) for last " << numStoredSamples() << " of " << totalSamples() << " samples" << std::endl
						<< recentMinimum() << " (min), " << recentMaximum() << " (max)";
				else oss << averageF();
			}
			else oss << "n/a";
			return oss;
		}

	protected:
		inline T	sum (void) const
		{
			AJAAutoLock tmp(&mLock);
			if (isEmpty())
				return T(0);
			T result = T(0);
			for (auto it(mValues.begin());  it != mValues.end();  ++it)
				result += *it;
			return result;
		}

		static inline T	smallestPossibleValue (void)
		{
			if (typeid(T) == typeid(char))
				return SCHAR_MIN;
			else if (typeid(T) == typeid(int8_t))
				return SCHAR_MIN;
			else if (typeid(T) == typeid(uint8_t))
				return 0;
			else if (typeid(T) == typeid(int16_t))
				return T(SHRT_MIN);
			else if (typeid(T) == typeid(uint16_t))
				return 0;
			else if (typeid(T) == typeid(int))
				return T(INT_MIN);
			else if (typeid(T) == typeid(int32_t))
				return T(LONG_MIN);
			else if (typeid(T) == typeid(uint32_t))
				return 0;
			else if (typeid(T) == typeid(int64_t))
				return T(LLONG_MIN);
			else if (typeid(T) == typeid(uint64_t))
				return 0;
			else if (typeid(T) == typeid(float))
				return T(-FLT_MAX);
			else if (typeid(T) == typeid(double))
				return T(-DBL_MAX);
			else if (typeid(T) == typeid(long double))
				return T(-LDBL_MAX);
			return 0;
		}

		static inline T	largestPossibleValue (void)
		{
			if (typeid(T) == typeid(char))
				return SCHAR_MAX;
			else if (typeid(T) == typeid(int8_t))
				return SCHAR_MAX;
			else if (typeid(T) == typeid(uint8_t))
				return T(UCHAR_MAX);
			else if (typeid(T) == typeid(int16_t))
				return T(SHRT_MAX);
			else if (typeid(T) == typeid(uint16_t))
				return T(0xFFFF);
			else if (typeid(T) == typeid(int))
				return T(INT_MAX);
			else if (typeid(T) == typeid(int32_t))
				return T(LONG_MAX);
			else if (typeid(T) == typeid(uint32_t))
				return T(0xFFFFFFFF);
			else if (typeid(T) == typeid(int64_t))
				return T(LLONG_MAX);
			else if (typeid(T) == typeid(uint64_t))
				return T(0xFFFFFFFFFFFFFFFF);
			else if (typeid(T) == typeid(float))
				return T(FLT_MAX);
			else if (typeid(T) == typeid(double))
				return T(DBL_MAX);
			else if (typeid(T) == typeid(long double))
				return T(LDBL_MAX);
			return 0;
		}

	private:
		mutable AJALock	mLock;
		std::deque<T>	mValues;			///< @brief	My last N values for computing an average
		size_t			mSampleCapacity;	///< @brief	Maximum allowed size of the mValues deque
		size_t			mTotNumSamples;		///< @brief	Total number of times that addSample was called
		T				mMinValue;			///< @brief	Smallest value ever seen by addSample
		T				mMaxValue;			///< @brief	Largest value ever seen by addSample

};	//	AJAMovingAvg


/**
 *	Streams the AJAMovingAvg into the given output stream.
 *
 *	@param		oss				The target output stream.
 *	@param[in]	inDetailed		Specifies the AJAMovingAvg of interest.
 */
template <typename T> inline std::ostream & operator << (std::ostream & oss,  const AJAMovingAvg<T> & inAvg)
{
	inAvg.Print(oss,true);
	return oss;
}

#endif	//	AJA_MOVINGAVG_H
