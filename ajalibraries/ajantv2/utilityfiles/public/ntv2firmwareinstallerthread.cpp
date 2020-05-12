/**
	@file		ntv2firmwareinstallerthread.cpp
	@brief		Implementation of CNTV2FirmwareInstallerThread class.
	@copyright	(C) 2014-2020 AJA Video Systems, Inc.	Proprietary and confidential information.  All rights reserved.
**/

#include "ntv2firmwareinstallerthread.h"
#include "ntv2bitfile.h"
#include "ntv2utils.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/systemtime.h"
#include "ntv2konaflashprogram.h"

using namespace std;


#if defined (AJADebug) || defined (_DEBUG) || defined (DEBUG)
	static const bool	gDebugging	(true);
#else
	static const bool	gDebugging	(false);
#endif


#define	REALLY_UPDATE		true		///<	Set this to false to simulate flashing a device


static const uint32_t		kMilliSecondsPerSecond	(1000);


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
	const string	firmwarePath	(::GetFirmwarePath (inDeviceInfo.deviceID));
	CNTV2Card		device			(inDeviceInfo.deviceIndex);
	CNTV2Bitfile	bitfile;
	CNTV2MCSfile	mcsFile;

	outReason.clear ();
    if (device.IsOpen () && device.IsDeviceReady(false))
	{
		if (firmwarePath.find(".mcs") != std::string::npos)
		{
			//We have an mcs file?
            CNTV2KonaFlashProgram kfp;
            kfp.SetBoard(device.GetIndexNumber());
            kfp.GetMCSInfo();
            if(!mcsFile.GetMCSHeaderInfo(firmwarePath))
			{
				outReason = "MCS File open failed";
				return kFirmwareUpdateCheckFailed;
			}
			else
			{
				string fileDate = mcsFile.GetMCSPackageDateString();
				string fileVersion = mcsFile.GetMCSPackageVersionString();
				PACKAGE_INFO_STRUCT currentInfo;
				device.GetPackageInformation(currentInfo);
				if(fileDate == currentInfo.date)
					return 0;
				else if(currentInfo.date > fileDate)
				{
					outReason = "on-device firmware " + installedDate + " newer than on-disk bitfile firmware " + bitfile.GetDate ();
					return 1;	//	on-device firmware newer than on-disk bitfile firmware
				}
				else
				{
					outReason = "on-device firmware " + installedDate + " older than on-disk bitfile firmware " + bitfile.GetDate ();
					return -1;	//	on-device firmware older than on-disk bitfile firmware
				}
			}
		}
		else if (device.GetInstalledBitfileInfo (numBytes, installedDate, installedTime))
		{
			if (bitfile.Open (firmwarePath))
			{
				//cout << inDeviceInfo.deviceIdentifier << ":  file: " << bitfile.GetDate () << "  device: " << installedDate << endl;
				if (bitfile.GetDate () == installedDate)
					return 0;	//	Identical!
				else if (installedDate > bitfile.GetDate ())
				{
					outReason = "on-device firmware " + installedDate + " newer than on-disk bitfile firmware " + bitfile.GetDate ();
					return 1;	//	on-device firmware newer than on-disk bitfile firmware
				}
				else
				{
					outReason = "on-device firmware " + installedDate + " older than on-disk bitfile firmware " + bitfile.GetDate ();
					return -1;	//	on-device firmware older than on-disk bitfile firmware
				}
			}
			else
				outReason = bitfile.GetLastError ();
		}
		else
			outReason = "GetInstalledBitfileInfo failed for device '" + inDeviceInfo.deviceIdentifier + "'";
	}
	else
		outReason = "device '" + inDeviceInfo.deviceIdentifier + "' not open";
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
		m_forceUpdate		(inForceUpdate)
{
	::memset (&m_statusStruct, 0, sizeof (m_statusStruct));
}



