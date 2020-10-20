/**
	@file		ntv2bitfile.cpp
	@brief		Implementation of CNTV2Bitfile class.
	@copyright	(C) 2010-2020 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
**/
#include "ntv2bitfile.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include <iostream>
#include <sys/stat.h>
#include <assert.h>
#if defined (AJALinux) || defined (AJAMac)
	#include <arpa/inet.h>
#endif
#include <map>

using namespace std;


// TODO: Handle compressed bit-files
#define MAX_BITFILEHEADERSIZE 512
static unsigned char signature[8] = {0xFF,0xFF,0xFF,0xFF,0xAA,0x99,0x55,0x66};
static const unsigned char Head13[] = { 0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01 };


CNTV2Bitfile::CNTV2Bitfile ()
{
	Init ();	//	Initialize everything
}

CNTV2Bitfile::~CNTV2Bitfile ()
{
	Close ();
}

void CNTV2Bitfile::Close (void)
{
	if (_fileReady)
		_bitFileStream.close();
	_fileReady = false;
	_programStreamPos = 0;
	_fileStreamPos = 0;
	_fileProgrammingPosition = 0;
	_date = _time = _designName = _partName = _lastError = "";
	_tandem = false;
	_partial = false;
	_clear = false;
	_compress = false;
	_userID = 0;
	_designID = 0;
	_designVersion = 0;
	_bitfileID = 0;
	_bitfileVersion = 0;
}


void CNTV2Bitfile::Init (void)
{
	_fileReady = false;
	Close ();
}


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Bitfile::Open (const char * const & inBitfilePath)		{ return Open (string (inBitfilePath)); }
#endif	//	!defined (NTV2_DEPRECATE)


bool CNTV2Bitfile::Open (const string & inBitfileName)
{
	Close ();

	struct stat	fsinfo;
	::stat (inBitfileName.c_str (), &fsinfo);
	_fileSize = unsigned(fsinfo.st_size);

	_bitFileStream.open (inBitfileName.c_str (), std::ios::binary);

	do
	{
		if (_bitFileStream.fail ())
			{_lastError = "Unable to open bitfile";		break;}

		//	Preload bitfile header into mem
		for (int i = 0; i < MAX_BITFILEHEADERSIZE - 1; i++)
			_fileHeader.push_back (static_cast<unsigned char>(_bitFileStream.get()));

		_lastError = ParseHeader ();
		if (!_lastError.empty ())
			break;

		_fileReady = true;
	}
	while (false);

	return _fileReady;

}	//	Open

string CNTV2Bitfile::ParseHeaderFromBuffer(const uint8_t* inBitfileBuffer, const size_t inBufferSize)
{
	Close();

	do
	{
		//	Preload bitfile header into mem
		for (int i = 0; i < MAX_BITFILEHEADERSIZE - 1; i++) 
		{
			if (i < inBufferSize) 
			{
				_fileHeader.push_back(inBitfileBuffer[i]);
			}
			else 
			{
				break;
			}
		}

		_lastError = ParseHeader();
		if (!_lastError.empty())
			break;

		_fileReady = false;
	} while (false);

	return _lastError;

}	//	Open


