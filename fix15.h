#ifndef FIX15_H
#define FIX15_H

// Fixed point data type
typedef signed int fix15 ;
#define multfix15(a,b) ((fix15)(((( signed long long)(a))*(( signed long long)(b)))>>16)) 
#define float2fix15(a) ((fix15)((a)*65536.0f)) // 2^16
#define fix2float15(a) ((float)(a)/65536.0f) 
#define int2fix15(a) ((a)<<16)
#define fix2int15(a) ((a)>>16)
#define divfix(a,b) ((fix15)(((( signed long long)(a) << 16 / (b)))))

#endif