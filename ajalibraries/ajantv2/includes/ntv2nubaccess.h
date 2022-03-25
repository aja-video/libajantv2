/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubaccess.h
	@brief		Declares NTV2 "nub" client functions.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2NUBACCESS_H
#define NTV2NUBACCESS_H

#include "ajaexport.h"
#include "ntv2nubtypes.h"
#include "ntv2utils.h"		//	For NTV2StringList
#include <string>
#include <vector>

//	Values returned by LastError:  Remote/fake device connection errors
#define NTV2_REMOTE_ACCESS_SUCCESS						  	 0x0000
#define NTV2_REMOTE_ACCESS_NOT_CONNECTED  				 	-1
#define NTV2_REMOTE_ACCESS_OUT_OF_MEMORY				 	-2
#define NTV2_REMOTE_ACCESS_SEND_ERR						 	-3
#define NTV2_REMOTE_ACCESS_CONNECTION_CLOSED 			 	-4
#define NTV2_REMOTE_ACCESS_RECV_ERR						 	-5
#define NTV2_REMOTE_ACCESS_TIMEDOUT				  		 	-6
#define NTV2_REMOTE_ACCESS_NO_CARD						 	-7
#define NTV2_REMOTE_ACCESS_NOT_OPEN_RESP				 	-8
#define NTV2_REMOTE_ACCESS_NON_NUB_PKT					 	-9
#define NTV2_REMOTE_ACCESS_NOT_READ_REGISTER_RESP			-10
#define NTV2_REMOTE_ACCESS_NOT_WRITE_REGISTER_RESP			-11
#define NTV2_REMOTE_ACCESS_NOT_AUTOCIRC_RESP				-12
#define NTV2_REMOTE_ACCESS_NOT_WAIT_FOR_INTERRUPT_RESP		-13
#define NTV2_REMOTE_ACCESS_WAIT_FOR_INTERRUPT_FAILED		-14
#define NTV2_REMOTE_AUTOCIRC_FAILED							-15
#define NTV2_REMOTE_ACCESS_DRIVER_GET_BITFILE_INFO_FAILED	-16
#define NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BITFILE_INFO		-17
#define NTV2_REMOTE_ACCESS_NOT_DOWNLOAD_TEST_PATTERN		-18
#define NTV2_REMOTE_ACCESS_DOWNLOAD_TEST_PATTERN_FAILED		-19
#define NTV2_REMOTE_ACCESS_READ_REG_MULTI_FAILED			-20
#define NTV2_REMOTE_ACCESS_NOT_READ_REG_MULTI				-21
#define NTV2_REMOTE_ACCESS_GET_DRIVER_VERSION_FAILED		-22
#define NTV2_REMOTE_ACCESS_NOT_GET_DRIVER_VERSION_RESP		-23
#define NTV2_REMOTE_ACCESS_READ_REG_FAILED					-24
#define NTV2_REMOTE_ACCESS_DRIVER_GET_BUILD_INFO_FAILED		-25
#define NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BUILD_INFO		-26
#define NTV2_REMOTE_ACCESS_UNIMPLEMENTED					-27
#define NTV2_REMOTE_ACCESS_CONNECTION_REFUSED 			 	-28
#define NTV2_REMOTE_ACCESS_RECV_UNDERRUN					-29
#define NTV2_REMOTE_ACCESS_INCOMPATIBLE						-30
#define NTV2_REMOTE_ACCESS_UNRESOLVED						-31


typedef std::pair<NTV2DeviceID, ULWord64>		NTV2DeviceIDSerialPair;		///< @brief	Identifies a device by its NTV2DeviceID and serial number
typedef std::vector<NTV2DeviceIDSerialPair>		NTV2DeviceIDSerialPairs;	///< @brief	An ordered sequence of NTV2DeviceIDSerialPairs
typedef NTV2DeviceIDSerialPairs::iterator		NTV2DeviceIDSerialPairsIter;
typedef NTV2DeviceIDSerialPairs::const_iterator	NTV2DeviceIDSerialPairsConstIter;

typedef std::map<std::string, std::string>		NTV2Dictionary, NTV2Dict;	///< @brief	A mapping of key strings to their corresponding value strings.
typedef std::pair<std::string, std::string>		NTV2DictionaryEntry, NTV2DictEntry;	///< @brief	A parameter used to connect to remote/fake devices
typedef NTV2Dict::const_iterator				NTV2DictConstIter;
typedef NTV2Dict								NTV2ConnectParams;		///< @brief	A dictionary of parameters used to connect to remote/fake devices
typedef NTV2DictEntry							NTV2ConnectParam;		///< @brief	A parameter used to connect to remote/fake devices
typedef NTV2DictConstIter						NTV2ConnectParamsCIter;	///< @brief	A const iterator into a parameter dictionary

