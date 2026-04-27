/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

static struct queue_t running_list;
#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
static int slot[MAX_PRIO];
#endif

int queue_empty(void) {
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if(!empty(&mlq_ready_queue[prio]))
			return -1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++) {
		mlq_ready_queue[i].size = 0;
		slot[i] = MAX_PRIO - i;
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	running_list.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/*
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t * get_mlq_proc(void) {
	struct pcb_t * proc = NULL;

	pthread_mutex_lock(&queue_lock);
	/*TODO: get a process from PRIORITY [ready_queue].
	 *      It worth to protect by a mechanism.
	 * */
    static int current_prio = 0; // Biến static để lưu trạng thái hàng đợi đang được phục vụ
    // Kiểm tra xem có tiến trình nào trong tất cả các hàng đợi không để tránh vòng lặp vô tận
	int is_all_empty = 1;
	for (int i = 0; i < MAX_PRIO; i++) {
		if (!empty(&mlq_ready_queue[i])) {
			is_all_empty = 0;
			break;
		}
	}

	if (!is_all_empty) {
		while (1) {
			if (!empty(&mlq_ready_queue[current_prio])) {
				if (slot[current_prio] > 0) {
					// Lấy tiến trình ra và giảm số slot
					proc = dequeue(&mlq_ready_queue[current_prio]);
					slot[current_prio]--;
					break;
				} else {
					// Hết slot, nạp lại slot cho hàng đợi này theo công thức
					slot[current_prio] = MAX_PRIO - current_prio;
					// Chuyển sang hàng đợi kế tiếp
				}
			}
			// Nếu hàng đợi rỗng hoặc vừa dùng hết slot, xoay vòng sang mức ưu tiên tiếp theo
			current_prio = (current_prio + 1) % MAX_PRIO;
		}
	}

	if (proc != NULL)
		enqueue(&running_list, proc);

    pthread_mutex_unlock(&queue_lock)
	return proc;
}

void put_mlq_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->mlq_ready_queue = mlq_ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list
	 *       It worth to protect by a mechanism.
	 *
	 */
    pthread_mutex_lock(&queue_lock);

    // Lấy tiến trình ra khỏi danh sách đang chạy
	purgequeue(&running_list, proc);

	// Đưa tiến trình trở lại hàng đợi sẵn sàng tương ứng với mức ưu tiên
	enqueue(&mlq_ready_queue[proc->prio], proc);

	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->mlq_ready_queue = mlq_ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list
	 *       It worth to protect by a mechanism.
	 *
	 */

	pthread_mutex_lock(&queue_lock);

	// Lấy tiến trình ra khỏi danh sách đang chạy
	purgequeue(&running_list, proc);

	// Đưa tiến trình trở lại hàng đợi sẵn sàng tương ứng với mức ưu tiên
	enqueue(&mlq_ready_queue[proc->prio], proc);

	pthread_mutex_unlock(&queue_lock);
}

struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	return add_mlq_proc(proc);
}
#else
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;

	pthread_mutex_lock(&queue_lock);
	/*TODO: get a process from [ready_queue].
	 *       It worth to protect by a mechanism.
	 *
	 */
    if (!empty(&ready_queue)) {
		proc = dequeue(&ready_queue);
//	} else if (!empty(&run_queue)) { // Xử lý cho run_queue (mặc dù obsolete)
//		proc = dequeue(&run_queue);
	}

	if (proc != NULL) {
		enqueue(&running_list, proc);
	}

	pthread_mutex_unlock(&queue_lock);

	return proc;
}

void put_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list
	 *       It worth to protect by a mechanism.
	 *
	 */

	pthread_mutex_lock(&queue_lock);

	purgequeue(&running_list, proc); // Lấy tiến trình ra khỏi danh sách đang chạy

	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list
	 *       It worth to protect by a mechanism.
	 *
	 */

	pthread_mutex_lock(&queue_lock);

	purgequeue(&running_list, proc); // Lấy tiến trình ra khỏi danh sách đang chạy

	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}
#endif


