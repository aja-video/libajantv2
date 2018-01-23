#!/usr/bin/env python

"""
	SUMMARY:
		Generates a .hpp and corresponding .hh file that contains "device features" code from tables (CSV files).

	SYNTAX:
		ntv2devicefeatures.py   [--verbose|-v]   [--ohh  hhOutputFolder]   [-ohpp  hppOutputFolder]  [--csv|-c  pathToCSVs]

	REQUIRED PARAMETERS:
		none

	OPTIONAL PARAMETERS:
		[--ohh  outputFolder]		Optionally specifies folder path into which .hh file is written.
									If not specified, .hh file is written into current directory.

		[--ohpp  outputFolder]		Optionally specifies folder path into which .hpp file is written.
									If not specified, .hpp file is written into current directory.

		[--csv  folderPath]			Optionally specifies folder path to look for .csv files.
									If not specified, looks in current directory.

		[--verbose]					Optionally emits progress information to stdout.
"""

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import sys
import os
import stat
import argparse
import datetime
import csv


def parse_args ():
    """ Parse the command line arguments """
    parser = argparse.ArgumentParser (description = "Generates device features code from tables (CSV files)")
    parser.add_argument ('-c', '--csv',				help = """path to folder in which to look for .csv files""")
    parser.add_argument ('--ohh',					help = """path to folder into which .hh file is written""")
    parser.add_argument ('--ohpp',					help = """path to folder into which .hpp file is written""")
    parser.add_argument ('-v', '--verbose',			help = """Use verbose output.""",									action = 'store_true')
    parser.add_argument ('-f', '--failwarnings',	help = """Treat warnings as errors.""",								action = 'store_true')
    return parser.parse_args ()


