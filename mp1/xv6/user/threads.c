#include "kernel/types.h"
#include "user/setjmp.h"
#include "user/threads.h"
#include "user/user.h"
#define NULL 0


static struct thread* current_thread = NULL;
static int id = 1;
static jmp_buf env_st;
//static jmp_buf env_tmp;

struct thread *thread_create(void (*f)(void *), void *arg){
    struct thread *t = (struct thread*) malloc(sizeof(struct thread));
    unsigned long new_stack_p;
    unsigned long new_stack;
    new_stack = (unsigned long) malloc(sizeof(unsigned long)*0x100);
    new_stack_p = new_stack +0x100*8-0x2*8;
    t->fp = f;
    t->arg = arg;
    t->ID  = id;
    t->buf_set = 0;
    t->stack = (void*) new_stack;
    t->stack_p = (void*) new_stack_p;
    id++;

    // part 2
    t->sig_handler[0] = NULL_FUNC;
    t->sig_handler[1] = NULL_FUNC;
    t->signo = -1;
    t->handler_buf_set = 0;
    return t;
}
void thread_add_runqueue(struct thread *t){
    if(current_thread == NULL){
        current_thread = t;
        current_thread->next = current_thread;
        current_thread->previous = current_thread;
    }
    else{
        struct thread *last_thread = current_thread->previous;
        t->previous = last_thread;
        t->next = current_thread;
        current_thread->previous = t;
        last_thread->next = t;
    }
    for (int i = 0; i < 2; i++) {
        t->sig_handler[i] = current_thread->sig_handler[i];
    }
}
void thread_yield(void){

    int is_in_handler = current_thread->handler_buf_set && current_thread->signo != -1;

    if(is_in_handler) {
        if(setjmp(current_thread->handler_env)) {
            return;
        }
    } else {
        if(setjmp(current_thread->env)) {
            return;
        }
    }
    
    schedule();
    dispatch();
}
void handle_signal(void) {
    int signo = current_thread->signo;
    if(current_thread->sig_handler[signo] != NULL_FUNC) {
        if(current_thread->handler_buf_set == 0) {
            current_thread->handler_buf_set = 1;
            if(setjmp(current_thread->handler_env) == 0) {
                current_thread->handler_env->sp = current_thread->env->sp;
                longjmp(current_thread->handler_env, 1);
            }
            else {
                current_thread->sig_handler[signo](signo);
                current_thread->handler_buf_set = 0;
                current_thread->signo = -1;
                longjmp(current_thread->env, 1);
            }
        }
        else {
            longjmp(current_thread->handler_env, 1);
        }
    } else {
        thread_exit();
    }
}
void dispatch(void){
    if(current_thread->buf_set == 0){
        current_thread->buf_set = 1;
        if(setjmp(current_thread->env) == 0){
            current_thread->env->sp = (unsigned long)current_thread->stack_p;
            longjmp(current_thread->env, 1);
        }
        else{
            if (current_thread->signo != -1) handle_signal();
            current_thread->fp(current_thread->arg);
            thread_exit();
        }
    }
    else{
        if (current_thread->signo != -1) handle_signal();
        longjmp(current_thread->env, 1);
    }
}
void schedule(void){
    if(current_thread->next != current_thread){
        current_thread = current_thread->next;
    }
}
void thread_exit(void){
    if(current_thread->next != current_thread){
        struct thread* prev_thread = current_thread->previous;
        struct thread* next_thread = current_thread->next;
        prev_thread->next = next_thread;
        next_thread->previous = prev_thread;
        free(current_thread->stack);
        free(current_thread);
        current_thread = next_thread;
        dispatch();
    }
    else{
        free(current_thread->stack);
        free(current_thread);
        current_thread = NULL;
        longjmp(env_st, 1);
    }
}
void thread_start_threading(void){
    if(setjmp(env_st)) 
        return;
    schedule();
    dispatch();
}
// part 2
void thread_register_handler(int signo, void (*handler)(int)){
    current_thread->sig_handler[signo] = handler;
}
void thread_kill(struct thread *t, int signo){
    t->signo = signo;
}