#include "includes.h"
#include "liba_dec.h"
#include "typedefs.h"
#include "chip.h"
#include "hbi.h"

#ifdef __KERNEL__
/* TODO: this mode is not tested for zl380xx driver */
#include "liba_enc.h"
#include <linux/delay.h>
#include "libacodec.h"
#include "codec_setting_start.h"
//typedef s32 int;
//typedef u8 unsigned char;
#define I2C_DEV_ID 0x1a
#define CONFIG_AUDIO_ENC_EN
#define CONFIG_AUDIO_DEC_EN
#endif



/* List of configuration option for ZL380xx device ----------*/

#if 0
/* Needed if we are developing on a system where i2s connections are available 
without SPI/I2C. (Ideally to be removed in final version) */
#define CONFIG_ZL380XX_DISABLE_HBI      
#endif
/* Macro to select between available port A or B */

#ifdef CONFIG_ZL380XX_PORT_I2SB     
#define ZL380XX_CLK_CFG_REG  ZL380xx_TDMB_CLK_CFG_REG
#else
#define ZL380XX_CLK_CFG_REG  ZL380xx_TDMA_CLK_CFG_REG
#endif

/* list of attributes supported by ZL380xx audio driver */
typedef enum 
{
    ZL380XX_AUD_ATTRIB_SR,
    ZL380XX_AUD_ATTRIB_DAC_VOL, 
    ZL380XX_AUD_ATTRIB_ADC_VOL,
    ZL380XX_AUD_ATTRIB_PWRDWN,
    ZL380XX_AUD_ATTRIB_MUTE,
}ZL380XX_AUD_ATTRIB;

static int host_ai_master = 0;
hbi_handle_t h_zl380xx;

#ifdef CONFIG_ZL380XX_HBI_I2C/* i2c */
static int bus_num=0;
static int dev_id = 0x45;
#else 
static int bus_num = 0;
static int dev_id = 0;
#endif

#ifdef CONFIG_ZL380XX_DISABLE_HBI
#define internal_zl380xx_aud_read
#define internal_zl380xx_aud_wr   
#define internal_zl380xx_aud_reset 
#else
#define internal_zl380xx_aud_read  HBI_read
#define internal_zl380xx_aud_wr    HBI_write
#define internal_zl380xx_aud_reset HBI_reset
#endif

static s32 internal_zl380xx_aud_set_attrib(ZL380XX_AUD_ATTRIB , void *);
static s32 internal_zl380xx_aud_init(void);

static s32 internal_zl380xx_aud_init(void)
{
#ifdef CONFIG_ZL380XX_DISBLE_HBI  
    return 0;
#endif
    hbi_status_t  status=0;
    hbi_dev_cfg_t cfg;

    status = HBI_init(NULL);
    if(status != HBI_STATUS_SUCCESS)
    {
        debug_printf("HBI Init failed\n");
        return -1;
    }
    cfg.bus_num = bus_num;
    cfg.dev_addr = dev_id;
    cfg.pDevName = NULL;
    
    status = HBI_open(&h_zl380xx,&cfg);
    if(status != HBI_STATUS_SUCCESS)
    {
        debug_printf("HBI open failed.Terminate driver\n");
        status = HBI_term();
        h_zl380xx = (hbi_handle_t) NULL;
        return -1;
    }
    return 0;
}


/*
 * set sample rate and format
 */
