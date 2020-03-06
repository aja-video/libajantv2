/**
	@file		ntv2bitmanager.h
	@brief		Declares the CNTV2BitManager class that manages Xilinx bitfiles.
	@copyright	(C) 2019 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
**/

#ifndef NTV2BITMANAGER_H
#define NTV2BITMANAGER_H

#include <string>
#include <vector>
#include <fstream>
#ifdef AJALinux
	#include <stdint.h>
	#include <stdlib.h>
#endif
#include "ntv2publicinterface.h"

/**
	@brief	Bitfile information flags.
**/
#define NTV2_BITFILE_FLAG_TANDEM		BIT(0)		///< @brief This is a tandem bitfile
#define NTV2_BITFILE_FLAG_PARTIAL		BIT(1)		///< @brief This is a partial bitfile
#define NTV2_BITFILE_FLAG_CLEAR			BIT(2)		///< @brief This is a clear bitfile


/**
	@brief	Bitfile information.
**/
struct AJAExport NTV2BitfileInfo
{
	std::string bitfilePath;
	std::string designName;
	ULWord designID;
	ULWord designVersion;
	ULWord bitfileID;
	ULWord bitfileVersion;
	ULWord bitfileFlags;
	NTV2DeviceID deviceID;
};

typedef std::vector <NTV2BitfileInfo>		NTV2BitfileInfoList;
typedef NTV2BitfileInfoList::iterator		NTV2BitfileInfoListIter;


/**
	@brief	Instances of me manages bitfiles.
**/
class AJAExport CNTV2BitManager
{
public:
	/**
	   @brief		My constructor.
	**/
	CNTV2BitManager ();

	/**
	   @brief		My destructor.
	**/
	virtual								~CNTV2BitManager ();

	/**
	   @brief		Add the bitfile at the given path to the list of bitfiles.
	   @param[in]	inBitfilePath	Specifies the path name of the bitfile.
	   @return		True if add succeeds; otherwise false.
	**/
	virtual bool						AddFile (const std::string & inBitfilePath);

	/**
	   @brief		Add the bitfile(s) at the given path to the list of bitfiles.
	   @param[in]	inBitfilePath	Specifies the path name of the directory.
	   @return		True if add succeeds; otherwise false.
	**/
	virtual bool						AddDirectory (const std::string & inDirectory);

	/**
	   @brief		Clear the list of bitfiles.
	**/
	virtual void						Clear (void);

	/**
		@brief		Returns the number of bitfiles.
		@return		Number of bitfiles.
	**/
	virtual size_t						GetNumBitfiles (void);

	/**
		@brief	Returns an NTV2BitfileInfoList standard C++ vector.
		@return	A reference to my NTV2BitfileInfoList.
	**/
	virtual NTV2BitfileInfoList &		GetBitfileInfoList (void);

	/**
	   @brief		Get a pointer to the specified bitstream.
	   @param[in]	designID		Specifies the design ID.
	   @param[in]	designVersion	Specifies the design version.
	   @param[in]	bitfileID		Specifies the bitfile ID.
	   @param[in]	bitfileVersion	Specifies the bitfile version (0xff for latest).
	   @param[in]	bitfileFlags	Specifies the bitfile flags.
	   @return		True if the bitfile is present; otherwise false.
	**/
	virtual bool						GetBitStream (NTV2_POINTER & bitstream,
													  ULWord designID,
													  ULWord designVersion,
													  ULWord bitfileID,
													  ULWord bitfileVersion,
													  ULWord bitfileFlags);

private:

	/**
	   @brief		Read the specified bitstream.
	   @param[in]	index		Specifies the index of the bitfile info.
	   @return		True if the bitstream was read; otherwise false.
	**/
	bool ReadBitstream(int index);
		
	typedef std::vector <NTV2_POINTER>		NTV2BitstreamList;
	typedef NTV2BitstreamList::iterator		NTV2BitstreamListIter;

	NTV2BitfileInfoList		_bitfileList;
	NTV2BitstreamList		_bitstreamList;
};


#endif
