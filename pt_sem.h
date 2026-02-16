// default protothread semaphores, not core safe, but OK for one core

#ifndef __PT_SEM_H__
#define __PT_SEM_H__

typedef struct pt_sem {
  unsigned int count;
} pt_sem_t;

#define PT_SEM_INIT(s, c) (s)->count = c
#define PT_SEM_WAIT(pt, s)	{ PT_YIELD_UNTIL(pt, (s)->count > 0);	--(s)->count; }
#define PT_SEM_SIGNAL(pt,s) ++(s)->count

#endif /* __PT_SEM_H__ */
