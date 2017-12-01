/*
* hbi_u.c -  hbi user space driver
*
*
* Copyright 2016 Microsemi Inc.
*/

#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "typedefs.h"
#include "chip.h"
#include "hbi.h"
#include "hbi_prv.h"
#include "hbi_k.h"
#include "vproc_u_dbg.h"



int gHbiFd=-1;
static int gDrvInitialised = FALSE;

/* Variable for current debug level set in the system */
VPROC_DBG_LVL vproc_dbg_lvl = DEBUG_LEVEL;

hbi_status_t HBI_init(hbi_init_cfg_t *pCfg)
{
   int ret=-1;
   hbi_lnx_drv_cfg_t initcfg;

   VPROC_U_DBG_PRINT(VPROC_DBG_LVL_FUNC, "%s Entry..\n",__func__);

   /* initialize user spa*/
   gHbiFd = open("/dev/hbi",O_RDWR);

   if(gHbiFd <0)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "Couldn't open HBI driver Err 0x%x\n",errno);
      return HBI_STATUS_RESOURCE_ERR;
   }

   if(pCfg != NULL)
      initcfg.pCfg = pCfg;

   ret = ioctl(gHbiFd,HBI_INIT,&initcfg);
   if(ret <0)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "Error in calling ioctl command.Err 0x%x\n",errno);
      return HBI_STATUS_INTERNAL_ERR;
   }
   
   if(initcfg.status == HBI_STATUS_SUCCESS)
      gDrvInitialised = TRUE;

   return initcfg.status;
}

hbi_status_t HBI_open(hbi_handle_t *pHandle, hbi_dev_cfg_t *pDevCfg)
{
   int ret;
   hbi_lnx_open_arg_t OpenArg;

   if(gDrvInitialised == FALSE)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "HBI Driver is not initialized\n");
      return HBI_STATUS_NOT_INIT;
   }

   memset(&OpenArg,0,sizeof(OpenArg));

   OpenArg.pDevCfg = pDevCfg;
   OpenArg.pHandle = pHandle;

   ret = ioctl(gHbiFd,HBI_OPEN,&OpenArg);

   if (ret <0)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "Err 0x%x in HBI_OPEN \n",errno);
      return HBI_STATUS_RESOURCE_ERR;
   }

   if(OpenArg.status == HBI_STATUS_SUCCESS)
   {
      *pHandle = *(OpenArg.pHandle);
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_INFO, "Returned HBI handle 0x%x\n",*pHandle);
   }

   return OpenArg.status;
}

hbi_status_t HBI_close(hbi_handle_t Handle)
{
   int ret;
   hbi_lnx_close_arg_t closeArg;

   if(gDrvInitialised == FALSE)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "HBI Driver is not initialized\n");
      return HBI_STATUS_NOT_INIT;
   }

   memset(&closeArg,0,sizeof(closeArg));

   closeArg.handle = Handle;

   ret = ioctl(gHbiFd,HBI_CLOSE,&closeArg);

   if (ret <0)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "Err 0x%x in HBI_CLOSE \n",errno);
      return HBI_STATUS_RESOURCE_ERR;
   }

   return closeArg.status;
}

hbi_status_t HBI_term()
{
   int ret;
   hbi_lnx_drv_term_arg_t termArg;

   if(gDrvInitialised == FALSE)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "HBI Driver is not initialized\n");
      return HBI_STATUS_NOT_INIT;
   }

   memset(&termArg,0,sizeof(termArg));

   ret = ioctl(gHbiFd,HBI_TERM,&termArg);

   if (ret <0)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "Err 0x%x in HBI_TERM \n",errno);
      return HBI_STATUS_RESOURCE_ERR;
   }

   return termArg.status;
}

hbi_status_t HBI_read(hbi_handle_t handle,reg_addr_t reg,user_buffer_t * pData,size_t length)
{
   hbi_lnx_drv_rw_arg_t  rwArg;
   int ret;
   
   if(gDrvInitialised == FALSE)
   {
      return HBI_STATUS_NOT_INIT;
   }
   memset(&rwArg,0,sizeof(rwArg));

   rwArg.handle = handle;
   rwArg.pData = pData;
   rwArg.len = length;
   rwArg.reg = reg;

   ret = ioctl(gHbiFd,HBI_READ,&rwArg);

   if(ret <0)
   {
      return HBI_STATUS_RESOURCE_ERR;
   }

   return rwArg.status;
}

hbi_status_t HBI_write(hbi_handle_t handle,reg_addr_t reg,user_buffer_t * pData,size_t length)
{
   hbi_lnx_drv_rw_arg_t  rwArg;
   int ret;
   
   if(gDrvInitialised == FALSE)
   {
      return HBI_STATUS_NOT_INIT;
   }
   memset(&rwArg,0,sizeof(rwArg));

   rwArg.handle = handle;
   rwArg.pData = pData;
   rwArg.len = length;
   rwArg.reg = reg;

   ret = ioctl(gHbiFd,HBI_WRITE,&rwArg);

   if(ret <0)
   {
      return HBI_STATUS_RESOURCE_ERR;
   }

   return rwArg.status;
}

