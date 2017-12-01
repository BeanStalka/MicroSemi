#include "includes.h"
#include "app_includes.h"
#include "liba_dec.h"



#define AUDIO_PLAY_DEBUG

//#define AUDIO_IN_FILE

//#define TEST_MFS

#ifdef TEST_MFS
#define USE_MF_API
#else
//#define USE_MF_API
#endif

#ifdef AUDIO_PLAY_PCM
extern short audio_pcm[];
#endif
#ifdef AUDIO_PLAY_ADPCM
extern unsigned char audio_adpcm[];
#endif
#ifdef AUDIO_PLAY_AF3
extern unsigned char audio_af3[];
#endif
#ifdef AUDIO_PLAY_AAC
extern unsigned char audio_aac[];
#endif
extern t_signal *evt_mb;
extern t_signal *evt_int;

#define INLIST_NUM		4



#ifdef USE_MF_API
ADEC_ALLOCATE_MF(20*1024)
#else
ADEC_ALLOCATE(INBUF_ONELEN, INLIST_NUM, OUTPCM_BUFLEN)
#endif

void audio_config(int sr, E_ACHANNEL ch)
{
	memset(&g_liba_dcfg, 0, sizeof(g_liba_dcfg));
	ADEC_CONFIG(sr,  ch, 0);

#ifdef CONFIG_DHW_AI1
	g_liba_dcfg.evt_int = evt_mb;
#endif
#ifdef CONFIG_DHW_AI2	
	g_liba_dcfg.evt_mb = evt_mb;
#endif
}

void audio_play_config(void)
{
	//	memset(&g_liba_dcfg, 0, sizeof(g_liba_dcfg));

#ifdef AUDIO_PLAY_G711
	ADEC_CONFIG(11025,  MONO, 0);
#endif

#ifdef AUDIO_PLAY_PCM
#ifdef USE_MF_API
	 ADEC_INIT_FMT(AUDIO_FMT_F1)
#endif
//	ADEC_CONFIG(11025,  STEREO, 0);
//	ADEC_CONFIG(11025, MONO, 0);
//    debug_printf("CONFIGURING FOR 16k stereo\n");
//    ADEC_CONFIG(16000, STEREO, 0);
    debug_printf("CONFIGURING FOR 16k mono\n");
    ADEC_CONFIG(16000, MONO, 0);

//    debug_printf("CONFIGURING FOR 8k mono\n");

//ADEC_CONFIG(8000, MONO, 0);
//  debug_printf("CONFIGURING FOR 8k stereo\n");

//  ADEC_CONFIG(8000, STEREO, 0);
//    debug_printf("CONFIGURING FOR 48k stereo \n");

//ADEC_CONFIG(48000, STEREO, 0);
    debug_printf("CONFIGURING FOR 44k stereo \n");

ADEC_CONFIG(44100, STEREO, 0);

#endif
#ifdef AUDIO_PLAY_ADPCM
	ADEC_CONFIG(11025, MONO, 0);
#ifdef AUDIO_PLAY_DEBUG
	debug_printf("adpcm decoder!\n");
#endif
#endif
#ifdef AUDIO_PLAY_ADPCM_IMA
	ADEC_CONFIG(11025, MONO, 0);
#ifdef AUDIO_PLAY_DEBUG
	debug_printf("adpcm decoder!\n");
#endif
#endif
#ifdef AUDIO_PLAY_AF3

#ifdef USE_MF_API
	 ADEC_INIT_FMT(AUDIO_FMT_F3)
#endif
	 ADEC_CONFIG(44100,  MONO, 0);
#ifdef AUDIO_PLAY_DEBUG
	debug_printf("af3 decoder!\n");
#endif
#endif
#ifdef AUDIO_PLAY_AAC
	ADEC_CONFIG(44100,  MONO, 0);
#ifdef AUDIO_PLAY_DEBUG
	debug_printf("aac decoder!\n");
#endif
#endif
	
#ifdef CONFIG_DHW_AI1
	g_liba_dcfg.evt_int = evt_mb;
#endif
#ifdef CONFIG_DHW_AI2	
	g_liba_dcfg.evt_mb = evt_mb;
#endif
	g_liba_dcfg.ai_master=0;
}

#ifdef AUDIO_IN_FILE
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
#else
static char bitstream[]=
{
#include "ulaw16.txt" //g711 ulaw 16K mono
};

