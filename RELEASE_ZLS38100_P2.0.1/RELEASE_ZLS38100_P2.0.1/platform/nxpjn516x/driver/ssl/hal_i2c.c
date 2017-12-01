/*
Header files defining a "Hardware Abstraction Layer for Voice processor devices
over linux kernel 3.18.x i2c core driver. Every successful call would return 0 or 
a linux error code as defined in linux errno.h
*/
#include "typedefs.h"
#include "ssl.h"
#include "hal.h"
#include "vproc_dbg.h"
#include "AppHardwareApi.h"

#define NXP_JN516X_PERIPHERAL_CLOCK  16000000 /*That's the peripheal clock supported by the JN516x*/
#define DESIRED_I2C_SPEED 100000   /*NXP_J516x and ZL380xx support up to 400KHz*/
#define SPI_CLOCK_DIVIDER  (NXP_JN516X_PERIPHERAL_CLOCK/SPI_SPEED)
#define JN516x_CLK_PRESCALER ((NXP_JN516X_PERIPHERAL_CLOCK/(5*DESIRED_I2C_SPEED)) - 1)

// i2c bus clk = 16MHz / ((Prescaler + 1) * 5)
//   or
// Prescaler = ((16MHz / i2c bus clk) / 5) - 1
// Prescaler = ((16000kHz / 400kHz) / 5) - 1 = ( 40 / 5) - 1 =  8 - 1 = 7
// Prescaler = ((16000kHz / 100kHz) / 5) - 1 = (160 / 5) - 1 = 32 - 1 = 31
// Prescaler = ((16000kHz /  80kHz) / 5) - 1 = (200 / 5) - 1 = 40 - 1 = 39
// Prescaler = ((16000kHz /  40kHz) / 5) - 1 = (400 / 5) - 1 = 80 - 1 = 79

ssl_dev_cfg_t vproc_dev_cfg[VPROC_MAX_NUM_DEVS];
int8_t vproc_dev_busy[VPROC_MAX_NUM_DEVS];
int hal_init(void)
{
    uint8 u8PreScaler = JN516x_CLK_PRESCALER;  // default 100KHz
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter...\n");
    memset(vproc_dev_busy,0,sizeof(vproc_dev_busy));
    memset(vproc_dev_cfg,0,sizeof(vproc_dev_cfg));
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit\n");
    /*The JN51x supports up to 3 SPI slaves*/
    if (VPROC_MAX_NUM_DEVS > 3) {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"The JN51x supports up to 3 SPI slaves\n");
        return -1;
    }
    if (u8PreScaler != 31)
    {
        u8PreScaler = 7; /*400Khz*/                
    }
    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"JN516x_CLK_PRESCALER= %d \n", u8PreScaler);
    vAHI_SiMasterConfigure(TRUE, FALSE, u8PreScaler);
/*   bool_t bPulseSuppressionEnable,
 *   bool_t bInterruptEnable,
 *   uint8 u8PreScaler */
    vAHI_SiSetLocation(FALSE);   // TRUE=use DIO16-17, FALSE=use DIO14-15 (default)      
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

static int checkForCompletion(void) {
    while (bAHI_SiMasterPollTransferInProgress());    
#if 0    
    if (bAHI_SiMasterCheckRxNack())
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"No ACK from Slave\n");
        return TRUE;
    }
#endif
    return 0;
}

static int jn516xI2cStop()
{
    /*I2C_STOP*/
    if (bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
                           E_AHI_SI_STOP_BIT,
                           E_AHI_SI_NO_SLAVE_READ,
                           E_AHI_SI_NO_SLAVE_WRITE,
                           E_AHI_SI_SEND_NACK,
                           E_AHI_SI_NO_IRQ_ACK))
    {
        if (checkForCompletion())
        {
            return -1;
        }
    } else {
         VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"I2C protocol config is invalid\n");
         return -1;
    }        
}

