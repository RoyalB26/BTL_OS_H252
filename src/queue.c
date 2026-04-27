#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: put a new process to queue [q] */
        if (q == nullptr || proc == nullptr) return;
        // Kiểm tra xem hàng đợi đã đầy chưa
        if (q->size >= MAX_QUEUE_SIZE) return;

        // Thêm tiến trình vào cuối hàng và tăng kích thước hàng đợi
        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (empty(q)) return NULL;
        int highestPR_idx = 0;

        // Tìm vị trí của tiến trình có độ ưu tiên cao nhất (giá trị nhỏ nhất)
        for (int i = 1; i < q->size; i++) {
#ifdef MLQ_SCHED
                if (q->proc[i]->prio < q->proc[highestPR_idx]->prio)
#else
                if (q->proc[i]->priority < q->proc[highestPR_idx]->priority)
#endif
                {
                        highestPR_idx = i;
                }
        }

        // Lưu lại tiến trình cần lấy ra
        struct pcb_t *return_proc = q->proc[highestPR_idx];

        // Dịch các phần tử phía sau sang trái để lấp đầy khoảng trống
        for (int i = highestPR_idx; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }

        q->size--; // Giảm kích thước hàng đợi
        return return_proc;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: remove a specific item from queue
         * */
        if (empty(q) || proc == NULL) return NULL;

        for (int i = 0; i < q->size; i++) {
                // Kiểm tra trùng khớp dựa trên memory address hoặc PID
                if (q->proc[i] == proc || q->proc[i]->pid == proc->pid) {
                    struct pcb_t *return_proc = q->proc[i];

                    // Dịch các phần tử phía sau sang trái
                    for (int j = i; j < q->size - 1; j++) {
                            q->proc[j] = q->proc[j + 1];
                    }

                    q->size--;
                    return return_proc;
                }
        }

        return NULL; // Không tìm thấy
}
