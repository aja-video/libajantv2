/**
	@file		ntv2bitfile.cpp
	@brief		Implementation of CNTV2Bitfile class.
	@copyright	(C) 2010-2017 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
**/
#include "ntv2bitfile.h"
#include <iostream>
#include "ntv2status.h"
#include <sys/stat.h>
#include <assert.h>
#if defined (AJALinux) || defined (AJAMac)
	#include <arpa/inet.h>
#endif
#include "ntv2utils.h"
#include <map>

using namespace std;


// TODO: Handle compressed bit-files
#define MAX_BITFILEHEADERSIZE 184
#define BITFILE_SYNCWORD_SIZE 6

static const unsigned char SyncWord[ BITFILE_SYNCWORD_SIZE ] = {0xFF,0xFF,0xFF,0xFF,0xAA,0x99};
static const unsigned char Head13[] = { 0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01 };


CNTV2Bitfile::CNTV2Bitfile ()
{
	Init ();	//	Initialize everything
}


void CNTV2Bitfile::Close (void)
{
	if (_fileReady)
		_bitFileStream.close();
	_fileReady = false;
	_bitFileCompressed = false;
	_programStreamPos = 0;
	_fileStreamPos = 0;
	_fileProgrammingPosition = 0;
	_date = _time = _designName = _partName = _lastError = "";
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
	_fileSize = fsinfo.st_size;

	_bitFileStream.open (inBitfileName.c_str (), std::ios::binary);

	do
	{
		unsigned	preambleSize	(0);

		if (_bitFileStream.fail ())
			{_lastError = "Unable to open bitfile";		break;}

		//	Preload bitfile header into mem
		for (int i = 0; i < MAX_BITFILEHEADERSIZE - 1; i++)
			_fileHeader.push_back (_bitFileStream.get ());

		_lastError = ParseHeader (preambleSize);
		if (!_lastError.empty ())
			break;

		if (!_bitFileCompressed)
		{
			//	tellg fails on a pipe and we only get 0xffs afterwards.
			_fileProgrammingPosition = (int) _bitFileStream.tellg ();

			//	Back up to include preamble in download
			//	preamble is all ff's preceding the signature
			_fileProgrammingPosition -= preambleSize;
		}

		_fileReady = true;
	}
	while (false);

	return _fileReady;

}	//	Open

string CNTV2Bitfile::ParseHeaderFromBuffer(const uint8_t* bitfileBuffer)
{
	Close();

	do
	{
		unsigned	preambleSize(0);

		//	Preload bitfile header into mem
		for (int i = 0; i < MAX_BITFILEHEADERSIZE - 1; i++)
			_fileHeader.push_back(bitfileBuffer[i]);

		_lastError = ParseHeader(preambleSize);
		if (!_lastError.empty())
			break;

		_fileReady = false;
	} while (false);

	return _lastError;

}	//	Open


// FindSyncWord()
// Check last BITFILE_SYNCWORD_SIZE bytes in header for signature
// returns true if the signature is found.
bool CNTV2Bitfile::FindSyncWord (void) const
{
	const size_t	headerSize	(static_cast <int> (_fileHeader.size ()));

	if (headerSize < BITFILE_SYNCWORD_SIZE)
		return false;

	//	Brute force check for signature...
	size_t	headerPos	(headerSize - BITFILE_SYNCWORD_SIZE);
	for (size_t ndx (0);  ndx < BITFILE_SYNCWORD_SIZE;  ndx++)
		if (SyncWord [ndx] != _fileHeader [headerPos + ndx])
			return false;

	return true;

}	//	FindSyncWord