// Supported NTV2ConnectParams:
static const std::string	kConnectParamScheme		("Scheme");			///< @brief	URL scheme
static const std::string	kConnectParamHost		("Host");			///< @brief	DNS name, IPv4 or sw device DLL name
static const std::string	kConnectParamPort		("Port");			///< @brief	Port number (optional)
static const std::string	kConnectParamDevIndex	("DeviceIndex");	///< @brief	Device having this index number
static const std::string	kConnectParamDevSerial	("DeviceSerial");	///< @brief	Device with this serial number
static const std::string	kConnectParamDevModel	("DeviceModel");	///< @brief	First device of this model (e.g. 'kona4')
static const std::string	kConnectParamDevID		("DeviceID");		///< @brief	First device having this ID (e.g. '0x10518400')
static const std::string	kConnectParamResource	("ResourcePath");	///< @brief	Resource path -- everything past URL [scheme://host[:port]/], excluding [?query]
static const std::string	kConnectParamQuery		("Query");			///< @brief	Query -- everything past '?' in URL

// Supported URL schemes:
static const std::string	kLegalSchemeNTV2		("ntv2");			///< @brief	Software/fake device
static const std::string	kLegalSchemeNTV2Nub		("ntv2nub");		///< @brief	Remote device via network RPC
static const std::string	kLegalSchemeNTV2Local	("ntv2local");		///< @brief	Real device on local host


AJAExport std::ostream & operator << (std::ostream & oss, const NTV2Dictionary & inDict);

/**
	@brief	One-stop shop for parsing device specifications.
			I do very little in the way of validating semantics.
			I simply do the parsing and provide the information needed to connect to local or remote devices, real or fake.
**/
class AJAExport NTV2DeviceSpecParser
{
	public:
		static const NTV2StringSet &	SupportedSchemes (void);							///< @return	A set of supported schemes
		static bool						IsSupportedScheme (const std::string & inScheme);	///< @return	True if the given scheme is supported

	public:
											NTV2DeviceSpecParser (const std::string inSpec = "");	///< @brief	My constructor. If given device specification is non-empty, proceeds to Parse it
		void								Reset (const std::string inSpec = "");	///< @brief	Resets me, then parses the given device specification
		inline const std::string &			DeviceSpec (void) const						{return mSpec;}		///< @return	The device specification I've parsed
		inline bool							HasDeviceSpec (void) const					{return !DeviceSpec().empty();}	///< @return	True if I have a device specification
		inline bool							Successful (void) const						{return !Failed();}	///< @return	True if successfully parsed
		inline bool							Failed (void) const							{return DeviceSpec().empty() ? true : HasErrors();}	///< @return	True if empty device spec or parser had errors
		inline bool							HasScheme (void) const						{return HasResult(kConnectParamScheme);}	///< @return	True if parser results contain a scheme
		inline bool							IsLocalDevice (void) const					{return Result(kConnectParamScheme) == kLegalSchemeNTV2Local;}	///< @return	True if parser results indicate a local device
		inline bool							IsRemoteDevice (void) const					{return Result(kConnectParamScheme) == kLegalSchemeNTV2Nub;}	///< @return	True if parser results indicate a remote device
		inline bool							IsSoftwareDevice (void) const				{return Result(kConnectParamScheme) == kLegalSchemeNTV2;}	///< @return	True if parser results indicate a software/fake device
		inline size_t						ErrorCount (void) const						{return mErrors.size();}	///< @return	Number of errors found by parser
		inline bool							HasErrors (void) const						{return ErrorCount() > 0;}	///< @return	True if ErrorCount is non-zero
		inline std::string					Error (const size_t inIndex = 0) const		{if (inIndex < mErrors.size()) return mErrors.at(inIndex); return "";}	///< @return	The Nth error found by parser
		inline NTV2StringList				Errors (void) const							{return mErrors;}	///< @return	All errors found by parser
		inline const NTV2ConnectParams &	Results (void) const						{return mResult;}	///< @return	A const reference to my parse results, a dictionary of values.
		bool								HasResult (const std::string & inKey) const;	///< @return	True if the given result exists.
		std::string							Result (const std::string & inKey) const;		///< @return	The result value for the given key.
		std::string							Resource (const bool inStripLeadSlash = true) const;	///< @return	The Result for the kConnectParamResource key
		std::ostream &						PrintErrors (std::ostream & oss) const;
		std::ostream &						Print (std::ostream & oss, const bool inDumpResults = false) const;
		std::string							InfoString (void) const;
		uint64_t							DeviceSerial (void) const;
		inline std::string					DeviceModel (void) const					{return Result(kConnectParamDevModel);}
		NTV2DeviceID						DeviceID (void) const;
		UWord								DeviceIndex (void) const;
		inline const NTV2Dictionary &		QueryParams (void) const					{return mQueryParams;}	///< @return	True if ErrorCount is non-zero
		inline std::string					QueryParam (const std::string & inKey) const;	///< @return	Query parameter value for the given query parameter key (empty string if invalid key)
		#if defined(_DEBUG)
		static void							test (void);
		#endif	//	defined(_DEBUG)