AJAStatus CNTV2FirmwareInstallerThread::ThreadRun (void)
{
	m_device.Open (m_deviceInfo.deviceIndex);
	if (!m_device.IsOpen ())
	{
		cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Device not open" << endl;
		return AJA_STATUS_OPEN;
	}
	if (m_bitfilePath.empty ())
	{
		cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Bitfile path is empty!" << endl;
		return AJA_STATUS_BAD_PARAM;
	}

	m_device.WriteRegister(kVRegFlashStatus, 0);

	//	Preflight bitfile...
	ULWord	numBytes	(0);
	string	installedDate, installedTime, serialNumStr, newFirmwareDescription;
	if (!m_device.GetInstalledBitfileInfo (numBytes, installedDate, installedTime))
		cerr << "## WARNING:  CNTV2FirmwareInstallerThread:  Unable to obtain installed bitfile info" << endl;
	m_device.GetSerialNumberString (serialNumStr);

    if (m_bitfilePath.find(".mcs") != std::string::npos)
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
            m_updateSuccessful = false;
            return AJA_STATUS_FAIL;
        }

		{
			CNTV2MCSfile mcsFile;
			mcsFile.GetMCSHeaderInfo(m_bitfilePath);
			if (!m_forceUpdate)
			{
				if (!ShouldUpdate(m_deviceInfo.deviceID, mcsFile.GetBitfileDesignString()))
				{
                    cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Invalid MCS update" << endl;
					m_updateSuccessful = false;
                    return AJA_STATUS_BAD_PARAM;
				}
			}
		}

        m_device. WriteRegister(kVRegFlashStatus,kfp.NextMcsStep());
        rv = kfp.SetMCSFile(m_bitfilePath.c_str());
        if (!rv)
        {
            cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  SetMCSFile failed" << endl;
            m_updateSuccessful = false;
            return AJA_STATUS_FAIL;
        }

        if(m_forceUpdate)
            kfp.SetMBReset();
        rv = kfp.ProgramFromMCS(true);
        if (!rv)
        {
            cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  ProgramFromMCS failed" << endl;
            m_updateSuccessful = false;
            return AJA_STATUS_FAIL;
        }

        m_updateSuccessful = true;
        m_device.WriteRegister(kVRegFlashState,kProgramStateFinished);
        m_device.WriteRegister(kVRegFlashSize,MCS_STEPS);
        m_device.WriteRegister(kVRegFlashStatus,MCS_STEPS);

        cout << "## NOTE:  CNTV2FirmwareInstallerThread:  MCS update succeeded" << endl;
        return AJA_STATUS_SUCCESS;
    }

	{
		CNTV2Bitfile	bitfile;

		if (!bitfile.Open (m_bitfilePath))
		{
			const string	extraInfo	(bitfile.GetLastError());
			cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Bitfile '" << m_bitfilePath << "' open/parse error";
			if (!extraInfo.empty())
				cerr << ": " << extraInfo;
			cerr << endl;
			return AJA_STATUS_OPEN;
		}

		const unsigned	bitfileLength	(bitfile.GetFileStreamLength ());
		unsigned char *	bitfileBuffer	(new unsigned char [bitfileLength + 512]);
		if (bitfileBuffer == NULL)
		{
			cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Unable to allocate bitfile buffer" << endl;
			return AJA_STATUS_MEMORY;
		}

		::memset (bitfileBuffer, 0xFF, bitfileLength + 512);
		const unsigned	readBytes	(bitfile.GetFileByteStream (bitfileBuffer, bitfileLength));
		const string	designName	(bitfile.GetDesignName ());
		newFirmwareDescription = m_bitfilePath + " - " + bitfile.GetDate () + " " + bitfile.GetTime ();
		if (readBytes != bitfileLength)
		{
			delete [] bitfileBuffer;
			cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Invalid bitfile length, read " << readBytes << " bytes, expected " << bitfileLength << endl;
			return AJA_STATUS_FAIL;
		}

		if (!m_forceUpdate)
		{
			if (!bitfile.CanFlashDevice (m_deviceInfo.deviceID))
			{
				cerr	<< "## ERROR:  CNTV2FirmwareInstallerThread:  Bitfile design '" << designName << "' is not compatible with device '"
						<< m_deviceInfo.deviceIdentifier << "'" << endl;
				return AJA_STATUS_FAIL;
			}
		}
	}

	//	Update firmware...
	if (m_verbose)
		cerr	<< "## NOTE:  CNTV2FirmwareInstallerThread:  Firmware update started" << endl
				<< "    bitfile: " << m_bitfilePath << endl
				<< "     device: " << m_deviceInfo.deviceIdentifier << ", S/N " << serialNumStr << endl
				<< "   firmware: " << newFirmwareDescription << endl;

	if (REALLY_UPDATE)
	{
		//	ProgramMainFlash used to be able to throw (because XilinxBitfile could throw), but with 12.1 SDK, this is no longer the case.
		m_updateSuccessful = m_device.ProgramMainFlash (m_bitfilePath.c_str (), m_forceUpdate, !m_verbose);
		if (!m_updateSuccessful)
		{
			cerr	<< "## ERROR:  CNTV2FirmwareInstallerThread:  'ProgramMainFlash' failed" << endl
					<< "     bitfile: " << m_bitfilePath << endl
					<< "      device: " << m_deviceInfo.deviceIdentifier << ", S/N " << serialNumStr << endl
					<< "   serialNum: " << serialNumStr << endl
					<< "    firmware: " << newFirmwareDescription << endl;
			m_updateSuccessful = false;
		}
	}	//	if REALLY_UPDATE
	else
	{
		//	PHONEY PROGRESS FOR TESTING
		static unsigned	sTestCount (0);		//	Used to alternate success/failure with successive updates

		m_statusStruct.programState = kProgramStateEraseMainFlashBlock;
		m_statusStruct.programProgress = 0;
		m_statusStruct.programTotalSize = 50;
		while (m_statusStruct.programProgress < 5)	{AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
		m_statusStruct.programState = kProgramStateEraseSecondFlashBlock;
		while (m_statusStruct.programProgress < 10)	{AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
		m_statusStruct.programState = kProgramStateEraseFailSafeFlashBlock;

		//	Alternate failure/success with each successive update
		if ((sTestCount++ & 1) != 0)
		{
			while (m_statusStruct.programProgress < 15)	{AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
			m_statusStruct.programState = kProgramStateProgramFlash;
			while (m_statusStruct.programProgress < 35)	{AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
			m_statusStruct.programState = kProgramStateVerifyFlash;
			while (m_statusStruct.programProgress < 50)	{AJATime::Sleep (kMilliSecondsPerSecond);	m_statusStruct.programProgress++;}
			m_updateSuccessful = true;
		}
	}	//	else phony update

	if (!m_updateSuccessful)
	{
		cerr	<< "## ERROR:  CNTV2FirmwareInstallerThread:  Firmware update failed" << endl
				<< "    bitfile: " << m_bitfilePath << endl
				<< "     device: " << m_deviceInfo.deviceIdentifier << ", S/N " << serialNumStr << endl
				<< "   firmware: " << newFirmwareDescription << endl;
		return AJA_STATUS_FAIL;
	}
	if (m_verbose)
		cout	<< "## NOTE:  CNTV2FirmwareInstallerThread:  Firmware update completed" << endl
				<< "    bitfile: " << m_bitfilePath << endl
				<< "     device: " << m_deviceInfo.deviceIdentifier << ", S/N " << serialNumStr << endl
				<< "   firmware: " << newFirmwareDescription << endl;

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
        case kProgramStateEraseBank3:               return "Erasing bank 3...";
        case kProgramStateProgramBank3:             return "Programmming bank 3...";
        case kProgramStateVerifyBank3:              return "Verifying bank 3...";
        case kProgramStateEraseBank4:               return "Erasing bank 4...";
        case kProgramStateProgramBank4:             return "Programming bank 4...";
        case kProgramStateVerifyBank4:              return "Verifying bank 4...";
        case kProgramStateCalculating:              return "Calculating.....";
        case kProgramStateErasePackageInfo:         return "Erasing Package Info...";
        case kProgramStateProgramPackageInfo:       return "Programming Package Info...";
        case kProgramStateVerifyPackageInfo:        return "VerifyingPackageInfo....";
        default:
            return "Internal error";
	}

	return "";
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
	if (REALLY_UPDATE && m_device.IsOpen ())
		m_device.GetProgramStatus (&m_statusStruct);
}


CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread ()
	:	m_deviceInfo		(),
		m_bitfilePath		(),
		m_updateSuccessful	(false),
		m_verbose			(false),
		m_forceUpdate		(false)
{
	NTV2_ASSERT (false);
}

CNTV2FirmwareInstallerThread::CNTV2FirmwareInstallerThread (const CNTV2FirmwareInstallerThread & inObj)
	:	m_deviceInfo		(),
		m_bitfilePath		(),
		m_updateSuccessful	(false),
		m_verbose			(false),
		m_forceUpdate		(false)
{
	(void) inObj;
	NTV2_ASSERT (false);
}

CNTV2FirmwareInstallerThread & CNTV2FirmwareInstallerThread::operator = (const CNTV2FirmwareInstallerThread & inObj)
{
    (void)inObj;

	NTV2_ASSERT (false);
	return *this;
}

bool CNTV2FirmwareInstallerThread::ShouldUpdate(const NTV2DeviceID inDeviceID, const std::string designName) const
{
    std::string name;

#if 0
    name = GetPrimaryDesignName(DEVICE_ID_KONAIP_2022);
    printf("DEVICE_ID_KONAIP_2022 name %s\n", name.c_str());

    name = GetPrimaryDesignName(DEVICE_ID_KONAIP_4CH_2SFP);
    printf("DEVICE_ID_KONAIP_4CH_2SFP name %s\n", name.c_str());

    name = GetPrimaryDesignName(DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K);
    printf("DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K name %s\n", name.c_str());

    name = GetPrimaryDesignName(DEVICE_ID_KONAIP_2TX_1SFP_J2K);
    printf("DEVICE_ID_KONAIP_2TX_1SFP_J2K name %s\n", name.c_str());

	name = GetPrimaryDesignName(DEVICE_ID_KONAIP_1RX_1TX_2110);
	printf("DEVICE_ID_KONAIP_1RX_1TX_2110 name %s\n", name.c_str());

    name = GetPrimaryDesignName(DEVICE_ID_KONAIP_2110);
    printf("DEVICE_ID_KONAIP_2110 name %s\n", name.c_str());

    name = GetPrimaryDesignName(DEVICE_ID_IOIP_2022);
    printf("DEVICE_ID_IOIP_2022 name %s\n", name.c_str());

    name = GetPrimaryDesignName(DEVICE_ID_IOIP_2110);
    printf("DEVICE_ID_IOIP_2110 name %s\n", name.c_str());

    name = GetPrimaryDesignName(inDeviceID);
    printf("inDeviceID name %s\n", name.c_str());
    printf("designName name %s\n", designName.c_str());
#endif

    name = GetPrimaryDesignName(inDeviceID);

    // Can always install over self
    if (designName == name)
        return true;

    printf("Make sure we can install %s, replacing %s\n", designName.c_str(), name.c_str());

	//	Special cases -- e.g. bitfile flipping, P2P, etc...
	switch (inDeviceID)
	{
	case DEVICE_ID_CORVID44:
		return (designName == GetPrimaryDesignName(DEVICE_ID_CORVID44) ||
				designName == "corvid_446");	//	Corvid 446
	case DEVICE_ID_KONA3GQUAD:
		return (designName == GetPrimaryDesignName(DEVICE_ID_KONA3G) ||
				designName == "K3G_quad_p2p");	//	K3G_quad_p2p.ncd
	case DEVICE_ID_KONA3G:
		return (designName == GetPrimaryDesignName(DEVICE_ID_KONA3GQUAD) ||
				designName == "K3G_p2p");		//	K3G_p2p.ncd
	case DEVICE_ID_KONA4UFC:
		return (designName == GetPrimaryDesignName(DEVICE_ID_KONA4));
	case DEVICE_ID_KONA5:
	case DEVICE_ID_KONA5_2:
	case DEVICE_ID_KONA5_8KMK:
	case DEVICE_ID_KONA5_8K:
        return (designName == GetPrimaryDesignName(DEVICE_ID_KONA5) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_2) ||
                designName == GetPrimaryDesignName(DEVICE_ID_KONA5_8KMK) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONA5_8K));
	case DEVICE_ID_CORVID44_8KMK:
	case DEVICE_ID_CORVID44_8K:
	case DEVICE_ID_CORVID44_2X4K:
        return (designName == GetPrimaryDesignName(DEVICE_ID_CORVID44_8KMK) ||
				designName == GetPrimaryDesignName(DEVICE_ID_CORVID44_8K) ||
				designName == GetPrimaryDesignName(DEVICE_ID_CORVID44_2X4K));
	case DEVICE_ID_IO4K:
		return (designName == GetPrimaryDesignName(DEVICE_ID_IO4KUFC));
	case DEVICE_ID_IO4KUFC:
		return (designName == GetPrimaryDesignName(DEVICE_ID_IO4K));
	case DEVICE_ID_CORVID88:
		return (designName == GetPrimaryDesignName(DEVICE_ID_CORVID88) ||
				designName == "CORVID88");		//	older design name
	case DEVICE_ID_KONA4:
	{
		if (m_device.IsIPDevice())
			return (designName == GetPrimaryDesignName(DEVICE_ID_KONA4UFC) ||
					designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_2022) ||
					designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_4CH_2SFP) ||
                    designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
                    designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_2TX_1SFP_J2K) ||
					designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_1RX_1TX_2110) ||
					designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_2110) ||
					designName == "s2022_56_2p2ch_rxtx_mb" ||
                    designName == "s2022_12_2ch_tx_spoof" ||
                    designName == "s2022_12_2ch_tx" ||
                    designName == "s2022_12_2ch_rx" ||
                    designName == "s2022_56_4ch_rxtx_fec" ||
                    designName == "s2022_56_4ch_rxtx" ||
                    designName == "s2110_4tx" ||
                    designName == "s2022_56_1rx_1tx_2110");
		else
			return (designName == GetPrimaryDesignName(DEVICE_ID_KONA4UFC));
	}
	case DEVICE_ID_KONAIP_2022:
    case DEVICE_ID_KONAIP_4CH_2SFP:
    case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:
	case DEVICE_ID_KONAIP_2TX_1SFP_J2K:
	case DEVICE_ID_KONAIP_1RX_1TX_2110:
	case DEVICE_ID_KONAIP_2110:
        return (designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_2022) ||
                designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_4CH_2SFP) ||
                designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_2TX_1SFP_J2K) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_1RX_1TX_2110) ||
				designName == GetPrimaryDesignName(DEVICE_ID_KONAIP_2110) ||
                designName == "s2022_56_2p2ch_rxtx_mb" ||
                designName == "s2022_12_2ch_tx_spoof" ||
                designName == "s2022_12_2ch_tx" ||
                designName == "s2022_12_2ch_rx" ||
				designName == "s2022_56_4ch_rxtx_fec" ||
                designName == "s2110_1rx_1tx"); 
    case DEVICE_ID_IOIP_2022:
    case DEVICE_ID_IOIP_2110:
        return (designName == GetPrimaryDesignName(DEVICE_ID_IOIP_2022) ||
                designName == GetPrimaryDesignName(DEVICE_ID_IOIP_2110));
	case DEVICE_ID_T3TAP:
		return designName == GetPrimaryDesignName(DEVICE_ID_T3TAP);
	default: break;
	}
	return false;
}