static s32 internal_zl380xx_aud_set_attrib(ZL380XX_AUD_ATTRIB attrib, void *arg)
{
    u16  val;
    reg_addr_t reg;
    hbi_status_t ret=0;

    switch(attrib)
    {
        case ZL380XX_AUD_ATTRIB_SR:
        {
            u16 sr;
            s32 rate = *((s32 *)arg);
            debug_printf("set ZL380xx samplerate = %d\n",rate);

            switch(rate){
                case 8000:
                    sr = ZL380xx_FR_8KHZ;
                break;
                case 16000:
                    sr = ZL380xx_FR_16KHZ;
                break;
                case 44100:
                    sr = ZL380xx_FR_441KHZ;
                break;
                case 48000:
                    sr = ZL380xx_FR_48KHZ;
                break;
                default:
                    debug_printf("FR %d not supported\n",rate);
                    return -1;
            }

            reg = ZL380XX_CLK_CFG_REG; 

            ret = internal_zl380xx_aud_read(h_zl380xx,
                                             reg,
                                             (user_buffer_t *)&val,
                                             2);
            if(ret != HBI_STATUS_SUCCESS)
            {
                debug_printf("[%s:%d] failed\n",__FUNCTION__,__LINE__);
                return -1;
            }
            val &= ~((1<<ZL380xx_TDM_CLK_MASTER_SHIFT)|
                     (ZL380xx_TDM_CLK_FSRATE_MASK));

            val |= (sr << ZL380xx_TDM_CLK_FSRATE_SHIFT);

            if(!host_ai_master)
                val |= (ZL380xx_TDM_CLK_MASTER);

            debug_printf("Setting 0x%x to TDM CLK CFG REG 0x%x\n",val,reg);

            ret = internal_zl380xx_aud_wr(h_zl380xx,reg,(user_buffer_t *)&val,2);
            if(ret != HBI_STATUS_SUCCESS)
            {
                debug_printf("[%s:%d] failed\n",__FUNCTION__,__LINE__);
                return -1;
            }

            if(!host_ai_master)
            {
                /* You will need to issue this after sample rate change */
                debug_printf("Issuing Soft Reset\n");
                val = ZL380xx_HOST_SW_FLAGS_APP_REBOOT;
                ret = internal_zl380xx_aud_wr(h_zl380xx,
                                             ZL380xx_HOST_SW_FLAGS_REG,
                                             (user_buffer_t *)&val,
                                             2);
                if(ret == HBI_STATUS_SUCCESS)
                {
                    val=0;
                    ret = internal_zl380xx_aud_read(h_zl380xx,
                                                   ZL380xx_SYSSTAT_REG,
                                                   (user_buffer_t *)&val,
                                                   2);

                    if(ret != HBI_STATUS_SUCCESS)
                        debug_printf("[%s:%d] failed\n",__FUNCTION__,__LINE__);
                    else if(val)
                        debug_printf("Warning: Cross Point cfg status 0x%x\n ",
                                     val);
                }
                else
                {
                    debug_printf("[%s:%d] failed\n",__FUNCTION__,__LINE__);
                }
           }
        }
        break;

        case ZL380XX_AUD_ATTRIB_DAC_VOL:
        {
            /*  
            Points to be noted:
            
            1. Volume value here is expected to be converted by user as per 
            ROUT Gain register description and then pass here.

            For ex. for setting 0DB , user should send value 0x40.
            See Device FW manual for reference.
            
            2. This implementation would work if user has DAC source set to ROUT 
            for any other routing (Ex. By pass i2s_x -> DAC_x) this 
            implementation won't help much.
            for such cases, it is advisable for user to use register 
            read/write ioctls
            */
            s32 vol = *(s32*)arg;
            debug_printf("set dacvol = 0x%x\n",vol);
            
            ret = internal_zl380xx_aud_read(h_zl380xx,
                                             ZL380xx_ROUT_GAIN_CTRL_REG,
                                             (user_buffer_t *)&val,
                                             2);
            if(ret!=HBI_STATUS_SUCCESS)
            {
                debug_printf("[%s:%d] failed\n",__FUNCTION__,__LINE__);
                return ret;
            }
            

            ret = internal_zl380xx_aud_wr(h_zl380xx,
                                          ZL380xx_ROUT_GAIN_CTRL_REG,
                                          (user_buffer_t *)&val,
                                          2);
            if(ret!=HBI_STATUS_SUCCESS)
            {
                debug_printf("[%s:%d] failed\n",__FUNCTION__,__LINE__);
                return ret;
            }
        }
        break;
        case ZL380XX_AUD_ATTRIB_ADC_VOL:
        {
            /*  
            Points to be noted:
            
            1. Volume value here is expected to be converted by user as per 
            SOUT Gain register description and then pass here.

            For ex. for setting 0DB , user should send value 0x10.
            See Device FW manual for reference.
            
            2. This implementation would work if user has i2s_x source set to 
            SOUT for any other routing (Ex. By pass mic_x-> i2s_x ) this 
            implementation won't help much. for such cases, it is advisable for 
            user to use register read/write ioctls
            */


            s32 vol = *(s32*)arg;
            debug_printf("set adcvol = 0x%x\n",vol);

            ret = internal_zl380xx_aud_read(h_zl380xx,
                                             ZL380xx_SOUT_GAIN_CTRL_REG,
                                            (user_buffer_t *) &val,
                                             2);
            if(ret!=HBI_STATUS_SUCCESS)
            {
                debug_printf("[%s:%d] failed\n",__FUNCTION__,__LINE__);
                return ret;
            }
            debug_printf("Setting vol 0x%x @ sout\n",val);
            ret = internal_zl380xx_aud_wr(h_zl380xx,
                                          ZL380xx_SOUT_GAIN_CTRL_REG,
                                          (user_buffer_t *)&val,2);
            if(ret!=HBI_STATUS_SUCCESS)
            {
                debug_printf("[%s:%d] failed\n",__FUNCTION__,__LINE__);
                return ret;
            }
        }
        break;
        default:
            debug_printf("Not supported\n");
            return -1;
    }

    return ret;
}



