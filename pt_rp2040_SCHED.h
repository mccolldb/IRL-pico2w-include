
// IMPROVED SCHEDULER 
#ifndef __PT_RP2040_SCHED_H__
#define __PT_RP2040_SCHED_H__

typedef char (*pf_t)(pt_t *pt);  // pointer to a protothread function

// === thread structures ===
typedef struct ptx {
	pt_t pt;  // thread context
	pf_t pf;  // pointer to thread function
  char state; // thread state
	int num;  // thread number
} ptx_t;

#define MAX_THREADS 10
#define sched_stats
typedef struct sched_data 
{ int core_num;                           // core number for this scheduler
  int i;                                  // loop variable
  int task_count;                         // count of defined threads
  ptx_t thread_list[MAX_THREADS];         // an array of task structures
  #ifdef sched_stats
  int      sched_count;                    // count of scheduler runs
  int      sched_thread_runs[MAX_THREADS]; // count of runs for each thread
  uint64_t thread_time;                    // temperary variable to measure thread execution time
  uint64_t sched_thread_time[MAX_THREADS]; // accumulated execution time for each thread
  #endif
} sched_data_t;
static sched_data_t sched_data[NUM_CORES] = { {.core_num=0}, {.core_num=1} } ;
static pt_t         pt_sched[NUM_CORES]; 

int pt_add_thread( pf_t pf, void* data) 
{ sched_data_t* sched = &sched_data[get_core_num()];       // pointer to scheduler data structure
  if (sched->task_count >= MAX_THREADS) return 0;          // too many threads
  ptx_t* thread = &sched->thread_list[sched->task_count];  // get the current thread table entry 
  thread->pf    = pf;                                      // function pointer
  thread->num   = sched->task_count;
  thread->state = PT_RUNNING;                              // set thread state to enabled
  PT_INIT( &thread->pt, data );                            // initialize the thread context, and point to its data
  sched->task_count++;                                     // count of number of defined threads       
  return sched->task_count - 1;                            // return current entry
}

// choose schedule method
#define SCHED_ROUND_ROBIN 0
#define SCHED_PRIORITY    1
int pt_sched_method = SCHED_ROUND_ROBIN; // default is round robin

static PT_THREAD (sched_pt(pt_t *pt))
{   sched_data_t* sched = (sched_data_t*)pt->data; // pointer to provided scheduler data structure
    PT_BEGIN(pt);
    if (pt_sched_method==SCHED_ROUND_ROBIN)
    {   int active_tasks = sched->task_count;
        while(active_tasks > 0)  
        { ptx_t* thread = &sched->thread_list[0];
          active_tasks = sched->task_count;
          for (sched->i=0; sched->i<sched->task_count; sched->i++, thread++ ) // step thru all defined threads
          { if (thread->state < PT_EXITED) thread->state = (thread->pf)(&thread->pt); // call thread function if it is enabled
            else active_tasks--; // if thread is exited, then decrement active thread count 
          }
        }
    } //end if (pt_sched_method==RR)     

    if (pt_sched_method==SCHED_PRIORITY)
    {   while(1) 
        { 
          #ifdef sched_stats
            sched->sched_count++;
          #endif
          ptx_t* thread = &sched->thread_list[0];
          for (sched->i=0; sched->i<sched->task_count; sched->i++, thread++ )
          {   pt_executed[sched->core_num] = false;     // zero execute flag
              #ifdef sched_stats
              sched->thread_time = time_us_64();
              #endif
              (thread->pf)(&thread->pt);  // call thread function
              if (pt_executed[sched->core_num]) // if there was execution, then restart execution list
              {
                #ifdef sched_stats
                  sched->sched_thread_runs[sched->i]++ ;
                  sched->sched_thread_time[sched->i] += (time_us_64() - sched->thread_time);
                #endif
                break; // restart execution list
              }
          }
        }
    } 
    PT_END(pt);
} // scheduler thread



// ========================================================
// === package the scheduler =============================
#define pt_schedule_start \
{ int core = get_core_num(); \
  PT_INIT(&pt_sched[core], &sched_data[core]); \
  sched_pt(&pt_sched[core]); \
} 

#endif /* __PT_RP2040_SCHED_H__ */

