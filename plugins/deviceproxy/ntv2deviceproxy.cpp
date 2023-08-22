/**
	@file		ntv2deviceproxy.cpp
	@brief		A proxy device implementation.
	@copyright	(C) 2022-2023 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#include "ntv2card.h"
#include "ntv2devicescanner.h"
#include "ntv2nubaccess.h"
#include "ntv2publicinterface.h"
#include "ntv2utils.h"
#include "ntv2version.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include <fstream>
#include <iomanip>
#if defined(AJAMac)
	#include <CoreFoundation/CoreFoundation.h>
	#include <dlfcn.h>
#elif defined(AJALinux)
	#include <dlfcn.h>
#endif

using namespace std;

#define INSTP(_p_)			xHEX0N(uint64_t(_p_),16)
#define	NBFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

#define	AsNTV2GetRegisters(_p_)			(reinterpret_cast<NTV2GetRegisters*>(_p_))

#if defined(MSWindows)
	#define EXPORT __declspec(dllexport)	
#else
	#define EXPORT	
#endif

/*****************************************************************************************************************************************************
	NTV2DeviceProxy

	A proxy to another device.

	CONFIGURATION PARAMETERS
		Parameter Name			Required?	Default Value		Description
		------------------		----------	-------------------	---------------------------------------------------------------------------
		devspec=spec			No			"0"					Device specifier that identifies the underlying device to connect to.
		devid=spec				No			N/A					Specifies the device ID to behave as.

	EXAMPLE USAGE:
		To use this "device" in the NTV2Player demo on MacOS as a proxy to a real NTV2Card instance for the first Corvid88 board found on
		the host, but have it behave like a Corvid44:
			./bin/ntv2player  --device 'ntv2deviceproxy://localhost/?devspec=corvid88&devid=corvid44'
		...or in C++:
			CNTV2Card device;
			device.Open("ntv2deviceproxy://localhost/?devspec=corvid88&devid=corvid44");

		After URL-decoding, and viewed as key/value pairs, here's the software device config params:
			PARM				VALUE
			devspec				corvid88
			devid				corvid44
*****************************************************************************************************************************************************/

