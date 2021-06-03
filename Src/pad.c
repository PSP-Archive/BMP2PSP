#include "pad.h"
#include "pg.h"
#include "syscall.h"


u32 new_pad;
u32 old_pad;
u32 now_pad;
ctrl_data_t paddata;

void readpad(void)
{
	static int n=0;
	ctrl_data_t paddata;
	
	sceCtrlRead(&paddata, 1);
	
	now_pad = paddata.buttons;
	new_pad = now_pad & ~old_pad;
	if(old_pad==now_pad){
		n++;
		if(n>=25){
			new_pad=now_pad;
			n = 20;
		}
	}else{
		n=0;
		old_pad = now_pad;
	}
}


void wait_button(void) {
	ctrl_data_t ctl;
	int btn;

	btn=1;
	while(btn!=0){
		pgWaitV();
		sceCtrlRead(&ctl,1);
		btn = ((ctl.buttons & 0xF000) != 0);
	}
	btn=0;
	while(btn==0){
		pgWaitV();
		sceCtrlRead(&ctl,1);
		btn = ((ctl.buttons & 0xF000) != 0);
	}
}
