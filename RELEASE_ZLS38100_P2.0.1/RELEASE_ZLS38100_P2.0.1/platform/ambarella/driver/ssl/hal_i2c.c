/*
* hal_i2c.c - i2c driver implmentation for HBI
*
* Hardware Abstraction Layer for Voice processor devices
* over linux kernel 3.18.x i2c core driver. Every successful call would return 0 or
* a linux error code as defined in linux errno.h
*
* Copyright 2016 Microsemi Inc.
*
* This program is free software you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option)any later version.
*/

/*for SDK that only supports device tree driver registration*/
#define SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/

/* Ambarella-S2L I2S driver achitecture requires that the codec device driver registration
 * be either spi or I2C, it can not be platform since ambarella already provided a platform
 * driver that requires that both the spi/i2c codec spi/i2c device driver be registered as one unit
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>

#include <linux/i2c.h>
#include "typedefs.h"
#include "ssl.h"
#include "hal.h"
#include "vproc_dbg.h"

#define HAL_DEBUG 0


int vproc_i2c_probe(struct i2c_client *, const struct i2c_device_id *);
int vproc_i2c_remove(struct i2c_client *);
//int vproc_i2c_detect(struct i2c_client *, struct i2c_board_info *);

/* array containing device names supported by this driver */
static struct i2c_device_id vproc_i2c_device_id[] = {
    {"zl380xx", 0 },
    {}
 };
/* array containing device addresses supported by this driver */
//static unsigned short vproc_i2c_addr[VPROC_MAX_NUM_DEVS+1];
int dev_id = 0;


#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
static struct i2c_client i2cClient[VPROC_MAX_NUM_DEVS];
/*IMPORTANT note: Change this controller string maching accordingly per your *.dts or *dtsi compatible definition file*/
static struct of_device_id vproc_of_match[] = {
	{ .compatible = "ambarella,zl380i2c0",},
	{},
};
MODULE_DEVICE_TABLE(of, vproc_of_match);
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/

/* Structure definining i2c driver */
struct i2c_driver vproc_i2c_driver = {
    .driver = {
        .name = VPROC_DEV_NAME,
        .owner = THIS_MODULE,
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
        .of_match_table = vproc_of_match,
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/
    },
    .probe = vproc_i2c_probe,
    .remove = vproc_i2c_remove,
    .id_table = vproc_i2c_device_id,
//    .address_list = vproc_i2c_addr
};

int vproc_i2c_probe(struct i2c_client *pClient, const struct i2c_device_id *pDeviceId)
{
    /* Bind device to driver */

    pClient->dev.driver = &(vproc_i2c_driver.driver);
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    i2cClient[dev_id++] = *pClient;
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"i2c device %d found at addr:0x%02X\n", dev_id, pClient->addr);
#endif
    return 0;
}

int vproc_i2c_remove(struct i2c_client *pClient)
{
    return 0;
}


int hal_init(void)
{
    int status = i2c_add_driver(&vproc_i2c_driver);
    dev_id=0;
    return status;
}

int hal_term()
{
   i2c_del_driver(&vproc_i2c_driver);
   dev_id=0;
   return 0;
}

#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
/*check whether the device to be openend is valid*/
int getI2cClientId(ssl_dev_cfg_t *pDev) {
    int i = 0;
    for (i=0; i< VPROC_MAX_NUM_DEVS; i++)
    {
        if (i2cClient[i].addr == pDev->dev_addr)
        {
            dev_id = i;
            return i;
        }
    }
    return -1;
}
#endif
/*
    This function opens an i2c client to a device
    identifies via "devcfg" parameter.
    Returns 0 and update client into a reference handle, if successful
    or an error code for an invalid parameter or client instantiation
    fails.
*/
int hal_open(void **ppHandle,void *pDevCfg)
{
   struct i2c_adapter      *pAdap=NULL;
   struct i2c_client  *pClient = NULL;
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
   struct i2c_board_info   bi;
#endif
   ssl_dev_cfg_t *pDev = (ssl_dev_cfg_t *)pDevCfg;



   if(pDev == NULL)
   {
      VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Device Cfg Reference\n");
      return -EINVAL;
   }

#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    /*check whether the device to be openend is valid*/
    if (getI2cClientId(pDev) < 0)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Device Reference\n");
        return -EINVAL;
    }
    pClient = &i2cClient[dev_id];
