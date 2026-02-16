
// define macros to implement protothreads on the rp2040
#ifndef __PT_H__
#define __PT_H__

// Local continuations using address labels
#ifndef __LC_ADDRLABELS_H__
#define __LC_ADDRLABELS_H__
#define LC_CONCAT2(s1, s2) s1##s2
#define LC_CONCAT(s1, s2) LC_CONCAT2(s1, s2)

typedef void* lc_t;

#define LC_INIT(lc)   lc = NULL
#define LC_RESUME(lc) do {  if(lc != NULL) goto *lc;	} while(0)
#define LC_SET(lc)	  do {  LC_CONCAT(LC_LABEL, __LINE__):  (lc) = &&LC_CONCAT(LC_LABEL, __LINE__);	} while(0)
#define LC_END(lc)

#endif /* __LC_ADDRLABELS_H__ */

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
#define PT_END(pt)             LC_END((pt)->lc); PT_FLAG = PT_YIELDED; PT_INIT(pt,pt->data); return PT_ENDED; }
#define PT_EXIT(pt)				   { PT_INIT(pt,pt->data); return PT_EXITED; }
#define PT_RESTART(pt)       { PT_INIT(pt,pt->data); return PT_WAITING; }

#define PT_WAIT_WHILE(pt, condition) { LC_SET((pt)->lc); if(condition) return PT_WAITING;} 
#define PT_WAIT_UNTIL(pt, condition) PT_WAIT_WHILE((pt), !(condition))
#define PT_WAIT(pt)                  PT_WAIT_UNTIL((pt), true)

bool pt_executed[NUM_CORES] = {false,false} ; // flags to indicate if a thread executed, used by priority scheduler
#define PT_SET_EXECUTED {  pt_executed[get_core_num()] = true; }
#define PT_YIELD_WHILE(pt, cond)		\
  { PT_FLAG = PT_YIELDED; LC_SET((pt)->lc);	if((PT_FLAG == PT_YIELDED) || (cond)) return PT_YIELDED; PT_SET_EXECUTED; }
#define PT_YIELD_UNTIL(pt, cond)	PT_YIELD_WHILE((pt), !(cond))
#define PT_YIELD(pt)	            PT_YIELD_UNTIL((pt),true) 		

//Hierarchical protothreads
#define PT_WAIT_THREAD(pt, thread) PT_WAIT_WHILE((pt), PT_SCHEDULE(thread))
#define PT_SPAWN(pt, child, data, thread)	do { PT_INIT((child),(data)); PT_WAIT_THREAD((pt), (thread));	} while(0)

#endif /* __PT_H__ */

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

//=== BRL4 additions for rp2040 SDK =======================================
#ifndef __PT_RP2040_H__
#define __PT_RP2040_H__
//=====================================================================
//=== BRL4 additions for rp2040 =======================================
//=====================================================================

#define PT_GET_TIME_usec() time_us_64()

#define PT_YIELD_usec(delay_time)  \
    { static uint64_t time_thread ;time_thread = PT_GET_TIME_usec() + (uint64_t)delay_time ; \
    PT_YIELD_UNTIL(pt, (PT_GET_TIME_usec() >= time_thread)); }


// macros for interval yield
// attempts to make interval equal to specified value
#define PT_INTERVAL_INIT() static uint64_t pt_interval_marker
#define PT_YIELD_INTERVAL(interval_time)  \
    { PT_YIELD_UNTIL(pt, (uint32_t)(PT_GET_TIME_usec() >= pt_interval_marker)); \
      pt_interval_marker = PT_GET_TIME_usec() + (uint64_t)interval_time; }
//
// =================================================================
// core-safe semaphore based on pico/sync library
// NEEDS SDK 1.1.1 or higher
// a hardware spinlock to force core-safe alternation
// NOTE that the default protothreads semaphore is not
// multi-core safe, but is OK one one core
// The SAFE versions work across cores, but have more overhead

#define PT_SEM_SDK_WAIT(pt,s) {	PT_YIELD_UNTIL (pt, sem_try_acquire(s)); PT_SET_EXECUTED;}
#define PT_SEM_SDK_SIGNAL(pt,s) sem_release(s)  ;


// ==================================================================
// core-safe mutex based on pico/sync library
// NEEDS SDK 1.1.1 or higher