//	ParseHeader returns empty string if header looks OK, otherwise string will contain failure explanation
string CNTV2Bitfile::ParseHeader (unsigned & outPreambleSize)
{
	uint32_t		fieldLen	(0);	//	Holds size of various header fields, in bytes
	int				pos			(0);	//	Byte offset during parse
	char			testByte	(0);	//	Used when checking against expected header bytes
	ostringstream	oss;				//	Receives parse error information

	//	This really is bad form -- it precludes the ability to make this method 'const' (which it really is), plus it's safer to use iterators...
	char *	p	(reinterpret_cast <char *> (&_fileHeader [0]));

	outPreambleSize = 0;		//	Non-zero if auto bus sizing preamble is present

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

		fieldLen = htons (*((uint16_t *)p));// the next 2 bytes are the length of the FileName (including /0)

		p += 2;							// now pointing at the beginning of the file name
		pos += 2;

		SetDesignName (p);				// grab design name

		p += fieldLen;					// skip over design name - now pointing to beginning of 'b' field
		pos += fieldLen;


		//	The next byte should be 'b'...
		testByte = *p++;
		if (testByte != 'b')
			{oss << "ParseHeader failed at or near byte offset " << pos << ", expected 'b', instead got '" << testByte << "'";	break;}

		fieldLen = htons (*((uint16_t *)p));// the next 2 bytes are the length of the Part Name (including /0)

		p += 2;							// now pointing at the beginning of the part name
		pos += 2;

		_partName = p;					// grab part name

		p += fieldLen;					// skip over part name - now pointing to beginning of 'c' field
		pos += fieldLen;


		//	The next byte should be 'c'...
		testByte = *p++;
		if (testByte != 'c')
			{oss << "ParseHeader failed at or near byte offset " << pos << ", expected 'c', instead got '" << testByte << "'";	break;}

		fieldLen = htons (*((uint16_t *)p));// the next 2 bytes are the length of the date string (including /0)

		p += 2;							// now pointing at the beginning of the date string
		pos += 2;

		_date = p;						// grab date string

		p += fieldLen;					// skip over date string - now pointing to beginning of 'd' field
		pos += fieldLen;


		//	The next byte should be 'd'...
		testByte = *p++;
		if (testByte != 'd')
			{oss << "ParseHeader failed at or near byte offset " << pos << ", expected 'd', instead got '" << testByte << "'";	break;}

		fieldLen = htons (*((uint16_t *)p));// the next 2 bytes are the length of the time string (including /0)

		p += 2;							// now pointing at the beginning of the time string
		pos += 2;

		_time = p;						// grab time string

		p += fieldLen;					// skip over time string - now pointing to beginning of 'e' field
		pos += fieldLen;


		//	The next byte should be 'e'...
		testByte = *p++;
		if (testByte != 'e')
			{oss << "ParseHeader failed at or near byte offset " << pos << ", expected 'e', instead got '" << testByte << "'";	break;}

		_numBytes = htonl (*((uint32_t *)p));	// the next 4 bytes are the length of the raw program data

		if (_partName[0] == '5' || _partName[0] == '6' || _partName[0] == '7')
		{
			if (_partName[0] == '5' || (_partName[0] == '6' && _partName[1] == 'v'))
			{
				p += 48;
				pos += 48;
			}
			else if (_partName[0] == '7' && _partName[1] == 'k')
			{
				p += 48;
				pos += 48;
			}
			else
			{
				p += 16;
				pos += 16;
			}
		}
		else
		{
			p += 4;						// now pointing at the beginning of the identifier
			pos += 4;
		}

		outPreambleSize = (int32_t) _fileHeader.size () - pos;

		assert (oss.str ().empty ());	//	If we made it this far it must be an OK Header - and no error messages
	} while (false);

	return oss.str ();

}	//	ParseHeader


// Get length of program stream. 
// Return value:
//   Length of program stream
//   Return value < 0 indicates error
unsigned CNTV2Bitfile::GetProgramStreamLength (void) const
{
	if (!_fileReady)
		return unsigned (-1);
	return _numBytes;
}


