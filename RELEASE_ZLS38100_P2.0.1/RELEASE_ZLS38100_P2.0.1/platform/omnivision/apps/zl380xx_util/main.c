#include "app_includes.h"

extern void taskone_main(void);
extern void tasktwo_main(void);
void  OS_TaskOne (void *argv)
{

	debug_printf("task one start\n");

	//start tick
	tick1_set_rt(sysclk_get() / TICKS_PER_SEC);
	ENABLE_TICK;

	taskone_main();
}


#define TASK_ONE_STK_SIZE	256
#define TASK_TWO_STK_SIZE	256
static u32 TaskOneStk[TASK_ONE_STK_SIZE];
static u32 TaskTwoStk[TASK_TWO_STK_SIZE];

t_signal *evt_mb;

NEWOS_CONFIG(2, 2)

struct task_arg{
    void **argv;
    int argc;
};
struct task_arg targs;

	/*---------------------------------------------------------------------------*/
int main(void)
{
    
	WATCHDOG_DISABLE;
	int i;

	/* clear rtos bss */
	BSS_CLEAR();
	/* clear com bss */
	COM_CLEAR();
	
	WATCHDOG_DISABLE;
	SC_RESET_RELEASE(SC_RESET_ALL);
	SC_CLK_SET(SC_CLK_ALL);

	// config system clock 144M
	CLOCK_CONFIG();
	uart_config(115200, UART_IER_RLSI | UART_IER_RDI);

#ifdef CONFIG_ICACHE
	ic_enable();
	uart_putc('I');
#endif

#ifdef CONFIG_DCACHE
	dc_enable();
	uart_putc('D');
#endif

	debug_printf("bootloader2 here\n");

//	enable_irq(UART);

	for (i=0; i<TASK_ONE_STK_SIZE; i++) {
		TaskOneStk[i] = 0xffffffff;
	}

	for (i=0; i<TASK_TWO_STK_SIZE; i++) {
		TaskTwoStk[i] = 0xffffffff;
	}

	init_newos();

	evt_mb = newos_signal_create (NULL);
	
//	task_create(tasktwo_main, (void *)0, &TaskTwoStk[TASK_TWO_STK_SIZE - 1], 7);
	task_create(OS_TaskOne, (void *)0, &TaskOneStk[TASK_ONE_STK_SIZE - 1], 1);



	newos_start();

	while(1);
}
