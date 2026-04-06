#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/* Possible states of a thread: */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4


struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
};
char stack[STACK_SIZE] = {
  /*   0 */ [0]   = '\0', // ra
  /*   8 */ [8]   = '\0', // sp
  /*  16 */ [16]  = '\0', // gp
  /*  24 */ [24]  = '\0', // tp
  /*  32 */ [32]  = '\0', // t0
  /*  40 */ [40]  = '\0', // t1
  /*  48 */ [48]  = '\0', // t2
  /*  56 */ [56]  = '\0', // s0/fp
  /*  64 */ [64]  = '\0', // s1
  /*  72 */ [72]  = '\0', // a0
  /*  80 */ [80]  = '\0', // a1
  /*  88 */ [88]  = '\0', // a2
  /*  96 */ [96]  = '\0', // a3
  /* 104 */ [104] = '\0', // a4
  /* 112 */ [112] = '\0', // a5
  /* 120 */ [120] = '\0', // a6
  /* 128 */ [128] = '\0', // a7
  /* 136 */ [136] = '\0', // s2
  /* 144 */ [144] = '\0', // s3
  /* 152 */ [152] = '\0', // s4
  /* 160 */ [160] = '\0', // s5
  /* 168 */ [168] = '\0', // s6
  /* 176 */ [176] = '\0', // s7
  /* 184 */ [184] = '\0', // s8
  /* 192 */ [192] = '\0', // s9
  /* 200 */ [200] = '\0', // s10
  /* 208 */ [208] = '\0', // s11
  /* 216 */ [216] = '\0', // t3
  /* 224 */ [224] = '\0', // t4
  /* 232 */ [232] = '\0', // t5
  /* 240 */ [240] = '\0', // t6
};
struct thread all_thread[MAX_THREAD];
struct thread *current_thread;
extern void thread_switch(uint64, uint64);
              
void 
thread_init(void)
{
  // main() is thread 0, which will make the first invocation to
  // thread_schedule().  it needs a stack so that the first thread_switch() can
  // save thread 0's state.  thread_schedule() won't run the main thread ever
  // again, because its state is set to RUNNING, and thread_schedule() selects
  // a RUNNABLE thread.
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void 
thread_schedule(void)
{
  struct thread *t, *next_thread;

  /* Find another runnable thread. */
  next_thread = 0;
  t = current_thread + 1;
  for(int i = 0; i < MAX_THREAD; i++){
    if(t >= all_thread + MAX_THREAD)
      t = all_thread;
    if(t->state == RUNNABLE) {
      next_thread = t;
      break;
    }
    t = t + 1;
  }

  if (next_thread == 0) {
    printf("thread_schedule: no runnable threads\n");
    exit(-1);
  }

  if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    /* YOUR CODE HERE
     * Invoke thread_switch to switch from t to next_thread:
     * thread_switch(??, ??);
     */
    thread_switch((uint64)t, (uint64)current_thread);
  } else
    next_thread = 0;
}

void 
thread_create(void (*func)())
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  uint64 *context = (uint64 *)t->stack;

  for (int i = 0; i < 32; i++) {
    context[i] = 0;
  }
  context[0] = (uint64)func;
  context[1] = (uint64)(t->stack + STACK_SIZE);
  t->state = RUNNABLE;
}

void 
thread_yield(void)
{
  current_thread->state = RUNNABLE;
  thread_schedule();
}

volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;

void 
thread_a(void)
{
  int i;
  printf("thread_a started\n");
  a_started = 1;
  while(b_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_a %d\n", i);
    a_n += 1;
    thread_yield();
  }
  printf("thread_a: exit after %d\n", a_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_b(void)
{
  int i;
  printf("thread_b started\n");
  b_started = 1;
  while(a_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_b %d\n", i);
    b_n += 1;
    thread_yield();
  }
  printf("thread_b: exit after %d\n", b_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_c(void)
{
  int i;
  printf("thread_c started\n");
  c_started = 1;
  while(a_started == 0 || b_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_c %d\n", i);
    c_n += 1;
    thread_yield();
  }
  printf("thread_c: exit after %d\n", c_n);

  current_thread->state = FREE;
  thread_schedule();
}

int 
main(int argc, char *argv[]) 
{
  a_started = b_started = c_started = 0;
  a_n = b_n = c_n = 0;
  thread_init();
  thread_create(thread_a);
  thread_create(thread_b);
  thread_create(thread_c);
  thread_schedule();
  exit(0);
}
