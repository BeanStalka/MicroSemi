#include "app_includes.h"
#include "liba.h"
#include "liba_enc.h"
#include "liba_dec.h"
#define APLAY_EN
#define AREC_EN

//#define REC_TO_FILE
#ifndef LINUX_FIRMWARE
//#define	OUTPCM_BUFLEN	0xe0
//#define INBUF_ONELEN	0xe0*2
#define INLIST_NUM		4
ADEC_ALLOCATE(INBUF_ONELEN, INLIST_NUM, OUTPCM_BUFLEN)
//#define INPCM_BUFLEN	0xe0
//#define OUTBUF_ONELEN	0xe0*2
#define OUTLIST_NUM		4
//AREC_ALLOCATE_STEREO(INPCM_BUFLEN, OUTLIST_NUM, OUTBUF_ONELEN)
AREC_ALLOCATE_MONO(INPCM_BUFLEN, OUTLIST_NUM, OUTBUF_ONELEN)
#endif

#ifndef REC_TO_FILE
#define REC_DATA_LEN (1024*90)
#ifdef AUDIO_REC_PCM
short audio_pcm[REC_DATA_LEN];
int recdatacnt = 0;
#endif
#endif


void audio_duplex_config(void)
{
#ifndef LINUX_FIRMWARE
	/* record config */
	memset(&g_liba_ecfg, 0, sizeof(g_liba_ecfg));
#ifdef CONFIG_DCACHE
	g_liba_ecfg.dc_flush_all_proc = dc_flush_all;
	g_liba_ecfg.dc_invalidate_all_proc = dc_invalidate_all;
#endif
//	AREC_CONFIG_STEREO(8000, 0, 0)
//	AREC_CONFIG_STEREO(16000, 0, 0)
	AREC_CONFIG_MONO(8000,0, 0)
	g_liba_ecfg.evt_one = NULL; //evt_resp;
	g_liba_ecfg.evt_input = NULL;
	g_liba_ecfg.evt_mb = evt_mb;
	g_liba_ecfg.duplex = 1;
	/* playback config */
	memset(&g_liba_dcfg, 0, sizeof(g_liba_dcfg));
//	ADEC_CONFIG(8000, STEREO, 0);
	ADEC_CONFIG(8000, MONO, 0);
	g_liba_dcfg.evt_int = NULL;
	g_liba_dcfg.evt_mb = evt_mb;
	g_liba_dcfg.duplex = 1;
#ifdef CONFIG_AUDIO_I2S_MASTER
	g_liba_dcfg.ai_master = 1;
	g_liba_ecfg.ai_master = 1;
#else
	g_liba_dcfg.ai_master = 0;
	g_liba_ecfg.ai_master = 0;
#endif
#endif
}
#define CLUSTER_INDEX_BUF     (0x10000000+1024*60) //0x10000000: 2M start
#define MAX_SECTOR_CNT        ((0x10000-10240*2)/512) //0x10000: max size 64KB

void sd_init(void)
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


#define PLAY_SINE_DATA //if not define this macro, read pcm data from file.

#ifdef PLAY_SINE_DATA		
static short sinwave[] = 
{
#if 0
#include "8km.txt"
#else
	0,
-11613,
-16422,
-11613,
0,
11613,
16422,
11612
#endif
};

#define SWAP2BYTES(a)  (((a)<<8)+(((a)>>8)&0xff))
//#define SWAP2BYTES(a)  a
int sptr = 0;
void fill_sinedata(short *pcm, int len)
{
	   int i;
	   for(i=0;i<len;i++)
	   {
	   	    pcm[i] = SWAP2BYTES(sinwave[sptr++]);
	   	    if(sptr >= sizeof(sinwave)/2)
	   	    	 sptr = 0;	   	    
	   }
}
#endif

int dont_break = 0;

