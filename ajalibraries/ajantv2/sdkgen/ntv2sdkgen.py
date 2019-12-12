#!/usr/bin/env python

"""
	SUMMARY:
		Generates the device features code from device .gen files.

	SYNTAX:
		ntv2sdkgen.py   [--verbose|-v]   [--ajantv2  ajantv2Folder]   [--failwarnings|-f]  [--ohh  hhFolder]  [--ohpp  hppFolder]

	REQUIRED PARAMETERS:
		none

	OPTIONAL PARAMETERS:
		[--ajantv2  ajantv2Path]	Optionally specifies folder path in which to find the "ajalibraries/ajantv2" folder.
									If not specified, looks in the current directory.

		[--verbose]					Optionally emits progress information to stdout.

		[--ohh  outputFolder]		Optionally writes the .hh file into the specified folder.
									If not specified, no .hh file is written.

		[--ohpp  outputFolder]		Optionally writes the .hpp file into the specified folder.
									If not specified, no .hpp file is written.
"""

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import sys
import os
import stat
import argparse
import datetime
import csv
import fnmatch
import platform
import shutil
import time
import uuid


def parse_args ():
    """ Parse the command line arguments """
    parser = argparse.ArgumentParser (description = "Generates device features code from device .gen files")
    parser.add_argument ('--ajantv2',				help = """path to 'ajalibraries/ajantv2' folder""")
    parser.add_argument ('--ohh',					help = """write .hh file in given folder""")
    parser.add_argument ('--ohpp',					help = """write .hpp file in given folder""")
    parser.add_argument ('-v', '--verbose',			help = """Use verbose output.""",									action = 'store_true')
    parser.add_argument ('-f', '--failwarnings',	help = """Treat warnings as errors.""",								action = 'store_true')
    parser.add_argument ('-u', '--unused',			help = """Show unused/unreferenced symbols.""",						action = 'store_true')
    return parser.parse_args ()


# Reads 'ntv2enums.h' and returns a dict containing:
#	'err':			Result code (0 == success)
#	'device_ids':	Map whose keys are all legal NTV2DeviceID enums
def get_canonical_device_ids (args, ntv2enums):
    device_ids = {}
    line_num = 0
    if not os.path.exists(ntv2enums):
        print("## ERROR:  file '%s' not found" % (ntv2enums))
        return {'err': 404,	'device_ids': {}}
    if os.path.isdir(ntv2enums):
        print("## ERROR:  file '%s' is a folder" % (ntv2enums))
        return {'err': 501,	'device_ids': {}}
    with open(ntv2enums) as f:
        for line in f:
            line_num = line_num + 1
            line = line.lstrip().rstrip()
            if len(line) == 0:
                continue
            if len(line)>1 and line[0] == "/" and line[1] == "/":
                #print("SKIPPED: %s" % (line))
                continue
            if len(line)>0 and line[0] == "#":
                #print("SKIPPED: %s" % (line))
                continue
            if "deprecate" in line:
                #print("SKIPPED: %s" % (line))
                continue
            pieces = line.split(None,2)
            key = pieces[0]
            if key[0:10] == "DEVICE_ID_":
                if key == "DEVICE_ID_NOTFOUND":
                    break
                #print("ACCEPTED: %s" % (key))
                device_ids[key] = False
    if args.verbose:
        print("## NOTE:  %d canonical device IDs parsed from %d lines in 'ntv2enums.h'" % (len(device_ids), line_num))
        #print(device_ids)
    return {'err': 0,	'device_ids': device_ids}


# Reads 'ntv2enums.h' and returns a dict containing:
#	'err':			Result code (0 == success)
#	'videoformats':	Map whose keys are all legal NTV2VideoFormat enums
def get_canonical_video_formats (args, ntv2enums):
    videoformats = {}
    line_num = 0
    if not os.path.exists(ntv2enums):
        print("## ERROR:  file '%s' not found" % (ntv2enums))
        return {'err': 404,	'videoformats': {}}
    if os.path.isdir(ntv2enums):
        print("## ERROR:  file '%s' is a folder" % (ntv2enums))
        return {'err': 501,	'videoformats': {}}
    with open(ntv2enums) as f:
        for line in f:
            line_num = line_num + 1
            line = line.lstrip().rstrip()
            if len(line) == 0:
                #print("%d: %s\tZERO LENGTH" % (line_num, line))
                continue
            if len(line)>1 and line[0] == "/" and line[1] == "/":
                #print("%d: %s\tCOMMENT LINE" % (line_num, line))
                continue
            pieces = line.split()
            key = pieces[0]
            if key.startswith(","):
                key = key.replace(",","").lstrip().rstrip()
            if key.startswith("NTV2_FORMAT_"):
                #print(pieces)
                if "=" in key:
                    print("## ERROR:  Line %d in 'ntv2enums.h':  Missing whitespace around '='" % (line_num))
                    return {'err': 503,	'videoformats': {}}
                if len(pieces)>1 and pieces[1].startswith("=") and pieces[1] != "=":
                    print("## ERROR:  Line %d in 'ntv2enums.h':  Missing whitespace around '='" % (line_num))
                    return {'err': 503,	'videoformats': {}}
                if "," in key:
                    key = key.replace(",","")
                if len(pieces)>1 and pieces[1] is "=":
                    if not pieces[2].startswith("NTV2_FORMAT_FIRST_"):
                        #print("## DEBUG:  skipped '%s' -- dupe of '%s'" % (key, pieces[2]))
                        key = ""
                        continue
                #print("%d: %s" % (line_num, key))
                if key.startswith("NTV2_FORMAT_FIRST_"):
                    #print("## DEBUG:  skipped '%s'" % (key))
                    continue
                if key.startswith("NTV2_FORMAT_END_"):
                    if key == "NTV2_FORMAT_END_4K_DEF_FORMATS":
                        #print("## DEBUG:  skipped '%s'" % (key))
                        # Special case:
                        # NTV2_FORMAT_END_4K_DEF_FORMATS happens to match ordinal value 110, which matches
                        # NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT2 and NTV2_FORMAT_1080p_2K_6000 (this one matters).
                        # All other "NTV2_FORMAT_END_" ones can be skipped, but NTV2_FORMAT_END_4K_DEF_FORMATS can't,
                        # because it will cause a "duplicate case" error in _DEBUG builds.
                        continue
                    #print("## DEBUG:  retaining '%s'" % (key))
                if key.startswith("NTV2_FORMAT_DEPRECATED_"):
                    #print("## DEBUG:  skipped '%s'" % (key))
                    continue
                if key is "NTV2_FORMAT_UNKNOWN":
                    #print("## DEBUG:  skipped '%s'" % (key))
                    continue
                videoformats[key] = False
            elif key is "NTV2_MAX_NUM_VIDEO_FORMATS":
                break
    if args.verbose:
        print("## NOTE:  %d canonical NTV2VideoFormats parsed from %d lines in 'ntv2enums.h'" % (len(videoformats), line_num))
    #print "## DEBUG: CANONICAL LIST OF NTV2VideoFormats:", videoformats
    return {'err': 0,	'videoformats': videoformats}


