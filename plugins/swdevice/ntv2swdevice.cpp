/**
	@file		ntv2swdevice.cpp
	@brief		A software device implementation.
	@copyright	(C) 2019-2022 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#include "ajatypes.h"
#include "ntv2nubaccess.h"
#include "ntv2publicinterface.h"
#include "ntv2utils.h"
#include "ntv2version.h"
//#include "../../../ajadriver/ntv2autocirc.h"		//	<== TBD TBD TBD		Use user-space driver code
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/lock.h"
#include <fstream>
#include <iomanip>
#if defined(AJAMac)
	#include <CoreFoundation/CoreFoundation.h>
	#include <dlfcn.h>
#elif defined(AJALinux)
	#include <dlfcn.h>
#endif

using namespace std;

#define FAKE_DEVICE_SHARE_NAME		"ntv2shmdev"

#define INSTP(_p_)			xHEX0N(uint64_t(_p_),16)
#define	NBFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

#define	AsIOMemoryDescriptor(_f_)		(reinterpret_cast <IOMemoryDescriptor *> (_f_))
#define	AsIOMemoryMap(_f_)				(reinterpret_cast <IOMemoryMap *> (_f_))
#define	AsAUTOCIRCULATE_STATUS(_p_)		(reinterpret_cast <AUTOCIRCULATE_STATUS *> (_p_))
#define	AsAUTOCIRCULATE_TRANSFER(_p_)	(reinterpret_cast <AUTOCIRCULATE_TRANSFER *> (_p_))
#define	AsNTV2GetRegisters(_p_)			(reinterpret_cast <NTV2GetRegisters *> (_p_))
#define	AsNTV2SetRegisters(_p_)			(reinterpret_cast <NTV2SetRegisters *> (_p_))
#define	AsNTV2BankSelGetSetRegs(_p_)	(reinterpret_cast <NTV2BankSelGetSetRegs *> (_p_))
#define	AsNTV2NTV2VirtualData(_p_)      (reinterpret_cast <NTV2VirtualData *> (_p_))
#define	AsFRAME_STAMP(_p_)				(reinterpret_cast <FRAME_STAMP *> (_p_))
#define	AsNTV2BufferLock(_p_)      		(reinterpret_cast <NTV2BufferLock *> (_p_))
#define AsNTV2Bitstream(_p_)			(reinterpret_cast <NTV2Bitstream *> (_p_))

#if defined(MSWindows)
	#define EXPORT __declspec(dllexport)	
#else
	#define EXPORT	
#endif

/*****************************************************************************************************************************************************
	NTV2SoftwareDevice

	Simulates an NTV2 device. By default, uses named global host shared memory for frame & audio buffers, register storage,
	and driver state information (e.g. AutoCirculate).


	CONFIGURATION PARAMETERS
		Parameter Name			Required?	Default Value		Description
		------------------		----------	-------------------	---------------------------------------------------------------------------
		NoSharedMemory			No			n/a					If specified, allocates private buffer/register memory from the host process
																heap instead of global shared memory. NOTE: This will make the "device"
																unsharable between multiple processes.
																Example:  "ntv2://swdevice/?noSharedMemory"
		SupportLog=fileURL		No			n/a					If specified, initializes this device's registers from the given support log
																file. The 'fileURL' must be a URL-encoded path to the file. Currently only files
																on the local host are supported.
																NOTE:	The "supportlog" command line tool can generate these register dump files.
		SDRAM=fileURL			No			n/a					If specified, fills this device's buffer memory from the given binary data file.
																The 'fileURL' must be a URL-encoded path to the file. Currently only files on
																the local host are supported.
																NOTE:	The "supportlog --sdram" command line tool can generate these frame
																		buffer memory dumps.

	EXAMPLE USAGE:
		To use this "device" in the NTV2Player demo on MacOS:
			./bin/ntv2player  --device 'ntv2swdevice://localhost/?nosharedmemory&supportlog=file%3A%2F%2F%2FUsers%2Fdemo%2Fsupportlog_5XT713362.log&sdram=file%3A%2F%2F%2FUsers%2Fdemo%2Fsdram_5XT713362.raw'
		...or in C++:
			CNTV2Card device;
			device.Open("ntv2swdevice://localhost/?nosharedmemory&supportlog=file%3A%2F%2F%2FUsers%2Fdemo%2Fsupportlog_5XT713362.log&sdram=file%3A%2F%2F%2FUsers%2Fdemo%2Fsdram_5XT713362.raw");

		On MacOS, this will cause CNTV2DriverInterface::OpenRemote to look for a dynamic library named "swdevice.dylib" inside the folder
		"/Library/Application Support/AJA", and after loading it, if it has a "CreateClient" function, calls it, passing it the URL params.
		The CreateClient function (below) instantiates a new NTV2SoftwareDevice instance, and configures it based on the supplied URL params:
			"nosharedmemory&supportlog=file%3A%2F%2F%2FUsers%2Fdemo%2Fsupportlog_5XT713362.log&sdram=file%3A%2F%2F%2FUsers%2Fdemo%2Fsdram_5XT713362.raw"
		After URL-decoding, and viewed as key/value pairs, here's the software device configuration parameters:
			PARM				VALUE
			nosharedmemory		
			supportlog			file:///Users/demo/supportlog_5XT713362.log
			sdram				file:///Users/demo/sdram_5XT713362.raw
		This "device" will load its registers (including virtuals) from the "supportlog_5XT713362.log" file,
		and its frame buffer memory from the "sdram_5XT713362.raw" binary data file.
		If these files won't load, NTV2Connect will fail, ultimately causing CNTV2DriverInterface::OpenRemote to fail ("failed to open").


	SHARED MEMORY LAYOUT:
	[1K Hdr][---- 128MB Reg Memory ----][---------------------------- FB Memory ----------------------------][2MB InternalAC]

*****************************************************************************************************************************************************/

typedef struct _AJANTV2FakeDevice
{	//								//	1024-byte header:
	uint32_t	fMagicID;			//	'0DEV'
	uint32_t	fVersion;			//	Version 1
	uint32_t	fNumRegBytes;		//	FIRST:	# bytes 128MB of Register memory			(default  128 MB)
	uint32_t	fNumFBBytes;		//	SECOND:	# bytes of FB memory						(default 1024 MB)
	uint32_t	fNumACBytes;		//	THIRD:	# bytes for INTERNAL_AUTOCIRCULATE_STRUCT	(default    2 MB)
	uint32_t	fClientRefCount;	//	Initially 0
	uint32_t	fReserved[1000];	//	Filler to make 1024 byte header
} AJANTV2FakeDevice;


static const uint32_t		kDefaultNumRegBytes	(128ULL * 1024ULL * 1024ULL);	//	128MB
static const uint32_t		kDefaultNumFBBytes	(1024ULL * 1024ULL * 1024ULL);	//	1GB
static const uint32_t		kDefaultNumACBytes	(1024ULL * 1024ULL * 96ULL);	//	96MB
static const ULWord			kOffsetToRegBytes	(sizeof(AJANTV2FakeDevice));
static const uint32_t		kFakeDevCookie		(0xFACEDE00);
static AJANTV2FakeDevice *	spFakeDevice		(AJA_NULL);
static AJALock				sLock;


