#include "app_includes.h"

#ifdef TICK2_EN
unsigned long tick2_irq_handler(void)
{


}
#endif
void except_tick(void)
{

}

volatile int audio_uart =0;
inline void uart_irq_handler(void)
{
    u8 ch;
    if (uart_getc_nob(&ch) == 0) {
		debug_putc(ch);
		if(ch=='s'){
			uarts_init(sysclk_get(), 57600, 0);
			debug_ba2_internal_on();
		}
		if(ch=='S'){
			debug_ba2_internal_off();
		}
	}
}

inline void uarts_irq_handler(void)
{
    u8 ch;
	uarts_putc('s');
	debug_printf("uarts_irq_handler\n");	
}



IRQ_HANDLER_DECLARE_START


#if defined(CONFIG_DHW_AI2)||defined(CONFIG_AUDIO_EHW_AI2)||defined(CONFIG_AUDIO_DUPLEX_EN)
IRQ_MAP(MBX, libdba_m_hdlmbx);
#endif

#if defined(CONFIG_DHW_AI1)||defined(CONFIG_AUDIO_EHW_AI1)

extern void libadec_irq(void);
extern void libaenc_irq(void);
 
#ifdef CONFIG_AUDIO_ENC_EN
IRQ_MAP(AI1, libaenc_irq);
#endif

#ifdef CONFIG_AUDIO_DEC_EN
IRQ_MAP(AI2, libadec_irq);
#endif

#endif

IRQ_MAP(UART, uart_irq_handler);
IRQ_MAP(UARTS, uarts_irq_handler);
IRQ_HANDLER_DECLARE_END


