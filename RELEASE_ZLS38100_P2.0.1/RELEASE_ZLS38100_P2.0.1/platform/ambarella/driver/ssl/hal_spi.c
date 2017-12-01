/*
* hal_spi.c - spi driver implmentation for HBI
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

#define SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/moduleparam.h>
#include <linux/spi/spi.h>
#include <linux/list.h>
#include "typedefs.h"
#include "ssl.h"
#include "hal.h"
#include "vproc_dbg.h"

#define HAL_DEBUG 0

int vproc_probe(struct spi_device *);
int vproc_remove(struct spi_device *);

/* array containing device names supported by this driver */
static struct spi_device_id vproc_device_id[VPROC_MAX_NUM_DEVS];
int dev_id = 0;

#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
static struct spi_device spiClient[VPROC_MAX_NUM_DEVS];

/*IMPORTANT note: Change this controller string maching accordingly per your *.dts or *dtsi compatible definition file*/
static struct of_device_id vproc_of_match[] = {
	{ .compatible = "ambarella,zl380spi01",},
	{},
};
MODULE_DEVICE_TABLE(of, vproc_of_match);
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/

/* Structure definining spi driver */
struct spi_driver vproc_driver = {
    .id_table = vproc_device_id,
    .probe = vproc_probe,
    .remove = vproc_remove,
    .driver = {
        .name = VPROC_DEV_NAME, /* name field should be equal
                                       to module name and without spaces */
        .owner = THIS_MODULE,
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
        .of_match_table = vproc_of_match,
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/
    }
};

int vproc_probe(struct spi_device *pClient)
{

    /* Bind device to driver */
    pClient->dev.driver = &(vproc_driver.driver);
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"SPI slave device %d found at bus::cs = %d::%d\n", dev_id, pClient->master->bus_num, pClient->chip_select);
    /*setup the desired SPI parameters*/
    /* NOTE: the ZL380xx supports SPI speed up to 25MHz, however, this fast speed requires
     * very stringent signal integrity and shorter layout traces between the host and the ZL
     */
	pClient->mode = SPI_MODE_0;
	pClient->max_speed_hz = 1000000; /*ZL380xx supports SPI speed up to 25000000 - Set as desired*/
	if (spi_setup(pClient) < 0)
	{
		return -EAGAIN;
    }
    spiClient[dev_id++] = *pClient;
#endif

    return 0;
}

int vproc_remove(struct spi_device *pClient)
{
    return 0;
}

int hal_init(void)
{
    int status = spi_register_driver(&vproc_driver);
    dev_id = 0;
    return status;
}

int hal_term()
{
    spi_unregister_driver(&vproc_driver);
    dev_id = 0;
    return 0;
}
/*
    This function opens a SPI client to a device
    identifies via "devcfg" parameter.
    Returns 0 and update client into a reference handle, if successful
    or an error code for an invalid parameter or client instantiation
    fails.
*/

#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
/*check whether the device to be openend is valid*/
static int getSpiClientId(ssl_dev_cfg_t *pDev) {
    int i = 0;
    for (i=0; i< VPROC_MAX_NUM_DEVS; i++)
    {
        if ((spiClient[i].master->bus_num == pDev->bus_num) &&
           (spiClient[i].chip_select == pDev->dev_addr))
        {
            dev_id = i;
            return i;
        }
    }
    return -1;
}
#endif

int hal_open(void **ppHandle,void *pDevCfg)
{
    ssl_dev_cfg_t *pDev = (ssl_dev_cfg_t *)pDevCfg;
    struct spi_master *adap = NULL;
    struct spi_device *pClient = NULL;
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    struct spi_board_info bi;
#endif

    if(pDev == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Device Cfg Reference\n");
        return -EINVAL;
    }
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    /*check whether the device to be openend is valid*/
    if (getSpiClientId(pDev) < 0)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Device Reference\n");
        return -EINVAL;
    }
    pClient = &spiClient[dev_id];
