#!/usr/bin/env python

"""
	Synopsis:
		Builds the SDK docs.

	Usage (current directory contains 'ajalibraries', 'bin', etc.):
		build_sdk   -v {maj.min.point.build}  [-b betaNum]

	Options:
		--verbose						Use verbose output.
		--failwarnings					Treat warnings as errors.

	Assumptions (normal build):
		-	Current directory contains:
			-	'ntv2sdk' zip files for linux, mac and windows
			-	'_scripts' and 'doxygen' folders from 'ajalibraries/docs'
			-	'ajastyles.css' and 'config.doxy' from 'ajalibraries/docs'
			-	'installers/pythonlib' folder
"""

import sys
import os
import argparse
import shutil
import glob
import fnmatch
import subprocess
import datetime
import re
import platform

if sys.version_info >= (3, 6):
    pass
else:
    print("This script requires Python 3.6 or higher")
    exit(1)

sys.path.append(os.path.join("installers", "pythonlib"))

import aja.utils
import aja.installer
import aja.build


def make_redirect_html(
    platform_name, dest_url, major, minor, point, build_num, build_type
):
    # Create an HTML file that redirects to the given dest_url
    if build_type == "":
        display_version = "%s.%s.%s" % (major, minor, point)
        underscore_version = "%s_%s_%s_%s" % (major, minor, point, build_num)
    else:
        display_version = "%s.%s.%sb%s" % (major, minor, point, build_num)
        underscore_version = "%s_%s_%sbeta" % (major, minor, point)
    from_html = "NTV2SDK_docs_%s.html" % (underscore_version)
    from_url = "https://sdksupport.aja.com/docs/" + from_html
    with open(from_html, "w") as f:
        f.write(
            '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">\n'
        )
        f.write('<html xmlns="http://www.w3.org/1999/xhtml">\n')
        f.write("	<head>\n")
        f.write(
            '		<meta http-equiv="Content-Type" content="text/xhtml;charset=UTF-8"/>\n'
        )
        f.write(
            '		<noscript><meta http-equiv="refresh" content="5; url=%s"></noscript>\n'
            % (dest_url)
        )
        f.write("		<title>AJA NTV2 SDK %s Documentation</title>\n" % (display_version))
        f.write("	</head>\n")
        f.write('	<body onload="window.location = ' + "'" + dest_url + "'" + '">\n')
        f.write(
            "		<h1>AJA NTV2 SDK %s for %s</h1>\n" % (display_version, platform_name)
        )
        f.write("		<h2>Redirecting to %s...</h2>\n" % (dest_url))
        f.write("	</body>\n")
        f.write("</html>\n")
    print(
        "## NOTE:  MakeRedirectHTML:  %s:  File '%s' will redirect to '%s'"
        % (platform_name, from_url, dest_url)
    )
    return from_html


