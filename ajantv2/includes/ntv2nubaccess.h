/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubaccess.h
	@brief		Declares NTV2 "nub" client functions.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2NUBACCESS_H
#define NTV2NUBACCESS_H

#include "ntv2utils.h"		//	NTV2StringList
#include <string>
#include <vector>
#include <map>

//	TYPEDEFs
typedef std::pair<NTV2DeviceID, ULWord64>		NTV2DeviceIDSerialPair;		///< @brief	Identifies a device by its NTV2DeviceID and serial number
typedef std::vector<NTV2DeviceIDSerialPair>		NTV2DeviceIDSerialPairs;	///< @brief	An ordered sequence of NTV2DeviceIDSerialPairs
typedef NTV2DeviceIDSerialPairs::iterator		NTV2DeviceIDSerialPairsIter;
typedef NTV2DeviceIDSerialPairs::const_iterator	NTV2DeviceIDSerialPairsConstIter;

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

// Local URL schemes:
static const std::string	kLegalSchemeNTV2		("ntv2");
static const std::string	kLegalSchemeNTV2Local	("ntv2local");

// Exported Function Names:
static const std::string	kFuncNameCreateClient	("CreateClient");	///< @brief	Create an NTV2RPCClientAPI instance
static const std::string	kFuncNameCreateServer	("CreateServer");	///< @brief	Create an NTV2RPCServerAPI instance

/**
	@brief	A simple set of zero or more key/value pairs. (New in SDK 16.3)
**/
class AJAExport NTV2Dictionary
{
	public:
		/**
			@name	Inquiry
		**/
		///@{
		inline size_t		size (void) const	{return mDict.size();}	///< @return	The number of key/value pairs I'm storing
		inline bool			empty (void) const	{return mDict.empty();}	///< @return	True if I'm empty
		inline bool			hasKey (const std::string & inKey) const	{return mDict.find(inKey) != mDict.end();}	///< @return	True if I'm storing a value for the given key
		std::string			valueForKey (const std::string & inKey) const;	///< @return	The corresponding value for the given key, or empty string if no such key
		uint16_t			u16ValueForKey (const std::string & inKey, const uint16_t inDefault = 0) const;	///< @return	The corresponding uint16_t value for the given key, or inDefault if no such key
		NTV2StringSet		keys (void) const;	///< @return	My keys
		size_t				largestKeySize (void) const;	///< @return	The length of my largest key, in bytes
		size_t				largestValueSize (void) const;	///< @return	The length of my largest value, in bytes
		std::ostream &		Print (std::ostream & oss, const bool inCompact = true) const;	///< @brief	Prints human-readable representation to ostream
		///@}

		/**
			@name	Changing
		**/
		///@{
		inline void			clear (void)	{mDict.clear();}		///< @brief	Removes all of my key/value pairs
		inline bool			insert (const std::string & inKey, const std::string & inValue) {mDict[inKey] = inValue; return true;}	///< @return	Stores the given value using the given key; overwrites existing value if already present
		inline size_t		erase (const std::string & inKey)	{return mDict.erase(inKey);}	///< @brief	Erases the given key and its corresponding value from me, returns 1 if successful, 0 if not
		size_t				UpdateFrom (const NTV2Dictionary & inDict);	///< @brief	Updates all values from inDict with matching keys, ignoring all non-matching keys
		size_t				AddFrom (const NTV2Dictionary & inDict);	///< @brief	Adds all values from inDict with non-matching keys, ignoring all matching keys
		///@}

	protected:
		typedef std::map<std::string, std::string>	Dict;
		typedef Dict::const_iterator				DictConstIter;

	private:
		Dict	mDict;	///< @brief	My map
};	//	NTV2Dictionary