// Get length of file 
// Return value:
//   Length of file
//   Return value < 0 indicates error
unsigned CNTV2Bitfile::GetFileStreamLength (void) const
{
	if (!_fileReady)
		return unsigned (-1);
	return _fileSize;
}


// Get maximum of bufferLength bytes worth of configuration stream data in buffer.
// Return value:
//   number of bytes of data copied into buffer. This can be <= bufferLength
//   zero means configuration stream has finished.
//   Return value < 0 indicates error
unsigned CNTV2Bitfile::GetProgramByteStream (unsigned char * buffer, unsigned bufferLength)
{
	unsigned	posInBuffer			= 0;
	unsigned	programStreamLength	= GetProgramStreamLength ();

	if (!_fileReady)
		return unsigned (-1);

	_bitFileStream.seekg (_fileProgrammingPosition, std::ios::beg);

	while (_programStreamPos < programStreamLength)
	{
		if (_bitFileStream.eof ())
		{
			//	Unexpected end of file!
			printf ("Unexpected EOF at %d bytes!\n", _programStreamPos);
			return unsigned (-1);
		}
		buffer [posInBuffer++] = _bitFileStream.get ();
		_programStreamPos++;
		if (posInBuffer == bufferLength)
			break;
	}
	return posInBuffer;

}	//	GetProgramByteStream


// Get maximum of fileLength bytes worth of configuration stream data in buffer.
// Return value:
//   number of bytes of data copied into buffer. This can be <= bufferLength
//   zero means configuration stream has finished.
//   Return value < 0 indicates error
unsigned CNTV2Bitfile::GetFileByteStream (unsigned char * buffer, unsigned bufferLength)
{
	unsigned	posInBuffer			(0);
	unsigned	fileStreamLength	(GetFileStreamLength ());

	if (!_fileReady)
		return unsigned (-1);

	_bitFileStream.seekg (0, std::ios::beg);

	while (_fileStreamPos < fileStreamLength)
	{
		if (_bitFileStream.eof ())
		{
			//	Unexpected end of file!
			ostringstream	oss;	oss << "Unexpected EOF at " << _programStreamPos << "bytes";	_lastError = oss.str ();
			return unsigned (-1);
		}

		buffer [posInBuffer++] = _bitFileStream.get ();
		_programStreamPos++;
		if (posInBuffer == bufferLength)
			break;
	}
	return posInBuffer;

}	//	GetFileByteStream


