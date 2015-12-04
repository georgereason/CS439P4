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
	int type;
    uint32_t parent_pid;
    int niceness;
    int state;
    kthread_callback_handler cb_handler;
	uint32_t R0;
	uint32_t R1;
	uint32_t R2;
	uint32_t R3;
	uint32_t R4;
	uint32_t R5;
	uint32_t R6;
	uint32_t R7;

	//banked registers
	uint32_t R8;
	uint32_t R9;
	uint32_t R10;
	uint32_t R11;
	uint32_t R12;
	uint32_t R13; //corresponds to the SP; do we need both?
	uint32_t R14;
	uint32_t R15; //corresponds to the PC; do we need both?
	
} kthread_handle;


kthread_handle* kthread_create(kthread_callback_handler cb_handler, void* args);
uint32_t kthread_start(kthread_handle * kthread);
void kthread_load_state(kthread_handle * kthread);



#endif /* KERNEL_INCLUDE_KTHREAD_H_ */