# Reads 'ntv2enums.h' and returns a dict containing:
#	'err':			Result code (0 == success)
#	'pixelformats':	Map whose keys are all legal NTV2FrameBufferFormat enums
def get_canonical_pixel_formats (args, ntv2enums):
    pixelformats = {}
    line_num = 0
    if not os.path.exists(ntv2enums):
        print("## ERROR:  file '%s' not found" % (ntv2enums))
        return {'err': 404,	'pixelformats': {}}
    if os.path.isdir(ntv2enums):
        print("## ERROR:  file '%s' is a folder" % (ntv2enums))
        return {'err': 501,	'pixelformats': {}}
    with open(ntv2enums) as f:
        for line in f:
            line_num = line_num + 1
            line = line.lstrip().rstrip()
            if len(line) == 0:
                #print("%d: %s\tZERO LENGTH" % (line_num, line))
                continue
            if len(line)>1 and line[0] == "/" and line[1] == "/":
                #print("%d: %s\tCOMMENT LINE" % (line_num, line))
                continue
            pieces = line.split(None,2)
            key = pieces[0]
            if key.startswith(","):
                key = key.replace(",","").lstrip().rstrip()
            if key[0:9] == "NTV2_FBF_":
                if "=" in key:
                    pieces = key.split("=")
                    key = pieces[0].lstrip().rstrip()
                if "," in key:
                    key = key.replace(",","")
                if "NTV2_FBF_FIRST" in key:
                	continue
                if "NTV2_FBF_LAST" in key:
                    if key == "NTV2_FBF_LAST":
                        break
                    continue
                #print("%d: %s" % (line_num, key))
                pixelformats[key] = False
    if args.verbose:
        print("## NOTE:  %d canonical NTV2FrameBufferFormats parsed from %d lines in 'ntv2enums.h'" % (len(pixelformats), line_num))
    #print "## DEBUG: CANONICAL LIST OF NTV2FrameBufferFormats:", pixelformats
    return {'err': 0,	'pixelformats': pixelformats}


# Reads 'ntv2enums.h' and returns a dict containing:
#	'err':			Result code (0 == success)
#	'inputsources':	Map whose keys are all legal NTV2InputSource enums
def get_canonical_input_sources (args, ntv2enums):
    inputsources = {}
    line_num = 0
    if not os.path.exists(ntv2enums):
        print("## ERROR:  file '%s' not found" % (ntv2enums))
        return {'err': 404,	'inputsources': {}}
    if os.path.isdir(ntv2enums):
        print("## ERROR:  file '%s' is a folder" % (ntv2enums))
        return {'err': 501,	'inputsources': {}}
    with open(ntv2enums) as f:
        for line in f:
            line_num = line_num + 1
            line = line.lstrip().rstrip()
            if len(line) == 0:
                #print("%d: %s\tZERO LENGTH" % (line_num, line))
                continue
            if len(line)>1 and line[0] == "/" and line[1] == "/":
                #print("%d: %s\tCOMMENT LINE" % (line_num, line))
                continue
            pieces = line.split(None,2)
            key = pieces[0]
            if key.startswith(","):
                key = key.replace(",","").lstrip().rstrip()
            if key[0:17] == "NTV2_INPUTSOURCE_":
                if "=" in key:
                    pieces = key.split("=")
                    key = pieces[0]
                if "," in key:
                    key = key.replace(",","")
                #print("%d: %s" % (line_num, key))
                inputsources[key] = False
                if key == "NTV2_INPUTSOURCE_SDI8":
                	break
    if args.verbose:
        print("## NOTE:  %d canonical NTV2InputSources parsed from %d lines in 'ntv2enums.h'" % (len(inputsources), line_num))
    #print "## DEBUG: CANONICAL LIST OF NTV2InputSources:", inputsources
    return {'err': 0,	'inputsources': inputsources}


# Reads 'ntv2enums.h' and returns a dict containing:
#	'err':			Result code (0 == success)
#	'widgetids':	Map whose keys are all legal NTV2WidgetID enums
def get_canonical_widget_ids (args, ntv2enums):
    widgetids = {}
    line_num = 0
    if not os.path.exists(ntv2enums):
        print("## ERROR:  file '%s' not found" % (ntv2enums))
        return {'err': 404,	'widgetids': {}}
    if os.path.isdir(ntv2enums):
        print("## ERROR:  file '%s' is a folder" % (ntv2enums))
        return {'err': 501,	'widgetids': {}}
    with open(ntv2enums) as f:
        for line in f:
            line_num = line_num + 1
            line = line.lstrip().rstrip()
            if len(line) == 0:
                #print("%d: %s\tZERO LENGTH" % (line_num, line))
                continue
            if len(line)>1 and line[0] == "/" and line[1] == "/":
                #print("%d: %s\tCOMMENT LINE" % (line_num, line))
                continue
            pieces = line.split(None,2)
            key = pieces[0].lstrip().rstrip()
            if key.startswith(","):
                key = key.replace(",","").lstrip().rstrip()
            if key[0:8] == "NTV2_Wgt":
                if "=" in key:
                    pieces = key.split("=")
                    key = pieces[0].lstrip().rstrip()
                if "," in key:
                    key = key.replace(",","")
                #print("%d: %s" % (line_num, key))
                if key == "NTV2_WgtModuleTypeCount":
                	break
                widgetids[key] = False
    if args.verbose:
        print("## NOTE:  %d canonical NTV2WidgetIDs parsed from %d lines in 'ntv2enums.h'" % (len(widgetids), line_num))
    #print "## DEBUG: CANONICAL LIST OF NTV2WidgetIDs:", widgetids
    return {'err': 0,	'widgetids': widgetids}


