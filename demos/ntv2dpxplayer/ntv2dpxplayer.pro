# -----------------------------------------------------------------------------
# Cross platform project used to generate platform specific makefiles/projects.
#
# To generate makefile/projects:
# Make sure the NTV2_SDK_DIR environment variable is set for Windows and Mac
# Make sure the AJA_API_DIR environment variable is set for Windows and Mac and Linux
# Make sure the AJA_3RDPARTY_DIR environment variable is set for Windows and Mac and Linux
# Make sure the AJA_NTV2_ROOT environment variable is set for Linux
#
#
# Windows:	use the VisualStudio addon, from Qt menu select
#			'Open Qt Project File (.pro)...' and select this file.
#
# Mac:		for xcode project: qmake -spec macx-xcode qtcontrolpanel.pro
#			for makefile: qmake -spec macx-g++ ntv2qtpreview.pro
#
# Linux:	qmake -unix ntv2qtpreview.pro
#
# Note: We use the scope unix:!macx for unixes that are not Mac OS X
# -----------------------------------------------------------------------------

# Common across platforms

TEMPLATE = app

TARGET = ntv2dpxplayer

DEPENDPATH += .
UI_DIR += ./generatedfiles
RCC_DIR += ./generatedfiles
MOC_DIR += ./generatedfiles

# QtComponents needed (NOTE: core and gui are included by default)
QT += core gui multimedia

win32{
	NTV2_DIR = $$(NTV2_SDK_DIR)
	AJA_API = $$(AJA_API_DIR)
	CONFIG += debug_and_release
}
macx{
	NTV2_DIR = $$(NTV2_SDK_DIR)
	AJA_API = $$(AJA_API_DIR)
	CONFIG += debug
}
unix:!macx{
	NTV2_DIR = $$system(pwd)/../..
	AJA_API = $$NTV2_DIR/../ajaapi
	DEBUG_TEST_FLAG = $$(AJA_DEBUG)
}
AJA_3RDPARTY = $$(AJA_3RDPARTY_DIR)


INCLUDEPATH += $$AJA_API
INCLUDEPATH += $$NTV2_DIR/classes
INCLUDEPATH += $$NTV2_DIR/includes
INCLUDEPATH += $$NTV2_DIR/democlasses

# Windows specific
win32 {
	DEFINES +=  MSWindows
	DEFINES +=  AJA_WINDOWS
	DEFINES +=  AJADLL
	DEFINES +=  AJADLL_BUILD
	DEFINES +=  AJA_BASE_OBJ
	DEFINES +=  QT_LARGEFILE_SUPPORT
	DEFINES -= UNICODE _UNICODE


	#disable all-warnings-on flags, which is the default (i.e. -Wall -W)
	QMAKE_CFLAGS_WARN_ON	=
	QMAKE_CXXFLAGS_WARN_ON	=

	INCLUDEPATH += $$NTV2_DIR/winclasses

	LIBS += -lsetupapi
    LIBS += -lwinmm
    LIBS += -lImm32
    LIBS += -lshlwapi
    LIBS += -lurlmon
    LIBS += -lRpcrt4
    LIBS += -lWs2_32
}


# Mac specific
macx {
	# only build for Intel not PowerPC
	CONFIG += x86_64

	# force 10.6 and higher
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
	QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.6.sdk

	DEFINES += AJA_MAC
	DEFINES += AJAMac
	DEFINES += __STDC_CONSTANT_MACROS

	#disable all-warnings-on flags, which is the default (i.e. -Wall -W)
	QMAKE_CFLAGS_WARN_ON	=
	QMAKE_CXXFLAGS_WARN_ON	=

	#instead, turn on "most" warnings, plus some extra
	QMAKE_CFLAGS			+= -Wmost
	QMAKE_CFLAGS			+= -Wno-trigraphs
	QMAKE_CFLAGS			+= -Wno-four-char-constants
	QMAKE_CFLAGS			+= -Wno-unknown-pragmas
	QMAKE_CXXFLAGS			+= -Wmost
	QMAKE_CXXFLAGS			+= -Wno-trigraphs
	QMAKE_CXXFLAGS			+= -Wno-four-char-constants
	QMAKE_CXXFLAGS			+= -Wno-unknown-pragmas

	INCLUDEPATH += $$NTV2_DIR/macclasses

	LIBS += -framework ApplicationServices
   	LIBS += -framework IOKit
    # LIBS += -lSMDK-Mac.3.3
    # LIBS += -lmp4decMT

	# Set the custom plist
	QMAKE_INFO_PLIST = Info.plist

	ICON = app.icns
}


