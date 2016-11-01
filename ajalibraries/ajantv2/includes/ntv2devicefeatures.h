/**
	@file		ntv2devicefeatures.h
	@brief		Declares all device capability functions.
	@note		Although this is a .cpp file, it must be compiled by the Linux driver, which is C, not C++.
				Also, it's used in the Mac driver, which uses a restricted subset of C++ . . . so no STL!
	@copyright	(C) 2004-2014 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2DEVICEFEATURES_H
#define NTV2DEVICEFEATURES_H

#if defined(AJALinux) || defined(AJA_LINUX)
	#include <stddef.h>		// For size_t
#endif

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2publicinterface.h"

#if defined(__CPLUSPLUS__) || defined(__cplusplus)
#else
	#define false (0)
	#define true (!false)
#endif

#define	NTV2DeviceGetNumAudioEngines	NTV2DeviceGetNumAudioSystems


//	Most of the device features functions are generated from '.csv' files exported from a spreadsheet using a Python script.
//	The script writes the implementations into 'ntv2devicefeatures.hpp', and the declarations into 'ntv2devicefeatures.hh'...
#include "ntv2devicefeatures.hh"

AJAExport bool NTV2DeviceCanDoFormat(NTV2DeviceID boardID,
									NTV2FrameRate		framerate,
									NTV2FrameGeometry framegeometry, 
									NTV2Standard		standard);

AJAExport bool NTV2DeviceCanDo3GOut (NTV2DeviceID boardID, UWord index0);
AJAExport bool NTV2DeviceCanDoLTCEmbeddedN (NTV2DeviceID boardID, UWord index0);

// Overloading not supported by the ANSI C compiler used for Linux drivers.
// 
// TODO: Audit all platforms and switch all the original calls to the _Ex
// versions.
#if defined(__CPLUSPLUS__) || defined(__cplusplus)
	AJAExport ULWord NTV2DeviceGetFrameBufferSize(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetNumberFrameBuffers(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetAudioFrameBuffer(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetAudioFrameBuffer2(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
#else
	AJAExport ULWord NTV2DeviceGetFrameBufferSize_Ex(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetNumberFrameBuffers_Ex(NTV2DeviceID boardID);	//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetAudioFrameBuffer_Ex(NTV2DeviceID boardID);	//	Revisit for 2MB granularity
#endif

AJAExport ULWord NTV2DeviceGetFrameBufferSize(NTV2DeviceID boardID, NTV2FrameGeometry frameGeometry, NTV2FrameBufferFormat frameFormat);	//	Revisit for 2MB granularity
AJAExport ULWord NTV2DeviceGetNumberFrameBuffers(NTV2DeviceID boardID, NTV2FrameGeometry frameGeometry, NTV2FrameBufferFormat frameFormat);	//	Revisit for 2MB granularity
AJAExport ULWord NTV2DeviceGetAudioFrameBuffer(NTV2DeviceID boardID, NTV2FrameGeometry frameGeometry, NTV2FrameBufferFormat frameFormat);	//	Revisit for 2MB granularity
AJAExport ULWord NTV2DeviceGetAudioFrameBuffer2(NTV2DeviceID boardID, NTV2FrameGeometry frameGeometry, NTV2FrameBufferFormat frameFormat);	//	Revisit for 2MB granularity

#define	NTV2GetDACVersion						NTV2DeviceGetDACVersion					///< @deprecated	Use NTV2DeviceGetDACVersion instead.
#define	NTV2GetNumDMAEngines					NTV2DeviceGetNumDMAEngines				///< @deprecated	Use NTV2DeviceGetNumDMAEngines instead.
#define	NTV2DeviceGetNumAnlgVideoInputs			NTV2DeviceGetNumAnalogVideoInputs		///< @deprecated	Use NTV2DeviceGetNumAnalogVideoInputs instead.
#define	NTV2DeviceGetNumAnlgVideoOutputs		NTV2DeviceGetNumAnalogVideoOutputs		///< @deprecated	Use NTV2DeviceGetNumAnalogVideoOutputs instead.
#define	NTV2GetHDMIVersion						NTV2DeviceGetHDMIVersion				///< @deprecated	Use NTV2DeviceGetHDMIVersion instead.

AJAExport bool NTV2DeviceGetVideoFormatFromState (	NTV2VideoFormat *		pOutValue,
													const NTV2FrameRate		inFrameRate,
													const NTV2FrameGeometry	inFrameGeometry,
													const NTV2Standard		inStandard,
													const ULWord			inIsSMPTE372Enabled);

AJAExport bool NTV2DeviceGetVideoFormatFromState_Ex (	NTV2VideoFormat *		pOutValue,
														const NTV2FrameRate		inFrameRate,
														const NTV2FrameGeometry	inFrameGeometry,
														const NTV2Standard		inStandard,
														const ULWord			inIsSMPTE372Enabled,
														const bool				inIsProgressivePicture);

AJAExport bool NTV2DeviceCanConnect (const NTV2DeviceID inDeviceID, const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt);	///< @note	!!! NOT IMPLEMENTED YET !!!

AJAExport NTV2_DEPRECATED UWord NTV2DeviceGetNumAudioStreams (NTV2DeviceID boardID);		///< @deprecated	Will be deprecated soon. Use NTV2DeviceGetNumAudioSystems instead.
AJAExport NTV2_DEPRECATED bool NTV2DeviceCanDoAudioN (NTV2DeviceID boardID, UWord index0);	///< @deprecated	Will be deprecated soon. Use NTV2DeviceGetNumAudioSystems instead.
AJAExport NTV2_DEPRECATED bool NTV2DeviceCanDoLTCOutN (NTV2DeviceID boardID, UWord index0);	///< @deprecated	Will be deprecated soon. Use NTV2DeviceGetNumLTCOutputs instead.
AJAExport NTV2_DEPRECATED bool NTV2DeviceCanDoLTCInN (NTV2DeviceID boardID, UWord index0);	///< @deprecated	Will be deprecated soon. Use NTV2DeviceGetNumLTCInputs instead.
AJAExport NTV2_DEPRECATED bool NTV2DeviceCanDoRS422N (const NTV2DeviceID inDeviceID, const NTV2Channel inChannel);	///< @deprecated	Will be deprecated soon. Use NTV2DeviceGetNumSerialPorts instead.

#if !defined (NTV2_DEPRECATE)
	AJAExport NTV2_DEPRECATED	bool NTV2DeviceCanDoProAudio (NTV2DeviceID boardID);											///< @deprecated	This function is obsolete.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoProcAmp(NTV2DeviceID boardID);												///< @deprecated	This function is obsolete.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoBrightnessAdjustment(NTV2DeviceID boardID, NTV2LSVideoADCMode videoADCMode);	///< @deprecated	This function is obsolete.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoContrastAdjustment(NTV2DeviceID boardID, NTV2LSVideoADCMode videoADCMode);	///< @deprecated	This function is obsolete.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoSaturationAdjustment(NTV2DeviceID boardID, NTV2LSVideoADCMode videoADCMode);	///< @deprecated	This function is obsolete.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoHueAdjustment(NTV2DeviceID boardID, NTV2LSVideoADCMode videoADCMode);		///< @deprecated	This function is obsolete.

	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoAudio (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoAudioN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoAudio2 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoAudioN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoAudio3 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoAudioN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoAudio4 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoAudioN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoAudio5 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoAudioN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoAudio6 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoAudioN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoAudio7 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoAudioN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoAudio8 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoAudioN instead.

	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDo3G (NTV2DeviceID boardID);					///< @deprecated	Use NTV2DeviceCanDo3GOut instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDo3GOut2 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDo3GOut instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDo3GOut3 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDo3GOut instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDo3GOut4 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDo3GOut instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDo3GOut5 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDo3GOut instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDo3GOut6 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDo3GOut instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDo3GOut7 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDo3GOut instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDo3GOut8 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDo3GOut instead.

	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCOut (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoLTCOutN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCOut2 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoLTCOutN instead.

	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoMixer2 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceGetNumMixers instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoMixer3 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceGetNumMixers instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoMixer4 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceGetNumMixers instead.

	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoUART (NTV2DeviceID boardID);					///< @deprecated	Use NTV2DeviceCanDoRS422N instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoUART2 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoRS422N instead.

	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCIn (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoLTCInN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCIn2 (NTV2DeviceID boardID);				///< @deprecated	Use NTV2DeviceCanDoLTCInN instead.

	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCEmbedded (NTV2DeviceID boardID);			///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCEmbedded2 (NTV2DeviceID boardID);			///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCEmbedded3 (NTV2DeviceID boardID);			///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCEmbedded4 (NTV2DeviceID boardID);			///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCEmbedded5 (NTV2DeviceID boardID);			///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCEmbedded6 (NTV2DeviceID boardID);			///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCEmbedded7 (NTV2DeviceID boardID);			///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.
	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoLTCEmbedded8 (NTV2DeviceID boardID);			///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.

	AJAExport NTV2_DEPRECATED	bool NTV2BoardCanDoUARTN (NTV2DeviceID boardID, UWord index0);	///< @deprecated	Use NTV2DeviceCanDoRS422N instead.
	AJAExport NTV2_DEPRECATED	bool NTV2DeviceCanDoUARTN (NTV2DeviceID boardID, UWord index0);	///< @deprecated	Use NTV2DeviceCanDoRS422N instead.
#endif	//	!defined (NTV2_DEPRECATE)

#if 1	//	NOTE:  THESE WILL BE UNDEFINED WITH NTV2_DEPRECATE STARTING WITH SDK 13.0
	#define	NTV2BoardCanChangeEmbeddedAudioClock		NTV2DeviceCanChangeEmbeddedAudioClock			///< @deprecated	Use NTV2DeviceCanChangeEmbeddedAudioClock instead.
	#define	NTV2BoardCanChangeFrameBufferSize			NTV2DeviceCanChangeFrameBufferSize				///< @deprecated	Use NTV2DeviceCanChangeFrameBufferSize instead.
	#define	NTV2BoardCanDisableUFC						NTV2DeviceCanDisableUFC							///< @deprecated	Use NTV2DeviceCanDisableUFC instead.
	#define	NTV2BoardCanDo2KVideo						NTV2DeviceCanDo2KVideo							///< @deprecated	Use NTV2DeviceCanDo2KVideo instead.
	#define	NTV2BoardCanDo3GLevelConversion				NTV2DeviceCanDo3GLevelConversion				///< @deprecated	Use NTV2DeviceCanDo3GLevelConversion instead.
	#define	NTV2BoardCanDo3GOut							NTV2DeviceCanDo3GOut							///< @deprecated	Use NTV2DeviceCanDo3GOut instead.
	#define	NTV2BoardCanDo4KVideo						NTV2DeviceCanDo4KVideo							///< @deprecated	Use NTV2DeviceCanDo4KVideo instead.
	#define	NTV2BoardCanDoAESAudioIn					NTV2DeviceCanDoAESAudioIn						///< @deprecated	Use NTV2DeviceCanDoAESAudioIn instead.
	#define	NTV2BoardCanDoAnalogAudio					NTV2DeviceCanDoAnalogAudio						///< @deprecated	Use NTV2DeviceCanDoAnalogAudio instead.
	#define	NTV2BoardCanDoAnalogVideoIn					NTV2DeviceCanDoAnalogVideoIn					///< @deprecated	Use NTV2DeviceCanDoAnalogVideoIn instead.
	#define	NTV2BoardCanDoAnalogVideoOut				NTV2DeviceCanDoAnalogVideoOut					///< @deprecated	Use NTV2DeviceCanDoAnalogVideoOut instead.
	#define	NTV2BoardCanDoAudio2Channels				NTV2DeviceCanDoAudio2Channels					///< @deprecated	Use NTV2DeviceCanDoAudio2Channels instead.
	#define	NTV2BoardCanDoAudio6Channels				NTV2DeviceCanDoAudio6Channels					///< @deprecated	Use NTV2DeviceCanDoAudio6Channels instead.
	#define	NTV2BoardCanDoAudio8Channels				NTV2DeviceCanDoAudio8Channels					///< @deprecated	Use NTV2DeviceCanDoAudio8Channels instead.
	#define	NTV2BoardCanDoAudio96K						NTV2DeviceCanDoAudio96K							///< @deprecated	Use NTV2DeviceCanDoAudio96K instead.
	#define	NTV2BoardCanDoAudioDelay					NTV2DeviceCanDoAudioDelay						///< @deprecated	Use NTV2DeviceCanDoAudioDelay instead.
	#define	NTV2BoardCanDoAudioN						NTV2DeviceCanDoAudioN							///< @deprecated	Use NTV2DeviceCanDoAudioN instead.
	#define	NTV2BoardCanDoBreakoutBox					NTV2DeviceCanDoBreakoutBox						///< @deprecated	Use NTV2DeviceCanDoBreakoutBox instead.
	#define	NTV2BoardCanDoCapture						NTV2DeviceCanDoCapture							///< @deprecated	Use NTV2DeviceCanDoCapture instead.
	#define	NTV2BoardCanDoColorCorrection				NTV2DeviceCanDoColorCorrection					///< @deprecated	Use NTV2DeviceCanDoColorCorrection instead.
	#define	NTV2BoardCanDoConversionMode				NTV2DeviceCanDoConversionMode					///< @deprecated	Use NTV2DeviceCanDoConversionMode instead.
	#define	NTV2BoardCanDoDSKMode						NTV2DeviceCanDoDSKMode							///< @deprecated	Use NTV2DeviceCanDoDSKMode instead.
	#define	NTV2BoardCanDoDSKOpacity					NTV2DeviceCanDoDSKOpacity						///< @deprecated	Use NTV2DeviceCanDoDSKOpacity instead.
	#define	NTV2BoardCanDoDualLink						NTV2DeviceCanDoDualLink							///< @deprecated	Use NTV2DeviceCanDoDualLink instead.
	#define	NTV2BoardCanDoDVCProHD						NTV2DeviceCanDoDVCProHD							///< @deprecated	Use NTV2DeviceCanDoDVCProHD instead.
	#define	NTV2BoardCanDoFormat						NTV2DeviceCanDoFormat							///< @deprecated	Use NTV2DeviceCanDoFormat instead.
	#define	NTV2BoardCanDoFrameBufferFormat				NTV2DeviceCanDoFrameBufferFormat				///< @deprecated	Use NTV2DeviceCanDoFrameBufferFormat instead.
	#define	NTV2BoardCanDoFreezeOutput					NTV2DeviceCanDoFreezeOutput						///< @deprecated	Use NTV2DeviceCanDoFreezeOutput instead.
	#define	NTV2BoardCanDoHDMIOutStereo					NTV2DeviceCanDoHDMIOutStereo					///< @deprecated	Use NTV2DeviceCanDoHDMIOutStereo instead.
	#define	NTV2BoardCanDoHDV							NTV2DeviceCanDoHDV								///< @deprecated	Use NTV2DeviceCanDoHDV instead.
	#define	NTV2BoardCanDoHDVideo						NTV2DeviceCanDoHDVideo							///< @deprecated	Use NTV2DeviceCanDoHDVideo instead.
	#define	NTV2BoardCanDoInputSource					NTV2DeviceCanDoInputSource						///< @deprecated	Use NTV2DeviceCanDoInputSource instead.
	#define	NTV2BoardCanDoIsoConvert					NTV2DeviceCanDoIsoConvert						///< @deprecated	Use NTV2DeviceCanDoIsoConvert instead.
	#define	NTV2BoardCanDoLTC							NTV2DeviceCanDoLTC								///< @deprecated	Use NTV2DeviceCanDoLTC instead.
	#define	NTV2BoardCanDoLTCEmbeddedN					NTV2DeviceCanDoLTCEmbeddedN						///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.
	#define	NTV2BoardCanDoLTCInN						NTV2DeviceCanDoLTCInN							///< @deprecated	Use NTV2DeviceCanDoLTCInN instead.
	#define	NTV2BoardCanDoLTCInOnRefPort				NTV2DeviceCanDoLTCInOnRefPort					///< @deprecated	Use NTV2DeviceCanDoLTCInOnRefPort instead.
	#define	NTV2BoardCanDoLTCOutN						NTV2DeviceCanDoLTCOutN							///< @deprecated	Use NTV2DeviceCanDoLTCOutN instead.
	#define	NTV2BoardCanDoMSI							NTV2DeviceCanDoMSI								///< @deprecated	Use NTV2DeviceCanDoMSI instead.
	#define	NTV2BoardCanDoMultiFormat					NTV2DeviceCanDoMultiFormat						///< @deprecated	Use NTV2DeviceCanDoMultiFormat instead.
	#define	NTV2BoardCanDoPCMControl					NTV2DeviceCanDoPCMControl						///< @deprecated	Use NTV2DeviceCanDoPCMControl instead.
	#define	NTV2BoardCanDoPIO							NTV2DeviceCanDoPIO								///< @deprecated	Use NTV2DeviceCanDoPIO instead.
	#define	NTV2BoardCanDoPlayback						NTV2DeviceCanDoPlayback							///< @deprecated	Use NTV2DeviceCanDoPlayback instead.
	#define	NTV2BoardCanDoProAudio						NTV2DeviceCanDoProAudio							///< @deprecated	Use NTV2DeviceCanDoProAudio instead.
	#define	NTV2BoardCanDoProgrammableCSC				NTV2DeviceCanDoProgrammableCSC					///< @deprecated	Use NTV2DeviceCanDoProgrammableCSC instead.
	#define	NTV2BoardCanDoProgrammableRS422				NTV2DeviceCanDoProgrammableRS422				///< @deprecated	Use NTV2DeviceCanDoProgrammableRS422 instead.
	#define	NTV2BoardCanDoProRes						NTV2DeviceCanDoProRes							///< @deprecated	Use NTV2DeviceCanDoProRes instead.
	#define	NTV2BoardCanDoQREZ							NTV2DeviceCanDoQREZ								///< @deprecated	Use NTV2DeviceCanDoQREZ instead.
	#define	NTV2BoardCanDoQuarterExpand					NTV2DeviceCanDoQuarterExpand					///< @deprecated	Use NTV2DeviceCanDoQuarterExpand instead.
	#define	NTV2BoardCanDoRateConvert					NTV2DeviceCanDoRateConvert						///< @deprecated	Use NTV2DeviceCanDoRateConvert instead.
	#define	NTV2BoardCanDoRGBPlusAlphaOut				NTV2DeviceCanDoRGBPlusAlphaOut					///< @deprecated	Use NTV2DeviceCanDoRGBPlusAlphaOut instead.
	#define	NTV2BoardCanDoRP188							NTV2DeviceCanDoRP188							///< @deprecated	Use NTV2DeviceCanDoRP188 instead.
	#define	NTV2BoardCanDoRS422N						NTV2DeviceCanDoRS422N							///< @deprecated	Use NTV2DeviceCanDoRS422N instead.
	#define	NTV2BoardCanDoSDVideo						NTV2DeviceCanDoSDVideo							///< @deprecated	Use NTV2DeviceCanDoSDVideo instead.
	#define	NTV2BoardCanDoStackedAudio					NTV2DeviceCanDoStackedAudio						///< @deprecated	Use NTV2DeviceCanDoStackedAudio instead.
	#define	NTV2BoardCanDoStereoIn						NTV2DeviceCanDoStereoIn							///< @deprecated	Use NTV2DeviceCanDoStereoIn instead.
	#define	NTV2BoardCanDoStereoOut						NTV2DeviceCanDoStereoOut						///< @deprecated	Use NTV2DeviceCanDoStereoOut instead.
	#define	NTV2BoardCanDoThunderbolt					NTV2DeviceCanDoThunderbolt						///< @deprecated	Use NTV2DeviceCanDoThunderbolt instead.
	#define	NTV2BoardCanDoVideoFormat					NTV2DeviceCanDoVideoFormat						///< @deprecated	Use NTV2DeviceCanDoVideoFormat instead.
	#define	NTV2BoardCanDoVideoProcessing				NTV2DeviceCanDoVideoProcessing					///< @deprecated	Use NTV2DeviceCanDoVideoProcessing instead.
	#define	NTV2BoardCanDoWidget						NTV2DeviceCanDoWidget							///< @deprecated	Use NTV2DeviceCanDoWidget instead.
	#define	NTV2BoardGetActiveMemorySize				NTV2DeviceGetActiveMemorySize					///< @deprecated	Use NTV2DeviceGetActiveMemorySize instead.
	#define	NTV2BoardGetAudioFrameBuffer				NTV2DeviceGetAudioFrameBuffer					///< @deprecated	Use NTV2DeviceGetAudioFrameBuffer instead.
	#define	NTV2BoardGetAudioFrameBuffer				NTV2DeviceGetAudioFrameBuffer					///< @deprecated	Use NTV2DeviceGetAudioFrameBuffer instead.
	#define	NTV2BoardGetAudioFrameBuffer2				NTV2DeviceGetAudioFrameBuffer2					///< @deprecated	Use NTV2DeviceGetAudioFrameBuffer2 instead.
	#define	NTV2BoardGetDownConverterDelay				NTV2DeviceGetDownConverterDelay					///< @deprecated	Use NTV2DeviceGetDownConverterDelay instead.
	#define	NTV2BoardGetFrameBufferSize					NTV2DeviceGetFrameBufferSize					///< @deprecated	Use NTV2DeviceGetFrameBufferSize instead.
	#define	NTV2BoardGetFrameBufferSize_Ex				NTV2DeviceGetFrameBufferSize_Ex					///< @deprecated	Use NTV2DeviceGetFrameBufferSize_Ex instead.
	#define	NTV2BoardGetHDMIVersion						NTV2DeviceGetHDMIVersion						///< @deprecated	Use NTV2DeviceGetHDMIVersion instead.
	#define	NTV2BoardGetLUTVersion						NTV2DeviceGetLUTVersion							///< @deprecated	Use NTV2DeviceGetLUTVersion instead.
	#define	NTV2BoardGetMaxAudioChannels				NTV2DeviceGetMaxAudioChannels					///< @deprecated	Use NTV2DeviceGetMaxAudioChannels instead.
	#define	NTV2BoardGetMaxTransferCount				NTV2DeviceGetMaxTransferCount					///< @deprecated	Use NTV2DeviceGetMaxTransferCount instead.
	#define	NTV2BoardGetNum4kQuarterSizeConverters		NTV2DeviceGetNum4kQuarterSizeConverters			///< @deprecated	Use NTV2DeviceGetNum4kQuarterSizeConverters instead.
	#define	NTV2BoardGetNumAESAudioInputChannels		NTV2DeviceGetNumAESAudioInputChannels			///< @deprecated	Use NTV2DeviceGetNumAESAudioInputChannels instead.
	#define	NTV2BoardGetNumAESAudioOutputChannels		NTV2DeviceGetNumAESAudioOutputChannels			///< @deprecated	Use NTV2DeviceGetNumAESAudioOutputChannels instead.
	#define	NTV2BoardGetNumAnalogAudioInputChannels		NTV2DeviceGetNumAnalogAudioInputChannels		///< @deprecated	Use NTV2DeviceGetNumAnalogAudioInputChannels instead.
	#define	NTV2BoardGetNumAnalogAudioOutputChannels	NTV2DeviceGetNumAnalogAudioOutputChannels		///< @deprecated	Use NTV2DeviceGetNumAnalogAudioOutputChannels instead.
	#define	NTV2BoardGetNumAnlgVideoInputs				NTV2DeviceGetNumAnalogVideoInputs				///< @deprecated	Use NTV2DeviceGetNumAnalogVideoInputs instead.
	#define	NTV2BoardGetNumAnlgVideoOutputs				NTV2DeviceGetNumAnalogVideoOutputs				///< @deprecated	Use NTV2DeviceGetNumAnalogVideoOutputs instead.
	#define	NTV2BoardGetNumAudioStreams					NTV2DeviceGetNumAudioStreams					///< @deprecated	Use NTV2DeviceGetNumAudioStreams instead.
	#define	NTV2BoardGetNumberFrameBuffers				NTV2DeviceGetNumberFrameBuffers					///< @deprecated	Use NTV2DeviceGetNumberFrameBuffers instead.
	#define	NTV2DeviceGetNumberRegisters				NTV2DeviceGetMaxRegisterNumber					///< @deprecated	Use NTV2DeviceGetMaxRegisterNumber instead.
	#define	NTV2BoardGetNumberRegisters					NTV2DeviceGetMaxRegisterNumber					///< @deprecated	Use NTV2DeviceGetMaxRegisterNumber instead.
	#define	NTV2BoardGetNumCrossConverters				NTV2DeviceGetNumCrossConverters					///< @deprecated	Use NTV2DeviceGetNumCrossConverters instead.
	#define	NTV2BoardGetNumCSCs							NTV2DeviceGetNumCSCs							///< @deprecated	Use NTV2DeviceGetNumCSCs instead.
	#define	NTV2BoardGetNumDownConverters				NTV2DeviceGetNumDownConverters					///< @deprecated	Use NTV2DeviceGetNumDownConverters instead.
	#define	NTV2BoardGetNumEmbeddedAudioInputChannels	NTV2DeviceGetNumEmbeddedAudioInputChannels		///< @deprecated	Use NTV2DeviceGetNumEmbeddedAudioInputChannels instead.
	#define	NTV2BoardGetNumEmbeddedAudioOutputChannels	NTV2DeviceGetNumEmbeddedAudioOutputChannels		///< @deprecated	Use NTV2DeviceGetNumEmbeddedAudioOutputChannels instead.
	#define	NTV2BoardGetNumFrameStores					NTV2DeviceGetNumFrameStores						///< @deprecated	Use NTV2DeviceGetNumFrameStores instead.
	#define	NTV2BoardGetNumFrameSyncs					NTV2DeviceGetNumFrameSyncs						///< @deprecated	Use NTV2DeviceGetNumFrameSyncs instead.
	#define	NTV2BoardGetNumHDMIAudioInputChannels		NTV2DeviceGetNumHDMIAudioInputChannels			///< @deprecated	Use NTV2DeviceGetNumHDMIAudioInputChannels instead.
	#define	NTV2BoardGetNumHDMIAudioOutputChannels		NTV2DeviceGetNumHDMIAudioOutputChannels			///< @deprecated	Use NTV2DeviceGetNumHDMIAudioOutputChannels instead.
	#define	NTV2BoardGetNumHDMIVideoInputs				NTV2DeviceGetNumHDMIVideoInputs					///< @deprecated	Use NTV2DeviceGetNumHDMIVideoInputs instead.
	#define	NTV2BoardGetNumHDMIVideoOutputs				NTV2DeviceGetNumHDMIVideoOutputs				///< @deprecated	Use NTV2DeviceGetNumHDMIVideoOutputs instead.
	#define	NTV2BoardGetNumInputConverters				NTV2DeviceGetNumInputConverters					///< @deprecated	Use NTV2DeviceGetNumInputConverters instead.
	#define	NTV2BoardGetNumLUTs							NTV2DeviceGetNumLUTs							///< @deprecated	Use NTV2DeviceGetNumLUTs instead.
	#define	NTV2BoardGetNumMixers						NTV2DeviceGetNumMixers							///< @deprecated	Use NTV2DeviceGetNumMixers instead.
	#define	NTV2BoardGetNumOutputConverters				NTV2DeviceGetNumOutputConverters				///< @deprecated	Use NTV2DeviceGetNumOutputConverters instead.
	#define	NTV2BoardGetNumReferenceVideoInputs			NTV2DeviceGetNumReferenceVideoInputs			///< @deprecated	Use NTV2DeviceGetNumReferenceVideoInputs instead.
	#define	NTV2BoardGetNumSerialPorts					NTV2DeviceGetNumSerialPorts						///< @deprecated	Use NTV2DeviceGetNumSerialPorts instead.
	#define	NTV2BoardGetNumUpConverters					NTV2DeviceGetNumUpConverters					///< @deprecated	Use NTV2DeviceGetNumUpConverters instead.
	#define	NTV2BoardGetNumVideoChannels				NTV2DeviceGetNumVideoChannels					///< @deprecated	Use NTV2DeviceGetNumVideoChannels instead.
	#define	NTV2BoardGetNumVideoInputs					NTV2DeviceGetNumVideoInputs						///< @deprecated	Use NTV2DeviceGetNumVideoInputs instead.
	#define	NTV2BoardGetNumVideoOutputs					NTV2DeviceGetNumVideoOutputs					///< @deprecated	Use NTV2DeviceGetNumVideoOutputs instead.
	#define	NTV2BoardGetPingLED							NTV2DeviceGetPingLED							///< @deprecated	Use NTV2DeviceGetPingLED instead.
	#define	NTV2BoardGetUFCVersion						NTV2DeviceGetUFCVersion							///< @deprecated	Use NTV2DeviceGetUFCVersion instead.
	#define	NTV2BoardGetVideoFormatFromState			NTV2DeviceGetVideoFormatFromState				///< @deprecated	Use NTV2DeviceGetVideoFormatFromState instead.
	#define	NTV2BoardGetVideoFormatFromState_Ex			NTV2DeviceGetVideoFormatFromState_Ex			///< @deprecated	Use NTV2DeviceGetVideoFormatFromState_Ex instead.
	#define	NTV2BoardHasBiDirectionalSDI				NTV2DeviceHasBiDirectionalSDI					///< @deprecated	Use NTV2DeviceHasBiDirectionalSDI instead.
	#define	NTV2BoardHasColorSpaceConverterOnChannel2	NTV2DeviceHasColorSpaceConverterOnChannel2		///< @deprecated	Use NTV2DeviceHasColorSpaceConverterOnChannel2 instead.
	#define	NTV2BoardHasNWL								NTV2DeviceHasNWL								///< @deprecated	Use NTV2DeviceHasNWL instead.
	#define	NTV2BoardHasPCIeGen2						NTV2DeviceHasPCIeGen2							///< @deprecated	Use NTV2DeviceHasPCIeGen2 instead.
	#define	NTV2BoardHasSDIRelays						NTV2DeviceHasSDIRelays							///< @deprecated	Use NTV2DeviceHasSDIRelays instead.
	#define	NTV2BoardHasSPIFlash						NTV2DeviceHasSPIFlash							///< @deprecated	Use NTV2DeviceHasSPIFlash instead.
	#define	NTV2BoardHasSPIFlashSerial					NTV2DeviceHasSPIFlashSerial						///< @deprecated	Use NTV2DeviceHasSPIFlashSerial instead.
	#define	NTV2BoardHasSPIv2							NTV2DeviceHasSPIv2								///< @deprecated	Use NTV2DeviceHasSPIv2 instead.
	#define	NTV2BoardHasSPIv3							NTV2DeviceHasSPIv3								///< @deprecated	Use NTV2DeviceHasSPIv3 instead.
	#define	NTV2BoardIs64Bit							NTV2DeviceIs64Bit								///< @deprecated	Use NTV2DeviceIs64Bit instead.
	#define	NTV2BoardIsDirectAddressable				NTV2DeviceIsDirectAddressable					///< @deprecated	Use NTV2DeviceIsDirectAddressable instead.
	#define	NTV2BoardIsExternalToHost					NTV2DeviceIsExternalToHost						///< @deprecated	Use NTV2DeviceIsExternalToHost instead.
	#define	NTV2BoardNeedsRoutingSetup					NTV2DeviceNeedsRoutingSetup						///< @deprecated	Use NTV2DeviceNeedsRoutingSetup instead.
	#define	NTV2BoardSoftwareCanChangeFrameBufferSize	NTV2DeviceSoftwareCanChangeFrameBufferSize		///< @deprecated	Use NTV2DeviceSoftwareCanChangeFrameBufferSize instead.
#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	NTV2DEVICEFEATURES_H