# Reads 'ntv2enums.h' and returns a dict containing:
#	'err':			Result code (0 == success)
#	'dskmodes':		Map whose keys are all legal NTV2DSKMode enums
def get_canonical_dsk_modes (args, ntv2enums):
    dskmodes = {}
    line_num = 0
    if not os.path.exists(ntv2enums):
        print("## ERROR:  file '%s' not found" % (ntv2enums))
        return {'err': 404,	'dskmodes': {}}
    if os.path.isdir(ntv2enums):
        print("## ERROR:  file '%s' is a folder" % (ntv2enums))
        return {'err': 501,	'dskmodes': {}}
    with open(ntv2enums) as f:
        for line in f:
            line_num = line_num + 1
            line = line.lstrip().rstrip()
            if len(line) == 0:
                #print("%d: %s\tZERO LENGTH" % (line_num, line))
                continue
            if len(line)>1 and line[0] == "/" and line[1] == "/":
                #print("%d: %s\tCOMMENT LINE" % (line_num, line))
                continue
            pieces = line.split(None,2)
            key = pieces[0].lstrip().rstrip()
            if key.startswith(","):
                key = key.replace(",","").lstrip().rstrip()
            if key[0:12] == "NTV2_DSKMode":
                if "=" in key:
                    pieces = key.split("=")
                    key = pieces[0].lstrip().rstrip()
                if "," in key:
                    key = key.replace(",","")
                #print("%d: %s" % (line_num, key))
                if key == "NTV2_DSKModeMax":
                	break
                dskmodes[key] = False
    if args.verbose:
        print("## NOTE:  %d canonical NTV2DSKModes parsed from %d lines in 'ntv2enums.h'" % (len(dskmodes), line_num))
    #print "## DEBUG: CANONICAL LIST OF NTV2DSKModes:", dskmodes
    return {'err': 0,	'dskmodes': dskmodes}


# Reads 'ntv2enums.h' and returns a dict containing:
#	'err':			Result code (0 == success)
#	'convmodes':	Map whose keys are all legal NTV2ConversionMode enums
def get_canonical_conversion_modes (args, ntv2enums):
    convmodes = {}
    line_num = 0
    if not os.path.exists(ntv2enums):
        print("## ERROR:  file '%s' not found" % (ntv2enums))
        return {'err': 404,	'convmodes': {}}
    if os.path.isdir(ntv2enums):
        print("## ERROR:  file '%s' is a folder" % (ntv2enums))
        return {'err': 501,	'convmodes': {}}
    with open(ntv2enums) as f:
        for line in f:
            line_num = line_num + 1
            line = line.lstrip().rstrip()
            if len(line) == 0:
                #print("%d: %s\tZERO LENGTH" % (line_num, line))
                continue
            if len(line)>1 and line[0] == "/" and line[1] == "/":
                #print("%d: %s\tCOMMENT LINE" % (line_num, line))
                continue
            pieces = line.split(None,2)
            key = pieces[0].lstrip().rstrip()
            if key.startswith(","):
                key = key.replace(",","").lstrip().rstrip()
            first8 = key[0:8]
            if "to" in key and (first8 == "NTV2_108" or first8 == "NTV2_720" or first8 == "NTV2_525" or first8 == "NTV2_625" or first8 == "NTV2_720"):
                if "=" in key:
                    pieces = key.split("=")
                    key = pieces[0].lstrip().rstrip()
                if "," in key:
                    key = key.replace(",","")
                #print("%d: %s" % (line_num, key))
                if key == "NTV2_NUM_CONVERSIONMODES":
                	break
                convmodes[key] = False
    if args.verbose:
        print("## NOTE:  %d canonical NTV2ConversionModes parsed from %d lines in 'ntv2enums.h'" % (len(convmodes), line_num))
    #print "## DEBUG: CANONICAL LIST OF NTV2ConversionModes:", convmodes
    return {'err': 0,	'convmodes': convmodes}


# Reads a canonical definition file and returns a dict containing:
#	'err':			Result code (0 == success)
#	'functions':	Map whose keys are all legal function names
#	'return_types':	Map of function names to corresponding return type
#	'return_descs':	Map of function names to corresponding return value's description text
def get_canonical_functions (args, filepath, numcols = 2):
    functions = {}
    return_types = {}
    return_descs = {}
    line_num = 0
    if not os.path.exists(filepath):
        print("## ERROR:  file '%s' not found" % (filepath))
        return {'err': 404,	'functions': {},		'return_types': {},	'return_descs': {}}
    if os.path.isdir(filepath):
        print("## ERROR:  file '%s' is a folder" % (filepath))
        return {'err': 501,	'functions': {},		'return_types': {},	'return_descs': {}}
    with open(filepath) as f:
        for line in f:
            line_num = line_num + 1
            line = line.lstrip().rstrip()
            if len(line) == 0:
                #print("%d: %s\tZERO LENGTH" % (line_num, line))
                continue
            if len(line)>1 and line[0] == "/" and line[1] == "/":
                #print("%d: %s\tCOMMENT LINE" % (line_num, line))
                continue
            if len(line)>0 and line[0] == "#":
                #print("%d: %s\tCOMMENT LINE" % (line_num, line))
                continue
            pieces = line.split(None,numcols-1)
            key = pieces[0].lstrip().rstrip()
            if not key.startswith("NTV2Device"):
                print("## ERROR:  File %s:  Line %d:  Function name '%s' must start with 'NTV2Device'" % (filepath, line_num, key))
                return {'err': 503,	'functions': {},  'return_types': {},  'return_descs': {}}
            if key in functions:
                print("## ERROR:  File %s:  Line %d:  Duplicate function name '%s', defined more than once" % (filepath, line_num, key))
                return {'err': 503,	'functions': {},  'return_types': {},  'return_descs': {}}
            functions[key] = False
            #print pieces		#print("%d: %s" % (line_num, key))
            piece1 = ""
            piece2 = ""
            if len(pieces)>2:
                piece2 = pieces[2].lstrip().rstrip()
            if len(pieces)>1:
                piece1 = pieces[1].lstrip().rstrip()
            if numcols > 2:
                return_types[key] = piece1
                return_descs[key] = piece2
            else:
                return_descs[key] = piece1
    if args.verbose:
        print("## NOTE:  %d canonical functions parsed from %d lines in file '%s'" % (len(functions), line_num, filepath))
    #print "## DEBUG: FUNCTION RETURN VALUE DESCRIPTIONS:", return_descs
    return {'err': 0,	'functions': functions,		'return_types': return_types,	'return_descs': return_descs}


