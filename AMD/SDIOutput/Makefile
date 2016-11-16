#!make

.SUFFIXES: .cpp .h

NTV2_ROOT ?= $(shell pwd)/../../..

SRCDIR = ./
OBJDIR = ./obj
BINDIR = $(NTV2_ROOT)/../bin

CXX = g++
CXXFLAGS = -O2 -I./ -I. -I../common -I../SDICommon -I/usr/X11R6/include -I$(NTV2_ROOT)/includes -I$(NTV2_ROOT)/classes -DGLEW_STATIC -DAJALinux -DFS1 -DLINUX

CC = gcc
CFLAGS = $(CXXFLAGS)

LD_FLAGS = -L/usr/X11R6/lib -L$(NTV2_ROOT)/../lib

LIBS = -lGLU -lGL -laja -lpthread
OBJECTS_DIR   = ./


TARGET  = 	$(BINDIR)/SDIOutput


OBJECTS       = $(OBJDIR)/AJA_SDIInOut.o	\
				$(OBJDIR)/glew.o 		\
				$(OBJDIR)/GLTransferBuffers.o 	\
				$(OBJDIR)/GLSource.o 		\
				$(OBJDIR)/RenderTarget.o 	\
				$(OBJDIR)/SyncedBuffer.o 	\
                $(OBJDIR)/Thread.o 		\
				$(OBJDIR)/FormatInfo.o 		\
				$(OBJDIR)/LnxMain.o

all: 
	if [ ! -d ./$(OBJDIR) ] ; then mkdir ./$(OBJDIR); fi
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR); fi
	make $(TARGET)    


$(TARGET): $(OBJECTS)
	$(CXX) -o $@  $(LD_FLAGS) $(OBJECTS) $(LIBS)

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<



$(OBJDIR)/glew.o: ../common/glew.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ ../common/glew.c

$(OBJDIR)/AJA_SDIInOut.o: ../SDICommon/AJA_SDIInOut.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ ../SDICommon/AJA_SDIInOut.cpp


$(OBJDIR)/GLTransferBuffers.o: ../SDICommon/GLTransferBuffers.cpp .
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ ../SDICommon/GLTransferBuffers.cpp

$(OBJDIR)/FormatInfo.o: ../common/FormatInfo.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ ../common/FormatInfo.cpp


$(OBJDIR)/SyncedBuffer.o: ../common/SyncedBuffer.cpp .
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@  ../common/SyncedBuffer.cpp

$(OBJDIR)/Thread.o: ../common/Thread.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ ../common/Thread.cpp

$(OBJDIR)/GLSource.o: GLSource.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ GLSource.cpp

$(OBJDIR)/RenderTarget.o: ../common/RenderTarget.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ ../common/RenderTarget.cpp

$(OBJDIR)/LnxMain.o: LnxMain.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ LnxMain.cpp


clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(OBJDIR)/$(TARGET)
	rm -f $(TARGET)

