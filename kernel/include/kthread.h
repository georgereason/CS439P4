/*
 * kthread.h
 *
 *  Created on: Apr 23, 2015
 *      Author: mwkurian
 */

#include <global_defs.h>
#ifndef KERNEL_INCLUDE_KTHREAD_H_
#define KERNEL_INCLUDE_KTHREAD_H_

typedef void (*kthread_callback_handler)();

typedef struct kthread_handle {
    uint32_t parent_pid;
    int niceness;
    int state;
    kthread_callback_handler cb_handler;
} kthread_handle;


kthread_handle* kthread_create(kthread_callback_handler cb_handler);
uint32_t kthread_start(kthread_handle * kthread);


#endif /* KERNEL_INCLUDE_KTHREAD_H_ */
