#include "global_defs.h"
#include "scheduler.h"
#include "vm.h"
#include "klibc.h"
#include "process.h"
#include "kthread.h"
#include "interrupt.h"
#include "data_structures/linked_list.h"
#include "data_structures/hash_map.h"
#include "data_structures/array_list.h"
#include "drivers/timer.h"

#define MAX_TASKS 100   // in the future, cap will be removed
#define MAX_ACTIVE_TASKS 4  // in the future, will dynamically change based on load
#define MAX_NICENESS -20
#define MIN_NICENESS 20
#define TASK_STATE_FINISHED 3   // task has run by the scheduler and finished
#define TASK_STATE_INACTIVE  2  // task is in the wait queue (about to be executed)
#define TASK_STATE_ACTIVE 1     // task is part of the running tasks; it is being interleaved and executed atm
#define TASK_STATE_NONE 0       // task is just created (no real state)
#define SAFE_NICE(n) MAX(MIN(MAX_NICENESS, n), n)
#define KTHREAD 0
#define PROCESS 1

#define AS_PROCESS(a) ((pcb*) a->task)
#define AS_KTHREAD(a) ((kthread_handle*) a->task)
#define IS_PROCESS(a) (a->state == PROCESS)
#define IS_KTHREAD(a) (a->state == KTHREAD)

static char *last_err;
static prq_handle * inactive_tasks;
static prq_handle * active_tasks;
static sched_task * active_task;
static hmap_handle * all_tasks_map;
// NOTE
// scheduler logic only. not tested

// FIXME
// - add comments
// - register interrupt
// - deregister interrupts instead of flag
// - optimize
// - add error messages
// - remove a task (and children)
// - emitting and receiving messages!

// scheduler
// ---------
// round-robin timer-enabled scheduler that runs tasks in descending priority
// with the help of a heap-based priority queue

// supported syscalls
// ---------
// create: create a process (not execute)
// exec: start a process which you created before
// waitpid: wait for a process (i.e. child) to finish
// kill: kill a process and its children processes

void __sched_dispatch();
//
void __sched_register_timer_irq(void)
{
	// TODO: register the timer here
    interrupt_handler_t *handler = kmalloc(sizeof(interrupt_handler_t));
    handler->handler = __sched_dispatch;
    register_interrupt_handler(3, handler);
}


void __sched_deregister_timer_irq()
{
	// TODO: unregister the timer here
}

void __sched_pause_timer_irq()
{
	// TODO: suspend the timer here
}

void __sched_resume_timer_irq()
{
	// TODO: resume the timer here
}

// get the current process id
uint32_t sched_get_active_tid() {
    if (active_task) {
        return active_task->tid;
    }

    return (uint32_t) STATUS_FAIL;
}


sched_task* __sched_find_subtask(sched_task * parent_task, uint32_t pid);

// Initialize the scheduler. Should be called by the kernel ONLY
uint32_t sched_init() {
	vm_use_kernel_vas();

    os_printf("Initializing scheduler..................................................................................................\n");
    last_err = "No error";
    inactive_tasks = prq_create_fixed(MAX_TASKS);
    active_tasks = prq_create_fixed(MAX_ACTIVE_TASKS);
    all_tasks_map = hmap_create_fixed(MAX_TASKS);
    active_task = 0;

    __sched_register_timer_irq();

    return STATUS_OK;
}

uint32_t sched_register_callback_handler(sched_callback_handler cb_handler) {
    sched_task * task = hmap_get(all_tasks_map, sched_get_active_tid());
    task->cb_handler = cb_handler;
    return STATUS_OK;
}

uint32_t sched_deregister_callback_handler() {
    sched_task * task = hmap_get(all_tasks_map, sched_get_active_tid());
    task->cb_handler = 0;

    return STATUS_OK;
}

// Free the resources used the scheduler
uint32_t sched_free() {
	vm_use_kernel_vas();

    // FIXME kill active tasks

    __sched_deregister_timer_irq();

    prq_free(inactive_tasks);
    prq_free(active_tasks);

    return STATUS_OK;
}

static uint32_t sched_tid = 0;

