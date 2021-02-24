/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2enumerateboards.h
	@brief		Header file that defines the NTV2EnumerateDevices class
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef NTV2_ENUMERATE_BOARDS_CLASS
#define NTV2_ENUMERATE_BOARDS_CLASS

#include "ntv2devicescanner.h"
#include <string>


/**
	@brief	I am an object that knows how to discover and enumerate AJA devices.
			Once constructed, I can report how many boards there are, and report
			information about them.
**/
class NTV2EnumerateDevices
{
	//	Instance Methods
	public:

		//	@brief	My default constructor
					NTV2EnumerateDevices ();

		//	@brief	My destructor
		virtual		~NTV2EnumerateDevices ();


		/**
			@brief	Returns the number of boards that were discovered when I was constructed.
			@return	Number of AJA boards found.
		**/
		size_t		GetDeviceCount (void) const;


		/**
			@brief		Returns a string containing a human-readable description of the board having the given
						index position.
			@param[in]	inDeviceIndex	Specifies the board to describe. This must be a zero-based index number,
										and must be less than the total number of discovered boards.
			@return		A string containing a human-readable description of the board at the given index position.
			@note		The returned string will be empty if inDeviceIndex is out of range.
		**/
		std::string	GetDescription (uint32_t inDeviceIndex) const;


		/**
			@brief		Returns an NTV2DeviceInfo structure that describes the board at the given index position.
			@param[in]	inDeviceIndex	Specifies which board to return information for using a zero-based index value.
			@param[out]	boardInfo		Receives information about the given board.
			@return		True if successful; otherwise false (the specified board index was invalid).
		**/
		bool		GetDeviceInfo (uint32_t inDeviceIndex, NTV2DeviceInfo & boardInfo) const;


	//	Instance Data
	private:
		CNTV2DeviceScanner	mDeviceScanner;		///< @brief	My scanner object that enumerates AJA devices

};	//	NTV2EnumerateDevices

#endif	//	NTV2_ENUMERATE_BOARDS_CLASS
