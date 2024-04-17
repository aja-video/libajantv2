/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2kona.h
// Purpose:	 Common configuration and status
//
///////////////////////////////////////////////////////////////

#ifndef NTV2KONA_HEADER
#define NTV2KONA_HEADER

#include "ntv2system.h"
#include "ntv2devicefeatures.h"
#include "ntv2xpt.h"
#include "ntv2vpid.h"
#include "ntv2rp188.h"
#include "ntv2anc.h"
#include "ntv2aux.h"
#include "ntv2video.h"
#include "ntv2hdmiin.h"
#include "ntv2hdmiin4.h"
#include "ntv2hdmiout4.h"
#include "ntv2genlock.h"
#include "ntv2genlock2.h"
#include "ntv2videoraster.h"
#include "ntv2setup.h"
#include "ntv2mcap.h"

//Monitor processes
#define AJA_OUTPUT_SETUP
#define AJA_HDMI_IN
#define AJA_HDMI_OUT
#define AJA_GENLOCK
#define AJA_RASTERIZER


#if defined(NTV2_DEPRECATE_17_0)
	//	Copied from ntv2devicefeatures.c, deprecated in SDK 17.0
	bool NTV2DeviceHasSPIv2 (const NTV2DeviceID inDeviceID);
	bool NTV2DeviceHasSPIv3(const NTV2DeviceID inDeviceID);
	bool NTV2DeviceHasSPIv4(const NTV2DeviceID inDeviceID);
	bool NTV2DeviceHasSPIv5(const NTV2DeviceID inDeviceID);

	bool NTV2DeviceHasGenlockv2(const NTV2DeviceID devID);
	bool NTV2DeviceHasGenlockv3(const NTV2DeviceID devID);

	bool NTV2DeviceHasColorSpaceConverterOnChannel2(const NTV2DeviceID devID);

	bool NTV2DeviceCanDoAudio2Channels(const NTV2DeviceID devID);
	bool NTV2DeviceCanDoAudio6Channels(const NTV2DeviceID devID);
	bool NTV2DeviceCanDoAudio8Channels(const NTV2DeviceID devID);

	UWord NTV2DeviceGetNumAudioStreams(const NTV2DeviceID devID);
	bool NTV2DeviceCanDoAudioN(const NTV2DeviceID devID, UWord index0);
	bool NTV2DeviceCanDoLTCOutN(const NTV2DeviceID devID, UWord index0);
	bool NTV2DeviceCanDoLTCInN(const NTV2DeviceID devID, UWord index0);
	bool NTV2DeviceCanDoRS422N(const NTV2DeviceID devID, const NTV2Channel ch);
#endif	//	!defined(NTV2_DEPRECATE_17_0)

// Define interrupt control, dma, and flash register bits here because it's private to the driver
enum
{
	//Register 20
	kIntOutputVBLEnable		= BIT (0),
	kIntOut1Enable			= kIntOutputVBLEnable,
	kIntInput1VBLEnable		= BIT (1),
	kIntIn1Enable			= kIntInput1VBLEnable,
	kIntInput2VBLEnable		= BIT (2),
	kIntIn2Enable			= kIntInput2VBLEnable,
	kIntAudioOutWrapEnable	= BIT (4),
	kIntAudioInWrapEnable	= BIT (5),
	kIntAudioWrapRateEnable	= BIT (6),
	kIntUartTXEnable		= BIT (7),
	kIntUartRXEnable		= BIT (8),
	kIntUartRXClr			= BIT (15),
	kIntAudioChunkClr		= BIT (16),
	kIntUart2TXEnable		= BIT (17),
	kIntOut2Enable			= BIT (18),
	kIntOut3Enable			= BIT (19),
	kIntOut4Enable			= BIT (20),
	kIntOut4Clr				= BIT (21),
	kIntOut3Clr				= BIT (22),
	kIntOut2Clr				= BIT (23),
	kIntAudioChunkEnable	= BIT (23),
	kIntUart1TXClr			= BIT (24),
	kIntAudioWrapRateClr	= BIT (25),
	kIntUart2TXClr			= BIT (26),
	kIntAudioOutWrapClr		= BIT (27),
	kIntIn2Clr				= BIT (29),
	kIntIn1Clr				= BIT (30),
	kIntOut1Clr				= BIT (31),

	//						  UartRxClr  ????????   Out4Clr    Out3Clr   Out2Clr    UartTxClr  WrapClr    Uart2TxClr ????????   In2VerClr  In1VerClr  OutVerClr
	kIntVidAudMask			= BIT (15) + BIT (16) + BIT (21) + BIT (22) +BIT (23) + BIT (24) + BIT (25) + BIT (26) + BIT (27) + BIT (29) + BIT (30) + BIT (31),	//	Register 20  (kRegVidIntControl)