// issue messages for the active task
void __sched_emit_messages(void) {
    if (active_task->cb_handler) {
        sched_message_chunk * chunk;
        while ((chunk = llist_dequeue(active_task->message_queue)) != 0) {
            active_task->cb_handler(chunk->src_pid, chunk->event, chunk->data,
                    chunk->chunk_length, chunk->remain_length);
            if (chunk) {
                kfree(chunk);
            }
        }
    }
}

sched_task* __sched_create_task(void * task_data, int niceness, uint32_t type) {
    if (prq_count(inactive_tasks) >= MAX_TASKS) {
        last_err = "Too many tasks";
        return (sched_task*) STATUS_FAIL;
    }

    __sched_pause_timer_irq();

    vm_use_kernel_vas();
    sched_task * task = (sched_task*) kmalloc(sizeof(sched_task));
    niceness = SAFE_NICE(niceness);
    task->tid = ++sched_tid;
    task->niceness = niceness;
    task->task = task_data;
    task->type = type;
    task->state = TASK_STATE_NONE;
    task->node = 0;
    task->parent_tid = 0;
    task->children_tids = arrl_create();
    task->cb_handler = 0;

    if (active_task) {
        task->parent_tid = active_task->tid;
        arrl_append(active_task->children_tids, (void*) task->tid);
    }

    __sched_resume_timer_irq();

    return task;

}

sched_task* sched_create_task_from_kthread(kthread_handle * kthread,
        int niceness) {
    //sched_task * task = __sched_create_task(kthread, niceness, KTHREAD);
    return __sched_create_task(kthread, niceness, KTHREAD);
}

sched_task* sched_create_task_from_process(pcb * pcb_pointer, int niceness) {
    return __sched_create_task(pcb_pointer, niceness, PROCESS);
}

sched_task* sched_create_task(uint32_t * task, int niceness) {
    int type = ((pcb *) task)->type;
    os_printf("Task Type: %d\n", type);
    if(type == 1){
        return sched_create_task_from_kthread((kthread_handle *) task, niceness);
    }else if(type == 2){
        return sched_create_task_from_process((pcb *) task, niceness);
    }else{
        os_printf("Error: bad type in sched_create_task\n");
        return (sched_task *)-1;
    }
}


// Helper function used by the scheduler internally. It will traverse the parent/child
// list to find a subtask.
#if 0
sched_task* __sched_find_subtask(sched_task * parent_task, uint32_t tid) {
    if (parent_task && parent_task->tid == tid) {
        return parent_task;
    }

    int i = 0;
    for (; i < arrl_count(parent_task->children_tids); i++) {
        uint32_t child_tid = arrl_count(parent_task, i);
        sched_task * child_task = hmap_get(all_tasks_map, child_tid);
        if ((child_task = __sched_find_subtask(child_task, tid))) {
            return child_task;
        }
    }

    return 0;
}
#endif

//
// NOTE expecting access to kernel global vars
void sched_waittid(uint32_t tid) {
	// FIXME: broken!
	while (1) {
        sched_task * task = (sched_task*) hmap_get(all_tasks_map, (unsigned long) tid);
        if (task == 0 || task->state == TASK_STATE_FINISHED) {
            break;
        }

        //sleep(500);
    }
}

// contract
// --------
// must disable timer_interrupt
uint32_t __sched_remove_task(sched_task * task) {
    if (task == NULL) {
        return (uint32_t) STATUS_FAIL;
    }

    switch (task->state) {
        case TASK_STATE_INACTIVE: {
            __sched_pause_timer_irq();
            prq_remove(inactive_tasks, task->node);
            prq_free_node(task->node);
            __sched_resume_timer_irq();
            break;
        }
        case TASK_STATE_ACTIVE: {
            __sched_pause_timer_irq();

            task->state = TASK_STATE_FINISHED;

            if (IS_PROCESS(task)) {
                vm_use_kernel_vas();
                free_PCB(AS_PROCESS(task));
            } else if (IS_KTHREAD(task)) {
                // FIXME add later
            }

            hmap_remove(all_tasks_map, task->tid);
            prq_remove(active_tasks, task->node);
            prq_free_node(task->node);

            int i = 0;
            for (; i < arrl_count(task->children_tids); i++) {
                // FIXME: this API does not exist anymore
            	// sched_remove_task(arrl_get(task->children_tids, i));
            }

            __sched_resume_timer_irq();
            break;
        }
        case TASK_STATE_FINISHED: {
            // ignore
            break;
        }
    }

    __sched_resume_timer_irq();

    return task->state;
}