void CNTV2Bitfile::SetDesignName (const char * pInBuffer)
{
	if (pInBuffer)
		for (unsigned pos (0);  pos < ::strlen (pInBuffer);  pos++)
		{
			const char ch (pInBuffer [pos]);
			if ((ch < 'A' || ch > 'Z') && (ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') && ch != '_')
				break;	//	Stop here
			_designName += ch;
		}
}


static string NTV2GetPrimaryHardwareDesignName (const NTV2DeviceID inBoardID)
{
	switch (inBoardID)
	{
		#if !defined (NTV2_DEPRECATE)
			case BOARD_ID_XENA_SD:
			case BOARD_ID_XENA_SD22:
			case BOARD_ID_XENA_HD:
			case BOARD_ID_XENA_HD22:
			case BOARD_ID_HDNTV2:
			case BOARD_ID_KSD11:
			//case BOARD_ID_XENA_SD_MM:
			case BOARD_ID_KSD22:
			//case BOARD_ID_XENA_SD22_MM:
			case BOARD_ID_KHD11:
			//case BOARD_ID_XENA_HD_MM:
			case BOARD_ID_XENA_HD22_MM:
			case BOARD_ID_HDNTV2_MM:
			case BOARD_ID_KONA_SD:
			case BOARD_ID_KONA_HD:
			case BOARD_ID_KONA_HD2:
			case BOARD_ID_KONAR:
			case BOARD_ID_KONAR_MM:
			case BOARD_ID_KONA2:
			case BOARD_ID_HDNTV:
			case BOARD_ID_KONALS:
			//case BOARD_ID_XENALS:
			case BOARD_ID_KONAHDS:
			//case BOARD_ID_KONALH:
			//case BOARD_ID_XENALH:
			case BOARD_ID_XENADXT:
			//case BOARD_ID_XENAHS:
			case BOARD_ID_KONAX:
			case BOARD_ID_XENAX:
			case BOARD_ID_XENAHS2:
			case BOARD_ID_FS1:
			case BOARD_ID_FS2:
			case BOARD_ID_MOAB:
			case BOARD_ID_XENAX2:
			case BOARD_ID_BORG:
			case BOARD_ID_BONES:
			case BOARD_ID_BARCLAY:
			case BOARD_ID_KIPRO_QUAD:
			case BOARD_ID_KIPRO_SPARE1:
			case BOARD_ID_KIPRO_SPARE2:
			case BOARD_ID_FORGE:
			case BOARD_ID_XENA2:
			//case BOARD_ID_KONA3:
			case BOARD_ID_LHI_DVI:
			case BOARD_ID_LHI_T:
		#endif	//	!defined (NTV2_DEPRECATE)
        case DEVICE_ID_NOTFOUND:        break;
        case DEVICE_ID_CORVID1:         return "corvid1pcie";		//	top.ncd
        case DEVICE_ID_CORVID3G:        return "corvid1_3Gpcie";	//	corvid1_3Gpcie
        case DEVICE_ID_CORVID22:        return "top_c22";			//	top_c22.ncd
        case DEVICE_ID_CORVID24:        return "corvid24_quad";		//	corvid24_quad.ncd
        case DEVICE_ID_CORVID44:        return "corvid_44";			//	corvid_44
        case DEVICE_ID_CORVID88:        return "corvid_88";			//	CORVID88
        case DEVICE_ID_CORVIDHEVC:      return "corvid_hevc";       //	CORVIDHEVC
        case DEVICE_ID_KONA3G:          return "K3G_top";			//	K3G_top.ncd
        //case DEVICE_ID_KONA3G:        return "K3G_p2p";			//	K3G_p2p.ncd
        case DEVICE_ID_KONA3GQUAD:      return "K3G_quad";			//	K3G_quad.ncd
        //case DEVICE_ID_KONA3GQUAD:    return "K3G_quad_p2p";		//	K3G_quad_p2p.ncd
        case DEVICE_ID_KONA4:           return "kona_4_quad";		//	kona_4_quad
        case DEVICE_ID_KONA4UFC:        return "kona_4_ufc";		//	kona_4_ufc
        case DEVICE_ID_IO4K:            return "IO_XT_4K";			//	IO_XT_4K
        case DEVICE_ID_IO4KUFC:         return "IO_XT_4K_UFC";		//	IO_XT_4K_UFC
        case DEVICE_ID_IOEXPRESS:       return "chekov_00_pcie";	//	chekov_00_pcie.ncd
        case DEVICE_ID_IOXT:            return "top_IO_TX";			//	top_IO_TX.ncd
        case DEVICE_ID_LHE_PLUS:        return "lhe_12_pcie";		//	lhe_12_pcie.ncd
        case DEVICE_ID_LHI:             return "top_pike";			//	top_pike.ncd
        case DEVICE_ID_TTAP:            return "t_tap_top";			//	t_tap_top.ncd
        case DEVICE_ID_CORVIDHBR:       return "corvid_hb_r";		//	corvidhb-r
        case DEVICE_ID_IO4KPLUS:        return "io4kp";
        case DEVICE_ID_IOIP_2022:       return "ioip_s2022";
        case DEVICE_ID_IOIP_2110:       return "ioip_s2110";
        default:
			break;
	}
	return "";
}


bool CNTV2Bitfile::CanFlashDevice (const NTV2DeviceID inDeviceID) const
{
	if (_designName == ::NTV2GetPrimaryHardwareDesignName (inDeviceID))
		return true;

	//	Special cases -- e.g. bitfile flipping, P2P, etc...
	switch (inDeviceID)
	{
        case DEVICE_ID_CORVID44:        return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44) == _designName
											|| _designName == "corvid_446";	//	Corvid 446
        case DEVICE_ID_KONA3GQUAD:      return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA3G) == _designName
											|| _designName == "K3G_quad_p2p";	//	K3G_quad_p2p.ncd
        case DEVICE_ID_KONA3G:          return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA3GQUAD) == _designName
											|| _designName == "K3G_p2p";		//	K3G_p2p.ncd

        case DEVICE_ID_KONA4:           return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA4UFC) == _designName;
        case DEVICE_ID_KONA4UFC:        return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA4) == _designName;

        case DEVICE_ID_IO4K:            return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_IO4KUFC) == _designName;
        case DEVICE_ID_IO4KUFC:         return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_IO4K) == _designName;

        case DEVICE_ID_CORVID88:        return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_CORVID88) == _designName
											|| _designName == "CORVID88"
											|| _designName == "corvid88_top";
        case DEVICE_ID_CORVIDHBR:       return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_CORVIDHBR) == _designName
											|| _designName == "ZARTAN";
		case DEVICE_ID_IO4KPLUS:		return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IO4KPLUS) == _designName;
        case DEVICE_ID_IOIP_2022:		return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2022) == _designName;
        case DEVICE_ID_IOIP_2110:		return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2110) == _designName;
        default:                        break;
	}
	return false;
}