	//Register 21
	kIntOut4				= BIT (6),
	kIntOut3				= BIT (7),
	kIntOut2				= BIT (8),
	kIntUartRX				= BIT (15),
	kIntAudioChunk			= BIT (16),
	kIntUart1TX				= BIT (24),		//	UART 1 Tx Active bit in Register 21 (kRegStatus) -- really means "UART 1 Tx FIFO became empty"
	kIntAudioWrapRate		= BIT (25),
	kIntUart2TX				= BIT (26),		//	UART 2 Tx Active bit in Register 21 (kRegStatus) -- really means "UART 2 Tx FIFO became empty"
	kIntAudioInWrap			= BIT (26),
	kIntAudioOutWrap		= BIT (27),
	kIntIn2					= BIT (29),
	kIntIn1					= BIT (30),
	kIntOut1				= BIT (31),
	kIntInput2VBL			= BIT (29),
	kIntInput1VBL			= BIT (30),
	kIntOutputVBL			= BIT (31),


	//Register 266
	kIntIn3Enable			= BIT (1),
	kIntIn4Enable			= BIT (2),
	kIntIn5Enable			= BIT (8),
	kIntIn6Enable			= BIT (9),
	kIntIn7Enable			= BIT (10),
	kIntIn8Enable			= BIT (11),
	kIntOut5Enable			= BIT (12),
	kIntOut6Enable			= BIT (13),
	kIntOut7Enable			= BIT (14),
	kIntOut8Enable			= BIT (15),
	kIntOut8Clr				= BIT (16),
	kIntOut7Clr				= BIT (17),
	kIntOut6Clr				= BIT (18),
	kIntOut5Clr				= BIT (19),
	kIntIn8Clr				= BIT (25),
	kIntIn7Clr				= BIT (26),
	kIntIn6Clr				= BIT (27),
	kIntIn5Clr				= BIT (28),
	kIntIn4Clr				= BIT (29),
	kIntIn3Clr				= BIT (30),

	//						  Out8Clr    Out7Clr    Out6Clr   Out5Clr    In8Clr     In7Clr     In6Clr     In5Clr     In4VBIClr  In3VBIClr
	kIntVidAudMask2			= BIT (16) + BIT (17) + BIT (18) +BIT (19) + BIT (25) + BIT (26) + BIT (27) + BIT (28) + BIT (29) + BIT (30),	//	Register 266 (kRegVidIntControl2)

	//Register 265
	kIntOut8				= BIT (22),
	kIntOut7				= BIT (23),
	kIntOut6				= BIT (24),
	kIntIn8					= BIT (25),
	kIntIn7					= BIT (26),
	kIntIn6					= BIT (27),
	kIntIn5					= BIT (28),
	kIntIn4					= BIT (29),
	kIntIn3					= BIT (30),
	kIntOut5				= BIT (31),

	kIntDma1Enable			= BIT (0),
	kIntDma2Enable			= BIT (1),
	kIntDma3Enable			= BIT (2),
	kIntDma4Enable			= BIT (3),
	kIntBusErrEnable		= BIT (4),
	kIntDmaEnableMask		= BIT (0) + BIT (1) + BIT (2) + BIT (3) + BIT (4),

	kIntValidation			= BIT (26),

	kIntDMA1				= BIT (27),
	kIntDMA2				= BIT (28),
	kIntDMA3				= BIT (29),
	kIntDMA4				= BIT (30),
	kIntBusErr				= BIT (31),
	kIntDmaMask				= BIT (27) + BIT (28) + BIT (29) + BIT (30) + BIT (31),

	kDma1Go					= BIT (0),
	kDma2Go					= BIT (1),
	kDma3Go					= BIT (2),
	kDma4Go					= BIT (3),

	kRegDMAToHostBit		= BIT (31),
	kRegDMAAudioModeBit		= BIT (30),
	kRegDMA64ModeBit		= BIT (28),

	kRegFlashResetBit		= BIT (10),
	kRegFlashDoneBit		= BIT (9),
	kRegFlashPgmRdyBit		= BIT (8),
	kRegFlashDataMask		= BIT (7) + BIT (6) + BIT (5) + BIT (4) + BIT (3) + BIT (2) + BIT (1) + BIT (0)
};

#define NTV2_MAX_HDMI_MONITOR	4
typedef struct Ntv2DriverProcessContext
{
	Ntv2SystemContext*	pSystemContext;
	ntv2_hdmiin*		pHDMIInMonitor[NTV2_MAX_HDMI_MONITOR];
	ntv2_hdmiin4*		pHDMIIn4Monitor[NTV2_MAX_HDMI_MONITOR];
	ntv2_hdmiout4*		pHDMIOut4Monitor;
	ntv2_genlock*		pGenlockMonitor;
	ntv2_genlock2*		pGenlock2Monitor;
	ntv2_videoraster*	pRasterMonitor;
	ntv2_setup*			pSetupMonitor;
	ntv2_mcap*			pBitstream;
	_Atomic uint32_t	processCount;
}Ntv2DriverProcessContext;