/*Sub-routine to write 1 or up to 256 8-bits words to the I2C bus*/
static int nxpI2cMwordsWrite(uint8 i2c_addr, uint8 numbytes, uint8_t *pData) 
{
    int i =0;
    if (bAHI_SiMasterPollBusy())
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"I2C master is busy\n");
        return -1;
    }
    /*I2C_START+ADDR*/
    vAHI_SiMasterWriteSlaveAddr(i2c_addr, FALSE);  
    if (bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
                           E_AHI_SI_NO_STOP_BIT,
                           E_AHI_SI_NO_SLAVE_READ,
                           E_AHI_SI_SLAVE_WRITE,
                           E_AHI_SI_SEND_ACK,
                           E_AHI_SI_NO_IRQ_ACK))
    {
        if (checkForCompletion())
        {
            return -1;
        }
    } else {
         VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"I2C protocol config is invalid\n");
         return -1;
    }
    /*I2C_WRITE*/
    for (i=0; i<numbytes; i++) {
        vAHI_SiMasterWriteData8(*pData++);
        if (bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
                               E_AHI_SI_NO_STOP_BIT,
                               E_AHI_SI_NO_SLAVE_READ,
                               E_AHI_SI_SLAVE_WRITE,
                               E_AHI_SI_SEND_ACK,
                               E_AHI_SI_NO_IRQ_ACK))
        {
            if (checkForCompletion())
            {
                return -1;
            }
        } else {
             VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"I2C protocol config is invalid\n");
             return -1;
        }        
    }
    return 0;
}

/*Sub-routine to read 1 or up to 256 8-bits words from the I2C bus*/
static int nxpI2cMwordsRead(uint8 i2c_addr, uint8 numbytes, uint8_t *pData) 
{
    if (bAHI_SiMasterPollBusy())
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"I2C master is busy\n");
        return -1;
    }
    /*I2C_START+ADDR*/
    vAHI_SiMasterWriteSlaveAddr(i2c_addr, TRUE);  
    if (bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
                           E_AHI_SI_NO_STOP_BIT,
                           E_AHI_SI_NO_SLAVE_READ,
                           E_AHI_SI_SLAVE_WRITE,
                           E_AHI_SI_SEND_ACK,
                           E_AHI_SI_NO_IRQ_ACK))
    {
        if (checkForCompletion())
        {
            return -1;
        }
    } else {
         VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"I2C protocol config is invalid\n");
         return -1;
    }
    /*I2C_READ*/
    
	while (numbytes > 0) {
        if (bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
                               E_AHI_SI_NO_STOP_BIT,
                               E_AHI_SI_SLAVE_READ,
                               E_AHI_SI_NO_SLAVE_WRITE,
                               E_AHI_SI_SEND_ACK,
                               E_AHI_SI_NO_IRQ_ACK))
        {
            if (checkForCompletion())
            {
                return -1;
            }
        } else {
             VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"I2C protocol config is invalid\n");
             return -1;
        }
        *pData++ = u8AHI_SiMasterReadData8(); 
		numbytes -= 1;		
    }
    return 0;
}

int hal_port_rw(void *pHandle,ssl_port_access_t *pPort)
{
    int                 ret=0;
    ssl_op_t            op_type;
    ssl_dev_cfg_t       *pCfg;

    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Enter (handle 0x%x)...\n",(uint32_t)pHandle);

    if(pPort == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Parameters\n");
        return -1;
    }

   pCfg = &vproc_dev_cfg[(int32_t)pHandle];

   op_type = pPort->op_type;

   if(op_type & SSL_OP_PORT_WR)
   {
      if(pPort->pSrc == NULL)
      {
         VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL src buffer passed\n");

         return -1;
      }
      ret = nxpI2cMwordsWrite(pCfg->dev_addr, pPort->nwrite, pPort->pSrc);
   }

    if(op_type & SSL_OP_PORT_RD)
    {
        if(pPort->pDst == NULL)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL destination buffer passed\n");
            jn516xI2cStop();
            return -1;
        }
        ret = nxpI2cMwordsRead(pCfg->dev_addr, pPort->nread, pPort->pDst);

    }
    /*I2C_STOP*/
    jn516xI2cStop();      

    if(ret < 0)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"failed with Error %d\n",ret);
    }

    VPROC_DBG_PRINT(VPROC_DBG_LVL_FUNC,"Exit..\n");
    return ret;
}