//	Specific NTV2RPCAPI implementation to talk to software device
class NTV2SoftwareDevice : public NTV2RPCAPI
{
	//	Instance Methods
	public:
										NTV2SoftwareDevice (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inCallingVersion);
		virtual							~NTV2SoftwareDevice ();
		virtual string					Name						(void) const;
		virtual string					Description					(void) const;
		virtual inline bool				IsConnected					(void) const	{return spFakeDevice ? true : false;}
		virtual bool					NTV2Connect					(void);
		virtual	bool					NTV2Disconnect				(void);
		virtual bool					NTV2GetBoolParamRemote		(const ULWord inParamID,  ULWord & outValue);
		virtual bool					NTV2GetNumericParamRemote	(const ULWord inParamID,  ULWord & outValue);
		virtual bool					NTV2GetSupportedRemote		(const ULWord inEnumsID, ULWordSet & outSupported);
		virtual	bool					NTV2ReadRegisterRemote		(const ULWord regNum, ULWord & outRegValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		virtual	bool					NTV2WriteRegisterRemote		(const ULWord regNum, const ULWord regValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		virtual	bool					NTV2AutoCirculateRemote		(AUTOCIRCULATE_DATA & autoCircData);
		virtual	bool					NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs);
		virtual	bool					NTV2DMATransferRemote		(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
																	const ULWord inFrameNumber,			NTV2Buffer & inOutBuffer,
																	const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
																	const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,
																	const bool inSynchronous);
		virtual	bool					NTV2MessageRemote			(NTV2_HEADER * pInMessage);
		virtual string					getParam					(const string & inKey)			{return mConnectParams.valueForKey(inKey);}
		virtual bool					hasParam					(const string & inKey) const	{return mConnectParams.hasKey(inKey);}

	//	Protected & Private Instance Methods
	protected:
		virtual	bool					NTV2OpenRemote				(void);
		virtual	bool					NTV2CloseRemote				(void)							{return true;}
	private:
		virtual	void					InitRegs (void);
		virtual bool					InitRegsFromSupportLog		(const string & inLogFilePath);
		static uint32_t					GetSDRAMDumpFileSize		(const string & inFilePath);
		virtual bool					InitSDRAMFromFile			(const string & inFilePath);
//		virtual NTV2AutoCirc *			ACContext (void)			{return mpContext;}

	//	Instance Data
	private:
		uint64_t		mDLLHandle;			///< @brief	DLL handle
		const uint32_t	mHostSDKVersion;	///< @brief	Host/caller SDK version
		const uint32_t	mSDKVersion;		///< @brief	My SDK version
		NTV2Buffer		mAllMemory;			///< @brief	Handy NTV2Buffer to reference all memory
		NTV2Buffer		mRegMemory;			///< @brief	Handy NTV2Buffer to reference register memory (includes vregs)
		NTV2Buffer		mFBMemory;			///< @brief	Handy NTV2Buffer to reference frame buffer memory (includes audio buffers)
		NTV2Buffer		mACMemory;			///< @brief	Handy NTV2Buffer to reference AutoCirculate (driver) state memory
		uint32_t		mFBReqBytes;		///< @brief	Requested size of SDRAM complement, in bytes
		NTV2DeviceID	mDeviceID;			///< @brief	My device ID, if known
		uint64_t		mSerialNum;			///< @brief	My serial number, if known
		string			mHostname;			///< @brief	My "host" name
//		NTV2AutoCirc*	mpContext;			//	<== TBD TBD TBD		Use user-space driver code
};	//	NTV2SoftwareDevice

extern "C"
{
	//AJAExport NTV2RPCClientAPI * CreateClient (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inCallerSDKVers);
	//Not using AJAExport - AJASTATIC was stuck defined, breaking AJAExport in ajaexport.h - CMake dependent project issue
	EXPORT NTV2RPCClientAPI * CreateClient (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inCallerSDKVers);
}

NTV2RPCClientAPI * CreateClient (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inCallerSDKVers)
{
	AJADebug::Open();
    NTV2SoftwareDevice * pResult(new NTV2SoftwareDevice (pInDLLHandle, inParams, inCallerSDKVers));
	if (!pResult->NTV2Connect())
	{	AJA_sERROR(AJA_DebugUnit_RPCClient, AJAFUNC << ": NTV2Connect failed");
		delete pResult;
		return AJA_NULL;
	}	//	Failed
	AJA_sDEBUG(AJA_DebugUnit_RPCClient, AJAFUNC << ": returning " << xHEX0N(uint64_t(pResult),16));
	return pResult;
}


NTV2SoftwareDevice::NTV2SoftwareDevice (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inCallingVersion)
	:	NTV2RPCAPI		(inParams),
		mDLLHandle		(uint64_t(pInDLLHandle)),
		mHostSDKVersion	(inCallingVersion),
		mSDKVersion		(AJA_NTV2_SDK_VERSION),
		mAllMemory		(0),
		mRegMemory		(0),
		mFBMemory		(0),
		mACMemory		(0),
		mFBReqBytes		(kDefaultNumFBBytes),
		mDeviceID		(DEVICE_ID_NOTFOUND),
		mSerialNum		(0),
        mHostname		(FAKE_DEVICE_SHARE_NAME)
//		,mpContext		(AJA_NULL)		//	<== TBD TBD TBD		Use user-space driver code
{
	string queryStr(ConnectParam(kConnectParamQuery));
	if (!queryStr.empty())
		if (queryStr[0] == '?')
			queryStr.erase(0,1);	//	Remove leading '?'
	const NTV2StringList strs(aja::split(queryStr, "&"));
	for (NTV2StringListConstIter it(strs.begin());  it != strs.end();  ++it)
	{
		string str(*it), key, value;
		if (str.find("=") == string::npos)
		{
			key = aja::lower(str);
			mConnectParams.insert(key, value);
			NBDBG("'" << key << "'");
			continue;
		}
		NTV2StringList pieces(aja::split(str,"="));
		if (pieces.empty())
			continue;
		key = aja::lower(pieces.at(0));
		if (pieces.size() > 1)
			value = pieces.at(1);
		if (key.empty())
			{NBWARN("Empty key '" << key << "'");  continue;}
		if (HasConnectParam(key))
			NBDBG("Param '" << key << "' value '" << mConnectParams.valueForKey(key) << "' to be replaced with '" << value << "'");
		mConnectParams.insert(key, ::PercentDecode(value));
		NBDBG("'" << key << "' = '" << mConnectParams.valueForKey(key) << "'");
	}
	NBINFO("constructed, " << DEC(mConnectParams.size()) << " param(s)");
}

NTV2SoftwareDevice::~NTV2SoftwareDevice ()
{
	NTV2Disconnect();
	if (mDLLHandle)
	{
#if defined(MSWindows)
#else
		::dlclose(reinterpret_cast<void*>(mDLLHandle));  NBINFO("dlclose(" << xHEX0N(mDLLHandle,16) << ")");
#endif	//	AJAMac or AJALinux
	}
	else NBINFO("");
}

bool NTV2SoftwareDevice::NTV2Connect (void)
{
	if (mHostname != FAKE_DEVICE_SHARE_NAME)
		{NBFAIL("Share name '" << mHostname << "' doesn't match expected name '" << FAKE_DEVICE_SHARE_NAME << "'");  return false;}
	bool useSharedMemory(true);	//	Default to using global shared memory
	string supportLogPath, sdramPath;

	//	Version check...
	if (mSDKVersion  &&  mHostSDKVersion  &&  mSDKVersion != mHostSDKVersion)
		NBWARN(Name() << " SDK version " << xHEX0N(mSDKVersion,8) << " doesn't match host SDK version " << xHEX0N(mHostSDKVersion,8));

	//	Check config params:
	const NTV2StringSet keys(mConnectParams.keys());
	NTV2StringList skippedParams;
	const std::string URI_head = "file://";
	for (NTV2StringSetConstIter it(keys.begin());  it != keys.end();  ++it)
	{
		const string key(*it); string value(mConnectParams.valueForKey(key));
		if (key == "nosharedmemory")
		{
			useSharedMemory = false;	//	Disable
			if (!value.empty())
				NBWARN(Name() << " 'NoSharedMemory' parameter value '" << value << "' specified but not required");
			NBWARN(Name() << " using 'NoSharedMemory' -- will not use global shared memory");
		}
		else if (key == "supportlog")
		{
			if (value.empty())
				{NBWARN(Name() << " 'SupportLog' parameter value missing or empty");  continue;}
			if (value.find(URI_head) != 0)
				{NBFAIL(Name() << " 'SupportLog' URL parameter invalid -- expected '" << URI_head << "' scheme");  return false;}
			value.erase(0, 7);
			supportLogPath = value;
			NBINFO(Name() << " 'SupportLog' parameter value '" << supportLogPath << "' specified");
		}
		else if (key == "sdram")
		{	const ULWord k128MB (0x08000000);
			if (value.empty())
				{NBWARN(Name() << " 'SDRAM' parameter value missing or empty");  continue;}
			if (value.find(URI_head) != 0)
				{NBFAIL(Name() << " 'SDRAM' URL parameter invalid -- expected '" << URI_head << "' scheme");  return false;}
			value.erase(0, URI_head.length());
			sdramPath = value;
			mFBReqBytes = GetSDRAMDumpFileSize(sdramPath);
			if (!mFBReqBytes)	//	Empty?
				{NBFAIL(Name() << " 'SDRAM' file '" << sdramPath << "' is empty -- expected 128MB or larger");  return false;}
			if (mFBReqBytes < k128MB)	//	< 128MB?
				{NBFAIL(Name() << " 'SDRAM' file size is " << xHEX0N(mFBReqBytes,8) << " bytes -- expected " << xHEX0N(k128MB,8) << " (128MB) or larger");  return false;}
			NBINFO(Name() << " 'SDRAM' parameter value '" << sdramPath << "' specified: " << DEC(mFBReqBytes) << "(" << xHEX0N(mFBReqBytes,8) << ") bytes");
		}
		else if (key == "help")
		{
			ostringstream oss;
			oss << "ntv2swdevice:  This plugin simulates an NTV2 device completely in software." << endl
				<< "CONFIG PARAMS:" << endl
				<< "Name                Reqd    Default     Desc" << endl
				<< "nosharedmemory      No      N/A         If specified, device memory is allocated privately instead of globally shared." << endl
				<< "supportlog=fileurl  No      N/A         Specifies URL-encoded path to support log file to initialize registers." << endl
				<< "sdram=fileurl       No      N/A         Specifies URL-encoded path to binary data file to initialize SDRAM.";
			NBINFO(oss.str());
			cerr << oss.str() << endl;
			return false;
		}
		else
			skippedParams.push_back(key);
	}	//	for each connectParams key

	if (!skippedParams.empty())
		NBWARN("Skipped unrecognized parameter(s): " << skippedParams);
	if (!sLock.IsValid())
		{NBFAIL("Lock object is invalid");  return false;}
	AJAAutoLock lock(&sLock);
	const size_t fakeDevTotalBytes (sizeof(AJANTV2FakeDevice) + kDefaultNumRegBytes + mFBReqBytes + kDefaultNumACBytes);

	try
	{	//	Allocate the shared memory region
		if (!spFakeDevice)
		{
			//	Allocate the shared memory storage
			size_t sizeInBytes(fakeDevTotalBytes);
			if (useSharedMemory)
			{
				spFakeDevice = reinterpret_cast<AJANTV2FakeDevice*>(AJAMemory::AllocateShared (&sizeInBytes, FAKE_DEVICE_SHARE_NAME));
				if (!spFakeDevice  ||  spFakeDevice == (void*)(-1))
				{
					NBFAIL("AJAMemory::AllocateShared failed, name='" << FAKE_DEVICE_SHARE_NAME << "', size=" << DEC(sizeInBytes) << "(" << xHEX0N(sizeInBytes,8) << ")");
					spFakeDevice = AJA_NULL;
					NTV2Disconnect();
					return false;
				}
				if (sizeInBytes < fakeDevTotalBytes)
				{
					NBFAIL("'" << FAKE_DEVICE_SHARE_NAME << "' region created, but size " << DEC(sizeInBytes) << "(" << xHEX0N(sizeInBytes,8) << ") < "
							<< DEC(fakeDevTotalBytes) << "(" << xHEX0N(fakeDevTotalBytes,8) << ")");
					NTV2Disconnect();
					return false;
				}
			}
			else
			{
				spFakeDevice = reinterpret_cast<AJANTV2FakeDevice*>(AJAMemory::Allocate (sizeInBytes));
				if (!spFakeDevice  ||  spFakeDevice == (void*)(-1))
				{
					NBFAIL("AJAMemory::Allocate failed, size=" << sizeInBytes);
					spFakeDevice = AJA_NULL;
					NTV2Disconnect();
					return false;
				}
				::memset(spFakeDevice, 0, fakeDevTotalBytes);
			}	//	else using private heap memory

			//	Check version
			if (spFakeDevice->fVersion == 0)
			{
				::memset(spFakeDevice, 0, fakeDevTotalBytes);
				spFakeDevice->fMagicID			= kFakeDevCookie;
				spFakeDevice->fVersion			= 1;
				spFakeDevice->fNumRegBytes		= kDefaultNumRegBytes;
				spFakeDevice->fNumFBBytes		= mFBReqBytes;
				spFakeDevice->fNumACBytes		= kDefaultNumACBytes;
				spFakeDevice->fClientRefCount	= 0;
				NBNOTE("'" << FAKE_DEVICE_SHARE_NAME << "' created, initialized, size=" << DEC(sizeInBytes) << "(" << xHEX0N(sizeInBytes,8) << ")");
			}

			//	Must be correct version
			if (spFakeDevice->fVersion != 1)
			{
				NBFAIL("'" << FAKE_DEVICE_SHARE_NAME << "' created, but version " << spFakeDevice->fVersion << " != 1");
				spFakeDevice = AJA_NULL;
				NTV2Disconnect();
				return false;
			}

			mAllMemory.Set(spFakeDevice, fakeDevTotalBytes);
			mRegMemory.Set(mAllMemory.GetHostAddress(kOffsetToRegBytes),   spFakeDevice->fNumRegBytes);
			mFBMemory.Set(mAllMemory.GetHostAddress(kOffsetToRegBytes + spFakeDevice->fNumRegBytes),	spFakeDevice->fNumFBBytes);
			mACMemory.Set(mAllMemory.GetHostAddress(kOffsetToRegBytes + spFakeDevice->fNumFBBytes),		spFakeDevice->fNumACBytes);

            spFakeDevice->fClientRefCount++;
		}	//	if spFakeDevice is NULL
		else
		{
			mAllMemory.Set(spFakeDevice, fakeDevTotalBytes);
			mRegMemory.Set(mAllMemory.GetHostAddress(kOffsetToRegBytes),   spFakeDevice->fNumRegBytes);
			mFBMemory.Set(mAllMemory.GetHostAddress(kOffsetToRegBytes + spFakeDevice->fNumRegBytes),	spFakeDevice->fNumFBBytes);
			mACMemory.Set(mAllMemory.GetHostAddress(kOffsetToRegBytes + spFakeDevice->fNumFBBytes),		spFakeDevice->fNumACBytes);
		}
/*
		if (mACMemory.GetByteCount() < sizeof(NTV2AutoCirc))
		{
			NBFAIL(DEC(mACMemory.GetByteCount()) << "-byte AC state memory too small to accommodate " << DEC(sizeof(NTV2AutoCirc)) << "-byte NTV2AutoCirc struct");
			spFakeDevice = AJA_NULL;
			NTV2Disconnect();
			return false;
		}
		mpContext = mACMemory;*/
		//	Set registers...
		if (supportLogPath.empty())	//	supportLogPath specified?
			InitRegs();	//	Initialize registers to some reasonable default state
		else if (!InitRegsFromSupportLog(supportLogPath))	//	Load registers from support log file
		{
			spFakeDevice = AJA_NULL;
			NTV2Disconnect();
			return false;	//	failed
		}

		//	Read DeviceID and serial number...
		ULWord reg50(0), snLo(0), snHi(0);
		if (NTV2ReadRegisterRemote(kRegReserved54, snLo)  &&  NTV2ReadRegisterRemote(kRegReserved55, snHi)  &&  snLo  && snHi)
			mSerialNum = (uint64_t(snHi) << 32) | uint64_t(snLo);
		if (NTV2ReadRegisterRemote (kRegBoardID, reg50)  &&  reg50)
			mDeviceID = NTV2DeviceID(reg50);	//	Keep my board ID handy

		//	Set SDRAM...
		if (!sdramPath.empty())			//	sdramPath specified?
			if (!InitSDRAMFromFile (sdramPath))	//	Load SDRAM from dump file
			{
				spFakeDevice = AJA_NULL;
				NTV2Disconnect();
				return false;	//	failed
			}
	}
	catch(...)
	{
		NBFAIL("Exception during creation of '" << FAKE_DEVICE_SHARE_NAME << "'");
		spFakeDevice = AJA_NULL;
		NTV2Disconnect();
		return false;
	}

	NBINFO(Description() << " is ready, vers=" << spFakeDevice->fVersion
			<< " refCnt=" << spFakeDevice->fClientRefCount
			<< " reg=" << spFakeDevice->fNumRegBytes << " fb=" << spFakeDevice->fNumFBBytes
			<< " ac=" << spFakeDevice->fNumACBytes);
	return true;
}

bool NTV2SoftwareDevice::NTV2Disconnect (void)
{
	NBINFO("");
	return true;
}

string NTV2SoftwareDevice::Name (void) const
{
	ostringstream oss;
	oss << "swdevice " << ::NTV2DeviceIDToString(mDeviceID);
	return oss.str();
}

string NTV2SoftwareDevice::Description (void) const
{
	ostringstream oss;
	oss << "NTV2 software device '" << ::NTV2DeviceIDToString(mDeviceID) << "'";
	if (mSerialNum)
		if (!::SerialNum64ToString(mSerialNum).empty())
			oss << " serial '" << ::SerialNum64ToString(mSerialNum) << "'";
	return oss.str();
}

bool NTV2SoftwareDevice::NTV2OpenRemote (void)
{
	return true;
}

bool NTV2SoftwareDevice::NTV2GetBoolParamRemote (const ULWord inParamID,  ULWord & outValue)
{	(void)inParamID;	(void)outValue;
	//	Return false to retain the standard ::NTV2DeviceCanDoXXX behavior based on
	//	my NTV2DeviceID (register 50 value).
	return false;
}

bool NTV2SoftwareDevice::NTV2GetNumericParamRemote (const ULWord inParamID,  ULWord & outValue)
{	(void)inParamID;	(void)outValue;
	//	Return false to retain the standard ::NTV2DeviceGetNumXXX behavior based on
	//	my NTV2DeviceID (register 50 value).
	return false;
}

bool NTV2SoftwareDevice::NTV2GetSupportedRemote (const ULWord inEnumsID, ULWordSet & outSupported)
{	(void)inEnumsID;	(void)outSupported;
	//	Return false to retain the standard global ::NTV2DeviceXXX behavior based on
	//	my NTV2DeviceID (register 50 value).
	return false;
}

bool NTV2SoftwareDevice::NTV2ReadRegisterRemote (const ULWord inRegNum, ULWord & outRegValue, const ULWord inRegMask, const ULWord inRegShift)
{
	outRegValue = 0;
	if (inRegShift > 31)
		return false;	//	Bad shift value

	AJAAutoLock lock(&sLock);
	if (!mRegMemory)
		return false;
	if (inRegNum * sizeof(ULWord) > mRegMemory.GetByteCount())
		return false;	//	Bad reg num

	ULWord value(mRegMemory.U32(int(inRegNum)) & inRegMask);
	if (inRegShift)
		value >>= inRegShift;
	outRegValue = value;
	return true;
}

bool NTV2SoftwareDevice::NTV2WriteRegisterRemote (const ULWord inRegNum, const ULWord inRegVal, const ULWord inRegMask, const ULWord inRegShift)
{
	if (inRegShift > 31)
		return false;

//	TODO:	Some registers aren't actually written until the next VBI.
//			These will need to be queued and processed as a group in a properly-timed VBI thread.
	AJAAutoLock lock(&sLock);
	if (!mRegMemory)
		return false;
	if (inRegNum * sizeof(ULWord) > mRegMemory.GetByteCount())
		return false;	//	Bad reg num
	uint32_t & reg(mRegMemory.U32(int(inRegNum)));
//	ULWord oldValue(reg & inRegMask);
	ULWord newValue(inRegVal & inRegMask);
	if (inRegShift)
		newValue <<= inRegShift;
	reg &= ~inRegMask;
	reg |= newValue;
	return true;
}


bool NTV2SoftwareDevice::NTV2AutoCirculateRemote (AUTOCIRCULATE_DATA & autoCircData)
{
	autoCircData = AUTOCIRCULATE_DATA();
	AJAAutoLock lock(&sLock);
	if (!spFakeDevice)
		return false;
	if (spFakeDevice->fVersion != 1)
		return false;
	if (!mACMemory)
		return false;
	//	INTERNAL_AUTOCIRCULATE_STRUCT * pACStruct = reinterpret_cast<INTERNAL_AUTOCIRCULATE_STRUCT*>(mACMemory.GetHostPointer());
	//	TBD
	return true;
}

bool NTV2SoftwareDevice::NTV2WaitForInterruptRemote (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs)
{	(void)eInterrupt; (void)timeOutMs;
	AJAAutoLock lock(&sLock);
	if (!spFakeDevice)
		return false;
	if (spFakeDevice->fVersion != 1)
		return false;
	//	TBD
	return true;
}

bool NTV2SoftwareDevice::NTV2DMATransferRemote (const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
												const ULWord inFrameNumber,			NTV2Buffer & inOutBuffer,
												const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
												const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,	const bool inSynchronous)
{
	AJAAutoLock lock(&sLock);
	if (!mFBMemory)
		return false;
	if (inDMAEngine < NTV2_DMA1  ||  inDMAEngine > NTV2_DMA_FIRST_AVAILABLE)
		return false;	//	Bad DMA engine
	if (!inSynchronous)
		return false;	//	Must be synchronous

	if (inNumSegments)
	{
		NTV2SegmentedXferInfo	xferInfo;
		xferInfo.setDestOffset(inCardOffsetBytes).setSegmentCount(inNumSegments);
		xferInfo.setSegmentLength(inOutBuffer.GetByteCount() / inNumSegments);
		xferInfo.setSourceOffset(0);
		xferInfo.setSourcePitch(inSegmentHostPitch);
		xferInfo.setDestOffset(inFrameNumber * 8ULL*1024ULL*1024ULL + inCardOffsetBytes);	//	!!! ASSUMES 8MB FRAMES!
		xferInfo.setDestPitch(inSegmentCardPitch);
		if (inIsRead)
			return inOutBuffer.CopyFrom(mFBMemory, xferInfo);
		else
			return mFBMemory.CopyFrom(inOutBuffer, xferInfo);
	}
	else
	{
		if (inIsRead)
			return inOutBuffer.CopyFrom(mFBMemory, inFrameNumber * 8UL*1024ULL*1024ULL,  0,  inOutBuffer.GetByteCount());
		else
			return mFBMemory.CopyFrom(inOutBuffer, inFrameNumber * 8UL*1024ULL*1024ULL,  0,  inOutBuffer.GetByteCount());
	}
}

bool NTV2SoftwareDevice::NTV2MessageRemote (NTV2_HEADER * pInMessage)
{
	//	Validation & sanity checks...
	if (!pInMessage)
		return false;
	if (!pInMessage->IsValid())
		{NBFAIL ("Bad NTV2_HEADER or struct type");  return false;}
	if (pInMessage->GetHeaderVersion() != NTV2_CURRENT_HEADER_VERSION)
		{NBFAIL ("Bad or unsupported NTV2_HEADER version");  return false;}
	if (pInMessage->GetSizeInBytes() < (sizeof(NTV2_HEADER) + sizeof(NTV2_TRAILER)))
		{NBFAIL("Struct size smaller than NTV2_HEADER and NTV2_TRAILER size");  return false;}
	if (pInMessage->GetPointerSize() != 4  &&  pInMessage->GetPointerSize() != 8)
		{NBFAIL("Host pointer size " << DEC(pInMessage->GetPointerSize()) << " must be 4 or 8");  return false;}
	const NTV2_TRAILER * pTrailer = reinterpret_cast<const NTV2_TRAILER*>(reinterpret_cast<const UByte*>(pInMessage) + pInMessage->GetSizeInBytes() - sizeof(NTV2_TRAILER));
	if (!NTV2_IS_VALID_TRAILER_TAG(pTrailer->fTrailerTag))
		{NBFAIL("Bad NTV2_TRAILER tag");  return false;}

	//	Dispatch...
/**	switch (pInMessage->GetType())
	{
		case NTV2_TYPE_ACSTATUS:		return !AutoCirculateGetStatus (ACContext(), AsAUTOCIRCULATE_STATUS(pInMessage));
		case NTV2_TYPE_ACXFER:			return !AutoCirculateTransfer (ACContext(), AsAUTOCIRCULATE_TRANSFER(pInMessage));
//		case NTV2_TYPE_GETREGS:			return GetRegistersImmediate (ACContext(), AsNTV2GetRegisters(pInMessage));
//		case NTV2_TYPE_SETREGS:			return SetRegistersImmediate (AsNTV2SetRegisters(pInMessage));
		case NTV2_TYPE_ACFRAMESTAMP:	return AutoCirculateFrameStampImmediate (AsFRAME_STAMP(pInMessage), pClientTask);
		case NTV2_TYPE_AJABUFFERLOCK:	return BufferLockImmediate (AsNTV2BufferLock(pInMessage), pClientTask);
		case NTV2_TYPE_AJABITSTREAM:	return BitstreamImmediate (AsNTV2Bitstream(pInMessage), pClientTask);

		case NTV2_TYPE_BANKGETSET:
			if (AsNTV2BankSelGetSetRegs(pInOutStruct)->mIsWriting)
				return BankSelSetRegistersImmediate (AsNTV2BankSelGetSetRegs (pInOutStruct));
			else
				return BankSelGetRegistersImmediate (AsNTV2BankSelGetSetRegs (pInOutStruct));

		case NTV2_TYPE_VIRTUAL_DATA_RW:
			if (AsNTV2NTV2VirtualData (pInOutStruct)->mIsWriting)
				return VirtualDataSetImmediate (AsNTV2NTV2VirtualData (pInOutStruct));
			else
				return VirtualDataGetImmediate (AsNTV2NTV2VirtualData (pInOutStruct));
		default:	break;
	}
**/
	NBFAIL("Unhandled message type " << xHEX0N(pInMessage->GetType(),8));
	return false;
}

void NTV2SoftwareDevice::InitRegs (void)
{
	NTV2WriteRegisterRemote (kRegGlobalControl, 0x30000202);  // Reg 0  // Frame Rate: 59.94, Frame Geometry: 1920x1080, Standard: 1080p, Reference Source: Reference In, Ch 2 link B 1080p 50/60: Off, LEDs ...., Register Clocking: Sync To Field, Ch 1 RP-188 output: Enabled, Ch 2 RP-188 output: Enabled, Color Correction: Channel: 1 Bank 0
	NTV2WriteRegisterRemote (kRegCh1Control, 0x00200080);  // Reg 1  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 8 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh1PCIAccessFrame, 0x00000001);  // Reg 2
	NTV2WriteRegisterRemote (kRegCh1OutputFrame, 0x0000000E);  // Reg 3
	NTV2WriteRegisterRemote (kRegCh1InputFrame, 0x00000003);  // Reg 4
	NTV2WriteRegisterRemote (kRegCh2Control, 0x00200080);  // Reg 5  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 8 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh2PCIAccessFrame, 0x00000005);  // Reg 6
	NTV2WriteRegisterRemote (kRegCh2OutputFrame, 0x00000004);  // Reg 7
	NTV2WriteRegisterRemote (kRegCh2InputFrame, 0x00000007);  // Reg 8
	NTV2WriteRegisterRemote (kRegMixer1Coefficient, 0x00010000);  // Reg 11
	NTV2WriteRegisterRemote (kRegFlatMatteValue, 0x20080200);  // Reg 13  // Flat Matte Cb: 200, Flat Matte Y: 1C0, Flat Matte Cr: 200
	NTV2WriteRegisterRemote (kRegOutputTimingControl, 0x08001000);  // Reg 14
	NTV2WriteRegisterRemote (kRegFlashProgramReg, 0x12345678);  // Reg 17
	NTV2WriteRegisterRemote (kRegLineCount, 0x00000142);  // Reg 18
	NTV2WriteRegisterRemote (kRegAud1Delay, 0x1FDF0000);  // Reg 19
	NTV2WriteRegisterRemote (kRegVidIntControl, 0x001C01C7);  // Reg 20  // Output 1 Vertical Enable: Y, Input 1 Vertical Enable: Y, Input 2 Vertical Enable: Y, Audio Out Wrap Interrupt Enable: N, Audio In Wrap Interrupt Enable: N, Wrap Rate Interrupt Enable: Y, UART Tx Interrupt EnableY, UART Rx Interrupt EnableY, UART Rx Interrupt ClearInactive, UART 2 Tx Interrupt EnableN, Output 2 Vertical Enable: Y, Output 3 Vertical Enable: Y, Output 4 Vertical Enable: Y, Output 4 Vertical Clear: Inactive, Output 3 Vertical Clear: Inactive, Output 2 Vertical Clear: Inactive, UART Tx Interrupt ClearInactive, Wrap Rate Interrupt ClearInactive, UART 2 Tx Interrupt ClearInactive, Audio Out Wrap Interrupt ClearInactive, Input 2 Vertical Clear: Inactive, Input 1 Vertical Clear: Inactive, Output 1 Vertical Clear: Inactive
	NTV2WriteRegisterRemote (kRegStatus, 0x00200000);  // Reg 21  // Input 1 Vertical Blank: Inactive, Input 1 Field ID: 1, Input 1 Vertical Interrupt: Inactive, Input 2 Vertical Blank: Inactive, Input 2 Field ID: 0, Input 2 Vertical Interrupt: Inactive, Output 1 Vertical Blank: Inactive, Output 1 Field ID: 0, Output 1 Vertical Interrupt: Inactive, Output 2 Vertical Blank: Inactive, Output 2 Field ID: 0, Output 2 Vertical Interrupt: Inactive, Aux Vertical Interrupt: Inactive, I2C 1 Interrupt: Inactive, I2C 2 Interrupt: Inactive, Chunk Rate Interrupt: Inactive, Wrap Rate Interrupt: Inactive, Audio Out Wrap Interrupt: Inactive, Audio 50Hz Interrupt: Inactive
	NTV2WriteRegisterRemote (kRegInputStatus, 0x0F000000);  // Reg 22  // Input 1 Frame Rate: Unknown, Input 1 Geometry: Unknown, Input 1 Scan Mode: Interlaced, Input 2 Frame Rate: Unknown, Input 2 Geometry: Unknown, Input 2 Scan Mode: Interlaced, Reference Frame Rate: Unknown, Reference Geometry: Unknown, Reference Scan Mode: Interlaced, AES Channel 1-2: Invalid, AES Channel 3-4: Invalid, AES Channel 5-6: Invalid, AES Channel 7-8: Invalid
	NTV2WriteRegisterRemote (kRegAud1Control, 0xA8F00300);  // Reg 24  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut1: Enabled, Audio Embedder SDIOut2: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 16-Channel , 48kHz, 48kHz Support, Embedded Support, 8-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Present, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegAud1SourceSelect, 0xF0004321);  // Reg 25  // Audio Source: Embedded Groups 1 and 2, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Board Reference, 3G audio source: Data stream 1
	NTV2WriteRegisterRemote (kRegAud1Counter, 0xE618F83A);  // Reg 28
	NTV2WriteRegisterRemote (kRegRP188InOut1DBB, 0xFF000000);  // Reg 29  // RP188: No RP-188 received, Bypass: Disabled, Filter: FF, DBB: 00 00
	NTV2WriteRegisterRemote (kRegDMA1HostAddr, 0x01D22F78);  // Reg 32
	NTV2WriteRegisterRemote (kRegDMA1LocalAddr, 0x01525840);  // Reg 33
	NTV2WriteRegisterRemote (kRegDMA2HostAddr, 0x0103E210);  // Reg 36
	NTV2WriteRegisterRemote (kRegDMA2LocalAddr, 0x007E9000);  // Reg 37
	NTV2WriteRegisterRemote (kRegDMAControl, 0x01243C00);  // Reg 48  // DMA 1 Int Active?: N, DMA 2 Int Active?: N, DMA 3 Int Active?: N, DMA 4 Int Active?: N, Bus Error Int Active?: N, DMA 1 Busy?: N, DMA 2 Busy?: N, DMA 3 Busy?: N, DMA 4 Busy?: N, Strap: Not Installed, Firmware Rev: 0x3C (60), Gen: 2, Lanes: 4
	NTV2WriteRegisterRemote (kRegBoardID, 0x10538200);  // Reg 50		DEVICE_ID_CORVID88
	NTV2WriteRegisterRemote (kRegReserved54, 0x38543030);  // Reg 54
	NTV2WriteRegisterRemote (kRegReserved55, 0x32393735);  // Reg 55
	NTV2WriteRegisterRemote (kRegXenaxFlashControlStatus, 0x40009C0B);  // Reg 58
	NTV2WriteRegisterRemote (kRegXenaxFlashAddress, 0x000000FC);  // Reg 59
	NTV2WriteRegisterRemote (kRegXenaxFlashDOUT, 0x6B26B241);  // Reg 61
	NTV2WriteRegisterRemote (kRegCPLDVersion, 0x00000003);  // Reg 63  // CPLD Version: 3, Failsafe Bitfile Loaded: No, Force Reload: N
	NTV2WriteRegisterRemote (kRegRP188InOut2DBB, 0xFF0000FF);  // Reg 64  // RP188: No RP-188 received, Bypass: Disabled, Filter: FF, DBB: 00 FF
	NTV2WriteRegisterRemote (kRegCanDoStatus, 0x00000003);  // Reg 67  // Has CanConnect Xpt Route ROM: Y
	NTV2WriteRegisterRemote (kRegRS422Control, 0x0000010B);  // Reg 72
	NTV2WriteRegisterRemote (kRegBitfileDate, 0x20200325);  // Reg 88  // Bitfile Date: 03/25/2020
	NTV2WriteRegisterRemote (kRegBitfileTime, 0x00151600);  // Reg 89  // Bitfile Time: 15:16:00
	NTV2WriteRegisterRemote (kRegSysmonVccIntDieTemp, 0x53C1ABE1);  // Reg 119  // Die Temperature: 64.97 Celcius  (148.94 Fahrenheit, Core Voltage:  0.98 Volts DC
	NTV2WriteRegisterRemote (kRegInternalExternalVoltage, 0x000798A4);  // Reg 120
	NTV2WriteRegisterRemote (kRegHDMIOut3DControl, 0x00000080);  // Reg 124
	NTV2WriteRegisterRemote (kRegSDIOut1Control, 0x01040084);  // Reg 129  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x40, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut2Control, 0x01040084);  // Reg 130  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x40, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut3Control, 0x01040004);  // Reg 169  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut4Control, 0x01040004);  // Reg 170  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegAudioOutputSourceMap, 0x00003210);  // Reg 190  // AES Outputs 1-4 Source: AudSys1, Audio Channels 1-4, AES Outputs 5-8 Source: AudSys1, Audio Channels 5-8, AES Outputs 9-12 Source: AudSys1, Audio Channels 9-12, AES Outputs 13-16 Source: AudSys1, Audio Channels 13-16, Analog Audio Monitor Output Source: AudSys1, Channels 1-2, HDMI 2-Chl Audio Output Source: AudSys1, Channels 1-2, or HDMI 8-Chl Audio Output 1-4 Source: AudSys1, Channels 1-4, or HDMI 8-Chl Audio Output 5-8 Source: AudSys1, Channels 1-4
	NTV2WriteRegisterRemote (kRegRP188InOut5Bits0_31_2, 0x63030608);  // Reg 210
	NTV2WriteRegisterRemote (kRegRP188InOut5Bits32_63_2, 0x00000004);  // Reg 211
	NTV2WriteRegisterRemote (kRegRP188InOut6Bits0_31_2, 0x63030608);  // Reg 212
	NTV2WriteRegisterRemote (kRegRP188InOut6Bits32_63_2, 0x00000004);  // Reg 213
	NTV2WriteRegisterRemote (kRegRP188InOut7Bits0_31_2, 0x63030608);  // Reg 214
	NTV2WriteRegisterRemote (kRegRP188InOut7Bits32_63_2, 0x00000004);  // Reg 215
	NTV2WriteRegisterRemote (kRegRP188InOut8Bits0_31_2, 0x63030608);  // Reg 216
	NTV2WriteRegisterRemote (kRegRP188InOut8Bits32_63_2, 0x00000004);  // Reg 217
	NTV2WriteRegisterRemote (kRegSDIInput3GStatus, 0x000C0000);  // Reg 232
	NTV2WriteRegisterRemote (kRegAud2Control, 0x80C00300);  // Reg 240  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, Embedded Support, 8-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegRS4222Control, 0x00000010);  // Reg 246
	NTV2WriteRegisterRemote (kRegFlatMatte2Value, 0x18080240);  // Reg 249  // Flat Matte Cb: 240, Flat Matte Y: 1C0, Flat Matte Cr: 180
	NTV2WriteRegisterRemote (kRegSDITransmitControl, 0xF0000000);  // Reg 256  // (Bi-directional SDI not supported)
	NTV2WriteRegisterRemote (kRegCh3Control, 0x00000080);  // Reg 257  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh4Control, 0x00000080);  // Reg 260  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegStatus2, 0x00015400);  // Reg 265  // Input 3 Vertical Blank: Inactive, Input 3 Field ID: 0, Input 3 Vertical Interrupt: Inactive, Input 4 Vertical Blank: Inactive, Input 4 Field ID: 0, Input 4 Vertical Interrupt: Inactive, Input 5 Vertical Blank: Active, Input 5 Field ID: 0, Input 5 Vertical Interrupt: Inactive, Input 6 Vertical Blank: Active, Input 6 Field ID: 0, Input 6 Vertical Interrupt: Inactive, Input 7 Vertical Blank: Active, Input 7 Field ID: 0, Input 7 Vertical Interrupt: Inactive, Input 8 Vertical Blank: Active, Input 8 Field ID: 0, Input 8 Vertical Interrupt: Inactive, Output 5 Vertical Blank: Inactive, Output 5 Field ID: 0, Output 5 Vertical Interrupt: Inactive, Output 6 Vertical Blank: Inactive, Output 6 Field ID: 0, Output 6 Vertical Interrupt: Inactive, Output 7 Vertical Blank: Inactive, Output 7 Field ID: 0, Output 7 Vertical Interrupt: Inactive, Output 8 Vertical Blank: Inactive, Output 8 Field ID: 0, Output 8 Vertical Interrupt: Inactive, HDMI In Hot-Plug Detect Interrupt: Inactive, HDMI In Chip Interrupt: Inactive
	NTV2WriteRegisterRemote (kRegVidIntControl2, 0x0000FF06);  // Reg 266  // Input 3 Vertical Enable: Y, Input 4 Vertical Enable: Y, Input 5 Vertical Enable: Y, Input 6 Vertical Enable: Y, Input 7 Vertical Enable: Y, Input 8 Vertical Enable: Y, Output 5 Vertical Enable: Y, Output 6 Vertical Enable: Y, Output 7 Vertical Enable: Y, Output 8 Vertical Enable: Y, Output 8 Vertical Clear: Inactive, Output 7 Vertical Clear: Inactive, Output 6 Vertical Clear: Inactive, Output 5 Vertical Clear: Inactive, Input 8 Vertical Clear: Inactive, Input 7 Vertical Clear: Inactive, Input 6 Vertical Clear: Inactive, Input 5 Vertical Clear: Inactive, Input 4 Vertical Clear: Inactive, Input 3 Vertical Clear: Inactive
	NTV2WriteRegisterRemote (kRegGlobalControl2, 0xBC021009);  // Reg 267  // Reference source bit 4: Set, Quad Mode Channel 1-4: Set, Quad Mode Channel 5-8: Set, Independent Channel Mode: Not Set, 2MB Frame Support: Supported, Audio Mixer: Not Present, Is DNXIV Product: N, Audio 1 Play/Capture Mode: Off, Audio 2 Play/Capture Mode: Off, Audio 3 Play/Capture Mode: Off, Audio 4 Play/Capture Mode: Off, Audio 5 Play/Capture Mode: Off, Audio 6 Play/Capture Mode: Off, Audio 7 Play/Capture Mode: Off, Audio 8 Play/Capture Mode: Off, Ch 3 RP188 Output: Enabled, Ch 4 RP188 Output: Enabled, Ch 5 RP188 Output: Disabled, Ch 6 RP188 Output: Enabled, Ch 7 RP188 Output: Enabled, Ch 8 RP188 Output: Enabled, Ch 4 1080p50/p60 Link-B Mode: Disabled, Ch 6 1080p50/p60 Link-B Mode: Disabled, Ch 8 1080p50/p60 Link-B Mode: Disabled, Ch 1/2 2SI Mode: Disabled, Ch 2/3 2SI Mode: Disabled, Ch 3/4 2SI Mode: Disabled, Ch 4/5 2SI Mode: Disabled, 2SI Min Align Delay 1-4: Disabled, 2SI Min Align Delay 5-8: Disabled
	NTV2WriteRegisterRemote (kRegRP188InOut3DBB, 0x020000FF);  // Reg 268  // RP188: No RP-188 received, Bypass: Disabled, Filter: 02, DBB: 00 FF
	NTV2WriteRegisterRemote (kRegRP188InOut4DBB, 0x020000FF);  // Reg 273  // RP188: No RP-188 received, Bypass: Disabled, Filter: 02, DBB: 00 FF
	NTV2WriteRegisterRemote (kRegAud3Control, 0x80000300);  // Reg 278  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut3: Enabled, Audio Embedder SDIOut4: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegAud4Control, 0x80000300);  // Reg 279  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegSDIInput3GStatus2, 0x00000100);  // Reg 287
	NTV2WriteRegisterRemote (kRegInputStatus2, 0x00008000);  // Reg 288  // Input 3 Scan Mode: Interlaced, Input 3 Frame Rate: Unknown, Input 3 Geometry: Unknown, Input 4 Scan Mode: Progressive, Input 4 Frame Rate: Unknown, Input 4 Geometry: Unknown
	NTV2WriteRegisterRemote (kRegSDIOut5Control, 0x01000084);  // Reg 337  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x40, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 1, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegRP188InOut5Bits0_31, 0x6B030607);  // Reg 340
	NTV2WriteRegisterRemote (kRegRP188InOut5Bits32_63, 0x00000004);  // Reg 341
	NTV2WriteRegisterRemote (kRegRP188InOut5DBB, 0x020D0002);  // Reg 342  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 00 02
	NTV2WriteRegisterRemote (kRegLTC5EmbeddedBits0_31, 0x63030608);  // Reg 344
	NTV2WriteRegisterRemote (kRegLTC5EmbeddedBits32_63, 0x00000004);  // Reg 345
	NTV2WriteRegisterRemote (kRegReserved353, 0x00000CA3);  // Reg 353
	NTV2WriteRegisterRemote (kRegLUTV2Control, 0x000F1F00);  // Reg 376  // (Register data relevant for V2 LUT, this device has V0LUT)
	NTV2WriteRegisterRemote (kRegGlobalControlCh2, 0x00000202);  // Reg 377  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh3, 0x00000202);  // Reg 378  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh4, 0x00000202);  // Reg 379  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh5, 0x00000202);  // Reg 380  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh6, 0x00000202);  // Reg 381  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh7, 0x00000202);  // Reg 382  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh8, 0x00000202);  // Reg 383  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegCh5Control, 0x00000001);  // Reg 384  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh5InputFrame, 0x00000006);  // Reg 386
	NTV2WriteRegisterRemote (kRegCh6Control, 0x00000001);  // Reg 388  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh6OutputFrame, 0x00000008);  // Reg 389
	NTV2WriteRegisterRemote (kRegCh7Control, 0x00000001);  // Reg 392  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh8Control, 0x00000001);  // Reg 396  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegSDIIn5VPIDA, 0x0180CA89);  // Reg 410  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn5VPIDB, 0x4180CA89);  // Reg 411  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn6VPIDA, 0x0180CA89);  // Reg 412  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn6VPIDB, 0x4180CA89);  // Reg 413  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegRP188InOut6Bits0_31, 0x38000400);  // Reg 416
	NTV2WriteRegisterRemote (kRegRP188InOut6DBB, 0x020D0200);  // Reg 418  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 02 00
	NTV2WriteRegisterRemote (kRegLTC6EmbeddedBits0_31, 0x63030608);  // Reg 419
	NTV2WriteRegisterRemote (kRegLTC6EmbeddedBits32_63, 0x00000004);  // Reg 420
	NTV2WriteRegisterRemote (kRegSDIIn7VPIDA, 0x0180CA89);  // Reg 421  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn7VPIDB, 0x4180CA89);  // Reg 422  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegRP188InOut7Bits0_31, 0x38000400);  // Reg 425
	NTV2WriteRegisterRemote (kRegRP188InOut7DBB, 0x020D00FF);  // Reg 427  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 00 FF
	NTV2WriteRegisterRemote (kRegLTC7EmbeddedBits0_31, 0x63030608);  // Reg 428
	NTV2WriteRegisterRemote (kRegLTC7EmbeddedBits32_63, 0x00000004);  // Reg 429
	NTV2WriteRegisterRemote (kRegSDIIn8VPIDA, 0x0180CA89);  // Reg 430  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn8VPIDB, 0x4180CA89);  // Reg 431  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegRP188InOut8Bits0_31, 0x38000400);  // Reg 434
	NTV2WriteRegisterRemote (kRegRP188InOut8DBB, 0x020D0200);  // Reg 436  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 02 00
	NTV2WriteRegisterRemote (kRegLTC8EmbeddedBits0_31, 0x63030608);  // Reg 437
	NTV2WriteRegisterRemote (kRegLTC8EmbeddedBits32_63, 0x00000004);  // Reg 438
	NTV2WriteRegisterRemote (kRegAud5Control, 0x80100300);  // Reg 440  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut5: Enabled, Audio Embedder SDIOut6: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 16-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegAud5SourceSelect, 0x00400001);  // Reg 441  // Audio Source: Embedded Groups 1 and 2, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Video Input, 3G audio source: Data stream 1
	NTV2WriteRegisterRemote (kRegAud6Control, 0x00000300);  // Reg 444  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 1 MB
	NTV2WriteRegisterRemote (kRegAud7Control, 0x00000300);  // Reg 448  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut7: Enabled, Audio Embedder SDIOut8: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 1 MB
	NTV2WriteRegisterRemote (kRegAud8Control, 0x00000300);  // Reg 452  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 1 MB
	NTV2WriteRegisterRemote (kRegAudioDetect5678, 0xFFFFFFFF);  // Reg 456
	NTV2WriteRegisterRemote (kRegSDI5678Input3GStatus, 0x31313131);  // Reg 457
	NTV2WriteRegisterRemote (kRegInput56Status, 0x0000C2C2);  // Reg 458  // Input 5 Scan Mode: Progressive, Input 5 Frame Rate: 59.94, Input 5 Geometry: 1125, Input 6 Scan Mode: Progressive, Input 6 Frame Rate: 59.94, Input 6 Geometry: 1125
	NTV2WriteRegisterRemote (kRegInput78Status, 0x0000C2C2);  // Reg 459  // Input 7 Scan Mode: Progressive, Input 7 Frame Rate: 59.94, Input 7 Geometry: 1125, Input 8 Scan Mode: Progressive, Input 8 Frame Rate: 59.94, Input 8 Geometry: 1125
	NTV2WriteRegisterRemote (kRegSDIOut6Control, 0x01040004);  // Reg 475  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut7Control, 0x01040004);  // Reg 476  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut8Control, 0x01040004);  // Reg 477  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegOutputTimingControlch2, 0x08001000);  // Reg 478
	NTV2WriteRegisterRemote (kRegOutputTimingControlch3, 0x08001000);  // Reg 479
	NTV2WriteRegisterRemote (kRegOutputTimingControlch4, 0x08001000);  // Reg 480
	NTV2WriteRegisterRemote (kRegOutputTimingControlch5, 0x08001000);  // Reg 481
	NTV2WriteRegisterRemote (kRegOutputTimingControlch6, 0x08001000);  // Reg 482
	NTV2WriteRegisterRemote (kRegOutputTimingControlch7, 0x08001000);  // Reg 483
	NTV2WriteRegisterRemote (kRegOutputTimingControlch8, 0x08001000);  // Reg 484
	NTV2WriteRegisterRemote (kRegVidProc3Control, 0x00200000);  // Reg 485  // Mode: Full Raster, FG Control: Unshaped, BG Control: Full Raster, VANC Pass-Thru: Foreground, FG Matte: Disabled, BG Matte: Disabled, Input Sync: in sync, Limiting: Legal SDI, Split Video Std: 1080i
	NTV2WriteRegisterRemote (kRegFlatMatte3Value, 0x18080240);  // Reg 487  // Flat Matte Cb: 240, Flat Matte Y: 1C0, Flat Matte Cr: 180
	NTV2WriteRegisterRemote (kRegFlatMatte4Value, 0x18080240);  // Reg 490  // Flat Matte Cb: 240, Flat Matte Y: 1C0, Flat Matte Cr: 180
	NTV2WriteRegisterRemote (kRegTRSErrorStatus, 0x00F00000);  // Reg 491
	NTV2WriteRegisterRemote (kRegRXSDI1FrameCountLow, 0x00D56B2B);  // Reg 2050
	NTV2WriteRegisterRemote (kRegRXSDI1FrameRefCountLow, 0x9C3966CF);  // Reg 2052
	NTV2WriteRegisterRemote (kRegRXSDI1FrameRefCountHigh, 0x00000003);  // Reg 2053
	NTV2WriteRegisterRemote (kRegRXSDI2FrameCountLow, 0x00A2E900);  // Reg 2058
	NTV2WriteRegisterRemote (kRegRXSDI2FrameRefCountLow, 0x1AB48C3B);  // Reg 2060
	NTV2WriteRegisterRemote (kRegRXSDI3FrameCountLow, 0x00F6602B);  // Reg 2066
	NTV2WriteRegisterRemote (kRegRXSDI3FrameRefCountLow, 0x1AF13D4F);  // Reg 2068
	NTV2WriteRegisterRemote (kRegRXSDI4FrameCountLow, 0x000480B4);  // Reg 2074
	NTV2WriteRegisterRemote (kRegRXSDI4FrameRefCountLow, 0x1AAD9B03);  // Reg 2076
	NTV2WriteRegisterRemote (kRegRXSDI5Status, 0x00310010);  // Reg 2080
	NTV2WriteRegisterRemote (kRegRXSDI5CRCErrorCount, 0x00000015);  // Reg 2081
	NTV2WriteRegisterRemote (kRegRXSDI5FrameCountLow, 0x00516EF4);  // Reg 2082
	NTV2WriteRegisterRemote (kRegRXSDI5FrameRefCountLow, 0xDABA55D6);  // Reg 2084
	NTV2WriteRegisterRemote (kRegRXSDI5FrameRefCountHigh, 0x000024C8);  // Reg 2085
	NTV2WriteRegisterRemote (kRegRXSDI6Status, 0x0031000C);  // Reg 2088
	NTV2WriteRegisterRemote (kRegRXSDI6CRCErrorCount, 0x0000CC70);  // Reg 2089
	NTV2WriteRegisterRemote (kRegRXSDI6FrameCountLow, 0x020AD1CE);  // Reg 2090
	NTV2WriteRegisterRemote (kRegRXSDI6FrameRefCountLow, 0xDABA55D7);  // Reg 2092
	NTV2WriteRegisterRemote (kRegRXSDI6FrameRefCountHigh, 0x000024C8);  // Reg 2093
	NTV2WriteRegisterRemote (kRegRXSDI7Status, 0x00310007);  // Reg 2096
	NTV2WriteRegisterRemote (kRegRXSDI7CRCErrorCount, 0x00003CB2);  // Reg 2097
	NTV2WriteRegisterRemote (kRegRXSDI7FrameCountLow, 0x00C00088);  // Reg 2098
	NTV2WriteRegisterRemote (kRegRXSDI7FrameRefCountLow, 0xDABA55D7);  // Reg 2100
	NTV2WriteRegisterRemote (kRegRXSDI7FrameRefCountHigh, 0x000024C8);  // Reg 2101
	NTV2WriteRegisterRemote (kRegRXSDI8Status, 0x00310007);  // Reg 2104
	NTV2WriteRegisterRemote (kRegRXSDI8CRCErrorCount, 0x000164D5);  // Reg 2105
	NTV2WriteRegisterRemote (kRegRXSDI8FrameCountLow, 0x00642D78);  // Reg 2106
	NTV2WriteRegisterRemote (kRegRXSDI8FrameRefCountLow, 0xDABA55D8);  // Reg 2108
	NTV2WriteRegisterRemote (kRegRXSDI8FrameRefCountHigh, 0x000024C8);  // Reg 2109
	NTV2WriteRegisterRemote (kRegRXSDIFreeRunningClockLow, 0x18188497);  // Reg 2112
	NTV2WriteRegisterRemote (kRegRXSDIFreeRunningClockHigh, 0x00002F12);  // Reg 2113
	NTV2WriteRegisterRemote (4096, 0x10011111);  // Extract 1 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4097, 0x30000000);  // Extract 1 F1 Start Address
	NTV2WriteRegisterRemote (4098, 0x301FFFFF);  // Extract 1 F1 End Address
	NTV2WriteRegisterRemote (4099, 0x30200000);  // Extract 1 F2 Start Address
	NTV2WriteRegisterRemote (4100, 0x303FFFFF);  // Extract 1 F2 End Address
	NTV2WriteRegisterRemote (4101, 0x00640064);  // Extract 1 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4105, 0x00000462);  // Extract 1 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4106, 0x00000465);  // Extract 1 Lines Per Frame
	NTV2WriteRegisterRemote (4108, 0xE4E5E6E7);  // Extract 1 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4109, 0xE0E1E2E3);  // Extract 1 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4110, 0xA4A5A6A7);  // Extract 1 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4111, 0xA0A1A2A3);  // Extract 1 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4112, 0xE7E7E7E7);  // Extract 1 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4113, 0x010A0004);  // Extract 1 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4123, 0x00000780);  // Reg 0x101B
	NTV2WriteRegisterRemote (4160, 0x10011111);  // Extract 2 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4161, 0x30800000);  // Extract 2 F1 Start Address
	NTV2WriteRegisterRemote (4162, 0x309FFFFF);  // Extract 2 F1 End Address
	NTV2WriteRegisterRemote (4163, 0x30A00000);  // Extract 2 F2 Start Address
	NTV2WriteRegisterRemote (4164, 0x30BFFFFF);  // Extract 2 F2 End Address
	NTV2WriteRegisterRemote (4165, 0x00640064);  // Extract 2 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4169, 0x00000462);  // Extract 2 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4170, 0x00000465);  // Extract 2 Lines Per Frame
	NTV2WriteRegisterRemote (4172, 0xE4E5E6E7);  // Extract 2 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4173, 0xE0E1E2E3);  // Extract 2 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4174, 0xA4A5A6A7);  // Extract 2 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4175, 0xA0A1A2A3);  // Extract 2 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4176, 0xE7E7E7E7);  // Extract 2 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4177, 0x010A0004);  // Extract 2 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4187, 0x00000780);  // Reg 0x105B
	NTV2WriteRegisterRemote (4224, 0x10011111);  // Extract 3 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4225, 0x31000000);  // Extract 3 F1 Start Address
	NTV2WriteRegisterRemote (4226, 0x311FFFFF);  // Extract 3 F1 End Address
	NTV2WriteRegisterRemote (4227, 0x31200000);  // Extract 3 F2 Start Address
	NTV2WriteRegisterRemote (4228, 0x313FFFFF);  // Extract 3 F2 End Address
	NTV2WriteRegisterRemote (4229, 0x00640064);  // Extract 3 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4233, 0x00000462);  // Extract 3 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4234, 0x00000465);  // Extract 3 Lines Per Frame
	NTV2WriteRegisterRemote (4236, 0xE4E5E6E7);  // Extract 3 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4237, 0xE0E1E2E3);  // Extract 3 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4238, 0xA4A5A6A7);  // Extract 3 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4239, 0xA0A1A2A3);  // Extract 3 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4240, 0xE7E7E7E7);  // Extract 3 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4241, 0x010A0004);  // Extract 3 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4251, 0x00000780);  // Reg 0x109B
	NTV2WriteRegisterRemote (4288, 0x10011111);  // Extract 4 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4289, 0x31800000);  // Extract 4 F1 Start Address
	NTV2WriteRegisterRemote (4290, 0x319FFFFF);  // Extract 4 F1 End Address
	NTV2WriteRegisterRemote (4291, 0x31A00000);  // Extract 4 F2 Start Address
	NTV2WriteRegisterRemote (4292, 0x31BFFFFF);  // Extract 4 F2 End Address
	NTV2WriteRegisterRemote (4293, 0x00640064);  // Extract 4 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4297, 0x00000462);  // Extract 4 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4298, 0x00000465);  // Extract 4 Lines Per Frame
	NTV2WriteRegisterRemote (4300, 0xE4E5E6E7);  // Extract 4 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4301, 0xE0E1E2E3);  // Extract 4 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4302, 0xA4A5A6A7);  // Extract 4 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4303, 0xA0A1A2A3);  // Extract 4 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4304, 0xE7E7E7E7);  // Extract 4 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4305, 0x010A0004);  // Extract 4 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4315, 0x00000780);  // Reg 0x10DB
	NTV2WriteRegisterRemote (4352, 0x11010000);  // Extract 5 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Progressive video: Y, Synchronize: frame, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4353, 0x0DFFC000);  // Extract 5 F1 Start Address
	NTV2WriteRegisterRemote (4354, 0x0DFFDFFF);  // Extract 5 F1 End Address
	NTV2WriteRegisterRemote (4357, 0x00000465);  // Extract 5 Field Cutoff Lines  // F1 cutoff line: 1125, F2 cutoff line: 0
	NTV2WriteRegisterRemote (4360, 0x00000017);  // Extract 5 F2 Memory Usage  // Total F2 bytes: 23, Overrun: N
	NTV2WriteRegisterRemote (4361, 0x00000462);  // Extract 5 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4362, 0x00000465);  // Extract 5 Lines Per Frame
	NTV2WriteRegisterRemote (4364, 0xE4E5E6E7);  // Extract 5 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4365, 0xE0E1E2E3);  // Extract 5 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4366, 0xA4A5A6A7);  // Extract 5 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4367, 0xA0A1A2A3);  // Extract 5 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4379, 0x00000780);  // Reg 0x111B
	NTV2WriteRegisterRemote (4416, 0x10011111);  // Extract 6 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4417, 0x3A800000);  // Extract 6 F1 Start Address
	NTV2WriteRegisterRemote (4418, 0x3A9FFFFF);  // Extract 6 F1 End Address
	NTV2WriteRegisterRemote (4419, 0x3AA00000);  // Extract 6 F2 Start Address
	NTV2WriteRegisterRemote (4420, 0x3ABFFFFF);  // Extract 6 F2 End Address
	NTV2WriteRegisterRemote (4421, 0x00640064);  // Extract 6 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4425, 0x00000462);  // Extract 6 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4426, 0x00000465);  // Extract 6 Lines Per Frame
	NTV2WriteRegisterRemote (4428, 0xE4E5E6E7);  // Extract 6 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4429, 0xE0E1E2E3);  // Extract 6 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4430, 0xA4A5A6A7);  // Extract 6 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4431, 0xA0A1A2A3);  // Extract 6 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4432, 0xE7E7E7E7);  // Extract 6 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4433, 0x010A0004);  // Extract 6 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4443, 0x00000780);  // Reg 0x115B
	NTV2WriteRegisterRemote (4480, 0x10011111);  // Extract 7 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4481, 0x3B000000);  // Extract 7 F1 Start Address
	NTV2WriteRegisterRemote (4482, 0x3B1FFFFF);  // Extract 7 F1 End Address
	NTV2WriteRegisterRemote (4483, 0x3B200000);  // Extract 7 F2 Start Address
	NTV2WriteRegisterRemote (4484, 0x3B3FFFFF);  // Extract 7 F2 End Address
	NTV2WriteRegisterRemote (4485, 0x00640064);  // Extract 7 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4489, 0x00000462);  // Extract 7 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4490, 0x00000465);  // Extract 7 Lines Per Frame
	NTV2WriteRegisterRemote (4492, 0xE4E5E6E7);  // Extract 7 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4493, 0xE0E1E2E3);  // Extract 7 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4494, 0xA4A5A6A7);  // Extract 7 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4495, 0xA0A1A2A3);  // Extract 7 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4496, 0xE7E7E7E7);  // Extract 7 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4497, 0x010A0004);  // Extract 7 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4507, 0x00000780);  // Reg 0x119B
	NTV2WriteRegisterRemote (4544, 0x10011111);  // Extract 8 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4545, 0x3B800000);  // Extract 8 F1 Start Address
	NTV2WriteRegisterRemote (4546, 0x3B9FFFFF);  // Extract 8 F1 End Address
	NTV2WriteRegisterRemote (4547, 0x3BA00000);  // Extract 8 F2 Start Address
	NTV2WriteRegisterRemote (4548, 0x3BBFFFFF);  // Extract 8 F2 End Address
	NTV2WriteRegisterRemote (4549, 0x00640064);  // Extract 8 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4553, 0x00000462);  // Extract 8 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4554, 0x00000465);  // Extract 8 Lines Per Frame
	NTV2WriteRegisterRemote (4556, 0xE4E5E6E7);  // Extract 8 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4557, 0xE0E1E2E3);  // Extract 8 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4558, 0xA4A5A6A7);  // Extract 8 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4559, 0xA0A1A2A3);  // Extract 8 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4560, 0xE7E7E7E7);  // Extract 8 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4561, 0x010A0004);  // Extract 8 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4571, 0x00000780);  // Reg 0x11DB
	NTV2WriteRegisterRemote (4609, 0x11000000);  // Insert 1 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4610, 0x30000000);  // Insert 1 F1 Start Address
	NTV2WriteRegisterRemote (4611, 0x30400000);  // Insert 1 F2 Start Address
	NTV2WriteRegisterRemote (4612, 0x00000008);  // Insert 1 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4613, 0x0000002A);  // Insert 1 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4614, 0x08980780);  // Insert 1 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4615, 0x00000465);  // Insert 1 Lines Per Frame
	NTV2WriteRegisterRemote (4617, 0x0000000A);  // Insert 1 Payload ID Control
	NTV2WriteRegisterRemote (4618, 0x0100CA59);  // Insert 1 Payload ID
	NTV2WriteRegisterRemote (4673, 0x11000000);  // Insert 2 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4674, 0x30800000);  // Insert 2 F1 Start Address
	NTV2WriteRegisterRemote (4675, 0x30C00000);  // Insert 2 F2 Start Address
	NTV2WriteRegisterRemote (4676, 0x00000008);  // Insert 2 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4677, 0x0000002A);  // Insert 2 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4678, 0x08980780);  // Insert 2 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4679, 0x00000465);  // Insert 2 Lines Per Frame
	NTV2WriteRegisterRemote (4681, 0x0000000A);  // Insert 2 Payload ID Control
	NTV2WriteRegisterRemote (4682, 0x0100CA59);  // Insert 2 Payload ID
	NTV2WriteRegisterRemote (4737, 0x11000000);  // Insert 3 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4738, 0x31000000);  // Insert 3 F1 Start Address
	NTV2WriteRegisterRemote (4739, 0x31400000);  // Insert 3 F2 Start Address
	NTV2WriteRegisterRemote (4740, 0x00000008);  // Insert 3 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4741, 0x0000002A);  // Insert 3 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4742, 0x08980780);  // Insert 3 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4743, 0x00000465);  // Insert 3 Lines Per Frame
	NTV2WriteRegisterRemote (4745, 0x0000000A);  // Insert 3 Payload ID Control
	NTV2WriteRegisterRemote (4746, 0x0100CA59);  // Insert 3 Payload ID
	NTV2WriteRegisterRemote (4801, 0x11000000);  // Insert 4 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4802, 0x31800000);  // Insert 4 F1 Start Address
	NTV2WriteRegisterRemote (4803, 0x31C00000);  // Insert 4 F2 Start Address
	NTV2WriteRegisterRemote (4804, 0x00000008);  // Insert 4 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4805, 0x0000002A);  // Insert 4 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4806, 0x08980780);  // Insert 4 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4807, 0x00000465);  // Insert 4 Lines Per Frame
	NTV2WriteRegisterRemote (4809, 0x0000000A);  // Insert 4 Payload ID Control
	NTV2WriteRegisterRemote (4810, 0x0100CA59);  // Insert 4 Payload ID
	NTV2WriteRegisterRemote (4865, 0x11000000);  // Insert 5 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4866, 0x30A00000);  // Insert 5 F1 Start Address
	NTV2WriteRegisterRemote (4867, 0x30E00000);  // Insert 5 F2 Start Address
	NTV2WriteRegisterRemote (4868, 0x00000008);  // Insert 5 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4869, 0x0000002A);  // Insert 5 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4870, 0x08980780);  // Insert 5 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4871, 0x00000465);  // Insert 5 Lines Per Frame
	NTV2WriteRegisterRemote (4873, 0x0000000A);  // Insert 5 Payload ID Control
	NTV2WriteRegisterRemote (4874, 0x0100CA59);  // Insert 5 Payload ID
	NTV2WriteRegisterRemote (4929, 0x11000000);  // Insert 6 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4930, 0x31200000);  // Insert 6 F1 Start Address
	NTV2WriteRegisterRemote (4931, 0x31600000);  // Insert 6 F2 Start Address
	NTV2WriteRegisterRemote (4932, 0x00000008);  // Insert 6 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4933, 0x0000002A);  // Insert 6 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4934, 0x08980780);  // Insert 6 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4935, 0x00000465);  // Insert 6 Lines Per Frame
	NTV2WriteRegisterRemote (4937, 0x0000000A);  // Insert 6 Payload ID Control
	NTV2WriteRegisterRemote (4938, 0x0100CA59);  // Insert 6 Payload ID
	NTV2WriteRegisterRemote (4993, 0x11000000);  // Insert 7 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4994, 0x31A00000);  // Insert 7 F1 Start Address
	NTV2WriteRegisterRemote (4995, 0x31E00000);  // Insert 7 F2 Start Address
	NTV2WriteRegisterRemote (4996, 0x00000008);  // Insert 7 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4997, 0x0000002A);  // Insert 7 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4998, 0x08980780);  // Insert 7 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4999, 0x00000465);  // Insert 7 Lines Per Frame
	NTV2WriteRegisterRemote (5001, 0x0000000A);  // Insert 7 Payload ID Control
	NTV2WriteRegisterRemote (5002, 0x0100CA59);  // Insert 7 Payload ID
	NTV2WriteRegisterRemote (5057, 0x11000000);  // Insert 8 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (5058, 0x32200000);  // Insert 8 F1 Start Address
	NTV2WriteRegisterRemote (5059, 0x32600000);  // Insert 8 F2 Start Address
	NTV2WriteRegisterRemote (5060, 0x00000008);  // Insert 8 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (5061, 0x0000002A);  // Insert 8 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (5062, 0x08980780);  // Insert 8 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (5063, 0x00000465);  // Insert 8 Lines Per Frame
	NTV2WriteRegisterRemote (5065, 0x0000000A);  // Insert 8 Payload ID Control
	NTV2WriteRegisterRemote (5066, 0x0100CA59);  // Insert 8 Payload ID
	NTV2WriteRegisterRemote (kVRegDriverVersion, 0x03C50C08);  // Reg 10000
	NTV2WriteRegisterRemote (kVRegSecondaryFormatSelect, 0x00000002);  // Reg 10021
	NTV2WriteRegisterRemote (kVRegAnalogOutputType, 0x00000001);  // Reg 10025
	NTV2WriteRegisterRemote (kVRegGammaMode, 0x00000001);  // Reg 10043
	NTV2WriteRegisterRemote (kVRegLUTType, 0x00000002);  // Reg 10044
	NTV2WriteRegisterRemote (kVRegRGB10Range, 0x00000001);  // Reg 10045
	NTV2WriteRegisterRemote (kVRegRGB10Endian, 0x00000001);  // Reg 10046
	NTV2WriteRegisterRemote (kVRegAudioSyncTolerance, 0x00002710);  // Reg 10079
	NTV2WriteRegisterRemote (kVRegDSKAudioMode, 0x00000001);  // Reg 10125
	NTV2WriteRegisterRemote (kVRegCaptureReferenceSelect, 0x00000002);  // Reg 10128
	NTV2WriteRegisterRemote (kVRegSDIInput2RGBRange, 0x00000002);  // Reg 10135
	NTV2WriteRegisterRemote (kVRegSDIInput1Stereo3DMode, 0x00000001);  // Reg 10136
	NTV2WriteRegisterRemote (kVRegSDIInput2Stereo3DMode, 0x00000001);  // Reg 10137
	NTV2WriteRegisterRemote (kVRegFrameBuffer1RGBRange, 0x00000001);  // Reg 10138
	NTV2WriteRegisterRemote (kVRegFrameBuffer1Stereo3DMode, 0x00000001);  // Reg 10139
	NTV2WriteRegisterRemote (kVRegAnalogInputType, 0x00000001);  // Reg 10143
	NTV2WriteRegisterRemote (kVRegSDIInput2ColorSpaceMode, 0x00000001);  // Reg 10150
	NTV2WriteRegisterRemote (kVRegSDIOutput1Stereo3DMode, 0x00000001);  // Reg 10152
	NTV2WriteRegisterRemote (kVRegSDIOutput2Stereo3DMode, 0x00000001);  // Reg 10153
	NTV2WriteRegisterRemote (kVRegFrameBuffer2RGBRange, 0x00000001);  // Reg 10154
	NTV2WriteRegisterRemote (kVRegFrameBuffer2Stereo3DMode, 0x00000001);  // Reg 10155
	NTV2WriteRegisterRemote (kVRegAudioGainDisable, 0x00000001);  // Reg 10156
	NTV2WriteRegisterRemote (kVRegActiveVideoOutFilter, 0x0000007F);  // Reg 10158
	NTV2WriteRegisterRemote (kVRegDeviceOnline, 0x00000001);  // Reg 10168
	NTV2WriteRegisterRemote (kVRegTimelapseCaptureValue, 0x00000001);  // Reg 10175
	NTV2WriteRegisterRemote (kVRegTimelapseIntervalValue, 0x00000001);  // Reg 10177
	NTV2WriteRegisterRemote (kVRegTimelapseIntervalUnits, 0x00000001);  // Reg 10178
	NTV2WriteRegisterRemote (kVRegAnalogInStandard, 0x00000002);  // Reg 10180
	NTV2WriteRegisterRemote (kVRegAnalogIoSelect, 0x00000002);  // Reg 10193
	NTV2WriteRegisterRemote (kVRegProgressivePicture, 0x00000001);  // Reg 10260
	NTV2WriteRegisterRemote (kVRegLUT2Type, 0x00000002);  // Reg 10261
	NTV2WriteRegisterRemote (kVRegLUT3Type, 0x00000002);  // Reg 10262
	NTV2WriteRegisterRemote (kVRegLUT4Type, 0x00000002);  // Reg 10263
	NTV2WriteRegisterRemote (kVRegRGBRangeConverterLUTType, 0x00000002);  // Reg 10267
	NTV2WriteRegisterRemote (kVRegTestPatternChoice, 0x0000000C);  // Reg 10268
	NTV2WriteRegisterRemote (kVRegEveryFrameTaskFilter, 0x00000001);  // Reg 10270
	NTV2WriteRegisterRemote (kVRegDefaultInput, 0x00000001);  // Reg 10271
	NTV2WriteRegisterRemote (kVRegDefaultVideoOutMode, 0x00000004);  // Reg 10272
	NTV2WriteRegisterRemote (kVRegDefaultVideoFormat, 0x00000065);  // Reg 10273
	NTV2WriteRegisterRemote (kVRegDigitalOutput5Select, 0x0000000B);  // Reg 10274
	NTV2WriteRegisterRemote (kVRegLUT5Type, 0x00000002);  // Reg 10275
	NTV2WriteRegisterRemote (kVRegMacUserModeDebugLevel, 0x00000003);  // Reg 10300
	NTV2WriteRegisterRemote (kVRegMacUserModePingLevel, 0x00000003);  // Reg 10302
	NTV2WriteRegisterRemote (kVRegAudioInputSelect, 0x00000002);  // Reg 10306
	NTV2WriteRegisterRemote (kVRegStartupStatusFlags, 0x00000007);  // Reg 10311
	NTV2WriteRegisterRemote (kVRegRGBRangeMode, 0x00000001);  // Reg 10312
	NTV2WriteRegisterRemote (kVRegAdvancedIndexing, 0x00000001);  // Reg 10340
	NTV2WriteRegisterRemote (kVRegResetCycleCount, 0x00000001);  // Reg 10366
	NTV2WriteRegisterRemote (kVRegUseProgressive, 0x00000001);  // Reg 10367
	NTV2WriteRegisterRemote (kVRegEFTNeedsUpdating, 0x00000001);  // Reg 10373
	NTV2WriteRegisterRemote (kVRegServicesInitialized, 0x00000001);  // Reg 10378
	NTV2WriteRegisterRemote (kVRegChannelCrosspointFirst, 0x00000012);  // Reg 10380
	NTV2WriteRegisterRemote (VIRTUALREG_START+381, 0x00000012);  // Reg 10381
	NTV2WriteRegisterRemote (VIRTUALREG_START+382, 0x00000012);  // Reg 10382
	NTV2WriteRegisterRemote (VIRTUALREG_START+383, 0x00000012);  // Reg 10383
	NTV2WriteRegisterRemote (VIRTUALREG_START+384, 0x00000012);  // Reg 10384
	NTV2WriteRegisterRemote (VIRTUALREG_START+385, 0x00000012);  // Reg 10385
	NTV2WriteRegisterRemote (VIRTUALREG_START+386, 0x00000012);  // Reg 10386
	NTV2WriteRegisterRemote (kVRegChannelCrosspointLast, 0x00000012);  // Reg 10387
	NTV2WriteRegisterRemote (kVRegMonAncField1Offset, 0x00004000);  // Reg 10389
	NTV2WriteRegisterRemote (kVRegMonAncField2Offset, 0x00002000);  // Reg 10390
	NTV2WriteRegisterRemote (kVRegAncField1Offset, 0x00004000);  // Reg 10392
	NTV2WriteRegisterRemote (kVRegAncField2Offset, 0x00002000);  // Reg 10393
	NTV2WriteRegisterRemote (kVRegAgentCheck, 0x00000007);  // Reg 10394
	NTV2WriteRegisterRemote (kVReg4kOutputTransportSelection, 0x00000002);  // Reg 10396
	NTV2WriteRegisterRemote (kVRegVideoFormatCh1, 0x00000065);  // Reg 10401
	NTV2WriteRegisterRemote (kVRegVideoFormatCh2, 0x00000065);  // Reg 10402
	NTV2WriteRegisterRemote (kVRegVideoFormatCh3, 0x00000065);  // Reg 10403
	NTV2WriteRegisterRemote (kVRegVideoFormatCh4, 0x00000065);  // Reg 10404
	NTV2WriteRegisterRemote (kVRegVideoFormatCh5, 0x00000065);  // Reg 10405
	NTV2WriteRegisterRemote (kVRegVideoFormatCh6, 0x00000065);  // Reg 10406
	NTV2WriteRegisterRemote (kVRegVideoFormatCh7, 0x00000065);  // Reg 10407
	NTV2WriteRegisterRemote (kVRegVideoFormatCh8, 0x00000065);  // Reg 10408
	NTV2WriteRegisterRemote (kVRegMailBoxAcquire, 0x00000001);  // Reg 10459
	NTV2WriteRegisterRemote (kVRegMailBoxRelease, 0x00000001);  // Reg 10460
	NTV2WriteRegisterRemote (kVRegMailBoxAbort, 0x00000001);  // Reg 10461
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceMainEnable, 0x00000001);  // Reg 10509
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceAux1Enable, 0x00000001);  // Reg 10510
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceMainGain, 0x00010000);  // Reg 10512
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceAux1Gain, 0x00010000);  // Reg 10513
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceAux2Gain, 0x00010000);  // Reg 10514
	NTV2WriteRegisterRemote (kVRegAudioCapMixerSourceMainEnable, 0x00000001);  // Reg 10515
	NTV2WriteRegisterRemote (kVRegAudioCapMixerSourceMainGain, 0x00010000);  // Reg 10518
	NTV2WriteRegisterRemote (kVRegAudioCapMixerSourceAux1Gain, 0x00010000);  // Reg 10519
	NTV2WriteRegisterRemote (kVRegAudioCapMixerSourceAux2Gain, 0x00010000);  // Reg 10520
	NTV2WriteRegisterRemote (kVRegSwizzle4kInput, 0x00000001);  // Reg 10521
	NTV2WriteRegisterRemote (kVRegHdrColorimetryCh1, 0x0000FFFF);  // Reg 10557
	NTV2WriteRegisterRemote (kVRegHdrTransferCh1, 0x0000FFFF);  // Reg 10558
	NTV2WriteRegisterRemote (kVRegHdrLuminanceCh1, 0x0000FFFF);  // Reg 10559
	NTV2WriteRegisterRemote (kVRegHdrGreenXCh1, 0x0000FFFF);  // Reg 10560
	NTV2WriteRegisterRemote (kVRegHdrGreenYCh1, 0x0000FFFF);  // Reg 10561
	NTV2WriteRegisterRemote (kVRegHdrBlueXCh1, 0x0000FFFF);  // Reg 10562
	NTV2WriteRegisterRemote (kVRegHdrBlueYCh1, 0x0000FFFF);  // Reg 10563
	NTV2WriteRegisterRemote (kVRegHdrRedXCh1, 0x0000FFFF);  // Reg 10564
	NTV2WriteRegisterRemote (kVRegHdrRedYCh1, 0x0000FFFF);  // Reg 10565
	NTV2WriteRegisterRemote (kVRegHdrWhiteXCh1, 0x0000FFFF);  // Reg 10566
	NTV2WriteRegisterRemote (kVRegHdrWhiteYCh1, 0x0000FFFF);  // Reg 10567
	NTV2WriteRegisterRemote (kVRegHdrMasterLumMaxCh1, 0x0000FFFF);  // Reg 10568
	NTV2WriteRegisterRemote (kVRegHdrMasterLumMinCh1, 0x0000FFFF);  // Reg 10569
	NTV2WriteRegisterRemote (kVRegHdrMaxCLLCh1, 0x0000FFFF);  // Reg 10570
	NTV2WriteRegisterRemote (kVRegHdrMaxFALLCh1, 0x0000FFFF);  // Reg 10571
}	//	InitRegs