typedef NTV2Dictionary	NTV2Dict;
typedef NTV2Dictionary	NTV2ConnectParams;		///< @brief	A dictionary of parameters used to connect to remote/fake devices
typedef NTV2Dictionary	NTV2ConfigParams;		///< @brief	A dictionary of parameters used to configure an RPC server
typedef std::pair<std::string, std::string>		NTV2DictionaryEntry, NTV2DictEntry, NTV2ConnectParam;	///< @brief	A parameter used to connect to remote/fake devices

inline std::ostream & operator << (std::ostream & oss, const NTV2Dictionary & inDict)	{return inDict.Print(oss);}

/**
	@brief	One-stop shop for parsing device specifications. (New in SDK 16.3)
			I do very little in the way of validating semantics.
			I simply do the parsing and provide the information needed to connect to local or remote devices, real or fake.
**/
class AJAExport NTV2DeviceSpecParser
{
	public:
		static bool							IsSupportedScheme (const std::string & inScheme);	///< @return	True if the given scheme starts with "ntv2"

	public:
											NTV2DeviceSpecParser (const std::string inSpec = "");	///< @brief	My constructor. If given device specification is non-empty, proceeds to Parse it
		void								Reset (const std::string inSpec = "");	///< @brief	Resets me, then parses the given device specification
		inline const std::string &			DeviceSpec (void) const						{return mSpec;}		///< @return	The device specification I've parsed
		inline bool							HasDeviceSpec (void) const					{return !DeviceSpec().empty();}	///< @return	True if I have a device specification
		inline bool							Successful (void) const						{return !Failed();}	///< @return	True if successfully parsed
		inline bool							Failed (void) const							{return DeviceSpec().empty() ? true : HasErrors();}	///< @return	True if empty device spec or parser had errors
		inline bool							HasScheme (void) const						{return HasResult(kConnectParamScheme);}	///< @return	True if parser results contain a scheme
		inline std::string					Scheme (void) const							{return Result(kConnectParamScheme);}	///< @return	The scheme (or empty if no scheme)
		inline bool							IsLocalDevice (void) const					{return Scheme() == kLegalSchemeNTV2Local || Scheme() == kLegalSchemeNTV2;}	///< @return	True if parser results indicate a local device
		inline size_t						ErrorCount (void) const						{return mErrors.size();}	///< @return	Number of errors found by parser
		inline bool							HasErrors (void) const						{return ErrorCount() > 0;}	///< @return	True if ErrorCount is non-zero
		inline std::string					Error (const size_t inIndex = 0) const		{if (inIndex < mErrors.size()) return mErrors.at(inIndex); return "";}	///< @return	The Nth error found by parser
		inline NTV2StringList				Errors (void) const							{return mErrors;}	///< @return	All errors found by parser
		inline const NTV2ConnectParams &	Results (void) const						{return mResult;}	///< @return	A const reference to my parse results, a dictionary of values.
		inline bool							HasResult (const std::string & inKey) const	{return mResult.hasKey(inKey);}	///< @return	True if the given result exists.
		std::string							Result (const std::string & inKey) const	{return mResult.valueForKey(inKey);}	///< @return	The result value for the given key.
		std::string							Resource (const bool inStripLeadSlash = true) const;	///< @return	The Result for the kConnectParamResource key
		std::ostream &						PrintErrors (std::ostream & oss) const;
		std::ostream &						Print (std::ostream & oss, const bool inDumpResults = false) const;
		std::string							InfoString (void) const;
		uint64_t							DeviceSerial (void) const;
		inline std::string					DeviceModel (void) const					{return Result(kConnectParamDevModel);}
		NTV2DeviceID						DeviceID (void) const;
		UWord								DeviceIndex (void) const;
		inline const NTV2Dictionary &		QueryParams (void) const					{return mQueryParams;}	///< @return	True if ErrorCount is non-zero
		inline std::string					QueryParam (const std::string & inKey) const	{return mQueryParams.valueForKey(inKey);}	///< @return	Query parameter value for the given query parameter key (empty string if no such key)
		#if defined(_DEBUG)
		static void							test (void);
		#endif	//	defined(_DEBUG)