bool InitializeNtv2Driver(Ntv2DriverProcessContext* inProcessContext);
bool StartDriverProcesses(Ntv2DriverProcessContext* processContext);
void StopDriverProcesses(Ntv2DriverProcessContext* processContext);
void EnableNtv2Interrupts(Ntv2SystemContext* inSystemContext);
void DisableNtv2Interrupts(Ntv2SystemContext* inSystemContext);
void EnableXlnxUserInterrupt(Ntv2SystemContext* pSystemContext, int inIndex);
void DisableXlnxUserInterrupt(Ntv2SystemContext* pSystemContext, int inIndex);
void InitializeVirtualRegisters(Ntv2SystemContext* inSystemContext);
bool IsKonaIPDevice(Ntv2SystemContext* inSystemContext);

///////////////////////
//board format routines
NTV2VideoFormat GetBoardVideoFormat(Ntv2SystemContext* context, NTV2Channel channel);
NTV2Standard GetStandard(Ntv2SystemContext* context, NTV2Channel channel);
NTV2FrameGeometry GetFrameGeometry(Ntv2SystemContext* context, NTV2Channel channel);
NTV2FrameRate GetFrameRate(Ntv2SystemContext* context, NTV2Channel channel);
bool IsProgressiveStandard(Ntv2SystemContext* context, NTV2Channel channel);
bool GetSmpte372(Ntv2SystemContext* context, NTV2Channel channel);
bool GetQuadFrameEnable(Ntv2SystemContext* context, NTV2Channel channel);
bool Get4kSquaresEnable (Ntv2SystemContext* context, NTV2Channel channel);
bool Get425FrameEnable (Ntv2SystemContext* context, NTV2Channel channel);
bool Get12GTSIFrameEnable (Ntv2SystemContext* context, NTV2Channel channel);
bool GetQuadQuadFrameEnable(Ntv2SystemContext* context, NTV2Channel channel);
bool GetQuadQuadSquaresEnable(Ntv2SystemContext* context, NTV2Channel channel);
bool IsMultiFormatActive (Ntv2SystemContext* context);
bool GetEnable4KDCPSFOutMode(Ntv2SystemContext* context);
NTV2FrameBufferFormat GetFrameBufferFormat(Ntv2SystemContext* context, NTV2Channel channel);
void SetFrameBufferFormat(Ntv2SystemContext* context, NTV2Channel channel, NTV2FrameBufferFormat value);
NTV2VideoFrameBufferOrientation GetFrameBufferOrientation(Ntv2SystemContext* context, NTV2Channel channel);
void SetFrameBufferOrientation(Ntv2SystemContext* context, NTV2Channel channel, NTV2VideoFrameBufferOrientation value);
bool GetConverterOutStandard(Ntv2SystemContext* context, NTV2Standard* value);
bool ReadFSHDRRegValues(Ntv2SystemContext* context, NTV2Channel channel, HDRDriverValues* hdrRegValues);

