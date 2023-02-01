/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA boards.
//
// Filename:	ntv2driverdbgmsgctl.c
// Purpose:		Dynamic control of debug messages
// Notes:
//
///////////////////////////////////////////////////////////////

#if defined(CONFIG_SMP)
#define __SMP__
#endif

/*needed by kernel 2.6.18*/
#ifndef CONFIG_HZ
#include <linux/autoconf.h>
#endif

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/jiffies.h>
#include <linux/sched.h>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2audiodefines.h"
#include "ntv2videodefines.h"
#include "ntv2publicinterface.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2linuxpublicinterface.h"

#include "driverdbg.h"
#include "ntv2driverdbgmsgctl.h"


/***************************/
/* Local defines and types */
/***************************/

/*********************************************/
/* Prototypes for private utility functions. */
/*********************************************/

/********************/
/* Static variables */
/********************/

static bool msgEnable[NTV2_DRIVER_NUM_DEBUG_MESSAGE_SETS];

/*************/
/* Functions */
/*************/

bool
MsgsEnabled(NTV2_DriverDebugMessageSet msgSet)
{
   if ( 	msgSet == NTV2_DRIVER_ALL_DEBUG_MESSAGES 
		 || msgSet >= NTV2_DRIVER_NUM_DEBUG_MESSAGE_SETS) 
	{
		return 0;
	}
	
   return msgEnable[msgSet];
}

LWord 
ControlDebugMessages(	NTV2_DriverDebugMessageSet msgSet, 
	  			   		bool enable)
{
   int i;

	MSG("%sabled message set %d\n",
		  	enable ? "En" : "Dis", msgSet);

   // Enable/disable all messages
   if (msgSet == NTV2_DRIVER_ALL_DEBUG_MESSAGES)
   {
	  for (i = 0; i < NTV2_DRIVER_NUM_DEBUG_MESSAGE_SETS; i++)
	  {
		 msgEnable[i] = enable;
	  }
	  return 0;
   }

   // Validate input
   if (msgSet >= NTV2_DRIVER_NUM_DEBUG_MESSAGE_SETS) 
   {
 		if (MsgsEnabled(NTV2_DRIVER_DEBUG_DEBUG_MESSAGES))
		   MSG("ControlDebugMessages(): Recieved unknown DebugMessageSet %d\n", msgSet); 
		return -EINVAL;
   }

   // enable/disable a single class of messages
   msgEnable[msgSet] = enable;
   
   return 0;
}

void 
ShowDebugMessageControl(NTV2_DriverDebugMessageSet msgSet)
{
   int i;

   // Show all message enables
   if (msgSet == NTV2_DRIVER_ALL_DEBUG_MESSAGES)
   {
	  for (i = 0; i < NTV2_DRIVER_NUM_DEBUG_MESSAGE_SETS; i++)
	  {
		 MSG("msgEnable[%d] = %d\n", i, msgEnable[i]);
	  }
	  return;
   }

   // Validate input
   if (msgSet >= NTV2_DRIVER_NUM_DEBUG_MESSAGE_SETS) 
   {
	  printk("No message set %d exists.\n", msgSet);
	  return;
   }

	// Show a single message enable
	MSG("msgEnable[%d] = %d\n", msgSet, msgEnable[msgSet]);
}



