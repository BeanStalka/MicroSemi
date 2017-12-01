#include "typedefs.h"
#include "chip.h"
#include "hbi.h"
#include "app_util.h"

#define HEADER_STRING(s) #s
#define HEADER_I(name) HEADER_STRING(name)
#define HEADER(name) HEADER_I(name)

#if CONFIG_ZL380XX_HBI_LOAD_CFGREC
#if CONFIG_ZL380XX_HBI_LOAD_CFGREC_STATIC
#ifdef CFGREC_C_FILE
#include HEADER(CFGREC_C_FILE)
#else
# error "config record file not defined"
#endif
#endif
#endif

#if CONFIG_ZL380XX_HBI_I2C
static int bus_num = 0;
static int dev_id = 0x45;
#else
static int bus_num = 0;
static int dev_id = 0;
#endif

#ifdef printf
#undef printf
#define printf debug_printf
#endif

#undef SAVE_FWRCFG_TO_FLASH /*define this macro to save the firmware from RAM to flash*/

#define DBG_BOOT 0
#define DBG_CFGREC 0


static hbi_handle_t handle;


#define CLUSTER_INDEX_BUF     (0x10000000+1024*60) //0x10000000: 2M start
#define MAX_SECTOR_CNT        ((0x10000-10240*2)/512) //0x10000: max size 64KB
static void sd_init(void)
{
    unsigned int ret = app_scif_init();
    ///< check card
    debug_printf("init SCIF %d\n", ret);
    if(ret <= 10)
        debug_printf("init sd error!\n");

    debug_printf("Sys init\n");

    libefs_init(CLUSTER_INDEX_BUF, MAX_SECTOR_CNT, fs_ncf_init_scif);

    debug_printf("free cluster count2 = %d\n", FSFreeClusterCount);
}


/*This example host app load the firmware to the device RAM. Optionally save it to flash
 * Then start the firmware from the execution address in RAM
 */
int hbi_ldfwrcfg(int argc, char** argv) {
    hbi_status_t status = HBI_STATUS_SUCCESS;
    hbi_dev_cfg_t devcfg;

   unsigned char val[32];
    
#if (DBG_BOOT || DBG_CFGREC)
    int j,i;
    size_t size=12;
#endif

    status = HBI_init(NULL);
    if (status != HBI_STATUS_SUCCESS)
    {
        printf("HBI_init Failed!\n");
        
        return -1;
    }

    devcfg.dev_addr = dev_id;
    devcfg.bus_num = bus_num;

    status = HBI_open(&handle,&devcfg);

    if (status != HBI_STATUS_SUCCESS) {
        printf("Error %d:HBI_open()\n", status);
        HBI_term();
        return -1;
    }
    
#if DBG_BOOT
            /* take firmware product code information before updating */
            printf("Firmware Product code before\n");
            status = HBI_read(handle,0x0020,val,size);
            if(status == HBI_STATUS_SUCCESS)
            {
                for(i=0;i<size;i++)
                    printf("0x%x\t",val[i]);
               printf("\n");
            }
#endif


#if CONFIG_ZL380XX_HBI_BOOT
#if CONFIG_ZL380XX_HBI_BOOT_STATIC
    status=ldfwr(handle,NULL);
#else

   sd_init();
   status = ldfwr(handle,"test.bin");
   ioman_flushAll(&sh_efs.myIOman);

#endif
#endif

#ifdef CONFIG_ZL380XX_HBI_LOAD_CFGREC
   printf("Calling LoadCfgRec() ..\n");

#if CONFIG_ZL380XX_HBI_LOAD_CFGREC_STATIC
    cfgrec_t configRec;

    configRec.pImage = (void *)st_twConfig;
    configRec.ImageLen = configStreamLen;
    configRec.configBlockSize = zl_configBlockSize;
    status = ldcfgrec(handle,&configRec);
#else
    sd_init();
    status = ldcfgrec(handle,"test.cr2");
    ioman_flushAll(&sh_efs.myIOman);
#endif

    if(status == HBI_STATUS_SUCCESS)
    {
       /*Firmware reset - in order for the configuration to take effect*/

       val[0] = ZL380xx_HOST_SW_FLAGS_APP_REBOOT >> 8;
       val[1] = ZL380xx_HOST_SW_FLAGS_APP_REBOOT & 0xFF;

       status  = HBI_write(handle,0x0006,val,2);
       if (status != HBI_STATUS_SUCCESS) 
       {
           printf("Error %d:Soft Reset Failed()\n", status);
       }
    }
    else
    {
       printf("Error %d:Load Cfgrec failed\n", status);
    }
#endif

#ifdef SAVE_FWRCFG_TO_FLASH
    if(status == HBI_STATUS_SUCCESS)
    {
       printf("-- Saving firmware / config to flash....\n");
       status = HBI_set_command(handle,HBI_CMD_SAVE_FWRCFG_TO_FLASH,NULL);
       if (status != HBI_STATUS_SUCCESS) 
       {
            printf("Error %d:HBI_CMD_SAVE_FWRCFG_TO_FLASH failed\n", status);
       }
       else
       {
        printf("-- Saving to flash....done\n");
       }
    }
#endif

#if CONFIG_ZL380XX_HBI_BOOT
    if(status == HBI_STATUS_SUCCESS)
    {
      printf("- Start the image to RAM....done\n");

      status  = HBI_set_command(handle, HBI_CMD_START_FWR,NULL);
      if (status == HBI_STATUS_SUCCESS) 
      {
#if DBG_BOOT
         status = HBI_read(handle,0x0020,val,size);
         if(status == HBI_STATUS_SUCCESS)
         {
            for(i=0;i<size;i++)
               printf("0x%x\t",val[i]);
            printf("\n");
         }
#endif
      }
      else
         printf("Error %d:HBI_CMD_START_FWR failed\n", status);
    }
#endif /* CONFIG_ZL380XX_HBI_BOOT */

#if DBG_CFGREC
     /* Read back cfg record*/
     printf("Read back config record\n");
     char cfgrecval[256];
     size_t len=256;
     for(i=0x200;i<0x1000;i+=len)
     {
         status = HBI_read(handle,(reg_addr_t)i,(user_buffer_t *)cfgrecval,len);
         if(status == HBI_STATUS_SUCCESS)
         {
             for(j=0;j<len;j+=2)
                 printf("0x%x,0x%x\n",i+j,*((u16*)&cfgrecval[j]));
         }
     }
#endif

    HBI_close(handle);
    HBI_term();

    return 0;
}


