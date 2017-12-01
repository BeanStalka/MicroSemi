#include "app_includes.h"
#include "liba_enc.h"
#include "liba_enc_internal.h"


//#define AUDIO_IN_FILE
#ifndef AUDIO_REC_PCM
//#define AUDIO_REC_PCM
#endif
//#define TEST_MFS

#ifdef TEST_MFS
#define AUDIO_IN_FILE
#define USE_MF_API 
#else

//#define USE_MF_API 

#endif

#define OUTLIST_NUM		8
//#ifdef AUDIO_REC_G711
//#define INPCM_BUFLEN	F5_INPCM_BUFLEN
//#define OUTLIST_NUM		4
//#define OUTBUF_ONELEN	F5_OUTBUF_ONELEN
//#endif

#ifdef AUDIO_REC_PCM
#define REC_DATA_LEN (1024*80)
//short audio_pcm[REC_DATA_LEN];
short *audio_pcm=(short*)0x10000000;
#endif
int recdatacnt = 0;
#ifdef AUDIO_REC_ADPCM
extern unsigned char audio_adpcm[];

#endif
#if defined(AUDIO_REC_AAC)||defined(AUDIO_REC_AF3)
unsigned char *audio_af3=(u8*)0x10000000;
#define AAC_AF3_REC_DATA_LEN (1024*100)
#endif


#ifndef LINUX_FIRMWARE
#ifdef USE_MF_API
//AREC_ALLOCATE_MF(20*1024)
AREC_ALLOCATE_MF(20*1024)
#else
AREC_ALLOCATE_STEREO(INPCM_BUFLEN, OUTLIST_NUM, OUTBUF_ONELEN)
//AREC_ALLOCATE_MONO(INPCM_BUFLEN, OUTLIST_NUM, OUTBUF_ONELEN)
#endif
#endif

#define FOR_G711
#define BIG_ENDIAN


#define QUANTIZATION 0x10 // 16bit£¬
#define BYTES_EACH_SAMPLE 0x2 // QUANTIZATION / 8, 

/*------------------------Wave File Structure ------------------------------------ */
typedef struct RIFF_CHUNK{
	char fccID[4]; // must be "RIFF"
	unsigned long dwSize; // all bytes of the wave file subtracting 8,
				// which is the size of fccID and dwSize
	char fccType[4]; // must be "WAVE"
}WAVE_HEADER;
// 12 bytes

typedef struct	FORMAT_CHUNK{
	char fccID[4]; // must be "fmt "
	unsigned long dwSize; // size of this struct, subtracting 8, which
				// is the sizeof fccID and dwSize
	unsigned short wFormatTag; // one of these: 1: linear,6: a law,7:u-law
	unsigned short wChannels; // channel number
	unsigned long dwSamplesPerSec; // sampling rate
	unsigned long dwAvgBytesPerSec; // bytes number per second
	unsigned short wBlockAlign; 	
					// NumChannels * uiBitsPerSample/8
#if 0//def FOR_G711
	unsigned long uiBitsPerSample; // quantization
#else
	unsigned short uiBitsPerSample; // quantization
#endif
}__attribute__((packed)) FORMAT;


typedef struct
{
    FORMAT wfmt;
    unsigned short nSamplesPerBlock; 
}IMAADPCMWAVEFORMAT;



// 24 bytes
// The fact chunk is required for all new WAVE formats.
// and is not required for the standard WAVE_FORMAT_PCM files

typedef struct {
	char fccID[4]; // must be "fact"
	unsigned long id; // must be 0x4
	unsigned long dwSize; //
}FACT;

// 12 bytes
typedef struct {
	char fccID[4]; // must be "data"
	unsigned long dwSize; // byte_number of PCM data in byte
}DATA;


