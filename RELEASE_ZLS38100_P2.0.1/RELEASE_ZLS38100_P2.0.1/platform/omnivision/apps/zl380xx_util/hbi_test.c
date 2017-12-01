#include "typedefs.h"
#include "chip.h"
#include "hbi.h"

#if CONFIG_ZL380XX_HBI_I2C
static int bus_num = 0;
static int dev_id = 0x45;
#else
static int bus_num = 0;
static int dev_id = 0;
#endif


static hbi_handle_t handle;

#define TEST_RST 
//#define TEST_LOAD_FWRCFG_FROM_FLASH 
//#define TEST_ERASE_FLASH

#undef printf

#ifndef printf
#define printf debug_printf
#endif

int hbi_test(int argc, char** argv) {

    hbi_status_t status = HBI_STATUS_SUCCESS;
    hbi_dev_cfg_t devcfg;

    status = HBI_init(NULL);
    if (status != HBI_STATUS_SUCCESS)
    {
        printf("HBI_init failed\n");
        return -1;
    }

    devcfg.dev_addr = bus_num;
    devcfg.bus_num = dev_id;
    
    status = HBI_open(&handle,&devcfg);
    if(status != HBI_STATUS_SUCCESS)
    {
        printf("dev open error\n");
        return -1;
    }

#ifdef TEST_RST
{   /*for RESETTING ZL380xx*/         
#define TEST_BUF_SIZE 12
    uint8_t val[TEST_BUF_SIZE];
    reg_addr_t reg=0x0020;
    int i;
   
    printf("value @ 0x%x before...\n",reg);
    
    status = HBI_read(handle,reg,(user_buffer_t *)val,sizeof(val));
    if(status == HBI_STATUS_SUCCESS)
    {
        for(i=0;i<sizeof(val);i++)
            printf("0x%x\t", val[i]);
    }
    val[0] = ZL380xx_CLK_STATUS_HWRST >> 8;
    val[1] = ZL380xx_CLK_STATUS_HWRST & 0xFF;
    
    status  = HBI_write(handle,ZL380xx_CLK_STATUS_REG,(user_buffer_t *)val,2);        
    if (status != HBI_STATUS_SUCCESS) {
        printf("Error %d:HBI_reset()\n", status);
        HBI_close(handle);
        HBI_term();
        return -1;
    }
    printf("\nDevice reset completed successfully...\n");

    printf("value @ 0x%x after ....\n",reg);
    
    status = HBI_read(handle,reg,(user_buffer_t *)val,sizeof(val));
    if(status == HBI_STATUS_SUCCESS)
    {
        for(i=0;i<sizeof(val);i++)
            printf("0x%x\t", val[i]);
    }
}
#endif
#ifdef TEST_LOAD_FWRCFG_FROM_FLASH
{    
    /*Load ZL380x0 firmware + related config record from flash*/
    unsigned short image_num = 1;
    status  = HBI_set_command(handle,HBI_CMD_LOAD_FWRCFG_FROM_FLASH,&image_num);
    if (status != HBI_STATUS_SUCCESS) {
        printf("Error %d:HBI_set_command(HBI_CMD_LOAD_FWRCFG_FROM_FLASH)\n", status);
        HBI_close(handle);
        HBI_term();
        return -1;
    }
    printf("Device boot loading from flash completed successfully...\n");
}
#endif

#ifdef TEST_ERASE_FLASH
{ 
    /*Erase the full content of the ZL380x0 controlled slave flash*/              
    status  = HBI_set_command(handle, HBI_CMD_ERASE_WHOLE_FLASH, NULL);        
    if (status != HBI_STATUS_SUCCESS) {
        printf("Error %d:HBI_set_command(HBI_CMD_ERASE_WHOLE_FLASH)\n", status);
        HBI_close(handle);
        HBI_term();
        return -1;
    }
    printf("flash erasing completed successfully...\n");         
} 
#endif
    HBI_close(handle);
    HBI_term();

    return 0;
}