//	ParseHeader returns empty string if header looks OK, otherwise string will contain failure explanation
string CNTV2Bitfile::ParseHeader ()
{
	uint32_t		fieldLen	(0);	//	Holds size of various header fields, in bytes
	int				pos			(0);	//	Byte offset during parse
	char			testByte	(0);	//	Used when checking against expected header bytes
	ostringstream	oss;				//	Receives parse error information

	//	This really is bad form -- it precludes the ability to make this method 'const' (which it really is), plus it's safer to use iterators...
	char *	p	(reinterpret_cast <char *> (&_fileHeader [0]));
	size_t maxPosValue(_fileHeader.size());

	do
	{
		//	Make sure first 13 bytes are what we expect...
		if (::memcmp (p, Head13, 13))
			{oss << "ParseHeader failed, byte mismatch in first 13 bytes";	break;}

		p += 13;						// skip over header header
		pos += 13;


		//	The next byte should be 'a'...
		testByte = *p++;
		if (testByte != 'a')
			{oss << "ParseHeader failed at or near byte offset " << pos << ", expected 'a', instead got '" << testByte << "'";	break;}
		pos++;
		
		fieldLen = htons(*(reinterpret_cast<uint16_t*>(p)));// the next 2 bytes are the length of the FileName (including /0)

		p += 2;							// now pointing at the beginning of the file name
		pos += 2;

		SetDesignName (p, fieldLen);	// grab design name
		SetDesignFlags (p, fieldLen);	// grab design flags
		SetDesignUserID (p, fieldLen);	// grab design userid

		p += fieldLen;					// skip over design name - now pointing to beginning of 'b' field
		pos += fieldLen;


		//	The next byte should be 'b'...
		testByte = *p++;
		if (testByte != 'b')
			{oss << "ParseHeader failed at or near byte offset " << pos << ", expected 'b', instead got '" << testByte << "'";	break;}
		pos++;
		
		fieldLen = htons(*(reinterpret_cast<uint16_t*>(p)));// the next 2 bytes are the length of the Part Name (including /0)

		p += 2;							// now pointing at the beginning of the part name
		pos += 2;

		_partName = p;					// grab part name

		p += fieldLen;					// skip over part name - now pointing to beginning of 'c' field
		pos += fieldLen;

		//	The next byte should be 'c'...
		testByte = *p++;
		if (testByte != 'c')
			{oss << "ParseHeader failed at or near byte offset " << pos << ", expected 'c', instead got '" << testByte << "'";	break;}
		pos++;

		fieldLen = htons (*(reinterpret_cast<uint16_t*>(p)));// the next 2 bytes are the length of the date string (including /0)

		p += 2;							// now pointing at the beginning of the date string
		pos += 2;

		_date = p;						// grab date string

		p += fieldLen;					// skip over date string - now pointing to beginning of 'd' field
		pos += fieldLen;

		//	The next byte should be 'd'...
		testByte = *p++;
		if (testByte != 'd')
			{oss << "ParseHeader failed at or near byte offset " << pos << ", expected 'd', instead got '" << testByte << "'";	break;}
		pos++;

		fieldLen = htons (*(reinterpret_cast<uint16_t*>(p)));// the next 2 bytes are the length of the time string (including /0)

		p += 2;							// now pointing at the beginning of the time string
		pos += 2;

		_time = p;						// grab time string

		p += fieldLen;					// skip over time string - now pointing to beginning of 'e' field
		pos += fieldLen;


		//	The next byte should be 'e'...
		testByte = *p++;
		if (testByte != 'e')
			{oss << "ParseHeader failed at or near byte offset " << pos << ", expected 'e', instead got '" << testByte << "'";	break;}
		pos++;

		_numBytes = htonl (*(reinterpret_cast<uint32_t*>(p)));	// the next 4 bytes are the length of the raw program data

		p += 4;							// now pointing at the beginning of the time string
		pos += 4;
		
		_fileProgrammingPosition = pos;	// this is where to start the programming stream

		//Search for the start signature
		bool bFound = (strncmp(p, reinterpret_cast<const char*>(signature), 8) == 0);
		int i = 0;
		while (bFound == false && i < 1000 && pos < maxPosValue)
		{
			bFound = strncmp(p, reinterpret_cast<const char*>(signature), 8) == 0;
			if(!bFound)
			{
				p++;
				i++;
				pos++;
			}
		}
		if (!bFound)
			{oss << "ParseHeader failed at or near byte offset " << pos << ", signature not found";	break;}

		assert (oss.str().empty());	//	If we made it this far it must be an OK Header - and no error messages
	} while (false);

	return oss.str();

}	//	ParseHeader


// Get length of program stream. 
// Return value:
//   Length of program stream
//   Return value < 0 indicates error
size_t CNTV2Bitfile::GetProgramStreamLength (void) const
{
	if (!_fileReady)
		return 0;
	return _numBytes;
}


// Get length of file 
// Return value:
//   Length of file
//   Return value < 0 indicates error
size_t CNTV2Bitfile::GetFileStreamLength (void) const
{
	if (!_fileReady)
		return 0;
	return _fileSize;
}


// Get maximum of bufferLength bytes worth of configuration stream data in buffer.
// Return value:
//   number of bytes of data copied into buffer. This can be <= bufferLength
//   zero means configuration stream has finished.
//   Return value 0 indicates error
size_t CNTV2Bitfile::GetProgramByteStream (unsigned char * pOutBuffer, const size_t bufferLength)
{
	size_t	posInBuffer			= 0;
	size_t	programStreamLength	= GetProgramStreamLength();

	if (!pOutBuffer)
		return 0;
	if (!_fileReady)
		return 0;

	_bitFileStream.seekg (_fileProgrammingPosition, std::ios::beg);

	while (_programStreamPos < programStreamLength)
	{
		if (_bitFileStream.eof())
		{	//	Unexpected end of file!
			ostringstream	oss;
			oss << "Unexpected EOF at " << _programStreamPos << "bytes";
			_lastError = oss.str ();
			return 0;
		}
		pOutBuffer[posInBuffer++] = static_cast<unsigned char>(_bitFileStream.get());
		_programStreamPos++;
		if (posInBuffer == bufferLength)
			break;
	}
	return posInBuffer;

}	//	GetProgramByteStream


