/**
	@file	pnp/mac/ajadevicescannermacimpl.hpp
	@brief	This module ensures that the AJADeviceScanner module's static global ActiveDeviceMap
			gets populated/depopulated as AJA devices get attached/detached on the MacOS platform.
			This is done through AJADeviceScanner's private interface:
				extern bool			ActiveDeviceMap_Remove	(AJADevice *	pInDevice);
				extern bool			ActiveDeviceMap_Add		(AJADevicePtr	inNewDevice);
				extern AJADevicePtr	ActiveDeviceMap_Find	(AJADevice *	pInDevice);
**/
#if !defined (AJAMac)
	#error "This must be compiled on MacOS platform"
#endif	//	not AJAMac


//	Additional Includes
#include "deviceclasslistener.h"



/**
	@brief	I am a singleton that sets up everything necessary to listen for the attachment/detachment of AJA devices.
			I employ a battery of DeviceClassListeners, one per KEXT class.
**/
class AJADeviceScannerImpl
{
	//	Class Methods
	public:
		/**
			@brief	Returns the singleton instance, creating it if necessary.
			@return	Reference-counted "ptr" to the singleton instance. It may be NULL/empty, so be sure to test it before using it.
		**/
		static AJADeviceScannerImplPtr		Get (void);

	//	Instance Methods
	public:
		virtual 							~AJADeviceScannerImpl ();

	private:
		explicit							AJADeviceScannerImpl ();
		inline								AJADeviceScannerImpl (const AJADeviceScannerImpl & inObj)		{if (&inObj != this) assert (false);}
		inline AJADeviceScannerImpl &		operator = (const AJADeviceScannerImpl & inRHS)					{if (&inRHS != this) assert (false); return *this;}
		inline bool							IsOkay (void) const												{return !mDeviceClassListeners.empty ();}

	//	Instance Data
	private:
		std::list <DeviceClassListenerPtr>	mDeviceClassListeners;		///	My per-device-class listeners

};	//	AJADeviceScannerImpl


AJADeviceScannerImplPtr AJADeviceScannerImpl::Get (void)
{
	if (gScannerSingleton)
		return gScannerSingleton;
	try
	{
		gScannerSingleton = new AJADeviceScannerImpl;
		if (gScannerSingleton)
			if (!gScannerSingleton->IsOkay ())
				gScannerSingleton = NULL;
	}
	catch (std::bad_alloc)
	{
		gScannerSingleton = NULL;
	}
	return gScannerSingleton;

}	//	Get


AJADeviceScannerImpl::AJADeviceScannerImpl ()
{
	static const string sDriverClassNames [] =
	{
		//	NTV2 drivers/devices
		"com_aja_iokit_corvid",
		"com_aja_iokit_corvid22",
		"com_aja_iokit_corvid24",
		"com_aja_iokit_corvid3G",
		"com_aja_iokit_io4K",
		"com_aja_iokit_ioexpress",
		"com_aja_iokit_ioxt",
		"com_aja_iokit_kona3",
		"com_aja_iokit_kona3G",
		"com_aja_iokit_kona3GQuad",
		"com_aja_iokit_kona4",
		"com_aja_iokit_kona4Quad",
		"com_aja_iokit_konalheplus",
		"com_aja_iokit_konalhi",
		"com_aja_iokit_ttap",
		"com_aja_ntv2_pci",

		//	NTV4 drivers/devices
		"com_aja_ntv4_pci",
		""
	};	//	sDriverClassNames

	//
	//	Instantiate my DeviceClassListeners, one for each device class.
	//	In a perfect world, there'd be one generic NTV2 Mac PCI driver, and I'd only have to instantiate two listeners --
	//	"com_aja_ntv2_pci" and "com_aja_ntv4_pci". Unfortunately, there's a separate driver class per NTV2 device, so
	//	there are over a dozen listeners...
	//
	for (unsigned ndx (0); ndx < sizeof (sDriverClassNames); ndx++)
		if (!sDriverClassNames [ndx].empty ())
		{
			DeviceClassListenerPtr	listener (DeviceClassListener::Create (sDriverClassNames [ndx]));
			if (!listener)
			{
				mDeviceClassListeners.clear ();
				return;		//	Bail
			}
			mDeviceClassListeners.push_back (listener);
		}	//	for each non-empty device class name
		else
			break;

}	//	constructor


AJADeviceScannerImpl::~AJADeviceScannerImpl ()
{
	cerr << "Destructing AJADeviceScannerImpl (Mac) instance 0x" << hex << this << dec << endl;

}	//	destructor
