/**
	@file		ntv2caption708service.h
	@brief		Declares the CNTV2Caption708Service class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA708_SERVICE_
#define __NTV2_CEA708_SERVICE_

#include "ntv2caption708serviceblockqueue.h"
#include "ntv2caption708window.h"
#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif



class AJAExport CNTV2Caption708Service : public CNTV2CaptionLogConfig
{
	//	INSTANCE METHODS
	public:
						CNTV2Caption708Service ();
		virtual			~CNTV2Caption708Service ();

		virtual void	InitService (const int inServiceIndex);
		virtual bool	SetServiceInfo (const NTV2_CC708ServiceInfo & inNewSvcInfo);

		/**
			@brief		Parses the given Service Block into elementary commands and enqueues them as independent Service Blocks.
			@param[in]	pInData		Specifies a valid, non-NULL address of the first Data Word of Service Block (NOT the Service Block header!).
			@param[in]	inByteCount	Specifies the number of data bytes in the Service Block.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	ParseInputServiceBlockToLocalQueue (const UByte * pInData, const size_t inByteCount);

		/**
			@brief		Answers with the size of the 708 command that starts at the first byte in the given buffer.
			@param[in]	pInData		Specifies the valid, non-NULL address of the command buffer.
			@param[in]	inByteCount	Specifies the number of valid data bytes in the buffer.
			@return		The size of the command, in bytes, if parsed correctly; otherwise zero.
		**/
		virtual size_t	GetCommandSize (const UByte * pInData, const size_t inByteCount) const;

		/**
			@brief		Parses the 708 command that starts at the first byte in the given buffer.
			@param[in]	pInData		Specifies the valid, non-NULL address of the command buffer.
			@param[in]	inByteCount	Specifies the number of valid data bytes in the buffer.
			@return		The size of the command, in bytes, if parsed correctly; otherwise zero.
		**/
		virtual size_t	Parse708Command (const UByte * pInData, const size_t inByteCount);

		virtual size_t	DebugParse708Command (const UByte * pInData, const size_t inByteCount) const;

		/**
			@brief		Specifies my current window ID, replacing the former value. See CEA-708-D section 8.10.5.1.
						Subsequent calls to SetWindowAttributes, SetPenAttributes, SetPenLocation, etc. will affect
						this window.
			@param[in]	inWindowID	Specifies the window that is to become the current one. Must be 0 thru 7.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	SetCurrentWindow (const int inWindowID);

		/**
			@brief		Clears text from the given set of windows. See CEA-708-D section 8.10.5.3.
			@param[in]	inWindowMap		Specifies the set of windows to be affected, where each bit of the given
										8-bit value corresponds to a window ID (0 thru 7). If the map's bit is set,
										the window having the ID corresponding to that bit will be affected.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	ClearWindows (const UByte inWindowMap);

		/**
			@brief		Deletes the window definitions for the given set of windows. See CEA-708-D section 8.10.5.4.
			@param[in]	inWindowMap		Specifies the set of windows to be affected, where each bit of the given
										8-bit value corresponds to a window ID (0 thru 7). If the map's bit is set,
										the window having the ID corresponding to that bit will be affected.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	DeleteWindows (const UByte inWindowMap);

		/**
			@brief		Shows (unhides) the given set of windows. See CEA-708-D section 8.10.5.5.
			@param[in]	inWindowMap		Specifies the set of windows to be affected, where each bit of the given
										8-bit value corresponds to a window ID (0 thru 7). If the map's bit is set,
										the window having the ID corresponding to that bit will be affected.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	DisplayWindows (const UByte inWindowMap);

		/**
			@brief		Hides the given set of windows. See CEA-708-D section 8.10.5.6.
			@param[in]	inWindowMap		Specifies the set of windows to be affected, where each bit of the given
										8-bit value corresponds to a window ID (0 thru 7). If the map's bit is set,
										the window having the ID corresponding to that bit will be affected.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	HideWindows (const UByte inWindowMap);

		/**
			@brief		Toggles the display/hide status for the given set of windows. See CEA-708C section 8.10.5.7.
			@param[in]	inWindowMap		Specifies the set of windows to be affected, where each bit of the given
										8-bit value corresponds to a window ID (0 thru 7). If the map's bit is set,
										the window having the ID corresponding to that bit will be affected.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	ToggleWindows (const UByte inWindowMap);

		/**
			@brief		Defines a new window with the given ID and initial parameters, or updates the existing window's
						parameters. In either case, the given window becomes the new "current" window.
						See CEA-708-D section 8.10.5.2.
			@param[in]	inWindowID		Specifies the new window's ID.
			@param[in]	inParameters	Specifies the new window's initial parameters.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	DefineWindow (const int inWindowID, const CC708WindowParms & inParameters);

		/**
			@brief		Specifies new attributes for my current window. See CEA-708-D section 8.10.5.8.
			@param[in]	inAttributes	Specifies the window attributes to use.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	SetWindowAttributes (const CC708WindowAttr & inAttributes);

		/**
			@brief		Specifies new pen attributes for my current window. See CEA-708-D section 8.10.5.9.
			@param[in]	inAttributes	Specifies the pen attributes to use.
		**/
		virtual void	SetPenAttributes (const CC708PenAttr & inAttributes);

		/**
			@brief		Specifies a new pen color for my current window. See CEA-708-D section 8.10.5.10.
			@param[in]	inColor		Specifies the pen color to use.
		**/
		virtual void	SetPenColor (const CC708PenColor & inColor);

		/**
			@brief		Specifies a new pen location for my current window. See CEA-708-D section 8.10.5.11.
			@param[in]	inLocation	Specifies the pen location to use.
		**/
		virtual void	SetPenLocation (const CC708PenLocation & inLocation);

		/**
			@brief		Delays service data interpretation. See CEA-708-D Section 8.10.5.12.
			@todo		Currently not implemented.
		**/
		virtual void	Delay (const int inTenthsSec);

		/**
			@brief		Cancels an Active Delay Command. See CEA-708-D section 8.10.5.13.
			@todo		Currently not implemented.
		**/
		virtual void	DelayCancel (void);

		/**
			@brief		Resets the Caption Channel Service. See CEA-708-D section 8.10.5.14.
		**/
		virtual void	Reset (void);

		virtual void	AddCharacter (const UByte inChar, const CC708CodeGroup inCodeGroup);
		virtual void	DoETX (void);
		virtual void	DoBS (void);
		virtual void	DoFF (void);
		virtual void	DoCR (void);
		virtual void	DoHCR (void);


		virtual bool	PeekNextServiceBlockInfo (size_t & outBlockSize, size_t & outDataSize, int & outServiceNum, bool & outIsExtended) const;
		virtual size_t	PopServiceBlock (std::vector<UByte> & outData);
		virtual size_t	PopServiceBlockData (std::vector<UByte> & outData);
		virtual size_t	PopServiceBlock (UByte * pData);
		virtual size_t	PopServiceBlockData (UByte * pData);

		//	Debug Methods
		virtual NTV2CaptionLogMask	SetLogMask (const NTV2CaptionLogMask inLogMask);


	//	INSTANCE DATA
	private:
		CNTV2Caption708Window				mWindowArray [NTV2_CC708NumWindows];	///< @brief	Array of windows for this service
		NTV2_CC708ServiceInfo				mServiceInfo;							///< @brief	A copy of the current service_info for this service
		CNTV2Caption708ServiceBlockQueue	mServiceBlockQueue;						///< @brief	Queue for inbound 708 Service Blocks
		int									mCurrentWindow;							///< @brief	Window index into which we are currently pouring new data (characters)
};	//	CNTV2Caption708Service

#endif	// __NTV2_CEA708_SERVICE_