size_t CNTV2Bitfile::GetProgramByteStream (NTV2_POINTER & outBuffer)
{
	const size_t programStreamLength(GetProgramStreamLength());
	if (!programStreamLength)
		return 0;
	if (!_fileReady)
		return 0;

	ostringstream oss;
	if (outBuffer.GetByteCount() < programStreamLength)	//	If buffer IsNULL or too small...
		outBuffer.Allocate(programStreamLength);			//	...then Allocate or resize it
	if (outBuffer.GetByteCount() < programStreamLength)	//	Check again
	{	oss << "Buffer size " << DEC(outBuffer.GetByteCount()) << " < " << DEC(programStreamLength);
		_lastError = oss.str();
		return 0;
	}
	return GetProgramByteStream (reinterpret_cast<unsigned char *>(outBuffer.GetHostAddress(0)),
								outBuffer.GetByteCount());
}


// Get maximum of fileLength bytes worth of configuration stream data in buffer.
// Return value:
//   number of bytes of data copied into buffer. This can be <= bufferLength
//   zero means configuration stream has finished.
//   Return value 0 indicates error
size_t CNTV2Bitfile::GetFileByteStream (unsigned char * pOutBuffer, const size_t bufferLength)
{
	size_t	posInBuffer			(0);
	size_t	fileStreamLength	(GetFileStreamLength());
	if (!pOutBuffer)
		return 0;
	if (!_fileReady)
		return 0;

	_bitFileStream.seekg (0, std::ios::beg);

	while (_fileStreamPos < fileStreamLength)
	{
		if (_bitFileStream.eof())
		{	//	Unexpected end of file!
			ostringstream	oss;
			oss << "Unexpected EOF at " << _programStreamPos << "bytes";
			_lastError = oss.str();
			return 0;
		}

		pOutBuffer[posInBuffer++] = static_cast<unsigned char>(_bitFileStream.get());
		_programStreamPos++;
		if (posInBuffer == bufferLength)
			break;
	}
	return posInBuffer;

}	//	GetFileByteStream

size_t CNTV2Bitfile::GetFileByteStream (NTV2_POINTER & outBuffer)
{
	const size_t fileStreamLength(GetFileStreamLength());
	if (!fileStreamLength)
		return 0;
	if (!_fileReady)
		return 0;

	ostringstream oss;
	if (outBuffer.GetByteCount() < fileStreamLength)	//	If buffer IsNULL or too small...
		outBuffer.Allocate(fileStreamLength);			//	...then Allocate or resize it
	if (outBuffer.GetByteCount() < fileStreamLength)	//	Check again
	{	oss << "Buffer size " << DEC(outBuffer.GetByteCount()) << " < " << DEC(fileStreamLength);
		_lastError = oss.str();
		return 0;
	}
	return GetFileByteStream (reinterpret_cast<unsigned char *>(outBuffer.GetHostAddress(0)),
								outBuffer.GetByteCount());
}


