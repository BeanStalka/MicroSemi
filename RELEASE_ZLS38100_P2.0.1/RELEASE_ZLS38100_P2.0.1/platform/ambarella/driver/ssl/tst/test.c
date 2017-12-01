#include <ssl.h>

#define CHK_STATUS(status) if(status != SSL_STATUS_OK) {SSL_print(SSL_DBG_LVL_ERR,"Call failed.Err code %d\n",status);}

static int __init ssl_test_init(void)
{
    SSL_PORT_HANDLE handle;
    SSL_DEV_CFG     devcfg = {.dev_addr = 0x45,.dev_name = "zl38040",.bus_num=1};

    SSL_print_set_lvl(SSL_DBG_LVL_ALL);

    SSL_print(SSL_DBG_LVL_INFO,"initialising ssl\n");

    CHK_STATUS(SSL_init());
    CHK_STATUS(SSL_port_open(&handle,&devcfg));
    CHK_STATUS(SSL_port_close(handle));
    return 0;
}
static void __exit ssl_test_exit(void)
{
    CHK_STATUS(SSL_term());
    return ;
}

module_init(ssl_test_init);
module_exit(ssl_test_exit);

MODULE_AUTHOR("Shally Verma <shally.verna@microsemi.com>");
MODULE_DESCRIPTION(" Microsemi Timberwolf Voice Processor I2C Driver");
MODULE_LICENSE("GPL");