void audio_duplex_test(void)
{
	int err;
	u32 frm_addr;
	u32 frmlen;
	u32 aenc_frmaddr, aenc_frmlen;
	u32 arec_loop;
#ifndef PLAY_SINE_DATA	
	File fp_in;	
#endif
	File fp_out;
	dont_break = 0;
	

ad_start :
	arec_loop = 0;
	audio_duplex_config();
//	debug_ba2_internal_on();

#ifdef REC_TO_FILE
	sd_init();
#ifndef PLAY_SINE_DATA	
	if(file_fopen(&fp_in, &sh_efs.myFs, "11km.pcm", MODE_READ)!= 0)
	{
		debug_printf("pcm file open error\n");
	}
	debug_printf("pcm file open ok\n");
#endif
#ifdef AUDIO_REC_PCM
	if(file_fopen(&fp_out, &sh_efs.myFs, "drec16k.pcm", MODE_WRITE)!= 0)
	{
		debug_printf("dplxrec_16k.pcm file open error\n");
	}else{
		debug_printf("dplxrec_16k.pcm file open ok\n");
	}
#endif
#ifdef AUDIO_REC_AAC
	if(file_fopen(&fp_out, &sh_efs.myFs, "dplxrec_16K.aac", MODE_WRITE)!= 0)
	{
		debug_printf("dplxrec_16K.aac file open error\n");
	}else{
		debug_printf("dplxrec_16K.aac file open ok\n");
	}
#endif
#else
#ifdef AUDIO_REC_PCM
	memset(audio_pcm, 0, REC_DATA_LEN*2);
	recdatacnt = 0;
#endif
#endif //#ifdef REC_TO_FILE 

	debug_printf("test joerrrrrrrrrrrrrrrrr\n");
//	libadec_init();
//	libadec_start();
	libaenc_init();
	libaenc_start();
	{volatile int d;for(d=0;d<100000;d++);}
	libaenc_get_frm(&aenc_frmaddr, &aenc_frmlen);
//	g_aenc_frmlist_head->status = AREC_FRAME_STATUS_DONE;		
//	err = libaenc_update();
//	libadec_stop();
//	libadec_exit();
	libaenc_stop();
//	libaenc_exit();
//#endif

#ifdef APLAY_EN
	libadec_init();
	libadec_start();
#endif

#ifdef AREC_EN
	libaenc_init();
	libaenc_start();
#endif

	while(1){
aplay:
#ifdef APLAY_EN
		//debug_printf("p");
		frm_addr = libadec_inbuf_get();			
		if(!frm_addr){
			goto arec;
			//continue;
		}
#ifdef PLAY_SINE_DATA
		fill_sinedata((short *)frm_addr,  g_liba_dcfg.inbuf_onelen/2);
#else	
		unsigned int read_bytes = file_read(&fp_in, g_liba_dcfg.inbuf_onelen, (u8 *)frm_addr);
		if(read_bytes < g_liba_dcfg.inbuf_onelen){
			debug_printf("no more data!%d,%d\n", read_bytes, g_liba_dcfg.inbuf_onelen);
			break;
		}
#endif		
		err = libadec_frm_add(frm_addr, g_liba_dcfg.inbuf_onelen);
		debug_printf("p");
#endif
arec:
	{volatile int d;for(d=0;d<100;d++);}
#ifdef AREC_EN
		err = libaenc_get_frm(&aenc_frmaddr, &aenc_frmlen);
		if(err == -1){
			goto aplay;
		}
		{
			frmlen = aenc_frmlen;
#ifdef REC_TO_FILE
			err = file_write(&fp_out, aenc_frmlen, aenc_frmaddr);
#else
#ifdef AUDIO_REC_PCM
			if(recdatacnt+frmlen/2 > REC_DATA_LEN)
				break;
			memcpy(&audio_pcm[recdatacnt],(short *)aenc_frmaddr,frmlen);
			recdatacnt += frmlen/2;
#endif
#endif
			err = libaenc_update();
			arec_loop++;
			debug_printf("r");
			if(arec_loop > 4000)
			{
			    recdatacnt=0;
			//	break;
				}
		} 
#endif
	}
loop_exit:
#ifdef APLAY_EN
	libadec_stop();
    libadec_exit();
#endif
#ifdef AREC_EN

	libaenc_stop();
#ifdef REC_TO_FILE
	file_fclose(&fp_out);
	ioman_flushAll(&sh_efs.myIOman);
#else
#ifdef AUDIO_REC_PCM
	{
		int i;
		for(i=0; i<REC_DATA_LEN; i++){
			debug_printf("recpcm:%d\n", (short)(((audio_pcm[i]&0xff)<<8)|((audio_pcm[i]&0xff00)>>8)));
		}
	}
#endif
#endif
#endif
#ifndef PLAY_SINE_DATA	
	file_fclose(&fp_in);
#endif
//	goto ad_start;

//	volatile int d;for(d=0;d<100000;d++);
	goto ad_start;
	while(1);	
}
