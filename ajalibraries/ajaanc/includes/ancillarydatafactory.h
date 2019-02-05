/**
	@file		ancillarydatafactory.h
	@brief		Declaration of the AJAAncillaryDataFactory class.
	@copyright	(C) 2010-2019 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef AJA_ANCILLARYDATAFACTORY_H
#define AJA_ANCILLARYDATAFACTORY_H

#include "ancillarydata.h"


/**
	@brief	Use my GuessAncillaryDataType method to determine what kind of ancillary data is being held by
			a (generic) AJAAncillaryData object. Use my Create method to instantiate a new AJAAncillaryData
			object specific to a given type.
**/
class AJAExport AJAAncillaryDataFactory
{
public:

	AJAAncillaryDataFactory()	{}

	virtual ~AJAAncillaryDataFactory()	{}

	/**
		@brief	Creates a new AJAAncillaryData object having a particular subtype.
		@param[in]	ancType		Type of AJAAncillaryData object (subclass) to instantiate.
		@param[in]	pAncData	Optionally supplies an existing AJAAncillaryData object to clone from.
		@return		A pointer to the new instance.
	**/
	static AJAAncillaryData *Create(AJAAncillaryDataType ancType, AJAAncillaryData *pAncData = NULL);


	/**
		@brief		From the raw ancillary data in a base class AJAAncillaryData object, try to guess
					what kind of derived class it should be.
		@param[in]	pAncData	A valid, non-NULL pointer to an AJAAncillaryData object that contains
								"raw" packet data.
		@return		The guessed AJAAncillaryDataType (or AJAAncillaryDataType_Unknown if no idea...).
	**/
	static AJAAncillaryDataType GuessAncillaryDataType(AJAAncillaryData *pAncData);

};

#endif	// AJA_ANCILLARYDATAFACTORY_H