#define SWAP4BYTES(a)  ((((a)&0xff)<<24) + (((a)&0xff00)<<8) + (((a)&0xff0000)>>8) + (((a)>>24)&0xff))
#define SWAP2BYTES(a)  (((a)<<8)+(((a)>>8)&0xff))
// 8 bytes
/*------------------------Wave File Structure ------------------------------------ */
void WriteWaveHeader(File *fpwav,int sr, short ch,short fmtag, long length)
{
	WAVE_HEADER WaveHeader;
	FORMAT WaveFMT;
	unsigned short sbSize;
	unsigned short nSamplesPerBlock; //IMA-ADPCM need,maybe other adpcm need it too.
	DATA WaveData;
	FACT WaveFact;
	int tmp4bytes;
	short tmp2bytes;
	memset(&WaveHeader, 0, sizeof(WAVE_HEADER));
	memcpy(WaveHeader.fccID, "RIFF", 4);
	memcpy(WaveHeader.fccType, "WAVE", 4);
#if 0 //def FOR_G711
	WaveHeader.dwSize = length + 0x32; // 58-8 ¸öbytes
#else
	// WaveHeader.dwSize = length + 0x24; // 
	// 44- 8 = 36¸ö
	WaveHeader.dwSize = length + 0x30; // 
#endif
	memset(&WaveFMT, 0, sizeof(FORMAT));
	memcpy(WaveFMT.fccID, "fmt ", 4);
	WaveFMT.dwSize = 0x10;
	WaveFMT.dwSamplesPerSec = sr;
//	WaveFMT.dwAvgBytesPerSec = ch * sr * BYTES_EACH_SAMPLE;
	WaveFMT.wChannels = ch;


//#ifdef FOR_G711
	WaveFMT.wBlockAlign = BYTES_EACH_SAMPLE;
	if(fmtag==0x6 || fmtag==0x7)
	{
		WaveFMT.dwAvgBytesPerSec = ch * sr * 1;
		WaveFMT.uiBitsPerSample = 8;
	}
	else if(fmtag == 0x11) //IMA-ADPCM
	{
		WaveFMT.dwAvgBytesPerSec = ch * sr * 1/2;//4bits adpcm
		WaveFMT.uiBitsPerSample = 4;
		WaveFMT.dwSize = 0x14;
		WaveFMT.wBlockAlign = 256;
		sbSize = 2; //nSamplesPerBlock
		nSamplesPerBlock = 505; //blocksize = 256, it has 505 samples.
	}
//#else	
	else
	{
		WaveFMT.dwAvgBytesPerSec = ch * sr * 2; //lpcm
		WaveFMT.uiBitsPerSample = QUANTIZATION;
	}
//#endif
	WaveFMT.wFormatTag = fmtag;
	//WaveFMT.wBlockAlign = BYTES_EACH_SAMPLE;
	memset(&WaveFact, 0, sizeof(FACT));
	memcpy(WaveFact.fccID, "fact", 4);
	WaveFact.dwSize = length;
	WaveFact.id = 0x4;
	memset(&WaveData, 0, sizeof(DATA));
	memcpy(WaveData.fccID, "data", 4);
	WaveData.dwSize = length;

#ifdef BIG_ENDIAN
	WaveHeader.dwSize = SWAP4BYTES(WaveHeader.dwSize);
	WaveFMT.dwSize = SWAP4BYTES(WaveFMT.dwSize);
	WaveFMT.dwSamplesPerSec =  SWAP4BYTES(WaveFMT.dwSamplesPerSec);
	WaveFMT.dwAvgBytesPerSec =  SWAP4BYTES(WaveFMT.dwAvgBytesPerSec);
	WaveFMT.wChannels =  SWAP2BYTES(WaveFMT.wChannels);
	WaveFMT.wFormatTag =  SWAP2BYTES(WaveFMT.wFormatTag);
	WaveFMT.wBlockAlign =  SWAP2BYTES(WaveFMT.wBlockAlign);
	WaveFMT.uiBitsPerSample =  SWAP2BYTES(WaveFMT.uiBitsPerSample);
	WaveFact.dwSize =  SWAP4BYTES(WaveFact.dwSize);
	WaveFact.id =  SWAP4BYTES(WaveFact.id);
	WaveData.dwSize =  SWAP4BYTES(WaveData.dwSize);
#endif
	file_write(fpwav, sizeof(WAVE_HEADER), (unsigned char *)&WaveHeader);
	file_write(fpwav, sizeof(FORMAT), (unsigned char *)&WaveFMT);
	if(fmtag == 0x11)
	{
#ifdef BIG_ENDIAN
		sbSize = SWAP2BYTES(sbSize);
		nSamplesPerBlock = SWAP2BYTES(nSamplesPerBlock);
#endif
		file_write(fpwav, 2, &sbSize);
		file_write(fpwav, 2, &nSamplesPerBlock);
	}
	file_write(fpwav, sizeof(FACT), (unsigned char *)&WaveFact);
	file_write(fpwav, sizeof(DATA), (unsigned char *)&WaveData);
}