bool NTV2SoftwareDevice::InitRegsFromSupportLog (const string & inLogFilePath)
{
	ifstream fLog;
	fLog.open(inLogFilePath.c_str());
	if (!fLog)
		{NBFAIL("Unable to open '" << inLogFilePath << "'");  return false;}

	string line;
	uint32_t regNum(0), regVal(0), failures(0), successes(0);

	while (!fLog.bad()  &&  !fLog.eof())
	{
		getline(fLog, line);
		if (line.find ("Register Number: ", 0) != 0)
			continue;	//	Keep looking

		//	Line starts with "Register Number: "
		line.erase(0, 17);		//	Now the line should only contain a decimal reg number e.g. "4024"
		stringstream regNumStr(line);
		regNumStr >> regNum;

		getline(fLog, line);	//	Next line should start with "Register Value: "...
		if (line.find ("Register Value: ") != 0)
			{NBWARN("Found 'Register Number: " << regNum << "' -- but 'Register Value: ' missing");  continue;}

		//	Line starts with "Register Value: "
		size_t hexPos(line.find("0x"));
		line.erase(0, hexPos+2);	//	Remove everything up to the hex value
		stringstream regValStr(line);
		regValStr >> hex >> regVal;
		//NBDBG("Writing reg " << DEC(regNum) << " " << xHEX0N(regVal,8));
		if (NTV2WriteRegisterRemote(regNum, regVal))
			successes++;
		else
			failures++;
	}	//	loop til EOF
	NBINFO(DEC(successes) << " register(s) successfully written, " << DEC(failures) << " failed, for support log '" << inLogFilePath << "'");
	return successes > 0;
}


