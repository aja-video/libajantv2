/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2firmwareinstallerthread.cpp
	@brief		Implementation of CNTV2FirmwareInstallerThread class.
	@copyright	(C) 2014-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2firmwareinstallerthread.h"
#include "ntv2bitfile.h"
#include "ntv2utils.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/systemtime.h"
#include "ntv2konaflashprogram.h"

using namespace std;


#if defined (AJADebug) || defined (_DEBUG) || defined (DEBUG)
	static const bool	gDebugging	(true);
#else
	static const bool	gDebugging	(false);
#endif


static const bool		SIMULATE_UPDATE			(false);	//	Set this to true to simulate flashing a device
static const bool		SIMULATE_FAILURE		(false);	//	Set this to true to simulate a flash failure
static const uint32_t	kMilliSecondsPerSecond	(1000);

#define FITDBUG(__x__)	do {ostringstream oss;  oss << __x__;  cerr << "## DEBUG:    " << oss.str() << endl;  AJA_sDEBUG  (AJA_DebugUnit_Firmware, oss.str());} while(false)
#define FITWARN(__x__)	do {ostringstream oss;  oss << __x__;  cerr << "## WARNING:  " << oss.str() << endl;  AJA_sWARNING(AJA_DebugUnit_Firmware, oss.str());} while(false)
#define FITERR(__x__)	do {ostringstream oss;  oss << __x__;  cerr << "## ERROR:    " << oss.str() << endl;  AJA_sERROR  (AJA_DebugUnit_Firmware, oss.str());} while(false)
#define FITNOTE(__x__)	do {ostringstream oss;  oss << __x__;  cerr << "## NOTE:  "    << oss.str() << endl;  AJA_sNOTICE (AJA_DebugUnit_Firmware, oss.str());} while(false)


static string GetFirmwarePath (const NTV2DeviceID inDeviceID)
{
	const string	bitfileName		(::NTV2GetBitfileName (inDeviceID));
	const string	firmwareFolder	(::NTV2GetFirmwareFolderPath ());
	string			resultPath;

	#if defined (AJAMac)
		resultPath = firmwareFolder + "/" + bitfileName;	//	Unified Mac driver -- bitfiles in 'firmwareFolder'
	#elif defined (MSWindows)
		resultPath = firmwareFolder + "\\" + bitfileName;
	#elif defined (AJALinux)
		resultPath = firmwareFolder + "/" + bitfileName;	//	Linux platform-specific location of latest bitfile
	#endif

	return resultPath;
}


int NeedsFirmwareUpdate (const NTV2DeviceInfo & inDeviceInfo, string & outReason)
{
	string			installedDate, installedTime, serialNumStr, newFirmwareDescription;
	ULWord			numBytes		(0);
	const string	firmwarePath	(::GetFirmwarePath(inDeviceInfo.deviceID));
	CNTV2Card		device			(UWord(inDeviceInfo.deviceIndex));
	CNTV2Bitfile	bitfile;
	CNTV2MCSfile	mcsFile;

	outReason.clear();
	if (!device.IsOpen())
		{outReason = "device '" + inDeviceInfo.deviceIdentifier + "' not open";		return kFirmwareUpdateCheckFailed;}
	if (!device.IsDeviceReady(false))
		{outReason = "device '" + inDeviceInfo.deviceIdentifier + "' not ready";	return kFirmwareUpdateCheckFailed;}
	if (device.IsRemote())
		{outReason = "device '" + inDeviceInfo.deviceIdentifier + "' not local physical";	return kFirmwareUpdateCheckFailed;}
	if (firmwarePath.find(".mcs") != std::string::npos)
	{
		//	We have an mcs file?
		CNTV2KonaFlashProgram kfp (device.GetIndexNumber());
		kfp.GetMCSInfo();
		if (!mcsFile.GetMCSHeaderInfo(firmwarePath))
			{outReason = "MCS File open failed";	return kFirmwareUpdateCheckFailed;}

		string fileDate = mcsFile.GetMCSPackageDateString();
		string fileVersion = mcsFile.GetMCSPackageVersionString();
		PACKAGE_INFO_STRUCT currentInfo;
		device.GetPackageInformation(currentInfo);
		if (fileDate == currentInfo.date)
			return 0;	//	All good
		if (currentInfo.date > fileDate)
		{
			outReason = "on-device firmware " + installedDate + " newer than on-disk bitfile firmware " + bitfile.GetDate ();
			return 1;	//	on-device firmware newer than on-disk bitfile firmware
		}
		outReason = "on-device firmware " + installedDate + " older than on-disk bitfile firmware " + bitfile.GetDate ();
		return -1;	//	on-device firmware older than on-disk bitfile firmware
	}

	if (device.GetInstalledBitfileInfo (numBytes, installedDate, installedTime))
	{
		if (bitfile.Open (firmwarePath))
		{
			//	If we can dynamically reconfig return true
			if(device.IsDynamicDevice())
			{
				device.AddDynamicDirectory((::NTV2GetFirmwareFolderPath()));
#ifdef AJA_WINDOWS
				NTV2DeviceID desiredID (bitfile.GetDeviceID());
				if (device.CanLoadDynamicDevice(desiredID))
					return false;
#endif
			}

			//cout << inDeviceInfo.deviceIdentifier << ":  file: " << bitfile.GetDate() << "  device: " << installedDate << endl;
			if (bitfile.GetDate() == installedDate)
				return 0;	//	Identical!
			if (installedDate > bitfile.GetDate())
			{
				outReason = "on-device firmware " + installedDate + " newer than on-disk bitfile firmware " + bitfile.GetDate();
				return 1;	//	on-device firmware newer than on-disk bitfile firmware
			}
			outReason = "on-device firmware " + installedDate + " older than on-disk bitfile firmware " + bitfile.GetDate();
			return -1;	//	on-device firmware older than on-disk bitfile firmware
		}
		else
			outReason = bitfile.GetLastError();
	}
	else
		outReason = "GetInstalledBitfileInfo failed for device '" + inDeviceInfo.deviceIdentifier + "'";
	return kFirmwareUpdateCheckFailed;	//	failure
}


