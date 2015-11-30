/*
 * kthreads.c
 *
 *  Created on: Apr 23, 2015
 *      Author: mwkurian
 */

#include "kthread.h"
#include "klibc.h"
#include "scheduler.h"
#include <global_defs.h>

kthread_handle* kthread_create(kthread_callback_handler cb_handler)
{
	os_printf("Inside KThread Create\n");
	kthread_handle * kthread = kmalloc(sizeof(kthread_handle));
	kthread->cb_handler = cb_handler;
	return (kthread_handle *)-1;
}

uint32_t kthread_start(kthread_handle * kthread)
{
//	sched_task * task = sched_create_task(kthread);
//	sched_add_task(task);
	return (uint32_t) 1;
}

