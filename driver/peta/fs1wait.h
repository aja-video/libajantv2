/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
// Backported from 2.6 kernel so we can do wait_event_interruptible_timeout().

#ifndef _NTV2_LINUX_WAIT_H
#define _NTV2_LINUX_WAIT_H



#ifdef __KERNEL__

#include <linux/config.h>
#include <linux/list.h>
#include <linux/stddef.h>
#include <linux/spinlock.h>
#include <asm/system.h>

#define __wait_event_timeout(wq, condition, ret)                        \
do {                                                                    \
	wait_queue_t __wait;						\
	init_waitqueue_entry(&__wait, current);				\
									\
	add_wait_queue(&wq, &__wait);					\
        for (;;) {                                                      \
		set_current_state(TASK_UNINTERRUPTIBLE);		\
                if (condition)                                          \
                        break;                                          \
                ret = schedule_timeout(ret);                            \
                if (!ret)                                               \
                        break;                                          \
        }                                                               \
	current->state = TASK_RUNNING;					\
	remove_wait_queue(&wq, &__wait);				\
} while (0)

#define wait_event_timeout(wq, condition, timeout)                      \
({                                                                      \
        long __ret = timeout;                                           \
        if (!(condition))                                               \
                __wait_event_timeout(wq, condition, __ret);             \
        __ret;                                                          \
})


#define __wait_event_interruptible_timeout(wq, condition, ret)		\
do {									\
	wait_queue_t __wait;						\
	init_waitqueue_entry(&__wait, current);				\
									\
	add_wait_queue(&wq, &__wait);					\
	for (;;) {							\
		set_current_state(TASK_INTERRUPTIBLE);			\
		if (condition)						\
			break;						\
		if (!signal_pending(current)) {				\
			ret = schedule_timeout(ret);			\
			if (!ret)					\
				break;					\
			continue;					\
		}							\
		ret = -ERESTARTSYS;					\
		break;							\
	}								\
	current->state = TASK_RUNNING;					\
	remove_wait_queue(&wq, &__wait);				\
} while (0)

#define wait_event_interruptible_timeout(wq, condition, timeout)	\
({									\
	long __ret = timeout;						\
	if (!(condition))						\
		__wait_event_interruptible_timeout(wq, condition, __ret); \
	__ret;								\
})

#endif /* __KERNEL__ */

#endif