void CNTV2Bitfile::SetDesignName (const char * pInBuffer, const size_t bufferLength)
{
	if (!pInBuffer)
		return;
	
	for (unsigned pos (0);  pos < bufferLength;  pos++)
	{
		const char ch (pInBuffer [pos]);
		if ((ch < 'A' || ch > 'Z') && (ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') && ch != '_')
			break;	//	Stop here
		_designName += ch;
	}
}


void CNTV2Bitfile::SetDesignFlags (const char * pInBuffer, const size_t bufferLength)
{
	if (!pInBuffer)
		return;

	const string buffer(pInBuffer, bufferLength);
	
	if (buffer.find("TANDEM=TRUE") != string::npos)
		_tandem = true;
	if (buffer.find("PARTIAL=TRUE") != string::npos)
		_partial = true;
	if (buffer.find("CLEAR=TRUE") != string::npos)
		_clear = true;
	if (buffer.find("COMPRESS=TRUE") != string::npos)
		_compress = true;
}


void CNTV2Bitfile::SetDesignUserID (const char * pInBuffer, const size_t bufferLength)
{
	_userID = 0;
	_designID = 0;
	_designVersion = 0;
	_bitfileID = 0;
	_bitfileVersion = 0;

	if (!pInBuffer)
		return;
		
	string buffer(pInBuffer, size_t(bufferLength));

    ULWord userID = 0xffffffff;

    if (userID == 0xffffffff)
    {
        size_t pos = buffer.find("UserID=0X");
        if ((pos != string::npos) && (pos <= (bufferLength - 17)))
        {
            sscanf(buffer.substr(pos + 9, 8).c_str(), "%x", &userID);
        }
    }
	
    if (userID == 0xffffffff)
    {
        size_t pos = buffer.find("UserID=");
        if ((pos != string::npos) && (pos <= (bufferLength - 15)))
        {
            sscanf(buffer.substr(pos + 7, 8).c_str(), "%x", &userID);
        }
    }

    _userID = userID;
	_designID = GetDesignID(userID);
	_designVersion = GetDesignVersion(userID);
	_bitfileID = GetBitfileID(userID);
	_bitfileVersion = GetBitfileVersion(userID);
}


static string NTV2GetPrimaryHardwareDesignName (const NTV2DeviceID inDeviceID)
{
	switch (inDeviceID)
	{
		case DEVICE_ID_NOTFOUND:		break;
		case DEVICE_ID_CORVID1:			return "corvid1pcie";		//	top.ncd
		case DEVICE_ID_CORVID3G:		return "corvid1_3Gpcie";	//	corvid1_3Gpcie
		case DEVICE_ID_CORVID22:		return "top_c22";			//	top_c22.ncd
		case DEVICE_ID_CORVID24:		return "corvid24_quad";		//	corvid24_quad.ncd
		case DEVICE_ID_CORVID44:		return "corvid_44";			//	corvid_44
		case DEVICE_ID_CORVID88:		return "corvid_88";			//	CORVID88
		case DEVICE_ID_CORVIDHEVC:		return "corvid_hevc";       //	CORVIDHEVC
		case DEVICE_ID_KONA3G:			return "K3G_top";			//	K3G_top.ncd
		case DEVICE_ID_KONA3GQUAD:		return "K3G_quad";			//	K3G_quad.ncd
		case DEVICE_ID_KONA4:			return "kona_4_quad";		//	kona_4_quad
		case DEVICE_ID_KONA4UFC:		return "kona_4_ufc";		//	kona_4_ufc
		case DEVICE_ID_IO4K:			return "IO_XT_4K";			//	IO_XT_4K
		case DEVICE_ID_IO4KUFC:			return "IO_XT_4K_UFC";		//	IO_XT_4K_UFC
		case DEVICE_ID_IOEXPRESS:		return "chekov_00_pcie";	//	chekov_00_pcie.ncd
		case DEVICE_ID_IOXT:			return "top_IO_TX";			//	top_IO_TX.ncd
		case DEVICE_ID_KONALHEPLUS:		return "lhe_12_pcie";		//	lhe_12_pcie.ncd
		case DEVICE_ID_KONALHI:			return "top_pike";			//	top_pike.ncd
		case DEVICE_ID_TTAP:			return "t_tap_top";			//	t_tap_top.ncd
		case DEVICE_ID_CORVIDHBR:		return "corvid_hb_r";		//	corvidhb-r
		case DEVICE_ID_IO4KPLUS:		return "io4kp";
		case DEVICE_ID_IOIP_2022:		return "ioip_s2022";
		case DEVICE_ID_IOIP_2110:		return "ioip_s2110";
		case DEVICE_ID_KONA1:			return "kona1";
		case DEVICE_ID_KONAHDMI:		return "kona_hdmi_4rx";
		case DEVICE_ID_KONA5:			return "kona5_retail";
		case DEVICE_ID_KONA5_8KMK:		return "kona5_8k_mk";
		case DEVICE_ID_KONA5_8K:		return "kona5_8k";
		case DEVICE_ID_KONA5_2X4K:		return "kona5_2x4k";
		case DEVICE_ID_KONA5_3DLUT:		return "kona5_3d_lut";
		case DEVICE_ID_CORVID44_8KMK:	return "c44_12g_8k_mk";
		case DEVICE_ID_CORVID44_8K:		return "c44_12g_8k";
		case DEVICE_ID_CORVID44_2X4K:	return "c44_12g_2x4k";
		case DEVICE_ID_CORVID44_PLNR:	return "c44_12g_plnr";
		case DEVICE_ID_TTAP_PRO:		return "t_tap_pro";
		default:						break;
	}
	return "";
}

bool CNTV2Bitfile::CanFlashDevice (const NTV2DeviceID inDeviceID) const
{
	if (IsPartial () || IsClear ())
		return false;
	
	if (_designName == ::NTV2GetPrimaryHardwareDesignName (inDeviceID))
		return true;

	//	Special cases -- e.g. bitfile flipping, P2P, etc...
	switch (inDeviceID)
	{
		case DEVICE_ID_CORVID44:	return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44) == _designName
											|| _designName == "corvid_446";	//	Corvid 446
		case DEVICE_ID_KONA3GQUAD:	return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA3G) == _designName
											|| _designName == "K3G_quad_p2p";	//	K3G_quad_p2p.ncd
		case DEVICE_ID_KONA3G:		return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA3GQUAD) == _designName
											|| _designName == "K3G_p2p";		//	K3G_p2p.ncd

		case DEVICE_ID_KONA4:		return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA4UFC) == _designName;
		case DEVICE_ID_KONA4UFC:	return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA4) == _designName;

		case DEVICE_ID_IO4K:		return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_IO4KUFC) == _designName;
		case DEVICE_ID_IO4KUFC:		return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_IO4K) == _designName;

		case DEVICE_ID_CORVID88:	return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_CORVID88) == _designName
											|| _designName == "CORVID88"
											|| _designName == "corvid88_top";
		case DEVICE_ID_CORVIDHBR:	return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_CORVIDHBR) == _designName
											|| _designName == "ZARTAN";
		case DEVICE_ID_IO4KPLUS:	return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IO4KPLUS) == _designName;
        case DEVICE_ID_IOIP_2022:	return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2022) == _designName;
        case DEVICE_ID_IOIP_2110:	return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2110) == _designName;
		case DEVICE_ID_KONAHDMI:	return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_KONAHDMI) == _designName
											|| _designName == "Corvid_HDMI_4Rx_Top";
		case DEVICE_ID_KONA5_3DLUT:
		case DEVICE_ID_KONA5_2X4K:
		case DEVICE_ID_KONA5_8KMK:
		case DEVICE_ID_KONA5_8K:
        case DEVICE_ID_KONA5:		return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5) == _designName
                                            || _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8KMK)
											|| _designName == "kona5"
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8K)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_3DLUT)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_2X4K);
		case DEVICE_ID_CORVID44_8KMK:
		case DEVICE_ID_CORVID44_8K:
		case DEVICE_ID_CORVID44_PLNR:
		case DEVICE_ID_CORVID44_2X4K: return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_8KMK) == _designName
											|| _designName == ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_8K)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_2X4K)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_PLNR)
											|| _designName == "c44_12g";
		case DEVICE_ID_TTAP_PRO:		return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_TTAP_PRO) == _designName;
		default:					break;
	}
	return false;
}