int NeedsFirmwareUpdate (const NTV2DeviceInfo & inDeviceInfo)
{
	string	notUsed;
	return NeedsFirmwareUpdate (inDeviceInfo, notUsed);
}



CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread (const NTV2DeviceInfo & inDeviceInfo,
															const string & inBitfilePath,
															const bool inVerbose,
															const bool inForceUpdate)
	:	m_deviceInfo		(inDeviceInfo),
		m_bitfilePath		(inBitfilePath),
		m_updateSuccessful	(false),
		m_verbose			(inVerbose),
		m_forceUpdate		(inForceUpdate),
		m_useDynamicReconfig (false)
{
	::memset (&m_statusStruct, 0, sizeof (m_statusStruct));
}

CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread (const NTV2DeviceInfo & inDeviceInfo,
															const string & inDRFilesPath,
															const NTV2DeviceID inDesiredID,
															const bool inVerbose)
	:	m_deviceInfo		(inDeviceInfo),
		m_desiredID			(inDesiredID),
		m_drFilesPath		(inDRFilesPath),
		m_updateSuccessful	(false),
		m_verbose			(inVerbose),
		m_forceUpdate		(false),
		m_useDynamicReconfig (true)
{
	::memset (&m_statusStruct, 0, sizeof (m_statusStruct));
}