///////////////////////
NTV2Mode GetMode(Ntv2SystemContext* context, NTV2Channel channel);
void SetMode(Ntv2SystemContext* context, NTV2Channel channel, NTV2Mode value);
uint32_t GetOutputFrame(Ntv2SystemContext* context, NTV2Channel channel);
void SetOutputFrame(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value);
uint32_t GetInputFrame(Ntv2SystemContext* context, NTV2Channel channel);
void SetInputFrame(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value);
uint32_t GetPCIAccessFrame(Ntv2SystemContext* context, NTV2Channel channel);
void SetPCIAccessFrame(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value);
bool Get2piCSC(Ntv2SystemContext* context, NTV2Channel channel);
bool Set2piCSC(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
NTV2FrameBufferFormat GetDualLink5PixelFormat(Ntv2SystemContext* context);
void SetDualLink5PixelFormat(Ntv2SystemContext* context, NTV2FrameBufferFormat value);
ULWord GetHWFrameBufferSize(Ntv2SystemContext* context, NTV2Channel channel);
ULWord GetFrameBufferSize(Ntv2SystemContext* context, NTV2Channel channel);

///////////////////////
bool FieldDenotesStartOfFrame(Ntv2SystemContext* context, NTV2Crosspoint channelSpec);
bool IsFieldID0(Ntv2SystemContext* context, NTV2Crosspoint xpt);

///////////////////////
bool ProgramProductCode(Ntv2SystemContext* context);
bool WaitForFlashNOTBusy(Ntv2SystemContext* context);

///////////////////////
//sdi routines
bool SetVideoOutputStandard(Ntv2SystemContext* context, NTV2Channel channel);
bool SetSDIOutStandard(Ntv2SystemContext* context, NTV2Channel channel, NTV2Standard value);
bool SetSDIOut_2Kx1080Enable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOut3GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool SetSDIOut3GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOut3GbEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool SetSDIOut3GbEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOut6GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool SetSDIOut6GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOut12GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool SetSDIOut12GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOutRGBLevelAConversion(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool GetSDIOutLevelAtoLevelBConversion(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool GetSDIInLevelBtoLevelAConversion(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool GetSDIIn6GEnable(Ntv2SystemContext* context, NTV2Channel channel);
bool GetSDIIn12GEnable(Ntv2SystemContext* context, NTV2Channel channel);


///////////////////////
//hdmi routines
bool SetLHiHDMIOutputStandard(Ntv2SystemContext* context);
bool SetHDMIOutputStandard(Ntv2SystemContext* context);
bool SetHDMIV2LevelBEnable(Ntv2SystemContext* context, bool enable);
bool SetMultiRasterInputStandard(Ntv2SystemContext* context, NTV2Standard mrStandard, NTV2Channel mrChannel);
bool SetEnableMultiRasterCapture(Ntv2SystemContext* context, bool bEnable);
bool HasMultiRasterWidget(Ntv2SystemContext* context);
bool IsMultiRasterEnabled(Ntv2SystemContext* context);

///////////////////////
//hdr routines
bool EnableHDMIHDR(Ntv2SystemContext* context, bool inEnableHDMIHDR);
bool GetEnableHDMIHDR(Ntv2SystemContext* context);
bool SetHDRData(Ntv2SystemContext* context, HDRDriverValues inRegisterValues);
bool GetHDRData(Ntv2SystemContext* context, HDRDriverValues* inRegisterValues);

///////////////////////
//analog routines
bool SetLHiAnalogOutputStandard(Ntv2SystemContext* context);

///////////////////////
//converter routines
bool GetK2ConverterOutFormat(Ntv2SystemContext* context, NTV2VideoFormat* format);

///////////////////////
//input routines
bool GetSourceVideoFormat(Ntv2SystemContext* context, NTV2VideoFormat* format, NTV2OutputXptID crosspoint, bool* quadMode, bool* quadQuadMode, HDRDriverValues* hdrRegValues);
NTV2VideoFormat GetInputVideoFormat(Ntv2SystemContext* context, NTV2Channel channel);
NTV2VideoFormat GetHDMIInputVideoFormat(Ntv2SystemContext* context);
NTV2VideoFormat GetAnalogInputVideoFormat(Ntv2SystemContext* context);

///////////////////////
//interrupt routines
bool UpdateAudioMixerGainFromRotaryEncoder(Ntv2SystemContext* context);

///////////////////////
//util routines
ULWord IsScanGeometry2Kx1080(NTV2ScanGeometry scanGeometry);
bool IsVideoFormat2Kx1080(NTV2VideoFormat videoFormat);
NTV2Crosspoint GetNTV2CrosspointChannelForIndex(ULWord index);
ULWord GetIndexForNTV2CrosspointChannel(NTV2Crosspoint channel);
NTV2Crosspoint GetNTV2CrosspointInputForIndex(ULWord index);
ULWord GetIndexForNTV2CrosspointInput(NTV2Crosspoint channel);
NTV2Crosspoint GetNTV2CrosspointForIndex(ULWord index);
ULWord GetIndexForNTV2Crosspoint(NTV2Crosspoint channel);
NTV2Channel GetNTV2ChannelForNTV2Crosspoint(NTV2Crosspoint crosspoint);
NTV2VideoFormat GetVideoFormatFromState(NTV2Standard standard, NTV2FrameRate frameRate, ULWord is2Kx1080, ULWord smpte372Enabled);
NTV2Standard GetNTV2StandardFromVideoFormat(NTV2VideoFormat videoFormat);
NTV2FrameRate GetNTV2FrameRateFromVideoFormat(NTV2VideoFormat videoFormat);
NTV2Channel GetOutXptChannel(NTV2OutputCrosspointID inXpt, bool multiFormatActive);
NTV2Standard GetStandardFromScanGeometry(NTV2ScanGeometry scanGeometry, ULWord progressive);
NTV2VideoFormat GetQuadSizedVideoFormat(NTV2VideoFormat videoFormat);
NTV2VideoFormat Get12GVideoFormat(NTV2VideoFormat videoFormat);
NTV2VideoFormat GetQuadQuadSizedVideoFormat(NTV2VideoFormat videoFormat);
NTV2VideoFormat GetHDSizedVideoFormat(NTV2VideoFormat videoFormat);
bool HDRIsChanging(HDRDriverValues inCurrentHDR, HDRDriverValues inNewHDR);

#endif
