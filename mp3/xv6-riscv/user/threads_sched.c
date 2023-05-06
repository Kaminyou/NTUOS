#include "kernel/types.h"
#include "user/user.h"
#include "user/list.h"
#include "user/threads.h"
#include "user/threads_sched.h"

#define NULL 0

int min(int a, int b) {
    return ((a) < (b) ? (a) : (b));
}

/* default scheduling algorithm */
struct threads_sched_result schedule_default(struct threads_sched_args args)
{
    printf("time: %d\n", args.current_time);
    struct thread *thread_with_smallest_id = NULL;
    struct thread *th = NULL;
    struct release_queue_entry *rqe = NULL;
    list_for_each_entry(th, args.run_queue, thread_list) {
        if (thread_with_smallest_id == NULL || th->ID < thread_with_smallest_id->ID) {
            thread_with_smallest_id = th;
        }
    }
    list_for_each_entry(rqe, args.release_queue, thread_list) {
        printf("thread %d, release time: %d, deadline: %d\n", rqe->thrd->ID, rqe->release_time, rqe->thrd->current_deadline);
    }

    struct threads_sched_result r;
    if (thread_with_smallest_id != NULL) {
        r.scheduled_thread_list_member = &thread_with_smallest_id->thread_list;
        r.allocated_time = thread_with_smallest_id->remaining_time;
    } else {
        r.scheduled_thread_list_member = args.run_queue;
        r.allocated_time = 1;
    }

    return r;
}

/* Earliest-Deadline-First scheduling */
struct threads_sched_result schedule_edf(struct threads_sched_args args)
{
    // check miss deadline first
    struct thread *thread_with_smallest_id_miss_deadline = NULL;
    struct thread *th_miss_deadline = NULL;
    list_for_each_entry(th_miss_deadline, args.run_queue, thread_list) {
        if (th_miss_deadline->current_deadline <= args.current_time) {
            if (thread_with_smallest_id_miss_deadline == NULL) {
                thread_with_smallest_id_miss_deadline = th_miss_deadline;
            }
            if (thread_with_smallest_id_miss_deadline != NULL && th_miss_deadline->ID < thread_with_smallest_id_miss_deadline->ID) {
                thread_with_smallest_id_miss_deadline = th_miss_deadline;
            }
        }
    }
    if (thread_with_smallest_id_miss_deadline != NULL) {
        struct threads_sched_result r;
        r.scheduled_thread_list_member = &thread_with_smallest_id_miss_deadline->thread_list;
        r.allocated_time = 0;
        return r;
    }

    struct thread *thread_with_smallest_id = NULL;
    struct thread *th = NULL;
    int earliest_deadline = 2147483647;
    list_for_each_entry(th, args.run_queue, thread_list) {
        if (thread_with_smallest_id != NULL && th->current_deadline == earliest_deadline) {
            if (th->ID < thread_with_smallest_id->ID) {
                thread_with_smallest_id = th;
            }
        }
        if (thread_with_smallest_id == NULL || th->current_deadline < earliest_deadline) {
            earliest_deadline = th->current_deadline;
            thread_with_smallest_id = th;
        }
    }
    
    struct release_queue_entry *rqe = NULL;
    int preempt_start_time = 2147483647;
    int allocate_time;
    if (thread_with_smallest_id != NULL) {
        list_for_each_entry(rqe, args.release_queue, thread_list) {
            if (rqe->release_time + rqe->thrd->period == earliest_deadline && rqe->thrd->ID < th->ID) {
                preempt_start_time = min(preempt_start_time, rqe->release_time);
            }
            if (rqe->release_time + rqe->thrd->period < earliest_deadline) {
                preempt_start_time = min(preempt_start_time, rqe->release_time);
            }
        }

        allocate_time = min(thread_with_smallest_id->current_deadline - args.current_time, thread_with_smallest_id->remaining_time);
        if (preempt_start_time != 2147483647 && args.current_time + thread_with_smallest_id->remaining_time > preempt_start_time) {
            allocate_time = min(allocate_time, preempt_start_time - args.current_time);
        }
    }
    else {
        list_for_each_entry(rqe, args.release_queue, thread_list) {
            preempt_start_time = min(preempt_start_time, rqe->release_time);
        }
        allocate_time = preempt_start_time - args.current_time;
    }


    struct threads_sched_result r;
    if (thread_with_smallest_id != NULL) {
        r.scheduled_thread_list_member = &thread_with_smallest_id->thread_list;
        r.allocated_time = allocate_time;
    } else {
        r.scheduled_thread_list_member = args.run_queue;
        r.allocated_time = allocate_time;
    }

    return r;
}

/* Rate-Monotonic Scheduling */
struct threads_sched_result schedule_rm(struct threads_sched_args args)
{
    // check miss deadline first
    struct thread *thread_with_smallest_id_miss_deadline = NULL;
    struct thread *th_miss_deadline = NULL;
    list_for_each_entry(th_miss_deadline, args.run_queue, thread_list) {
        if (th_miss_deadline->current_deadline <= args.current_time) {
            if (thread_with_smallest_id_miss_deadline == NULL) {
                thread_with_smallest_id_miss_deadline = th_miss_deadline;
            }
            if (thread_with_smallest_id_miss_deadline != NULL && th_miss_deadline->ID < thread_with_smallest_id_miss_deadline->ID) {
                thread_with_smallest_id_miss_deadline = th_miss_deadline;
            }
        }
    }
    if (thread_with_smallest_id_miss_deadline != NULL) {
        struct threads_sched_result r;
        r.scheduled_thread_list_member = &thread_with_smallest_id_miss_deadline->thread_list;
        r.allocated_time = 0;
        return r;
    }

    struct thread *thread_with_smallest_id = NULL;
    struct thread *th = NULL;
    int smallest_period = 2147483647;
    list_for_each_entry(th, args.run_queue, thread_list) {
        if (thread_with_smallest_id != NULL && th->period == smallest_period) {
            if (th->ID < thread_with_smallest_id->ID) {
                thread_with_smallest_id = th;
            }
        }
        if (thread_with_smallest_id == NULL || th->period < smallest_period) {
            smallest_period = th->period;
            thread_with_smallest_id = th;
        }
    }

    struct release_queue_entry *rqe = NULL;
    int preempt_start_time = 2147483647;
    int allocate_time;
    if (thread_with_smallest_id != NULL) {
        list_for_each_entry(rqe, args.release_queue, thread_list) {
            if (rqe->thrd->period == smallest_period && rqe->thrd->ID < th->ID) {
                preempt_start_time = min(preempt_start_time, rqe->release_time);
            }
            if (rqe->thrd->period < smallest_period) {
                preempt_start_time = min(preempt_start_time, rqe->release_time);
            }
        }

        allocate_time = min(thread_with_smallest_id->current_deadline - args.current_time, thread_with_smallest_id->remaining_time);
        if (preempt_start_time != 2147483647 && args.current_time + thread_with_smallest_id->remaining_time > preempt_start_time) {
            allocate_time = min(allocate_time, preempt_start_time - args.current_time);
        }
    }
    else {
        list_for_each_entry(rqe, args.release_queue, thread_list) {
            preempt_start_time = min(preempt_start_time, rqe->release_time);
        }
        allocate_time = preempt_start_time - args.current_time;
    }
    struct threads_sched_result r;
    if (thread_with_smallest_id != NULL) {
        r.scheduled_thread_list_member = &thread_with_smallest_id->thread_list;
        r.allocated_time = allocate_time;
    } else {
        r.scheduled_thread_list_member = args.run_queue;
        r.allocated_time = allocate_time;
    }

    return r;
}
