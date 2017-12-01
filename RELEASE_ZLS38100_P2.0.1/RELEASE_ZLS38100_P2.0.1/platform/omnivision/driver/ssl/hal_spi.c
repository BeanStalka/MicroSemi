/*
Header files defining a "Hardware Abstraction Layer for Voice processor devices
over linux kernel 3.18.x i2c core driver. Every successful call would return 0 or 
a linux error code as defined in linux errno.h
*/
#include "typedefs.h"
#include "ssl.h"
#include "hal.h"
#include "vproc_dbg.h"




#define SPI_SPEED 8000000
#define CS_LOW(cs_num)          do{\
                                    if(cs_num) \
                                     libsif1_cs_low(); \
                                    else \
                                    libsif0_cs_low(); \
                                  }while(0)

#define CS_HI(cs_num)          do{\
                                if(cs_num) \
                                     libsif1_cs_high(); \
                                else \
                                    libsif0_cs_high(); \
                              }while(0)

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
int hal_open(void **ppHandle,ssl_dev_cfg_t *pDevCfg)
{
   int i;

    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter\n");
    
    if((pDevCfg == NULL) || (ppHandle==NULL))
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Device Cfg Reference\n");
        return -1;
    }

    i=0;
    while(i < sizeof(vproc_dev_busy) && vproc_dev_busy[i++]);
    
    if(i>=sizeof(vproc_dev_busy) && vproc_dev_busy[i-1])
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,
                        "Cannot open more device.Max allowed %d\n",
                        sizeof(vproc_dev_busy));
        return -1;
    }
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,
                     "i:%d Received addr 0x%x bus 0x%x\n",
                     i,pDevCfg->dev_addr,pDevCfg->bus_num);
    vproc_dev_cfg[i-1].dev_addr = pDevCfg->dev_addr;
    vproc_dev_cfg[i-1].bus_num = pDevCfg->bus_num;
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
    int                 ret=0;
    t_libsif_cfg        sifcfg;
    ssl_op_t         op_type;
    ssl_dev_cfg_t       *pCfg;

    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter (handle 0x%x)...\n",(uint32_t)pHandle);

    if(pPort == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Parameters\n");
        return -1;
    }

   pCfg = &vproc_dev_cfg[(int32_t)pHandle];

   op_type = pPort->op_type;

   memset(&sifcfg,0,sizeof(sifcfg));
   if(pCfg->bus_num)
   {
      PIN_SIF1_NORMAL;
      //PIN_SIF1_SI_NORMAL;
      if(pCfg->dev_addr)
         PIN_SIF1_SEL_CS1;
      else
         PIN_SIF1_SEL_CS0;
      ret = libsif1_init(&sifcfg,SIF_OP_CPU,SIF_MODE_MASTER,SPI_SPEED);
   }
   else
   {
      //     debug_printf("Selecting PIN_SIF0_GPIO_NORMAL \n");
      //this setting routing signals to SIF flash
      //PIN_SIF0_GPIO_NORMAL;
      //check if this setting route signals to J2
      // PIN_SIF0_SC_NORMAL;
        PIN_SIF0_GPIO_NORMAL;
        ret = libsif0_init(&sifcfg,SIF_OP_CPU,SIF_MODE_MASTER,SPI_SPEED);
   }
   if(ret <0)
   {
      VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"libsif%d_init failed\n",pCfg->bus_num);
      return ret;
   }
   CS_LOW(pCfg->bus_num);
   if(op_type & SSL_OP_PORT_WR)
   {
      if(pPort->pSrc == NULL)
      {
         VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL src buffer passed\n");
         CS_HI(pCfg->bus_num);
         return -1;
      }
      ret = libsif_write(&sifcfg,pPort->pSrc,pPort->nwrite);
   }

    if(op_type & SSL_OP_PORT_RD)
    {
        if(pPort->pDst == NULL)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL destination buffer passed\n");
		    CS_HI(pCfg->bus_num);
            return -1;
        }
       ret = libsif_read(&sifcfg,pPort->pDst,pPort->nread);

    }
    CS_HI(pCfg->bus_num);
    if(ret < 0)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"failed with Error %d\n",ret);
    }

    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit..\n");
    return ret;
}