# Unix specific
unix:!macx {
	DEFINES += AJA_LINUX
	DEFINES += AJALinux
	DEFINES += __STDC_CONSTANT_MACROS

	INCLUDEPATH += $$NTV2_DIR/linuxclasses

	QMAKE_LIBDIR += $$AJA_API/lib64
	QMAKE_CLEAN += $$DESTDIR/$$TARGET $$NTV2_DIR/bin/$$TARGET
}

# stuff to turn on if CONFIG contains debug
build_pass:CONFIG(release, debug|release) {
	DESTDIR = ../../bin
	OBJECTS_DIR += release

	#release specific per platform settings
	win32{
                CFLAGS -= MT
                CFKAGS += MD
   		QMAKE_LFLAGS_RELEASE += "/FORCE:MULTIPLE"
#		QMAKE_LIBDIR += $$NTV2_DIR/winworkspace/Release
#		LIBS += -lclassesDLL


	}

	macx {
		QtMenu.files = $$(HOME)/qt_menu.nib
		QtMenu.path = Contents/Resources
		QMAKE_BUNDLE_DATA += QtMenu

		####NOTE: Need SRCodec
	}

	unix:!macx {
		# Ignore build_pass an use environment to choose debug or release
		contains( DEBUG_TEST_FLAG, 1 ) {
			QMAKE_LIBDIR += $$NTV2_DIR/lib-DEBUG
			LIBS += -Wl,-Bstatic -lajabased -laja -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajabase/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
			OBJECTS_DIR -= release
			OBJECTS_DIR += debug
		} else {
			QMAKE_LIBDIR += $$NTV2_DIR/lib
			LIBS += -Wl,-Bstatic -lajabase -laja -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajabase/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
		}
	}
}
else {

	DEFINES += AJA_DEBUG

	DESTDIR = ../../bin
	OBJECTS_DIR += debug


	#debug specific per platform settings
	win32{
        CFLAGS -= MTd
        CFKAGS += MDd
		QMAKE_LFLAGS_DEBUG += "/FORCE:MULTIPLE"
#		QMAKE_LIBDIR += $$NTV2_DIR/winworkspace/Debug
#		LIBS += -lclassesDLL
	}

	macx {
		QtMenu.files = $$(HOME)/qt_menu.nib
		QtMenu.path = Contents/Resources
		QMAKE_BUNDLE_DATA += QtMenu
	}

	unix:!macx {
		# Ignore build_pass an use environment to choose debug or release
		contains( DEBUG_TEST_FLAG, 1 ) {
			QMAKE_LIBDIR += $$NTV2_DIR/lib-DEBUG
			LIBS += -Wl,-Bstatic -lajabased -laja -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajabase/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
		} else {
			QMAKE_LIBDIR += $$NTV2_DIR/lib
			LIBS += -Wl,-Bstatic -lajabase -laja -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajabase/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
			OBJECTS_DIR -= debug
			OBJECTS_DIR += release
		}
	}
}


#local sources
SOURCES += main.cpp
SOURCES += ntv2dpxplayer.cpp
HEADERS += ntv2dpxplayer.h

