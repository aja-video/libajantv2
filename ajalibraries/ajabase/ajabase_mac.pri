# common
SOURCES += $$NTV4_DIR/ajabase/common/buffer.cpp
HEADERS += $$NTV4_DIR/ajabase/common/buffer.h
HEADERS += $$NTV4_DIR/ajabase/common/common.h
HEADERS += $$NTV4_DIR/ajabase/common/dpx_hdr.h
SOURCES += $$NTV4_DIR/ajabase/common/dpx_hdr.cpp
HEADERS += $$NTV4_DIR/ajabase/common/export.h
SOURCES += $$NTV4_DIR/ajabase/common/guid.cpp
HEADERS += $$NTV4_DIR/ajabase/common/guid.h
SOURCES += $$NTV4_DIR/ajabase/common/pixelformat.cpp
HEADERS += $$NTV4_DIR/ajabase/common/pixelformat.h
HEADERS += $$NTV4_DIR/ajabase/common/public.h
SOURCES += $$NTV4_DIR/ajabase/common/testpatterngen.cpp
HEADERS += $$NTV4_DIR/ajabase/common/testpatterngen.h
SOURCES += $$NTV4_DIR/ajabase/common/timebase.cpp
HEADERS += $$NTV4_DIR/ajabase/common/timebase.h
SOURCES += $$NTV4_DIR/ajabase/common/timecode.cpp
HEADERS += $$NTV4_DIR/ajabase/common/timecode.h
SOURCES += $$NTV4_DIR/ajabase/common/timer.cpp
HEADERS += $$NTV4_DIR/ajabase/common/timer.h
HEADERS += $$NTV4_DIR/ajabase/common/types.h
HEADERS += $$NTV4_DIR/ajabase/common/videotypes.h
SOURCES += $$NTV4_DIR/ajabase/common/videoutilities.cpp
HEADERS += $$NTV4_DIR/ajabase/common/videoutilities.h
HEADERS += $$NTV4_DIR/ajabase/common/wavewriter.h
SOURCES += $$NTV4_DIR/ajabase/common/wavewriter.cpp

# persistence
SOURCES += $$NTV4_DIR/ajabase/persistence/persistence.cpp
HEADERS += $$NTV4_DIR/ajabase/persistence/persistence.h
SOURCES += $$NTV4_DIR/ajabase/persistence/sqlite3.c
HEADERS += $$NTV4_DIR/ajabase/persistence/sqlite3.h

# system
SOURCES += $$NTV4_DIR/ajabase/system/atomic.cpp
HEADERS += $$NTV4_DIR/ajabase/system/atomic.h
SOURCES += $$NTV4_DIR/ajabase/system/debug.cpp
HEADERS += $$NTV4_DIR/ajabase/system/debug.h
HEADERS += $$NTV4_DIR/ajabase/system/debugshare.h
SOURCES += $$NTV4_DIR/ajabase/system/diskstatus.cpp
HEADERS += $$NTV4_DIR/ajabase/system/diskstatus.h
SOURCES += $$NTV4_DIR/ajabase/system/event.cpp
HEADERS += $$NTV4_DIR/ajabase/system/event.h
SOURCES += $$NTV4_DIR/ajabase/system/lock.cpp
HEADERS += $$NTV4_DIR/ajabase/system/lock.h
SOURCES += $$NTV4_DIR/ajabase/system/log.cpp
HEADERS += $$NTV4_DIR/ajabase/system/log.h
SOURCES += $$NTV4_DIR/ajabase/system/memory.cpp
HEADERS += $$NTV4_DIR/ajabase/system/memory.h
SOURCES += $$NTV4_DIR/ajabase/system/process.cpp
HEADERS += $$NTV4_DIR/ajabase/system/process.h
HEADERS += $$NTV4_DIR/ajabase/system/system.h
SOURCES += $$NTV4_DIR/ajabase/system/systemtime.cpp
HEADERS += $$NTV4_DIR/ajabase/system/systemtime.h
SOURCES += $$NTV4_DIR/ajabase/system/thread.cpp
HEADERS += $$NTV4_DIR/ajabase/system/thread.h

# mac : systemimpl
macx:SOURCES += $$NTV4_DIR/ajabase/system/mac/eventimpl.cpp
macx:HEADERS += $$NTV4_DIR/ajabase/system/mac/eventimpl.h
macx:SOURCES += $$NTV4_DIR/ajabase/system/mac/lockimpl.cpp
macx:HEADERS += $$NTV4_DIR/ajabase/system/mac/lockimpl.h
macx:SOURCES += $$NTV4_DIR/ajabase/system/mac/processimpl.cpp
macx:HEADERS += $$NTV4_DIR/ajabase/system/mac/processimpl.h
macx:SOURCES += $$NTV4_DIR/ajabase/system/mac/pthreadsextra.cpp
macx:HEADERS += $$NTV4_DIR/ajabase/system/mac/pthreadsextra.h
macx:SOURCES += $$NTV4_DIR/ajabase/system/mac/threadimpl.cpp
macx:HEADERS += $$NTV4_DIR/ajabase/system/mac/threadimpl.h
macx:SOURCES += $$NTV4_DIR/ajabase/system/mac/file_io.cpp

# pnp
SOURCES += $$NTV4_DIR/ajabase/pnp/pnp.cpp
HEADERS += $$NTV4_DIR/ajabase/pnp/pnp.h


# mac : pnpimpl (not ready for prime time, these files are heavily dependent on NTV2 headers)
macx:SOURCES += $$NTV4_DIR/ajabase/pnp/mac/pnpimpl.cpp
macx:HEADERS += $$NTV4_DIR/ajabase/pnp/mac/pnpimpl.h