	private:
		void			Parse (void);
		bool			ParseHexNumber (size_t & pos, std::string & outToken);
		bool			ParseDecNumber (size_t & pos, std::string & outToken);
		bool			ParseAlphaNumeric (size_t & pos, std::string & outToken, const bool inUpperOnly = false);	//	A run of letters and/or decimal digits
		bool			ParseScheme (size_t & pos, std::string & outToken);	//	An alphanumeric name followed by "://"
		bool			ParseSerialNum (size_t & pos, std::string & outToken);	//	An 8 or 9 character alphanumeric name or a 64-bit hex number
		bool			ParseDeviceID (size_t & pos, std::string & outToken);	//	An 32-bit hex number that matches a known NTV2DeviceID
		bool			ParseModelName (size_t & pos, std::string & outToken);	//	An alphanumeric name that matches a known device model name
		bool			ParseDNSName (size_t & pos, std::string & outDNSName);	//	A domain name:  one or more alphanumeric names separated with '.'
		bool			ParseIPv4Address (size_t & pos, std::string & outIPv4);	//	Exactly four decimal numbers, each no greater than 255, separated with '.'
		bool			ParseHostAddressAndPortNumber (size_t & pos, std::string & outAddr, std::string & outPort);	//	A host name or IPv4 address with an optional port number
		bool			ParseResourcePath (size_t & pos, std::string & outRsrc);	//	One or more '/'-separated alphanumeric names
		bool			ParseParamAssignment (size_t & pos, std::string & outKey, std::string & outValue);	//	An alphanumeric key optionally followed by '=' optionally followed by URL-encoded value string
		bool			ParseQuery (size_t & pos, NTV2Dictionary & outParams);	//	Starts with '?' one or more '&'-separated param assignments
		int				LargestResultKey (void) const;
		int				LargestResultValue (void) const;
		inline char		CharAt (const size_t inPos)					{return inPos < mSpec.length() ? mSpec.at(inPos) : 0;}
		inline size_t	SpecLength (void) const						{return mSpec.length();}
		inline size_t	CurrentPosition (void) const				{return mPos;}
		inline void		AddError (const std::string & inError)		{mErrors.push_back(inError);}
		static bool		IsUpperLetter (const char inChar);
		static bool		IsLowerLetter (const char inChar);
		static bool		IsLetter (const char inChar, const bool inIncludeUnderscore = false);
		static bool		IsDecimalDigit (const char inChar);
		static bool		IsHexDigit (const char inChar);
		static bool		IsLegalSerialNumChar (const char inChar);

	private:
		std::string			mSpec;			///< @brief	Original device spec last parsed
		NTV2StringList		mErrors;		///< @brief	Error(s) reported from last Parse
		NTV2ConnectParams	mResult;		///< @brief	Parse results, a key/value dictionary
		NTV2Dictionary		mQueryParams;	///< @brief	Parse results, query params (key/value dictionary)
		size_t				mPos;			///< @brief	Last character position
};	//	NTV2DeviceSpecParser


/**
	@brief	Base class of objects that can connect to, and operate remote or fake devices. I have three general API groups:
			-	connection-related:  NTV2Connect, IsConnected, NTV2Disconnect;
			-	general inquiry: Name, HostName, Print, Version, etc.;
			-	remote/fake device operation: read/write register, (old) AutoCirculate, WaitForInterrupt, DMATransfer and NTV2Message.
	@note	Starting in SDK 16.3, most functions no longer return an int value, but instead return boolean true (success) or false (failure).
			To determine the failure reason, call LastError.
**/
class AJAExport NTV2RPCAPI
{
	public:
		static NTV2RPCAPI *	MakeNTV2NubRPCAPI (const NTV2ConnectParams & inParams);
		static NTV2RPCAPI *	FindNTV2SoftwareDevice (const NTV2ConnectParams & inParams);

