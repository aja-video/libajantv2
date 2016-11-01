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

TARGET = ntv2qtcudapreview

DEPENDPATH += $$PWD
UI_DIR	+= $$PWD/generatedfiles
RCC_DIR += $$PWD/generatedfiles
MOC_DIR += $$PWD/generatedfiles

QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}

win32{
	NTV2_DIR = $$PWD/../../../
        AJA_API    = $$PWD/../../../../ajaapi
        contains(QMAKE_TARGET.arch, x86_64) {
                DESTDIR = $$PWD/../../../../bin
	} else {
                DESTDIR = $$PWD/../../../../bin
	}
	CONFIG += debug_and_release
}


INCLUDEPATH += $$AJA_API
INCLUDEPATH += $$NTV2_DIR/classes
INCLUDEPATH += $$NTV2_DIR/includes
INCLUDEPATH += $$NTV2_DIR/democlasses

CUDA_DIR = C:/CUDA/v6.5/
CUDA_LIB_DIR = C:/CUDA/v6.5/lib/x64/
CUDA_INC_DIR = C:/CUDA/v6.5/include
CUDA_BIN_DIR = C:/CUDA/v6.5/bin/
INCLUDEPATH += $$CUDA_INC_DIR
QMAKE_LIBDIR += $$CUDA_LIB_DIR
LIBS += -lcudart -lcuda

# Windows specific
win32 {
	DEFINES += MSWindows
	DEFINES += AJA_WINDOWS
	DEFINES += AJADLL
	DEFINES += AJA_WINDLL
	DEFINES += QT_LARGEFILE_SUPPORT
	DEFINES -= UNICODE


	#disable all-warnings-on flags, which is the default (i.e. -Wall -W)
	#QMAKE_CFLAGS_WARN_ON =
	#QMAKE_CXXFLAGS_WARN_ON =

	INCLUDEPATH += $$NTV2_DIR/winclasses

	LIBS += -lsetupapi
	LIBS += -lwinmm
	LIBS += -lImm32
	LIBS += -lshlwapi
	LIBS += -lurlmon
	LIBS += -lRpcrt4
	LIBS += -lWs2_32
}



# stuff to turn on if CONFIG contains debug
build_pass:CONFIG(release, debug|release) {
	OBJECTS_DIR += release

	#release specific per platform settings
	win32{
		CFLAGS -= MT
		CFLAGS += MD
		QMAKE_LFLAGS_RELEASE += "/FORCE:MULTIPLE"


	}

}
else {

	DEFINES += AJA_DEBUG
	OBJECTS_DIR += debug


	#debug specific per platform settings
	win32{
		CFLAGS -= MTd
		CFLAGS += MDd
                QMAKE_LFLAGS_DEBUG += "/FORCE:MULTIPLE"


	}


}

#local sources
SOURCES += main.cpp
SOURCES += ntv2qtcudapreview.cpp
HEADERS += ntv2qtcudapreview.h

HEADERS += $$NTV2_DIR/democlasses/ntv2cudacapture.h
SOURCES += $$NTV2_DIR/democlasses/ntv2cudacapture.cpp

HEADERS += $$AJA_API/gpustuff/utility/cudacaptureviewer.h
SOURCES += $$AJA_API/gpustuff/utility/cudacaptureviewer.cpp

#resources
RESOURCES += ntv2qtcudapreview.qrc



win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../bin/x64/ -lajastuffdll_64
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../bin/x64/ -lajastuffdll_64d

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../bin/x64/ -lclassesDLL_64
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../bin/x64/ -lclassesDLL_64d

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../bin/x64/ -lcudaclassesDLL_64
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../bin/x64/ -lcudaclassesDLL_64d

