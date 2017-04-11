/**
	@file		ntv2registerexpert.cpp
	@brief		Implements the CNTV2RegisterExpert class.
	@copyright	(C) 2016-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#include "ntv2registerexpert.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2debug.h"
#if defined(AJALinux)
	#include <string.h>  // For memset
	#include <stdint.h>
#endif
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>


using namespace std;


#if defined (NTV2_DEPRECATE)
	#define	AJA_LOCAL_STATIC	static
#else	//	!defined (NTV2_DEPRECATE)
	#define	AJA_LOCAL_STATIC
#endif	//	!defined (NTV2_DEPRECATE)


#define	DefineRegName(__rn__,__cstr__)			do																								\
												{																								\
													const string __regName__ (__cstr__);														\
													if (!__regName__.empty())																	\
														if (mRegNumToStringMap.find ((__rn__)) == mRegNumToStringMap.end())						\
														{																						\
															mRegNumToStringMap.insert (RegNumToStringPair ((__rn__), __regName__));				\
															mStringToRegNumMap.insert (StringToRegNumPair (ToLower (__regName__), (__rn__)));	\
														}																						\
												} while (false)

#define	DefineRegDecoder(__rn__,__dec__)		mRegNumToDecoderMap.insert (RegNumToDecoderPair ((__rn__), &(__dec__)))

#define	DefineRegClass(__rn__,__str__)			do																						\
												{																						\
													if (!(__str__).empty())																\
														mRegClassToRegNumMap.insert (StringToRegNumPair ((__str__), (__rn__)));		\
												} while (false)

#define DefineRegReadWrite(__rn__,__rorw__)		do														\
												{														\
													if ((__rorw__) == READONLY)							\
													{													\
														NTV2_ASSERT (!IsRegisterWriteOnly(__rn__));		\
														DefineRegClass ((__rn__), kRegClass_ReadOnly);	\
													}													\
													if ((__rorw__) == WRITEONLY)						\
													{													\
														NTV2_ASSERT (!IsRegisterReadOnly(__rn__));		\
														DefineRegClass ((__rn__), kRegClass_WriteOnly);	\
													}													\
												} while (false)

#define	DefineRegister(__rnum__,__rname__,__decoder__,__rorw__,__c1__, __c2__, __c3__)	do													\
																						{													\
																							DefineRegName ((__rnum__), (__rname__));		\
																							DefineRegDecoder ((__rnum__), (__decoder__));	\
																							DefineRegReadWrite ((__rnum__), (__rorw__));	\
																							DefineRegClass ((__rnum__), (__c1__));			\
																							DefineRegClass ((__rnum__), (__c2__));			\
																							DefineRegClass ((__rnum__), (__c3__));			\
																						} while (false)

#define	DefineXptReg(__rn__,__xpt0__,__xpt1__,__xpt2__,__xpt3__)	do																															\
																	{																															\
																		DefineRegister ((__rn__),	"",	mDecodeXptGroupReg,	READWRITE,	kRegClass_Routing,	kRegClass_NULL,	kRegClass_NULL);	\
																		const NTV2InputCrosspointID	indexes	[4]	= {(__xpt0__), (__xpt1__), (__xpt2__), (__xpt3__)};								\
																		for (int ndx(0);  ndx < 4;  ndx++)																						\
																		{																														\
																			if (indexes[ndx] == NTV2_INPUT_CROSSPOINT_INVALID)																	\
																				continue;																										\
																			const XptRegNumAndMaskIndex regNumAndNdx ((__rn__), ndx);															\
																			if (mXptRegNumMaskIndex2InputXptMap.find (regNumAndNdx) == mXptRegNumMaskIndex2InputXptMap.end())					\
																				mXptRegNumMaskIndex2InputXptMap [regNumAndNdx] = indexes[ndx];													\
																			if (mInputXpt2XptRegNumMaskIndexMap.find (indexes[ndx]) == mInputXpt2XptRegNumMaskIndexMap.end())					\
																				mInputXpt2XptRegNumMaskIndexMap[indexes[ndx]] = regNumAndNdx;													\
																		}																														\
																	} while (false)

static const string	gChlClasses[8]	=	{	kRegClass_Channel1,	kRegClass_Channel2,	kRegClass_Channel3,	kRegClass_Channel4,
											kRegClass_Channel5,	kRegClass_Channel6,	kRegClass_Channel7,	kRegClass_Channel8	};

class RegisterExpert
{
	public:
		explicit RegisterExpert ()
		{
			for (ULWord regNum (0);  regNum < kRegNumRegisters;  regNum++)
				DefineRegName (regNum,	::NTV2RegisterNameString (regNum));

			DefineRegister (kRegGlobalControl,		"",	mDecodeGlobalControlReg,	READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister (kRegGlobalControlCh2,	"",	mDecodeGlobalControl2,		READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister (kRegGlobalControlCh3,	"",	mDecodeGlobalControl2,		READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister (kRegGlobalControlCh4,	"",	mDecodeGlobalControl2,		READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister (kRegGlobalControlCh5,	"",	mDecodeGlobalControl2,		READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister (kRegGlobalControlCh6,	"",	mDecodeGlobalControl2,		READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister (kRegGlobalControlCh7,	"",	mDecodeGlobalControl2,		READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister (kRegGlobalControlCh8,	"",	mDecodeGlobalControl2,		READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister (kRegCh1Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister (kRegCh2Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister (kRegCh3Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister (kRegCh4Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister (kRegCh5Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister (kRegCh6Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister (kRegCh7Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister (kRegCh8Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister (kRegCh1PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister (kRegCh2PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister (kRegCh3PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister (kRegCh4PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister (kRegCh5PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister (kRegCh6PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister (kRegCh7PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister (kRegCh8PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister (kRegCh1InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister (kRegCh2InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister (kRegCh3InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister (kRegCh4InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister (kRegCh5InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister (kRegCh6InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister (kRegCh7InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister (kRegCh8InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister (kRegCh1OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister (kRegCh2OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister (kRegCh3OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister (kRegCh4OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister (kRegCh5OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister (kRegCh6OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister (kRegCh7OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister (kRegCh8OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister (kRegSDIOut1Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister (kRegSDIOut2Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister (kRegSDIOut3Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister (kRegSDIOut4Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister (kRegSDIOut5Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister (kRegSDIOut6Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister (kRegSDIOut7Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister (kRegSDIOut8Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel8,	kRegClass_NULL);

			DefineRegister (kRegSysmonVccIntDieTemp,"",	mDecodeSysmonVccIntDieTemp,	READONLY,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);

			//	Anc Ins/Ext
			SetupAncInsExt();

			//	Xpt Select
			SetupXptSelect();

			//	DMA
			SetupDMARegs();

			//	Timecode
			SetupTimecodeRegs();

			//	Audio
			SetupAudioRegs();

			//	VidProc/Mixer/Keyer
			DefineRegister	(kRegVidProc1Control,	"",	mVidProcControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel1,	kRegClass_Channel2);
			DefineRegister	(kRegVidProc2Control,	"",	mVidProcControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel3,	kRegClass_Channel4);
			DefineRegister	(kRegVidProc3Control,	"",	mVidProcControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel5,	kRegClass_Channel6);
			DefineRegister	(kRegVidProc4Control,	"",	mVidProcControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel7,	kRegClass_Channel8);
			DefineRegister	(kRegSplitControl,		"",	mSplitControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister	(kRegFlatMatteValue,	"",	mFlatMatteValueRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel1,	kRegClass_NULL);

			//	HDMI
			SetupHDMIRegs();

			SetupSDIError();

			//	Virtuals
			SetupVRegs();

			//Print (cout);	//	For debugging
		}	//	constructor

private:
		void SetupTimecodeRegs(void)
		{
			DefineRegister	(kRegRP188InOut1DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut1Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut1Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut2DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut2Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut2Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister	(kRegLTCOutBits0_31,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_Output);
			DefineRegister	(kRegLTCOutBits32_63,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_Output);
			DefineRegister	(kRegLTCInBits0_31,				"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_Input);
			DefineRegister	(kRegLTCInBits32_63,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_Input);
			DefineRegister	(kRegRP188InOut1Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut1Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut2Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut2Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut3Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut3Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut4Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut4Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut5Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut5Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut6Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut6Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut7Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut7Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut8Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut8Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister	(kRegLTCStatusControl,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_NULL,		kRegClass_NULL);
			DefineRegister	(kRegLTC2EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister	(kRegLTC2EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister	(kRegLTC2AnalogBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister	(kRegLTC2AnalogBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut3DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut3Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut3Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut4DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut4Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut4Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister	(kRegLTC3EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister	(kRegLTC3EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister	(kRegLTC4EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister	(kRegLTC4EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut5Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut5Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut5DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister	(kRegLTC5EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister	(kRegLTC5EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut6Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut6Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut6DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister	(kRegLTC6EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister	(kRegLTC6EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut7Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut7Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut7DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister	(kRegLTC7EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister	(kRegLTC7EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut8Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut8Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister	(kRegRP188InOut8DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister	(kRegLTC8EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister	(kRegLTC8EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
		}	//	SetupTimecodeRegs

		void SetupAudioRegs(void)
		{
			DefineRegister (kRegAud1Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister (kRegAud2Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister (kRegAud3Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister (kRegAud4Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister (kRegAud5Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister (kRegAud6Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister (kRegAud7Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister (kRegAud8Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister (kRegAud1Detect,			"",	mDecodeAudDetectReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_Channel2);
			DefineRegister (kRegAudDetect2,			"",	mDecodeAudDetectReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_Channel4);
			DefineRegister (kRegAudioDetect5678,	"",	mDecodeAudDetectReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister (kRegAud1SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister (kRegAud2SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister (kRegAud3SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister (kRegAud4SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister (kRegAud5SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister (kRegAud6SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister (kRegAud7SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister (kRegAud8SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister (kRegAud1Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_NULL);
			DefineRegister (kRegAud2Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_NULL);
			DefineRegister (kRegAud3Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_NULL);
			DefineRegister (kRegAud4Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_NULL);
			DefineRegister (kRegAud5Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_NULL);
			DefineRegister (kRegAud6Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_NULL);
			DefineRegister (kRegAud7Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_NULL);
			DefineRegister (kRegAud8Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_NULL);
			DefineRegister (kRegAud1OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_Output);
			DefineRegister (kRegAud2OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_Output);
			DefineRegister (kRegAud3OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_Output);
			DefineRegister (kRegAud4OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_Output);
			DefineRegister (kRegAud5OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_Output);
			DefineRegister (kRegAud6OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_Output);
			DefineRegister (kRegAud7OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_Output);
			DefineRegister (kRegAud8OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_Output);
			DefineRegister (kRegAud1InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_Input);
			DefineRegister (kRegAud2InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_Input);
			DefineRegister (kRegAud3InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_Input);
			DefineRegister (kRegAud4InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_Input);
			DefineRegister (kRegAud5InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_Input);
			DefineRegister (kRegAud6InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_Input);
			DefineRegister (kRegAud7InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_Input);
			DefineRegister (kRegAud8InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_Input);
			DefineRegister (kRegPCMControl4321,		"",	mDecodePCMControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_Channel2);
			DefineRegister (kRegPCMControl8765,		"",	mDecodePCMControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_Channel6);
			DefineRegClass (kRegPCMControl4321, kRegClass_Channel3);	DefineRegClass (kRegPCMControl4321, kRegClass_Channel4);
			DefineRegClass (kRegPCMControl8765, kRegClass_Channel7);	DefineRegClass (kRegPCMControl8765, kRegClass_Channel8);
			DefineRegister (kRegAud1Counter,		"",	mDefaultRegDecoder,			READONLY,	kRegClass_Audio,	kRegClass_NULL,		kRegClass_NULL);
		}

		void SetupDMARegs(void)
		{
			DefineRegister	(kRegDMA1HostAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA1HostAddrHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA1LocalAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA1XferCount,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA1NextDesc,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA1NextDescHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA2HostAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA2HostAddrHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA2LocalAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA2XferCount,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA2NextDesc,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA2NextDescHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA3HostAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA3HostAddrHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA3LocalAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA3XferCount,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA3NextDesc,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA3NextDescHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA4HostAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA4HostAddrHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA4LocalAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA4XferCount,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA4NextDesc,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMA4NextDescHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMAControl,		"",	mDMAControlRegDecoder,		READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister	(kRegDMAIntControl,		"",	mDMAIntControlRegDecoder,	READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		}

		void SetupXptSelect(void)
		{
			//				RegNum					0								1								2								3
			DefineXptReg	(kRegXptSelectGroup1,	NTV2_XptLUT1Input,				NTV2_XptCSC1VidInput,			NTV2_XptConversionModInput,		NTV2_XptCompressionModInput);
			DefineXptReg	(kRegXptSelectGroup2,	NTV2_XptFrameBuffer1Input,		NTV2_XptFrameSync1Input,		NTV2_XptFrameSync2Input,		NTV2_XptDualLinkOut1Input);
			DefineXptReg	(kRegXptSelectGroup3,	NTV2_XptAnalogOutInput,			NTV2_XptSDIOut1Input,			NTV2_XptSDIOut2Input,			NTV2_XptCSC1KeyInput);
			DefineXptReg	(kRegXptSelectGroup4,	NTV2_XptMixer1FGVidInput,		NTV2_XptMixer1FGKeyInput,		NTV2_XptMixer1BGVidInput,		NTV2_XptMixer1BGKeyInput);
			DefineXptReg	(kRegXptSelectGroup5,	NTV2_XptFrameBuffer2Input,		NTV2_XptLUT2Input,				NTV2_XptCSC2VidInput,			NTV2_XptCSC2KeyInput);
			DefineXptReg	(kRegXptSelectGroup6,	NTV2_XptWaterMarker1Input,		NTV2_XptIICT1Input,				NTV2_XptHDMIOutInput,			NTV2_XptConversionMod2Input);
			{	//	An additional input Xpt for kRegXptSelectGroup6 in mask index 2...
				const XptRegNumAndMaskIndex regNumAndNdx (kRegXptSelectGroup6, 2);
				if (mXptRegNumMaskIndex2InputXptMap.find (regNumAndNdx) == mXptRegNumMaskIndex2InputXptMap.end())
					mXptRegNumMaskIndex2InputXptMap [regNumAndNdx] = NTV2_XptHDMIOutQ1Input;
				if (mInputXpt2XptRegNumMaskIndexMap.find (NTV2_XptHDMIOutQ1Input) == mInputXpt2XptRegNumMaskIndexMap.end())
					mInputXpt2XptRegNumMaskIndexMap[NTV2_XptHDMIOutQ1Input] = regNumAndNdx;
			}
			DefineXptReg	(kRegXptSelectGroup7,	NTV2_XptWaterMarker2Input,		NTV2_XptIICT2Input,				NTV2_XptDualLinkOut2Input,		NTV2_INPUT_CROSSPOINT_INVALID);
			DefineXptReg	(kRegXptSelectGroup8,	NTV2_XptSDIOut3Input,			NTV2_XptSDIOut4Input,			NTV2_XptSDIOut5Input,			NTV2_INPUT_CROSSPOINT_INVALID);
			DefineXptReg	(kRegXptSelectGroup9,	NTV2_XptMixer2FGVidInput,		NTV2_XptMixer2FGKeyInput,		NTV2_XptMixer2BGVidInput,		NTV2_XptMixer2BGKeyInput);
			DefineXptReg	(kRegXptSelectGroup10,	NTV2_XptSDIOut1InputDS2,		NTV2_XptSDIOut2InputDS2,		NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_INPUT_CROSSPOINT_INVALID);
			DefineXptReg	(kRegXptSelectGroup11,	NTV2_XptDualLinkIn1Input,		NTV2_XptDualLinkIn1DSInput,		NTV2_XptDualLinkIn2Input,		NTV2_XptDualLinkIn2DSInput);
			DefineXptReg	(kRegXptSelectGroup12,	NTV2_XptLUT3Input,				NTV2_XptLUT4Input,				NTV2_XptLUT5Input,				NTV2_INPUT_CROSSPOINT_INVALID);
			DefineXptReg	(kRegXptSelectGroup13,	NTV2_XptFrameBuffer3Input,		NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_XptFrameBuffer4Input,		NTV2_INPUT_CROSSPOINT_INVALID);
			DefineXptReg	(kRegXptSelectGroup14,	NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_XptSDIOut3InputDS2,		NTV2_XptSDIOut5InputDS2,		NTV2_XptSDIOut4InputDS2);
			DefineXptReg	(kRegXptSelectGroup15,	NTV2_XptDualLinkIn3Input,		NTV2_XptDualLinkIn3DSInput,		NTV2_XptDualLinkIn4Input,		NTV2_XptDualLinkIn4DSInput);
			DefineXptReg	(kRegXptSelectGroup16,	NTV2_XptDualLinkOut3Input,		NTV2_XptDualLinkOut4Input,		NTV2_XptDualLinkOut5Input,		NTV2_INPUT_CROSSPOINT_INVALID);
			DefineXptReg	(kRegXptSelectGroup17,	NTV2_XptCSC3VidInput,			NTV2_XptCSC3KeyInput,			NTV2_XptCSC4VidInput,			NTV2_XptCSC4KeyInput);
			DefineXptReg	(kRegXptSelectGroup18,	NTV2_XptCSC5VidInput,			NTV2_XptCSC5KeyInput,			NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_INPUT_CROSSPOINT_INVALID);
			DefineXptReg	(kRegXptSelectGroup19,	NTV2_Xpt4KDCQ1Input,			NTV2_Xpt4KDCQ2Input,			NTV2_Xpt4KDCQ3Input,			NTV2_Xpt4KDCQ4Input);
			DefineXptReg	(kRegXptSelectGroup20,	NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_XptHDMIOutQ2Input,			NTV2_XptHDMIOutQ3Input,			NTV2_XptHDMIOutQ4Input);
			DefineXptReg	(kRegXptSelectGroup21,	NTV2_XptFrameBuffer5Input,		NTV2_XptFrameBuffer6Input,		NTV2_XptFrameBuffer7Input,		NTV2_XptFrameBuffer8Input);
			DefineXptReg	(kRegXptSelectGroup22,	NTV2_XptSDIOut6Input,			NTV2_XptSDIOut6InputDS2,		NTV2_XptSDIOut7Input,			NTV2_XptSDIOut7InputDS2);
			DefineXptReg	(kRegXptSelectGroup23,	NTV2_XptCSC7VidInput,			NTV2_XptCSC7KeyInput,			NTV2_XptCSC8VidInput,			NTV2_XptCSC8KeyInput);
			DefineXptReg	(kRegXptSelectGroup24,	NTV2_XptLUT6Input,				NTV2_XptLUT7Input,				NTV2_XptLUT8Input,				NTV2_INPUT_CROSSPOINT_INVALID);
			DefineXptReg	(kRegXptSelectGroup25,	NTV2_XptDualLinkIn5Input,		NTV2_XptDualLinkIn5DSInput,		NTV2_XptDualLinkIn6Input,		NTV2_XptDualLinkIn6DSInput);
			DefineXptReg	(kRegXptSelectGroup26,	NTV2_XptDualLinkIn7Input,		NTV2_XptDualLinkIn7DSInput,		NTV2_XptDualLinkIn8Input,		NTV2_XptDualLinkIn8DSInput);
			DefineXptReg	(kRegXptSelectGroup27,	NTV2_XptDualLinkOut6Input,		NTV2_XptDualLinkOut7Input,		NTV2_XptDualLinkOut8Input,		NTV2_INPUT_CROSSPOINT_INVALID);
			DefineXptReg	(kRegXptSelectGroup28,	NTV2_XptMixer3FGVidInput,		NTV2_XptMixer3FGKeyInput,		NTV2_XptMixer3BGVidInput,		NTV2_XptMixer3BGKeyInput);
			DefineXptReg	(kRegXptSelectGroup29,	NTV2_XptMixer4FGVidInput,		NTV2_XptMixer4FGKeyInput,		NTV2_XptMixer4BGVidInput,		NTV2_XptMixer4BGKeyInput);
			DefineXptReg	(kRegXptSelectGroup30,	NTV2_XptSDIOut8Input,			NTV2_XptSDIOut8InputDS2,		NTV2_XptCSC6VidInput,			NTV2_XptCSC6KeyInput);
			DefineXptReg	(kRegXptSelectGroup32,	NTV2_Xpt425Mux1AInput,			NTV2_Xpt425Mux1BInput,			NTV2_Xpt425Mux2AInput,			NTV2_Xpt425Mux2BInput);
			DefineXptReg	(kRegXptSelectGroup33,	NTV2_Xpt425Mux3AInput,			NTV2_Xpt425Mux3BInput,			NTV2_Xpt425Mux4AInput,			NTV2_Xpt425Mux4BInput);
			DefineXptReg	(kRegXptSelectGroup34,	NTV2_XptFrameBuffer1BInput,		NTV2_XptFrameBuffer2BInput,		NTV2_XptFrameBuffer3BInput,		NTV2_XptFrameBuffer4BInput);
			DefineXptReg	(kRegXptSelectGroup35,	NTV2_XptFrameBuffer5BInput,		NTV2_XptFrameBuffer6BInput,		NTV2_XptFrameBuffer7BInput,		NTV2_XptFrameBuffer8BInput);
		}	//	SetupXptSelect

		void SetupAncInsExt(void)
		{
			static const string	AncExtRegNames []	=	{	"Control",				"F1 Start Address",		"F1 End Address",
															"F2 Start Address",		"F2 End Address",		"Field Cutoff Lines",
															"Memory Total",			"F1 Memory Usage",		"F2 Memory Usage",
															"V Blank Lines",		"Lines Per Frame",		"Field ID Lines",
															"Ignore DID 1-4",		"Ignore DID 5-8",		"Ignore DID 9-12",
															"Ignore DID 13-16",		"Ignore DID 17-20",		"Analog Start Line",
															"Analog F1 Y Filter",	"Analog F2 Y Filter",	"Analog F1 C Filter",
															"Analog F2 C Filter"	};
			static const string	AncInsRegNames []	=	{	"Field Bytes",			"Control",				"F1 Start Address",
															"F2 Start Address",		"Pixel Delays",			"First Active Lines",
															"Pixels Per Line",		"Lines Per Frame",		"Field ID Lines",
															"Payload ID Control",	"Payload ID",			"Chroma Blank Lines",
															"F1 C Blanking Mask",	"F2 C Blanking Mask"	};
			static const uint32_t	AncExtPerChlRegBase []	=	{	0x1000,	0x1040,	0x1080,	0x10C0,	0x1100,	0x1140,	0x1180,	0x11C0	};
			static const uint32_t	AncInsPerChlRegBase []	=	{	0x1200,	0x1240,	0x1280,	0x12C0,	0x1300,	0x1340,	0x1380,	0x13C0	};

			for (ULWord offsetNdx (0);  offsetNdx < 8;  offsetNdx++)
			{
				for (ULWord reg (regAncExtControl);  reg < regAncExt_LAST;  reg++)
				{
					ostringstream	oss;	oss << "Extract " << (offsetNdx+1) << " " << AncExtRegNames[reg];
					DefineRegName (AncExtPerChlRegBase [offsetNdx] + reg,	oss.str());
				}
				for (ULWord reg (regAncInsFieldBytes);  reg < regAncIns_LAST;  reg++)
				{
					ostringstream	oss;	oss << "Insert " << (offsetNdx+1) << " " << AncInsRegNames[reg];
					DefineRegName (AncInsPerChlRegBase [offsetNdx] + reg,	oss.str());
				}
			}
			for (ULWord ndx (0);  ndx < 8;  ndx++)
			{
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtControl,						"",	mDecodeAncExtControlReg,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1StartAddress,			"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1EndAddress,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2StartAddress,			"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2EndAddress,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtFieldCutoffLine,				"",	mDecodeAncExtFieldLines,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtTotalStatus,					"",	mDecodeAncExtStatus,			READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1Status,					"",	mDecodeAncExtStatus,			READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2Status,					"",	mDecodeAncExtStatus,			READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtFieldVBLStartLine,				"",	mDecodeAncExtFieldLines,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtTotalFrameLines,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtFID,							"",	mDecodeAncExtFieldLines,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_1_2_3_4,		"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_5_6_7_8,		"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_9_10_11_12,	"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_13_14_15_16,	"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_17_18_19_20,	"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtAnalogStartLine,				"",	mDecodeAncExtFieldLines,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1AnalogYFilter,			"",	mDecodeAncExtAnalogFilter,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2AnalogYFilter,			"",	mDecodeAncExtAnalogFilter,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1AnalogCFilter,			"",	mDecodeAncExtAnalogFilter,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
				DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2AnalogCFilter,			"",	mDecodeAncExtAnalogFilter,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);

				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsFieldBytes,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsControl,						"",	mDecodeAncInsControlReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsField1StartAddr,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsField2StartAddr,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsPixelDelay,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsActiveStart,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsLinePixels,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsFrameLines,					"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsFieldIDLines,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsBlankCStartLine,				"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsBlankField1CLines,				"",	mDecodeAncInsChromaBlankReg,	READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
				DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsBlandField2CLines,				"",	mDecodeAncInsChromaBlankReg,	READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			}
		}	//	SetupAncInsExt

		void SetupHDMIRegs(void)
		{
			DefineRegister (kRegHDMIOutControl,							"",	mDecodeHDMIOutputControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
			DefineRegister (kRegHDMIInputStatus,						"",	mDecodeHDMIInputStatus,		READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
			//DefineRegister (kRegHDMIInputControl,						"",	mDecodeHDMIInputControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
			DefineRegister (kRegHDMIHDRGreenPrimary,					"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
			DefineRegister (kRegHDMIHDRBluePrimary,						"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
			DefineRegister (kRegHDMIHDRRedPrimary,						"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
			DefineRegister (kRegHDMIHDRWhitePoint,						"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
			DefineRegister (kRegHDMIHDRMasteringLuminence,				"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
			DefineRegister (kRegHDMIHDRLightLevel,						"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
			DefineRegister (kRegHDMIHDRControl,							"",	mDecodeHDMIOutHDRControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
			DefineRegister (kRegHDMIV2I2C1Control,						"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2I2C1Data,							"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2VideoSetup,						"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2HSyncDurationAndBackPorch,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2HActive,							"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2VSyncDurationAndBackPorchField1,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2VSyncDurationAndBackPorchField2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2VActiveField1,					"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2VActiveField2,					"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2VideoStatus,						"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2HorizontalMeasurements,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2HBlankingMeasurements,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2HBlankingMeasurements1,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2VerticalMeasurementsField0,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2VerticalMeasurementsField1,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2i2c2Control,						"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
			DefineRegister (kRegHDMIV2i2c2Data,							"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		}

		void SetupSDIError(void)
		{
			static const ULWord	baseNum[]	=	{kRegRXSDI1Status,	kRegRXSDI2Status,	kRegRXSDI3Status,	kRegRXSDI4Status,	kRegRXSDI5Status,	kRegRXSDI6Status,	kRegRXSDI7Status,	kRegRXSDI8Status};
			static const string	suffixes []	=	{"Status",	"CRCErrorCount",	"FrameCountLow",	"FrameCountHigh",	"FrameRefCountLow",	"FrameRefCountHigh"};
			static const int	perms []	=	{READWRITE,	READWRITE,			READWRITE,			READWRITE,			READONLY,			READONLY};
			for (ULWord chan (0);  chan < 8;  chan++)
				for (UWord ndx(0);  ndx < 6;  ndx++)
				{
					ostringstream	ossName;	ossName << "kRegRXSDI" << DEC(chan+1) << suffixes[ndx];
					const string &	regName		(ossName.str());
					const uint32_t	regNum		(baseNum[chan] + ndx);
					const int		perm		(perms[ndx]);
					if (ndx == 0)
						DefineRegister (regNum,  regName,  mSDIErrorStatusRegDecoder,  perm,  kRegClass_SDIError,  gChlClasses[chan],  kRegClass_Input);
					else if (ndx == 1)
						DefineRegister (regNum,  regName,  mSDIErrorCountRegDecoder,   perm,  kRegClass_SDIError,  gChlClasses[chan],  kRegClass_Input);
					else
						DefineRegister (regNum,  regName,  mDefaultRegDecoder,         perm,  kRegClass_SDIError,  gChlClasses[chan],  kRegClass_Input);
				}
			DefineRegister (kRegRXSDIFreeRunningClockLow, "kRegRXSDIFreeRunningClockLow", mDefaultRegDecoder, READONLY, kRegClass_SDIError, kRegClass_NULL, kRegClass_NULL);
			DefineRegister (kRegRXSDIFreeRunningClockHigh, "kRegRXSDIFreeRunningClockHigh", mDefaultRegDecoder, READONLY, kRegClass_SDIError, kRegClass_NULL, kRegClass_NULL);
		}	//	SetupSDIError

		void SetupVRegs(void)
		{
			DefineRegName	(kVRegLinuxDriverVersion,				"kVRegLinuxDriverVersion");
			DefineRegName	(kVRegRelativeVideoPlaybackDelay,		"kVRegRelativeVideoPlaybackDelay");
			DefineRegName	(kVRegAudioRecordPinDelay,				"kVRegAudioRecordPinDelay");
			DefineRegName	(kVRegDriverVersion,					"kVRegDriverVersion");
			DefineRegName	(kVRegGlobalAudioPlaybackMode,			"kVRegGlobalAudioPlaybackMode");
			DefineRegName	(kVRegFlashProgramKey,					"kVRegFlashProgramKey");
			DefineRegName	(kVRegStrictTiming,						"kVRegStrictTiming");
			DefineRegName	(kVRegInputSelect,						"kVRegInputSelect");
			DefineRegName	(kVRegSecondaryFormatSelect,			"kVRegSecondaryFormatSelect");
			DefineRegName	(kVRegDigitalOutput1Select,				"kVRegDigitalOutput1Select");
			DefineRegName	(kVRegDigitalOutput2Select,				"kVRegDigitalOutput2Select");
			DefineRegName	(kVRegAnalogOutputSelect,				"kVRegAnalogOutputSelect");
			DefineRegName	(kVRegAnalogOutputType,					"kVRegAnalogOutputType");
			DefineRegName	(kVRegAnalogOutBlackLevel,				"kVRegAnalogOutBlackLevel");
			DefineRegName	(kVRegVideoOutPauseMode,				"kVRegVideoOutPauseMode");
			DefineRegName	(kVRegPulldownPattern,					"kVRegPulldownPattern");
			DefineRegName	(kVRegColorSpaceMode,					"kVRegColorSpaceMode");
			DefineRegName	(kVRegGammaMode,						"kVRegGammaMode");
			DefineRegName	(kVRegLUTType,							"kVRegLUTType");
			DefineRegName	(kVRegRGB10Range,						"kVRegRGB10Range");
			DefineRegName	(kVRegRGB10Endian,						"kVRegRGB10Endian");
			DefineRegName	(kVRegBitFileDownload,					"kVRegBitFileDownload");
			DefineRegName	(kVRegSaveRegistersToRegistry,			"kVRegSaveRegistersToRegistry");
			DefineRegName	(kVRegRecallRegistersFromRegistry,		"kVRegRecallRegistersFromRegistry");
			DefineRegName	(kVRegClearAllSubscriptions,			"kVRegClearAllSubscriptions");
			DefineRegName	(kVRegRestoreHardwareProcampRegisters,	"kVRegRestoreHardwareProcampRegisters");
			DefineRegName	(kVRegAcquireReferenceCount,			"kVRegAcquireReferenceCount");
			DefineRegName	(kVRegReleaseReferenceCount,			"kVRegReleaseReferenceCount");
			DefineRegName	(kVRegDTAudioMux0,						"kVRegDTAudioMux0");
			DefineRegName	(kVRegDTAudioMux1,						"kVRegDTAudioMux1");
			DefineRegName	(kVRegDTAudioMux2,						"kVRegDTAudioMux2");
			DefineRegName	(kVRegDTFirmware,						"kVRegDTFirmware");
			DefineRegName	(kVRegDTVersionAja,						"kVRegDTVersionAja");
			DefineRegName	(kVRegDTVersionDurian,					"kVRegDTVersionDurian");
			DefineRegName	(kVRegDTAudioCapturePinConnected,		"kVRegDTAudioCapturePinConnected");
			DefineRegName	(kVRegTimeStampMode,					"kVRegTimeStampMode");
			DefineRegName	(kVRegTimeStampLastOutputVerticalLo,	"kVRegTimeStampLastOutputVerticalLo");
			DefineRegName	(kVRegTimeStampLastOutputVerticalHi,	"kVRegTimeStampLastOutputVerticalHi");
			DefineRegName	(kVRegTimeStampLastInput1VerticalLo,	"kVRegTimeStampLastInput1VerticalLo");
			DefineRegName	(kVRegTimeStampLastInput1VerticalHi,	"kVRegTimeStampLastInput1VerticalHi");
			DefineRegName	(kVRegTimeStampLastInput2VerticalLo,	"kVRegTimeStampLastInput2VerticalLo");
			DefineRegName	(kVRegTimeStampLastInput2VerticalHi,	"kVRegTimeStampLastInput2VerticalHi");
			DefineRegName	(kVRegNumberVideoMappingRegisters,		"kVRegNumberVideoMappingRegisters");
			DefineRegName	(kVRegNumberAudioMappingRegisters,		"kVRegNumberAudioMappingRegisters");
			DefineRegName	(kVRegAudioSyncTolerance,				"kVRegAudioSyncTolerance");
			DefineRegName	(kVRegDmaSerialize,						"kVRegDmaSerialize");
			DefineRegName	(kVRegSyncChannel,						"kVRegSyncChannel");
			DefineRegName	(kVRegSyncChannels,						"kVRegSyncChannels");
			DefineRegName	(kVRegSoftwareUartFifo,					"kVRegSoftwareUartFifo");
			DefineRegName	(kVRegTimeCodeCh1Delay,					"kVRegTimeCodeCh1Delay");
			DefineRegName	(kVRegTimeCodeCh2Delay,					"kVRegTimeCodeCh2Delay");
			DefineRegName	(kVRegTimeCodeIn1Delay,					"kVRegTimeCodeIn1Delay");
			DefineRegName	(kVRegTimeCodeIn2Delay,					"kVRegTimeCodeIn2Delay");
			DefineRegName	(kVRegTimeCodeCh3Delay,					"kVRegTimeCodeCh3Delay");
			DefineRegName	(kVRegTimeCodeCh4Delay,					"kVRegTimeCodeCh4Delay");
			DefineRegName	(kVRegTimeCodeIn3Delay,					"kVRegTimeCodeIn3Delay");
			DefineRegName	(kVRegTimeCodeIn4Delay,					"kVRegTimeCodeIn4Delay");
			DefineRegName	(kVRegTimeCodeCh5Delay,					"kVRegTimeCodeCh5Delay");
			DefineRegName	(kVRegTimeCodeIn5Delay,					"kVRegTimeCodeIn5Delay");
			DefineRegName	(kVRegTimeCodeCh6Delay,					"kVRegTimeCodeCh6Delay");
			DefineRegName	(kVRegTimeCodeIn6Delay,					"kVRegTimeCodeIn6Delay");
			DefineRegName	(kVRegTimeCodeCh7Delay,					"kVRegTimeCodeCh7Delay");
			DefineRegName	(kVRegTimeCodeIn7Delay,					"kVRegTimeCodeIn7Delay");
			DefineRegName	(kVRegTimeCodeCh8Delay,					"kVRegTimeCodeCh8Delay");
			DefineRegName	(kVRegTimeCodeIn8Delay,					"kVRegTimeCodeIn8Delay");
			DefineRegName	(kVRegDebug1,							"kVRegDebug1");
			DefineRegName	(kVRegDisplayReferenceSelect,			"kVRegDisplayReferenceSelect");
			DefineRegName	(kVRegVANCMode,							"kVRegVANCMode");
			DefineRegName	(kVRegDualStreamTransportType,			"kVRegDualStreamTransportType");
			DefineRegName	(kVRegSDIOut1TransportType,				"kVRegSDIOut1TransportType");
			DefineRegName	(kVRegDSKMode,							"kVRegDSKMode");
			DefineRegName	(kVRegIsoConvertEnable,					"kVRegIsoConvertEnable");
			DefineRegName	(kVRegDSKAudioMode,						"kVRegDSKAudioMode");
			DefineRegName	(kVRegDSKForegroundMode,				"kVRegDSKForegroundMode");
			DefineRegName	(kVRegDSKForegroundFade,				"kVRegDSKForegroundFade");
			DefineRegName	(kVRegCaptureReferenceSelect,			"kVRegCaptureReferenceSelect");
			DefineRegName	(kVReg2XTransferMode,					"kVReg2XTransferMode");
			DefineRegName	(kVRegSDIOutput1RGBRange,				"kVRegSDIOutput1RGBRange");
			DefineRegName	(kVRegSDIInput1FormatSelect,			"kVRegSDIInput1FormatSelect");
			DefineRegName	(kVRegSDIInput2FormatSelect,			"kVRegSDIInput2FormatSelect");
			DefineRegName	(kVRegSDIInput1RGBRange,				"kVRegSDIInput1RGBRange");
			DefineRegName	(kVRegSDIInput2RGBRange,				"kVRegSDIInput2RGBRange");
			DefineRegName	(kVRegSDIInput1Stereo3DMode,			"kVRegSDIInput1Stereo3DMode");
			DefineRegName	(kVRegSDIInput2Stereo3DMode,			"kVRegSDIInput2Stereo3DMode");
			DefineRegName	(kVRegFrameBuffer1RGBRange,				"kVRegFrameBuffer1RGBRange");
			DefineRegName	(kVRegFrameBuffer1Stereo3DMode,			"kVRegFrameBuffer1Stereo3DMode");
			DefineRegName	(kVRegAnalogInBlackLevel,				"kVRegAnalogInBlackLevel");
			DefineRegName	(kVRegAnalogInputType,					"kVRegAnalogInputType");
			DefineRegName	(kVRegHDMIOutColorSpaceModeCtrl,		"kVRegHDMIOutColorSpaceModeCtrl");
			DefineRegName	(kVRegHDMIOutProtocolMode,				"kVRegHDMIOutProtocolMode");
			DefineRegName	(kVRegHDMIOutStereoSelect,				"kVRegHDMIOutStereoSelect");
			DefineRegName	(kVRegHDMIOutStereoCodecSelect,			"kVRegHDMIOutStereoCodecSelect");
			DefineRegName	(kVRegReversePulldownOffset,			"kVRegReversePulldownOffset");
			DefineRegName	(kVRegSDIInput1ColorSpaceMode,			"kVRegSDIInput1ColorSpaceMode");
			DefineRegName	(kVRegSDIInput2ColorSpaceMode,			"kVRegSDIInput2ColorSpaceMode");
			DefineRegName	(kVRegSDIOutput2RGBRange,				"kVRegSDIOutput2RGBRange");
			DefineRegName	(kVRegSDIOutput1Stereo3DMode,			"kVRegSDIOutput1Stereo3DMode");
			DefineRegName	(kVRegSDIOutput2Stereo3DMode,			"kVRegSDIOutput2Stereo3DMode");
			DefineRegName	(kVRegFrameBuffer2RGBRange,				"kVRegFrameBuffer2RGBRange");
			DefineRegName	(kVRegFrameBuffer2Stereo3DMode,			"kVRegFrameBuffer2Stereo3DMode");
			DefineRegName	(kVRegAudioGainDisable,					"kVRegAudioGainDisable");
			DefineRegName	(kVRegDBLAudioEnable,					"kVRegDBLAudioEnable");
			DefineRegName	(kVRegActiveVideoOutFilter,				"kVRegActiveVideoOutFilter");
			DefineRegName	(kVRegAudioInputMapSelect,				"kVRegAudioInputMapSelect");
			DefineRegName	(kVRegAudioInputDelay,					"kVRegAudioInputDelay");
			DefineRegName	(kVRegDSKGraphicFileIndex,				"kVRegDSKGraphicFileIndex");
			DefineRegName	(kVRegTimecodeBurnInMode,				"kVRegTimecodeBurnInMode");
			DefineRegName	(kVRegUseQTTimecode,					"kVRegUseQTTimecode");
			DefineRegName	(kVRegAvailable164,						"kVRegAvailable164");
			DefineRegName	(kVRegRP188SourceSelect,				"kVRegRP188SourceSelect");
			DefineRegName	(kVRegQTCodecModeDebug,					"kVRegQTCodecModeDebug");
			DefineRegName	(kVRegHDMIOutColorSpaceModeStatus,		"kVRegHDMIOutColorSpaceModeStatus");
			DefineRegName	(kVRegDeviceOnline,						"kVRegDeviceOnline");
			DefineRegName	(kVRegIsDefaultDevice,					"kVRegIsDefaultDevice");
			DefineRegName	(kVRegDesktopFrameBufferStatus,			"kVRegDesktopFrameBufferStatus");
			DefineRegName	(kVRegSDIOutput1ColorSpaceMode,			"kVRegSDIOutput1ColorSpaceMode");
			DefineRegName	(kVRegSDIOutput2ColorSpaceMode,			"kVRegSDIOutput2ColorSpaceMode");
			DefineRegName	(kVRegAudioOutputDelay,					"kVRegAudioOutputDelay");
			DefineRegName	(kVRegTimelapseEnable,					"kVRegTimelapseEnable");
			DefineRegName	(kVRegTimelapseCaptureValue,			"kVRegTimelapseCaptureValue");
			DefineRegName	(kVRegTimelapseCaptureUnits,			"kVRegTimelapseCaptureUnits");
			DefineRegName	(kVRegTimelapseIntervalValue,			"kVRegTimelapseIntervalValue");
			DefineRegName	(kVRegTimelapseIntervalUnits,			"kVRegTimelapseIntervalUnits");
			DefineRegName	(kVRegFrameBufferInstalled,				"kVRegFrameBufferInstalled");
			DefineRegName	(kVRegAnalogInStandard,					"kVRegAnalogInStandard");
			DefineRegName	(kVRegOutputTimecodeOffset,				"kVRegOutputTimecodeOffset");
			DefineRegName	(kVRegOutputTimecodeType,				"kVRegOutputTimecodeType");
			DefineRegName	(kVRegQuicktimeUsingBoard,				"kVRegQuicktimeUsingBoard");
			DefineRegName	(kVRegApplicationPID,					"kVRegApplicationPID");
			DefineRegName	(kVRegApplicationCode,					"kVRegApplicationCode");
			DefineRegName	(kVRegReleaseApplication,				"kVRegReleaseApplication");
			DefineRegName	(kVRegForceApplicationPID,				"kVRegForceApplicationPID");
			DefineRegName	(kVRegForceApplicationCode,				"kVRegForceApplicationCode");
			DefineRegName	(kVRegProcAmpSDRegsInitialized,			"kVRegProcAmpSDRegsInitialized");
			DefineRegName	(kVRegProcAmpStandardDefBrightness,		"kVRegProcAmpStandardDefBrightness");
			DefineRegName	(kVRegProcAmpStandardDefContrast,		"kVRegProcAmpStandardDefContrast");
			DefineRegName	(kVRegProcAmpStandardDefSaturation,		"kVRegProcAmpStandardDefSaturation");
			DefineRegName	(kVRegProcAmpStandardDefHue,			"kVRegProcAmpStandardDefHue");
			DefineRegName	(kVRegProcAmpStandardDefCbOffset,		"kVRegProcAmpStandardDefCbOffset");
			DefineRegName	(kVRegProcAmpStandardDefCrOffset,		"kVRegProcAmpStandardDefCrOffset");
			DefineRegName	(kVRegProcAmpEndStandardDefRange,		"kVRegProcAmpEndStandardDefRange");
			DefineRegName	(kVRegProcAmpHDRegsInitialized,			"kVRegProcAmpHDRegsInitialized");
			DefineRegName	(kVRegProcAmpHighDefBrightness,			"kVRegProcAmpHighDefBrightness");
			DefineRegName	(kVRegProcAmpHighDefContrast,			"kVRegProcAmpHighDefContrast");
			DefineRegName	(kVRegProcAmpHighDefSaturationCb,		"kVRegProcAmpHighDefSaturationCb");
			DefineRegName	(kVRegProcAmpHighDefSaturationCr,		"kVRegProcAmpHighDefSaturationCr");
			DefineRegName	(kVRegProcAmpHighDefHue,				"kVRegProcAmpHighDefHue");
			DefineRegName	(kVRegProcAmpHighDefCbOffset,			"kVRegProcAmpHighDefCbOffset");
			DefineRegName	(kVRegProcAmpHighDefCrOffset,			"kVRegProcAmpHighDefCrOffset");
			DefineRegName	(kVRegProcAmpEndHighDefRange,			"kVRegProcAmpEndHighDefRange");
			DefineRegName	(kVRegChannel1UserBufferLevel,			"kVRegChannel1UserBufferLevel");
			DefineRegName	(kVRegChannel2UserBufferLevel,			"kVRegChannel2UserBufferLevel");
			DefineRegName	(kVRegInput1UserBufferLevel,			"kVRegInput1UserBufferLevel");
			DefineRegName	(kVRegInput2UserBufferLevel,			"kVRegInput2UserBufferLevel");
			DefineRegName	(kVRegProgressivePicture,				"kVRegProgressivePicture");
			DefineRegName	(kVRegLUT2Type,							"kVRegLUT2Type");
			DefineRegName	(kVRegLUT3Type,							"kVRegLUT3Type");
			DefineRegName	(kVRegLUT4Type,							"kVRegLUT4Type");
			DefineRegName	(kVRegDigitalOutput3Select,				"kVRegDigitalOutput3Select");
			DefineRegName	(kVRegDigitalOutput4Select,				"kVRegDigitalOutput4Select");
			DefineRegName	(kVRegHDMIOutputSelect,					"kVRegHDMIOutputSelect");
			DefineRegName	(kVRegRGBRangeConverterLUTType,			"kVRegRGBRangeConverterLUTType");
			DefineRegName	(kVRegTestPatternChoice,				"kVRegTestPatternChoice");
			DefineRegName	(kVRegTestPatternFormat,				"kVRegTestPatternFormat");
			DefineRegName	(kVRegEveryFrameTaskFilter,				"kVRegEveryFrameTaskFilter");
			DefineRegName	(kVRegDefaultInput,						"kVRegDefaultInput");
			DefineRegName	(kVRegDefaultVideoOutMode,				"kVRegDefaultVideoOutMode");
			DefineRegName	(kVRegDefaultVideoFormat,				"kVRegDefaultVideoFormat");
			DefineRegName	(kVRegDigitalOutput5Select,				"kVRegDigitalOutput5Select");
			DefineRegName	(kVRegLUT5Type,							"kVRegLUT5Type");
			DefineRegName	(kVRegMacUserModeDebugLevel,			"kVRegMacUserModeDebugLevel");
			DefineRegName	(kVRegMacKernelModeDebugLevel,			"kVRegMacKernelModeDebugLevel");
			DefineRegName	(kVRegMacUserModePingLevel,				"kVRegMacUserModePingLevel");
			DefineRegName	(kVRegMacKernelModePingLevel,			"kVRegMacKernelModePingLevel");
			DefineRegName	(kVRegLatencyTimerValue,				"kVRegLatencyTimerValue");
			DefineRegName	(kVRegAudioAVSyncEnable,				"kVRegAudioAVSyncEnable");
			DefineRegName	(kVRegAudioInputSelect,					"kVRegAudioInputSelect");
			DefineRegName	(kVRegSerialSuspended,					"kVRegSerialSuspended");
			DefineRegName	(kVRegXilinxProgramming,				"kVRegXilinxProgramming");
			DefineRegName	(kVRegETTDiagLastSerialTimestamp,		"kVRegETTDiagLastSerialTimestamp");
			DefineRegName	(kVRegETTDiagLastSerialTimecode,		"kVRegETTDiagLastSerialTimecode");
			DefineRegName	(kVRegStartupStatusFlags,				"kVRegStartupStatusFlags");
			DefineRegName	(kVRegRGBRangeMode,						"kVRegRGBRangeMode");
			DefineRegName	(kVRegEnableQueuedDMAs,					"kVRegEnableQueuedDMAs");
			DefineRegName	(kVRegBA0MemorySize,					"kVRegBA0MemorySize");
			DefineRegName	(kVRegBA1MemorySize,					"kVRegBA1MemorySize");
			DefineRegName	(kVRegBA4MemorySize,					"kVRegBA4MemorySize");
			DefineRegName	(kVRegNumDmaDriverBuffers,				"kVRegNumDmaDriverBuffers");
			DefineRegName	(kVRegDMADriverBufferPhysicalAddress,	"kVRegDMADriverBufferPhysicalAddress");
			DefineRegName	(kVRegBA2MemorySize,					"kVRegBA2MemorySize");
			DefineRegName	(kVRegAcquireLinuxReferenceCount,		"kVRegAcquireLinuxReferenceCount");
			DefineRegName	(kVRegReleaseLinuxReferenceCount,		"kVRegReleaseLinuxReferenceCount");
			DefineRegName	(kVRegAdvancedIndexing,					"kVRegAdvancedIndexing");
			DefineRegName	(kVRegTimeStampLastInput3VerticalLo,	"kVRegTimeStampLastInput3VerticalLo");
			DefineRegName	(kVRegTimeStampLastInput3VerticalHi,	"kVRegTimeStampLastInput3VerticalHi");
			DefineRegName	(kVRegTimeStampLastInput4VerticalLo,	"kVRegTimeStampLastInput4VerticalLo");
			DefineRegName	(kVRegTimeStampLastInput4VerticalHi,	"kVRegTimeStampLastInput4VerticalHi");
			DefineRegName	(kVRegTimeStampLastInput5VerticalLo,	"kVRegTimeStampLastInput5VerticalLo");
			DefineRegName	(kVRegTimeStampLastInput5VerticalHi,	"kVRegTimeStampLastInput5VerticalHi");
			DefineRegName	(kVRegTimeStampLastInput6VerticalLo,	"kVRegTimeStampLastInput6VerticalLo");
			DefineRegName	(kVRegTimeStampLastInput6VerticalHi,	"kVRegTimeStampLastInput6VerticalHi");
			DefineRegName	(kVRegTimeStampLastInput7VerticalLo,	"kVRegTimeStampLastInput7VerticalLo");
			DefineRegName	(kVRegTimeStampLastInput7VerticalHi,	"kVRegTimeStampLastInput7VerticalHi");
			DefineRegName	(kVRegTimeStampLastInput8VerticalLo,	"kVRegTimeStampLastInput8VerticalLo");
			DefineRegName	(kVRegTimeStampLastInput8VerticalHi,	"kVRegTimeStampLastInput8VerticalHi");
			DefineRegName	(kVRegTimeStampLastOutput2VerticalLo,	"kVRegTimeStampLastOutput2VerticalLo");
			DefineRegName	(kVRegTimeStampLastOutput2VerticalHi,	"kVRegTimeStampLastOutput2VerticalHi");
			DefineRegName	(kVRegTimeStampLastOutput3VerticalLo,	"kVRegTimeStampLastOutput3VerticalLo");
			DefineRegName	(kVRegTimeStampLastOutput3VerticalHi,	"kVRegTimeStampLastOutput3VerticalHi");
			DefineRegName	(kVRegTimeStampLastOutput4VerticalLo,	"kVRegTimeStampLastOutput4VerticalLo");
			DefineRegName	(kVRegTimeStampLastOutput4VerticalHi,	"kVRegTimeStampLastOutput4VerticalHi");
			DefineRegName	(kVRegTimeStampLastOutput5VerticalLo,	"kVRegTimeStampLastOutput5VerticalLo");
			DefineRegName	(kVRegTimeStampLastOutput5VerticalHi,	"kVRegTimeStampLastOutput5VerticalHi");
			DefineRegName	(kVRegTimeStampLastOutput6VerticalLo,	"kVRegTimeStampLastOutput6VerticalLo");
			DefineRegName	(kVRegTimeStampLastOutput6VerticalHi,	"kVRegTimeStampLastOutput6VerticalHi");
			DefineRegName	(kVRegTimeStampLastOutput7VerticalLo,	"kVRegTimeStampLastOutput7VerticalLo");
			DefineRegName	(kVRegTimeStampLastOutput7VerticalHi,	"kVRegTimeStampLastOutput7VerticalHi");
			DefineRegName	(kVRegTimeStampLastOutput8VerticalLo,	"kVRegTimeStampLastOutput8VerticalLo");
			DefineRegName	(kVRegResetCycleCount,					"kVRegResetCycleCount");
			DefineRegName	(kVRegUseProgressive,					"kVRegUseProgressive");
			DefineRegName	(kVRegFlashSize,						"kVRegFlashSize");
			DefineRegName	(kVRegFlashStatus,						"kVRegFlashStatus");
			DefineRegName	(kVRegFlashState,						"kVRegFlashState");
			DefineRegName	(kVRegPCIDeviceID,						"kVRegPCIDeviceID");
			DefineRegName	(kVRegUartRxFifoSize,					"kVRegUartRxFifoSize");
			DefineRegName	(kVRegEFTNeedsUpdating,					"kVRegEFTNeedsUpdating");
			DefineRegName	(kVRegSuspendSystemAudio,				"kVRegSuspendSystemAudio");
			DefineRegName	(kVRegAcquireReferenceCounter,			"kVRegAcquireReferenceCounter");
			DefineRegName	(kVRegTimeStampLastOutput8VerticalHi,	"kVRegTimeStampLastOutput8VerticalHi");
			DefineRegName	(kVRegFramesPerVertical,				"kVRegFramesPerVertical");
			DefineRegName	(kVRegServicesInitialized,				"kVRegServicesInitialized");
			DefineRegName	(kVRegFrameBufferGangCount,				"kVRegFrameBufferGangCount");
			DefineRegName	(kVRegChannelCrosspointFirst,			"kVRegChannelCrosspointFirst");
			DefineRegName	(kVRegChannelCrosspointLast,			"kVRegChannelCrosspointLast");
			DefineRegName	(kVRegDriverVersionMajor,				"kVRegDriverVersionMajor");
			DefineRegName	(kVRegDriverVersionMinor,				"kVRegDriverVersionMinor");
			DefineRegName	(kVRegDriverVersionPoint,				"kVRegDriverVersionPoint");
			DefineRegName	(kVRegFollowInputFormat,				"kVRegFollowInputFormat");
			DefineRegName	(kVRegAncField1Offset,					"kVRegAncField1Offset");
			DefineRegName	(kVRegAncField2Offset,					"kVRegAncField2Offset");
			DefineRegName	(kVRegAgentCheck,						"kVRegAgentCheck");
			DefineRegName	(kVRegUnused_2,							"kVRegUnused_2");
			DefineRegName	(kVReg4kOutputTransportSelection,		"kVReg4kOutputTransportSelection");
			DefineRegName	(kVRegCustomAncInputSelect,				"kVRegCustomAncInputSelect");
			DefineRegName	(kVRegUseThermostat,					"kVRegUseThermostat");
			DefineRegName	(kVRegThermalSamplingRate,				"kVRegThermalSamplingRate");
			DefineRegName	(kVRegFanSpeed,							"kVRegFanSpeed");
			DefineRegName	(kVRegVideoFormatCh1,					"kVRegVideoFormatCh1");
			DefineRegName	(kVRegVideoFormatCh2,					"kVRegVideoFormatCh2");
			DefineRegName	(kVRegVideoFormatCh3,					"kVRegVideoFormatCh3");
			DefineRegName	(kVRegVideoFormatCh4,					"kVRegVideoFormatCh4");
			DefineRegName	(kVRegVideoFormatCh5,					"kVRegVideoFormatCh5");
			DefineRegName	(kVRegVideoFormatCh6,					"kVRegVideoFormatCh6");
			DefineRegName	(kVRegVideoFormatCh7,					"kVRegVideoFormatCh7");
			DefineRegName	(kVRegVideoFormatCh8,					"kVRegVideoFormatCh8");
			DefineRegName	(kVRegIPAddrEth0,						"kVRegIPAddrEth0");
			DefineRegName	(kVRegSubnetEth0,						"kVRegSubnetEth0");
			DefineRegName	(kVRegGatewayEth0,						"kVRegGatewayEth0");
			DefineRegName	(kVRegIPAddrEth1,						"kVRegIPAddrEth1");
			DefineRegName	(kVRegSubnetEth1,						"kVRegSubnetEth1");
			DefineRegName	(kVRegGatewayEth1,						"kVRegGatewayEth1");
			DefineRegName	(kVRegRxcEnable1,						"kVRegRxcEnable1");
			DefineRegName	(kVRegRxcPrimaryRxMatch1,				"kVRegRxcPrimaryRxMatch1");
			DefineRegName	(kVRegRxcPrimarySourceIp1,				"kVRegRxcPrimarySourceIp1");
			DefineRegName	(kVRegRxcPrimaryDestIp1,				"kVRegRxcPrimaryDestIp1");
			DefineRegName	(kVRegRxcPrimarySourcePort1,			"kVRegRxcPrimarySourcePort1");
			DefineRegName	(kVRegRxcPrimaryDestPort1,				"kVRegRxcPrimaryDestPort1");
			DefineRegName	(kVRegRxcPrimarySsrc1,					"kVRegRxcPrimarySsrc1");
			DefineRegName	(kVRegRxcPrimaryVlan1,					"kVRegRxcPrimaryVlan1");
			DefineRegName	(kVRegRxcSecondaryRxMatch1,				"kVRegRxcSecondaryRxMatch1");
			DefineRegName	(kVRegRxcSecondarySourceIp1,			"kVRegRxcSecondarySourceIp1");
			DefineRegName	(kVRegRxcSecondaryDestIp1,				"kVRegRxcSecondaryDestIp1");
			DefineRegName	(kVRegRxcSecondarySourcePort1,			"kVRegRxcSecondarySourcePort1");
			DefineRegName	(kVRegRxcSecondaryDestPort1,			"kVRegRxcSecondaryDestPort1");
			DefineRegName	(kVRegRxcSecondarySsrc1,				"kVRegRxcSecondarySsrc1");
			DefineRegName	(kVRegRxcSecondaryVlan1,				"kVRegRxcSecondaryVlan1");
			DefineRegName	(kVRegRxcNetworkPathDiff1,				"kVRegRxcNetworkPathDiff1");
			DefineRegName	(kVRegRxcPlayoutDelay1,					"kVRegRxcPlayoutDelay1");
			DefineRegName	(kVRegRxcEnable2,						"kVRegRxcEnable2");
			DefineRegName	(kVRegRxcPrimaryRxMatch2,				"kVRegRxcPrimaryRxMatch2");
			DefineRegName	(kVRegRxcPrimarySourceIp2,				"kVRegRxcPrimarySourceIp2");
			DefineRegName	(kVRegRxcPrimaryDestIp2,				"kVRegRxcPrimaryDestIp2");
			DefineRegName	(kVRegRxcPrimarySourcePort2,			"kVRegRxcPrimarySourcePort2");
			DefineRegName	(kVRegRxcPrimaryDestPort2,				"kVRegRxcPrimaryDestPort2");
			DefineRegName	(kVRegRxcPrimarySsrc2,					"kVRegRxcPrimarySsrc2");
			DefineRegName	(kVRegRxcPrimaryVlan2,					"kVRegRxcPrimaryVlan2");
			DefineRegName	(kVRegRxcSecondaryRxMatch2,				"kVRegRxcSecondaryRxMatch2");
			DefineRegName	(kVRegRxcSecondarySourceIp2,			"kVRegRxcSecondarySourceIp2");
			DefineRegName	(kVRegRxcSecondaryDestIp2,				"kVRegRxcSecondaryDestIp2");
			DefineRegName	(kVRegRxcSecondarySourcePort2,			"kVRegRxcSecondarySourcePort2");
			DefineRegName	(kVRegRxcSecondaryDestPort2,			"kVRegRxcSecondaryDestPort2");
			DefineRegName	(kVRegRxcSecondarySsrc2,				"kVRegRxcSecondarySsrc2");
			DefineRegName	(kVRegRxcSecondaryVlan2,				"kVRegRxcSecondaryVlan2");
			DefineRegName	(kVRegRxcNetworkPathDiff2,				"kVRegRxcNetworkPathDiff2");
			DefineRegName	(kVRegRxcPlayoutDelay2,					"kVRegRxcPlayoutDelay2");
			DefineRegName	(kVRegTxcEnable3,						"kVRegTxcEnable3");
			DefineRegName	(kVRegTxcPrimaryLocalPort3,				"kVRegTxcPrimaryLocalPort3");
			DefineRegName	(kVRegTxcPrimaryRemoteIp3,				"kVRegTxcPrimaryRemoteIp3");
			DefineRegName	(kVRegTxcPrimaryRemotePort3,			"kVRegTxcPrimaryRemotePort3");
			DefineRegName	(kVRegTxcPrimaryRemoteMAC_lo3,			"kVRegTxcPrimaryRemoteMAC_lo3");
			DefineRegName	(kVRegTxcPrimaryRemoteMAC_hi3,			"kVRegTxcPrimaryRemoteMAC_hi3");
			DefineRegName	(kVRegTxcPrimaryAutoMac3,				"kVRegTxcPrimaryAutoMac3");
			DefineRegName	(kVRegTxcSecondaryLocalPort3,			"kVRegTxcSecondaryLocalPort3");
			DefineRegName	(kVRegTxcSecondaryRemoteIp3,			"kVRegTxcSecondaryRemoteIp3");
			DefineRegName	(kVRegTxcSecondaryRemotePort3,			"kVRegTxcSecondaryRemotePort3");
			DefineRegName	(kVRegTxcSecondaryRemoteMAC_lo3,		"kVRegTxcSecondaryRemoteMAC_lo3");
			DefineRegName	(kVRegTxcSecondaryRemoteMAC_hi3,		"kVRegTxcSecondaryRemoteMAC_hi3");
			DefineRegName	(kVRegTxcSecondaryAutoMac3,				"kVRegTxcSecondaryAutoMac3");
			DefineRegName	(kVRegTxcEnable4,						"kVRegTxcEnable4");
			DefineRegName	(kVRegTxcPrimaryLocalPort4,				"kVRegTxcPrimaryLocalPort4");
			DefineRegName	(kVRegTxcPrimaryRemoteIp4,				"kVRegTxcPrimaryRemoteIp4");
			DefineRegName	(kVRegTxcPrimaryRemotePort4,			"kVRegTxcPrimaryRemotePort4");
			DefineRegName	(kVRegTxcPrimaryRemoteMAC_lo4,			"kVRegTxcPrimaryRemoteMAC_lo4");
			DefineRegName	(kVRegTxcPrimaryRemoteMAC_hi4,			"kVRegTxcPrimaryRemoteMAC_hi4");
			DefineRegName	(kVRegTxcPrimaryAutoMac4,				"kVRegTxcPrimaryAutoMac4");
			DefineRegName	(kVRegTxcSecondaryLocalPort4,			"kVRegTxcSecondaryLocalPort4");
			DefineRegName	(kVRegTxcSecondaryRemoteIp4,			"kVRegTxcSecondaryRemoteIp4");
			DefineRegName	(kVRegTxcSecondaryRemotePort4,			"kVRegTxcSecondaryRemotePort4");
			DefineRegName	(kVRegTxcSecondaryRemoteMAC_lo4,		"kVRegTxcSecondaryRemoteMAC_lo4");
			DefineRegName	(kVRegTxcSecondaryRemoteMAC_hi4,		"kVRegTxcSecondaryRemoteMAC_hi4");
			DefineRegName	(kVRegTxcSecondaryAutoMac4,				"kVRegTxcSecondaryAutoMac4");
			DefineRegName	(kVRegMailBoxAcquire,					"kVRegMailBoxAcquire");
			DefineRegName	(kVRegMailBoxRelease,					"kVRegMailBoxRelease");
			DefineRegName	(kVRegMailBoxAbort,						"kVRegMailBoxAbort");
			DefineRegName	(kVRegMailBoxTimeoutNS,					"kVRegMailBoxTimeoutNS");
			DefineRegName	(kVReg2022_7Enable,						"kVReg2022_7Enable");
			DefineRegName	(kVRegUserDefinedDBB,					"kVRegUserDefinedDBB");
			DefineRegName	(kVRegHDMIOutAudioChannels,				"kVRegHDMIOutAudioChannels");
			DefineRegName	(kVRegHDMIOutRGBRange,					"kVRegHDMIOutRGBRange");
			DefineRegName	(kVRegLastAJA,							"kVRegLastAJA");
			DefineRegName	(kVRegFirstOEM,							"kVRegFirstOEM");

			for (ULWord ndx (0);  ndx < 1024;  ndx++)
			{
				ostringstream	oss;
				string			foo;
				oss << "VIRTUALREG_START+" << ndx;
				const ULWord	regNum	(VIRTUALREG_START + ndx);
				const string regName (oss.str());
				if (!regName.empty())
					if (mRegNumToStringMap.find (regNum) == mRegNumToStringMap.end())
					{
						mRegNumToStringMap.insert (RegNumToStringPair (regNum, regName));
						mStringToRegNumMap.insert (StringToRegNumPair (ToLower (regName), regNum));
					}
				DefineRegDecoder (regNum, mDefaultRegDecoder);
				DefineRegReadWrite (regNum, READWRITE);
				DefineRegClass (regNum, kRegClass_Virtual);
			}
		}	//	SetupVRegs
public:
		string RegNameToString (const uint32_t inRegNum) const
		{
			RegNumToStringMap::const_iterator	iter	(mRegNumToStringMap.find (inRegNum));
			if (iter != mRegNumToStringMap.end())
				return iter->second;
			ostringstream oss;
			oss << dec << "Register " << inRegNum << " (0x" << hex << inRegNum << ")";
			return oss.str();
		}

		string RegValueToString (const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			RegNumToDecoderMap::const_iterator	iter	(mRegNumToDecoderMap.find (inRegNum));
			ostringstream	oss;
			if (iter != mRegNumToDecoderMap.end()  &&  iter->second)
			{
				const Decoder * pDecoder (iter->second);
				oss << (*pDecoder)(inRegNum, inRegValue, inDeviceID);
			}
			return oss.str();
		}

		bool	IsRegInClass (const uint32_t inRegNum, const string & inClassName) const
		{
			for (RegClassToRegNumConstIter	it	(mRegClassToRegNumMap.find (inClassName));  it != mRegClassToRegNumMap.end() && it->first == inClassName;  ++it)
				if (it->second == inRegNum)
					return true;
			return false;
		}

		inline bool		IsRegisterWriteOnly (const uint32_t inRegNum) const				{return IsRegInClass (inRegNum, kRegClass_WriteOnly);}
		inline bool		IsRegisterReadOnly (const uint32_t inRegNum) const				{return IsRegInClass (inRegNum, kRegClass_ReadOnly);}

		NTV2StringSet	GetAllRegisterClasses (void) const
		{
			if (mAllRegClasses.empty())
				for (RegClassToRegNumConstIter	it	(mRegClassToRegNumMap.begin ());  it != mRegClassToRegNumMap.end();  ++it)
					if (mAllRegClasses.find (it->first) == mAllRegClasses.end())
						mAllRegClasses.insert (it->first);
			return mAllRegClasses;
		}

		NTV2StringSet	GetRegisterClasses (const uint32_t inRegNum) const
		{
			NTV2StringSet	result;
			NTV2StringSet	allClasses	(GetAllRegisterClasses());
			for (NTV2StringSetConstIter	it	(allClasses.begin ());  it != allClasses.end();  ++it)
				if (IsRegInClass (inRegNum, *it))
					result.insert (*it);
			return result;
		}

		NTV2RegNumSet	GetRegistersForClass (const string & inClassName) const
		{
			NTV2RegNumSet	result;
			for (RegClassToRegNumConstIter	it	(mRegClassToRegNumMap.find (inClassName));  it != mRegClassToRegNumMap.end() && it->first == inClassName;  ++it)
				if (result.find (it->second) == result.end())
					result.insert (it->second);
			return result;
		}

		NTV2RegNumSet	GetRegistersForDevice (const NTV2DeviceID inDeviceID, const bool inIncludeVirtuals) const
		{
			NTV2RegNumSet		result;
			const uint32_t		maxRegNum	(::NTV2DeviceGetMaxRegisterNumber (inDeviceID));
			for (uint32_t regNum (0);  regNum <= maxRegNum;  regNum++)
				result.insert (regNum);
			if (::NTV2DeviceCanDoCustomAnc (inDeviceID))
			{
				const NTV2RegNumSet	ancRegs	(GetRegistersForClass (kRegClass_Anc));
				for (NTV2RegNumSetConstIter it (ancRegs.begin());  it != ancRegs.end();  ++it)
					result.insert (*it);
			}
			if (::NTV2DeviceCanDoSDIErrorChecks (inDeviceID))
			{
				const NTV2RegNumSet	ancRegs	(GetRegistersForClass (kRegClass_SDIError));
				for (NTV2RegNumSetConstIter it (ancRegs.begin());  it != ancRegs.end();  ++it)
					result.insert (*it);
			}
			if (inIncludeVirtuals)
			{
				const NTV2RegNumSet	vRegs	(GetRegistersForClass (kRegClass_Virtual));
				for (NTV2RegNumSetConstIter it (vRegs.begin());  it != vRegs.end();  ++it)
					result.insert (*it);
			}
			return result;
		}

		NTV2RegNumSet	GetRegistersWithName (const string & inName, const int inMatchStyle = EXACTMATCH) const
		{
			NTV2RegNumSet	result;
			const string	nameStr		(ToLower (inName));
			const size_t	nameStrLen	(nameStr.length());
			StringToRegNumConstIter it;
			if (inMatchStyle == EXACTMATCH)
			{
				it = mStringToRegNumMap.find (nameStr);
				if (it != mStringToRegNumMap.end())
					result.insert (it->second);
				return result;
			}
			//	Inexact match...
			for (it = mStringToRegNumMap.begin();  it != mStringToRegNumMap.end();  ++it)
			{
				const size_t	pos	(it->first.find (nameStr));
				if (pos == string::npos)
					continue;
				switch (inMatchStyle)
				{
					case CONTAINS:		result.insert (it->second);					break;
					case STARTSWITH:	if (pos == 0)	result.insert (it->second);	break;
					case ENDSWITH:		if (pos + nameStrLen == it->first.length())	result.insert (it->second);	break;
					default:			break;
				}
			}
			return result;
		}

		bool		GetXptRegNumAndMaskIndex (const NTV2InputCrosspointID inInputXpt, uint32_t & outXptRegNum, uint32_t & outMaskIndex) const
		{
			outXptRegNum = 0xFFFFFFFF;
			outMaskIndex = 0xFFFFFFFF;
			InputXpt2XptRegNumMaskIndexMapConstIter	iter	(mInputXpt2XptRegNumMaskIndexMap.find (inInputXpt));
			if (iter == mInputXpt2XptRegNumMaskIndexMap.end())
				return false;
			outXptRegNum = iter->second.first;
			outMaskIndex = iter->second.second;
			return true;
		}

		NTV2InputCrosspointID	GetInputCrosspointID (const uint32_t inXptRegNum, const uint32_t inMaskIndex) const
		{
			const XptRegNumAndMaskIndex				key		(inXptRegNum, inMaskIndex);
			XptRegNumMaskIndex2InputXptMapConstIter	iter	(mXptRegNumMaskIndex2InputXptMap.find (key));
			if (iter != mXptRegNumMaskIndex2InputXptMap.end())
				return iter->second;
			return NTV2_INPUT_CROSSPOINT_INVALID;
		}

		ostream &	Print (ostream & inOutStream) const
		{
			static const string		sLineBreak	(96, '=');
			static const uint32_t	sMasks[4]	=	{0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000};

			inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of RegNumToStringMap:  " << mRegNumToStringMap.size() << " mappings:" << endl << sLineBreak << endl;
			for (RegNumToStringMap::const_iterator it (mRegNumToStringMap.begin());  it != mRegNumToStringMap.end();  ++it)
				inOutStream	<< "reg " << setw(5) << it->first << "(" << HEX0N(it->first,8) << dec << ")  =>  '" << it->second << "'" << endl;

			inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of RegNumToDecoderMap:  " << mRegNumToDecoderMap.size() << " mappings:" << endl << sLineBreak << endl;
			for (RegNumToDecoderMap::const_iterator it (mRegNumToDecoderMap.begin());  it != mRegNumToDecoderMap.end();  ++it)
				inOutStream	<< "reg " << setw(5) << it->first << "(" << HEX0N(it->first,8) << dec << ")  =>  " << (it->second == &mDefaultRegDecoder ? "(default decoder)" : "Custom Decoder") << endl;

			inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of RegClassToRegNumMap:  " << mRegClassToRegNumMap.size() << " mappings:" << endl << sLineBreak << endl;
			for (RegClassToRegNumMap::const_iterator it (mRegClassToRegNumMap.begin());  it != mRegClassToRegNumMap.end();  ++it)
				inOutStream	<< setw(32) << it->first << "  =>  reg " << setw(5) << it->second << "(" << HEX0N(it->second,8) << dec << ") " << RegNameToString(it->second) << endl;

			inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of StringToRegNumMap:  " << mStringToRegNumMap.size() << " mappings:" << endl << sLineBreak << endl;
			for (StringToRegNumMap::const_iterator it (mStringToRegNumMap.begin());  it != mStringToRegNumMap.end();  ++it)
				inOutStream	<< setw(32) << it->first << "  =>  reg " << setw(5) << it->second << "(" << HEX0N(it->second,8) << dec << ") " << RegNameToString(it->second) << endl;

			inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of InputXpt2XptRegNumMaskIndexMap:  " << mInputXpt2XptRegNumMaskIndexMap.size() << " mappings:" << endl << sLineBreak << endl;
			for (InputXpt2XptRegNumMaskIndexMap::const_iterator it (mInputXpt2XptRegNumMaskIndexMap.begin());  it != mInputXpt2XptRegNumMaskIndexMap.end();  ++it)
				inOutStream	<< setw(32) << ::NTV2InputCrosspointIDToString(it->first) << "(" << HEX0N(it->first,2)
							<< ")  =>  reg " << setw(3) << it->second.first << "(" << HEX0N(it->second.first,3) << dec << "|" << setw(20) << RegNameToString(it->second.first)
							<< ") mask " << it->second.second << "(" << HEX0N(sMasks[it->second.second],8) << ")" << endl;

			inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of XptRegNumMaskIndex2InputXptMap:  " << mXptRegNumMaskIndex2InputXptMap.size() << " mappings:" << endl << sLineBreak << endl;
			for (XptRegNumMaskIndex2InputXptMap::const_iterator it (mXptRegNumMaskIndex2InputXptMap.begin());  it != mXptRegNumMaskIndex2InputXptMap.end();  ++it)
				inOutStream	<< "reg " << setw(3) << it->first.first << "(" << HEX0N(it->first.first,4) << "|" << setw(20) << RegNameToString(it->first.first)
							<< ") mask " << it->first.second << "(" << HEX0N(sMasks[it->first.second],8) << ")  =>  "
							<< setw(27) << ::NTV2InputCrosspointIDToString(it->second) << "(" << HEX0N(it->second,2) << ")" << endl;
			return inOutStream;
		}

	private:
		typedef std::map<uint32_t, string>	RegNumToStringMap;
		typedef std::pair<uint32_t, string>	RegNumToStringPair;

		static string ToLower (const string & inStr)
		{
			string	result (inStr);
			std::transform (result.begin (), result.end (), result.begin (), ::tolower);
			return result;
		}

		//	This class implements a functor that returns a string that contains a human-readable decoding
		//	of a register value, given its number and the ID of the device it came from.
		struct Decoder
		{
			//	This functor returns a string that contains a human-readable decoding of a register number, its value,
			//	and the ID of the device it was read from.
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inRegValue;
				(void) inDeviceID;
				return string();
			}
		} mDefaultRegDecoder;

		struct DecodeGlobalControlReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				const NTV2FrameGeometry		frameGeometry		(NTV2FrameGeometry		(((inRegValue & kRegMaskGeometry      ) >>  3)));
				const NTV2Standard			videoStandard		(NTV2Standard			((inRegValue & kRegMaskStandard       ) >>  7));
				const NTV2ReferenceSource	referenceSource		(NTV2ReferenceSource	((inRegValue & kRegMaskRefSource      ) >> 10));
				const NTV2RegisterWriteMode	registerWriteMode	(NTV2RegisterWriteMode	((inRegValue & kRegMaskRegClocking    ) >> 20));
				const NTV2FrameRate			frameRate			(NTV2FrameRate			(((inRegValue & kRegMaskFrameRate     ) >> kRegShiftFrameRate)
																					   | ((inRegValue & kRegMaskFrameRateHiBit) >> (kRegShiftFrameRateHiBit - 3))));
				ostringstream	oss;
				oss	<< "Frame Rate: "				<< ::NTV2FrameRateToString (frameRate, true)				<< endl
					<< "Frame Geometry: "			<< ::NTV2FrameGeometryToString (frameGeometry, true)		<< endl
					<< "Standard: "					<< ::NTV2StandardToString (videoStandard, true)				<< endl
					<< "Reference Source: "			<< ::NTV2ReferenceSourceToString (referenceSource, true)	<< endl
					<< "Ch 2 link B 1080p 50/60: "	<< ((inRegValue & kRegMaskSmpte372Enable) ? "On" : "Off")	<< endl
					<< "LEDs ";
				for (int led(0);  led < 4;  ++led)
					oss	<< (((inRegValue & kRegMaskLED) >> (16 + led))  ?  "*"  :  ".");
				oss	<< endl
					<< "Register Clocking: "		<< ::NTV2RegisterWriteModeToString (registerWriteMode, true).c_str() << endl
					<< "Ch 1 RP-188 output: "		<< (inRegValue & kRegMaskRP188ModeCh1 ? "Enabled" : "Disabled") << endl
					<< "Ch 2 RP-188 output: "		<< (inRegValue & kRegMaskRP188ModeCh2 ? "Enabled" : "Disabled") << endl
					<< "Color Correction: "			<< "Channel: " << ((inRegValue & BIT(31)) ? "2" : "1")
													<< " Bank " << ((inRegValue & BIT (30)) ? "1" : "0");
				return oss.str();
			}
		}	mDecodeGlobalControlReg;

		struct DecodeGlobalControl2Reg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				oss << "Reference source bit 4: "		<< SetNotset(inRegValue & kRegMaskRefSource2)		<< endl
					<< "Channel 1-4 Quad: "				<< SetNotset(inRegValue & kRegMaskQuadMode)			<< endl
					<< "Channel 5-8 Quad: "				<< SetNotset(inRegValue & kRegMaskQuadMode2)		<< endl
					<< "Independent Channel Mode: "		<< SetNotset(inRegValue & kRegMaskIndependentMode)	<< endl
					<< "2M frame support: "				<< (inRegValue & kRegMask2MFrameSupport		? "Supported" : "Not Supported") << endl
					<< "Audio 1 play/capture mode: "	<< OnOff(inRegValue & kRegMaskAud1PlayCapMode)		<< endl
					<< "Audio 2 play/capture mode: "	<< OnOff(inRegValue & kRegMaskAud2PlayCapMode)		<< endl
					<< "Audio 3 play/capture mode: "	<< OnOff(inRegValue & kRegMaskAud3PlayCapMode)		<< endl
					<< "Audio 4 play/capture mode: "	<< OnOff(inRegValue & kRegMaskAud4PlayCapMode)		<< endl
					<< "Audio 5 play/capture mode: "	<< OnOff(inRegValue & kRegMaskAud5PlayCapMode)		<< endl
					<< "Audio 6 play/capture mode: "	<< OnOff(inRegValue & kRegMaskAud6PlayCapMode)		<< endl
					<< "Audio 7 play/capture mode: "	<< OnOff(inRegValue & kRegMaskAud7PlayCapMode)		<< endl
					<< "Audio 8 play/capture mode: "	<< OnOff(inRegValue & kRegMaskAud8PlayCapMode)		<< endl
					<< "Ch 3 RP-188 output: "			<< EnabDisab(inRegValue & kRegMaskRP188ModeCh3)		<< endl
					<< "Ch 4 RP-188 output: "			<< EnabDisab(inRegValue & kRegMaskRP188ModeCh4)		<< endl
					<< "Ch 5 RP-188 output: "			<< EnabDisab(inRegValue & kRegMaskRP188ModeCh5)		<< endl
					<< "Ch 6 RP-188 output: "			<< EnabDisab(inRegValue & kRegMaskRP188ModeCh6)		<< endl
					<< "Ch 7 RP-188 output: "			<< EnabDisab(inRegValue & kRegMaskRP188ModeCh7)		<< endl
					<< "Ch 8 RP-188 output: "			<< EnabDisab(inRegValue & kRegMaskRP188ModeCh8)		<< endl
					<< "Ch 1/2 425: "					<< EnabDisab(inRegValue & kRegMask425FB12)			<< endl
					<< "Ch 3/4 425: "					<< EnabDisab(inRegValue & kRegMask425FB34)			<< endl
					<< "Ch 5/6 425: "					<< EnabDisab(inRegValue & kRegMask425FB56)			<< endl
					<< "Ch 7/8 425: "					<< EnabDisab(inRegValue & kRegMask425FB78);
				return oss.str();
			}
		}	mDecodeGlobalControl2;

		struct DecodeChannelControlReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				oss	<< "Mode: "		<< (inRegValue & kRegMaskMode ? "Capture" : "Display") << endl
					<< "Format: ";
				if (inRegValue & kRegMaskFrameFormatHiBit)
				{
					if ((inRegValue & kRegMaskFrameFormat) == 0)
						oss << "16-bit RGB (dual-link only)";
					else
						oss << "?? " << uint16_t (inRegValue & kRegMaskFrameFormat) << "(0x" << hex << uint16_t (inRegValue & kRegMaskFrameFormat) << dec << ")";
				}
				else switch ((inRegValue & kRegMaskFrameFormat) >> 1)
				{
					case 0:		oss << "10-bit YCbCr";					break;
					case 1:		oss << "8-bit YCbCr(UYVY)";				break;
					case 2:		oss << "8-bit ARGB";					break;
					case 3:		oss << "8-bit RGBA";					break;
					case 4:		oss << "10-bit RGB";					break;
					case 5:		oss << "8-bit YCbCr(YUY2)";				break;
					case 6:		oss << "8-bit ABGR";					break;
					case 7:		oss << "DPX 10-bit ABGR";				break;
					case 8:		oss << "DPX YCbCr";						break;
					case 9:		oss << "DVCPro [8-bit YCbCr (UYVY)]";	break;
					case 10:	oss << "10(0xA) -- reserved";			break;
					case 11:	oss << "HDV 8-bit YCbCr";				break;
					case 12:	oss << "8-bit packed RGB";				break;
					case 13:	oss << "8-bit packed BGR";				break;
					case 14:	oss << "DPX 10-bit RGB, little endian";	break;
					case 15:	oss << "10-bit ARGB";					break;
					case 16:	oss << "16-bit ARGB";					break;
				}
				oss								<< (inRegValue & kRegMaskFrameFormatHiBit	? " (extended)"			: " (normal)")					<< endl
					<< "Channel: "				<< (inRegValue & kRegMaskChannelDisable		? "Disabled"			: "Enabled")					<< endl
					<< "Viper Squeeze: "		<< (inRegValue & BIT(9)						? "Squeeze"				: "Normal")						<< endl
					<< "Flip Vertical: "		<< (inRegValue & kRegMaskFrameOrientation	? "Upside Down"			: "Normal")						<< endl
					<< "DRT Display: "			<< (inRegValue & kRegMaskQuarterSizeMode	? "On"					: "Off")						<< endl
					<< "Frame Buffer Mode: "	<< (inRegValue & kRegMaskFrameBufferMode	? "Field"				: "Frame")						<< endl
					<< "Dither: "				<< (inRegValue & kRegMaskDitherOn8BitInput	? "Dither 8-bit inputs"	: "No dithering")				<< endl
					<< "Frame Size: "			<< (1 << (((inRegValue & kK2RegMaskFrameSize) >> 20) + 1)) << " MB"									<< endl
					<< "RGB Range: "			<< (inRegValue & BIT(24)					? "Black = 0x40"		: "Black = 0")					<< endl
					<< "VANC Data Shift: "		<< (inRegValue & kRegMaskVidProcVANCShift	? "Enabled"				: "Normal 8 bit conversion");
				return oss.str();
			}
		}	mDecodeChannelControl;

		struct DecodeFBControlReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				const bool		isOn	((inRegValue & (1 << 29)) != 0);
				const uint16_t	format	((inRegValue >> 15) & 0x1F);
				ostringstream	oss;
				oss << "\t" << OnOff(isOn) << endl
					<< "\tFormat=0x" << hex << format << dec << "(" << format << ")";
				return oss.str();
			}
		}	mDecodeFBControlReg;

		struct DecodeSysmonVccIntDieTemp : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				const UWord		rawDieTemp	((inRegValue & 0x0000FFFF) >> 6);
				const UWord		rawVoltage	((inRegValue >> 22) & 0x3FF);
				const double	dieTempC	((double(rawDieTemp)) * 503.975 / 1024.0 - 273.15 );
				const double	dieTempF	(dieTempC * 9.0 / 5.0  +  32.0);
				const double	voltage		(double(rawVoltage)/ 1024.0 * 3.0);
				ostringstream	oss;
				oss << "\tDie Temperature: " << fDEC(dieTempC,5,2) << " Celcius  (" << fDEC(dieTempF,5,2) << " Fahrenheit"	<< endl
					<< "\tCore Voltage: " << fDEC(voltage,5,2) << " Volts DC";
				return oss.str();
			}
		}	mDecodeSysmonVccIntDieTemp;

		struct DecodeVidControlReg : public Decoder		//	Bit31=Is16x9 | Bit30=IsMono
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				const bool		is16x9	((inRegValue & (1 << 31)) != 0);
				const bool		isMono	((inRegValue & (1 << 30)) != 0);
				ostringstream	oss;
				oss << "\tAspect Ratio: " << (is16x9 ? "16x9" : "4x3") << endl
					<< "\tDepth: " << (isMono ? "Monochrome" : "Color");
				return oss.str();
			}
		}	mDecodeVidControlReg;

		struct DecodeAudDetectReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inDeviceID;
				ostringstream	oss;
				switch (inRegNum)
				{
					case kRegAud1Detect:
					case kRegAudDetect2:
						for (uint16_t num(0);  num < 8;  )
						{
							const uint16_t	group		(num / 2);
							const bool		isChan34	(num & 1);
							oss		<< "Group " << group << " CH " << (isChan34 ? "3-4: " : "1-2: ") << (inRegValue & BIT(num) ? "Present" : "Absent");
							if (++num < 8)
								oss << endl;
						}
						break;

					case kRegAudioDetect5678:
						break;
				}
				return oss.str();
			}
		}	mDecodeAudDetectReg;

		struct DecodeAudControlReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				static const string	ChStrs []	=	{	"Ch 1/2",	"Ch 3/4",	"Ch 5/6",	"Ch 7/8"	};
				uint16_t			sdiOutput	(0);
				switch (inRegNum)
				{	case kRegAud1Control:	sdiOutput = 1;	break;
					case kRegAud3Control:	sdiOutput = 3;	break;
					case kRegAud5Control:	sdiOutput = 5;	break;
					case kRegAud7Control:	sdiOutput = 7;	break;
					default:								break;
				}

				ostringstream		oss;
				oss		<< "Audio Capture: "		<< (BIT(0) & inRegValue ? "Enabled" : "Disabled")	<< endl
						<< "Audio Loopback: "		<< (BIT(3) & inRegValue ? "Enabled" : "Disabled")	<< endl
						<< "Audio Input: "			<< (BIT(8) & inRegValue ? "Disabled" : "Enabled")	<< endl
						<< "Audio Output: "			<< (BIT(9) & inRegValue ? "Disabled" : "Enabled")	<< endl;
				if (sdiOutput)
					oss	<< "Audio Embedder SDIOut" << sdiOutput		<< ": " << (BIT(13) & inRegValue ? "Disabled" : "Enabled")	<< endl
						<< "Audio Embedder SDIOut" << (sdiOutput+1)	<< ": " << (BIT(15) & inRegValue ? "Disabled" : "Enabled")	<< endl;

				oss		<< "A/V Sync Mode: "		<< (BIT(15) & inRegValue ? "Enabled" : "Disabled")	<< endl
						<< "AES Rate Converter: "	<< (BIT(19) & inRegValue ? "Disabled" : "Enabled")	<< endl
						<< "Audio Buffer Format: "	<< (BIT(20) & inRegValue ? "16-Channel " : (BIT(16) & inRegValue ? "8-Channel " : "6-Channel "))	<< endl
						<< (BIT(18) & inRegValue ? "96kHz" : "48kHz")									<< endl
						<< (BIT(18) & inRegValue ? "96kHz Support" : "48kHz Support")					<< endl
						<< (BIT(22) & inRegValue ? "Embedded Support" : "No Embedded Support")			<< endl
						<< (BIT(23) & inRegValue ? "8-Channel Support" : "6-Channel Support")			<< endl
						<< "K-box, Monitor: "		<< ChStrs [(BIT(24) & BIT(25) & inRegValue) >> 24]	<< endl
						<< "K-Box Input: "			<< (BIT(26) & inRegValue ? "XLR" : "BNC")			<< endl
						<< "K-Box: "				<< (BIT(27) & inRegValue ? "Present" : "Absent")	<< endl
						<< "Cable: "				<< (BIT(28) & inRegValue ? "XLR" : "BNC")			<< endl
						<< "Audio Buffer Size: "	<< (BIT(31) & inRegValue ? "4 MB" : "1 MB");
				return oss.str();
			}
		}	mDecodeAudControlReg;

		struct DecodeAudSourceSelectReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				static const string		SrcStrs []		=	{	"AES Input",	"Embedded Groups 1 and 2",	""	};
				static const unsigned	SrcStrMap []	=	{	0,	1,	2,	2,	2,	1,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2	};
				const uint16_t			vidInput		=	(inRegValue & BIT(23) ? 2 : 0)  +  (inRegValue & BIT(16) ? 1 : 0);
															//	WARNING!  BIT(23) had better be clear on 0 & 1-input boards!!
				ostringstream	oss;
				oss	<< "Audio Source: "	<< SrcStrs [SrcStrMap [(BIT(0) | BIT(1) | BIT(2) | BIT(3)) & inRegValue]]	<< endl
					<< "Embedded Source Select: Video Input " << (1 + vidInput)										<< endl
					<< "PCM disabled: "				<< YesNo(inRegValue & BIT(17))									<< endl
					<< "Erase head enable: "		<< YesNo(inRegValue & BIT(19))									<< endl
					<< "Embedded Clock Select: "	<< (inRegValue & BIT(22) ? "Video Input" : "Board Reference")	<< endl
					<< "3G audio source: "			<< (inRegValue & BIT(21) ? "Data stream 2" : "Data stream 1");
				return oss.str();
			}
		}	mDecodeAudSourceSelectReg;

		struct DecodePCMControlReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inDeviceID;
				ostringstream	oss;
				const UWord		startAudioSystem (inRegNum == kRegPCMControl4321  ?  1  :  5);
				for (uint8_t audChan (0);  audChan < 4;  audChan++)
				{
					oss << "Audio System " << (startAudioSystem + audChan) << ": ";
					const uint8_t	pcmBits	(uint32_t(inRegValue >> (audChan * 8)) & 0x000000FF);
					if (pcmBits == 0x00)
						oss << "normal";
					else
					{
						oss << "non-PCM channels";
						for (uint8_t chanPair (0);  chanPair < 8;  chanPair++)
							if (pcmBits & (0x01 << chanPair))
								oss << "  " << (chanPair*2+1) << "-" << (chanPair*2+2);
					}
					if (audChan < 3)
						oss << endl;
				}
				return oss.str();
			}
		}	mDecodePCMControlReg;

		struct DecodeAncExtControlReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				static const string	SyncStrs []	=	{	"field",	"frame",	"immediate",	"unknown"	};
				oss	<< "HANC Y enable: "		<< YesNo(inRegValue & BIT( 0))	<< endl
					<< "VANC Y enable: "		<< YesNo(inRegValue & BIT( 4))	<< endl
					<< "HANC C enable: "		<< YesNo(inRegValue & BIT( 8))	<< endl
					<< "VANC C enable: "		<< YesNo(inRegValue & BIT(12))	<< endl
					<< "Progressive video: "	<< YesNo(inRegValue & BIT(16))	<< endl
					<< "Synchronize: "			<< SyncStrs [(inRegValue & (BIT(24) | BIT(25))) >> 24]	<< endl
					<< "Memory writes "			<< (inRegValue & BIT(28) ? "disabled" : "enabled")	<< endl
					<< "Metadata from "			<< (inRegValue & BIT(31) ? "LSBs" : "MSBs");
				return oss.str();
			}
		}	mDecodeAncExtControlReg;

		struct DecodeAncExtFieldLinesReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inDeviceID;
				ostringstream	oss;
				const uint32_t	which		(inRegNum & 0x1F);
				const uint32_t	valueLow	(inRegValue & 0x7FF);
				const uint32_t	valueHigh	((inRegValue >> 16) & 0x7FF);
				switch (which)
				{
					case 5:		oss << "F1 cutoff line: "			<< valueLow	<< endl		//	regAncExtFieldCutoffLine
									<< "F2 cutoff line: "			<< valueHigh;
								break;
					case 9:		oss	<< "F1 VBL start line: "		<< valueLow	<< endl		//	regAncExtFieldVBLStartLine
									<< "F2 VBL start line: "		<< valueHigh;
								break;
					case 11:	oss	<< "Field ID high on line: "	<< valueLow	<< endl		//	regAncExtFID
									<< "Field ID low on line: "		<< valueHigh;
								break;
					case 17:	oss	<< "F1 analog start line: "		<< valueLow	<< endl		//	regAncExtAnalogStartLine
									<< "F2 analog start line: "		<< valueHigh;
								break;
					default:
								oss	<< "Invalid register type";
								break;
				}
				return oss.str();
			}
		}	mDecodeAncExtFieldLines;

		struct DecodeAncExtStatusReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inDeviceID;
				ostringstream	oss;
				const uint32_t	which		(inRegNum & 0x1F);
				const uint32_t	byteTotal	(inRegValue & 0xFFFF);
				const bool		overrun		((inRegValue & BIT(28)) ? true : false);
				switch (which)
				{
					case 6:		oss	<<	"Total bytes: ";		break;
					case 7:		oss	<< "Total F1 bytes: ";		break;
					case 8:		oss	<< "Total F2 bytes: ";		break;
					default:	return "Invalid register type";	break;
				}
				oss	<< byteTotal	<< endl
					<< "Overrun: "	<< (overrun ? "yes" : "no");
				return oss.str();
			}
		}	mDecodeAncExtStatus;

		struct DecodeAncExtIgnoreDIDReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				oss	<< "Ignoring DIDs " << HEX0N((inRegValue >> 0) & 0xFF, 2)
					<< ", " << HEX0N((inRegValue >> 8) & 0xFF, 2)
					<< ", " << HEX0N((inRegValue >> 16) & 0xFF, 2)
					<< ", " << HEX0N((inRegValue >> 24) & 0xFF, 2);
				return oss.str();
			}
		}	mDecodeAncExtIgnoreDIDs;

		struct DecodeAncExtAnalogFilterReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegValue;
				(void) inDeviceID;
				ostringstream	oss;
				uint32_t		which	(inRegNum & 0x1F);
				oss	<< "Each 1 bit specifies capturing ";
				switch (which)
				{
					case 18:	oss << "F1 Y";		break;
					case 19:	oss << "F2 Y";		break;
					case 20:	oss << "F1 C";		break;
					case 21:	oss << "F2 C";		break;
					default:	return "Invalid register type";
				}
				oss << " line as analog, else digital";
				return oss.str();
			}
		}	mDecodeAncExtAnalogFilter;

		struct DecodeAncInsValuePairReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inDeviceID;
				ostringstream	oss;
				const uint32_t	which		(inRegNum & 0x1F);
				const uint32_t	valueLow	(inRegValue & 0xFFFF);
				const uint32_t	valueHigh	((inRegValue >> 16) & 0xFFFF);

				switch (which)
				{
					case 0:		oss	<< "F1 byte count: "				<< valueLow				<< endl
									<< "F2 byte count: "				<< valueHigh;
								break;
					case 4:		oss	<< "HANC pixel delay: "				<< (valueLow & 0x3FF)	<< endl
									<< "VANC pixel delay: "				<< (valueHigh & 0x7FF);
								break;
					case 5:		oss	<< "F1 first active line: "			<< (valueLow & 0x7FF)	<< endl
									<< "F2 first active line: "			<< (valueHigh & 0x7FF);
								break;
					case 6:		oss	<< "Active line length: "			<< (valueLow & 0x7FF)	<< endl
									<< "Total line length: "			<< (valueHigh & 0xFFF);
								break;
					case 8:		oss	<< "Field ID high on line: "		<< (valueLow & 0x7FF)	<< endl
									<< "Field ID low on line: "			<< (valueHigh & 0x7FF);
								break;
					case 11:	oss	<< "F1 chroma blnk start line: "	<< (valueLow & 0x7FF)	<< endl
									<< "F2 chroma blnk start line: "	<< (valueHigh & 0x7FF);
								break;
					default:	return "Invalid register type";
				}
				return oss.str();
			}
		}	mDecodeAncInsValuePairReg;

		struct DecodeAncInsControlReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				oss	<< "HANC Y enable: "		<< YesNo(inRegValue & BIT( 0))	<< endl
					<< "VANC Y enable: "		<< YesNo(inRegValue & BIT( 4))	<< endl
					<< "HANC C enable: "		<< YesNo(inRegValue & BIT( 8))	<< endl
					<< "VANC C enable: "		<< YesNo(inRegValue & BIT(12))	<< endl
					<< "Payload Y insert: "		<< YesNo(inRegValue & BIT(16))	<< endl
					<< "Payload C insert: "		<< YesNo(inRegValue & BIT(17))	<< endl
					<< "Payload F1 insert: "	<< YesNo(inRegValue & BIT(20))	<< endl
					<< "Payload F2 insert: "	<< YesNo(inRegValue & BIT(21))	<< endl
					<< "Progressive video: "	<< YesNo(inRegValue & BIT(24))	<< endl
					<< "Memory writes: "		<< ((inRegValue & BIT(28)) ? "disabled" : "enabled");
				return oss.str();
			}
		}	mDecodeAncInsControlReg;

		struct DecodeAncInsChromaBlankReg : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegValue;
				(void) inDeviceID;
				ostringstream	oss;
				uint32_t		which	(inRegNum & 0x1F);

				oss	<< "Each 1 bit specifies if chroma in ";
				switch (which)
				{
					case 12:	oss	<< "F1";	break;
					case 13:	oss	<< "F2";	break;
					default:	return "Invalid register type";
				}
				oss	<< " should be blanked or passed thru";
				return oss.str();
			}
		}	mDecodeAncInsChromaBlankReg;

		struct DecodeXptGroupReg : public Decoder		//	Every byte is a crosspoint number 0-255
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				static unsigned	sShifts[4]	= {0, 8, 16, 24};
				ostringstream	oss;
				for (unsigned ndx(0);  ndx < 4;  ndx++)
				{
					const NTV2InputCrosspointID		inputXpt	(CNTV2RegisterExpert::GetInputCrosspointID (inRegNum, ndx));
					const NTV2OutputCrosspointID	outputXpt	(static_cast <NTV2OutputCrosspointID> ((inRegValue >> sShifts[ndx]) & 0xFF));
					if (NTV2_IS_VALID_InputCrosspointID (inputXpt))
						oss << ::NTV2InputCrosspointIDToString (inputXpt, false) << " <== " << ::NTV2OutputCrosspointIDToString (outputXpt, false);
					if (ndx < 3)
						oss << endl;
				}
				return oss.str();
			}
		}	mDecodeXptGroupReg;

		struct DecodeHDMIOutputControl : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				ostringstream	oss;
				static const ULWord	sMasks[]		=	{	0,	kRegMaskHDMIOutVideoStd,	kRegMaskHDMIOutV2VideoStd,	kRegMaskHDMIOutV2VideoStd,	0};
				static const string	sHDMIStdV1[]	=	{	"1080i",	"720p",	"480i",	"576i",	"1080p",	"SXGA",	""	};
				static const string	sHDMIStdV2V3[]	=	{	"1080i",	"720p",	"480i",	"576i",	"1080p",	"1556i",	"2Kx1080p",	"2Kx1080i",	"UHD",	"4K",	""	};
				static const string	sVidRates[]		=	{	"",	"60.00",	"59.94",	"30.00",	"29.97",	"25.00",	"24.00",	"23.98",	"",	"",	""	};
				const ULWord	hdmiVers		(::NTV2DeviceGetHDMIVersion(inDeviceID) & 0x00000003);
				const ULWord	rawVideoStd		(inRegValue & sMasks[hdmiVers]);
				const string	hdmiVidStdStr	(hdmiVers == 2 || hdmiVers == 3 ? sHDMIStdV2V3[rawVideoStd] : (hdmiVers == 1 ? sHDMIStdV1[rawVideoStd] : ""));
				const string	vidStdStr		(::NTV2StandardToString (NTV2Standard(rawVideoStd), true));
				oss << "Video Standard: " << hdmiVidStdStr;
				if (hdmiVidStdStr != vidStdStr)
					oss << " (" << vidStdStr << ")";
				oss	<< endl
					<< "Color Mode: "		<< ((inRegValue & BIT( 8))	? "RGB"			: "YCbCr")		<< endl
					<< "Video Rate: "		<< sVidRates[(inRegValue & kLHIRegMaskHDMIOutFPS) >> kLHIRegShiftHDMIOutFPS]  << endl
					<< "Scan Mode: "		<< ((inRegValue & BIT(13))	? "Progressive"	: "Interlaced")	<< endl
					<< "Bit Depth: "		<< ((inRegValue & BIT(14))	? "10-bit"		: "8-bit")		<< endl
					<< "Color Sampling: "	<< ((inRegValue & BIT(15))	? "4:4:4"		: "4:2:2")		<< endl
					<< "Output Range: "		<< ((inRegValue & BIT(28))	? "Full"		: "SMPTE")		<< endl
					<< "Audio Channels: "	<< ((inRegValue & BIT(29))	? "8"			: "2")			<< endl
					<< "Output: "			<< ((inRegValue & BIT(30))	? "DVI"			: "HDMI")		<< endl
					<< "Audio Loopback: "	<< OnOff(inRegValue & BIT(31));
				return oss.str();
			}
		}	mDecodeHDMIOutputControl;

		struct DecodeHDMIInputStatus : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				ostringstream	oss;
				const ULWord	hdmiVers(::NTV2DeviceGetHDMIVersion (inDeviceID));
				const uint32_t	vidStd	(hdmiVers == 2 ? (inRegValue & kRegMaskHDMIInV2VideoStd) >> kRegShiftHDMIInputStatusV2Std : (inRegValue & kRegMaskHDMIInStandard) >> kRegShiftInputStatusStd);
				const uint32_t	rate	((inRegValue & kRegMaskHDMIInFPS) >> kRegShiftInputStatusFPS);
				static const string	sStds[32] = {"1080i", "720p", "480i", "576i", "1080p", "SXGA", "6", "7", "3840p", "4096p"};
				static const string	sRates[32] = {"invalid", "60.00", "59.94", "30.00", "29.97", "25.00", "24.00", "23.98"};
				oss	<< "HDMI Input: " << (inRegValue & BIT(0) ? "Locked" : "Unlocked")			<< endl
					<< "HDMI Input: " << (inRegValue & BIT(1) ? "Stable" : "Unstable")			<< endl
					<< "Color Mode: " << (inRegValue & BIT(2) ? "RGB" : "YCbCr")				<< endl
					<< "Bitdepth: " << (inRegValue & BIT(3) ? "10-bit" : "8-bit")				<< endl
					<< "Audio Channels: " << (inRegValue & BIT(12) ? 8 : 2)						<< endl
					<< "Scan Mode: " << (inRegValue & BIT(13) ? "Progressive" : "Interlaced")	<< endl
					<< "Standard: " << (inRegValue & BIT(14) ? "SD" : "HD")						<< endl;
				if (hdmiVers == 1 || hdmiVers == 2)
					oss << "Video Standard: " << sStds[vidStd]									<< endl;
				oss	<< "Receiving: " << (inRegValue & BIT(27) ? "DVI" : "HDMI")					<< endl
					<< "Video Rate : " << (rate < 8 ? sRates[rate] : string("invalid"));
				return oss.str();
			}
		}	mDecodeHDMIInputStatus;

		struct DecodeHDMIOutHDRPrimary : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				ostringstream	oss;
				if (::NTV2DeviceCanDoHDMIHDROut (inDeviceID))
					switch (inRegNum)
					{
						case kRegHDMIHDRGreenPrimary:
						case kRegHDMIHDRBluePrimary:
						case kRegHDMIHDRRedPrimary:
						case kRegHDMIHDRWhitePoint:
						{	//	Asserts to validate this one code block will handle all cases:
							NTV2_ASSERT (kRegMaskHDMIHDRGreenPrimaryX == kRegMaskHDMIHDRBluePrimaryX  &&  kRegMaskHDMIHDRBluePrimaryX == kRegMaskHDMIHDRRedPrimaryX);
							NTV2_ASSERT (kRegMaskHDMIHDRGreenPrimaryY == kRegMaskHDMIHDRBluePrimaryY  &&  kRegMaskHDMIHDRBluePrimaryY == kRegMaskHDMIHDRRedPrimaryY);
							NTV2_ASSERT (kRegMaskHDMIHDRRedPrimaryX == kRegMaskHDMIHDRWhitePointX  &&  kRegMaskHDMIHDRRedPrimaryY == kRegMaskHDMIHDRWhitePointY);
							NTV2_ASSERT (kRegShiftHDMIHDRGreenPrimaryX == kRegShiftHDMIHDRBluePrimaryX  &&  kRegShiftHDMIHDRBluePrimaryX == kRegShiftHDMIHDRRedPrimaryX);
							NTV2_ASSERT (kRegShiftHDMIHDRGreenPrimaryY == kRegShiftHDMIHDRBluePrimaryY  &&  kRegShiftHDMIHDRBluePrimaryY == kRegShiftHDMIHDRRedPrimaryY);
							NTV2_ASSERT (kRegShiftHDMIHDRRedPrimaryX == kRegShiftHDMIHDRWhitePointX  &&  kRegShiftHDMIHDRRedPrimaryY == kRegShiftHDMIHDRWhitePointY);
							const uint16_t	xPrimary	((inRegValue & kRegMaskHDMIHDRRedPrimaryX) >> kRegShiftHDMIHDRRedPrimaryX);
							const uint16_t	yPrimary	((inRegValue & kRegMaskHDMIHDRRedPrimaryY) >> kRegShiftHDMIHDRRedPrimaryY);
							const double	xFloat		(double(xPrimary) * 0.00002);
							const double	yFloat		(double(yPrimary) * 0.00002);
							if (NTV2_IS_VALID_HDR_PRIMARY (xPrimary))
								oss	<< "X: "	<< fDEC(xFloat,7,5) << endl;
							else
								oss	<< "X: "	<< HEX0N(xPrimary, 4)	<< "(invalid)" << endl;
							if (NTV2_IS_VALID_HDR_PRIMARY (yPrimary))
								oss	<< "Y: "	<< fDEC(yFloat,7,5);
							else
								oss	<< "Y: "	<< HEX0N(yPrimary, 4)	<< "(invalid)";
							break;
						}
						case kRegHDMIHDRMasteringLuminence:
						{
							const uint16_t	minValue	((inRegValue & kRegMaskHDMIHDRMinMasteringLuminance) >> kRegShiftHDMIHDRMinMasteringLuminance);
							const uint16_t	maxValue	((inRegValue & kRegMaskHDMIHDRMaxMasteringLuminance) >> kRegShiftHDMIHDRMaxMasteringLuminance);
							const double	minFloat	(double(minValue) * 0.00001);
							const double	maxFloat	(maxValue);
							if (NTV2_IS_VALID_HDR_MASTERING_LUMINENCE (minValue))
								oss	<< "Min: "	<< fDEC(minFloat,7,5) << endl;
							else
								oss	<< "Min: "	<< HEX0N(minValue, 4)	<< "(invalid)" << endl;
							oss	<< "Max: "	<< fDEC(maxFloat,7,5);
							break;
						}
						case kRegHDMIHDRLightLevel:
						{
							const uint16_t	cntValue	((inRegValue & kRegMaskHDMIHDRMaxContentLightLevel) >> kRegShiftHDMIHDRMaxContentLightLevel);
							const uint16_t	frmValue	((inRegValue & kRegMaskHDMIHDRMaxFrameAverageLightLevel) >> kRegShiftHDMIHDRMaxFrameAverageLightLevel);
							const double	cntFloat	(cntValue);
							const double	frmFloat	(frmValue);
							if (NTV2_IS_VALID_HDR_LIGHT_LEVEL (cntValue))
								oss	<< "Max Content Light Level: "	<< fDEC(cntFloat,7,5)					<< endl;
							else
								oss	<< "Max Content Light Level: "	<< HEX0N(cntValue, 4) << "(invalid)"	<< endl;
							if (NTV2_IS_VALID_HDR_LIGHT_LEVEL (frmValue))
								oss	<< "Max Frame Light Level: "	<< fDEC(frmFloat,7,5);
							else
								oss	<< "Max Frame Light Level: "	<< HEX0N(frmValue, 4) << "(invalid)";
							break;
						}
						default:	NTV2_ASSERT(false);
					}
				return oss.str();
			}
		}	mDecodeHDMIOutHDRPrimary;

		struct DecodeHDMIOutHDRControl : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				static const string	sEOTFs[]	=	{"Trad Gamma SDR", "Trad Gamma HDR", "SMPTE ST 2084", "HLG"};
				ostringstream	oss;
				if (::NTV2DeviceCanDoHDMIHDROut (inDeviceID))
				{
					const uint16_t	EOTFvalue				((inRegValue & kRegMaskElectroOpticalTransferFunction) >> kRegShiftElectroOpticalTransferFunction);
					const uint16_t	staticMetaDataDescID	((inRegValue & kRegMaskHDRStaticMetadataDescriptorID) >> kRegShiftHDRStaticMetadataDescriptorID);
                    oss	<< "HDMI Out Dolby Vision Enabled: " << YesNo(inRegValue & kRegMaskHDMIHDRDolbyVisionEnable)     << endl
                        << "HDMI HDR Out Enabled: "		     << YesNo(inRegValue & kRegMaskHDMIHDREnable)				<< endl
                        << "Constant Luminance: "		     << YesNo(inRegValue & kRegMaskHDMIHDRNonContantLuminance)	<< endl
                        << "EOTF: "						     << sEOTFs[(EOTFvalue < 3) ? EOTFvalue : 3]					<< endl
                        << "Static MetaData Desc ID: "	     << HEX0N(staticMetaDataDescID, 2) << " (" << DEC(staticMetaDataDescID) << ")";
				}
				return oss.str();
			}
		}	mDecodeHDMIOutHDRControl;

		struct DecodeSDIOutputControl : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream		oss;
				const uint32_t		vidStd	(inRegValue & (BIT(0)|BIT(1)|BIT(2)));
				static const string	sStds[32] = {"1080i", "720p", "480i", "576i", "1080p", "1556i", "6", "7"};
				oss	<< "Video Standard : "			<< sStds[vidStd]										<< endl
					<< "2Kx1080 mode: "				<< (inRegValue & BIT(3) ? "2048x1080" : "1920x1080")	<< endl
					<< "HBlank RGB Range: Black="	<< (inRegValue & BIT(7) ? "0x40" : "0x04")				<< endl
					<< "3G enable: "				<< YesNo(inRegValue & BIT(24))							<< endl
					<< "3G mode: "					<< (inRegValue & BIT(25) ? "b" : "a")					<< endl
					<< "VPID insert enable: "		<< YesNo(inRegValue & BIT(26))							<< endl
					<< "VPID overwrite enable: "	<< YesNo(inRegValue & BIT(27))							<< endl
					<< "DS 1 audio source: Subsystem ";
				switch ((inRegValue & (BIT(28)|BIT(30))) >> 28)
				{
					case 0:	oss << (inRegValue & BIT(18) ? 5 : 1);	break;
					case 1:	oss << (inRegValue & BIT(18) ? 7 : 3);	break;
					case 4:	oss << (inRegValue & BIT(18) ? 6 : 2);	break;
					case 5:	oss << (inRegValue & BIT(18) ? 8 : 4);	break;
				}
				oss	<< endl	<< "DS 2 audio source: Subsystem ";
				switch ((inRegValue & (BIT(29)|BIT(31))) >> 29)
				{
					case 0:	oss << (inRegValue & BIT(19) ? 5 : 1);	break;
					case 1:	oss << (inRegValue & BIT(19) ? 7 : 3);	break;
					case 4:	oss << (inRegValue & BIT(19) ? 6 : 2);	break;
					case 5:	oss << (inRegValue & BIT(19) ? 8 : 4);	break;
				}
				return oss.str();
			}
		}	mDecodeSDIOutputControl;

		struct DecodeDMAControl : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				const uint16_t	gen		((inRegValue & (BIT(20)|BIT(21)|BIT(22)|BIT(23))) >> 20);
				const uint16_t	lanes	((inRegValue & (BIT(16)|BIT(17)|BIT(18)|BIT(19))) >> 16);
				ostringstream	oss;
				for (uint16_t engine(0);  engine < 4;  engine++)
					oss	<< "DMA " << (engine+1) << " Int Active?: "	<< YesNo(inRegValue & BIT(27+engine))						<< endl;
				oss	<< "Bus Error Int Active?: "					<< YesNo(inRegValue & BIT(31))								<< endl;
				for (uint16_t engine(0);  engine < 4;  engine++)
					oss	<< "DMA " << (engine+1) << " Busy?: "		<< YesNo(inRegValue & BIT(27+engine))						<< endl;
				oss	<< "Strap: "									<< ((inRegValue & BIT(7)) ? "Installed" : "Not Installed")	<< endl
					<< "Gen: "										<< gen << ((gen > 0 && gen < 4) ? "" : " <invalid>")		<< endl
					<< "Lanes: "									<< lanes << ((lanes >= 0  &&  lanes < 9) ? "" : " <invalid>");
				return oss.str();
			}
		}	mDMAControlRegDecoder;
		
		struct DecodeDMAIntControl : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				for (uint16_t eng(0);  eng < 4;  eng++)
					oss	<< "DMA " << (eng+1) << " Enabled?: "	<< YesNo(inRegValue & BIT(eng))		<< endl;
				oss	<< "Bus Error Enabled?: "					<< YesNo(inRegValue & BIT(4))		<< endl;
				for (uint16_t eng(0);  eng < 4;  eng++)
					oss	<< "DMA " << (eng+1) << " Active?: "	<< YesNo(inRegValue & BIT(27+eng))	<< endl;
				oss	<< "Bus Error: "							<< YesNo(inRegValue & BIT(31));
				return oss.str();
			}
		}	mDMAIntControlRegDecoder;

		struct DecodeRP188InOutDBB : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				oss	<< "RP188: "	<< ((inRegValue & BIT(16)) ? (inRegValue & BIT(17) ? "Selected" : "Unselected") : "No") << " RP-188 received"	<< endl
					<< "Bypass: "	<< (inRegValue & BIT(23) ? (inRegValue & BIT(22) ? "SDI In 2" : "SDI In 1") : "Disabled")						<< endl
					<< "Filter: "	<< HEX0N((inRegValue & 0xFF000000) >> 24, 2)																		<< endl
					<< "DBB: "		<< HEX0N((inRegValue & 0x0000FF00) >> 8, 2) << " " << HEX0N(inRegValue & 0x000000FF, 2);
				return oss.str();
			}
		}	mRP188InOutDBBRegDecoder;

		struct DecodeVidProcControl : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				static const string	sSplitStds [8]	=	{"1080i", "720p", "480i", "576i", "1080p", "1556i", "?6?", "?7?"};
				oss	<< "Limiting: "		<< ((inRegValue & BIT(11)) ? "Pass illegal data values" : "Limit to legal SDI")									<< endl
					<< "Limiting: "		<< ((inRegValue & BIT(12)) ? "Limit" : "Don't limit") << " to legal broadcast data values"						<< endl
					<< "FG Matte: "		<< (inRegValue & kRegMaskVidProcFGMatteEnable ? "Enabled" : "Disabled")											<< endl
					<< "BG Matte: "		<< (inRegValue & kRegMaskVidProcBGMatteEnable ? "Enabled" : "Disabled")											<< endl
					<< "FG Control: "	<< (inRegValue & kRegMaskVidProcFGControl ? ((inRegValue & BIT(20)) ? "Shaped" : "Unshaped") : "Full Raster")	<< endl
					<< "BG Control: "	<< (inRegValue & kRegMaskVidProcBGControl ? ((inRegValue & BIT(22)) ? "Shaped" : "Unshaped") : "Full Raster")	<< endl
					<< "Input Sync: "	<< "Inputs " << (inRegValue & kRegMaskVidProcSyncFail ? "not in sync" : "in sync")								<< endl
					<< "Split Video Standard: "	<< sSplitStds[inRegValue & kRegMaskVidProcSplitStd];
				return oss.str();
			}
		}	mVidProcControlRegDecoder;

		struct DecodeSplitControl : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				const uint32_t	startmask	(0x0000FFFF);	//	16 bits
				const uint32_t	slopemask	(0x3FFF0000);	//	14 bits / high order byte
				const uint32_t	fractionmask(0x00000007);	//	3 bits for fractions
				oss	<< "Split Start: "	<< HEX0N((inRegValue & startmask) & ~fractionmask, 4) << " "
										<< HEX0N((inRegValue & startmask) & fractionmask, 4)					<< endl
					<< "Split Slope: "	<< HEX0N(((inRegValue & slopemask) >> 16) & ~fractionmask, 4) << " "
										<< HEX0N(((inRegValue & slopemask) >> 16) & fractionmask, 4)			<< endl
					<< "Split Type: "	<< ((inRegValue & BIT(30)) ? "Vertical" : "Horizontal");
				return oss.str();
			}
		}	mSplitControlRegDecoder;

		struct DecodeFlatMatteValue : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				const uint32_t	mask	(0x000003FF);	//	10 bits
				oss	<< "Flat Matte Cb: "	<< HEX0N(inRegValue & mask, 3)					<< endl
					<< "Flat Matte Y: "		<< HEX0N(((inRegValue >> 10) & mask) - 0x40, 3)	<< endl
					<< "Flat Matte Cr: "	<< HEX0N((inRegValue >> 20) & mask, 3);
				return oss.str();
			}
		}	mFlatMatteValueRegDecoder;

		struct DecodeSDIErrorStatus : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				oss	<< "Lock Count: "			<< DEC(inRegValue & 0x7FFF)		<< endl
					<< "Locked: "				<< YesNo(inRegValue & BIT(16))	<< endl
					<< "Link A VID Valid: "		<< YesNo(inRegValue & BIT(20))	<< endl
					<< "Link B VID Valid: "		<< YesNo(inRegValue & BIT(21))	<< endl
					<< "TRS Error Detected: "	<< YesNo(inRegValue & BIT(24));
				return oss.str();
			}
		}	mSDIErrorStatusRegDecoder;

		struct DecodeSDIErrorCount : public Decoder
		{
			virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
			{
				(void) inRegNum;
				(void) inDeviceID;
				ostringstream	oss;
				oss	<< "Link A: "		<< DEC(inRegValue & 0x0000FFFF)			<< endl
					<< "Link B: "		<< DEC((inRegValue & 0xFFFF0000) >> 16);
				return oss.str();
			}
		}	mSDIErrorCountRegDecoder;

		static const int	NOREADWRITE	=	0;
		static const int	READONLY	=	1;
		static const int	WRITEONLY	=	2;
		static const int	READWRITE	=	3;

		static const int	CONTAINS	=	0;
		static const int	STARTSWITH	=	1;
		static const int	ENDSWITH	=	2;
		static const int	EXACTMATCH	=	3;

		typedef map <uint32_t, const Decoder *>		RegNumToDecoderMap;
		typedef pair <uint32_t, const Decoder *>	RegNumToDecoderPair;
		typedef multimap <string, uint32_t>			RegClassToRegNumMap, StringToRegNumMap;
		typedef pair <string, uint32_t>				StringToRegNumPair;
		typedef RegClassToRegNumMap::const_iterator	RegClassToRegNumConstIter;
		typedef StringToRegNumMap::const_iterator	StringToRegNumConstIter;

		RegNumToStringMap		mRegNumToStringMap;
		RegNumToDecoderMap		mRegNumToDecoderMap;
		RegClassToRegNumMap		mRegClassToRegNumMap;
		StringToRegNumMap		mStringToRegNumMap;
		mutable NTV2StringSet	mAllRegClasses;

		typedef pair <uint32_t, uint32_t>							XptRegNumAndMaskIndex;	//	First: register number;  second: mask index (0=0x000000FF, 1=0x0000FF00, 2=0x00FF0000, 3=0xFF000000)
		typedef map <NTV2InputCrosspointID, XptRegNumAndMaskIndex>	InputXpt2XptRegNumMaskIndexMap;
		typedef map <XptRegNumAndMaskIndex, NTV2InputCrosspointID>	XptRegNumMaskIndex2InputXptMap;
		typedef	InputXpt2XptRegNumMaskIndexMap::const_iterator		InputXpt2XptRegNumMaskIndexMapConstIter;
		typedef	XptRegNumMaskIndex2InputXptMap::const_iterator		XptRegNumMaskIndex2InputXptMapConstIter;

		InputXpt2XptRegNumMaskIndexMap		mInputXpt2XptRegNumMaskIndexMap;
		XptRegNumMaskIndex2InputXptMap		mXptRegNumMaskIndex2InputXptMap;

};	//	RegisterExpert

static const RegisterExpert	gRegExpert;		//	Register Expert Singleton



string CNTV2RegisterExpert::GetDisplayName (const uint32_t inRegNum)
{
	return gRegExpert.RegNameToString (inRegNum);
}

string CNTV2RegisterExpert::GetDisplayValue (const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID)
{
	return gRegExpert.RegValueToString (inRegNum, inRegValue, inDeviceID);
}

bool CNTV2RegisterExpert::IsRegisterInClass (const uint32_t inRegNum, const string & inClassName)
{
	return gRegExpert.IsRegInClass (inRegNum, inClassName);
}

NTV2StringSet CNTV2RegisterExpert::GetAllRegisterClasses (void)
{
	return gRegExpert.GetAllRegisterClasses ();
}

NTV2StringSet CNTV2RegisterExpert::GetRegisterClasses (const uint32_t inRegNum)
{
	return gRegExpert.GetRegisterClasses (inRegNum);
}

NTV2RegNumSet CNTV2RegisterExpert::GetRegistersForClass	(const string & inClassName)
{
	return gRegExpert.GetRegistersForClass (inClassName);
}

NTV2RegNumSet CNTV2RegisterExpert::GetRegistersForChannel (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL (inChannel)  ?  gRegExpert.GetRegistersForClass (gChlClasses[inChannel])  :  NTV2RegNumSet();
}

NTV2RegNumSet CNTV2RegisterExpert::GetRegistersForDevice (const NTV2DeviceID inDeviceID, const bool inIncludeVirtuals)
{
	return gRegExpert.GetRegistersForDevice (inDeviceID, inIncludeVirtuals);
}

NTV2RegNumSet CNTV2RegisterExpert::GetRegistersWithName (const string & inName, const int inSearchStyle)
{
	return gRegExpert.GetRegistersWithName (inName, inSearchStyle);
}

NTV2InputCrosspointID CNTV2RegisterExpert::GetInputCrosspointID (const uint32_t inXptRegNum, const uint32_t inMaskIndex)
{
	return gRegExpert.GetInputCrosspointID (inXptRegNum, inMaskIndex);
}

bool CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (const NTV2InputCrosspointID inInputXpt, uint32_t & outXptRegNum, uint32_t & outMaskIndex)
{
	return gRegExpert.GetXptRegNumAndMaskIndex (inInputXpt, outXptRegNum, outMaskIndex);
}
