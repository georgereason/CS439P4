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

kthread_handle* kthread_create(kthread_callback_handler cb_handler, void* args)
{
	vm_use_kernel_vas();
	kthread_handle * kthread = kmalloc(sizeof(kthread_handle));
	kthread->cb_handler = cb_handler;
	os_printf("CB Handler: %x\n", (uint32_t) cb_handler);
	kthread->type = 1;
    kthread->parent_pid = sched_get_active_pid();
    pcb* pcb_p = get_PCB(kthread->parent_pid);

	kthread->R0 = pcb_p->R0;
	kthread->R1 = pcb_p->R1;
	kthread->R2 = pcb_p->R2;
	kthread->R3 = pcb_p->R3;
	kthread->R4 = pcb_p->R4;
	kthread->R5 = pcb_p->R5;
	kthread->R6 = pcb_p->R6;
	kthread->R7 = pcb_p->R7;
	kthread->R8 = pcb_p->R8;
	kthread->R9 = pcb_p->R9;
	kthread->R10 = pcb_p->R10;
	kthread->R11 = pcb_p->R11;
	kthread->R12 = pcb_p->R12;
	kthread->R13 = (uint32_t) kmalloc(sizeof(int) * 42);
	kthread->R14 = pcb_p->R14;
	kthread->R15 = (uint32_t) kthread->cb_handler;

	//Put arguments on the stack here
	printf("THREAD CREATE ARG VALUE %d\n", (uint32_t)args);
	kthread->R13 = (uint32_t) args;

	kthread_start(kthread);
	return kthread;
}

uint32_t kthread_start(kthread_handle * kthread)
{
	sched_task * task = sched_create_task((uint32_t *) kthread, 0);
	return sched_add_task(task);
}

void kthread_load_state(kthread_handle * kthread) {
	os_printf("Switching To Thread\n");
	os_printf("Parent ID: %d\n", kthread->parent_pid);
	
	//LOAD THE THREAD STATE HERE TO SWITCH TO ITuint32_t* process_to_load = get_address_of_PCB(PID);
	pcb* pcb_p = get_PCB(kthread->parent_pid);

	if (pcb_p == 0)
	{
		os_printf("Invalid PID in kthread_load_state");
		return;// 0;
	}
	//while(1);
	asm("MOV r0, %0"::"r"(kthread->R0):);
	asm("MOV r1, %0"::"r"(kthread->R1):);
	asm("MOV r2, %0"::"r"(kthread->R2):);
	asm("MOV r3, %0"::"r"(kthread->R3):);
	asm("MOV r4, %0"::"r"(kthread->R4):);
	asm("MOV r5, %0"::"r"(kthread->R5):);
	asm("MOV r6, %0"::"r"(kthread->R6):);
	asm("MOV r7, %0"::"r"(kthread->R7):);
	asm("MOV r8, %0"::"r"(kthread->R8):);
	asm("MOV r9, %0"::"r"(kthread->R9):);
	asm("MOV r10, %0"::"r"(kthread->R10):);
	//asm("MOV r11, %0"::"r"(11):);
	asm("MOV r12, %0"::"r"(kthread->R12):);

	asm("MOV r13, %0"::"r"(kthread->R13):);

	asm("MOV r14, %0"::"r"(kthread->R14):);


	asm("MOV r15, %0"::"r"(kthread->R15):);

	//return 1;

}