# Reads the given device file, and returns a dict containing:
#	'err':			Result code (0 == success)
#	'device_id':	NTV2DeviceID of the file.
#	'map':			Key/value pairs. If value is 'None', the key is an enum value; otherwise, key is GetNum function.
def parse_device_file (args, dev_path, dev_file):
    file_path = os.path.join(dev_path, dev_file)
    err = 0
    device_id = ""
    the_dict = {}
    definitions = {}
    line_num = 0
    if os.path.isdir(file_path):
        print("## ERROR:  device file '%s' is a folder" % (dev_file))
        err = 501
    else:
        device_id = "DEVICE_ID" + dev_file.upper()[3:-4]
        #print("## DEBUG:  parse_device_file:  '%s', %s" % (dev_file, device_id))
        with open(file_path) as f:
            for line in f:
                line_num = line_num + 1
                line = line.lstrip().rstrip()
                if len(line) == 0:
                    continue
                if line[0] == "#":
                    continue
                if len(line)>1 and line[0] == "/" and line[1] == "/":
                    continue
                pieces = line.split(None,2)
                key = pieces[0]
                value = None
                if key in definitions:
                    print("## ERROR:  %s[line %d]: '%s' previously defined in line %d" % (dev_file, line_num, key, definitions[key]))
                    err = 501
                    break
                definitions[key] = line_num
                if len(pieces) > 1:
                    value = pieces[1]
                the_dict[key] = value
    if err!=0:
        print("## ERROR:  parse_device_file failed for device file '%s', returning %d" % (dev_file, err))
    elif args.verbose:
        print("## NOTE:  %s has %d symbols defined from device file '%s'" % (device_id, len(the_dict), dev_file))
    return {'err': err, 'device_id': device_id, 'map': the_dict}


# Reads all device files, and returns a dict containing:
#	'err':		Result code (0 == success)
#	'devices':	Dict that maps NTV2DeviceIDs to the corresponding Key/value pairs parsed from their files.
def parse_device_files (args, dev_path):
    err = 0
    device_maps = {}
    file_num = 0
    for file in os.listdir(dev_path):
        if fnmatch.fnmatch(file, "dev_*.gen"):
            file_num = file_num + 1
            result = parse_device_file(args, dev_path, file)
            err        = result['err']
            device_id  = result['device_id']
            device_map = result['map']
            if err != 0:
                break
            device_maps[device_id] = device_map
    if err != 0:
        print("## ERROR:  'parse_device_files' failed after parsing %d file(s), returning %d" % (file_num, err))
    elif args.verbose:
        print("## NOTE:  %d device files successfully parsed" % (file_num))
    return {'err': err, 'devices': device_maps}


def write_can_do_function (args, f, device_ids, funcName, typeName, paramName, canonical_enums, Enum_to_devs_map, invalid_enum = ""):
            f.write("\n\n/**\n\t%s\n**/\n" % (funcName))
            f.write("bool %s (const NTV2DeviceID inDeviceID, const %s %s)\n" % (funcName, typeName, paramName))
            f.write("{\n")
            f.write("\tswitch (%s)\n" % (paramName))
            f.write("\t{\n")
            all_enums = canonical_enums
            for enum in all_enums:
                all_enums[enum] = False
            if invalid_enum:
                all_enums[invalid_enum] = False
            for enum in sorted(all_enums, key=str.lower):
                if enum in Enum_to_devs_map:
                    f.write("\t\tcase %s:\n" % (enum))
                    supported_devices = Enum_to_devs_map[enum]
                    if len(supported_devices) > 0:
                        all_enums[enum] = True
                        f.write("\t\t\tswitch (inDeviceID)\n")
                        f.write("\t\t\t{\n")
                        all_devices = device_ids
                        for device_name in all_devices:
                            all_devices[device_name] = False
                        for supported_device in supported_devices:
                            f.write("\t\t\t\tcase %s:\n" % (supported_device))
                            all_devices[supported_device] = True
                        f.write("\t\t\t\t\treturn true;\n")
                        f.write("\t\t\t#if defined(_DEBUG)\t\t// These devices don't support %s:\n" % (enum))
                        for device_name in all_devices:
                            if not all_devices[device_name]:
                                f.write("\t\t\t\tcase %s:\n" % (device_name))
                        f.write("\t\t\t#else\n")
                        f.write("\t\t\t\tdefault:\n")
                        f.write("\t\t\t#endif\n")
                        f.write("\t\t\t\t\tbreak;\n")
                        f.write("\t\t\t}\t//  switch on inDeviceID for %s\n" % (enum))
                        f.write("\t\t\tbreak;\t//  %s\n" % (enum))
                        f.write("\n")
            f.write("\t#if defined(_DEBUG)\t\t// These are unreferenced:\n")
            for enum in sorted(all_enums, key=str.lower):
                if not all_enums[enum]:
                    f.write("\t\tcase %s:\n" % (enum))
            f.write("\t#else\n")
            f.write("\t\tdefault:\n")
            f.write("\t#endif\n")
            f.write("\t\t\tbreak;\n")
            f.write("\t}\t//  switch on %s\n" % (paramName))
            f.write("\n")
            f.write("\treturn false;\n")
            f.write("\n")
            f.write("}\t//  %s (auto-generated)\n" % (funcName))


def write_can_do_function_by_device (args, f, device_ids, funcName, typeName, paramName, canonical_enums, Enum_to_devs_map, invalid_enum = ""):
            f.write("\n\n/**\n\t%s\n**/\n" % (funcName))
            f.write("bool %s (const NTV2DeviceID inDeviceID, const %s %s)\n" % (funcName, typeName, paramName))
            f.write("{\n")

            f.write("\tswitch (inDeviceID)\n")
            f.write("\t{\n")

            sorted_devices = sorted(device_ids, key=str.lower)
            all_devices = {}
            for device in sorted_devices:
                all_devices[device] = False

            for device in sorted_devices:
                if device == "DEVICE_ID_NOTFOUND":
                    continue  # Skip DEVICE_ID_NOTFOUND
                f.write("\t\tcase %s:\n" % (device))
                f.write("\t\t\tswitch (%s)\n" % (paramName))
                f.write("\t\t\t{\n")

                all_enums = canonical_enums
                for enum in all_enums:
                    all_enums[enum] = False
                if invalid_enum:
                    all_enums[invalid_enum] = False
                for enum in sorted(all_enums, key=str.lower):
                    if enum in Enum_to_devs_map:
                        devs_can_do = Enum_to_devs_map[enum]
                        if device in devs_can_do:
                            all_enums[enum] = True;
                            f.write("\t\t\t\tcase %s:\n" % (enum))
                f.write("\t\t\t\t\treturn true;\n")
                f.write("\t\t\t#if defined(_DEBUG)\t\t// %ss not supported by %s:\n" % (typeName, device))
                for enum in sorted(all_enums, key=str.lower):
                    if not all_enums[enum]:
                        f.write("\t\t\t\tcase %s:\n" % (enum))
                f.write("\t\t\t#else\n")
                f.write("\t\t\t\tdefault:\n")
                f.write("\t\t\t#endif\n")
                f.write("\t\t\t\t\tbreak;\n")
                f.write("\t\t\t}\t//\tswitch on %s\n" % (paramName))
                f.write("\t\t\tbreak;\t//\tcase %s\n" % (device))
                f.write("\n")

            f.write("\t\tcase DEVICE_ID_NOTFOUND:\n")
            f.write("\t\t\tbreak;\t//\tcase DEVICE_ID_NOTFOUND\n")
            f.write("\t}\t//  switch on device ID\n")
            f.write("\n")
            f.write("\treturn false;\n")
            f.write("\n")
            f.write("}\t//  %s (auto-generated)\n" % (funcName))