	private:
		void			Parse (void);
		bool			ParseHexNumber (size_t & pos, std::string & outToken);
		bool			ParseDecNumber (size_t & pos, std::string & outToken);
		bool			ParseAlphaNumeric (size_t & pos, std::string & outToken, const std::string & inOtherChars = "");	//	A run of letters and/or decimal digits and/or other chars
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
		inline int		LargestResultKey (void) const				{return int(mResult.largestKeySize());}
		inline int		LargestResultValue (void) const				{return int(mResult.largestValueSize());}
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
			-	connection:  NTV2Connect, IsConnected, NTV2Disconnect;
			-	configuration:  ConnectParams, HasConnectParam, ConnectParam, ConnectHasScheme, SetConnectParams
			-	inquiry:  Name, HostName, Print, Version, etc.;
			-	device operation:  read/write register, (old) AutoCirculate, WaitForInterrupt, DMATransfer and NTV2Message.
			-	The Create factory method attempts to find and load a plugin that implements the requested software or remote device.
	@note	Starting in SDK 16.3, most functions no longer return an int value, but instead return boolean true (success) or false (failure).
			(Check the AJADebug log for AJA_DebugUnit_RPCServer or AJA_DebugUnit_RPCClient messages.)
**/
class AJAExport NTV2RPCClientAPI
{
	public:
		static NTV2RPCClientAPI *	CreateClient (const NTV2ConnectParams & inParams);

	public:
		/**
			@name	General Inquiry
		**/
		///@{
		virtual std::string		HostName (void) const	{return ConnectParam(kConnectParamHost);}	///< @return	My host name
		virtual std::string		Name (void) const		{return "";}	///< @return	The "user-friendly" name of the remote/fake host
		virtual std::string		Description (void) const;	///< @return	A description of the remote/fake host
		virtual std::ostream &	Print (std::ostream & oss) const;
		///@}

		/**
			@name	Connection Management
		**/
		///@{
		virtual bool	NTV2Connect (void);
		virtual bool	NTV2Disconnect (void);		///< @brief	Disconnects me from the remote/fake host, closing the connection.
		virtual bool	IsConnected (void) const	{return false;}	///< @return	True if I have an open connection to the remote host
		///@}

		/**
			@name	Configuration Management
		**/
		///@{
		virtual const NTV2ConnectParams &	ConnectParams (void) const		{return mConnectParams;}	///< @return	My connect parameters
		virtual bool						HasConnectParam (const std::string & inParam) const	{return mConnectParams.hasKey(inParam);}	///< @return	True if I have the given connect parameter
		virtual std::string					ConnectParam (const std::string & inParam) const	{return mConnectParams.valueForKey(inParam);}	///< @return	The given connect parameter (or empty string if missing)
		virtual bool						ConnectHasScheme (void) const	{return HasConnectParam(kConnectParamScheme);}	///< @return	True if connect params contains a scheme
		virtual bool						SetConnectParams (const NTV2ConnectParams & inNewParams, const bool inAugment = false);	///< @brief	Replaces or adds to my connect parameters
		///@}

		/**
			@name	Device Operation
		**/
		///@{
		virtual bool	NTV2ReadRegisterRemote	(const ULWord regNum, ULWord & outRegValue, const ULWord regMask, const ULWord regShift);
		virtual bool	NTV2WriteRegisterRemote	(const ULWord regNum, const ULWord regValue, const ULWord regMask, const ULWord regShift);
		virtual bool	NTV2AutoCirculateRemote	(AUTOCIRCULATE_DATA & autoCircData);
		virtual bool	NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs);
		virtual	bool	NTV2DMATransferRemote		(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
													const ULWord inFrameNumber,			NTV2Buffer & inOutBuffer,
													const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
													const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,
													const bool inSynchronous);
		virtual bool	NTV2MessageRemote	(NTV2_HEADER *	pInMessage);
		///@}