extern t_signal *evt_mb;

void record_config_one(void)
{
	memset(&g_liba_ecfg, 0, sizeof(g_liba_ecfg));
#ifdef CONFIG_DCACHE
	g_liba_ecfg.dc_flush_all_proc = dc_flush_all;
	g_liba_ecfg.dc_invalidate_all_proc = dc_invalidate_all;
#endif
	g_liba_ecfg.evt_one = NULL; //evt_resp;
	g_liba_ecfg.evt_input = NULL;

#ifdef CONFIG_AUDIO_EHW_AI1
	g_liba_ecfg.evt_input = evt_mb;
#endif
#ifdef CONFIG_AUDIO_EHW_AI2
	g_liba_ecfg.evt_mb = evt_mb;
#endif
}

void record_config(void)
{
#ifndef LINUX_FIRMWARE

	memset(&g_liba_ecfg, 0, sizeof(g_liba_ecfg));
#ifdef CONFIG_DCACHE
	g_liba_ecfg.dc_flush_all_proc = dc_flush_all;
	g_liba_ecfg.dc_invalidate_all_proc = dc_invalidate_all;
#endif
#ifdef AUDIO_REC_G711
	//AREC_CONFIG_MONO(CONFIG_SAMPLE_RATE, 0, 0)
	AREC_CONFIG_MONO(22050, 0, 0)
#endif	
#ifdef AUDIO_REC_PCM
#ifdef USE_MF_API
	AREC_INIT_FMT_MONO(AUDIO_FMT_F1);
#endif
//	AREC_CONFIG_MONO(CONFIG_SAMPLE_RATE, 0, 0)
//	AREC_CONFIG_MONO(96000, 0, 0)
//	AREC_CONFIG_MONO(44100, 0, 0)
//	AREC_CONFIG_MONO(11025, 0, 0)
AREC_CONFIG_MONO(16000, 0, 0)
//	AREC_CONFIG_STEREO(CONFIG_SAMPLE_RATE, -6, 0)
    //AREC_CONFIG_EPRE_PROCESS(liba_epre_init,liba_epre_process)
#endif
#ifdef AUDIO_REC_ADPCM
	AREC_CONFIG_MONO(CONFIG_SAMPLE_RATE, 0, 0)
#endif
#ifdef AUDIO_REC_AF3
#ifdef USE_MF_API
	AREC_INIT_FMT_STEREO(AUDIO_FMT_F3);
#endif
	AREC_CONFIG_STEREO(CONFIG_SAMPLE_RATE 0, 0)
#endif
#ifdef AUDIO_REC_AAC
	AREC_CONFIG_MONO(CONFIG_SAMPLE_RATE, 0, 0)
#endif

	g_liba_ecfg.evt_one = NULL; //evt_resp;
	g_liba_ecfg.evt_input = NULL;
#ifdef CONFIG_AUDIO_EHW_AI1
	g_liba_ecfg.evt_input = evt_mb;
#endif
#ifdef CONFIG_AUDIO_EHW_AI2
	g_liba_ecfg.evt_mb = evt_mb;
#endif
//	g_liba_ecfg.arec_path = AREC_PATH2;

#endif //LINUX_FIRMWARE
	g_liba_ecfg.ai_master = 0;
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

#endif

File fp_out;
extern __com volatile s32 dbcnt;
static void record_test_one(short fmtag);
#ifdef TEST_MFS
void audio_record_test(void)
{
	int err;
	u32 frmlen;
	short *daddr, *saddr;
	u32 loop = 0, i;
	u32 frm_addr, frm_len;
        u32 outbytes = 0;
	short fmtag = 6;//6 A-LAW,7 U-LAW	
	debug_printf("audio_record_test start!\n");
	
	sd_init();

	while(1){
#if 1	
	if(file_fopen(&fp_out, &sh_efs.myFs, "epcm8000.pcm", MODE_WRITE)!= 0){
		debug_printf("open pcm out file error!\n");
	}
	debug_printf("pcm file open ok\n");
	record_config_one();
	AREC_INIT_FMT_MONO(AUDIO_FMT_F1);
	AREC_CONFIG_MONO(CONFIG_SAMPLE_RATE, 0, 0)
	record_test_one(fmtag);
#endif
#if 0
	debug_printf("start to record af3 file\n");
	if(file_fopen(&fp_out, &sh_efs.myFs, "eaf3_16k.af3", MODE_WRITE)!= 0){
		debug_printf("open af3 out file error!\n");
	}
	debug_printf("af3 file open ok\n");
	record_config_one(fmtag);
	//AREC_INIT_FMT_MONO(AUDIO_FMT_F3);
	//AREC_CONFIG_MONO(CONFIG_SAMPLE_RATE, 0, 0)
	AREC_INIT_FMT_STEREO(AUDIO_FMT_F3);
	AREC_CONFIG_STEREO(CONFIG_SAMPLE_RATE, 0, 0)
	record_test_one(fmtag);
#endif
#if 0
	debug_printf("start to record PCM file\n");
	if(file_fopen(&fp_out, &sh_efs.myFs, "epcm8000.pcm", MODE_WRITE)!= 0){
		debug_printf("open pcm out file error!\n");
	}
	debug_printf("pcm file open ok\n");
	record_config_one();
	//AREC_INIT_FMT_MONO(AUDIO_FMT_F1);
	//AREC_CONFIG_MONO(CONFIG_SAMPLE_RATE, 0, 0)
	AREC_INIT_FMT_STEREO(AUDIO_FMT_F1);
	AREC_CONFIG_STEREO(CONFIG_SAMPLE_RATE, 0, 0)
	record_test_one();
#endif	
	}
	while(1);
}
#else
void audio_record_test(void)
{
	int err;
	u32 frmlen;
	short *daddr, *saddr;
	u32 loop = 0, i;
	u32 frm_addr, frm_len;
        u32 outbytes = 0;
	short fmtag = 7;//6 A-LAW,7 U-LAW	
	debug_printf("audio_record_test start!\n");
#ifndef AUDIO_IN_FILE
#ifdef AUDIO_REC_PCM
	memset(audio_pcm, 0, REC_DATA_LEN*2);
#endif
#ifdef AUDIO_REC_ADPCM
	memset(audio_adpcm, 0, 20224);
#endif
#if defined(AUDIO_REC_AF3)||defined(AUDIO_REC_AAC)
	unsigned char *af3_ptr;
	memset(audio_af3, 0, AAC_AF3_REC_DATA_LEN);
	af3_ptr = audio_af3;
#endif

#else	//AUDIO_IN_FILE
	
	sd_init();

#ifdef AUDIO_REC_G711
	if(file_fopen(&fp_out, &sh_efs.myFs, "alaw22.wav", MODE_WRITE)!= 0)
#endif	
#ifdef AUDIO_REC_ADPCM
	fmtag = 0x11;//0x11:DVI/AMI adpcm 0x2:MS-ADPCM, not supported yet
	if(file_fopen(&fp_out, &sh_efs.myFs, "ima256.wav", MODE_WRITE)!= 0)
#endif
	
#ifdef AUDIO_REC_PCM
	if(file_fopen(&fp_out, &sh_efs.myFs, "r44k.pcm", MODE_WRITE)!= 0)
#endif
#ifdef AUDIO_REC_AF3
	if(file_fopen(&fp_out, &sh_efs.myFs, "eaf3-16k.af3", MODE_WRITE)!= 0)
#endif
#ifdef AUDIO_REC_AAC
	if(file_fopen(&fp_out, &sh_efs.myFs,  "eaac.aac", MODE_WRITE) != 0)
#endif
	{
		debug_printf("open out file error!\n");
	}
	debug_printf("file open ok\n");
#endif


#if defined(AUDIO_REC_G711) || defined(AUDIO_REC_ADPCM)
	WriteWaveHeader(&fp_out,g_liba_ecfg.asample_rate, g_liba_ecfg.achannels,fmtag, 0);
#endif
	
	record_config();

    	record_test_one(fmtag);
	while(1);
}
#endif

static void record_test_one(short fmtag)
{
	int err;
	u32 frmlen;
	short *daddr, *saddr;
	u32 loop = 0, i;
	u32 frm_addr, frm_len;
    u32 outbytes = 0;
	recdatacnt = 0;

#if 0
	libgpio_config(21,0);
	libgpio_write(21, 1);


	//reset AIC
	libgpio_config(8,0);
	libgpio_write(8, 0);
	{volatile int d;for(d=0;d<10000;d++);}
	libgpio_write(8, 1);
#endif

	// init audio
	libaenc_init();
	debug_printf("audio init ok!\n");

	//start audio
	libaenc_start();

	debug_printf("start write data loop!\n");
	while(1){
#ifdef CONFIG_AUDIO_EHW_AI1
		newos_wait(g_liba_ecfg.evt_input, 0, &err);
		err = libaenc_update_input();
		while(1){
#endif
		err = libaenc_get_frm(&frm_addr, &frm_len);
		if(err == -1)
		{
#ifdef CONFIG_AUDIO_EHW_AI1
			break;
#else
//			debug_printf("e");
			continue;
#endif
		}
		//debug_printf("dbcnt = 0x%x\n",dbcnt);
		//debug_printf("w%d",loop);
#ifndef AUDIO_IN_FILE
#ifdef AUDIO_REC_PCM
			if(recdatacnt+frm_len/2 > REC_DATA_LEN)
				break;
			memcpy(&audio_pcm[recdatacnt],(short *)frm_addr,frm_len);
			recdatacnt += frm_len/2;
#endif
#if defined(AUDIO_REC_AAC)||defined(AUDIO_REC_AF3)
			if(recdatacnt+frm_len > AAC_AF3_REC_DATA_LEN)
				break;
			memcpy(&audio_af3[recdatacnt],(u8 *)frm_addr,frm_len);
			recdatacnt += frm_len;
#endif

#else	//AUDIO_IN_FILE
#ifdef AUDIO_REC_PCM
			if(recdatacnt+frm_len/2 > REC_DATA_LEN){
				debug_printf("buffer full\n");
				goto done;
			}
			memcpy(&audio_pcm[recdatacnt],(short *)frm_addr,frm_len);
			recdatacnt += frm_len/2;
#else
			err = file_write(&fp_out, frm_len, frm_addr);
			outbytes += frm_len;
			recdatacnt += frm_len;
#endif

#endif
			err = libaenc_update();
			loop++;

#ifdef AUDIO_IN_FILE
		if(loop == 300){
				debug_printf("no more space to store audio!\n");
				libaenc_stop();
				debug_printf("stop encoder!\n");
				goto done;
			}
#endif
#ifdef CONFIG_AUDIO_EHW_AI1
		}
#endif
	}
done:
	libaenc_stop();
	//debug_printf("done!!!\n");
#if defined(AUDIO_REC_G711)||defined(AUDIO_REC_ADPCM)
	file_setpos(&fp_out,0);
	WriteWaveHeader(&fp_out,g_liba_ecfg.asample_rate, g_liba_ecfg.achannels,fmtag, outbytes);
#endif
	

#ifndef AUDIO_IN_FILE
#ifdef AUDIO_REC_PCM
	for(i=0; i<REC_DATA_LEN; i++){
		debug_printf("%d,", (short)(((audio_pcm[i]&0xff)<<8)|((audio_pcm[i]&0xff00)>>8)));
	}
#endif
#ifdef AUDIO_REC_ADPCM
	for(i=0; i<(loop*frmlen); i++){
		debug_printf("0x%x\n", (audio_adpcm[i]&0xff));
	}
#endif
#if defined(AUDIO_REC_AF3)||defined(AUDIO_REC_AAC)
	for(i=0; i<AAC_AF3_REC_DATA_LEN; i++){
		debug_printf("0x%x\n", (audio_af3[i]&0xff));
	}
#endif
#else	//AUDIO_IN_FILE
#ifdef AUDIO_REC_PCM
	debug_printf("write to SD card %d bytes\n",recdatacnt*2);
	file_write(&fp_out, recdatacnt*2, audio_pcm);
#endif
	file_fclose(&fp_out);
	ioman_flushAll(&sh_efs.myIOman);
#endif

	debug_printf("complete!\n");
//	while(1);
}
