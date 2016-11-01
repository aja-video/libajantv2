#if !defined ACTIVE_H
#define ACTIVE_H
//------------------------------------
//  (c) Reliable Software, 1997
//	Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//------------------------------------

#include "ntv2thread.h"

#if defined (MSWindows)
#  include <windows.h>
#endif

class ActiveObject
{
public:
    ActiveObject ();
    virtual ~ActiveObject () {}
    void Kill ();
	void Resume( ) { _thread.Resume(); }
	void Suspend( ) { _thread.Suspend(); }
	DWORD WaitUntilFinished() { return _thread.WaitForDeath(); }
protected:
    virtual void InitThread () = 0;
    virtual void Loop () = 0;
    virtual void FlushThread () = 0;

    int             _isDying;
    static void * ThreadEntry ( void *pArg );
    Thread          _thread;
};

#endif