		/**
			@name	Device Features
		**/
		///@{
		virtual bool	NTV2GetBoolParamRemote (const ULWord inParamID,  ULWord & outValue);	//	New in SDK 17.0
		virtual bool	NTV2GetNumericParamRemote (const ULWord inParamID,  ULWord & outValue);	//	New in SDK 17.0
		virtual bool	NTV2GetSupportedRemote (const ULWord inEnumsID, ULWordSet & outSupported);	//	New in SDK 17.0
		///@}

		/**
			@brief		Queries the devices that are accessible on the remote host.
			@param[out]	outDeviceInfos	Receives a list of zero or more device information strings, one string per device.
										Each string is ':' delimited with the following information fields, in this order:
										"host:port:deviceid:serial:index" where...
										-	host:		server IPv4 dotted-quad address or host name;
										-	port:		server port number (decimal, no leading zeroes);
										-	deviceID:	NTV2DeviceID as 8-digit hex string (or device name);
										-	serial:		device serial number as 16-digit hex string, or its character string equivalent;
										-	index:		16-bit unsigned index number (optional;  only specified for real, connected hardware)
										Subclasses must re-implement to return whateever is appropriate.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	NTV2QueryDevices (NTV2StringList & outDeviceInfos)	{outDeviceInfos.clear(); return true;}

						NTV2RPCClientAPI (const NTV2ConnectParams & inParams);	///< @brief	My constructor.
		virtual			~NTV2RPCClientAPI();	///< @brief	My destructor, automatically calls NTV2Disconnect.

		#if !defined(NTV2_DEPRECATE_16_3)	//	These functions are going away
		virtual bool	NTV2DriverGetBitFileInformationRemote	(BITFILE_INFO_STRUCT & bitFileInfo, const NTV2BitFileType bitFileType);
		virtual bool	NTV2DriverGetBuildInformationRemote	(BUILD_INFO_STRUCT & buildInfo);
		virtual bool	NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
														const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum);
		virtual bool	NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum, NTV2RegInfo outRegs[]);
		virtual bool	NTV2GetDriverVersionRemote	(ULWord & outDriverVersion);
		#endif	//	!defined(NTV2_DEPRECATE_16_3)

	protected:
		virtual bool	NTV2OpenRemote	(void);
		virtual bool	NTV2CloseRemote	(void);

	protected:
		NTV2ConnectParams	mConnectParams;		///< @brief	Copy of connection parameters passed in to NTV2Connect
		uint32_t			mSpare[1024];		///< @brief	Reserved
};	//	NTV2RPCClientAPI

typedef NTV2RPCClientAPI NTV2RPCAPI;

inline std::ostream & operator << (std::ostream & oss, const NTV2RPCClientAPI & inObj)	{return inObj.Print(oss);}

/**
	@brief	Base class of objects that can serve device operation RPCs with NTV2RPCClientAPI instances.
			-	The Create factory method attempts to find and load a plugin that instantiates a server with a requested configuration.
**/
class AJAExport NTV2RPCServerAPI
{
	public:
		/**
			@brief		Factory method that instantiates a new NTV2RPCServerAPI instance using a plugin based on the
						specified config parameters.
			@param[in]	inParams	Specifies the server configuration.
									The "Scheme" parameter (required) determines the plugin to load.
			@return		If successful, a non-zero pointer to the new NTV2RPCServerAPI instance;  otherwise nullptr (zero).
		**/
		static NTV2RPCServerAPI *	CreateServer (const NTV2ConfigParams & inParams);

		/**
			@brief		Factory method that instantiates a new NTV2RPCServerAPI instance using a plugin based on the
						specified URL.
			@param[in]	inURL	Specifies the server configuration.
								The URL scheme (required) determines the plugin to load.
								It must start with "ntv2", and the remainder of the name must match a DLL/dylib/so name.
			@return		If successful, a non-zero pointer to the new NTV2RPCServerAPI instance;  otherwise nullptr (zero).
		**/
		static NTV2RPCServerAPI *	CreateServer (const std::string & inURL);

