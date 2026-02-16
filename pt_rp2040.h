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
