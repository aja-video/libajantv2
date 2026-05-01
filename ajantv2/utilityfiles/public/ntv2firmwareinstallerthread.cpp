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
	const string bitfileName	(::NTV2GetBitfileName (inDeviceID));
	const string firmwareFolder	(::NTV2GetFirmwareFolderPath (/*trailingSlash*/true));
	const string resultPath		(firmwareFolder + bitfileName);
	return resultPath;
}


int NeedsFirmwareUpdate (CNTV2Card & inDevice, string & outReason)
{
	string			installedDate, installedTime, serialNumStr, newFirmwareDescription;
	ULWord			numBytes		(0);
	CNTV2Bitfile	bitfile;

	outReason.clear();
	if (!inDevice.IsOpen())
		{outReason = "device '" + inDevice.GetDescription() + "' not open";		return kFirmwareUpdateCheckFailed;}
	if (!inDevice.IsDeviceReady(false))
		{outReason = "device '" + inDevice.GetDescription() + "' not ready";	return kFirmwareUpdateCheckFailed;}
	if (inDevice.IsRemote())
		{outReason = "device '" + inDevice.GetDescription() + "' not local physical";	return kFirmwareUpdateCheckFailed;}
	const string firmwarePath (::GetFirmwarePath(inDevice.GetDeviceID()));
#if 0	//	IP10G purge
	CNTV2MCSfile	mcsFile;
	if (firmwarePath.find(".mcs") != std::string::npos)
	{
		//	.MCS file?
		CNTV2KonaFlashProgram kfp (inDevice.GetIndexNumber());
		kfp.GetMCSInfo();
		if (!mcsFile.GetMCSHeaderInfo(firmwarePath))
			{outReason = "MCS File open failed";	return kFirmwareUpdateCheckFailed;}

		string fileDate = mcsFile.GetMCSPackageDateString();
		string fileVersion = mcsFile.GetMCSPackageVersionString();
		PACKAGE_INFO_STRUCT currentInfo;
		inDevice.GetPackageInformation(currentInfo);
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
#endif	//	IP10G purge
	if (inDevice.GetInstalledBitfileInfo (numBytes, installedDate, installedTime))
	{
		if (bitfile.Open (firmwarePath))
		{
			//	If we can dynamically reconfig return true
			if (inDevice.IsDynamicDevice())
			{
				inDevice.AddDynamicDirectory((::NTV2GetFirmwareFolderPath()));
#ifdef AJA_WINDOWS
				NTV2DeviceID desiredID (bitfile.GetDeviceID());
				if (inDevice.CanLoadDynamicDevice(desiredID))
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
		outReason = "GetInstalledBitfileInfo failed for " + inDevice.GetDescription();
	return kFirmwareUpdateCheckFailed;	//	failure
}


int NeedsFirmwareUpdate (CNTV2Card & inDevice)
{
	string	notUsed;
	return NeedsFirmwareUpdate (inDevice, notUsed);
}



CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread (CNTV2Card & inDevice,
															const string & inBitfilePath,
															const bool inVerbose,
															const bool inForceUpdate)
	:	m_device			(inDevice),
		m_bitfilePath		(inBitfilePath),
		m_updateSuccessful	(false),
		m_verbose			(inVerbose),
		m_forceUpdate		(inForceUpdate),
		m_useDynamicReconfig (false)
{
	::memset (&m_statusStruct, 0, sizeof (m_statusStruct));
}

CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread (CNTV2Card & inDevice,
															const string & inDRFilesPath,
															const NTV2DeviceID inDesiredID,
															const bool inVerbose)
	:	m_device			(inDevice),
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
	if (!m_device.IsOpen())
	{
		FITERR("CNTV2FirmwareInstallerThread:  Device not open");
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
#if 0	//	IoIP/KonaIP10g purge
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
		if (!m_forceUpdate  &&  !ShouldUpdateIPDevice(m_device.GetDeviceID(), mcsFile.GetBitfileDesignString()))
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
#endif	//	IP10G purge
	if (m_useDynamicReconfig)
	{
		m_device.AddDynamicDirectory(::NTV2GetFirmwareFolderPath());
		if (!m_device.CanLoadDynamicDevice(m_desiredID))
		{
			FITERR("CNTV2FirmwareInstallerThread: '" << m_desiredID << "' is not compatible with "
					<< m_device.GetDescription());
			return AJA_STATUS_FAIL;
		}
		if (m_verbose)
			FITNOTE("CNTV2FirmwareInstallerThread:  Dynamic Reconfig started" << endl
					<< "     device: " << m_device.GetDescription() << ", S/N " << serialNumStr << endl
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
		NTV2Buffer		bitfileBuffer(bitfileLength + 512);
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
		if (!m_forceUpdate  &&  !bitfile.CanFlashDevice(m_device.GetDeviceID()))
		{
			FITERR("CNTV2FirmwareInstallerThread:  Bitfile design '" << designName << "' is not compatible with "
					<< m_device.GetDescription());
			return AJA_STATUS_FAIL;
		}

		//	Update firmware...
		if (m_verbose)
			FITNOTE("CNTV2FirmwareInstallerThread:  Firmware update started" << endl
					<< "    bitfile: " << m_bitfilePath << endl
					<< "     device: " << m_device.GetDescription() << ", S/N " << serialNumStr << endl
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
						<< "	  device: " << m_device.GetDescription() << ", S/N " << serialNumStr << endl
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
				<< "	 device: " << m_device.GetDescription() << ", S/N " << serialNumStr << endl
				<< "   firmware: " << newFirmwareDescription);
		return AJA_STATUS_FAIL;
	}
	if (m_verbose)
		FITNOTE("CNTV2FirmwareInstallerThread:  " << (SIMULATE_UPDATE?"SIMULATED ":"") << "Firmware update completed" << endl
				<< "	bitfile: " << m_bitfilePath << endl
				<< "	 device: " << m_device.GetDescription() << ", S/N " << serialNumStr << endl
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

static CNTV2Card sNullDevice;

CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread ()
	:	m_device			(sNullDevice),
		m_bitfilePath		(),
		m_updateSuccessful	(false),
		m_verbose			(false),
		m_forceUpdate		(false),
		m_useDynamicReconfig (false)
{
	NTV2_ASSERT (false);
}

CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread (const CNTV2FirmwareInstallerThread & inObj)
	:	m_device			(inObj.m_device),
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
#if 0	//	IoIP/KonaIP10g purge
bool CNTV2FirmwareInstallerThread::ShouldUpdateIPDevice (const NTV2DeviceID inDeviceID, const string & designName) const
{
	string name (CNTV2Bitfile::GetPrimaryHardwareDesignName(inDeviceID));
	// Can always install over self
	return designName == name;
}	//	ShouldUpdateIPDevice
#endif	//	IoIP/KonaIP10g purge