#define PT_MUTEX_SDK_AQUIRE(pt,m)	{ PT_YIELD_UNTIL(pt, mutex_try_enter(m, NULL)); PT_SET_EXECUTED; }
#define PT_MUTEX_SDK_RELEASE(m) mutex_exit(m);

//====================================================================
// Multicore communication via FIFO
#define PT_FIFO_WRITE(data)    { PT_YIELD_UNTIL(pt, multicore_fifo_wready()); multicore_fifo_push_blocking(data); } 
#define PT_FIFO_READ(fifo_out) { PT_YIELD_UNTIL(pt, multicore_fifo_rvalid());  fifo_out = multicore_fifo_pop_blocking(); }
#define PT_FIFO_FLUSH            multicore_fifo_drain(); 

#endif /* __PT_RP2040_H__ */

// default protothread scheduler 
#ifndef __PT_RP2040_SCHED_H__
#define __PT_RP2040_SCHED_H__

typedef char (*pf_t)(pt_t *pt);  // pointer to a protothread function

// === thread structures ===
typedef struct ptx {
	pt_t pt;  // thread context
	pf_t pf;  // pointer to thread function
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
  thread->num   = sched->task_count;                       // enter the task data into the thread table
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
    //sched_data_t* sched = &sched_data[get_core_num()]; // pointer to scheduler data structure
    PT_BEGIN(pt);
    if (pt_sched_method==SCHED_ROUND_ROBIN)
    {   while(1) 
        { ptx_t* thread = &sched->thread_list[0];
          for (sched->i=0; sched->i<sched->task_count; sched->i++, thread++ ) // step thru all defined threads
          { (thread->pf)(&thread->pt); // call thread function
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

// default protothread serial input and output
#ifndef __PT_RP2040_SERIAL_H__
#define __PT_RP2040_SERIAL_H__
// === serial input thread ================================
#define pt_buffer_size 255
static pt_t pt_serialin, pt_serialout ;
#define UART_ID uart0
#define pt_backspace 0x7f // make sure your backspace matches this!

static PT_THREAD (pt_serialin_polled(pt_t *pt))
{ char* buffer = (char*)pt->data;
  PT_BEGIN(pt);
  static uint8_t ch ;
  static int pt_current_char_count ;

  memset(buffer, 0, pt_buffer_size);       // clear the string
  pt_current_char_count = 0 ;
  while(uart_is_readable(UART_ID)) uart_getc(UART_ID);       // clear uart fifo

  while(pt_current_char_count < pt_buffer_size) // build the output string
  { PT_YIELD_UNTIL(pt, (int)uart_is_readable(UART_ID)); ch = uart_getc(UART_ID); // read one character
    PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)); uart_putc(UART_ID, ch);  // echo back
    if (ch == '\r' ) // check for <enter> or <backspace>
    { // advances the cursor to the next line, then exits
      buffer[pt_current_char_count] = 0 ; // <enter>> character terminates string,
      PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)); uart_putc(UART_ID, '\n') ;
      break; 
    }

    if (ch == pt_backspace) // check for <backspace>
    {
      PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)); uart_putc(UART_ID, ' ') ;
      PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)); uart_putc(UART_ID, pt_backspace);
      pt_current_char_count-- ;
      if (pt_current_char_count<0) pt_current_char_count = 0 ;
      continue;
    }
    buffer[pt_current_char_count++] = ch; // must be a real character, build the output string
  } // END WHILe
  PT_EXIT(pt); // kill this input thread, to allow spawning thread to execute
  PT_END(pt);
} // serial input thread

// ================================================================
// === serial output thread
//
int pt_serialout_polled(pt_t *pt)
{   char* buffer = (char*)pt->data;
    static int num_send_chars ;
    PT_BEGIN(pt);
    num_send_chars = 0;
    while (buffer[num_send_chars] != 0)
    {
        PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)) ;
        uart_putc(UART_ID, buffer[num_send_chars]) ;
        num_send_chars++;
    }
    PT_EXIT(pt);
    PT_END(pt);
}
// ================================================================
// package the spawn read/write macros to make them look better
#define serial_write(buffer) PT_SPAWN(pt,&pt_serialout,(buffer),pt_serialout_polled(&pt_serialout));
#define serial_read(buffer)  PT_SPAWN(pt,&pt_serialin,(buffer),pt_serialin_polled(&pt_serialin));

#endif /* __PT_RP2040_SERIAL_H__ */
