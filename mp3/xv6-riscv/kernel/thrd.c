#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// for mp3
uint64
sys_thrdstop(void)
{
  int delay;
  uint64 context_id_ptr;
  uint64 handler, handler_arg;
  if (argint(0, &delay) < 0)
    return -1;
  if (argaddr(1, &context_id_ptr) < 0)
    return -1;
  if (argaddr(2, &handler) < 0)
    return -1;
  if (argaddr(3, &handler_arg) < 0)
    return -1;

  //struct proc *proc = myproc();  
  //TODO: mp3
  struct proc *p = myproc();
  int thrdstop_context_id;
  copyin(p->pagetable, (char *)&thrdstop_context_id, context_id_ptr, sizeof(int));
  //printf("thrdstop_context_id %d \n", thrdstop_context_id);

  p->thrdstop_ticks = 0;
  p->thrdstop_delay = delay;
  p->thrdstop_handler_pointer = handler;
  p->thrstop_handler_arg = handler_arg;
  if(thrdstop_context_id >= 0 && thrdstop_context_id < MAX_THRD_NUM) {
    p->thrdstop_context_id = thrdstop_context_id;
    p->thrdstop_context_used[thrdstop_context_id] = 1;
    //return thrdstop_context_id;
    return 0;
  }
  else if(thrdstop_context_id == -1) {
    for(int i = 0; i < MAX_THRD_NUM; ++i) {
      if(!p->thrdstop_context_used[i]) {
        p->thrdstop_context_id = i;
        p->thrdstop_context_used[i] = 1;
        //return i;
        return 0;
      }
    }
  }
  return -1;

  //return 0;
}

// for mp3
uint64
sys_cancelthrdstop(void)
{
  int context_id, is_exit;
  if (argint(0, &context_id) < 0)
    return -1;
  if (argint(1, &is_exit) < 0)
    return -1;

  if (context_id < 0 || context_id >= MAX_THRD_NUM) {
    return -1;
  }

  //struct proc *proc = myproc();

  //TODO: mp3
  struct proc *p = myproc();
  p->thrdstop_context_id = context_id;
  p->sys_def = 2;
  p->is_exit = is_exit;
  p->thrdstop_delay= -1;
  return p->thrdstop_ticks;

  //return 0;
}

// for mp3
uint64
sys_thrdresume(void)
{
  int context_id;
  if (argint(0, &context_id) < 0)
    return -1;

  //struct proc *proc = myproc();

  //TODO: mp3
  myproc()->sys_def = 3;
  myproc()->thrdstop_context_id = context_id;
  return 0;
}
