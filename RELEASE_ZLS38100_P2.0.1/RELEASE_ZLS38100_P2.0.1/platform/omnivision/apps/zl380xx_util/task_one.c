#include "app_includes.h"

#if CONFIG_ZL380XX_TEST_HBI
int hbi_test(int,void **);
#endif
#if CONFIG_ZL380XX_HBI_LOAD_FWRCFG
int hbi_ldfwrcfg(int,void **);
#endif
#define TASKONE_DBG

#define EASYCAM_BOARD

#ifdef EASYCAM_BOARD
//#define V1
#define V2

#ifdef V1
//easycam V1 board
#define ACODEC_RESET_PIN PIN_GPIO_15
#endif

#ifdef V2
//easycam V2 board
#define ACODEC_RESET_PIN PIN_GPIO_LCD_WRB
#endif
#endif

extern t_signal *evt_mb;
void taskone_main(void)
{
	//unsigned int regval;

	ENABLE_INTERRUPT;
	enable_irq(MBX);

#ifdef TASKONE_DBG
	//init uart slave and let BA2 use uart slave
//	enable_irq(UARTS);
//	enable_irq(UART);
//	uarts_init(sysclk_get(), 57600,UART_IER_RLSI | UART_IER_RDI);
	uarts_init(sysclk_get(), 57600, 0);
#endif

#if 1
#ifdef EASYCAM_BOARD
	libgpio_config(ACODEC_RESET_PIN, PIN_DIR_OUTPUT);
	libgpio_write(ACODEC_RESET_PIN, PIN_LVL_HIGH);
	{volatile int d;for(d=0;d<10000;d++);}
	libgpio_write(ACODEC_RESET_PIN, PIN_LVL_LOW);
	{volatile int d;for(d=0;d<10000;d++);}
	libgpio_write(ACODEC_RESET_PIN, PIN_LVL_HIGH);
#endif
#endif

        //wait for signal, it maybe from sec_BA's notification
        //		newos_wait(evt_one, 0, &err);
#ifdef CONFIG_ZL380XX_TEST_HBI
        debug_printf("calling hbi_test !\n");
        hbi_test(0,(void **)NULL);
#endif
#if CONFIG_ZL380XX_HBI_LOAD_FWRCFG
    debug_printf("calling hbi_ldfwrcfg !\n");
    hbi_ldfwrcfg(0,(void **)NULL);
#endif
}