// essentially a kill process
uint32_t sched_remove_task(uint32_t tid) {
    sched_task * task = (sched_task*) hmap_get(all_tasks_map, tid);
    if (task) {
        __sched_pause_timer_irq();
        uint32_t status = __sched_remove_task(task);
        __sched_resume_timer_irq();
        return status;
    }

    return STATUS_FAIL;
}

void __sched_dispatch(void) {
    os_printf("In dispatch!!........\n");
    // prevent interrupts while handling another interrupt
    __sched_pause_timer_irq();

    // use the kernel memory
    vm_use_kernel_vas();

    if (prq_count(active_tasks) < MAX_ACTIVE_TASKS) {
        if (prq_count(inactive_tasks) > 0) {
            prq_enqueue(active_tasks, prq_dequeue(inactive_tasks)); // add to active_tasks if the task
        }
    }

    if (prq_count(active_tasks) == 0) {
        __sched_resume_timer_irq();
        return;
    }

    sched_task * last_task;

    // check if there is active task
    if (active_task) {
        last_task = active_task;
    } else {
        prq_node * node = prq_peek(active_tasks);
        last_task = (sched_task*) node->data;
    }

    os_printf("Scheduling something.................................................................................................\n");  
    switch (last_task->state) {
        case TASK_STATE_INACTIVE: {
            active_task = last_task;
            active_task->state = TASK_STATE_ACTIVE;
            os_printf("Scheduling Inactive.................................................................................................\n");  
            if (IS_PROCESS(active_task)) {
                os_printf("Scheduling inactive process.................................................................................................\n");  
                vm_enable_vas(AS_PROCESS(active_task)->stored_vas);
                __sched_resume_timer_irq();
                execute_process(AS_PROCESS(active_task));
            } else if (IS_KTHREAD(active_task)) {
                os_printf("Scheduling inactive thread.................................................................................................\n");  
                AS_KTHREAD(active_task)->cb_handler();
                }
            os_printf("Heree..............................................\n");
            __sched_pause_timer_irq();
            sched_remove_task(active_task->tid);
            active_task = 0;

            // NOTE next interrupt will get the start the process

            break;
        }
        case TASK_STATE_ACTIVE: {
            if (prq_count(active_tasks) > 1) {
                
                prq_remove(active_tasks, active_task->node);
                prq_enqueue(active_tasks, active_task->node);
                sched_task * next_task = (sched_task*) prq_peek(active_tasks)->data;
                os_printf("Scheduling active.................................................................................................\n");  
                // old task
                if (IS_PROCESS(active_task)) {
                    os_printf("Scheduling active process.................................................................................................\n");  
                    if (active_task == next_task) {
                        vm_enable_vas(AS_PROCESS(active_task)->stored_vas);
                        break;
                    }

                    save_process_state(AS_PROCESS(last_task)->PID);
                } else if (IS_KTHREAD(active_task)) {
                    os_printf("Scheduling active thread.................................................................................................\n");  
                    if (active_task == next_task) {
                        
                        break;
                    }

                    // FIXME: implement
                    // kthread_save_state(AS_KTHREAD(active_task));
                }

                active_task = next_task;

                // new task
                if (AS_PROCESS(active_task)->type != 1){//is process
                    vm_enable_vas(AS_PROCESS(active_task)->stored_vas);
                    __sched_emit_messages();
                    os_printf("Loading new process.................................................................................................\n");  
                    load_process_state(AS_PROCESS(active_task)->PID); // continue with the next process
                } else if (AS_PROCESS(active_task)->type == 1) { //is thread
                    __sched_emit_messages();
                    os_printf("Loading new thread.................................................................................................\n");  
                    // FIXME: implement
                    kthread_load_state(AS_KTHREAD(active_task));
                }
            }
            break;
        }
    }

    __sched_resume_timer_irq();
}