#endif

   pAdap = i2c_get_adapter(pDev->bus_num);
   if(pAdap==NULL)
   {
     VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Bus Num \n");
     return SSL_STATUS_INVALID_ARG;
   }

   VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"i2c adap name %s \n",pAdap->name);

   pClient->addr = pDev->dev_addr;
   if(pDev->pDevName != NULL)
   {
        strcpy(pClient->name,pDev->pDevName);
   }
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
   memset(&bi,0,sizeof(bi));
    if(pDev->pDevName != NULL)
   {
     strcpy(bi.type,pDev->pDevName);
   }
   bi.addr = pDev->dev_addr;

    pClient = i2c_new_device(pAdap,(struct i2c_board_info const *)&bi);
   //pClient = i2c_new_probed_device(pAdap, (struct i2c_board_info const *)&bi, normal_i2c, NULL);
#endif
   if(pClient == NULL)
   {
     /* call failed either because address is invalid, valid but occupied
        or there is resource err. Just return code to try again later */
     VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"i2c device instantiation failed\n");
     return -EAGAIN;
   }

   *((struct i2c_client **)ppHandle) =  pClient;
   VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"Opened i2c device %d addr:0x%02X...\n", dev_id, pClient->addr);

   return 0;
}

int hal_close(void *pHandle)
{
    if(pHandle == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL client handle passed\n");
        return -EINVAL;
    }
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    i2c_unregister_device((struct i2c_client *) pHandle);
#endif
    return 0;
}

int hal_port_rw(void *pHandle,void *pPortAccess)
{
   int ret=0;
   ssl_port_access_t *port = (ssl_port_access_t *)pPortAccess;
   struct i2c_client *pClient = pHandle;
   struct i2c_msg msg[2];
   int msgnum=0;
   ssl_op_t op_type;
   int i;

   if(pHandle == NULL || pPortAccess == NULL)
   {
      VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Parameters\n");
      return -EINVAL;
   }

   op_type = port->op_type;

   if(op_type & SSL_OP_PORT_WR)
   {
      if(port->pSrc == NULL)
      {
         VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL src buffer passed\n");
         return -EINVAL;
      }
      msg[0].addr = pClient->addr;
      msg[0].flags = 0;
      msg[0].buf = port->pSrc;
      msg[0].len = port->nwrite;

      VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"writing %d bytes..\n",msg[0].len);
      for(i=0;i<port->nwrite;i++)
      {
         printk("0x%x\t",((uint8_t *)(port->pSrc))[i]);
      }
      printk("\n");

      msgnum++;
   }
   if(op_type & SSL_OP_PORT_RD)
   {
      if(port->pDst == NULL)
      {
         VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL destination buffer passed\n");
         return -EINVAL;
      }
      msg[msgnum].addr = pClient->addr;
      msg[msgnum].flags = I2C_M_RD;
      msg[msgnum].buf = port->pDst;
      msg[msgnum].len = port->nread;
      msgnum++;

      VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"read %d bytes..\n",msg[1].len);
   }

   ret = i2c_transfer(pClient->adapter,msg,msgnum);
   if(ret < 0)
   {
      VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"failed with Error 0x%x\n",ret);
   }
#if HAL_DEBUG
   if(!ret && (msgnum >=1))
   {
      printk("Received...\n");
      for(i=0;i<port->nread;i++)
      {
         printk("0x%x\t",((uint8_t *)(port->pDst))[i]);
      }
      printk("\n");
   }
#endif
   return ret;
}