	public:
		/**
			@name	General Inquiry
		**/
		///@{
		virtual std::ostream &	Print (std::ostream & oss) const;
		///@}

		/**
			@name	Configuration Management
		**/
		///@{
		virtual const NTV2ConfigParams &	ConfigParams (void) const		{return mConfigParams;}	///< @return	My config parameters
		virtual bool						HasConfigParam (const std::string & inParam) const	{return mConfigParams.hasKey(inParam);}	///< @return	True if I have the given config parameter
		virtual std::string					ConfigParam (const std::string & inParam) const	{return mConfigParams.valueForKey(inParam);}	///< @return	The given config parameter (or empty string if missing)
		virtual bool						SetConfigParams (const NTV2ConfigParams & inNewParams, const bool inAugment = false);	///< @brief	Replaces or adds to my config parameters
		///@}

		/**
			@name	Operation/Control
		**/
		///@{
		virtual void		RunServer	(void);	///< @brief	Principal server thread function, subclsses should override
		///@}

	protected:
							NTV2RPCServerAPI (const NTV2ConnectParams & inParams);	///< @brief	My constructor.
		virtual				~NTV2RPCServerAPI();	///< @brief	My destructor, automatically calls NTV2Disconnect.

	protected:
		NTV2ConfigParams	mConfigParams;		///< @brief	Copy of config params passed in to my constructor
		uint32_t			mSpare[1024];		///< @brief	Reserved
};	//	NTV2RPCServerAPI

inline std::ostream & operator << (std::ostream & oss, const NTV2RPCServerAPI & inObj)	{return inObj.Print(oss);}

/**
	The scheme specified in a device specifier URL identifies the dynamically-loaded library to load in order to provide
	a requested NTV2RPCClientAPI or NTV2RPCServerAPI. These are the functions the library must provide that instantiate
	the client or server instance.
**/
extern "C"
{
	/**
		@brief	Instantiates a new client instance to talk to a remote server.
				-	pDLLHandle:	A pointer to the DLL/dylib/so handle.
				-	inParams: A const reference to the NTV2ConnectParams that specify the "what" and "how" the new client should access the server device.
				-	inHostSDKVersion:	Specifies the NTV2 SDK version the caller was compiled with.
		@return	A pointer to the new client instance if successful, or nullptr (zero) upon failure.
	**/
	typedef NTV2RPCClientAPI* (*fpCreateClient) (void * /*pInDLLHandle*/, const NTV2ConnectParams & /*inParams*/, const uint32_t /*inHostSDKVersion*/);

	/**
		@brief	Instantiates a new server instance for talking to clients.
				-	pDLLHandle:	A pointer to the DLL/dylib/so handle.
				-	inParams: A const reference to the NTV2ConfigParams that specify how to configure the server.
				-	inHostSDKVersion:	Specifies the NTV2 SDK version the caller was compiled with.
		@return	A pointer to the new server instance if successful, or nullptr (zero) upon failure.
		@note	Do not implement this function if a server implementation is not required.
	**/
	typedef NTV2RPCServerAPI* (*fpCreateServer) (void * /*pInDLLHandle*/, const NTV2ConfigParams & /*inParams*/, const uint32_t /*inHostSDKVersion*/);

	#if !defined(NTV2_DEPRECATE_16_3)	//	Don't use these functions going forward
	typedef NTV2RPCAPI* (*fpCreateNTV2SoftwareDevice) (void * /*pInDLLHandle*/, const std::string & /*inQueryStr*/, const uint32_t /*inHostSDKVersion*/);
	#endif	//	!defined(NTV2_DEPRECATE_16_3)
}

#endif	//	NTV2NUBACCESS_H
