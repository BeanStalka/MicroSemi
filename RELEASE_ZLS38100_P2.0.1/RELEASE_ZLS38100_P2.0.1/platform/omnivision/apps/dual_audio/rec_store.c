#include "app_includes.h"

#define WC_BUFSIZE 2048
static struct s_writecache mywc;
static u8 wc_buf[WC_BUFSIZE];


int rec_file_param_init(void)
{
    memset(&mywc, 0, sizeof(mywc));
	mywc.write = wc_write;
	mywc.flush = wc_flush;
	mywc.size = WC_BUFSIZE/512;
	mywc.buf = wc_buf;
	mywc.fastwrite_size = 16*512;
	return 0;
}

#define REC_PARAM	void
s32 rec_file_open(File *pfile, u8 *file_name, REC_PARAM *p_rec_param)
{
    rec_file_param_init();
    pfile->wc = &mywc;
	if( file_fopen(pfile, &sh_efs.myFs, file_name, 'r') < 0){
		debug_printf("cannot open file:%s\n", file_name);
		return -1;
	}else{
		debug_printf("file opened ok: %s\n", file_name);
    }

//    g_recfile_hdl.wr_hdr(pfile,  p_rec_param);

    return 0;
}
s32 rec_file_open_wr(File *pfile, u8 *file_name, REC_PARAM *p_rec_param)
{
    rec_file_param_init();
    pfile->wc = &mywc;
	if( file_fopen(pfile, &sh_efs.myFs, file_name, 'w') < 0){
		debug_printf("cannot open file:%s\n", file_name);
		return -1;
	}else{
		debug_printf("file opened ok: %s\n", file_name);
    }

//    g_recfile_hdl.wr_hdr(pfile,  p_rec_param);

    return 0;
}


void rec_file_close(File *pfile, u8 *file_name)
{
    debug_printf("ready to file close\n");
//	euint32 fileptr;
	//u32 last_tag_size;

	// Flush data to SD Card, and disable Write Cache
    if(pfile->wc)
	    WC_FLUSH(pfile);
	ioman_flushAll(&sh_efs.myIOman);
	pfile->wc = NULL;

    file_fclose(pfile);
	ioman_flushAll(&sh_efs.myIOman);
    
    debug_printf("file close\n");

}
#if 0
static u32 time_get(void)
{
    return start_time + ticks/TICKS_PER_SEC ;
}
#endif