typedef map <string, NTV2DeviceID>			DesignNameToIDMap;
typedef DesignNameToIDMap::iterator			DesignNameToIDIter;
typedef DesignNameToIDMap::const_iterator	DesignNameToIDConstIter;	
static DesignNameToIDMap					sDesignNameToIDMap;

class CDesignNameToIDMapMaker
{
	public:

		CDesignNameToIDMapMaker ()
		{
			assert (sDesignNameToIDMap.empty ());
			const NTV2DeviceIDSet goodDeviceIDs (::NTV2GetSupportedDevices ());
			for (NTV2DeviceIDSetConstIter iter (goodDeviceIDs.begin ());  iter != goodDeviceIDs.end ();  ++iter)
				sDesignNameToIDMap[::NTV2GetPrimaryHardwareDesignName (*iter)] = *iter;
			sDesignNameToIDMap["K3G_quad_p2p"] = DEVICE_ID_KONA3GQUAD;	//	special case
			sDesignNameToIDMap["K3G_p2p"] = DEVICE_ID_KONA3G;			//	special case
			sDesignNameToIDMap["CORVID88"] = DEVICE_ID_CORVID88;		//	special case
			sDesignNameToIDMap["ZARTAN"] = DEVICE_ID_CORVIDHBR;			//	special case
		}

		~CDesignNameToIDMapMaker ()
		{
			sDesignNameToIDMap.clear ();
		}

