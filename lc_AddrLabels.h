// Local continuations using address labels
#ifndef __LC_H__
#define __LC_H__
#define LC_CONCAT2(s1, s2) s1##s2
#define LC_CONCAT(s1, s2) LC_CONCAT2(s1, s2)

typedef void* lc_t;

#define LC_INIT(lc)   lc = NULL
#define LC_RESUME(lc) do {  if(lc != NULL) goto *lc;	} while(0)
#define LC_SET(lc)	  do {  LC_CONCAT(LC_LABEL, __LINE__):  (lc) = &&LC_CONCAT(LC_LABEL, __LINE__);	} while(0)
#define LC_END(lc)

#endif
