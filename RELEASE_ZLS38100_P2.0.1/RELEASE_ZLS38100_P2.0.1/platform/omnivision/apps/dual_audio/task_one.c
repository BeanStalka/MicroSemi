#include "app_includes.h"

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

#ifdef AUDIO_PLAY
extern void audio_play_test(void);
#endif
#ifdef AUDIO_REC
extern void audio_record_test(void);
#endif
#ifdef AUDIO_REC_PLAY
extern void audio_duplex_test(void);
#endif

extern void audio_sys(void);
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


#ifdef EASYCAM_BOARD
	libgpio_config(ACODEC_RESET_PIN, PIN_DIR_OUTPUT);
	libgpio_write(ACODEC_RESET_PIN, PIN_LVL_HIGH);
	{volatile int d;for(d=0;d<10000;d++);}
	libgpio_write(ACODEC_RESET_PIN, PIN_LVL_LOW);
	{volatile int d;for(d=0;d<10000;d++);}
	libgpio_write(ACODEC_RESET_PIN, PIN_LVL_HIGH);
#endif

while(1){
		//wait for signal, it maybe from sec_BA's notification
//		newos_wait(evt_one, 0, &err);
#ifdef TASKONE_DBG
		debug_printf("start audio task!\n");
#endif
#ifdef AUDIO_PLAY
		audio_play_test();
#endif
#ifdef AUDIO_REC
		audio_record_test();	
#endif
#ifdef AUDIO_REC_PLAY
		debug_printf("start audio task!\n");
		audio_duplex_test();	
#endif
	}
}