//play a speech (8k mono)
static short sinwave[] = 
{
#if 1
//#include "test_8ks.txt"//8k stereo
//#include "test_8km.txt"//8k mono
//#include "8km.txt"
//#include "test_48ks.txt"//48k stereo
//#include "d11k.txt"
#include "DTM16k_1.txt"//16k mono
//#include "DTS16k_1.txt"
//#include "test_44ks.txt"//44k stereo
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
void fill_sinedata(short *pcmsrc, int pcmsrclen,short *pcm, int len)
{
	   int i;
	   for(i=0;i<len;i++)
	   {
           if(sptr<10)
           debug_printf("%d:0x%x ",sptr,pcmsrc[sptr]);

	   	    pcm[i] =SWAP2BYTES(pcmsrc[sptr++]);

	   	    if(sptr >= pcmsrclen/2)
	   	    	 sptr = 0;	   	    
	   }
	   
       debug_printf("\n ");
}

void fill_bitstream(char *src, int srclen,char *dst, int len)
{
	   int i;
	   for(i=0;i<len;i++)
	   {
	   	    dst[i] =src[sptr++];
	   	    debug_printf("%d:0x%x ",i,dst[i]);
	   	    if(sptr >= srclen)
	   	    	 sptr = 0;	   	    
	   }
}



#endif

static void audio_play_one_from_file(void);
static void audio_play_one_from_buffer(char *buf,int buflen,int compressed);
File fp_in;

#ifdef TEST_MFS
void audio_play_test(void)
{
//	File fp_in;
#ifdef AUDIO_IN_FILE
	sd_init();
#endif
	while(1){
#ifdef AUDIO_IN_FILE
	//G711
	if(file_fopen(&fp_in, &sh_efs.myFs, "ulaw11.wav", MODE_READ)!= 0)
	{
		debug_printf("ualw11.wav file open error\n");
		while(1);
	}
//	memset(g_adec_buf,0,20*1024);
	{
		int i;
		char tmp[58];
		 file_read(&fp_in, 58, tmp);
	}	

	debug_printf("g711 file open ok\n");
	 ADEC_INIT_FMT(AUDIO_FMT_F5)
	audio_config(11025,MONO);
	audio_play_one_from_file();
#else
	debug_printf("play G711 buffer\n");
	 ADEC_INIT_FMT(AUDIO_FMT_F5)
	audio_config(16000,MONO);
	audio_play_one_from_buffer(bitstream,sizeof(bitstream),1);
#endif

#ifdef AUDIO_IN_FILE
	if(file_fopen(&fp_in, &sh_efs.myFs, "r11k.pcm", MODE_READ)!= 0)
	{
		debug_printf("pcm file open error\n");
		while(1);
	}
	debug_printf("pcm file open ok\n");
	 ADEC_INIT_FMT(AUDIO_FMT_F1)
	audio_config(11025,MONO);
	audio_play_one_from_file();
#else
	debug_printf("play PCM buffer\n");
	ADEC_INIT_FMT(AUDIO_FMT_F1)
	audio_config(11025,MONO);
	audio_play_one_from_buffer(sinwave,sizeof(sinwave),0);
#endif

#if 0
	if(file_fopen(&fp_in, &sh_efs.myFs, "44km.af3", MODE_READ)!= 0)
	{
		debug_printf("af3 file open error\n");
	}
	memset(g_adec_buf,0,20*1024);
	debug_printf("af3 file open ok\n");
	 ADEC_INIT_FMT(AUDIO_FMT_F3)
	audio_config(44100,MONO);
	audio_play_one_from_file();
#endif
	}
}
#else
extern void set_audio_clock(void);
void audio_play_test(void)
{
	//int err;
	//u32 frm_addr, frm_len;
	//u32 loop = 0;

//	debug_ba2_internal_on();
	
#ifdef AUDIO_IN_FILE
//	File fp_in;

	sd_init();

#ifdef AUDIO_PLAY_G711
	if(file_fopen(&fp_in, &sh_efs.myFs, "ulaw11.wav", MODE_READ)!= 0){
		debug_printf("g711 file open error\n");
		while(1);
	}
	debug_printf("g711 file open ok\n");

        //to skip 58 bytes which is the size of WAVE header in A/U-Law Wave files
	{
		int i;
		char tmp[58];
		 file_read(&fp_in, 58, tmp);
	}	

#endif


#ifdef AUDIO_PLAY_PCM
	{
	char filename[20]="cq32km.pcm";
	if(file_fopen(&fp_in, &sh_efs.myFs, filename, MODE_READ)!= 0)
	{
		debug_printf("pcm file %s open error\n",filename);
		return;
	}
		debug_printf("pcm file %s open ok\n",filename);
	}
//	if(file_fopen(&fp_in, &sh_efs.myFs, "8ks.pcm", MODE_READ)!= 0)
#endif
#ifdef AUDIO_PLAY_ADPCM
	if(file_fopen(&fp_in, &sh_efs.myFs, "audio.swf", MODE_READ)!= 0)
	{
		debug_printf("adpcm swf file open error\n");
	}
	debug_printf("adpcm swf file open ok\n");	
#endif
#ifdef AUDIO_PLAY_ADPCM_IMA
	if(file_fopen(&fp_in, &sh_efs.myFs, "1.wav", MODE_READ)!= 0)
	{
		debug_printf("ima adpcm file open error\n");
	}
	        //to skip 60 bytes which is the size of WAVE header in A/U-Law Wave files
	{
		int i;
		char tmp[60];
		 file_read(&fp_in, 60, tmp);
	}	
	debug_printf("ima adpcm file open ok\n");	
#endif
#ifdef AUDIO_PLAY_AF3
	if(file_fopen(&fp_in, &sh_efs.myFs, "44km.af3", MODE_READ)!= 0)
	{
		debug_printf("af3 file open error\n");
	}
	debug_printf("mpe file open ok\n");
#endif
#ifdef AUDIO_PLAY_AAC
	if(file_fopen(&fp_in, &sh_efs.myFs, "44km.aac", MODE_READ)!= 0)
	{
		debug_printf("aac file open error\n");
	}
	debug_printf("aac file open ok\n");	
#endif

	audio_play_config();
	audio_play_one_from_file();
#else
	audio_play_config();

#ifdef AUDIO_PLAY_PCM
    debug_printf("PLAY FROM SINWAVE BUFFER\n");
	audio_play_one_from_buffer(sinwave,sizeof(sinwave),0);
//	audio_play_one_from_buffer(audio_pcm,3072,0);
#else
	audio_play_one_from_buffer(bitstream,sizeof(bitstream),1);
#endif
#endif

//	set_audio_clock();
//	while(1);
	while(1);
}
#endif


extern __com u32 test_ba2;
extern __com u32 ai_inited;
extern __com u32 ai_ctl;
int first_frame = 1;

#ifdef AUDIO_IN_FILE
static void audio_play_one_from_file(void)
{
	int err;
	u32 frm_addr, frm_len;
	//u32 loop = 0;
	libadec_init();
#if 1
	debug_printf("0000xc0001048 = 0x%x\n",ReadReg32(0xc0001048));
	debug_printf("0xc0001008 = 0x%x\n",ReadReg32(0xc0001008));
	debug_printf("0xc0001008 = 0x%x\n",ReadReg32(0xc0001008));
	debug_printf("REG_SC_DIV3 = 0x%x\n",ReadReg32(REG_SC_DIV3));
	debug_printf("REG_SC_DIV2 = 0x%x\n",ReadReg32(REG_SC_DIV2));
	debug_printf("REG_SC_DIV1 = 0x%x\n",ReadReg32(REG_SC_DIV1));
#endif

#ifdef AUDIO_PLAY_DEBUG
	debug_printf("audio init ok!\n");
#endif
	
	libadec_start();
	debug_printf("111xc0001048 = 0x%x\n",ReadReg32(0xc0001048));
#ifdef AUDIO_PLAY_DEBUG
	debug_printf("audio play start!");
	debug_printf("start add data loop!\n");
#endif

//    WriteReg32(0xc0001010, ReadReg32(0xc0001010) | BIT9);//ba2 2X
//   WriteReg32(0xc0001010, ReadReg32(0xc0001010) | BIT8);//ba1 2X
//	WriteReg32(REG_SC_ADDR+0x404,0x9);//R2 slave, codec master
	do{
		//fill data
		while(1){
		//	debug_printf("ai_inited = 0x%x\n",ai_inited);
			frm_addr = libadec_inbuf_get();			
			if(frm_addr == 0){
//				debug_printf("F\n");
				break;
			}

			debug_printf("1");
			unsigned int read_bytes = file_read(&fp_in, g_liba_dcfg.inbuf_onelen, (u8 *)frm_addr);
			if(read_bytes < g_liba_dcfg.inbuf_onelen){	
				debug_printf("no more data!%d,%d\n", read_bytes, g_liba_dcfg.inbuf_onelen);
				goto audio_play_stop;
			}
			frm_len = g_liba_dcfg.inbuf_onelen;	//byte number
	add_frame:
			err = libadec_frm_add(frm_addr, frm_len);
			if(err < 0){
				goto add_frame;
			}
		}

#ifdef CONFIG_DHW_AI1
		debug_printf("D\n");
//		debug_printf("0xc0001048 = 0x%x\n",ReadReg32(0xc0001048));
		err = libadec_update_output();
		if(first_frame){
			err = libadec_update_output();
			first_frame = 0;
		}
		newos_wait(g_liba_dcfg.evt_int, 0, &err);
#endif
	}while(1);
audio_play_stop:
	file_fclose(&fp_in);
	ioman_flushAll(&sh_efs.myIOman);
	libadec_stop();
	libadec_exit();
#ifdef AUDIO_PLAY_DEBUG
	debug_printf("audio play exit!\n");
#endif
//	while(1);
}
#else
static void audio_play_one_from_buffer(char *buf,int buflen,int compressed)
{
	int err;
	u32 frm_addr, frm_len;
	u32 loop = 0;

	sptr = 0;
	libadec_init();
#if 1
	debug_printf("0000xc0001048 = 0x%x\n",ReadReg32(0xc0001048));
	debug_printf("0xc0001008 = 0x%x\n",ReadReg32(0xc0001008));
	debug_printf("0xc0001008 = 0x%x\n",ReadReg32(0xc0001008));
	debug_printf("REG_SC_DIV3 = 0x%x\n",ReadReg32(REG_SC_DIV3));
	debug_printf("REG_SC_DIV2 = 0x%x\n",ReadReg32(REG_SC_DIV2));
	debug_printf("REG_SC_DIV1 = 0x%x\n",ReadReg32(REG_SC_DIV1));
#endif

#ifdef AUDIO_PLAY_DEBUG
	debug_printf("audio init ok!\n");
#endif
	
	libadec_start();
	debug_printf("111xc0001048 = 0x%x\n",ReadReg32(0xc0001048));
#ifdef AUDIO_PLAY_DEBUG
	debug_printf("audio play start!");
	debug_printf("start add data loop!\n");
#endif

//   WriteReg32(0xc0001010, ReadReg32(0xc0001010) | BIT9);//ba2 2X
//   WriteReg32(0xc0001010, ReadReg32(0xc0001010) | BIT8);//ba1 2X
//   WriteReg32(REG_SC_ADDR+0x404,0x9);//R2 slave, codec master
	do{
		//fill data
		while(1){
		//	debug_printf("ai_inited = 0x%x\n",ai_inited);
			frm_addr = libadec_inbuf_get();			
			if(frm_addr == 0){
//				debug_printf("F\n");
				break;
			}

			//debug_printf("1");
			if(compressed){
							fill_bitstream(buf,buflen,(char *)frm_addr,  g_liba_dcfg.inbuf_onelen);
			}else{
				fill_sinedata((short*)buf, buflen,(short *)frm_addr,  g_liba_dcfg.inbuf_onelen/2);
			}

			loop++;	
			frm_len = g_liba_dcfg.inbuf_onelen;	//byte number
	add_frame:
			err = libadec_frm_add(frm_addr, frm_len);
			if(err < 0){
				goto add_frame;
			}
		}
		if(loop > 20000){
		//	break;
		}


#ifdef CONFIG_DHW_AI1
		debug_printf("D\n");
//		debug_printf("0xc0001048 = 0x%x\n",ReadReg32(0xc0001048));
		err = libadec_update_output();
		if(first_frame){
			err = libadec_update_output();
			first_frame = 0;
		}
		newos_wait(g_liba_dcfg.evt_int, 0, &err);
#endif
	}while(1);
	
audio_play_stop:
	libadec_stop();
	libadec_exit();
#ifdef AUDIO_PLAY_DEBUG
	debug_printf("audio play exit!\n");
#endif
//	while(1);
}
#endif
