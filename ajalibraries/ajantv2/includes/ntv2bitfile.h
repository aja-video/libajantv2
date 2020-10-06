/**
	@file		ntv2bitfile.h
	@brief		Declares the CNTV2Bitfile class.
	@copyright	(C) 2010-2020 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
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
#include "ajaexport.h"
#include "ntv2enums.h"
#include "ntv2publicinterface.h"
#include "ntv2utils.h"


/**
	@brief	Instances of me can parse a bitfile.
**/
class AJAExport CNTV2Bitfile
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
			virtual NTV2_DEPRECATED_f(bool	Open (const char * const & inBitfilePath));	///< @deprecated	Use the std::string version of Open instead.
		#endif	//	!defined (NTV2_DEPRECATE)

		/**
			@brief	Closes bitfile (if open).
		**/
		virtual void						Close (void);

		/**
			@brief		Parse a bitfile header in a buffer.
			@param[in]	inBitfileBuffer	Specifies the buffer of the bitfile to be parsed.
			@param[in]	inBufferSize	Specifies the size of the buffer to be parsed.
			@return		A std::string containing parsing errors.
		**/
		virtual std::string					ParseHeaderFromBuffer(const uint8_t* inBitfileBuffer, const size_t inBufferSize);

		/**
			@brief		Answers with the bitfile build date, as extracted from the bitfile.
			@return		A string containing the bitfile build date.
		**/
		virtual inline const std::string &	GetDate (void) const			{ return _date; }

		/**
			@brief		Answers with the bitfile build time, as extracted from the bitfile.
			@return		A string containing the bitfile build time.
		**/
		virtual inline const std::string &	GetTime (void) const			{ return _time; }

		/**
			@brief		Answers with the design name, as extracted from the bitfile.
			@return		A string containing the bitfile design name.
		**/
		virtual inline const std::string &	GetDesignName (void) const		{ return _designName; }

		/**
			@brief		Answers true if design includes tandem flag, as extracted from the bitfile.
			@return		True if the bitfile header includes tandem flag; otherwise false.
		**/
		virtual inline bool					IsTandem (void) const		{ return _tandem; }

		/**
			@brief		Answers true if design includes partial flag, as extracted from the bitfile.
			@return		True if the bitfile header includes partial flag; otherwise false.
		**/
		virtual inline bool					IsPartial (void) const		{ return _partial; }

		/**
			@brief		Answers true if design includes clear flag, as extracted from the bitfile.
			@return		True if the bitfile header includes clear flag; otherwise false.
		**/
		virtual inline bool					IsClear (void) const		{ return _clear; }

		/**
			@brief		Answers true if design includes compress flag, as extracted from the bitfile.
			@return		True if the bitfile header includes compress flag; otherwise false.
		**/
		virtual inline bool					IsCompress (void) const		{ return _compress; }

		/**
			@brief		Answers with the design design ID, as extracted from the bitfile.
			@return		A ULWord containing the design design ID.
		**/
		virtual inline ULWord				GetDesignID (void) const	{ return _designID; }

		/**
			@brief		Answers with the design version, as extracted from the bitfile.
			@return		A ULWord containing the design version.
		**/
		virtual inline ULWord				GetDesignVersion (void) const	{ return _designVersion; }

		/**
			@brief		Answers with the design bitfile ID, as extracted from the bitfile.
			@return		A ULWord containing the design bitfile ID.
		**/
		virtual inline ULWord				GetBitfileID (void) const	{ return _bitfileID; }

		/**
			@brief		Answers with the design bitfile version, as extracted from the bitfile.
			@return		A ULWord containing the design bitfile version.
		**/
		virtual inline ULWord				GetBitfileVersion (void) const	{ return _bitfileVersion; }

		/**
			@brief		Answers with the design user ID, as extracted from the bitfile.
			@return		A ULWord containing the design user ID.
		**/
		virtual inline ULWord				GetUserID (void) const	{ return _userID; }

		/**
			@brief		Answers with the part name, as extracted from the bitfile.
			@return		A string containing the bitfile part name.
		**/
		virtual inline const std::string &	GetPartName (void) const		{ return _partName; }

		/**
			@brief		Answers true if the bitfile can be installed on the given device.
			@return		True if the bitfile can be flashed onto the device; otherwise false.
		**/
		virtual bool						CanFlashDevice (const NTV2DeviceID inDeviceID) const;

		/**
			@return		My instrinsic NTV2DeviceID.
		**/
		virtual NTV2DeviceID				GetDeviceID (void) const;

		/**
			@return		A string containing the error message, if any, from the last function that failed.
		**/
		virtual inline const std::string &	GetLastError (void) const		{ return _lastError; }

		/**
			@return		Program stream length in bytes, or zero if error/invalid.
		**/
		virtual size_t						GetProgramStreamLength (void) const;

		/**
			@return		File stream length in bytes, or zero if error/invalid.
		**/
		virtual size_t						GetFileStreamLength (void) const;
		
		/**
			@brief		Retrieves the program bitstream.
			@param[out]	outBuffer		Specifies the buffer that will receive the data.
			@return		Program stream length, in bytes, or zero upon failure.
		**/
		virtual size_t						GetProgramByteStream (NTV2_POINTER & outBuffer);

		/**
			@brief		Retrieves the file bitstream.
			@param[out]	outBuffer		Specifies the buffer that will receive the data.
			@return		File stream length, in bytes, or zero upon failure.
		**/
		virtual size_t						GetFileByteStream (NTV2_POINTER & outBuffer);

		//	Older, non-NTV2_POINTER-based functions:
		virtual size_t						GetProgramByteStream (unsigned char * pOutBuffer, const size_t inBufferLength);
		virtual size_t						GetFileByteStream (unsigned char * pOutBuffer, const size_t inBufferLength);

		// NO IMPLEMENTATION YET:	static NTV2StringList &	GetPartialDesignNames (const ULWord deviceID);
		static ULWord			GetDesignID			(const ULWord userID)		{ return (userID & 0xff000000) >> 24; }
		static ULWord			GetDesignVersion	(const ULWord userID)		{ return (userID & 0x00ff0000) >> 16; }
		static ULWord			GetBitfileID		(const ULWord userID)		{ return (userID & 0x0000ff00) >> 8; }
		static ULWord			GetBitfileVersion	(const ULWord userID)		{ return (userID & 0x000000ff) >> 0; }
		static NTV2DeviceID		ConvertToDeviceID	(const ULWord inDesignID, const ULWord inBitfileID);
		static ULWord			ConvertToDesignID	(const NTV2DeviceID inDeviceID);
		static ULWord			ConvertToBitfileID	(const NTV2DeviceID inDeviceID);

	private:
		virtual void						Init (void);
		virtual std::string					ParseHeader (void);
		virtual void						SetDesignName (const char * pInBuffer, const size_t bufferLength);
		virtual void						SetDesignFlags (const char * pInBuffer, const size_t bufferLength);
		virtual void						SetDesignUserID (const char * pInBuffer, const size_t bufferLength);

		std::ifstream				_bitFileStream;
		std::vector <unsigned char> _fileHeader;
		int							_fileProgrammingPosition;
		std::string					_date;
		std::string					_time;
		std::string					_designName;
		std::string					_partName;
		std::string					_lastError;
		size_t						_numBytes;
		size_t						_fileSize;
		bool						_fileReady;
		size_t						_programStreamPos;
		size_t						_fileStreamPos;
		bool						_tandem;
		bool						_partial;
		bool						_clear;
		bool						_compress;
		ULWord						_userID;
		ULWord						_designID;
		ULWord						_designVersion;
		ULWord						_bitfileID;
		ULWord						_bitfileVersion;

};	//	CNTV2Bitfile

#endif // NTV2BITFILE_H