def generate_code_NTV2DeviceCanDoFrameBufferFormat (args, hppPath, hPath, device_names, supported_formats):
    if hppPath:
        with open (hppPath, 'a+') as f:
            f.write ("\n\n/**\n\tNTV2DeviceCanDoFrameBufferFormat\n**/\n")
            f.write ("bool NTV2DeviceCanDoFrameBufferFormat (const NTV2DeviceID inDeviceID, const NTV2FrameBufferFormat inFBFormat)\n")
            f.write ("{\n")
            f.write ("\tswitch (inDeviceID)\n")
            f.write ("\t{\n")
            for device_name in device_names:
                num_supported_formats = 0
                for supported_format in supported_formats:
                    if supported_format [device_name] == "X":
                        num_supported_formats = num_supported_formats + 1
                if num_supported_formats > 0:
                    f.write ("\t\tcase %s:\n" % (device_name))
                    f.write ("\t\t\tswitch (inFBFormat)\n")
                    f.write ("\t\t\t{\n")
                    for supported_format in supported_formats:
                        format_name = supported_format ['NTV2FrameBufferFormat']
                        if supported_format [device_name] == "X":
                            f.write ("\t\t\t\tcase %s:\n" % (format_name))
                    f.write ("\t\t\t\t\treturn true;\n")
                    f.write ("\t\t\t\tdefault:\n")
                    f.write ("\t\t\t\t\tbreak;\n")
                    f.write ("\t\t\t}\t//  switch on inFBFormat for device %s\n" % (device_name))
                    f.write ("\t\t\tbreak;\t//  %s\n" % (device_name))
                    f.write ("\n")
            f.write ("\t\tdefault:\n")
            f.write ("\t\t\tbreak;\n")
            f.write ("\t}\t//  switch on device ID\n")
            f.write ("\n")
            f.write ("\treturn false;\n")
            f.write ("\n")
            f.write ("}\t//  NTV2DeviceCanDoFrameBufferFormat (auto-generated)\n")
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote 'NTV2DeviceCanDoFrameBufferFormat' function into '%s'" % (sys.argv[0], hppPath))
    if hPath:
        with open (hPath, 'a+') as f:
            f.write ("\n/**\n\t@brief\t\tReturns true if the device having the given ID supports the given NTV2FrameBufferFormat.\n")
            f.write ("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write ("\t@param[in]\tinFBFormat\t\tSpecifies the NTV2FrameBufferFormat.\n")
            f.write ("\t@return\t\tTrue if the device supports the given frame buffer (pixel) format.\n")
            f.write ("**/\n")
            f.write ("AJAExport bool NTV2DeviceCanDoFrameBufferFormat (const NTV2DeviceID inDeviceID, const NTV2FrameBufferFormat inFBFormat);\n")
    return 0


def generate_code_NTV2DeviceCanDoVideoFormat (args, hppPath, hPath, device_names, supported_formats):
    if hppPath:
        with open (hppPath, 'a+') as f:
            f.write ("\n\n/**\n\tNTV2DeviceCanDoVideoFormat\n**/\n")
            f.write ("bool NTV2DeviceCanDoVideoFormat (const NTV2DeviceID inDeviceID, const NTV2VideoFormat inVideoFormat)\n")
            f.write ("{\n")
            f.write ("\tswitch (inDeviceID)\n")
            f.write ("\t{\n")
            for device_name in device_names:
                num_supported_formats = 0
                for supported_format in supported_formats:
                    if supported_format [device_name] == "X":
                        num_supported_formats = num_supported_formats + 1
                if num_supported_formats > 0:
                    f.write ("\t\tcase %s:\n" % (device_name))
                    f.write ("\t\t\tswitch (inVideoFormat)\n")
                    f.write ("\t\t\t{\n")
                    for supported_format in supported_formats:
                        format_name = supported_format ['NTV2VideoFormat']
                        if supported_format [device_name] == "X":
                            f.write ("\t\t\t\tcase %s:\n" % (format_name))
                    f.write ("\t\t\t\t\treturn true;\n")
                    f.write ("\t\t\t\tdefault:\n")
                    f.write ("\t\t\t\t\tbreak;\n")
                    f.write ("\t\t\t}\t//  switch on inVideoFormat for device %s\n" % (device_name))
                    f.write ("\t\t\tbreak;\t//  %s\n" % (device_name))
                    f.write ("\n")
            f.write ("\t\tdefault:\n")
            f.write ("\t\t\tbreak;\n")
            f.write ("\t}\t//  switch on device ID\n")
            f.write ("\n")
            f.write ("\treturn false;\n")
            f.write ("\n")
            f.write ("}\t//  NTV2DeviceCanDoVideoFormat (auto-generated)\n")
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote 'NTV2DeviceCanDoVideoFormat' function into '%s'" % (sys.argv[0], hppPath))
    if hPath:
        with open (hPath, 'a+') as f:
            f.write ("\n/**\n\t@brief\t\tReturns true if the device having the given ID supports the given NTV2VideoFormat.\n")
            f.write ("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write ("\t@param[in]\tinVideoFormat\tSpecifies the NTV2VideoFormat.\n")
            f.write ("\t@return\t\tTrue if the device supports the given video format.\n")
            f.write ("**/\n")
            f.write ("AJAExport bool NTV2DeviceCanDoVideoFormat (const NTV2DeviceID inDeviceID, const NTV2VideoFormat inVideoFormat);\n")
    return 0


def generate_code_NTV2DeviceCanDoWidget (args, hppPath, hPath, device_names, supported_widgets):
    if hppPath:
        with open (hppPath, 'a+') as f:
            f.write ("\n\n/**\n\tNTV2DeviceCanDoWidget\n**/\n")
            f.write ("bool NTV2DeviceCanDoWidget (const NTV2DeviceID inDeviceID, const NTV2WidgetID inWidgetID)\n")
            f.write ("{\n")
            f.write ("\tswitch (inDeviceID)\n")
            f.write ("\t{\n")
            for device_name in device_names:
                num_supported_widgets = 0
                for supported_widget in supported_widgets:
                    if supported_widget [device_name] == "X":
                        num_supported_widgets = num_supported_widgets + 1
                if num_supported_widgets > 0:
                    f.write ("\t\tcase %s:\n" % (device_name))
                    f.write ("\t\t\tswitch (inWidgetID)\n")
                    f.write ("\t\t\t{\n")
                    for supported_widget in supported_widgets:
                        widget_name = supported_widget ['NTV2WidgetID']
                        if supported_widget [device_name] == "X":
                            f.write ("\t\t\t\tcase %s:\n" % (widget_name))
                    f.write ("\t\t\t\t\treturn true;\n")
                    f.write ("\t\t\t\tdefault:\n")
                    f.write ("\t\t\t\t\tbreak;\n")
                    f.write ("\t\t\t}\t//  switch on inWidgetID for device %s\n" % (device_name))
                    f.write ("\t\t\tbreak;\t//  %s\n" % (device_name))
                    f.write ("\n")
            f.write ("\t\tdefault:\n")
            f.write ("\t\t\tbreak;\n")
            f.write ("\t}\t//  switch on device ID\n")
            f.write ("\n")
            f.write ("\treturn false;\n")
            f.write ("\n")
            f.write ("}\t//  NTV2DeviceCanDoWidget (auto-generated)\n")
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote 'NTV2DeviceCanDoWidget' function into '%s'" % (sys.argv[0], hppPath))
    if hPath:
        with open (hPath, 'a+') as f:
            f.write ("\n/**\n\t@brief\t\tReturns true if the device having the given ID supports the given NTV2WidgetID.\n")
            f.write ("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write ("\t@param[in]\tinWidgetID\t\tSpecifies the NTV2WidgetID.\n")
            f.write ("\t@return\t\tTrue if the device supports the given widget.\n")
            f.write ("**/\n")
            f.write ("AJAExport bool NTV2DeviceCanDoWidget (const NTV2DeviceID inDeviceID, const NTV2WidgetID inWidgetID);\n")
    return 0


def generate_code_NTV2DeviceCanDoConversionMode (args, hppPath, hPath, device_names, supported_conversion_modes):
    if hppPath:
        with open (hppPath, 'a+') as f:
            f.write ("\n\n/**\n\tNTV2DeviceCanDoConversionMode\n**/\n")
            f.write ("bool NTV2DeviceCanDoConversionMode (const NTV2DeviceID inDeviceID, const NTV2ConversionMode inConversionMode)\n")
            f.write ("{\n")
            f.write ("\tswitch (inDeviceID)\n")
            f.write ("\t{\n")
            for device_name in device_names:
                num_supported_conversion_modes = 0
                for supported_conversion_mode in supported_conversion_modes:
                    if supported_conversion_mode [device_name] == "X":
                        num_supported_conversion_modes = num_supported_conversion_modes + 1
                if num_supported_conversion_modes > 0:
                    f.write ("\t\tcase %s:\n" % (device_name))
                    f.write ("\t\t\tswitch (inConversionMode)\n")
                    f.write ("\t\t\t{\n")
                    for supported_conversion_mode in supported_conversion_modes:
                        conversion_mode = supported_conversion_mode ['NTV2ConversionMode']
                        if supported_conversion_mode [device_name] == "X":
                            f.write ("\t\t\t\tcase %s:\n" % (conversion_mode))
                    f.write ("\t\t\t\t\treturn true;\n")
                    f.write ("\t\t\t\tdefault:\n")
                    f.write ("\t\t\t\t\tbreak;\n")
                    f.write ("\t\t\t}\t//  switch on inConversionMode for device %s\n" % (device_name))
                    f.write ("\t\t\tbreak;\t//  %s\n" % (device_name))
                    f.write ("\n")
            f.write ("\t\tdefault:\n")
            f.write ("\t\t\tbreak;\n")
            f.write ("\t}\t//  switch on device ID\n")
            f.write ("\n")
            f.write ("\treturn false;\n")
            f.write ("\n")
            f.write ("}\t//  NTV2DeviceCanDoConversionMode (auto-generated)\n")
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote 'NTV2DeviceCanDoConversionMode' function into '%s'" % (sys.argv[0], hppPath))
    if hPath:
        with open (hPath, 'a+') as f:
            f.write ("\n/**\n\t@brief\t\tReturns true if the device having the given ID supports the given NTV2ConversionMode.\n")
            f.write ("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write ("\t@param[in]\tinConversionMode\tSpecifies the NTV2ConversionMode.\n")
            f.write ("\t@return\t\tTrue if the device supports the given conversion mode.\n")
            f.write ("**/\n")
            f.write ("AJAExport bool NTV2DeviceCanDoConversionMode (const NTV2DeviceID inDeviceID, const NTV2ConversionMode inConversionMode);\n")
    return 0


def generate_code_NTV2DeviceCanDoDSKMode (args, hppPath, hPath, device_names, supported_dsk_modes):
    if hppPath:
        with open (hppPath, 'a+') as f:
            f.write ("\n\n/**\n\tNTV2DeviceCanDoDSKMode\n**/\n")
            f.write ("bool NTV2DeviceCanDoDSKMode (const NTV2DeviceID inDeviceID, const NTV2DSKMode inDSKMode)\n")
            f.write ("{\n")
            f.write ("\tswitch (inDeviceID)\n")
            f.write ("\t{\n")
            for device_name in device_names:
                num_supported_dsk_modes = 0
                for supported_dsk_mode in supported_dsk_modes:
                    if supported_dsk_mode [device_name] == "X":
                        num_supported_dsk_modes = num_supported_dsk_modes + 1
                if num_supported_dsk_modes > 0:
                    f.write ("\t\tcase %s:\n" % (device_name))
                    f.write ("\t\t\tswitch (inDSKMode)\n")
                    f.write ("\t\t\t{\n")
                    for supported_dsk_mode in supported_dsk_modes:
                        dsk_mode = supported_dsk_mode ['NTV2DSKMode']
                        if supported_dsk_mode [device_name] == "X":
                            f.write ("\t\t\t\tcase %s:\n" % (dsk_mode))
                    f.write ("\t\t\t\t\treturn true;\n")
                    f.write ("\t\t\t\tdefault:\n")
                    f.write ("\t\t\t\t\tbreak;\n")
                    f.write ("\t\t\t}\t//  switch on inDSKMode for device %s\n" % (device_name))
                    f.write ("\t\t\tbreak;\t//  %s\n" % (device_name))
                    f.write ("\n")
            f.write ("\t\tdefault:\n")
            f.write ("\t\t\tbreak;\n")
            f.write ("\t}\t//  switch on device ID\n")
            f.write ("\n")
            f.write ("\treturn false;\n")
            f.write ("\n")
            f.write ("}\t//  NTV2DeviceCanDoDSKMode (auto-generated)\n")
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote 'NTV2DeviceCanDoDSKMode' function into '%s'" % (sys.argv[0], hppPath))
    if hPath:
        with open (hPath, 'a+') as f:
            f.write ("\n/**\n\t@brief\t\tReturns true if the device having the given ID supports the given NTV2DSKMode.\n")
            f.write ("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write ("\t@param[in]\tinDSKMode\t\tSpecifies the NTV2DSKMode.\n")
            f.write ("\t@return\t\tTrue if the device supports the given DSK mode.\n")
            f.write ("**/\n")
            f.write ("AJAExport bool NTV2DeviceCanDoDSKMode (const NTV2DeviceID inDeviceID, const NTV2DSKMode inDSKMode);\n")
    return 0


def generate_code_NTV2DeviceCanDoInputSource (args, hppPath, hPath, device_names, supported_input_sources):
    if hppPath:
        with open (hppPath, 'a+') as f:
            f.write ("\n\n/**\n\tNTV2DeviceCanDoInputSource\n**/\n")
            f.write ("bool NTV2DeviceCanDoInputSource (const NTV2DeviceID inDeviceID, const NTV2InputSource inInputSource)\n")
            f.write ("{\n")
            f.write ("\tswitch (inDeviceID)\n")
            f.write ("\t{\n")
            for device_name in device_names:
                num_supported_input_sources = 0
                for supported_input_source in supported_input_sources:
                    if supported_input_source [device_name] == "X":
                        num_supported_input_sources = num_supported_input_sources + 1
                if num_supported_input_sources > 0:
                    f.write ("\t\tcase %s:\n" % (device_name))
                    f.write ("\t\t\tswitch (inInputSource)\n")
                    f.write ("\t\t\t{\n")
                    nonDeprecate = False
                    for supported_input_source in supported_input_sources:
                        input_source_name = supported_input_source ['NTV2InputSource']
                        if supported_input_source [device_name] == "X":
                            if supported_input_source ['NonDeprecate?'] == "X" or supported_input_source ['NonDeprecate?'] == "1":
                                if not nonDeprecate:
                                    f.write ("\t\t\t#if !defined (NTV2_DEPRECATE)\n")
                                    nonDeprecate = True
                            elif nonDeprecate:
                                f.write ("\t\t\t#endif\t//\tif !defined (NTV2_DEPRECATE)\n")
                                nonDeprecate = False
                            f.write ("\t\t\t\tcase %s:\n" % (input_source_name))
                    if nonDeprecate:
                        f.write ("\t\t\t#endif\t//\tif !defined (NTV2_DEPRECATE)\n")
                    f.write ("\t\t\t\t\treturn true;\n")
                    f.write ("\t\t\t\tdefault:\n")
                    f.write ("\t\t\t\t\tbreak;\n")
                    f.write ("\t\t\t}\t//  switch on inInputSource for device %s\n" % (device_name))
                    f.write ("\t\t\tbreak;\t//  %s\n" % (device_name))
                    f.write ("\n")
            f.write ("\t\tdefault:\n")
            f.write ("\t\t\tbreak;\n")
            f.write ("\t}\t//  switch on device ID\n")
            f.write ("\n")
            f.write ("\treturn false;\n")
            f.write ("\n")
            f.write ("}\t//  NTV2DeviceCanDoInputSource (auto-generated)\n")
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote 'NTV2DeviceCanDoInputSource' function into '%s'" % (sys.argv[0], hppPath))
    if hPath:
        with open (hPath, 'a+') as f:
            f.write ("\n/**\n\t@brief\t\tReturns true if the device having the given ID supports the given NTV2InputSource.\n")
            f.write ("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write ("\t@param[in]\tinInputSource\tSpecifies the NTV2InputSource.\n")
            f.write ("\t@return\t\tTrue if the device supports the given input source.\n")
            f.write ("**/\n")
            f.write ("AJAExport bool NTV2DeviceCanDoInputSource (const NTV2DeviceID inDeviceID, const NTV2InputSource inInputSource);\n")
    return 0


def generate_code_NTV2DeviceCanDo (args, hppPath, hPath, device_names, supported_can_do_features):
    if hppPath:
        with open (hppPath, 'a+') as f:
            for supported_feature in supported_can_do_features:
                function_name = supported_feature ['FunctionName']
                description = supported_feature ['Brief']
                num_supported_devices = 0
                f.write ("\n\n/**\n\t%s\n" % (function_name))
                if description:
                    f.write ("\t%s\n" % (description))
                f.write ("**/\n")
                f.write ("bool %s (const NTV2DeviceID inDeviceID)\n" % (function_name))
                f.write ("{\n")
                for device_name in device_names:
                    if supported_feature [device_name] == "X":
                        num_supported_devices = num_supported_devices + 1
                if num_supported_devices > 0:
                    f.write ("\tswitch (inDeviceID)\n")
                    f.write ("\t{\n")
                    for device_name in device_names:
                        if supported_feature [device_name] == "X":
                            f.write ("\t\tcase %s:\n" % (device_name))
                    f.write ("\t\t\treturn true;\n")
                    f.write ("\t\tdefault:\n")
                    f.write ("\t\t\tbreak;\n")
                    f.write ("\t}\t//\tswitch on inDeviceID\n")
                else:
                    f.write ("\t(void) inDeviceID;\n")
                f.write ("\n")
                f.write ("\treturn false;\n")
                f.write ("\n")
                f.write ("}\t//  %s (auto-generated)\n" % (function_name))
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote %d 'NTV2DeviceCanDo...' functions into '%s'" % (sys.argv[0], len (supported_can_do_features), hppPath))
    if hPath:
        with open (hPath, 'a+') as f:
            for supported_feature in supported_can_do_features:
                function_name = supported_feature ['FunctionName']
                description = supported_feature ['Brief']
                num_supported_devices = 0
                f.write ("\n/**\n")
                if description:
                    f.write ("\t@brief\t\t%s\n" % (description))
                f.write ("\t@param[in]\tinDeviceID		Specifies the NTV2DeviceID of interest.\n")
                f.write ("**/\n")
                f.write ("AJAExport bool %s (const NTV2DeviceID inDeviceID);\n" % (function_name))
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote %d 'NTV2DeviceCanDo...' functions into '%s'" % (sys.argv[0], len (supported_can_do_features), hPath))
    return 0


def generate_code_NTV2DeviceGetNum (args, hppPath, hPath, device_names, get_nums):
    if hppPath:
        with open (hppPath, 'a+') as f:
            for get_num_feature in get_nums:
                function_name = get_num_feature ['FunctionName']
                description = get_num_feature ['Brief']
                return_type = get_num_feature ['Returns']
                f.write ("\n\n/**\n\t%s\n" % (function_name))
                if description:
                    f.write ("\t%s\n" % (description))
                f.write ("**/\n")
                f.write ("%s %s (const NTV2DeviceID inDeviceID)\n" % (return_type, function_name))
                f.write ("{\n")
                f.write ("\tswitch (inDeviceID)\n")
                f.write ("\t{\n")
                for device_name in device_names:
                    if device_name:
                        num_value = get_num_feature [device_name]
                        if len (device_name) < 14:
                            tabs = "\t\t\t"
                        elif len (device_name) < 18:
                            tabs = "\t\t"
                        else:
                            tabs = "\t"
                        f.write ("\t\tcase %s:%sreturn %s;\n" % (device_name, tabs, num_value))
                        if device_name == 'DEVICE_ID_NOTFOUND':
                            default_value = num_value
                            default_tabs = tabs
                f.write ("\t#if !defined (NTV2_DEPRECATE)\n")
                f.write ("\t\tdefault:%sreturn %s;\n" % (default_tabs, default_value))
                f.write ("\t#endif\t//\t!defined (NTV2_DEPRECATE)\n")
                f.write ("\t}\t//\tswitch on inDeviceID\n")
                f.write ("\n")
                f.write ("\treturn 0;\n")
                f.write ("\n")
                f.write ("}\t//  %s (auto-generated)\n" % (function_name))
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote %d 'NTV2DeviceGetNum...' functions into '%s'" % (sys.argv[0], len (get_nums), hppPath))
    if hPath:
        with open (hPath, 'a+') as f:
            for get_num_feature in get_nums:
                function_name = get_num_feature ['FunctionName']
                description = get_num_feature ['Brief']
                return_type = get_num_feature ['Returns']
                f.write ("\n/**\n")
                if description:
                    f.write ("\t@brief\t\t%s\n" % (description))
                f.write ("\t@param[in]\tinDeviceID		Specifies the NTV2DeviceID of interest.\n")
                f.write ("**/\n")
                f.write ("AJAExport %s %s (const NTV2DeviceID inDeviceID);\n" % (return_type, function_name))
            f.flush ()
            if args.verbose:
                print("## NOTE:  %s:  Wrote %d 'NTV2DeviceGetNum...' functions into '%s'" % (sys.argv[0], len (get_nums), hPath))
    return 0



def main ():
    #	CSV file names:
    vidFormatCSV		= "VideoFormats.csv"
    fbFormatCSV			= "FBFormats.csv"
    widgetsCSV			= "Widgets.csv"
    conversionModesCSV	= "ConversionModes.csv"
    inputSourcesCSV		= "InputSources.csv"
    dskModesCSV			= "DSKModes.csv"
    canDoCSV			= "CanDo.csv"
    getNumCSV			= "GetNum.csv"

    hppName				= "ntv2devicefeatures.hpp"
    hName				= "ntv2devicefeatures.hh"

    hhOutputDir = ""
    hppOutputDir = ""
    csvDir = ""

    args = parse_args ()
    if args.csv:
        csvDir = os.path.join (args.csv)
        if not os.path.exists (csvDir) or not os.path.isdir (csvDir):
            print("## ERROR:  CSV folder '%s' not found or not a folder" % (args.csv))
            return 404
        if args.verbose:
            print("## NOTE:  Looking for CSV files in folder '%s'" % (args.csv))
    else:
        csvDir = os.getcwd ()

    for csvFile in ["VideoFormats.csv", "FBFormats.csv", "Widgets.csv", "ConversionModes.csv", "DSKModes.csv", "InputSources.csv", "CanDo.csv", "GetNum.csv"]:
        csvPath = os.path.join (csvDir, csvFile)
        if not os.path.exists (csvPath):
            print("## ERROR:  CSV file '%s' not found" % (csvPath))
            return 404
        if os.path.isdir (csvPath):
            print("## ERROR:  CSV file '%s' is a folder, not a file" % (csvPath))
            return 404

    if args.ohh:
        hhOutputDir = os.path.join (args.ohh)
        if not os.path.exists (hhOutputDir) or not os.path.isdir (hhOutputDir):
            print("## ERROR:  '.hh' output folder '%s' not found or not a folder" % (args.ohh))
            return 404
        if args.verbose:
            print("## NOTE:  Will write '.hh' file into folder '%s'" % (args.ohh))
    else:
        hhOutputDir = os.getcwd ()

    if args.ohpp:
        hppOutputDir = os.path.join (args.ohpp)
        if not os.path.exists (hppOutputDir) or not os.path.isdir (hppOutputDir):
            print("## ERROR:  '.hpp' output folder '%s' not found or not a folder" % (args.ohpp))
            return 404
        if args.verbose:
            print("## NOTE:  Will write '.hpp' file into folder '%s'" % (args.ohpp))
    else:
        hppOutputDir = os.getcwd ()

    hppPath = os.path.join (hppOutputDir, hppName)
    if os.path.exists (hppPath):
        if os.path.isdir (hppPath):
            print("## ERROR:  Output HPP file '%s' is a folder" % (hppPath))
            return 505
        if args.verbose:
            print("## NOTE:  Will overwrite existing .hpp file '%s'" % (hppPath))

    hPath = os.path.join (hhOutputDir, hName)
    if os.path.exists (hPath):
        if os.path.isdir (hPath):
            print("## ERROR:  Output HH file '%s' is a folder" % (hPath))
            return 505
        if args.verbose:
            print("## NOTE:  Will overwrite existing .hh file '%s'" % (hPath))

    #	device_names ['DEVICE_ID_UNKNOWN', 'DEVICE_ID_CORVID1', ...]
    device_names = []

    #	supported_video_formats ['DEVICE_ID_KONA4'] == "X" means the device supports the video format
    #	supported_video_formats ['NTV2VideoFormat'] contains the video format name
    supported_video_formats = []
    with open (os.path.join (csvDir, vidFormatCSV), 'rU') as csvfile:
        reader = csv.DictReader (csvfile)
        for row in reader:
            supported_video_formats.append (row)

    with open (os.path.join (csvDir, vidFormatCSV), 'rU') as f:
        for line in f:
            if '\n' == line [-1]:
                line = line [:-1]
            if '\r' == line [-1]:
                line = line [:-1]
            if len (device_names) == 0:
                device_names = line.split (",")
                device_names.pop (0)

    #	supported_fb_formats ['DEVICE_ID_KONA4'] == "X" means the device supports that frame buffer format
    #	supported_fb_formats ['NTV2FrameBufferFormat'] contains the frame buffer format name
    supported_fb_formats = []
    with open (os.path.join (csvDir, fbFormatCSV), 'rU') as csvfile:
        reader = csv.DictReader (csvfile)
        for row in reader:
            supported_fb_formats.append (row)

    #	supported_widgets ['DEVICE_ID_KONA4'] == "X" means the device supports that widget
    #	supported_widgets ['NTV2WidgetID'] contains the widget name
    supported_widgets = []
    with open (os.path.join (csvDir, widgetsCSV), 'rU') as csvfile:
        reader = csv.DictReader (csvfile)
        for row in reader:
            supported_widgets.append (row)

    #	supported_conversion_modes ['DEVICE_ID_KONA4'] == "X" means the device supports that conversion mode
    #	supported_conversion_modes ['NTV2ConversionMode'] contains the conversion mode
    supported_conversion_modes = []
    with open (os.path.join (csvDir, conversionModesCSV), 'rU') as csvfile:
        reader = csv.DictReader (csvfile)
        for row in reader:
            supported_conversion_modes.append (row)

    #	supported_dsk_modes ['DEVICE_ID_KONA4'] == "X" means the device supports that DownStream Keyer mode
    #	supported_dsk_modes ['NTV2DSKMode'] contains the DSK mode
    supported_dsk_modes = []
    with open (os.path.join (csvDir, dskModesCSV), 'rU') as csvfile:
        reader = csv.DictReader (csvfile)
        for row in reader:
            supported_dsk_modes.append (row)

    #	supported_input_sources ['DEVICE_ID_KONA4'] == "X" means the device supports that input source
    #	supported_input_sources ['NTV2InputSource'] contains the input source
    supported_input_sources = []
    with open (os.path.join (csvDir, inputSourcesCSV), 'rU') as csvfile:
        reader = csv.DictReader (csvfile)
        for row in reader:
            supported_input_sources.append (row)

    #	supported_can_do_features ['DEVICE_ID_KONA4'] == "X" means the device supports that feature
    #	supported_can_do_features ['FunctionName'] contains the function name
    #	supported_can_do_features ['Brief'] contains the doxygen '@brief' description of the function
    supported_can_do_features = []
    with open (os.path.join (csvDir, canDoCSV), 'rU') as csvfile:
        reader = csv.DictReader (csvfile)
        for row in reader:
            supported_can_do_features.append (row)

    #	get_nums ['DEVICE_ID_KONA4'] contains the return value for that device
    #	get_nums ['FunctionName'] contains the function name
    #	get_nums ['Brief'] contains the doxygen '@brief' description of the function
    get_nums = []
    with open (os.path.join (csvDir, getNumCSV), 'rU') as csvfile:
        reader = csv.DictReader (csvfile)
        for row in reader:
            get_nums.append (row)

    with open (hppPath, 'w') as f:
        f.write ("/**\n\t@file\t\t%s\n\t@brief\t\tContains implementations of NTV2DeviceCanDo... and NTV2DeviceGetNum... functions.\n" % (hppName))
        f.write ("\t\t\t\tThis module is included at compile time from 'ntv2devicefeatures.cpp'.\n\t@copyright\t(C) 2004-2018 AJA Video Systems, Inc.\tProprietary and confidential information.\n")
        f.write ("\t@note\t\tGenerated by '%s' on %s.\n**/\n" % (sys.argv[0], datetime.datetime.now ().strftime ("%c")))

    with open (hPath, 'w') as f:
        f.write ("/**\n\t@file\t\t%s\n\t@brief\t\tDeclares NTV2DeviceCanDo... and NTV2DeviceGetNum... functions.\n" % (hName))
        f.write ("\t\t\t\tThis module is included at compile time from 'ntv2devicefeatures.h'.\n\t@copyright\t(C) 2004-2018 AJA Video Systems, Inc.\tProprietary and confidential information.\n")
        f.write ("\t@note\t\tGenerated by '%s' on %s.\n**/\n" % (sys.argv[0], datetime.datetime.now ().strftime ("%c")))
        f.write ("#ifndef NTV2DEVICEFEATURES_HH\n")
        f.write ("#define NTV2DEVICEFEATURES_HH\n")

    generate_code_NTV2DeviceCanDoVideoFormat (args, hppPath, hPath, device_names, supported_video_formats)
    generate_code_NTV2DeviceCanDoFrameBufferFormat (args, hppPath, hPath, device_names, supported_fb_formats)
    generate_code_NTV2DeviceCanDoWidget (args, hppPath, hPath, device_names, supported_widgets)
    generate_code_NTV2DeviceCanDoConversionMode (args, hppPath, hPath, device_names, supported_conversion_modes)
    generate_code_NTV2DeviceCanDoDSKMode (args, hppPath, hPath, device_names, supported_dsk_modes)
    generate_code_NTV2DeviceCanDoInputSource (args, hppPath, hPath, device_names, supported_input_sources)
    generate_code_NTV2DeviceCanDo (args, hppPath, hPath, device_names, supported_can_do_features)
    generate_code_NTV2DeviceGetNum (args, hppPath, hPath, device_names, get_nums)

    with open (hPath, 'a+') as f:
        f.write ("\n\n#endif\t//\tNTV2DEVICEFEATURES_HH\n")

    return 0


if __name__ == '__main__':
    status = main ()
    sys.exit (status)