typedef map <string, NTV2DeviceID>		DesignNameToID;
typedef pair <string, NTV2DeviceID>		DesignNameToIDPair;
typedef DesignNameToID::iterator		DesignNameToIDIter;
typedef DesignNameToID::const_iterator	DesignNameToIDConstIter;
static	DesignNameToID					sDesignNamesToIDs;

class CDesignNameToDeviceIDMapMaker
{
	public:
		CDesignNameToDeviceIDMapMaker ()
		{
			assert (sDesignNamesToIDs.empty ());
			const NTV2DeviceIDSet goodDeviceIDs (::NTV2GetSupportedDevices ());
			for (NTV2DeviceIDSetConstIter iter (goodDeviceIDs.begin ());  iter != goodDeviceIDs.end ();  ++iter)
				sDesignNamesToIDs.insert (DesignNameToIDPair (::NTV2GetPrimaryHardwareDesignName (*iter), *iter));
			sDesignNamesToIDs.insert (DesignNameToIDPair ("K3G_quad_p2p", DEVICE_ID_KONA3GQUAD));	//	special case
			sDesignNamesToIDs.insert (DesignNameToIDPair ("K3G_p2p", DEVICE_ID_KONA3G));			//	special case
			sDesignNamesToIDs.insert (DesignNameToIDPair ("CORVID88", DEVICE_ID_CORVID88));			//	special case
			sDesignNamesToIDs.insert (DesignNameToIDPair ("ZARTAN", DEVICE_ID_CORVIDHBR));			//	special case
		}
		virtual ~CDesignNameToDeviceIDMapMaker ()
		{
			sDesignNamesToIDs.clear ();
		}
		static NTV2DeviceID DesignNameToDeviceID (const string & inDesignName)
		{
			assert (!sDesignNamesToIDs.empty ());
			const DesignNameToIDConstIter	iter	(sDesignNamesToIDs.find (inDesignName));
			return iter != sDesignNamesToIDs.end () ? iter->second : DEVICE_ID_NOTFOUND;
		}
};

static CDesignNameToDeviceIDMapMaker	sDesignNameToIDMapMaker;


NTV2DeviceID CNTV2Bitfile::GetDeviceID (void) const
{
	return sDesignNameToIDMapMaker.DesignNameToDeviceID (GetDesignName ());
}


CNTV2Bitfile::~CNTV2Bitfile ()
{
	Close ();
}
