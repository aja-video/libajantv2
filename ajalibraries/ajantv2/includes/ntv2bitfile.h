/**
	@file		ntv2bitfile.h
	@brief		Declares the CNTV2Bitfile class that knows how to parse Xilinx bit-files and extract configuration bytestream.
	@copyright	(C) 2010-2014 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
**/

#ifndef NTV2BITFILE_H
#define NTV2BITFILE_H

#include <string>
#include <vector>
#include <fstream>
#ifdef AJALinux
	#include <stdint.h>
	#include <stdlib.h>
#endif
#include "ajatypes.h"
#include "ntv2enums.h"


/**
	@brief	Instances of me can parse a bitfile.
**/
class CNTV2Bitfile
{
	public:
		/**
			@brief		My constructor.
		**/
											CNTV2Bitfile ();

		/**
			@brief		My destructor.
		**/
		virtual								~CNTV2Bitfile ();

		/**
			@brief		Opens the bitfile at the given path, then parses its header.
			@param[in]	inBitfilePath	Specifies the path name of the bitfile to be parsed.
			@return		True if open & parse succeeds; otherwise false.
		**/
		virtual bool						Open (const std::string & inBitfilePath);

		#if !defined (NTV2_DEPRECATE)
			virtual NTV2_DEPRECATED	bool	Open (const char * const & inBitfilePath);	///< @deprecated	Use the std::string version of Open instead.
		#endif	//	!defined (NTV2_DEPRECATE)

		/**
			@brief	Closes bitfile (if open).
		**/
		virtual void						Close (void);

		/**
			@brief		Answers with the bitfile build date, as extracted from the bitfile.
			@return		A std::string containing the bitfile build date.
		**/
		virtual inline const std::string &	GetDate (void) const			{ return _date; }

		/**
			@brief		Answers with the bitfile build time, as extracted from the bitfile.
			@return		A std::string containing the bitfile build time.
		**/
		virtual inline const std::string &	GetTime (void) const			{ return _time; }

		/**
			@brief		Answers with the design name, as extracted from the bitfile.
			@return		A std::string containing the bitfile design name.
		**/
		virtual inline const std::string &	GetDesignName (void) const		{ return _designName; }

		/**
			@brief		Answers with the part name, as extracted from the bitfile.
			@return		A std::string containing the bitfile part name.
		**/
		virtual inline const std::string &	GetPartName (void) const		{ return _partName; }

		/**
			@brief		Answers true if the bitfile can be installed on the given device.
			@return		True if the bitfile can be flashed onto the device; otherwise false.
		**/
		virtual bool						CanFlashDevice (const NTV2DeviceID inDeviceID) const;

		/**
			@brief		Answers with my intrinsic NTV2DeviceID.
			@return		My instrinsic NTV2DeviceID.
		**/
		virtual NTV2DeviceID				GetDeviceID (void) const;

		/**
			@brief		Answers with the error message, if any, from the last function that failed (e.g., Open, or ParseHeader).
			@return		A std::string containing the error message.
		**/
		virtual inline const std::string &	GetLastError (void) const		{ return _lastError; }

		virtual unsigned					GetProgramStreamLength (void) const;
		virtual unsigned					GetFileStreamLength (void) const;
		virtual unsigned					GetProgramByteStream (unsigned char * buffer, unsigned bufferLength);
		virtual unsigned					GetFileByteStream (unsigned char * buffer, unsigned bufferLength);
		virtual void						SetDesignName (const char * pInBuffer);
		virtual std::string					ParseHeaderFromBuffer(const uint8_t* bitfileBuffer);

	private:
		virtual bool						FindSyncWord (void) const;
		virtual std::string					ParseHeader (unsigned & outPreambleSize);
		virtual void						Init (void);

		std::ifstream				_bitFileStream;
		std::vector <unsigned char> _fileHeader;
		int							_fileProgrammingPosition;
		std::string					_date;
		std::string					_time;
		std::string					_designName;
		std::string					_partName;
		std::string					_lastError;
		unsigned					_numBytes;
		unsigned					_fileSize;
		bool						_fileReady;
		bool						_bitFileCompressed;
		unsigned					_programStreamPos;
		unsigned					_fileStreamPos;

};	//	CNTV2Bitfile

#endif // NTV2BITFILE_H