def main ():
    err = 0
    devicesDir = ""
    typesDir = ""
    functionsDir = ""
    sdkgenDir = ""
    ohhDir = ""
    ohhFile = ""
    ohppDir = ""
    ohppFile = ""

    args = parse_args ()
    if args.ajantv2:
        ajantv2Dir = os.path.join(args.ajantv2)
        if not os.path.exists(ajantv2Dir) or not os.path.isdir(ajantv2Dir):
            print("## ERROR:  'ajantv2' folder '%s' not found or not a folder" % (args.ajantv2))
            return 404
    else:
        ajantv2Dir = os.getcwd()
    if args.verbose:
        print("## NOTE:  Using 'ajantv2' folder '%s'" % (ajantv2Dir))

    sdkgenDir = os.path.join(ajantv2Dir, "sdkgen")
    if not os.path.exists(sdkgenDir) or not os.path.isdir(sdkgenDir):
        print("## ERROR:  'sdkgen' folder '%s' not found or not a folder" % (sdkgenDir))
        return 404
    if args.verbose:
        print("## NOTE:  Using 'sdkgen' folder '%s'" % (sdkgenDir))

    devicesDir = os.path.join(sdkgenDir, "devices")
    if not os.path.exists(devicesDir) or not os.path.isdir(devicesDir):
        print("## ERROR:  'devices' folder '%s' not found or not a folder" % (devicesDir))
        return 404
    if args.verbose:
        print("## NOTE:  Using 'devices' folder '%s'" % (devicesDir))

    if args.ohh:
        ohhDir = os.path.join(args.ohh)
        if not os.path.exists(ohhDir) or not os.path.isdir(ohhDir):
            print("## ERROR:  '.hh' header folder '%s' not found or not a folder" % (args.ohh))
            return 404
    if ohhDir:
        if args.verbose:
            print("## NOTE:  Will generate .hh file in folder '%s'" % (ohhDir))
        ohhFile = os.path.join(ohhDir, "ntv2devicefeatures.hh")
        if os.path.exists(ohhFile) and os.path.isdir(ohhFile):
            print("## ERROR:  '%s' is a folder and cannot be replaced" % (ohhFile))
            return 404
        if os.path.exists(ohhFile):
            print("## WARNING:  '%s' header file already exists and will be replaced" % (ohhFile))
            if args.failwarnings:
                return 603

    if args.ohpp:
        ohppDir = os.path.join(args.ohpp)
        if not os.path.exists(ohppDir) or not os.path.isdir(ohppDir):
            print("## ERROR:  '.hpp' source folder '%s' not found or not a folder" % (args.ohpp))
            return 404
    if ohppDir:
        if args.verbose:
            print("## NOTE:  Will generate .hpp file in folder '%s'" % (ohppDir))
        ohppFile = os.path.join(ohppDir, "ntv2devicefeatures.hpp")
        if os.path.exists(ohppFile) and os.path.isdir(ohppFile):
            print("## ERROR:  '%s' is a folder and cannot be replaced" % (ohppFile))
            return 404
        if os.path.exists(ohppFile):
            print("## WARNING:  '%s' source file already exists and will be replaced" % (ohppFile))
            if args.failwarnings:
                return 603

    result = get_canonical_device_ids (args, os.path.join(ajantv2Dir, "includes", "ntv2enums.h"))
    err        = result['err']
    device_ids = result['device_ids']
    if err != 0:
        return err
    device_ids["DEVICE_ID_NOTFOUND"] = False

    result = get_canonical_video_formats (args, os.path.join(ajantv2Dir, "includes", "ntv2enums.h"))
    err           = result['err']
    video_formats = result['videoformats']
    if err != 0:
        return err

    result = get_canonical_pixel_formats (args, os.path.join(ajantv2Dir, "includes", "ntv2enums.h"))
    err        = result['err']
    pixel_formats = result['pixelformats']
    if err != 0:
        return err

    result = get_canonical_input_sources (args, os.path.join(ajantv2Dir, "includes", "ntv2enums.h"))
    err        = result['err']
    input_sources = result['inputsources']
    if err != 0:
        return err

    result = get_canonical_widget_ids (args, os.path.join(ajantv2Dir, "includes", "ntv2enums.h"))
    err        = result['err']
    widget_ids = result['widgetids']
    if err != 0:
        return err

    result = get_canonical_dsk_modes (args, os.path.join(ajantv2Dir, "includes", "ntv2enums.h"))
    err        = result['err']
    dsk_modes = result['dskmodes']
    if err != 0:
        return err

    result = get_canonical_conversion_modes (args, os.path.join(ajantv2Dir, "includes", "ntv2enums.h"))
    err              = result['err']
    conversion_modes = result['convmodes']
    if err != 0:
        return err

    result = get_canonical_functions (args, os.path.join(sdkgenDir, "cando_canon.gen"))
    err              = result['err']
    can_do_functions = result['functions']
    can_do_descs     = result['return_descs']
    if err != 0:
        return err

    result = get_canonical_functions (args, os.path.join(sdkgenDir, "getnum_canon.gen"), 3)
    err               = result['err']
    get_num_functions = result['functions']
    get_num_types     = result['return_types']
    get_num_descs     = result['return_descs']
    if err != 0:
        return err

    result = parse_device_files(args, devicesDir)
    err         = result['err']
    device_maps = result['devices']
    if err != 0:
        print("## NOTE:  'sdkgen' failed, error %d" % (err))

    # Check that each canonical device is represented in device_maps (other than DEVICE_ID_NOTFOUND)...
    for device_id in device_ids:
        if device_id != "DEVICE_ID_NOTFOUND":
            if not device_id in device_maps:
                print("## ERROR:  Canonical device %s not represented in device files" % device_id)
                return 611

    DSKModes = {}
    ConversionModes = {}
    FrameBufferFormats = {}
    InputSources = {}
    VideoFormats = {}
    WidgetIDs = {}
    CanDos = {}
    GetNums = {}

    # Process each device...
    for device_id in device_maps:
        # Check that the device is canonical...
        if not device_id in device_ids:
            print("## ERROR:  Device ID %s parsed from device file not in canonical list of NTV2DeviceIDs" % device_id)
            return 610
        
        symbols = device_maps[device_id]
        
        # Process each symbol...
        for symbol in symbols:
            if symbol.startswith("NTV2_DSKMode"):
                if symbol in dsk_modes:
                    dsk_modes[symbol] = True
                    if symbol in DSKModes:
                        devices = DSKModes[symbol]
                        devices.append(device_id)
                        DSKModes[symbol] = devices
                    else:
                        DSKModes[symbol] = [device_id]
                else:
                    print("## ERROR:  Downstream keyer mode %s parsed from device file %s not in canonical list of NTV2DSKModes" % (symbol, device_id))
                    return 605
            elif symbol.startswith("NTV2_FBF_"):
                if symbol in pixel_formats:
                    pixel_formats[symbol] = True
                    if symbol in FrameBufferFormats:
                        devices = FrameBufferFormats[symbol]
                        devices.append(device_id)
                        FrameBufferFormats[symbol] = devices
                    else:
                        FrameBufferFormats[symbol] = [device_id]
                else:
                    print("## ERROR:  Frame buffer format %s parsed from device file %s not in canonical list of NTV2FrameBufferFormats" % (symbol, device_id))
                    return 605
            elif symbol.startswith("NTV2DeviceGet"):
                if symbol in get_num_functions:
                    get_num_functions[symbol] = True
                    value = symbols[symbol]
                    if symbol in GetNums:
                        devices = GetNums[symbol]
                        devices[device_id] = value
                        GetNums[symbol] = devices
                    else:
                        GetNums[symbol] = {device_id: value}
                else:
                    print("## ERROR:  %s parsed from device file %s not in canonical list of 'GetNum' functions" % (symbol, device_id))
                    return 605
            elif symbol.startswith("NTV2_INPUTSOURCE_"):
                if symbol in input_sources:
                    input_sources[symbol] = True
                    if symbol in InputSources:
                        devices = InputSources[symbol]
                        devices.append(device_id)
                        InputSources[symbol] = devices
                    else:
                        InputSources[symbol] = [device_id]
                else:
                    print("## ERROR:  Input source %s parsed from device file %s not in canonical list of NTV2InputSources" % (symbol, device_id))
                    return 605
            elif symbol.startswith("NTV2_FORMAT_"):
                if symbol in video_formats:
                    video_formats[symbol] = True
                    if symbol in VideoFormats:
                        devices = VideoFormats[symbol]
                        devices.append(device_id)
                        VideoFormats[symbol] = devices
                    else:
                        VideoFormats[symbol] = [device_id]
                else:
                    print("## ERROR:  Video format %s parsed from device file %s not in canonical list of NTV2VideoFormats" % (symbol, device_id))
                    return 605
            elif symbol.startswith("NTV2_Wgt"):
                if symbol in widget_ids:
                    widget_ids[symbol] = True
                    if symbol in WidgetIDs:
                        devices = WidgetIDs[symbol]
                        devices.append(device_id)
                        WidgetIDs[symbol] = devices
                    else:
                        WidgetIDs[symbol] = [device_id]
                else:
                    print("## ERROR:  Widget ID %s parsed from device file %s not in canonical list of NTV2WidgetIDs" % (symbol, device_id))
                    return 605
            elif symbol.startswith("NTV2_108") or symbol.startswith("NTV2_525") or symbol.startswith("NTV2_625") or symbol.startswith("NTV2_720"):
                if symbol in conversion_modes:
                    conversion_modes[symbol] = True
                    if symbol in ConversionModes:
                        devices = ConversionModes[symbol]
                        devices.append(device_id)
                        ConversionModes[symbol] = devices
                    else:
                        ConversionModes[symbol] = [device_id]
                else:
                    print("## ERROR:  Conversion mode %s parsed from device file %s not in canonical list of NTV2ConversionModes" % (symbol, device_id))
                    return 605
            elif symbol.startswith("NTV2DeviceCan") or symbol.startswith("NTV2DeviceHas") or symbol.startswith("NTV2DeviceIs") or symbol.startswith("NTV2DeviceNeed") or symbol.startswith("NTV2DeviceSof"):
                if symbol in can_do_functions:
                    can_do_functions[symbol] = True
                    if symbol in CanDos:
                        devices = CanDos[symbol]
                        devices.append(device_id)
                        CanDos[symbol] = devices
                    else:
                        CanDos[symbol] = [device_id]
                else:
                    print("## ERROR:  %s parsed from device file %s not in canonical list of 'CanDo' functions" % (symbol, device_id))
                    return 605
            else:
                print("## WARNING:  Symbol %s parsed from device file %s not recognized as valid" % (symbol, device_id))
                if args.failwarnings:
                    return 602

    # Optional:  Report unused/unreferenced canonical symbols:
    if args.unused:
        unused = []
        for vf in video_formats:
            if not video_formats[vf]:
                unused.append(vf)
        if len(unused)>0:
            print("## WARNING:  %d unreferenced NTV2VideoFormat enums:  %s" % (len(unused), ", ".join(unused)))
            if args.failwarnings:
                return 602

        unused = []
        for fbf in pixel_formats:
            if not pixel_formats[fbf]:
                unused.append(fbf)
        if len(unused)>0:
            print("## WARNING:  %d unreferenced NTV2FrameBufferFormat enums:  %s" % (len(unused), ", ".join(unused)))
            if args.failwarnings:
                return 602

        unused = []
        for inp in input_sources:
            if not input_sources[inp]:
                unused.append(inp)
        if len(unused)>0:
            print("## WARNING:  %d unreferenced NTV2InputSource enums:  %s" % (len(unused), ", ".join(unused)))
            if args.failwarnings:
                return 602

        unused = []
        for wid in widget_ids:
            if not widget_ids[wid]:
                unused.append(wid)
        if len(unused)>0:
            print("## WARNING:  %d unreferenced NTV2WidgetID enums:  %s" % (len(unused), ", ".join(unused)))
            if args.failwarnings:
                return 602

        unused = []
        for dskm in dsk_modes:
            if not dsk_modes[dskm]:
                unused.append(dskm)
        if len(unused)>0:
            print("## WARNING:  %d unreferenced NTV2DSKMode enums:  %s" % (len(unused), ", ".join(unused)))
            if args.failwarnings:
                return 602

        unused = []
        for cm in conversion_modes:
            if not conversion_modes[cm]:
                unused.append(cm)
        if len(unused)>0:
            print("## WARNING:  %d unreferenced NTV2ConversionMode enums:  %s" % (len(unused), ", ".join(unused)))
            if args.failwarnings:
                return 602

        unused = []
        for func in get_num_functions:
            if not get_num_functions[func]:
                unused.append(func)
        if len(unused)>0:
            print("## WARNING:  %d unreferenced 'GetNum' functions:  %s" % (len(unused), ", ".join(unused)))
            if args.failwarnings:
                return 602

        unused = []
        for func in can_do_functions:
            if not can_do_functions[func]:
                unused.append(func)
        if len(unused)>0:
            print("## WARNING:  %d unreferenced 'CanDo' functions:  %s" % (len(unused), ", ".join(unused)))
            if args.failwarnings:
                return 602
    if False:
        print("\nVIDEO FORMATS:\n", VideoFormats)
        print("\nFRAME BUFFER FORMATS:\n", FrameBufferFormats)
        print("\nWIDGETS:\n", WidgetIDs)
        print("\nINPUT SOURCES:\n", InputSources)
        print("\nDSK MODES:\n", DSKModes)
        print("\nCONVERSION MODES:\n", ConversionModes)

    # Keep an alphabetically sorted list of device IDs:
    device_ids_sorted = list(device_ids.keys())
    device_ids_sorted.sort()

    # Optional:  Write .hh file:
    if ohhFile:
        result = 0
        with open (ohhFile, 'w') as f:
            f.write("/**\n\t@file\t\tntv2devicefeatures.hh\n\t@brief\t\tDeclares NTV2DeviceCanDo... and NTV2DeviceGetNum... functions.\n")
            f.write("\t\t\t\tThis module is included at compile time from 'ntv2devicefeatures.h'.\n\t@copyright\t(C) 2004-%s AJA Video Systems, Inc.\tProprietary and confidential information.\n" % (datetime.datetime.now().strftime("%Y")))
            f.write("\t@note\t\tGenerated by '%s' on %s.\n**/\n" % (sys.argv[0], datetime.datetime.now().strftime("%c")))
            f.write("#ifndef NTV2DEVICEFEATURES_HH\n")
            f.write("#define NTV2DEVICEFEATURES_HH\n\n")
            f.write("#if defined(__cplusplus) && defined(NTV2_BUILDING_DRIVER)\n")
            f.write("extern \"C\"\n")
            f.write("{\n")
            f.write("#endif\n\n")
            # CanDo
            for func in sorted(can_do_functions, key=str.lower):
                desc = ""
                if func in can_do_descs:
                    desc = can_do_descs[func]
                f.write("\n/**\n")
                if desc:
                    f.write("\t@return\t\t%s\n" % (desc))
                f.write("\t@param[in]\tinDeviceID		Specifies the NTV2DeviceID of interest.\n")
                f.write("**/\n")
                f.write("AJAExport bool %s (const NTV2DeviceID inDeviceID);\n" % (func))
            # GetNum
            for func in sorted(get_num_functions, key=str.lower):
                desc = ""
                if func in get_num_descs:
                    desc = get_num_descs[func]
                type = ""
                if func in get_num_types:
                    type = get_num_types[func]
                f.write ("\n/**\n")
                if desc:
                    f.write("\t@return\t\t%s\n" % (desc))
                f.write("\t@param[in]\tinDeviceID		Specifies the NTV2DeviceID of interest.\n")
                f.write("**/\n")
                f.write("AJAExport %s %s (const NTV2DeviceID inDeviceID);\n" % (type, func))
            # ConversionMode
            f.write("\n/**\n\t@return\t\tTrue if the device having the given ID supports the given NTV2ConversionMode.\n")
            f.write("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write("\t@param[in]\tinConversionMode\tSpecifies the NTV2ConversionMode.\n")
            f.write("**/\n")
            f.write("AJAExport bool NTV2DeviceCanDoConversionMode (const NTV2DeviceID inDeviceID, const NTV2ConversionMode inConversionMode);\n")
            # DSKMode
            f.write("\n/**\n\t@return\t\tTrue if the device having the given ID supports the given NTV2DSKMode.\n")
            f.write("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write("\t@param[in]\tinDSKMode\t\tSpecifies the NTV2DSKMode.\n")
            f.write("**/\n")
            f.write("AJAExport bool NTV2DeviceCanDoDSKMode (const NTV2DeviceID inDeviceID, const NTV2DSKMode inDSKMode);\n")
            # FBFormat
            f.write("\n/**\n\t@return\t\tTrue if the device having the given ID supports the given NTV2FrameBufferFormat.\n")
            f.write("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write("\t@param[in]\tinFBFormat\t\tSpecifies the NTV2FrameBufferFormat.\n")
            f.write("**/\n")
            f.write("AJAExport bool NTV2DeviceCanDoFrameBufferFormat (const NTV2DeviceID inDeviceID, const NTV2FrameBufferFormat inFBFormat);\n")
            # InputSource
            f.write("\n/**\n\t@return\t\tTrue if the device having the given ID supports the given NTV2InputSource.\n")
            f.write("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write("\t@param[in]\tinInputSource\tSpecifies the NTV2InputSource.\n")
            f.write("**/\n")
            f.write("AJAExport bool NTV2DeviceCanDoInputSource (const NTV2DeviceID inDeviceID, const NTV2InputSource inInputSource);\n")
            # VideoFormat
            f.write("\n/**\n\t@return\t\tTrue if the device having the given ID supports the given NTV2VideoFormat.\n")
            f.write("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write("\t@param[in]\tinVideoFormat\tSpecifies the NTV2VideoFormat.\n")
            f.write("**/\n")
            f.write("AJAExport bool NTV2DeviceCanDoVideoFormat (const NTV2DeviceID inDeviceID, const NTV2VideoFormat inVideoFormat);\n")
            # Widget
            f.write("\n/**\n\t@return\t\tTrue if the device having the given ID supports the given NTV2WidgetID.\n")
            f.write("\t@param[in]\tinDeviceID\t\tSpecifies the NTV2DeviceID of interest.\n")
            f.write("\t@param[in]\tinWidgetID\t\tSpecifies the NTV2WidgetID.\n")
            f.write("**/\n")
            f.write("AJAExport bool NTV2DeviceCanDoWidget (const NTV2DeviceID inDeviceID, const NTV2WidgetID inWidgetID);\n")
            f.write("#define NTV2DEVICEFEATURES_HH\n\n")
            f.write("#if defined(__cplusplus) && defined(NTV2_BUILDING_DRIVER)\n")
            f.write("}\n")
            f.write("#endif\n\n")
            f.write("\n\n#endif\t//\tNTV2DEVICEFEATURES_HH\n")
        if args.verbose:
            print("## NOTE:  '%s' written successfully" % (ohhFile))
        if result != 0:
            return result

    if ohppFile:
        result = 0
        with open (ohppFile, 'w') as f:
            f.write("/**\n\t@file\t\tntv2devicefeatures.hpp\n\t@brief\t\tContains implementations of NTV2DeviceCanDo... and NTV2DeviceGetNum... functions.\n")
            f.write("\t\t\t\tThis module is included at compile time from 'ntv2devicefeatures.cpp'.\n\t@copyright\t(C) 2004-%s AJA Video Systems, Inc.\tProprietary and confidential information.\n" % (datetime.datetime.now().strftime("%Y")))
            f.write("\t@note\t\tGenerated by '%s' on %s.\n**/\n" % (sys.argv[0], datetime.datetime.now().strftime("%c")))

            # CanDo
            for func in sorted(can_do_functions, key=str.lower):
                desc = ""
                if func in can_do_descs:
                    desc = can_do_descs[func]
                f.write("\n\n/**\n\t%s\n" % (func))
                if desc:
                    f.write("\t%s\n" % (desc))
                f.write("**/\n")
                f.write("bool %s (const NTV2DeviceID inDeviceID)\n" % (func))
                f.write("{\n")
                if func in CanDos:
                    devices_that_support_this_feature = CanDos[func]
                    if len(devices_that_support_this_feature) > 0:
                        f.write("\tswitch (inDeviceID)\n")
                        f.write("\t{\n")
                        all_devices = device_ids
                        for device_name in all_devices:
                            all_devices[device_name] = False
                        for device_name in device_ids_sorted:
                            if device_name in devices_that_support_this_feature:
                                f.write("\t\tcase %s:\n" % (device_name))
                                all_devices[device_name] = True
                        f.write("\t\t\treturn true;\n")
                        f.write("\t#if defined(_DEBUG)\n")
                        for device_name in device_ids_sorted:
                            if not all_devices[device_name]:
                                f.write("\t\tcase %s:\n" % (device_name))
                        f.write("\t#else\n")
                        f.write("\t\tdefault:\n")
                        f.write("\t#endif\n")
                        f.write("\t\t\tbreak;\n")
                        f.write("\t}\t//\tswitch on inDeviceID\n")
                    else:
                        f.write("\t(void) inDeviceID;\n")
                else:
                    f.write("\t(void) inDeviceID;\n")
                f.write("\n")
                f.write("\treturn false;\n")
                f.write("\n")
                f.write("}\t//  %s (auto-generated)\n" % (func))

            # GetNum
            for func in sorted(get_num_functions, key=str.lower):
                desc = ""
                if func in get_num_descs:
                    desc = get_num_descs[func]
                type = "void"
                if func in get_num_types:
                    type = get_num_types[func]
                f.write("\n\n/**\n\t%s\n" % (func))
                if desc:
                    f.write("\t%s\n" % (desc))
                f.write("**/\n")
                f.write("%s %s (const NTV2DeviceID inDeviceID)\n" % (type, func))
                f.write("{\n")
                device_to_value_map = {}
                if func in GetNums:
                    device_to_value_map = GetNums[func]
                if len(device_to_value_map) is 0:
                    f.write("\t(void) inDeviceID;\t\t// No devices support this function\n")
                else:
                    f.write("\tswitch (inDeviceID)\n")
                    f.write("\t{\n")
                    all_devices = device_ids
                    for device_name in all_devices:
                        all_devices[device_name] = False
                    for device_name in device_ids_sorted:
                        if device_name in device_to_value_map:
                            if device_name:
                                all_devices[device_name] = True
                                value = device_to_value_map[device_name]
                                if len(device_name) < 14:
                                    tabs = "\t\t\t"
                                elif len(device_name) < 18:
                                    tabs = "\t\t"
                                else:
                                    tabs = "\t"
                                f.write("\t\tcase %s:%sreturn %s;\n" % (device_name, tabs, value))
                    f.write("\t#if defined(_DEBUG)\t\t// These devices all return zero:\n")
                    for device_name in device_ids_sorted:
                        if not all_devices[device_name]:
                            f.write("\t\tcase %s:\n" % (device_name))
                    f.write("\t#else\n")
                    f.write("\t\tdefault:\n")
                    f.write("\t#endif\t//\tdefined(_DEBUG)\n")
                    f.write("\t\t\tbreak;\n")
                    f.write("\t}\t//\tswitch on inDeviceID\n")
                    f.write("\n")
                f.write("\treturn 0;\n")
                f.write("\n")
                f.write("}\t//  %s (auto-generated)\n" % (func))

            write_can_do_function           (args, f, device_ids, "NTV2DeviceCanDoConversionMode",    "NTV2ConversionMode",    "inConversionMode", conversion_modes, ConversionModes,    "NTV2_CONVERSIONMODE_INVALID")
            write_can_do_function           (args, f, device_ids, "NTV2DeviceCanDoDSKMode",           "NTV2DSKMode",           "inDSKMode",        dsk_modes,        DSKModes,           "NTV2_DSKMODE_INVALID")
            write_can_do_function           (args, f, device_ids, "NTV2DeviceCanDoFrameBufferFormat", "NTV2FrameBufferFormat", "inFBFormat",       pixel_formats,    FrameBufferFormats, "NTV2_FBF_INVALID")
            write_can_do_function           (args, f, device_ids, "NTV2DeviceCanDoInputSource",       "NTV2InputSource",       "inInputSource",    input_sources,    InputSources,       "NTV2_INPUTSOURCE_INVALID")
            write_can_do_function_by_device (args, f, device_ids, "NTV2DeviceCanDoVideoFormat",       "NTV2VideoFormat",       "inVideoFormat",    video_formats,    VideoFormats,       "NTV2_FORMAT_UNKNOWN")
            write_can_do_function_by_device (args, f, device_ids, "NTV2DeviceCanDoWidget",            "NTV2WidgetID",          "inWidgetID",       widget_ids,       WidgetIDs,          "NTV2_WIDGET_INVALID")
        if args.verbose:
            print("## NOTE:  '%s' written successfully" % (ohppFile))
        if result != 0:
            return result

    if args.verbose:
        print("## NOTE:  'sdkgen' completed with no errors")
    return 0


if __name__ == '__main__':
    status = main ()
    sys.exit (status)