class NTV2DeviceProxy : public NTV2RPCAPI
{
	//	Instance Methods
	public:
									NTV2DeviceProxy (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inCallingVersion);
		virtual						~NTV2DeviceProxy ();
		virtual string				Name						(void) const;
		virtual string				Description					(void) const;
		virtual inline bool			IsConnected					(void) const	{return mCard.IsOpen();}
		virtual bool				NTV2Connect					(void);
		virtual	bool				NTV2Disconnect				(void);
		virtual bool				NTV2GetBoolParamRemote		(const ULWord inParamID,  ULWord & outValue);
		virtual bool				NTV2GetNumericParamRemote	(const ULWord inParamID,  ULWord & outValue);
		virtual bool				NTV2GetSupportedRemote		(const ULWord inEnumsID, ULWordSet & outSupported);
		virtual	bool				NTV2ReadRegisterRemote		(const ULWord regNum, ULWord & outRegValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		virtual	bool				NTV2WriteRegisterRemote		(const ULWord regNum, const ULWord regValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		virtual	bool				NTV2AutoCirculateRemote		(AUTOCIRCULATE_DATA & autoCircData);
		virtual	bool				NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs);
		virtual	bool				NTV2DMATransferRemote		(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
																const ULWord inFrameNumber,			NTV2Buffer & inOutBuffer,
																const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
																const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,
																const bool inSynchronous);
		virtual	bool				NTV2MessageRemote			(NTV2_HEADER * pInMessage);
		virtual inline string		getParam					(const string & inKey)			{return mConnectParams.valueForKey(inKey);}
		virtual inline bool			hasParam					(const string & inKey) const	{return mConnectParams.hasKey(inKey);}
		virtual inline NTV2DeviceID	simulatedDeviceID			(void) const					{return mSimDeviceID;}
		virtual inline bool			isSimulatedDevice			(void) const					{return simulatedDeviceID() != DEVICE_ID_INVALID;}

	//	Protected & Private Instance Methods
	protected:
		virtual	bool			NTV2OpenRemote				(void);
		virtual	inline bool		NTV2CloseRemote				(void)							{return true;}
		virtual bool			GetKnownDevices				(void);

		typedef std::map<std::string, NTV2DeviceID>	NameToIDs;

	//	Instance Data
	private:
		uint64_t			mDLLHandle;			///< @brief	DLL handle
		const uint32_t		mHostSDKVersion;	///< @brief	Host/caller SDK version
		const uint32_t		mSDKVersion;		///< @brief	My SDK version
		NTV2DeviceID		mSimDeviceID;		///< @brief	Simulated device ID (DEVICE_ID_INVALID uses real device ID)
		mutable CNTV2Card	mCard;				///< @brief	My CNTV2Card object
		NameToIDs			mKnownDevs;			///< @brief	Known device name to device ID map
};	//	NTV2DeviceProxy

extern "C"
{
	EXPORT NTV2RPCClientAPI * CreateClient (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inCallerSDKVers);
}

bool NTV2DeviceProxy::GetKnownDevices (void)
{
	const NTV2DeviceIDSet allDevIDs(::NTV2GetSupportedDevices());
	for (NTV2DeviceIDSetConstIter it(allDevIDs.begin());  it != allDevIDs.end();  ++it)
	{
		const NTV2DeviceID devID(*it);
		const ULWord id(*it);
		string str (::NTV2DeviceIDToString(devID));
		mKnownDevs[aja::lower(str)] = devID;
		ostringstream ossHex;		ossHex << Hex(id);			mKnownDevs[ossHex.str()] = devID;
		ostringstream ossxHex;		ossxHex << xHex(id);		mKnownDevs[ossxHex.str()] = devID;
		ostringstream ossHexN;		ossHexN << HexN(id,8);		mKnownDevs[ossHexN.str()] = devID;
		ostringstream ossxHexN;		ossxHexN << xHexN(id,8);	mKnownDevs[ossxHexN.str()] = devID;
		ostringstream ossHex0N;		ossHex0N << Hex0N(id,8);	mKnownDevs[ossHex0N.str()] = devID;
		ostringstream ossxHex0N;	ossxHex0N << xHex0N(id,8);	mKnownDevs[ossxHex0N.str()] = devID;
		ostringstream ossHEX;		ossHEX << HEX(id);			mKnownDevs[ossHEX.str()] = devID;
		ostringstream ossxHEX;		ossxHEX << xHEX(id);		mKnownDevs[ossxHEX.str()] = devID;
		ostringstream ossHEXN;		ossHEXN << HEXN(id,8);		mKnownDevs[ossHEXN.str()] = devID;
		ostringstream ossxHEXN;		ossxHEXN << xHEXN(id,8);	mKnownDevs[ossxHEXN.str()] = devID;
		ostringstream ossHEX0N;		ossHEX0N << HEX0N(id,8);	mKnownDevs[ossHEX0N.str()] = devID;
		ostringstream ossxHEX0N;	ossxHEX0N << xHEX0N(id,8);	mKnownDevs[ossxHEX0N.str()] = devID;
	}
	return true;
}

NTV2RPCClientAPI * CreateClient (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inCallerSDKVers)
{
	AJADebug::Open();
    NTV2DeviceProxy * pResult(new NTV2DeviceProxy (pInDLLHandle, inParams, inCallerSDKVers));
	if (!pResult->NTV2Connect())
	{	AJA_sERROR(AJA_DebugUnit_RPCClient, AJAFUNC << ": NTV2Connect failed");
		delete pResult;
		return AJA_NULL;
	}	//	Failed
	AJA_sDEBUG(AJA_DebugUnit_RPCClient, AJAFUNC << ": returning " << xHEX0N(uint64_t(pResult),16));
	return pResult;
}


NTV2DeviceProxy::NTV2DeviceProxy (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inCallingVersion)
	:	NTV2RPCAPI		(inParams),
		mDLLHandle		(uint64_t(pInDLLHandle)),
		mHostSDKVersion	(inCallingVersion),
		mSDKVersion		(AJA_NTV2_SDK_VERSION),
		mSimDeviceID	(DEVICE_ID_INVALID)
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

NTV2DeviceProxy::~NTV2DeviceProxy ()
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

bool NTV2DeviceProxy::NTV2Connect (void)
{
	//	Version check...
	if (mSDKVersion  &&  mHostSDKVersion  &&  mSDKVersion != mHostSDKVersion)
		NBWARN(Name() << " SDK version " << xHEX0N(mSDKVersion,8) << " doesn't match host SDK version " << xHEX0N(mHostSDKVersion,8));

	//	Check config params:
	string devSpec;
	const NTV2StringSet keys(mConnectParams.keys());
	NTV2StringList skippedParams;
	for (NTV2StringSetConstIter it(keys.begin());  it != keys.end();  ++it)
	{
		const string key(*it); string value(mConnectParams.valueForKey(key));
		if (key == "devspec")
		{
			if (value.empty())
				{NBWARN(Name() << " 'DevSpec' parameter value missing or empty");  continue;}
			if (!devSpec.empty())
				{NBFAIL(Name() << " 'DevSpec' parameter specified more than once, was '" << devSpec << "', now '" << value << "'"); return false;}
			devSpec = value;
			NBINFO(Name() << " 'DevSpec' parameter value '" << devSpec << "' specified");
		}
		else if (key == "devid")
		{
			if (value.empty())
				{NBWARN(Name() << " 'DevID' parameter value missing or empty");  continue;}
			if (!devSpec.empty())
				{NBFAIL(Name() << " 'DevID' parameter specified more than once, was '" << devSpec << "', now '" << value << "'"); return false;}
			GetKnownDevices();
			string valueLower(value);
			aja::lower(valueLower);
			if (mKnownDevs.find(value) != mKnownDevs.end())
				mSimDeviceID = mKnownDevs[value];
			if (!isSimulatedDevice()  &&  mKnownDevs.find(valueLower) != mKnownDevs.end())
				mSimDeviceID = mKnownDevs[valueLower];
			if (!isSimulatedDevice())
				{NBFAIL("'" << Name() << "' parameter '" << value << "' invalid, no such device or ID"); return false;}
			NBINFO("'" << Name() << "' parameter '" << value << "' specified");
		}
		else if (key == "help")
		{
			ostringstream oss;
			oss << "ntv2deviceproxy:  This plugin is a proxy to another NTV2 device." << endl
				<< "CONFIG PARAMS:" << endl
				<< "Name            Reqd    Default     Desc" << endl
				<< "devspec=spec    No      '0'         'spec' identifies the underlying device to connect to." << endl
				<< "devid=id        No      N/A         'id' overrides the NTV2DeviceID of the underlying device.";
			NBINFO(oss.str());
			cerr << oss.str() << endl;
			return false;
		}
		else
			skippedParams.push_back(key);
	}	//	for each connectParams key

	if (!skippedParams.empty())
		NBWARN("Skipped unrecognized parameter(s): " << skippedParams);

	//	Open the devSpec...
	if (devSpec.empty())
		devSpec = "0";
	if (CNTV2DeviceScanner::GetFirstDeviceFromArgument(devSpec, mCard))
		NBINFO(Description() << " ready");
	return mCard.IsOpen();
}

bool NTV2DeviceProxy::NTV2Disconnect (void)
{
	NBINFO("");
	return true;
}

string NTV2DeviceProxy::Name (void) const
{
	ostringstream oss;
	if (isSimulatedDevice())
		oss << "'" << ::NTV2DeviceIDToString(simulatedDeviceID()) << "' proxy to '" << mCard.GetDisplayName() << "'";
	else
		oss << "Proxy to '" << mCard.GetDisplayName() << "'";
	return oss.str();
}

string NTV2DeviceProxy::Description (void) const
{
	ostringstream oss;
	oss << Name();
	if (mCard.GetSerialNumber())
		if (!::SerialNum64ToString(mCard.GetSerialNumber()).empty())
			oss << " serial '" << ::SerialNum64ToString(mCard.GetSerialNumber()) << "'";
	return oss.str();
}

bool NTV2DeviceProxy::NTV2OpenRemote (void)
{
	return true;
}

bool NTV2DeviceProxy::NTV2GetBoolParamRemote (const ULWord inParamID,  ULWord & outValue)
{
	return false;//mCard.GetBoolParam(inParamID, outValue);
}

bool NTV2DeviceProxy::NTV2GetNumericParamRemote (const ULWord inParamID,  ULWord & outValue)
{
	return false;//mCard.GetNumericParam(inParamID, outValue);
}

bool NTV2DeviceProxy::NTV2GetSupportedRemote (const ULWord inEnumsID, ULWordSet & outSupported)
{
	return false;//return mCard.features().GetSupported(inEnumsID, outSupported);
}

bool NTV2DeviceProxy::NTV2ReadRegisterRemote (const ULWord inRegNum, ULWord & outRegValue, const ULWord inRegMask, const ULWord inRegShift)
{
	bool ok (mCard.ReadRegister(inRegNum, outRegValue, inRegMask, inRegShift));
	if (ok  &&  inRegNum == kRegBoardID  &&  mSimDeviceID != DEVICE_ID_INVALID)
	{
		outRegValue = mSimDeviceID & inRegMask;
		if (inRegShift)
			outRegValue >>= inRegShift;
	}
	return ok;
}

bool NTV2DeviceProxy::NTV2WriteRegisterRemote (const ULWord inRegNum, const ULWord inRegVal, const ULWord inRegMask, const ULWord inRegShift)
{
	return mCard.WriteRegister(inRegNum, inRegVal, inRegMask, inRegShift);
}


bool NTV2DeviceProxy::NTV2AutoCirculateRemote (AUTOCIRCULATE_DATA & autoCircData)
{
	return mCard.AutoCirculate(autoCircData);
}

bool NTV2DeviceProxy::NTV2WaitForInterruptRemote (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs)
{
	return mCard.WaitForInterrupt(eInterrupt, timeOutMs);
}

bool NTV2DeviceProxy::NTV2DMATransferRemote (const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
												const ULWord inFrameNumber,			NTV2Buffer & inOutBuffer,
												const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
												const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,	const bool inSynchronous)
{
	return mCard.DmaTransfer(inDMAEngine, inIsRead, inFrameNumber, inOutBuffer, inCardOffsetBytes, inNumSegments, inSegmentHostPitch, inSegmentCardPitch, inSynchronous);
}


bool NTV2DeviceProxy::NTV2MessageRemote (NTV2_HEADER * pInMessage)
{
	bool ok(mCard.NTV2Message(pInMessage));
	if (ok  &&  mSimDeviceID != DEVICE_ID_INVALID  &&  pInMessage->GetType() == NTV2_TYPE_GETREGS)
	{
		NTV2GetRegisters * pGetRegs = AsNTV2GetRegisters(pInMessage);
		if (pGetRegs)
			pGetRegs->PatchRegister (kRegBoardID, mSimDeviceID);
	}
	return ok;
}
