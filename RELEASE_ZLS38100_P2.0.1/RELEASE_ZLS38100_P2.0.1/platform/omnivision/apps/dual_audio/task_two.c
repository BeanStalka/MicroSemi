#include "app_includes.h"
#include "liba_dec.h"

extern t_signal *evt_int;
void tasktwo_main(void)
{
	volatile int d;	
	while(1){
		for(d=0; d<100000;d++);
//		debug_printf("L");
	}
}
