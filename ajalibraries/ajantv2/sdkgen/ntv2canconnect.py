#!/usr/bin/env python

"""
	SUMMARY:
		Parses verilog to generate a "CanConnect" function.

	SYNTAX:
		ntv2canconnect.py   [--verbose|-v]   [--output|-o  outputFolder]  [--input|-i  parentPath]

	REQUIRED PARAMETERS:
		none

	OPTIONAL PARAMETERS:
		[--output  outputFolder]	Optionally specifies folder path into which .hpp & .hh files are written.
									If not specified, .hpp & .hh files are written into current directory.

		[--input  parentPath]		Optionally specifies folder path to look for device folders that contain
									verilog code. If not specified, looks in current directory.

		[--device  deviceName]		Optionally specifies a device name to restrict processing to a single device.
									If not specified, processes all known devices.

		[--verbose]					Optionally emits progress information to stdout.

	EXAMPLE:
		ntv2projects/sdkgen/ntv2canconnect.py  --input ~/dev  --verbose  --device io4k
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
    parser.add_argument ('-i', '--input',			help = """path to folder containing device folders containing verilog""")
    parser.add_argument ('-o', '--output',			help = """path to folder into which .hpp & .hh files are written""")
    parser.add_argument ('-d', '--device',			help = """device (all devices if omitted)""")
    parser.add_argument ('-v', '--verbose',			help = """Use verbose output.""",									action = 'store_true')
    parser.add_argument ('-f', '--failwarnings',	help = """Treat warnings as errors.""",								action = 'store_true')
    return parser.parse_args ()


def VerilogToNTV2OutXpt (inVerilogOutputXptName):
    return ""


def VerilogToNTV2InXpt (inVerilogInputSelectName):
    inputXptMap	=	{	"avid_in_sel":						"NTV2_XptAnalogOutInput",
    					"out1a_in_sel":						"NTV2_XptSDIOut1Input",
    					"out1b_in_sel":						"NTV2_XptSDIOut1InputDS2",
    					"out2a_in_sel":						"NTV2_XptSDIOut2Input",
    					"out2b_in_sel":						"NTV2_XptSDIOut2InputDS2",
    					"out3a_in_sel":						"NTV2_XptSDIOut3Input",
    					"out3b_in_sel":						"NTV2_XptSDIOut3InputDS2",
    					"out4a_in_sel":						"NTV2_XptSDIOut4Input",
    					"out4b_in_sel":						"NTV2_XptSDIOut4InputDS2",
    					"out5a_in_sel":						"NTV2_XptSDIOut5Input",
    					"out5b_in_sel":						"NTV2_XptSDIOut5InputDS2",
    					"out6a_in_sel":						"NTV2_XptSDIOut6Input",
    					"out6b_in_sel":						"NTV2_XptSDIOut6InputDS2",
    					"out7a_in_sel":						"NTV2_XptSDIOut7Input",
    					"out7b_in_sel":						"NTV2_XptSDIOut7InputDS2",
    					"out8a_in_sel":						"NTV2_XptSDIOut8Input",
    					"out8b_in_sel":						"NTV2_XptSDIOut8InputDS2",

    					"fb1_in_sel":						"NTV2_XptFrameBuffer1Input",
    					"fb1b_in_sel":						"NTV2_XptFrameBuffer1BInput",
    					"fb2_in_sel":						"NTV2_XptFrameBuffer2Input",
    					"fb2b_in_sel":						"NTV2_XptFrameBuffer2BInput",
    					"fb3_in_sel":						"NTV2_XptFrameBuffer3Input",
    					"fb3b_in_sel":						"NTV2_XptFrameBuffer3BInput",
    					"fb4_in_sel":						"NTV2_XptFrameBuffer4Input",
    					"fb4b_in_sel":						"NTV2_XptFrameBuffer4BInput",
    					"fb5_in_sel":						"NTV2_XptFrameBuffer5Input",
    					"fb5b_in_sel":						"NTV2_XptFrameBuffer5BInput",
    					"fb6_in_sel":						"NTV2_XptFrameBuffer6Input",
    					"fb6b_in_sel":						"NTV2_XptFrameBuffer6BInput",
    					"fb7_in_sel":						"NTV2_XptFrameBuffer7Input",
    					"fb7b_in_sel":						"NTV2_XptFrameBuffer7BInput",
    					"fb8_in_sel":						"NTV2_XptFrameBuffer8Input",
    					"fb8b_in_sel":						"NTV2_XptFrameBuffer8BInput",

    					"csc1_vin_sel":						"NTV2_XptCSC1VidInput",
    					"csc1_kin_sel":						"NTV2_XptCSC1KeyInput",
    					"csc2_vin_sel":						"NTV2_XptCSC2VidInput",
    					"csc2_kin_sel":						"NTV2_XptCSC2KeyInput",
    					"csc3_vin_sel":						"NTV2_XptCSC3VidInput",
    					"csc3_kin_sel":						"NTV2_XptCSC3KeyInput",
    					"csc4_vin_sel":						"NTV2_XptCSC4VidInput",
    					"csc4_kin_sel":						"NTV2_XptCSC4KeyInput",
    					"csc5_vin_sel":						"NTV2_XptCSC5VidInput",
    					"csc5_kin_sel":						"NTV2_XptCSC5KeyInput",
    					"csc6_vin_sel":						"NTV2_XptCSC6VidInput",
    					"csc6_kin_sel":						"NTV2_XptCSC6KeyInput",
    					"csc7_vin_sel":						"NTV2_XptCSC7VidInput",
    					"csc7_kin_sel":						"NTV2_XptCSC7KeyInput",
    					"csc8_vin_sel":						"NTV2_XptCSC8VidInput",
    					"csc8_kin_sel":						"NTV2_XptCSC8KeyInput",

    					"lut1_in_sel":						"NTV2_XptLUT1Input",
    					"lut2_in_sel":						"NTV2_XptLUT2Input",
    					"lut3_in_sel":						"NTV2_XptLUT3Input",
    					"lut4_in_sel":						"NTV2_XptLUT4Input",
    					"lut5_in_sel":						"NTV2_XptLUT5Input",
    					"lut6_in_sel":						"NTV2_XptLUT61Input",
    					"lut7_in_sel":						"NTV2_XptLUT7Input",
    					"lut8_in_sel":						"NTV2_XptLUT8Input",

    					#"conv_in_sel":						"NTV2_Xpt???Input",

    					#"conv2a_in_sel":					"NTV2_Xpt4KDCQ1Input",
    					#"conv2b_in_sel":					"NTV2_Xpt4KDCQ2Input",
    					#"conv2c_in_sel":					"NTV2_Xpt4KDCQ3Input",
    					#"conv2d_in_sel":					"NTV2_Xpt4KDCQ4Input",

    					"dlo1_in_sel":						"NTV2_XptDualLinkOut1Input",
    					"dlo2_in_sel":						"NTV2_XptDualLinkOut2Input",
    					"dlo3_in_sel":						"NTV2_XptDualLinkOut3Input",
    					"dlo4_in_sel":						"NTV2_XptDualLinkOut4Input",
    					"dlo5_in_sel":						"NTV2_XptDualLinkOut5Input",
    					"dlo6_in_sel":						"NTV2_XptDualLinkOut6Input",
    					"dlo7_in_sel":						"NTV2_XptDualLinkOut7Input",
    					"dlo8_in_sel":						"NTV2_XptDualLinkOut8Input",

    					#"comp_in_sel":						"NTV2_Xpt???",

    					"mix1_fgv_sel":						"NTV2_XptMixer1FGVidInput",
    					"mix1_fgk_sel":						"NTV2_XptMixer1FGKeyInput",
    					"mix1_bgv_sel":						"NTV2_XptMixer1BGVidInput",
    					"mix1_bgk_sel":						"NTV2_XptMixer1BGKeyInput",
    					"mix2_fgv_sel":						"NTV2_XptMixer2FGVidInput",
    					"mix2_fgk_sel":						"NTV2_XptMixer2FGKeyInput",
    					"mix2_bgv_sel":						"NTV2_XptMixer2BGVidInput",
    					"mix2_bgk_sel":						"NTV2_XptMixer2BGKeyInput",
    					"mix3_fgv_sel":						"NTV2_XptMixer3FGVidInput",
    					"mix3_fgk_sel":						"NTV2_XptMixer3FGKeyInput",
    					"mix3_bgv_sel":						"NTV2_XptMixer3BGVidInput",
    					"mix3_bgk_sel":						"NTV2_XptMixer3BGKeyInput",
    					"mix4_fgv_sel":						"NTV2_XptMixer4FGVidInput",
    					"mix4_fgk_sel":						"NTV2_XptMixer4FGKeyInput",
    					"mix4_bgv_sel":						"NTV2_XptMixer4BGVidInput",
    					"mix4_bgk_sel":						"NTV2_XptMixer4BGKeyInput",
    					
    					#"dual_link_receiver_stream1_sel":	"NTV2_XptDualLinkIn1Input",
    					#"dual_link_receiver_stream2_sel":	"NTV2_XptDualLinkIn2Input",
    					#"dual_link_receiver_stream3_sel":	"NTV2_XptDualLinkIn3Input",
    					#"dual_link_receiver_stream4_sel":	"NTV2_XptDualLinkIn4Input",
    					#"dual_link_receiver_stream5_sel":	"NTV2_XptDualLinkIn5Input",
    					#"dual_link_receiver_stream6_sel":	"NTV2_XptDualLinkIn6Input",
    					#"dual_link_receiver_stream7_sel":	"NTV2_XptDualLinkIn7Input",
    					#"dual_link_receiver_stream8_sel":	"NTV2_XptDualLinkIn8Input",

    					"dual_link_receiver_1_stream_sel1":	"NTV2_XptDualLinkIn1Input",
    					"dual_link_receiver_1_stream_sel2":	"NTV2_XptDualLinkIn1DSInput",
    					"dual_link_receiver_2_stream_sel1":	"NTV2_XptDualLinkIn2Input",
    					"dual_link_receiver_2_stream_sel2":	"NTV2_XptDualLinkIn2DSInput",
    					"dual_link_receiver_3_stream_sel1":	"NTV2_XptDualLinkIn3Input",
    					"dual_link_receiver_3_stream_sel2":	"NTV2_XptDualLinkIn3DSInput",
    					"dual_link_receiver_4_stream_sel1":	"NTV2_XptDualLinkIn4Input",
    					"dual_link_receiver_4_stream_sel2":	"NTV2_XptDualLinkIn4DSInput",
    					"dual_link_receiver_5_stream_sel1":	"NTV2_XptDualLinkIn5Input",
    					"dual_link_receiver_5_stream_sel2":	"NTV2_XptDualLinkIn5DSInput",
    					"dual_link_receiver_6_stream_sel1":	"NTV2_XptDualLinkIn6Input",
    					"dual_link_receiver_6_stream_sel2":	"NTV2_XptDualLinkIn6DSInput",
    					"dual_link_receiver_7_stream_sel1":	"NTV2_XptDualLinkIn7Input",
    					"dual_link_receiver_7_stream_sel2":	"NTV2_XptDualLinkIn7DSInput",
    					"dual_link_receiver_8_stream_sel1":	"NTV2_XptDualLinkIn8Input",
    					"dual_link_receiver_8_stream_sel2":	"NTV2_XptDualLinkIn8DSInput",

    					#"qrc_in1_sel":						"NTV2_Xpt???",
    					#"qrc_in2_sel":						"NTV2_Xpt???",
    					#"qrc_in3_sel":						"NTV2_Xpt???",
    					#"qrc_in4_sel":						"NTV2_Xpt???",

    					"mux1a_in_sel":						"NTV2_Xpt425Mux1AInput",
    					"mux1b_in_sel":						"NTV2_Xpt425Mux1BInput",
    					"mux2a_in_sel":						"NTV2_Xpt425Mux2AInput",
    					"mux2b_in_sel":						"NTV2_Xpt425Mux2BInput",
    					"mux3a_in_sel":						"NTV2_Xpt425Mux3AInput",
    					"mux3b_in_sel":						"NTV2_Xpt425Mux3BInput",
    					"mux4a_in_sel":						"NTV2_Xpt425Mux4AInput",
    					"mux4b_in_sel":						"NTV2_Xpt425Mux4BInput",
    					"mux5a_in_sel":						"NTV2_Xpt425Mux5AInput",
    					"mux5b_in_sel":						"NTV2_Xpt425Mux5BInput",
    					"mux6a_in_sel":						"NTV2_Xpt425Mux6AInput",
    					"mux6b_in_sel":						"NTV2_Xpt425Mux6BInput",
    					"mux7a_in_sel":						"NTV2_Xpt425Mux7AInput",
    					"mux7b_in_sel":						"NTV2_Xpt425Mux7BInput",
    					"mux8a_in_sel":						"NTV2_Xpt425Mux8AInput",
    					"mux8b_in_sel":						"NTV2_Xpt425Mux8BInput"		}
    if inVerilogInputSelectName in inputXptMap:
        return inputXptMap [inVerilogInputSelectName]
    return ""


def do_device (args, device_id, verilogPath, hppPath):
    ErrorMessage =	(	"\n%s\n"												+
    					"## ERROR:  While processing '%s' in '%s'\n"			+
    					"           In line %d of verilog source file '%s'\n"	+
    					"           %d occurrence(s) of '%s' were found\n"		+
    					"           Expected only one"	)
    ErrorMessage2 =	(	"\n%s\n"												+
    					"## ERROR:  While processing '%s'\n"					+
    					"           In line %d of verilog source file '%s'\n"	+
    					"           'casex' statement encountered without prior closing 'endcase' for '%s'\n"	)
    ErrorMessage3 =	(	"\n%s\n"												+
    					"## ERROR:  While processing '%s'\n"					+
    					"           In line %d of verilog source file '%s'\n"	+
    					"           'endcase' statement encountered without prior opening 'casex'\n"	)
    ErrorMessage4 =	(	"\n%s\n"												+
    					"## ERROR:  While processing '%s'\n"					+
    					"           At end of verilog source file '%s'\n"		+
    					"           No prior closing 'endcase' for '%s'\n"	)
    input_line_count = 0
    output_line_count = 0
    missingInputSelects = {}
    if hppPath:
        with open (hppPath, 'a+') as f:
            inputSelect = ""
            f.write ("\n\n%s:\t\t//\tFrom '%s'\n" % (device_id, verilogPath))
            with open (verilogPath) as v:
                for line in v:
                    input_line_count = input_line_count + 1
                    if "casex (" in line:
                        if len (inputSelect) > 0:
                            msg = ErrorMessage2 % (line.strip (), device_id, input_line_count, verilogPath, inputSelect)
                            print(msg)
                            f.write ("%s\n\n** %s ABORTED **\n" % (msg, device_id))
                            f.flush ()
                            return 503
                        inputSelect = line.replace ("casex", "").replace ("(", "").replace (")", "").strip ()
                        #print "## DEBUG:  '%s' begin" % (inputSelect)
                    elif "endcase" in line:
                        if len (inputSelect) == 0:
                            msg = ErrorMessage3 % (line.strip (), device_id, input_line_count, verilogPath)
                            print(msg)
                            f.write ("%s\n\n** %s ABORTED **\n" % (msg, device_id))
                            f.flush ()
                            return 503
                        #print "## DEBUG:  '%s' end" % (inputSelect)
                        inputSelect = ""
                    elif "<=" in line and len (inputSelect) > 0 and not "default" in line:
                        if line.strip()[:2] != "//":
                            chunks = line.split ("<=")
                            if len (chunks) != 2:
                                msg = ErrorMessage % (line.strip (), inputSelect, device_id, input_line_count, verilogPath, len (chunks) - 1, '<=')
                                print(msg)
                                f.write ("%s\n\n** %s ABORTED **\n" % (msg, device_id))
                                f.flush ()
                                return 503
                            chunks = chunks[0].strip ().split (":")
                            if len (chunks) != 2 and len (chunks) != 3:
                                msg = ErrorMessage % (line.strip (), inputSelect, device_id, input_line_count, verilogPath, len (chunks) - 1, ':')
                                print(msg)
                                f.write ("%s\n\n** %s ABORTED **\n" % (msg, device_id))
                                f.flush ()
                                return 503
                            output_xpt_name = chunks[0].strip ().replace ("`", "")
                            ntv2OutputXpt = VerilogToNTV2OutXpt (output_xpt_name)
                            ntv2InputXpt = VerilogToNTV2InXpt (inputSelect)
                            output_line_count = output_line_count + 1
                            if ntv2InputXpt:
                                f.write ("%s <== '%s'\n" % (ntv2InputXpt, output_xpt_name))
                            else:
                                f.write ("'%s' <== '%s'\n" % (inputSelect, output_xpt_name))
                                if not inputSelect in missingInputSelects:
                                    print("## WARNING:  VerilogToNTV2InXpt:  No equivalent NTV2InputCrosspointID for '%s'" % (inputSelect))
                                    missingInputSelects [inputSelect] = 1 
            f.flush ()
            if len (inputSelect) > 0:
                msg = ErrorMessage4 % (line.strip (), device_id, verilogPath, inputSelect)
                print(msg)
                f.write ("%s\n\n** %s ABORTED **\n" % (msg, device_id))
                return 503
            if args.verbose:
                print("## NOTE:  %s:  %d lines read from '%s', %d lines written to '%s'" % (device_id, input_line_count, verilogPath, output_line_count, hppPath))
    return 0


def main ():
    #	Verilog file paths, relative to input directory:
    verilogPaths		= {	'DEVICE_ID_CORVID1':		"corvid/fpga/units/vp/verilog/crosspoint_c.v",
    						'DEVICE_ID_CORVID22':		"corvid22/fpga/units/vp/verilog/crosspoint_c22.v",
    						'DEVICE_ID_CORVID24':		"",
    						'DEVICE_ID_CORVID3G':		"",
    						'DEVICE_ID_CORVID44':		"DAX/FPGA/source/vp/xpt_c44.v",
    						'DEVICE_ID_CORVID88':		"DAX/FPGA/source/vp/xpt_dax.v",
    						'DEVICE_ID_CORVIDHBR':		"",
    						'DEVICE_ID_CORVIDHEVC':		"",
    						'DEVICE_ID_IO4K':			"IO-XT-4K/source/vp/xpt_io_xt_4k.v",
    						'DEVICE_ID_IO4KUFC':		"IO-XT-4K/source/vp_ufc/xpt_io_xt_4k_ufc.v",
    						'DEVICE_ID_IOEXPRESS':		"chekov/fpga/units/vp/verilog/crosspoint_c.v",
    						'DEVICE_ID_IOXT':			"IO_Thunderbolt/fpga/units/verilog/crosspoint.v",
    						'DEVICE_ID_KONA3G':			"KONA_3G/fpga/units/vp/verilog/crosspoint_c22.v",
    						'DEVICE_ID_KONA3GQUAD':		"",
    						'DEVICE_ID_KONA4':			"KONA4/source/vp/xpt_kona_4.v",
    						'DEVICE_ID_KONA4UFC':		"KONA4/source/vp_ufc/xpt_kona_4_ufc.v",
    						'DEVICE_ID_KONALHEPLUS':	"LHE_PLUS/fpga/units_new/vp/verilog/crosspoint.v",
    						'DEVICE_ID_KONALHI':		"",
    						'DEVICE_ID_TTAP':			""	}

    known_devices		= {	'corvid':		'DEVICE_ID_CORVID1',
    						'corvid22':		'DEVICE_ID_CORVID22',
    						'corvid24':		'DEVICE_ID_CORVID24',
    						'corvid3g':		'DEVICE_ID_CORVID3G',
    						'corvid44':		'DEVICE_ID_CORVID44',
    						'corvid88':		'DEVICE_ID_CORVID88',
    						'corvidhbr':	'DEVICE_ID_CORVIDHBR',
    						'corvidhevc':	'DEVICE_ID_CORVIDHEVC',
    						'io4k':			'DEVICE_ID_IO4K',
    						'io4kufc':		'DEVICE_ID_IO4KUFC',
    						'ioexpress':	'DEVICE_ID_IOEXPRESS',
    						'ioxt':			'DEVICE_ID_IOXT',
    						'kona3g':		'DEVICE_ID_KONA3G',
    						'kona3gquad':	'DEVICE_ID_KONA3GQUAD',
    						'kona4':		'DEVICE_ID_KONA4',
    						'kona4ufc':		'DEVICE_ID_KONA4UFC',
    						'konalheplus':	'DEVICE_ID_KONALHEPLUS',
    						'konalhi':		'DEVICE_ID_KONALHI',
    						'ttap':			'DEVICE_ID_TTAP'	}

    hppName		= "ntv2canconnect.hpp"
    hName		= "ntv2canconnect.hh"

    outputDir = ""
    inputDir = ""

    args = parse_args ()
    if args.input:
        inputDir = os.path.join (args.input)
        if not os.path.exists (inputDir) or not os.path.isdir (inputDir):
            print("## ERROR:  Input folder '%s' not found or not a folder" % (args.input))
            return 404
        if args.verbose:
            print("## NOTE:  Looking for verilog files relative to '%s'" % (args.input))
    else:
        inputDir = os.getcwd ()

    if args.output:
        outputDir = os.path.join (args.output)
        if not os.path.exists (outputDir) or not os.path.isdir (outputDir):
            print("## ERROR:  Output folder '%s' not found or not a folder" % (args.output))
            return 404
        if args.verbose:
            print("## NOTE:  Will write .hpp & .hh files into folder '%s'" % (args.output))
    else:
        outputDir = os.getcwd ()

    hppPath = os.path.join (outputDir, hppName)
    if os.path.exists (hppPath):
        if os.path.isdir (hppPath):
            print("## ERROR:  Output HPP file '%s' is a folder" % (hppPath))
            return 505
        if args.verbose:
            print("## NOTE:  Will overwrite existing .hpp file '%s'" % (hppPath))

    device_ids = list(known_devices.values ())
    if args.device:
        if not args.device in known_devices:
            print("## ERROR:  No such device '%s'" % (args.device))
            return 500
        device_ids = [known_devices [args.device]]

    with open (hppPath, 'w') as f:
        f.write ("//\tGenerated by '%s' on %s\n" % (sys.argv[0], datetime.datetime.now ().strftime ("%c")))
    for device_id in device_ids:
        if not device_id in verilogPaths:
            print("## WARNING:  '%s' not in 'verilogPaths' map" % (device_id))
            continue
        verilogPath = verilogPaths [device_id]
        if len (verilogPath) == 0:
            print("## WARNING:  Empty path to verilog file for '%s'" % (device_id))
            continue
        verilogPath = os.path.join (inputDir, verilogPath)
        if not os.path.exists (verilogPath):
            print("## WARNING:  Device '%s' verilog file path '%s' not found" % (device_id, verilogPath))
            continue
        if os.path.isdir (verilogPath):
            print("## WARNING:  Device '%s' verilog file path '%s' is folder" % (device_id, verilogPath))
            continue
        result_code = do_device (args, device_id, verilogPath, hppPath)

    return 0


if __name__ == '__main__':
    status = main ()
    sys.exit (status)