def main():
    args = aja.installer.parse_args("require_version")
    result_code = 0

    versionComponents = args.version.split(".")
    if len(versionComponents) < 3:
        print(
            "## ERROR:  %d version components found -- expected at least 3"
            % (len(versionComponents))
        )
        return 501

    expected_major = versionComponents[0]
    expected_minor = versionComponents[1]
    expected_point = versionComponents[2]
    # test_number = versionComponents [3]
    # print("## DEBUG:  Expecting SDK %s.%s.%s   test_number: '%s'" % (expected_major, expected_minor, expected_point, test_number))

    print("## NOTE:  Starting Pre-Check")
    result_code = aja.utils.check (  {       "name": "Pre-Check",
                                       "must-exist": [ "artifacts-input/ntv2sdkmac_*.zip",
                                                       "artifacts-input/ntv2sdkwin_*.zip",
                                                       "artifacts-input/ntv2sdklinux_*.zip",
                                                       "ajalibraries/docs/config.doxy",
                                                       "ajalibraries/docs/doxygen"      ]  }  )
    if result_code != 0:
        print("## NOTE:  Unzip Phase skipped due to error(s)")
        return result_code

    print("## NOTE:  Starting Unzip Phase")
    stuff_to_unzip = [  {"name": "MacSDK",  "rename": "ntv2sdkmac_*.zip",   "from": "artifacts-input",    "to": "ntv2sdkmac.zip"},
                        {"name": "MacSDK",   "unzip": "artifacts-input/ntv2sdkmac.zip",                   "to": "work"},
                        {"name": "MacSDK",  "rename": "ntv2sdkmac_*",       "from": "work",               "to": "AJAMacSDK"},

                        {"name": "WinSDK",  "rename": "ntv2sdkwin_*.zip",   "from": "artifacts-input",    "to": "ntv2sdkwin.zip"},
                        {"name": "WinSDK",   "unzip": "artifacts-input/ntv2sdkwin.zip",                   "to": "work"},
                        {"name": "WinSDK",  "rename": "ntv2sdkwin_*",       "from": "work",               "to": "AJAWinSDK"},
                        {"name": "WinSDK",    "move": "win",                "from": "work/AJAWinSDK/ajalibraries/ajantv2/src",     "to": "work/AJAMacSDK/ajalibraries/ajantv2/src"},
                        {"name": "WinSDK",    "move": "windows",            "from": "work/AJAWinSDK/ajalibraries/ajabase/system",  "to": "work/AJAMacSDK/ajalibraries/ajabase/system"},
                        #{"name": "WinSDK",  "delete": "AJAWinSDK",          "from": "work",               "to": ""},

                        {"name": "LinSDK",  "rename": "ntv2sdklinux_*.zip", "from": "artifacts-input",    "to": "ntv2linux.zip"},
                        {"name": "LinSDK",   "unzip": "artifacts-input/ntv2linux.zip",                    "to": "work"},
                        {"name": "LinSDK",  "rename": "ntv2sdklinux_*",     "from": "work",               "to": "AJALinSDK"},
                        {"name": "LinSDK",    "move": "lin",                "from": "work/AJALinSDK/ajalibraries/ajantv2/src",     "to": "work/AJAMacSDK/ajalibraries/ajantv2/src"},
                        {"name": "LinSDK",    "move": "linux",              "from": "work/AJALinSDK/ajalibraries/ajabase/system",  "to": "work/AJAMacSDK/ajalibraries/ajabase/system"},
                        #{"name": "LinSDK",  "delete": "AJALinSDK",          "from": "work",               "to": ""},

                        {"name": "Doxy",      "copy": "config.doxy",        "from": "ajalibraries/docs",  "to": "work/AJAMacSDK"},
                        {"name": "Doxy",    "rename": "config.doxy",        "from": "work/AJAMacSDK",     "to": "patched_config.doxy"},
                        {"name": "Doxy",      "copy": "docs",               "from": "ajalibraries",       "to": "work/AJAMacSDK/ajalibraries"}
                    ]
    result_code = aja.utils.stage(stuff_to_unzip)
    if result_code != 0:
        print("## NOTE:  Patching Phase skipped due to error(s)")
        return result_code

    # Grab Mac SDK build numbers...
    major      = aja.utils.get_defined_macro_value ("work/AJAMacSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_VERSION_MAJOR")
    minor      = aja.utils.get_defined_macro_value ("work/AJAMacSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_VERSION_MINOR")
    point      = aja.utils.get_defined_macro_value ("work/AJAMacSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_VERSION_POINT")
    build_num  = aja.utils.get_defined_macro_value ("work/AJAMacSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_BUILD_NUMBER")
    build_when = aja.utils.get_defined_macro_value ("work/AJAMacSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_BUILD_DATETIME")
    build_type = aja.utils.get_defined_macro_value ("work/AJAMacSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_BUILD_TYPE")
    if major != expected_major  or  minor != expected_minor  or  point != expected_point:
        print ("## ERROR:  SDK version %s.%s.%s doesn't match expected version %s.%s.%s" % (major, minor, point, expected_major, expected_minor, expected_point))
        return 502

    if build_type == "":
        display_version = "%s.%s.%s" % (major, minor, point)
        underscore_version = "%s_%s_%s_%s" % (major, minor, point, build_num)
        print ("## NOTE:  Documenting SDK %s.%s.%s build %s built on '%s'" % (major, minor, point, build_num, build_when))
    else:
        display_version = "%s.%s.%sb%s" % (major, minor, point, build_num)
        underscore_version = "%s_%s_%sbeta" % (major, minor, point)
        print ("## NOTE:  Documenting SDK %s.%s.%s build %s (beta) built on '%s' -- underscore_version='%s'" % (major, minor, point, build_num, build_when, underscore_version))

    # Grab Windows SDK build numbers...
    win_major      = aja.utils.get_defined_macro_value("work/AJAWinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_VERSION_MAJOR")
    win_minor      = aja.utils.get_defined_macro_value("work/AJAWinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_VERSION_MINOR")
    win_point      = aja.utils.get_defined_macro_value("work/AJAWinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_VERSION_POINT")
    win_build_num  = aja.utils.get_defined_macro_value("work/AJAWinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_BUILD_NUMBER")
    win_build_when = aja.utils.get_defined_macro_value("work/AJAWinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_BUILD_DATETIME")
    win_build_type = aja.utils.get_defined_macro_value("work/AJAWinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_BUILD_TYPE")
    if major != win_major  or  minor != win_minor  or  point != win_point:
        print ("## ERROR:  SDK version %s.%s.%s doesn't match Windows version %s.%s.%s" % (major, minor, point, win_major, win_minor, win_point))
        return 502

    # Grab Linux SDK build numbers...
    lin_major      = aja.utils.get_defined_macro_value("work/AJALinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_VERSION_MAJOR")
    lin_minor      = aja.utils.get_defined_macro_value("work/AJALinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_VERSION_MINOR")
    lin_point      = aja.utils.get_defined_macro_value("work/AJALinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_VERSION_POINT")
    lin_build_num  = aja.utils.get_defined_macro_value("work/AJALinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_BUILD_NUMBER")
    lin_build_when = aja.utils.get_defined_macro_value("work/AJALinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_BUILD_DATETIME")
    lin_build_type = aja.utils.get_defined_macro_value("work/AJALinSDK/ajalibraries/ajantv2/includes/ntv2version.h", "AJA_NTV2_SDK_BUILD_TYPE")
    if major != lin_major  or  minor != lin_minor  or  point != lin_point:
        print ("## WARNING:  SDK version %s.%s.%s doesn't match Linux version %s.%s.%s" % (major, minor, point, lin_major, lin_minor, lin_point))
        # return 502

    html_folder_name = "NTV2SDK_docs_%s" % (underscore_version)
    client_browser_url = "https://sdksupport.aja.com/docs/" + html_folder_name + "/"

    """
    Patch in version number...
    """
    result_code = aja.utils.patch (  [  {    "name": "Versionize Doxy Config File",
                                            "files": "work/AJAMacSDK/patched_config.doxy",
                                          "replace": {"0.0.0": display_version}        }  ]  )
    if result_code != 0:
        print("## NOTE:  Build Phase skipped due to error(s)")
        return result_code

    """
    Build docs...
    """
    if args.nocompile is False:
        with aja.utils.controlled_cd("work/AJAMacSDK"):
            print("## NOTE:  Starting Build Phase")
            result_code = aja.build.build (  [  {    "name": "Doxygen",
                                                 "makefile": "patched_config.doxy",
                                                      "log": "doxygen.log"         }  ]  )
        if result_code != 0:
            print("## NOTE:  Delivery Phase skipped due to error(s)")
            return result_code
    else:
        print("## NOTE:  Build Phase intentionally skipped")


    """
    Deliver docs...
    """
    print("## NOTE:  Delivering HTML Docs")
    result_code = aja.utils.stage (  [  {"name": "HTML",    "move": "*.qch",           "from": "work/AJAMacSDK/html",  "to": "artifacts-output" },
                                        {"name": "HTML",  "rename": "html",            "from": "work/AJAMacSDK",       "to": html_folder_name   },
                                        {"name": "HTML",     "zip": html_folder_name,  "from": "work/AJAMacSDK",       "to": ""                 },
                                        {"name": "HTML",    "move": "NTV2SDK_docs_*",  "from": "work/AJAMacSDK",       "to": "artifacts-output" },
                                        {"name": "HTML",    "move": "doxygen.log",     "from": "work/AJAMacSDK",       "to": "artifacts-output" }  ]  )
    if result_code != 0:
        print("## NOTE:  'rsync' Phase skipped due to error(s)")
        return result_code

    html_folder_path = "artifacts-output/%s" % (html_folder_name)
    destination_rsync_url = "sdkdocs@sdksupport.aja.com:/docs/"
    rsync_options = "-avc"
    if platform.node().lower() == "mrbillmp.aja.com":
        destination_rsync_url = "/Users/demo/Sites/"
        rsync_options = "-avnc"
    print("## NOTE:  rsync '%s' to '%s'..." % (html_folder_path, destination_rsync_url))
    cmd_lines = []
    cmd_lines.append(["rsync", rsync_options, "--delete", html_folder_path, destination_rsync_url])
    if build_type:
        win_html = make_redirect_html ("Windows",  client_browser_url,  win_major,  win_minor,  win_point,  win_build_num,  win_build_type)
        lin_html = make_redirect_html ("Linux",    client_browser_url,  lin_major,  lin_minor,  lin_point,  lin_build_num,  lin_build_type)
        cmd_lines.append(["rsync", "-av", win_html, destination_rsync_url])
        cmd_lines.append(["rsync", "-av", lin_html, destination_rsync_url])
    result_code = aja.utils.run_command_lines(cmd_lines, "artifacts-output/rsync.log", 4)
    if result_code == 0:
        print ("## NOTE:  Documentation set should be accessible using URL:  %s" % (client_browser_url))
    else:
        print ("## ERROR:  Documentation failed to rsync to '%s' due to error %d" % (destination_rsync_url, result_code))
    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
