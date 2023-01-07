/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/ntv2thermo/main.cpp
	@brief		Command-line tool that reports and/or controls thermal-related settings for NTV2 devices.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajabase/common/options_popt.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/common/common.h"
#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include <ctime>

using namespace std;


#define	CHECK_FALSE(__x__)		do																	\
								{																	\
									if (!(__x__))													\
									{																\
										cerr << "## ERROR:  Failure at line " << __LINE__ << endl;	\
										return 3;													\
									}																\
								} while (false)


/**
	@return		The given string converted to lower-case.
	@param[in]	str		Specifies the string to be converted to lower case.
**/
static inline string ToLower (string str)	{return aja::lower(str);}


/**
	@brief		Reads the current temperature and fan state for the given device.
	@param[in]	inDevice		Specifies the device whose temperature and fan state is to be read.
	@param[in]	inTempScale		Specifies the temperature unit scale to use.
	@param[out]	outTemp			Receives the temperature value that was read from the device.
	@param[out]	outUnitsStr		Receives a string containing the human-readable name of the temperature units.
	@param[out]	outFanStateStr	Receives a string containing the human-readable fan state that was read from the device.
	@param[out]	outIsAuto		Receives 'true' if automatic fan control is enabled in the driver;  otherwise false.
	@return		Zero if successful.
**/
static int ReadTempAndFanState (CNTV2Card & inDevice, const string & inTempScale, double & outTemp, string & outUnitsStr, string & outFanStateStr, bool & outIsAuto)
{
	outTemp			= 0.0;
	outFanStateStr	= "";
	outUnitsStr		= "";
	outIsAuto		= false;

	//	Read the temperature...
	NTV2DieTempScale	scale	(NTV2DieTempScale_Celsius);
	if (inTempScale == "celsius" || inTempScale == "c")
		{scale = NTV2DieTempScale_Celsius;		outUnitsStr = "degrees Celsius";}
	else if (inTempScale == "fahrenheit" || inTempScale == "f")
		{scale = NTV2DieTempScale_Fahrenheit;	outUnitsStr = "degrees Fahrenheit";}
	else if (inTempScale == "kelvin" || inTempScale == "k")
		{scale = NTV2DieTempScale_Kelvin;		outUnitsStr = "degrees Kelvin";}
	else if (inTempScale == "rankine" || inTempScale == "r")
		{scale = NTV2DieTempScale_Rankine;		outUnitsStr = "degrees Rankine";}
	else
		{cerr << "## ERROR:  Bad '--scale' value '" << inTempScale << "'" << endl;  return 1;}
	CHECK_FALSE (inDevice.GetDieTemperature (outTemp, scale));

	//	Read the fan speed...
	ULWord	rawRegValue	(0);
	if (::NTV2DeviceCanThermostat (inDevice.GetDeviceID ()))
	{
		CHECK_FALSE (inDevice.ReadRegister (kVRegFanSpeed, rawRegValue, kRegFanHiMask, kRegFanHiShift));
		if (rawRegValue == NTV2_FanSpeed_Low)
			outFanStateStr = "LOW";
		else if (rawRegValue == NTV2_FanSpeed_Medium)
			outFanStateStr = "MED";
		else if (rawRegValue == NTV2_FanSpeed_High)
			outFanStateStr = "HI";
		else
			outFanStateStr = "??";

		CHECK_FALSE (inDevice.ReadRegister (kVRegUseThermostat, rawRegValue));
		outIsAuto = rawRegValue ? true : false;
	}
	else
	{
		CHECK_FALSE (inDevice.ReadRegister (kRegSysmonConfig2, rawRegValue, BIT(16), 16));
		outFanStateStr = rawRegValue ? "ON" : "OFF";
	}
	return 0;
}


