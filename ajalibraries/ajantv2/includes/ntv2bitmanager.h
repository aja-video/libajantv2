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
#include "ajatypes.h"
#include "ajaexport.h"
#include "ntv2enums.h"

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
	ULWord flags;
	ULWord deviceID;
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
	virtual NTV2DeviceInfoList &		GetBitfileInfoList (void);

	/**
	   @brief		Get a pointer to the specified tandem bitfile.
	   @param[in]	deviceID		Specifies the device ID of the bitfile.
	   @param[in]	bitfileVersion	Specifies the bitfile version (0xff for latest).
	   @param[in]	designVersion	Specifies the design version of the bitfile (0xff for latest).
	   @return		True if the bitfile is present; otherwise false.
	**/
	virtual bool						GetTandemStream (NTV2_POINTER & bitstream,
														 ULWord deviceID,
														 ULWord bitfileVersion,
														 ULWord designVersion);

	/**
	   @brief		Get a pointer to the specified clear bitfile.
	   @param[in]	deviceID		Specifies the device ID of the bitfile.
	   @param[in]	bitfileVersion	Specifies the bitfile version (0xff for latest).
	   @param[in]	designVersion	Specifies the design version of the bitfile (0xff for latest).
	   @return		True if the bitfile is present; otherwise false.
	**/
	virtual bool						GetClearStream (NTV2_POINTER & bitstream,
														ULWord deviceID,
														ULWord bitfileVersion,
														ULWord designVersion);

	/**
	   @brief		Get a pointer to the specified partial bitfile.
	   @param[in]	deviceID		Specifies the device ID of the bitfile.
	   @param[in]	bitfileVersion	Specifies the bitfile version (0xff for latest).
	   @param[in]	designVersion	Specifies the design version of the bitfile (0xff for latest).
	   @return		True if the bitfile is present; otherwise false.
	**/
	virtual bool						GetPartialStream (NTV2_POINTER & bitstream,
														  ULWord deviceID,
														  ULWord bitfileVersion,
														  ULWord designVersion);

private:

	typedef std::vector <NTV2_POINTER>		NTV2BitstreamList;
	typedef NTV2BitstreamList::iterator		NTV2BitstreamListIter;

	NTV2BitfileInfoList		BitfileList;
	NTV2BitstreamList		BitstreamList;
};
