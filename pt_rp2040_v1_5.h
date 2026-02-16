
// define macros to implement protothreads on the rp2040
#ifndef __PT_H__
#include "irl/pt.h"   // for protothread macros, data structures, and API functions
#endif /* __PT_H__ */

// default protothread semaphores, not core safe, but OK for one core
#ifndef __PT_SEM_H__
#include "irl/pt_sem.h"  // for protothread semaphoresS
#endif /* __PT_SEM_H__ */

//=== BRL4 additions for rp2040 SDK =======================================
#ifndef __PT_RP2040_H__
#include "irl/pt_rp2040.h"  // for rp2040 specific protothread macros
#endif /* __PT_RP2040_H__ */

// default protothread scheduler 
#ifndef __PT_RP2040_SCHED_H__
#include "irl/pt_rp2040_SCHED.h"  // for protothread scheduler
#endif /* __PT_RP2040_SCHED_H__ */

// default protothread serial input and output
#ifndef __PT_RP2040_SERIAL_H__
#include "irl/pt_rp2040_SERIAL.h"  // for protothread serial input and output
#endif /* __PT_RP2040_SERIAL_H__ */