/**
	@brief		Main entry point for 'ntv2thermo'.
	@param[in]	argc	Number arguments specified on the command line, including the path to the executable.
	@param[in]	argv	Array of 'const char' pointers, one for each argument.
	@return		Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
	int			result			(0);
	char *		pDeviceSpec		(AJA_NULL);	//	Which device?
	char *		pFormat			(AJA_NULL);	//	What format is desired?
	char *		pScale			(AJA_NULL);	//	What scale is desired?
	char *		pFan			(AJA_NULL);	//	What fan setting is desired?
	char *		pLogInterval	(AJA_NULL);	//	Continuously log? At what interval?
	poptContext	optionsContext;				//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"index#|serial#|model"				},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"index#|serial#|model"				},
		{"format",		'f',	POPT_ARG_STRING,	&pFormat,		0,	"desired format",				"brief|verbose|json"				},
		{"scale",		's',	POPT_ARG_STRING,	&pScale,		0,	"desired scale",				"celsius|fahrenheit|kelvin|rankine"	},
		{"fan",			0,		POPT_ARG_STRING,	&pFan,			0,	"desired fan operation",		"read|off|on|lo|med|hi|auto"		},
		{"log",			'l',	POPT_ARG_STRING,	&pLogInterval,	0,	"log temp & fan state",			"log interval in seconds"			},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	if (::poptGetNextOpt (optionsContext) < -1)
		{cerr << "## ERROR:  Bad command line argument(s)" << endl;		return 1;}
	optionsContext = ::poptFreeContext (optionsContext);

	//	Find the requested device...
	const string	deviceSpec	(pDeviceSpec ? pDeviceSpec : "0");
	const string	format		(pFormat ? ::ToLower (pFormat) : "verbose");
	const string	scale		(pScale ? ::ToLower (pScale) : "celsius");
	const string	fan			(pFan ? ::ToLower (pFan) : "");
	const string	logInterval	(pLogInterval ? pLogInterval : "");
	uint32_t		intervalSecs(0);

	if (format != "brief" && format != "b" && format != "verbose" && format != "v" && format != "json")
		{cerr << "## ERROR:  Bad '--format' value '" << format << "'" << endl;  return 1;}

	CNTV2Card		theDevice;
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpec, theDevice))
		{cerr << "## ERROR:  Device '" << deviceSpec << "' not found" << endl;  return 2;}
	const NTV2DeviceID	deviceID	(theDevice.GetDeviceID());

	CHECK_FALSE (theDevice.IsOpen ());
	ostringstream	oss;
	oss << ::NTV2DeviceIDToString (deviceID) << " - " << theDevice.GetIndexNumber();

	if (!::NTV2DeviceCanMeasureTemperature(deviceID))
		{cerr << "## ERROR:  Device '" << oss.str() << "' cannot read its temperature" << endl;  return 3;}

	if (!logInterval.empty ())
	{
		stringstream	iss	(logInterval);
		iss >> intervalSecs;
		if (!intervalSecs)
			{cerr << "## ERROR:  Zero specified for --log interval" << endl;  return 2;}
	}

	if (fan == "on" || fan == "hi" || fan == "high")
	{
		if (::NTV2DeviceCanThermostat(deviceID))
		{
			CHECK_FALSE (theDevice.WriteRegister (kVRegUseThermostat, 0));				//	Turn off thermostat
			CHECK_FALSE (theDevice.WriteRegister (kVRegFanSpeed, NTV2_FanSpeed_High));	//	Set fan to HI
		}
		else
			CHECK_FALSE (theDevice.WriteRegister (kRegSysmonConfig2, 1, BIT(16), 16));	//	Set bit 16 to turn ON
		if (format == "verbose" || format == "v")
			cout << "Fan turned ON on '" << oss.str () << "'" << endl;
		else if (format == "brief" || format == "b")
			cout << "1" << endl;
		return 0;
	}
	else if (fan == "off" || fan == "lo" || fan == "low")
	{
		if (::NTV2DeviceCanThermostat(deviceID))
		{
			CHECK_FALSE (theDevice.WriteRegister (kVRegUseThermostat, 0));	//	Turn off thermostat
			CHECK_FALSE (theDevice.WriteRegister (kVRegFanSpeed, NTV2_FanSpeed_Low));	//	Set fan to LO
		}
		else
			CHECK_FALSE (theDevice.WriteRegister (kRegSysmonConfig2, 0, BIT(16), 16));	//	Clear bit 16 to turn OFF
		if (format == "verbose" || format == "v")
			cout << "Fan turned OFF on '" << oss.str () << "'" << endl;
		else if (format == "brief" || format == "b")
			cout << "0" << endl;
		return 0;
	}
	else if (fan == "med")
	{
		if (!::NTV2DeviceCanThermostat(deviceID))
			{cerr << "## ERROR:  Fan on '" << oss.str () << "' cannot be set to 'MED'" << endl;  return 2;}

		CHECK_FALSE (theDevice.WriteRegister (kVRegUseThermostat, 0));				//	Turn off thermostat
		CHECK_FALSE (theDevice.WriteRegister (kVRegFanSpeed, NTV2_FanSpeed_Medium));	//	Set fan to MED
		if (format == "verbose" || format == "v")
			cout << "Fan set to MED on '" << oss.str () << "'" << endl;
		else if (format == "brief" || format == "b")
			cout << "1" << endl;
		return 0;
	}
	else if (fan == "auto")
	{
		if (!::NTV2DeviceCanThermostat(deviceID))
			{cerr << "## ERROR:  Device '" << oss.str() << "' does not support 'auto' fan feature" << endl;  return 2;}

		CHECK_FALSE (theDevice.WriteRegister (kVRegUseThermostat, 1));	//	Turn on thermostat
		if (format == "verbose" || format == "v")
			cout << "Automatic fan control enabled on '" << oss.str () << "'" << endl;
		else if (format == "brief" || format == "b")
			cout << "0" << endl;
		return 0;
	}
	else if (!fan.empty () && fan != "read")
		{cerr << "## ERROR:  Illegal '--fan' value '" << fan << "'" << endl;  return 1;}

	if (!fan.empty()  &&  !::NTV2DeviceCanThermostat(deviceID))
		cerr << "## WARNING:  '--fan' specified for device without fan or has non-adjustable fan" << endl;

	const bool	logging			(!logInterval.empty ());
	const bool	showTemp		(logging ? true : fan.empty ());
	const bool	showFanState	(::NTV2DeviceCanThermostat(deviceID)  &&  (logging ? true : fan == "read"));

	do
	{
		string	unitsStr;
		double	dieTemp	(0.0);
		string	fanStateStr;
		bool	afcEnabled (false);
		result = ::ReadTempAndFanState (theDevice, scale, dieTemp, unitsStr, fanStateStr, afcEnabled);
		if (result)
			return result;

		if (format == "verbose" || format == "v")
			cout << "'" << oss.str () << "'";
		else if (format == "json")
		{
			const time_t	now		(time (AJA_NULL));
			const string	nowStr	(asctime (localtime (&now)));
			cout	<< "{\"timestamp\": \"" << nowStr.substr (0, nowStr.length () - 1)
					<< "\", \"name\": \"" << oss.str () << "\", \"deviceIndex\": " << theDevice.GetIndexNumber ();
		}

		if (showTemp)
		{
			//	Report the temperature...
			if (format == "brief" || format == "b")
				cout << dieTemp;
			else if (format == "verbose" || format == "v")
				cout << " die temperature is " << dieTemp << " " << unitsStr;
			else if (format == "json")
				cout << ", \"temperature\": " << dieTemp << ", \"temperatureScale\": \"" << scale << "\"";
		}
		if (showFanState)
		{
			//	Report the fan state...
			if (showTemp)
				cout << ", ";
			else if (format == "verbose" || format == "v")
				cout << " ";
			else if (format == "json")
				cout << ", ";
			if (format == "brief" || format == "b")
				cout << (fanStateStr == "OFF" ? "0" : "1");
			else if (format == "verbose" || format == "v")
				cout	<< (::NTV2DeviceCanThermostat (deviceID) ? "fan speed is " : "fan is ") << fanStateStr
						<< (afcEnabled ? " (auto)" : "");
			else if (format == "json")
				cout	<< "\"fanIsOn\": \"" << fanStateStr
						<< "\", \"autoFanControl\": " << (afcEnabled ? "true" : "false");
		}

		if (format == "brief" || format == "b" || format == "verbose" || format == "v")
			cout << endl;
		else if (format == "json")
			cout << "}" << endl;

		if (!logging)
			break;
		AJATime::Sleep (int32_t(intervalSecs) * 1000);
	} while (logging);

	return result;

}	//	main