std::string CNTV2FirmwareInstallerThread::GetPrimaryDesignName(const NTV2DeviceID inDeviceID) const
{
	switch (inDeviceID)
	{
        case DEVICE_ID_CORVID1:						return "corvid1pcie";               //	top.ncd
        case DEVICE_ID_CORVID3G:					return "corvid1_3Gpcie";            //	corvid1_3Gpcie
        case DEVICE_ID_CORVID22:					return "top_c22";                   //	top_c22.ncd
        case DEVICE_ID_CORVID24:					return "corvid24_quad";             //	corvid24_quad.ncd
        case DEVICE_ID_CORVID44:					return "corvid_44";                 //	corvid_44
        case DEVICE_ID_CORVID88:					return "corvid_88";                 //	CORVID88
        case DEVICE_ID_CORVIDHEVC:					return "corvid_hevc";               //	CORVIDHEVC
        case DEVICE_ID_KONA3G:						return "K3G_top";                   //	K3G_top.ncd
        case DEVICE_ID_KONA3GQUAD:					return "K3G_quad";                  //	K3G_quad.ncd
        case DEVICE_ID_KONA4:						return "kona_4_quad";               //	kona_4_quad
        case DEVICE_ID_KONA4UFC:					return "kona_4_ufc";                //	kona_4_ufc
        case DEVICE_ID_IO4K:						return "IO_XT_4K";                  //	IO_XT_4K
        case DEVICE_ID_IO4KUFC:						return "IO_XT_4K_UFC";              //	IO_XT_4K_UFC
        case DEVICE_ID_IOEXPRESS:					return "chekov_00_pcie";            //	chekov_00_pcie.ncd
        case DEVICE_ID_IOXT:						return "top_IO_TX";                 //	top_IO_TX.ncd
        case DEVICE_ID_KONALHEPLUS:					return "lhe_12_pcie";               //	lhe_12_pcie.ncd
        case DEVICE_ID_KONALHI:						return "top_pike";                  //	top_pike.ncd
        case DEVICE_ID_TTAP:						return "t_tap_top";                 //	t_tap_top.ncd
        case DEVICE_ID_KONAIP_2022:                 return "s2022_56_4ch_rxtx";         //	konaip22
        case DEVICE_ID_KONAIP_4CH_2SFP:             return "s2022_56_2p2ch_rxtx";
        case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:     return "s2022_12_1rx_1tx";
        case DEVICE_ID_KONAIP_2TX_1SFP_J2K:         return "s2022_12_2ch_tx_mb";
        case DEVICE_ID_KONAIP_1RX_1TX_2110:         return "s2110_1rx_1tx";
        case DEVICE_ID_IO4KPLUS:					return "IO_XT_4K_PLUS";
        case DEVICE_ID_IOIP_2022:                   return "ioip_s2022";
        case DEVICE_ID_IOIP_2110:                   return "ioip_s2110";
        case DEVICE_ID_KONAIP_2110:                 return "s2110_4tx";
        case DEVICE_ID_KONA1:                       return "kona_alpha";
        case DEVICE_ID_KONAHDMI:                    return "kona_hdmi";
		case DEVICE_ID_KONA5:						return "kona_5";
		case DEVICE_ID_KONA5_2:						return "kona_5_2";
		case DEVICE_ID_KONA5_8KMK:					return "kona5_8k_mk";
		case DEVICE_ID_CORVID44_8KMK:				return "c44_12g_8k_mk";
		case DEVICE_ID_KONA5_8K:					return "kona5_8k";
		case DEVICE_ID_CORVID44_8K:					return "c44_12g_8k";
		case DEVICE_ID_CORVID44_2X4K:				return "c44_12g_2X4K";
		case DEVICE_ID_T3TAP:						return "t3_tap";
        default: return "";
	}
}
