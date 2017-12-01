#ifndef __APP_UTIL_H_
#define __APP_UTIL_H__

typedef struct 
{
   void *pImage; /* point to struct carrying config record data*/
   unsigned short ImageLen;/* length of array pointed by pImage*/
   unsigned short configBlockSize;/* size of 1 chunk */
}cfgrec_t;


hbi_status_t ldcfgrec(hbi_handle_t handle,void *arg);
hbi_status_t ldfwr(hbi_handle_t handle,void *arg);
#endif


