#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

#include "includes.h"
#include <stdint.h>
#include <stddef.h>




#ifndef NULL
    #define NULL (0)
#endif

typedef uint16_t dev_addr_t;

#ifndef __bool_true_and_false_are_defined
#define __bool_true_and_false_are_defined

#define TRUE        1
#define FALSE      (!TRUE)
#endif

/* typedef for SSL port and lock handle. User can redefine to any as per their system need */
typedef uint32_t ssl_port_handle_t;
typedef uint32_t ssl_lock_handle_t;

/* structure defining device configuration */
typedef struct
{
    dev_addr_t   dev_addr; /* device address */
    uint8_t     *pDevName; /* null terminated  device name as passed by user*/
    uint8_t      bus_num; /* bus id device is connected on */
}ssl_dev_cfg_t;

typedef void ssl_drv_cfg_t;
#endif /*__TYPEDEF_H__ */ 