int codec_zl380xx_init(E_AI_DIR dir)
{

    if(h_zl380xx == (hbi_handle_t) NULL)
    {
        if(internal_zl380xx_aud_init() < 0)
        {
            debug_printf("Audio init failed\n");
            return -1;
        }
    }

/*
    ZL380xx devices support one-time configuration record loading
    which configures driver for playback/record/duplex op.
    We presume, user will enable config record loading via new ioctl or may use
    reg rd/wr ioctl.
    if needed, will be added based on customer requirement.
    User is expected to call this atleast once, if system doesn't have flash 
    with relevant configuration record saved.
*/


#ifdef CONFIG_AUDIO_DEC_EN
    if(g_liba_dcfg.ai_master)
        host_ai_master = 1;
#endif

#ifdef CONFIG_AUDIO_ENC_EN
    if(g_liba_ecfg.ai_master)
        host_ai_master=1;
#endif

#if defined(CONFIG_AUDIO_DEC_EN) && defined(CONFIG_AUDIO_ENC_EN)
    if(g_liba_ecfg.ai_master || g_liba_dcfg.ai_master)
        host_ai_master = 1;
#endif

    return 0;
}

int codec_zl380xx_ioctl(E_AI_IOCTL ioctl, void *arg)
{
    int ret=0;
    switch(ioctl)
    {
        case AI_CTL_CHN:
        break;
        case AI_CTL_SR:
            if(!host_ai_master)
            {   
                    /* TODO: does in slave mode TW autodetect sample rate ?*/
                    ret = internal_zl380xx_aud_set_attrib(ZL380XX_AUD_ATTRIB_SR,
                                                          arg );
                }else{
                debug_printf("Host AI is master,don't need set samplerate in zl380xx\n");
            }
        break;

        case AI_CTL_DAC_DVOL:
        {
            ret = internal_zl380xx_aud_set_attrib(ZL380XX_AUD_ATTRIB_DAC_VOL,arg);
            return ret; 
        }

        case AI_CTL_ADC_DVOL:
        {
            ret = internal_zl380xx_aud_set_attrib(ZL380XX_AUD_ATTRIB_ADC_VOL,arg);
            return ret; 
        }

       case AI_CTL_REG_WRITE:
       {
           reg_addr_t reg = ((*((u32 *)arg)) >> 16) & 0xFFFF;
           u16 val = (*((u32 *)arg)) & 0xFFFF;
           ret = internal_zl380xx_aud_wr(h_zl380xx,reg,(user_buffer_t *)&val,2);
           if(ret != HBI_STATUS_SUCCESS)
                ret = -1;
       }
       break;

        case AI_CTL_MUTE:
        break;

        case AI_CTL_PWRDWN:
        /* HBI driver doesn't support this at this point*/
        break;

        case AI_CTL_EQ:
            debug_printf("eq not supported in zl380xx\n");
        break;

        case AI_CTL_TRIS_ADCDAT:
        break;

        default:
            debug_printf("unsupported ioctl cmd %d\n",ioctl);
        break;
    }
    return 0;
}

t_libacodec_setting g_libacodec_setting = {
//	.ai_mode = AI_MODE,
    .init = codec_zl380xx_init,
    .ioctl = codec_zl380xx_ioctl
};

#ifdef __KERNEL__
#include "codec_setting_end.h"
#endif