hbi_status_t HBI_set_command(hbi_handle_t handle,hbi_cmd_t cmd,void *pCmdArgs)
{
   int            ret;
   hbi_status_t   status;

   if(gDrvInitialised == FALSE)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "Driver not initialized\n");
      return HBI_STATUS_NOT_INIT;
   }

   
   switch(cmd)
   {
      case HBI_CMD_LOAD_FWR_FROM_HOST:
      {
         hbi_lnx_send_data_arg_t dataArg;

         memset(&dataArg,0,sizeof(dataArg));

         dataArg.data.pData = ((hbi_data_t *)pCmdArgs)->pData;
         dataArg.data.size  = ((hbi_data_t *)pCmdArgs)->size;
         dataArg.handle = handle;

         ret=ioctl(gHbiFd,HBI_LOAD_FW,&dataArg);
         if(ret < 0)
         {
            VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "call to LOAD_FW failed\n");
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
         {
            status = dataArg.status;
         }
         break;
      }
      case HBI_CMD_LOAD_CFGREC_FROM_HOST:
      {
         hbi_lnx_send_data_arg_t dataArg;

         memset(&dataArg,0,sizeof(dataArg));

         dataArg.data.pData = ((hbi_data_t *)pCmdArgs)->pData;
         dataArg.data.size  = ((hbi_data_t *)pCmdArgs)->size;
         dataArg.handle = handle;

         ret=ioctl(gHbiFd,HBI_LOAD_CFG,&dataArg);
         if(ret < 0)
         {
            VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "call to LOAD_CFG failed\n");
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
         {
            status = dataArg.status;
         }
         break;
      }
      case HBI_CMD_LOAD_FWR_COMPLETE:
      {
         hbi_lnx_ldfw_done_arg_t args;

         args.handle = handle;

         ret = ioctl(gHbiFd,HBI_LOAD_FW_COMPLETE,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;
         break;
      }
      
      case HBI_CMD_START_FWR:
      {
         hbi_lnx_start_fw_arg_t args;

         args.handle = handle;

         ret = ioctl(gHbiFd,HBI_START_FW,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;

         break;
      }
      case HBI_CMD_LOAD_FWRCFG_FROM_FLASH:
      {
         hbi_lnx_flash_load_fwrcfg_arg_t args;

         memset(&args,0,sizeof(args));

         args.handle = handle;
         args.image_num = *((int32_t *)pCmdArgs);

         ret = ioctl(gHbiFd,HBI_FLASH_LOAD_FWR_CFGREC,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;

         break;
      }
      case HBI_CMD_ERASE_WHOLE_FLASH:
      case HBI_CMD_ERASE_FWRCFG_FROM_FLASH:
      {
         hbi_lnx_flash_erase_fwcfg_arg_t args;

         memset(&args,0,sizeof(args));
         args.handle = handle;

         if(cmd == HBI_CMD_ERASE_FWRCFG_FROM_FLASH && (pCmdArgs != NULL))
         {
            args.image_num = *((int32_t *)pCmdArgs);
            cmd = HBI_FLASH_ERASE_FWRCFGREC;
         }
         else
            cmd = HBI_FLASH_ERASE_WHOLE;

         ret = ioctl(gHbiFd,cmd,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;
         break;
      }
      case HBI_CMD_SAVE_FWRCFG_TO_FLASH:
      {
         hbi_lnx_flash_save_fwrcfg_arg_t args;
         memset(&args,0,sizeof(args));
         args.handle = handle;
         ret = ioctl(gHbiFd,HBI_FLASH_SAVE_FWR_CFGREC,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;
         break;
      }
      case HBI_CMD_SAVE_CFG_TO_FLASH:
      {
         hbi_lnx_flash_save_fwrcfg_arg_t args;
         memset(&args,0,sizeof(args));
         args.handle = handle;
         args.image_num = *((int32_t *)pCmdArgs);
         ret = ioctl(gHbiFd,HBI_FLASH_SAVE_CFGREC,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;
         break;
      }

      default:
         status = HBI_STATUS_INVALID_ARG;
   }

   return status;
}

hbi_status_t HBI_reset(hbi_handle_t handle, hbi_rst_mode_t mode)
{
   if(gDrvInitialised == FALSE)
   {
      return HBI_STATUS_NOT_INIT;
   }

   return HBI_STATUS_INTERNAL_ERR;
}

hbi_status_t HBI_sleep(hbi_handle_t handle)
{
   if(gDrvInitialised == FALSE)
   {
      return HBI_STATUS_NOT_INIT;
   }

   return HBI_STATUS_INTERNAL_ERR;
}

hbi_status_t HBI_wake(hbi_handle_t handle)
{
   if(gDrvInitialised == FALSE)
   {
      return HBI_STATUS_NOT_INIT;
   }

   return HBI_STATUS_INTERNAL_ERR;
}


hbi_status_t HBI_get_header(hbi_data_t * pImg,hbi_img_hdr_t * pHdr)
{
   return internal_hbi_get_hdr(pImg,pHdr);
}

