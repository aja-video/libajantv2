//------------------------------------
//  (c) Reliable Software, 1997
//  Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//------------------------------------
#include "ntv2active.h"

// The constructor of the derived class
// should call
//    _thread.Resume ();
// at the end of construction

ActiveObject::ActiveObject () 
	: _isDying (0),
	  _thread (ThreadEntry, this) { }

void ActiveObject::Kill ()
{
    _isDying++;
    FlushThread ();
    // Let's make sure it's gone
    _thread.WaitForDeath ();
}

void * ActiveObject::ThreadEntry (void* pArg)
{
    ActiveObject * pActive = (ActiveObject *) pArg;
    pActive->InitThread ();
    pActive->Loop ();
    return 0;
}