// start process
uint32_t sched_add_task(sched_task * task) {
    vm_use_kernel_vas();    
    if (task) {
        if (task->state != TASK_STATE_NONE) {
            last_err = "Reusing task object not allowed";
            return (uint32_t) STATUS_FAIL;
        }

        // use the kernel memory

        prq_node * new_node = (prq_node*) kmalloc(sizeof(prq_node));
        new_node->data = task;
        new_node->priority = task->niceness;

        task->state = TASK_STATE_INACTIVE;
        task->node = new_node;
        prq_enqueue(inactive_tasks, new_node);

            os_printf("Adding task..............................................................\n");
        hmap_put(all_tasks_map, active_task->tid, active_task);
            os_printf("Adding task..............................................................\n");

        if (IS_PROCESS(active_task)) {
            vm_enable_vas(AS_PROCESS(active_task)->stored_vas);
        } else if (IS_KTHREAD(active_task)) {
            // ignore
        }
    
        //Schedule dispatch if no other task is there
        if ((prq_count(active_tasks) == 0) && (prq_count(inactive_tasks) == 1)) { 
            __sched_dispatch();
        }

        return active_task->tid;
    }

    last_err = "Invalid sched_task pointer";
    return (uint32_t) STATUS_FAIL;
}


const char * sched_last_err() {
    return last_err;
}

uint32_t sched_set_niceness(uint32_t pid, uint32_t niceness) {

    sched_task * task;

    if (!(task = hmap_get(all_tasks_map, pid))) {
        return STATUS_FAIL;
    }

    __sched_pause_timer_irq();

    prq_handle * tasks;

    switch (task->state) {
        case TASK_STATE_ACTIVE:
            tasks = active_tasks;
            break;
        case TASK_STATE_INACTIVE:
            tasks = inactive_tasks;
            break;
        case TASK_STATE_FINISHED:
            break;
    }

    if (tasks) {
        prq_remove(tasks, task->node); // remove the running task from queue
        task->node->priority = SAFE_NICE(niceness);
        prq_enqueue(tasks, task->node); // add it again to see if its position in the queue

    }

    __sched_resume_timer_irq();

    return STATUS_OK;
}

uint32_t sched_post_message(uint32_t dest_pid, uint32_t event, char * data,
        int len)
{
    /*sched_task * dest_task;

     if (!(dest_task = hmap_get(&all_tasks_map, dest_pid))) {
     return STATUS_FAIL;
     }
     __sched_pause_timer_irq();

     vm_use_kernel_vas();

     // create space on kernel stack
     char data_cpy[512];
     while (len > 0) {
     // get length to cpy
     int cpy_len = MIN(len, 512);
     // enable orignal struct
     vm_enable_vas(active_task->pcb->stored_vas);
     // copy to stack
     os_memcpy(data, data_cpy, cpy_len);
     // remaining bytes
     len -= cpy_len;
     // increment data pointer
     data = (char*) (data + cpy_len / 4);
     // get the dest_task
     sched_task * dest_task = hmap_get(&all_tasks_map, dest_pid);
     // switch to dest vas
     vm_enable_vas(dest_task->pcb->stored_vas);
     // copy from global kernel stack to process heap
     char* task_mem_data_cpy = kmalloc(cpy_len);
     os_memcpy(data_cpy, task_mem_data_cpy, cpy_len);

     vm_use_kernel_vas();

     // FIXME create the message object and pass it in
     // messages will be broken up into chunks since we have to
     // copy into kernel stack first.
     sched_message_chunk * chunk = kmalloc(sizeof(sched_message_chunk));
     chunk->data = task_mem_data_cpy;
     chunk->chunk_length = cpy_len;
     chunk->remain_length = len;
     chunk->event = event;
     chunk->src_pid = active_task ? active_task->pcb->PID : 0;

     llist_enqueue(dest_task->message_queue, chunk);
     }

     if (active_task) {
     vm_enable_vas(active_task->pcb->stored_vas);
     }

     __sched_resume_timer_irq();*/

    return STATUS_OK;
}
