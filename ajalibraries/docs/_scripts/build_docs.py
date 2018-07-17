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

	Assumptions (local development build):
		-	Desktop or Downloads folder contains: 'ntv2sdk*.zip' files for linux, mac and windows.
		-	Current directory contains full 'ajalibraries' source tree.
"""

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

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

sys.path.append (os.path.join ("installers", "pythonlib"))

import aja.utils
import aja.installer
import aja.build


def make_redirect_html (platform_name, dest_url, major, minor, point, build_num, build_type):
    # Create an HTML file that redirects to the given dest_url
    if build_type == '':
        display_version = "%s.%s.%s" % (major, minor, point)
        underscore_version = "%s_%s_%s_%s" % (major, minor, point, build_num)
    else:
        display_version = "%s.%s.%sb%s" % (major, minor, point, build_num)
        underscore_version = "%s_%s_%sbeta" % (major, minor, point)
    from_html = "NTV2SDK_docs_%s.html" % (underscore_version)
    from_url = "https://sdksupport.aja.com/docs/" + from_html
    with open (from_html, "w") as f:
        f.write ('<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">\n')
        f.write ('<html xmlns="http://www.w3.org/1999/xhtml">\n')
        f.write ('	<head>\n')
        f.write ('		<meta http-equiv="Content-Type" content="text/xhtml;charset=UTF-8"/>\n')
        f.write ('		<noscript><meta http-equiv="refresh" content="5; url=%s"></noscript>\n' % (dest_url))
        f.write ('		<title>AJA NTV2 SDK %s Documentation</title>\n' % (display_version))
        f.write ('	</head>\n')
        f.write ('	<body onload="window.location = ' + "'" + dest_url + "'" + '">\n')
        f.write ('		<h1>AJA NTV2 SDK %s for %s</h1>\n' % (display_version, platform_name))
        f.write ('		<h2>Redirecting to %s...</h2>\n' % (dest_url))
        f.write ('	</body>\n')
        f.write ('</html>\n')
    print "## NOTE:  MakeRedirectHTML:  %s:  File '%s' will redirect to '%s'" % (platform_name, from_url, dest_url)
    return from_html


def main ():
    args = aja.installer.parse_args ("require_version")
    result_code = 0

    versionComponents = args.version.split (".")
    if len (versionComponents) < 3:
        print "## ERROR:  %d version components found -- expected at least 3" % (len (versionComponents))
        return 501

    expected_major = versionComponents [0]
    expected_minor = versionComponents [1]
    expected_point = versionComponents [2]
    #test_number = versionComponents [3]
    #print "## DEBUG:  Expecting SDK %s.%s.%s   test_number: '%s'" % (expected_major, expected_minor, expected_point, test_number)

    print "## NOTE:  Platform node is '%s'" % (platform.node())
    if args.local == True and platform.node() == "mrbillmp.aja.com":
        build_dir = os.path.join("build-doxygen")
        home_dir = os.path.expanduser("~")
        print "## NOTE:  Local development build, using build folder '%s', home='%s'" % (build_dir, home_dir)
        if os.path.exists(build_dir):
            shutil.rmtree(build_dir)
        os.makedirs(build_dir)
        
        # Copy essentials into 'build_dir'...
        # Copy 'ajastyles.css' and 'config.doxy' from 'ajalibraries/docs'...
        aja.utils.copy_filelist(os.path.join('ajalibraries','docs'), build_dir, ['ajastyles.css', 'config.doxy'], False, "3")
        
        # Copy '_scripts' and 'doxygen' folders from 'ajalibraries/docs'...
        aja.utils.copy_filelist(os.path.join('ajalibraries','docs'), build_dir, ['_scripts', 'doxygen'], False, "3")
        
        # Try copying 'ntv2sdk*.zip' files out of current dir...
        copied_items = aja.utils.copy_filelist(os.path.join('.'), build_dir, ['ntv2sdk*.zip'], False, "3")
        if len(copied_items) == 0:
            # Try copying 'ntv2sdk*.zip' files out of Desktop...
            copied_items = aja.utils.copy_filelist(os.path.join(home_dir,'Desktop'), build_dir, ['ntv2sdk*.zip'], False, "3")
        if len(copied_items) == 0:
            # Try Downloads folder... 'ntv2sdk*.zip' files for linux, mac and windows...
            copied_items = aja.utils.copy_filelist(os.path.join(home_dir,'Downloads'), build_dir, ['ntv2sdk*.zip'], False, "3")
        if len(copied_items) != 3:
            print ("## ERROR:  %d SDK .zip file(s) copied, should be 3" % (len(copied_items)))
            print (copied_items)
            return 502
        # "cd" into 'build_dir' and continue...
        os.chdir(build_dir)

    print "## NOTE:  Starting Pre-Check"
    result_code = aja.utils.check ({'name':	'Pre-Check',	'must-exist': [	'ntv2sdkmac_*.zip',		'ntv2sdkwin_*.zip',		'ntv2sdklinux_*.zip',
    																		'config.doxy',			'doxygen'	] })
    if result_code <> 0:
        print "## NOTE:  Unzip Phase skipped due to error(s)"
        return result_code

    print "## NOTE:  Starting Unzip Phase"
    stuff_to_unzip =	[	{'name': 'MacSDK',	'rename': 'ntv2sdkmac_*.zip',					'to': 'ntv2sdkmac.zip'												},
    						{'name': 'MacSDK',	'unzip':  'ntv2sdkmac.zip',						'to': ''															},
    						{'name': 'MacSDK',	'delete': 'ntv2sdkmac.zip',						'to': ''															},
    						{'name': 'MacSDK',	'rename': 'ntv2sdkmac_*',						'to': 'AJAMacSDK'													},

    						{'name': 'WinSDK',	'rename': 'ntv2sdkwin_*.zip',					'to': 'ntv2sdkwin.zip'												},
    						{'name': 'WinSDK',	'unzip':  'ntv2sdkwin.zip',						'to': ''															},
    						{'name': 'WinSDK',	'delete': 'ntv2sdkwin.zip',						'to': ''															},
    						{'name': 'WinSDK',	'rename': 'ntv2sdkwin_*',						'to': 'AJAWinSDK'													},
    						{'name': 'WinSDK',	'move':   'win',								'from': 'AJAWinSDK/ajalibraries/ajantv2/src',			'to': ''	},
    						{'name': 'WinSDK',	'move':   'windows',							'from': 'AJAWinSDK/ajalibraries/ajabase/pnp',			'to': ''	},
    						{'name': 'WinSDK',	'rename': 'windows',							'to': 'winpnp'														},
    						{'name': 'WinSDK',	'move':   'windows',							'from': 'AJAWinSDK/ajalibraries/ajabase/system',		'to': ''	},
    						{'name': 'WinSDK',	'rename': 'windows',							'to': 'winsystem'													},
    						{'name': 'WinSDK',	'move':   'ntv2enums.h',						'from': 'AJAWinSDK/ajalibraries/ajantv2/includes',		'to': ''	},
    						{'name': 'WinSDK',	'rename': 'ntv2enums.h',						'to': 'winntv2enums.h'												},
    						{'name': 'WinSDK',	'delete': 'AJAWinSDK',							'to': ''															},

    						{'name': 'LinSDK',	'rename': 'ntv2sdklinux_*.zip',					'to': 'ntv2linux.zip'												},
    						{'name': 'LinSDK',	'unzip':  'ntv2linux.zip',						'to': ''															},
    						{'name': 'LinSDK',	'delete': 'ntv2linux.zip',						'to': ''															},
    						{'name': 'LinSDK',	'rename': 'ntv2sdklinux_*',						'to': 'AJALinuxSDK'													},
    						{'name': 'LinSDK',	'move':   'lin',								'from': 'AJALinuxSDK/ajalibraries/ajantv2/src',			'to': ''	},
    						{'name': 'LinSDK',	'move':   'linux',								'from': 'AJALinuxSDK/ajalibraries/ajabase/pnp',			'to': ''	},
    						{'name': 'LinSDK',	'rename': 'linux',								'to': 'linuxpnp'													},
    						{'name': 'LinSDK',	'move':   'linux',								'from': 'AJALinuxSDK/ajalibraries/ajabase/system',		'to': ''	},
    						{'name': 'LinSDK',	'rename': 'linux',								'to': 'linuxsystem'													},
    						{'name': 'LinSDK',	'move':   'ntv2enums.h',						'from': 'AJALinuxSDK/ajalibraries/ajantv2/includes',	'to': ''	},
    						{'name': 'LinSDK',	'rename': 'ntv2enums.h',						'to': 'linntv2enums.h'												},
    						{'name': 'LinSDK',	'delete': 'AJALinuxSDK',						'to': ''															},

    						{'name': 'Doxy',	'rename': 'config.doxy',						'to': 'patched_config.doxy'											},
    						{'name': 'MacSDK',	'move':   'ajaapps',							'from': 'AJAMacSDK',		'to': ''								},
    						{'name': 'MacSDK',	'move':   'ajalibraries',						'from': 'AJAMacSDK',		'to': ''								},
    						{'name': 'MacSDK',	'delete': 'AJAMacSDK',														'to': ''								},
    						{'name': 'LinSDK',	'move':   'lin',								'from': '',					'to': 'ajalibraries/ajantv2/src'		},
    						{'name': 'LinSDK',	'rename': 'linuxpnp',														'to': 'linux'							},
    						{'name': 'LinSDK',	'move':   'linux',								'from': '',					'to': 'ajalibraries/ajabase/pnp'		},
    						{'name': 'LinSDK',	'rename': 'linuxsystem',													'to': 'linux'							},
    						{'name': 'LinSDK',	'move':   'linux',								'from': '',					'to': 'ajalibraries/ajabase/system'	},

    						{'name': 'WinSDK',	'move':   'win',								'from': '',					'to': 'ajalibraries/ajantv2/src'		},
    						{'name': 'WinSDK',	'rename': 'winpnp',															'to': 'windows'							},
    						{'name': 'WinSDK',	'move':   'windows',							'from': '',					'to': 'ajalibraries/ajabase/pnp'		},
    						{'name': 'WinSDK',	'rename': 'winsystem',														'to': 'windows'							},
    						{'name': 'WinSDK',	'move':   'windows',							'from': '',					'to': 'ajalibraries/ajabase/system'	}	]
    result_code = aja.utils.stage (stuff_to_unzip)
    if result_code <> 0:
        print "## NOTE:  Patching Phase skipped due to error(s)"
        return result_code

    result_code = aja.utils.check ({'name': 'Check ntv2enums.h',  'file-exists': ['ajalibraries/ajantv2/includes/ntv2enums.h']})
    if result_code <> 0:
        print "## NOTE:  Patching Phase skipped due to error(s)"
        return result_code

    major =			aja.utils.get_defined_macro_value ('ajalibraries/ajantv2/includes/ntv2enums.h', 'AJA_NTV2_SDK_VERSION_MAJOR')
    minor =			aja.utils.get_defined_macro_value ('ajalibraries/ajantv2/includes/ntv2enums.h', 'AJA_NTV2_SDK_VERSION_MINOR')
    point =			aja.utils.get_defined_macro_value ('ajalibraries/ajantv2/includes/ntv2enums.h', 'AJA_NTV2_SDK_VERSION_POINT')
    build_num =		aja.utils.get_defined_macro_value ('ajalibraries/ajantv2/includes/ntv2enums.h', 'AJA_NTV2_SDK_BUILD_NUMBER')
    build_when =	aja.utils.get_defined_macro_value ('ajalibraries/ajantv2/includes/ntv2enums.h', 'AJA_NTV2_SDK_BUILD_DATETIME')
    build_type =	aja.utils.get_defined_macro_value ('ajalibraries/ajantv2/includes/ntv2enums.h', 'AJA_NTV2_SDK_BUILD_TYPE')
    if major != expected_major or minor != expected_minor or point != expected_point:
        print "## ERROR:  SDK version %s.%s.%s doesn't match expected version %s.%s.%s" % (major, minor, point, expected_major, expected_minor, expected_point)
        return 502

    if build_type == '':
        display_version = "%s.%s.%s" % (major, minor, point)
        underscore_version = "%s_%s_%s_%s" % (major, minor, point, build_num)
        print "## NOTE:  Documenting SDK %s.%s.%s build %s built on '%s'" % (major, minor, point, build_num, build_when)
    else:
        display_version = "%s.%s.%sb%s" % (major, minor, point, build_num)
        underscore_version = "%s_%s_%sbeta" % (major, minor, point)
        print "## NOTE:  Documenting SDK %s.%s.%s build %s (beta) built on '%s' -- underscore_version='%s'" % (major, minor, point, build_num, build_when, underscore_version)

	# Grab Windows SDK build numbers...
    win_major =			aja.utils.get_defined_macro_value ('winntv2enums.h', 'AJA_NTV2_SDK_VERSION_MAJOR')
    win_minor =			aja.utils.get_defined_macro_value ('winntv2enums.h', 'AJA_NTV2_SDK_VERSION_MINOR')
    win_point =			aja.utils.get_defined_macro_value ('winntv2enums.h', 'AJA_NTV2_SDK_VERSION_POINT')
    win_build_num =		aja.utils.get_defined_macro_value ('winntv2enums.h', 'AJA_NTV2_SDK_BUILD_NUMBER')
    win_build_when =	aja.utils.get_defined_macro_value ('winntv2enums.h', 'AJA_NTV2_SDK_BUILD_DATETIME')
    win_build_type =	aja.utils.get_defined_macro_value ('winntv2enums.h', 'AJA_NTV2_SDK_BUILD_TYPE')
    if major != win_major or minor != win_minor or point != win_point:
        print "## ERROR:  SDK version %s.%s.%s doesn't match Windows version %s.%s.%s" % (major, minor, point, win_major, win_minor, win_point)
        return 502

	# Grab Linux SDK build numbers...
    lin_major =			aja.utils.get_defined_macro_value ('linntv2enums.h', 'AJA_NTV2_SDK_VERSION_MAJOR')
    lin_minor =			aja.utils.get_defined_macro_value ('linntv2enums.h', 'AJA_NTV2_SDK_VERSION_MINOR')
    lin_point =			aja.utils.get_defined_macro_value ('linntv2enums.h', 'AJA_NTV2_SDK_VERSION_POINT')
    lin_build_num =		aja.utils.get_defined_macro_value ('linntv2enums.h', 'AJA_NTV2_SDK_BUILD_NUMBER')
    lin_build_when =	aja.utils.get_defined_macro_value ('linntv2enums.h', 'AJA_NTV2_SDK_BUILD_DATETIME')
    lin_build_type =	aja.utils.get_defined_macro_value ('linntv2enums.h', 'AJA_NTV2_SDK_BUILD_TYPE')
    if major != lin_major or minor != lin_minor or point != lin_point:
        print "## WARNING:  SDK version %s.%s.%s doesn't match Linux version %s.%s.%s" % (major, minor, point, lin_major, lin_minor, lin_point)
        #return 502

    html_folder_path = "NTV2SDK_docs_%s" % (underscore_version)
    client_browser_url = "https://sdksupport.aja.com/docs/" + html_folder_path + "/"

    if args.local == True and platform.node() == "mrbillmp.aja.com":
        # Local dev builds use the local dev tree's ajalibraries ... not those extracted from the SDK zips...
        print("Local dev build ... replace ajalibraries/ajabase with dev tree's ajabase folder...")
        shutil.rmtree(os.path.join("ajalibraries","ajabase"))
        copied_items = aja.utils.copy_filelist(os.path.join("..", "ajalibraries"), os.path.join("ajalibraries"), ['ajabase'], False, "3")
        
        print("Local dev build ... replace ajalibraries/ajaanc with dev tree's ajaanc folder...")
        shutil.rmtree(os.path.join("ajalibraries","ajaanc"))
        copied_items = aja.utils.copy_filelist(os.path.join("..", "ajalibraries"), os.path.join("ajalibraries"), ['ajaanc'], False, "3")
        
        print("Local dev build ... replace ajalibraries/ajacc with dev tree's ajacc folder...")
        shutil.rmtree(os.path.join("ajalibraries","ajacc"))
        copied_items = aja.utils.copy_filelist(os.path.join("..", "ajalibraries"), os.path.join("ajalibraries"), ['ajacc'], False, "3")
        
        print("Local dev build ... replace ajalibraries/ajantv2 with dev tree's ajantv2 folder...")
        shutil.rmtree(os.path.join("ajalibraries","ajantv2"))
        copied_items = aja.utils.copy_filelist(os.path.join("..", "ajalibraries"), os.path.join("ajalibraries"), ['ajantv2'], False, "3")
        shutil.rmtree(os.path.join("ajalibraries","ajantv2","gpuclasses"))

    """
    Patch in version number, remove NTV2_NUB_CLIENT_SUPPORT...
    """
    stuff_to_patch =	[	{	'name':		'Versionize Doxy Config File',
    							'files':	'patched_config.doxy',
    							'replace':	{'0.0.0':	display_version}				},
    						{	'name':		'Remove Nub Client References',
    							'files':	'ajalibraries/ajantv2/includes/ajatypes.h',
    							'dellines':	'NTV2_NUB_CLIENT_SUPPORT'	}	]
    result_code = aja.utils.patch (stuff_to_patch)
    if result_code <> 0:
        print "## NOTE:  Build Phase skipped due to error(s)"
        return result_code

    """
    Items to build...
    """
    if args.nocompile == False:
        print "## NOTE:  Starting Build Phase"
        result_code = aja.build.build ([{'name': 'Doxygen',	'makefile': 'patched_config.doxy', 'log': 'doxygen.log'}])
        if result_code <> 0:
            print "## NOTE:  Delivery Phase skipped due to error(s)"
            return result_code
    else:
        print "## NOTE:  Skipping Build Phase"

    print "## NOTE:  Delivering HTML Docs"
    result_code = aja.utils.stage ([{'name': 'Rename HTML Folder',  'rename': 'html',			'from': '',		'to': html_folder_path	},
    								{'name': 'Zip HTML Folder',		'zip':  html_folder_path,	'from': '',		'to': ''				}])
    if result_code <> 0:
        print "## NOTE:  'rsync' Phase skipped due to error(s)"
        return result_code

    destination_rsync_url = "sdkdocs@sdksupport.aja.com:/docs/"
    if platform.node () == "mrbillmp.aja.com":
        destination_rsync_url = "/Users/demo/Sites/"
    print "## NOTE:  rsync '%s' to '%s'..." % (html_folder_path, destination_rsync_url)
    cmd_lines = []
    cmd_lines.append (['rsync', '-avc', '--delete', html_folder_path, destination_rsync_url])
    if build_type:
        win_html = make_redirect_html ('Windows', client_browser_url, win_major, win_minor, win_point, win_build_num, win_build_type)
        lin_html = make_redirect_html ('Linux',   client_browser_url, lin_major, lin_minor, lin_point, lin_build_num, lin_build_type)
        cmd_lines.append (['rsync', '-av', win_html, destination_rsync_url])
        cmd_lines.append (['rsync', '-av', lin_html, destination_rsync_url])
    result_code = aja.utils.run_command_lines (cmd_lines, "rsync.log", 4)
    if result_code == 0:
        print "## NOTE:  Documentation set should be accessible using URL:  %s" % (client_browser_url)
    else:
        print "## ERROR:  Documentation failed to rsync to '%s' due to error %d" % (destination_rsync_url, result_code)
    return 0

if __name__ == '__main__':
    status = main ()
    sys.exit (status)