	public:
		/**
			@name	General Inquiry
		**/
		///@{
		virtual inline const NTV2ConnectParams &	ConnectParams (void) const	{return mConnectParams;}	///< @return	My connect parameters
		virtual bool								HasConnectParam (const std::string & inParam) const;	///< @return	True if I have the given connect parameter
		virtual std::string							ConnectParam (const std::string & inParam) const;		///< @return	The given connect parameter (or empty string if missing)
		virtual inline std::string					HostName (void) const	{return ConnectParam(kConnectParamHost);}	///< @return	My host name
		virtual inline std::string					Name (void) const		{return "";};	///< @return	The "user-friendly" name of the remote/fake host
		virtual inline uint32_t						ProtocolVersion (void) const	{return NTV2_RPC_PROTOCOL_VERSION;}	///< @return	The protocol version of the remote host
		virtual std::ostream &						Print (std::ostream & oss) const;
		///@}

		/**
			@name	Connection Management
		**/
		///@{
		virtual bool	NTV2Connect (const NTV2ConnectParams & inParams);
		virtual bool	NTV2Disconnect (void);		///< @brief	Disconnects me from the remote/fake host, closing the connection.
		virtual bool	IsConnected (void) const	{return false;}		///< @return	True if I have an open connection to the remote host
		///@}

		/**
			@name	Device Operation
		**/
		///@{
		virtual bool	NTV2ReadRegisterRemote	(const ULWord regNum, ULWord & outRegValue, const ULWord regMask, const ULWord regShift);
		virtual bool	NTV2WriteRegisterRemote	(const ULWord regNum, const ULWord regValue, const ULWord regMask, const ULWord regShift);
		virtual bool	NTV2AutoCirculateRemote	(AUTOCIRCULATE_DATA & autoCircData);
		virtual bool	NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs);
		virtual bool	NTV2DriverGetBitFileInformationRemote	(BITFILE_INFO_STRUCT & bitFileInfo, const NTV2BitFileType bitFileType);
		virtual bool	NTV2DriverGetBuildInformationRemote	(BUILD_INFO_STRUCT & buildInfo);
		virtual bool	NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
														const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum);
		virtual bool	NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum, NTV2RegInfo outRegs[]);
		virtual bool	NTV2GetDriverVersionRemote	(ULWord & outDriverVersion);
		virtual	bool	NTV2DMATransferRemote		(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
													const ULWord inFrameNumber,			NTV2_POINTER & inOutBuffer,
													const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
													const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,
													const bool inSynchronous);
		virtual bool	NTV2MessageRemote	(NTV2_HEADER *	pInMessage);
		///@}

		/**
			@brief		Queries the devices that are accessible on the remote host.
			@param[out]	outDevices	Receives the list of devices (both NTV2DeviceID and device serial number are returned).
									Be default, answers with a zero-length list, and returns success.
									Subclasses must re-implement this to return whateever is appropriate.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	NTV2QueryDevices (NTV2DeviceIDSerialPairs & outDevices)	{outDevices.clear(); return true;}

											NTV2RPCAPI ();	///< @brief	My constructor.
		virtual								~NTV2RPCAPI();	///< @brief	My destructor, which automatically calls NTV2Disconnect.
		virtual inline NTV2_POINTER &		localStorage (void)				{return mPvt;}
		virtual inline const NTV2_POINTER &	localStorage (void) const		{return mPvt;}


	protected:
		virtual bool			NTV2OpenRemote	(void);
		virtual bool			NTV2CloseRemote	(void);

	protected:
		NTV2ConnectParams	mConnectParams;			///< @brief	Copy of connection parameters passed in to NTV2Connect
		uint32_t			mInstanceData[1024];	///< @brief	Private storage
		NTV2_POINTER		mPvt;
};	//	NTV2RPCAPI

inline std::ostream & operator << (std::ostream & oss, const NTV2RPCAPI & inObj)	{return inObj.Print(oss);}

extern "C" {
	typedef NTV2RPCAPI* (*fpCreateNTV2SoftwareDevice) (void * /*pInDLLHandle*/, const std::string & /*inParams*/, const uint32_t /*inHostSDKVersion*/);
}

#endif	//	NTV2NUBACCESS_H
