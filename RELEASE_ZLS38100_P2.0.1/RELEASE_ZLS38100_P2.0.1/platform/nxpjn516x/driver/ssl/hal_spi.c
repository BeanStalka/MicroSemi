/*
Header files defining a "Hardware Abstraction Layer for Voice processor devices
over NXP JN516x SPI/I2C Peripherals API. Every successful call would return 0 or 
an  error code -1
*/
#include "typedefs.h"
#include "ssl.h"
#include "hal.h"
#include "vproc_dbg.h"

VPROC_DBG_LVL vproc_dbg_lvl = VPROC_DBG_LVL_ALL;

#define NXP_JN516X_PERIPHERAL_CLOCK  16000000 /*That's the peripheral clock supported by the JN516x*/

#define SPI_SPEED 2000000   /*NXP_J516x supports up to 16MHz*/

#define SPI_CLOCK_DIVIDER  (NXP_JN516X_PERIPHERAL_CLOCK/SPI_SPEED)

ssl_dev_cfg_t vproc_dev_cfg[VPROC_MAX_NUM_DEVS];
int8_t vproc_dev_busy[VPROC_MAX_NUM_DEVS];
int hal_init(void)
{

    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter hal_init...\n");
    memset(vproc_dev_busy,0,sizeof(vproc_dev_busy));
    memset(vproc_dev_cfg,0,sizeof(vproc_dev_cfg));
     /*The JN51x supports up to 3 SPI slaves*/
    if (VPROC_MAX_NUM_DEVS > 3) {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"The JN51x supports up to 3 SPI slaves\n");
        return -1;
    }
    vAHI_SpiConfigure(VPROC_MAX_NUM_DEVS, FALSE, FALSE, FALSE, SPI_CLOCK_DIVIDER, FALSE, FALSE);
/*     uint8 u8SlaveEnable
 *     bool_t bLsbFirst,
 *     bool_t bPolarity,
 *     bool_t bPhase,
 *     uint8 u8ClockDivider,
 *     bool_t bInterruptEnable,
 *     bool_t bAutoSlaveSelect  */

    vAHI_SpiSelSetLocation(E_AHI_SPISEL_1, FALSE);  /*FALSE: use DIO0 for SS_SEL1*/
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit hal_init ...\n");

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

    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter hal_open ...\n");
    
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
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,
                     "i:%d Received addr 0x%x bus 0x%x\n",
                     i,pDevCfg->dev_addr,pDevCfg->bus_num);
    vproc_dev_cfg[i-1].dev_addr = pDevCfg->dev_addr;
    vproc_dev_cfg[i-1].bus_num = pDevCfg->bus_num;
    vproc_dev_busy[i-1] = 1;

   *((int32_t **)ppHandle) =  (int32_t *)(i-1);
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Return Handle %d\n",i-1);
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit hal_open ...\n");
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

/*Sub-routine to write 1 or up to 256 8-bits words to the SPI bus*/
static int nxpSpiMwordsWrite(uint8 numbytes, uint8_t *pData) 
{
    int i =0;
    vAHI_SpiWaitBusy();
    for (i=0; i<numbytes; i++)
    {
        /*VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"WR: *(pData+%d) = 0x%2x\n", i, *(pData+i)); */
        vAHI_SpiStartTransfer(7, *(pData+i));
    	vAHI_SpiWaitBusy();
    }
    return 0;
}

/*Sub-routine to read 1 or up to 256 8-bits words from the SPI bus*/
static int nxpSpiMwordsRead(uint8 numbytes, uint8_t *pData)
{
    uint8 i =0;   
    for (i=0; i<numbytes; i++)
    {
    	vAHI_SpiStartTransfer(7, 0xFF); /*Send HBI NO-OP=0xFFFF*/
    	vAHI_SpiWaitBusy();
        *(pData+i) = u8AHI_SpiReadTransfer8();
        vAHI_SpiWaitBusy();
        /*VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"RD: *(pData+%d) = 0x%2x\n", i, *(pData+i));*/
    }
    return 0;
}

int hal_port_rw(void *pHandle,ssl_port_access_t *pPort)
{
    int                 ret=0;
    ssl_op_t            op_type;
    ssl_dev_cfg_t       *pCfg;

    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter (hal_port_rw::handle 0x%x)...\n",(uint32_t)pHandle);

    if(pPort == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Parameters\n");
        return -1;
    }

   pCfg = &vproc_dev_cfg[(int32_t)pHandle];

   op_type = pPort->op_type;
   if(op_type & SSL_OP_PORT_WR)
   {
	  /*VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"WR: SlaveSelect = %d \n", (1 << pCfg->dev_addr));*/
      if(pPort->pSrc == NULL)
      {
         VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL src buffer passed\n");
         return -1;
      }
      vAHI_SpiSelect(1 << pCfg->dev_addr);  /*drive the selected Chip select specified by addr LOW*/
      ret = nxpSpiMwordsWrite(pPort->nwrite, pPort->pSrc);
      vAHI_SpiSelect(0);  /*drive the Chip select specified by addr HIGH*/

   }


    if(op_type & SSL_OP_PORT_RD)
    {
        if(pPort->pDst == NULL)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL destination buffer passed\n");
		    vAHI_SpiSelect(0);  /*drive the Chip slect specified by addr HIGH*/
            return -1;
        }
        vAHI_SpiSelect(1 << pCfg->dev_addr);  /*drive the selected Chip select specified by addr LOW*/
        ret = nxpSpiMwordsRead(pPort->nread, pPort->pDst);
   	    /*VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"RD: SlaveSelect = %d \n", (1 << pCfg->dev_addr));*/
        vAHI_SpiSelect(0);  /*drive the Chip select specified by addr HIGH*/

    }
    if(ret < 0)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"failed with Error %d\n",ret);
    }

    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit..SlaveSelect = 0 , hal_port_rw:: ret = %d....\n", ret);
    return ret;
}