AJAStatus CNTV2FirmwareInstallerThread::ThreadRun (void)
{
	ostringstream ossNote, ossWarn, ossErr;
	m_device.Open(UWord(m_deviceInfo.deviceIndex));
	if (!m_device.IsOpen())
	{
		FITERR("CNTV2FirmwareInstallerThread:  Device '" << DEC(m_deviceInfo.deviceIndex) << "' not open");
		return AJA_STATUS_OPEN;
	}
	if (m_bitfilePath.empty() && !m_useDynamicReconfig)
	{
		FITERR("CNTV2FirmwareInstallerThread:  Empty bitfile path!");
		return AJA_STATUS_BAD_PARAM;
	}

	m_device.WriteRegister(kVRegFlashStatus, 0);

	//	Preflight bitfile...
	ULWord	numBytes	(0);
	string	installedDate, installedTime, serialNumStr, newFirmwareDescription;
	if (!m_device.GetInstalledBitfileInfo (numBytes, installedDate, installedTime))
		FITWARN("CNTV2FirmwareInstallerThread:  Unable to obtain installed bitfile info");
	m_device.GetSerialNumberString(serialNumStr);

	if (m_bitfilePath.find(".mcs") != string::npos)
	{
		CNTV2KonaFlashProgram kfp;
		if (!m_verbose)
			kfp.SetQuietMode();

		m_device.WriteRegister(kVRegFlashState,kProgramStateCalculating);
		m_device.WriteRegister(kVRegFlashSize,MCS_STEPS);
		m_device.WriteRegister(kVRegFlashStatus,0);

		bool rv = kfp.SetBoard(m_device.GetIndexNumber());
		if (!rv)
		{
			FITERR("CNTV2KonaFlashProgram::SetBoard(" << DEC(m_device.GetIndexNumber()) << ") failed");
			m_updateSuccessful = false;
			return AJA_STATUS_FAIL;
		}

		CNTV2MCSfile mcsFile;
		mcsFile.GetMCSHeaderInfo(m_bitfilePath);
		if (!m_forceUpdate  &&  !ShouldUpdateIPDevice(m_deviceInfo.deviceID, mcsFile.GetBitfileDesignString()))
		{
			FITERR("CNTV2FirmwareInstallerThread:  Invalid MCS update");
			m_updateSuccessful = false;
			return AJA_STATUS_BAD_PARAM;
		}

		m_device.WriteRegister(kVRegFlashStatus, ULWord(kfp.NextMcsStep()));
		rv = kfp.SetMCSFile(m_bitfilePath.c_str());
		if (!rv)
		{
			FITERR("CNTV2FirmwareInstallerThread:  SetMCSFile failed");
			m_updateSuccessful = false;
			return AJA_STATUS_FAIL;
		}

		if (m_forceUpdate)
			kfp.SetMBReset();
		m_updateSuccessful = kfp.ProgramFromMCS(true);
		if (!m_updateSuccessful)
		{
			FITERR("CNTV2FirmwareInstallerThread:  ProgramFromMCS failed");
			return AJA_STATUS_FAIL;
		}

		m_device.WriteRegister(kVRegFlashState,kProgramStateFinished);
		m_device.WriteRegister(kVRegFlashSize,MCS_STEPS);
		m_device.WriteRegister(kVRegFlashStatus,MCS_STEPS);

		FITNOTE("CNTV2FirmwareInstallerThread:  MCS update succeeded");
		return AJA_STATUS_SUCCESS;
	}	//	if MCS

	if (m_useDynamicReconfig)
	{
		m_device.AddDynamicDirectory(::NTV2GetFirmwareFolderPath());
		if (!m_device.CanLoadDynamicDevice(m_desiredID))
		{
			FITERR("CNTV2FirmwareInstallerThread: '" << m_desiredID << "' is not compatible with device '"
					<< m_deviceInfo.deviceIdentifier << "'");
			return AJA_STATUS_FAIL;
		}
		if (m_verbose)
			FITNOTE("CNTV2FirmwareInstallerThread:  Dynamic Reconfig started" << endl
					<< "     device: " << m_deviceInfo.deviceIdentifier << ", S/N " << serialNumStr << endl
					<< "  new devID: " << xHEX0N(m_desiredID,8));
	}
	else	//	NOT DYNAMIC RECONFIG
	{
		//	Open bitfile & parse its header...
		CNTV2Bitfile bitfile;
		if (!bitfile.Open(m_bitfilePath))
		{
			const string	extraInfo	(bitfile.GetLastError());
			FITERR("CNTV2FirmwareInstallerThread:  Bitfile '" << m_bitfilePath << "' open/parse error");
			if (!extraInfo.empty())
				cerr << extraInfo << endl;
			return AJA_STATUS_OPEN;
		}

		//	Sanity-check bitfile length...
		const size_t	bitfileLength	(bitfile.GetFileStreamLength());
		NTV2_POINTER	bitfileBuffer(bitfileLength + 512);
		if (!bitfileBuffer)
		{
			FITERR("CNTV2FirmwareInstallerThread:  Unable to allocate " << DEC(bitfileLength+512) << "-byte bitfile buffer");
			return AJA_STATUS_MEMORY;
		}

		bitfileBuffer.Fill(0xFFFFFFFF);
		const size_t	readBytes	(bitfile.GetFileByteStream(bitfileBuffer));
		const string	designName	(bitfile.GetDesignName());
		newFirmwareDescription = m_bitfilePath + " - " + bitfile.GetDate() + " " + bitfile.GetTime();
		if (readBytes != bitfileLength)
		{
			const string err(bitfile.GetLastError());
			FITERR("CNTV2FirmwareInstallerThread:  Invalid bitfile length, read " << DEC(readBytes)
					<< " bytes, expected " << DEC(bitfileLength));
			if (!err.empty())
				cerr << err << endl;
			return AJA_STATUS_FAIL;
		}

		//	Verify that this bitfile is compatible with this device...
		if (!m_forceUpdate  &&  !bitfile.CanFlashDevice(m_deviceInfo.deviceID))
		{
			FITERR("CNTV2FirmwareInstallerThread:  Bitfile design '" << designName << "' is not compatible with device '"
					<< m_deviceInfo.deviceIdentifier << "'");
			return AJA_STATUS_FAIL;
		}

		//	Update firmware...
		if (m_verbose)
			FITNOTE("CNTV2FirmwareInstallerThread:  Firmware update started" << endl
					<< "    bitfile: " << m_bitfilePath << endl
					<< "     device: " << m_deviceInfo.deviceIdentifier << ", S/N " << serialNumStr << endl
					<< "   firmware: " << newFirmwareDescription);
	}	//	not dynamic reconfig

	if (!SIMULATE_UPDATE)
	{
		if (m_useDynamicReconfig)
		{
			m_updateSuccessful = m_device.LoadDynamicDevice(m_desiredID);
			if (!m_updateSuccessful)
				FITERR("CNTV2FirmwareInstallerThread:  'Dynamic Reconfig' failed, desired deviceID: " << xHEX0N(m_desiredID,8));
		}
		else
		{
			//	ProgramMainFlash used to be able to throw (because XilinxBitfile could throw), but with 12.1 SDK, this is no longer the case.
			m_updateSuccessful = m_device.ProgramMainFlash (m_bitfilePath.c_str(), m_forceUpdate, !m_verbose);
			if (!m_updateSuccessful)
				FITNOTE("CNTV2FirmwareInstallerThread:  'ProgramMainFlash' failed" << endl
						<< "	 bitfile: " << m_bitfilePath << endl
						<< "	  device: " << m_deviceInfo.deviceIdentifier << ", S/N " << serialNumStr << endl
						<< "   serialNum: " << serialNumStr << endl
						<< "	firmware: " << newFirmwareDescription);
		}
	}	//	if real update
	else
	{	//	SIMULATE_UPDATE FOR TESTING
		m_statusStruct.programState = kProgramStateEraseMainFlashBlock;
		m_statusStruct.programProgress = 0;
		m_statusStruct.programTotalSize = 50;
		while (m_statusStruct.programProgress < 5)	{AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
		m_statusStruct.programState = kProgramStateEraseSecondFlashBlock;
		while (m_statusStruct.programProgress < 10) {AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
		m_statusStruct.programState = kProgramStateEraseFailSafeFlashBlock;

		//	Alternate failure/success with each successive update
		if (!SIMULATE_FAILURE)
		{
			while (m_statusStruct.programProgress < 15) {AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
			m_statusStruct.programState = kProgramStateProgramFlash;
			while (m_statusStruct.programProgress < 35) {AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
			m_statusStruct.programState = kProgramStateVerifyFlash;
			while (m_statusStruct.programProgress < 50) {AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
			m_updateSuccessful = true;
		}
	}	//	else SIMULATE_UPDATE

	if (!m_updateSuccessful)
	{
		FITERR("CNTV2FirmwareInstallerThread:  " << (SIMULATE_UPDATE?"SIMULATED ":"") << "Firmware update failed" << endl
				<< "	bitfile: " << m_bitfilePath << endl
				<< "	 device: " << m_deviceInfo.deviceIdentifier << ", S/N " << serialNumStr << endl
				<< "   firmware: " << newFirmwareDescription);
		return AJA_STATUS_FAIL;
	}
	if (m_verbose)
		FITNOTE("CNTV2FirmwareInstallerThread:  " << (SIMULATE_UPDATE?"SIMULATED ":"") << "Firmware update completed" << endl
				<< "	bitfile: " << m_bitfilePath << endl
				<< "	 device: " << m_deviceInfo.deviceIdentifier << ", S/N " << serialNumStr << endl
				<< "   firmware: " << newFirmwareDescription);

	return AJA_STATUS_SUCCESS;

}	//	run


string CNTV2FirmwareInstallerThread::GetStatusString (void) const
{
	InternalUpdateStatus ();
	switch (m_statusStruct.programState)
	{
		case kProgramStateEraseMainFlashBlock:		return "Erasing...";
		case kProgramStateEraseSecondFlashBlock:	return gDebugging ? "Erasing second flash block..." : "Erasing...";
		case kProgramStateEraseFailSafeFlashBlock:	return gDebugging ? "Erasing fail-safe..." : "Erasing...";
		case kProgramStateProgramFlash:				return "Programming...";
		case kProgramStateVerifyFlash:				return "Verifying...";
		case kProgramStateFinished:					return "Done";
		case kProgramStateEraseBank3:				return "Erasing bank 3...";
		case kProgramStateProgramBank3:				return "Programmming bank 3...";
		case kProgramStateVerifyBank3:				return "Verifying bank 3...";
		case kProgramStateEraseBank4:				return "Erasing bank 4...";
		case kProgramStateProgramBank4:				return "Programming bank 4...";
		case kProgramStateVerifyBank4:				return "Verifying bank 4...";
		case kProgramStateCalculating:				return "Calculating.....";
		case kProgramStateErasePackageInfo:			return "Erasing Package Info...";
		case kProgramStateProgramPackageInfo:		return "Programming Package Info...";
		case kProgramStateVerifyPackageInfo:		return "VerifyingPackageInfo....";
	}
	return "Internal error";
}

uint32_t CNTV2FirmwareInstallerThread::GetProgressValue (void) const
{
	InternalUpdateStatus ();
	return m_statusStruct.programProgress;
}


uint32_t CNTV2FirmwareInstallerThread::GetProgressMax (void) const
{
	InternalUpdateStatus ();
	if (m_statusStruct.programTotalSize == 0)
		return 1;
	else
		return m_statusStruct.programTotalSize;
}


void CNTV2FirmwareInstallerThread::InternalUpdateStatus (void) const
{
	if (!SIMULATE_UPDATE  &&  m_device.IsOpen ())
		m_device.GetProgramStatus (&m_statusStruct);
}


CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread ()
	:	m_deviceInfo		(),
		m_bitfilePath		(),
		m_updateSuccessful	(false),
		m_verbose			(false),
		m_forceUpdate		(false),
		m_useDynamicReconfig (false)
{
	NTV2_ASSERT (false);
}

CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread (const CNTV2FirmwareInstallerThread & inObj)
	:	m_deviceInfo		(),
		m_bitfilePath		(),
		m_updateSuccessful	(false),
		m_verbose			(false),
		m_forceUpdate		(false),
		m_useDynamicReconfig (false)
{
	(void) inObj;
	NTV2_ASSERT (false);
}

CNTV2FirmwareInstallerThread & CNTV2FirmwareInstallerThread::operator = (const CNTV2FirmwareInstallerThread & inObj)
{	(void)inObj;
	NTV2_ASSERT (false);
	return *this;
}

bool CNTV2FirmwareInstallerThread::ShouldUpdateIPDevice (const NTV2DeviceID inDeviceID, const string & designName) const
{
#if 1
	static const NTV2DeviceID devIDs[] = {	DEVICE_ID_KONAIP_2022,			DEVICE_ID_KONAIP_4CH_2SFP,		DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K,
											DEVICE_ID_KONAIP_2TX_1SFP_J2K,	DEVICE_ID_KONAIP_1RX_1TX_2110,	DEVICE_ID_KONAIP_2110,
											DEVICE_ID_IOIP_2022,			DEVICE_ID_IOIP_2110,			inDeviceID};
	for (int ndx(0);  ndx < 9;  ndx++)
		cout << ::NTV2DeviceIDToString(devIDs[ndx]) << " name " << CNTV2Bitfile::GetPrimaryHardwareDesignName(devIDs[ndx]) << endl;
#endif
	string name (CNTV2Bitfile::GetPrimaryHardwareDesignName(inDeviceID));

	// Can always install over self
	if (designName == name)
		return true;

	cout << "Be sure we can install '" << designName.c_str() << "', replacing '" << name.c_str() << "'" << endl;

	//	Special cases -- e.g. bitfile flipping, P2P, etc...
	//	**MrBill**	DUPLICITOUS		See CNTV2Bitfile::CanFlashDevice in ntv2bitfile.cpp
	switch (inDeviceID)
	{
/*	case DEVICE_ID_CORVID44:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44) ||
				designName == "corvid_446");	//	Corvid 446
	case DEVICE_ID_KONA3GQUAD:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONA3G) ||
				designName == "K3G_quad_p2p");	//	K3G_quad_p2p.ncd
	case DEVICE_ID_KONA3G:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONA3GQUAD) ||
				designName == "K3G_p2p");		//	K3G_p2p.ncd
	case DEVICE_ID_KONA4UFC:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONA4));
	case DEVICE_ID_KONA5:
	case DEVICE_ID_KONA5_2X4K:
	case DEVICE_ID_KONA5_3DLUT:
	case DEVICE_ID_KONA5_8KMK:
	case DEVICE_ID_KONA5_8K:
	case DEVICE_ID_KONA5_OE1:
	case DEVICE_ID_KONA5_OE2:
	case DEVICE_ID_KONA5_OE3:
	case DEVICE_ID_KONA5_OE4:
	case DEVICE_ID_KONA5_OE5:
	case DEVICE_ID_KONA5_OE6:
	case DEVICE_ID_KONA5_OE7:
	case DEVICE_ID_KONA5_OE8:
	case DEVICE_ID_KONA5_OE9:
	case DEVICE_ID_KONA5_OE10:
	case DEVICE_ID_KONA5_OE11:
	case DEVICE_ID_KONA5_OE12:
	case DEVICE_ID_KONA5_8K_MV_TX:
		return (designName == GetPrimaryDesignName(DEVICE_ID_KONA5) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_2X4K) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_3DLUT) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_8KMK) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_8K) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE1) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE2) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE3) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE4) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE5) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE6) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE7) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE8) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE9) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE10) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE11) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_OE12) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_8K_MV_TX));
	case DEVICE_ID_CORVID44_8KMK:
	case DEVICE_ID_CORVID44_8K:
	case DEVICE_ID_CORVID44_2X4K:
	case DEVICE_ID_CORVID44_PLNR:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_8KMK) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_8K) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_2X4K) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_PLNR));
	case DEVICE_ID_IO4K:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_IO4KUFC));
	case DEVICE_ID_IO4KUFC:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_IO4K));
	case DEVICE_ID_CORVID88:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_CORVID88) ||
				designName == "CORVID88");		//	older design name
	case DEVICE_ID_KONA4:
	{
		if (m_device.IsIPDevice())
			return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONA4UFC) ||
					designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2022) ||
					designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_4CH_2SFP) ||
					designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
					designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2TX_1SFP_J2K) ||
					designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_1RX_1TX_2110) ||
					designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2110) ||
					designName == "s2022_56_2p2ch_rxtx_mb" ||
					designName == "s2022_12_2ch_tx_spoof" ||
					designName == "s2022_12_2ch_tx" ||
					designName == "s2022_12_2ch_rx" ||
					designName == "s2022_56_4ch_rxtx_fec" ||
					designName == "s2022_56_4ch_rxtx" ||
					designName == "s2110_4tx" ||
					designName == "s2022_56_1rx_1tx_2110");
		else
			return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONA4UFC));
	}
	case DEVICE_ID_TTAP_PRO:
		return designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_TTAP_PRO);
*/	case DEVICE_ID_KONAIP_2022:
	case DEVICE_ID_KONAIP_4CH_2SFP:
	case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:
	case DEVICE_ID_KONAIP_2TX_1SFP_J2K:
	case DEVICE_ID_KONAIP_1RX_1TX_2110:
	case DEVICE_ID_KONAIP_2110:
	case DEVICE_ID_KONAIP_2110_RGB12:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2022) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_4CH_2SFP) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2TX_1SFP_J2K) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_1RX_1TX_2110) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2110) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2110_RGB12) ||
				designName == "s2022_56_2p2ch_rxtx_mb" ||
				designName == "s2022_12_2ch_tx_spoof" ||
				designName == "s2022_12_2ch_tx" ||
				designName == "s2022_12_2ch_rx" ||
				designName == "s2022_56_4ch_rxtx_fec" ||
				designName == "s2110_1rx_1tx"); 
	case DEVICE_ID_IOIP_2022:
	case DEVICE_ID_IOIP_2110:
	case DEVICE_ID_IOIP_2110_RGB12:
		return (designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2022) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2110) ||
				designName == CNTV2Bitfile::GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2110_RGB12));
	default: break;
	}
	return false;
}