uint32_t NTV2SoftwareDevice::GetSDRAMDumpFileSize (const string & inFilePath)
{
	streampos result(0);
	ifstream ifile(inFilePath.c_str(), std::ifstream::in | std::ifstream::binary | ios::ate);	//	Open read-only binary, positioned "ate" (at end)
	if (ifile)
		return uint32_t(ifile.tellg());
	return 0;
}


bool NTV2SoftwareDevice::InitSDRAMFromFile (const string & inFilePath)
{
	streampos dumpFileSizeInBytes(0), bytesToRead(0);
	ifstream ifile(inFilePath.c_str(), std::ifstream::in | std::ifstream::binary | ios::ate);	//	Open read-only binary, positioned "ate" (at end)
	if (!ifile)
		{NBFAIL("Unable to open SDRAM dump file '" << inFilePath << "' for reading");  return false;}
	dumpFileSizeInBytes = ifile.tellg();
	ifile.seekg(0, ios::beg);	//	Rewind to start
	if (dumpFileSizeInBytes > streampos(mFBMemory.GetByteCount()))
	{	NBWARN("SDRAM file size " << DEC(dumpFileSizeInBytes) << " (" << DEC(dumpFileSizeInBytes/1024/1024)
				<< "MB) is larger than device SDRAM size " << DEC(mFBMemory.GetByteCount()) << " (" << DEC(mFBMemory.GetByteCount()/1024/1024) << "MB)");
		bytesToRead = streampos(mFBMemory.GetByteCount());
	}
	else if (dumpFileSizeInBytes < streampos(mFBMemory.GetByteCount()))
	{	NBWARN("SDRAM file size " << DEC(dumpFileSizeInBytes) << " (" << DEC(dumpFileSizeInBytes/1024/1024)
				<< "MB) is smaller than device SDRAM size " << DEC(mFBMemory.GetByteCount()) << " (" << DEC(mFBMemory.GetByteCount()/1024/1024) << "MB)");
		bytesToRead = dumpFileSizeInBytes;
	}
	else
		bytesToRead = dumpFileSizeInBytes;

	if (!ifile.read(mFBMemory, streamsize(bytesToRead)).good())
		{NBFAIL("Failed reading " << DEC(bytesToRead) << "  bytes");  return false;}

	NBINFO(DEC(bytesToRead) << " bytes successfully read into device SDRAM from '" << inFilePath << "'");
	return true;
}