HEADERS += $$NTV2_DIR/democlasses/ntv2record.h
HEADERS += $$NTV2_DIR/democlasses/ntv2playback.h

HEADERS += $$NTV2_DIR/democlasses/ntv2playbackdpx.h
SOURCES += $$NTV2_DIR/democlasses/ntv2playbackdpx.cpp
HEADERS += $$NTV2_DIR/democlasses/ntv2recorddpx.h
SOURCES += $$NTV2_DIR/democlasses/ntv2recorddpx.cpp

HEADERS += $$NTV2_DIR/democlasses/dpx_hdr.h
SOURCES += $$NTV2_DIR/democlasses/dpx_hdr.cpp

HEADERS += $$NTV2_DIR/democlasses/ajawavewriter.h
SOURCES += $$NTV2_DIR/democlasses/ajawavewriter.cpp

HEADERS += $$NTV2_DIR/democlasses/ajapreviewwidget.h
SOURCES += $$NTV2_DIR/democlasses/ajapreviewwidget.cpp

HEADERS += $$AJA_API/ajainternal/gui/ajaapplication.h
SOURCES += $$AJA_API/ajainternal/gui/ajaapplication.cpp
HEADERS += $$AJA_API/ajainternal/gui/ajacombobox.h
SOURCES += $$AJA_API/ajainternal/gui/ajacombobox.cpp
HEADERS += $$AJA_API/ajainternal/gui/ajapushbutton.h
SOURCES += $$AJA_API/ajainternal/gui/ajapushbutton.cpp
HEADERS += $$AJA_API/ajainternal/gui/ajaslider.h
SOURCES += $$AJA_API/ajainternal/gui/ajaslider.cpp
HEADERS += $$AJA_API/ajainternal/gui/ajaspinbox.h
SOURCES += $$AJA_API/ajainternal/gui/ajaspinbox.cpp

macx {
	SOURCES += $$NTV2_DIR/classes/ntv2autocirculate.cpp
	SOURCES += $$NTV2_DIR/classes/ntv2devicefeatures.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2devicefeatures.h
	SOURCES += $$NTV2_DIR/classes/ntv2devicescanner.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2devicescanner.h
	SOURCES += $$NTV2_DIR/classes/ntv2card.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2card.h
	SOURCES += $$NTV2_DIR/classes/ntv2driverinterface.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2driverinterface.h
	SOURCES += $$NTV2_DIR/classes/ntv2interrupts.cpp
	SOURCES += $$NTV2_DIR/macclasses/ntv2macdriverinterface.cpp
	HEADERS += $$NTV2_DIR/macclasses/ntv2macdriverinterface.h
	SOURCES += $$NTV2_DIR/classes/ntv2nubaccess.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2nubaccess.h
	SOURCES += $$NTV2_DIR/classes/ntv2nubpktcom.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2nubpktcom.h
	SOURCES += $$NTV2_DIR/classes/ntv2register.cpp
	SOURCES += $$NTV2_DIR/classes/ntv2status.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2status.h
	SOURCES += $$NTV2_DIR/classes/ntv2subscriptions.cpp
	SOURCES += $$NTV2_DIR/classes/ntv2transcode.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2transcode.h
	SOURCES += $$NTV2_DIR/classes/ntv2utils.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2utils.h
	SOURCES += $$NTV2_DIR/classes/ntv2xilinxbitfile.cpp
	HEADERS += $$NTV2_DIR/classes/ntv2xilinxbitfile.h
	SOURCES += $$NTV2_DIR/classes/videoutilities.cpp
	HEADERS += $$NTV2_DIR/classes/videoutilities.h
	SOURCES += $$NTV2_DIR/classes/xena2routing.cpp
	HEADERS += $$NTV2_DIR/classes/xena2routing.h
}


#resources
RESOURCES += ntv2dpxplayer.qrc
RESOURCES += $$AJA_API/ajainternal/gui/ajaapplication.qrc