		static NTV2DeviceID DesignNameToID (const string & inDesignName)
		{
			assert (!sDesignNameToIDMap.empty ());
			const DesignNameToIDConstIter	iter	(sDesignNameToIDMap.find (inDesignName));
			return iter != sDesignNameToIDMap.end () ? iter->second : DEVICE_ID_NOTFOUND;
		}
};

static CDesignNameToIDMapMaker				sDesignNameToIDMapMaker;

typedef pair <ULWord, ULWord>				DesignPair;
typedef map <DesignPair, NTV2DeviceID>		DesignPairToIDMap;
typedef DesignPairToIDMap::const_iterator	DesignPairToIDMapConstIter;
static DesignPairToIDMap					sDesignPairToIDMap;

class CDesignPairToIDMapMaker
{
public:

	CDesignPairToIDMapMaker ()
		{
			assert (sDesignPairToIDMap.empty ());
			sDesignPairToIDMap[make_pair(0x01, 0x00)] = DEVICE_ID_KONA5;
			sDesignPairToIDMap[make_pair(0x01, 0x01)] = DEVICE_ID_KONA5_8KMK;
            sDesignPairToIDMap[make_pair(0x01, 0x02)] = DEVICE_ID_KONA5_8K;
            sDesignPairToIDMap[make_pair(0x01, 0x03)] = DEVICE_ID_KONA5_2X4K;
			sDesignPairToIDMap[make_pair(0x01, 0x04)] = DEVICE_ID_KONA5_3DLUT;
			sDesignPairToIDMap[make_pair(0x02, 0x00)] = DEVICE_ID_CORVID44_8KMK;
			sDesignPairToIDMap[make_pair(0x02, 0x01)] = DEVICE_ID_CORVID44_8K;
			sDesignPairToIDMap[make_pair(0x02, 0x02)] = DEVICE_ID_CORVID44_2X4K;
			sDesignPairToIDMap[make_pair(0x02, 0x03)] = DEVICE_ID_CORVID44_PLNR;
        }
	~CDesignPairToIDMapMaker ()
		{
			sDesignPairToIDMap.clear ();
		}
	static NTV2DeviceID DesignPairToID(ULWord designID, ULWord bitfileID)
		{
			assert (!sDesignPairToIDMap.empty ());
			const DesignPairToIDMapConstIter	iter	(sDesignPairToIDMap.find (make_pair(designID, bitfileID)));
			return iter != sDesignPairToIDMap.end () ? iter->second : DEVICE_ID_NOTFOUND;
		}
	static ULWord DeviceIDToDesignID(NTV2DeviceID deviceID)
		{
			assert (!sDesignPairToIDMap.empty ());
			DesignPairToIDMapConstIter iter;

			for (iter = sDesignPairToIDMap.begin(); iter != sDesignPairToIDMap.end(); ++iter)
			{
				if (iter->second == deviceID)
					return iter->first.first;
			}

			return 0;
		}
	static ULWord DeviceIDToBitfileID(NTV2DeviceID deviceID)
		{
			assert (!sDesignPairToIDMap.empty ());
			DesignPairToIDMapConstIter iter;

			for (iter = sDesignPairToIDMap.begin(); iter != sDesignPairToIDMap.end(); ++iter)
			{
				if (iter->second == deviceID)
					return iter->first.second;
			}

			return 0;
		}
};

static CDesignPairToIDMapMaker 				sDesignPairToIDMapMaker;

NTV2DeviceID CNTV2Bitfile::GetDeviceID (void) const
{
	if ((_userID != 0) && (_userID != 0xffffffff))
	{
		return sDesignPairToIDMapMaker.DesignPairToID(_designID, _bitfileID);
	}
	
	return sDesignNameToIDMapMaker.DesignNameToID (_designName);
}

NTV2DeviceID CNTV2Bitfile::ConvertToDeviceID (const ULWord designID, const ULWord bitfileID)
{
	return sDesignPairToIDMapMaker.DesignPairToID (designID, bitfileID);
}

ULWord CNTV2Bitfile::ConvertToDesignID (const NTV2DeviceID deviceID)
{
	return sDesignPairToIDMapMaker.DeviceIDToDesignID(deviceID);
}

ULWord CNTV2Bitfile::ConvertToBitfileID (const NTV2DeviceID deviceID)
{
	return sDesignPairToIDMapMaker.DeviceIDToBitfileID(deviceID);
}

