/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_hdr_hlg.h
	@brief		Declares the AJAAncillaryData_HDR_HLG class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATA_HDMI_AUX_H
#define AJA_ANCILLARYDATA_HDMI_AUX_H

#include "ancillarydatafactory.h"
#include "ancillarydata.h"


/**
	@brief	This class handles HDMI Auxillary packets.
**/
class AJA_EXPORT AJAAncillaryData_HDMI_Aux : public AJAAncillaryData
{
public:
	AJAAncillaryData_HDMI_Aux ();	///< @brief	My default constructor.

	/**
		@brief	My copy constructor.
		@param[in]	inClone	The AJAAncillaryData object to be cloned.
	**/
	AJAAncillaryData_HDMI_Aux (const AJAAncillaryData_HDMI_Aux & inClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInClone	A valid pointer to the AJAAncillaryData object to be cloned.
	**/
	AJAAncillaryData_HDMI_Aux (const AJAAncillaryData_HDMI_Aux * pInClone);

	/**
		@brief	Constructs me from a generic AJAAncillaryData object.
		@param[in]	pInData	A valid pointer to the AJAAncillaryData object.
	**/
	AJAAncillaryData_HDMI_Aux (const AJAAncillaryData * pInData);

	virtual									~AJAAncillaryData_HDMI_Aux ();	///< @brief		My destructor.

	virtual void							Clear (void);								///< @brief	Frees my allocated memory, if any, and resets my members to their default values.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryData_HDMI_Aux &			operator = (const AJAAncillaryData_HDMI_Aux & inRHS);


	virtual inline AJAAncillaryData_HDMI_Aux *	Clone (void) const	{return new AJAAncillaryData_HDMI_Aux (this);}	///< @return	A clone of myself.

	/**
		@brief		Parses out (interprets) the "local" ancillary data from my payload data.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						ParsePayloadData (void);

	/**
		@brief		Streams a human-readable representation of me to the given output stream.
		@param		inOutStream		Specifies the output stream.
		@param[in]	inDetailed		Specify 'true' for a detailed representation;  otherwise use 'false' for a brief one.
		@return		The given output stream.
	**/
	virtual std::ostream &					Print (std::ostream & inOutStream, const bool inDetailed = false) const;

		/**
		@brief		Returns whether or not this is an HDMI Aux InfoFrame Packet
		@return		Returns true if this packet is an InfoFrame Packet, false if not.
	**/
	virtual bool 							isHDMIAuxInfoFrame(void) const;	


protected:
	void		Init (void);	// NOT virtual - called by constructors

};	//	AJAAncillaryData_HDR_HLG

#endif	// AJA_ANCILLARYDATA_HDR_HLG_H
