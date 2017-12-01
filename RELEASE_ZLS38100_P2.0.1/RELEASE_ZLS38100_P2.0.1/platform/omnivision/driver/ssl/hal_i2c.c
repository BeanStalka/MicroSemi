/*
Header files defining a "Hardware Abstraction Layer for Voice processor devices
over linux kernel 3.18.x i2c core driver. Every successful call would return 0 or 
a linux error code as defined in linux errno.h
*/
#include "typedefs.h"
#include "ssl.h"
#include "hal.h"
#include "vproc_dbg.h"


ssl_dev_cfg_t vproc_dev_cfg[VPROC_MAX_NUM_DEVS];
int32_t vproc_dev_busy[VPROC_MAX_NUM_DEVS];


int hal_init(void)
{

   VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter...\n");
   memset(vproc_dev_busy,0,sizeof(vproc_dev_busy));
   memset(vproc_dev_cfg,0,sizeof(vproc_dev_cfg));
   VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit\n");
   return 0;
}

int hal_term()
{
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter\n");


    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit\n");

    return 0;
}
/*
    This function opens an i2c client to a device 
    identifies via "devcfg" parameter.
    Returns 0 and update client into a reference handle, if successful
    or an error code for an invalid parameter or client instantiation 
    fails.
*/
int hal_open(void **ppHandle,ssl_dev_cfg_t *pDev)
{
   int i;
    
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter\n");
    
    if(pDev == NULL || ppHandle==NULL )
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Device Cfg Reference\n");
        return -1;
    }

    i=0;
    while(i < sizeof(vproc_dev_busy) && vproc_dev_busy[i++]);

    if(i>=sizeof(vproc_dev_busy) && vproc_dev_busy[i-1])
    {
        /* call failed either because address is invalid, valid but occupied
           or there is resource err. Just return code to try again later */
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,
                        "Cannot open more device.Max allowed %d\n",
                        sizeof(vproc_dev_busy));
        return -1;
    }

    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,
                     "i:%d Received addr 0x%x bus 0x%x\n",
                     i,pDev->dev_addr,pDev->bus_num);
    vproc_dev_cfg[i-1].dev_addr = pDev->dev_addr;
    vproc_dev_cfg[i-1].bus_num = pDev->bus_num;
    vproc_dev_busy[i-1] = 1;

   *((int32_t **)ppHandle) =  (int32_t *)(i-1);
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"Return Handle %d\n",i-1);
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit..\n");

    return 0;
}

int hal_close(void *pHandle)
{
    int32_t index = (int32_t)pHandle;
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter\n");

    if(index >= sizeof(vproc_dev_cfg))
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid port handle\n");
        return -1;
    }

    vproc_dev_busy[index]=0;
    
    
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit..\n");
    return 0;
}

int hal_port_rw(void *pHandle,ssl_port_access_t *pPort)
{
   int            ret=0;
   ssl_op_t       op_type;
   t_libsccb_cfg  sccbcfg;


   VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,
                     "Enter (handle 0x%x)...\n",(int32_t)handle);

   if(pPort == NULL)
   {
      VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Parameters\n");
      return -1;
   }

   op_type = pPort->op_type;

   memset(&sccbcfg,0,sizeof(sccbcfg));

   sccbcfg.id = vproc_dev_cfg[(int32_t)pHandle].dev_addr;
   sccbcfg.clk = 400*1024;
   sccbcfg.mode = SCCB_MODE8;
   sccbcfg.rd_mode = RD_MODE_RESTART;

   ret = sccb_set(&sccbcfg);
   if(ret != 0)
   {
      /* call failed either because address is invalid, valid but occupied
        or there is resource err. Just return code to try again later */
      VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"sccb set failed err %d\n",ret);
      return -1;
   }
   if(op_type & SSL_OP_PORT_WR)
   {
        if(pPort->pSrc == NULL)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL src buffer passed\n");
            return -1;
        }

        ret = sccb_seqwrite(&sccbcfg,(int)pHandle,pPort->nwrite,pPort->pSrc);
        if(ret != 0)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"failed with Error 0x%x\n",ret);
            return -1;
        }
    }
    if(op_type & SSL_OP_PORT_RD)
    {
        if(pPort->pDst == NULL)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL destination buffer passed\n");
            return -1;
        }
        ret = sccb_seqread(&sccbcfg,(int)pHandle,pPort->nread,pPort->pDst);
         if(ret != 0)
       {
           VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"failed with Error %d\n",ret);
       }
    }


    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit..\n");

    return ret;
}