#endif
    /* get the controller driver through bus num */
    adap = spi_busnum_to_master(pDev->bus_num);
    if(adap==NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Bus Num %d \n",pDev->bus_num);
        return SSL_STATUS_INVALID_ARG;
    }

    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"spi adap name %s \n",adap->dev.init_name);


    if(pDev->pDevName != NULL)
    {
        strcpy(pClient->modalias,pDev->pDevName);
    }

#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING

    memset(&bi,0,sizeof(bi));
    if(pDev->pDevName != NULL)
    {
        strcpy(bi.modalias,pDev->pDevName);
    }

    bi.bus_num = pDev->bus_num;
    bi.chip_select = pDev->dev_addr;
    bi.max_speed_hz = 1000000;
    bi.mode = SPI_MODE_0;

    pClient = spi_new_device(adap,&bi);
#endif
    if(pClient == NULL)
    {
        /* call failed either because address is invalid, valid but occupied
           or there is resource err. Just return code to try again later */
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"spi device instantiation failed\n");
        return -EAGAIN;
    }
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,
                     "Updating handle 0x%x back to user\n",(uint32_t)pClient);

   *((struct spi_device **)ppHandle) =  pClient;
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"Opened SPI slave device %d found at bus::cs = %d::%d\n", dev_id, pClient->master->bus_num, pClient->chip_select);

    return 0;
}



int hal_close(void *pHandle)
{
    if(pHandle == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL client handle passed\n");
        return -EINVAL;
    }

    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,
                     "Unregistering client 0x%x\n",(u32) pHandle);

    /*Do not unregister the SPI if using device tree registration*/
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    spi_unregister_device((struct spi_device *) pHandle);
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/
    return 0;
}

int hal_port_rw(void *pHandle,void *pPortAccess)
{
    int                 ret=0;
    ssl_port_access_t   *pPort = (ssl_port_access_t *)pPortAccess;
    struct spi_device  *pClient = (struct spi_device *)pHandle;
    int                 msgnum=0;
    struct spi_transfer xfers[2];
    ssl_op_t          op_type;
#if HAL_DEBUG
    int                  i;
#endif

    if(pHandle == NULL || pPort == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Parameters\n");
        return -EINVAL;
    }

    op_type = pPort->op_type;
    memset(xfers,0,sizeof(xfers));
    if(op_type & SSL_OP_PORT_WR)
    {
        if(pPort->pSrc == NULL)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL src buffer passed\n");
            return -EINVAL;
        }

        xfers[0].tx_buf = pPort->pSrc;
        xfers[0].len = pPort->nwrite;
        //xfers[0].speed_hz = 25000000;

#if HAL_DEBUG
        VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"writing %d bytes..\n",pPort->nwrite);

        for(i=0;i<pPort->nwrite;i++)
        {
            printk("0x%x\t",((uint8_t *)(pPort->pSrc))[i]);
        }
        printk("\n");
#endif
        msgnum++;
    }

    if(op_type & SSL_OP_PORT_RD)
    {
        if(pPort->pDst == NULL)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL destination buffer passed\n");
            return -EINVAL;
        }
        xfers[msgnum].rx_buf = pPort->pDst;
        xfers[msgnum].len = pPort->nread;
        //xfers[msgnum].speed_hz = 25000000;

        msgnum++;

        VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"read %d bytes..\n",pPort->nread);
    }
#if 0
    spi_message_init_with_transfers(&msg,xfers,msgnum);
#endif
    ret  = spi_sync_transfer(pClient,xfers,msgnum);
    if(ret < 0)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"failed with Error %d\n",ret);
    }
    #if HAL_DEBUG
    if(msgnum >=1)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"Received...\n");
        for(i=0;i<pPort->nread;i++)
        {
            printk("0x%x\t",((uint8_t *)(pPort->pDst))[i]);
        }
        printk("\n");
    }
    #endif
    return ret;
}




