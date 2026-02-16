// define macros to implement protothreads on the rp2040
#ifndef __PT_H__
#define __PT_H__

// include before for custom protothread local continuations
#ifndef __LC_H__
#include "irl/lc_AddrLabels.h"  // default Local continuations (using address labels)
#endif

// start Proto Threads code
typedef struct pt 
{ lc_t lc;    // protothread local continuation
  void *data; // for passing data to threads, if needed
} pt_t;

// protothread states
#define PT_RUNNING 0
#define PT_WAITING 0
#define PT_YIELDED 1
#define PT_EXITED  2
#define PT_ENDED   3

#define PT_THREAD(func_args) char func_args
#define PT_SCHEDULE(func)    ((func) < PT_EXITED)

#define PT_INIT(pt,dataptr)  { LC_INIT((pt)->lc); (pt)->data = (dataptr); }
#define PT_BEGIN(pt)         { char PT_FLAG = PT_RUNNING; LC_RESUME((pt)->lc) 
#define PT_END(pt)             LC_END((pt)->lc); PT_FLAG = PT_YIELDED; \
                               PT_INIT(pt,pt->data); return PT_ENDED; }
#define PT_EXIT(pt)				   { PT_INIT(pt,pt->data); return PT_EXITED;  }
#define PT_RESTART(pt)       { PT_INIT(pt,pt->data); return PT_WAITING; }

#define PT_WAIT_WHILE(pt, condition) { LC_SET((pt)->lc); if(condition) return PT_WAITING; } 
#define PT_WAIT_UNTIL(pt, condition) PT_WAIT_WHILE((pt), !(condition))
#define PT_WAIT(pt)                  PT_WAIT_UNTIL((pt), true)

bool pt_executed[NUM_CORES] = {false,false} ; // flags to indicate if a thread executed, used by priority scheduler
#define PT_SET_EXECUTED {  pt_executed[get_core_num()] = true; }
#define PT_YIELD_WHILE(pt, cond)		\
  { PT_FLAG = PT_YIELDED; LC_SET((pt)->lc);	if((PT_FLAG == PT_YIELDED) || (cond)) return PT_YIELDED; PT_SET_EXECUTED; }
#define PT_YIELD_UNTIL(pt, cond)	PT_YIELD_WHILE((pt), !(cond))
#define PT_YIELD(pt)	            PT_YIELD_UNTIL((pt),true) 		

//Hierarchical protothreads -- spawn a child thread and wait for it to complete
#define PT_WAIT_THREAD(pt, thread) PT_WAIT_WHILE((pt), PT_SCHEDULE(thread))
#define PT_SPAWN(pt, child, data, thread)	do { PT_INIT((child),(data)); PT_WAIT_THREAD((pt), (thread));	} while(0)

#endif /* __PT_H__ */




