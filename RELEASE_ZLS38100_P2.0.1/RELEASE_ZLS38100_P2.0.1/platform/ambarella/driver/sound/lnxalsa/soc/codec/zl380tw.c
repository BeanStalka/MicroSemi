/*
 * zl380tw.c  --  zl380tw ALSA Soc Audio driver
 *
 * Copyright 2014 Microsemi Inc.
 *
 * This program is free software you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>



/* driver private data */
    struct zl380tw {
    int sysclk;
    struct platform_device *dev;
} *zl380tw_priv;

/*--------------------------------------------------------------------
 *    ALSA  SOC CODEC driver
 *--------------------------------------------------------------------*/


/*Formatting for the Audio*/
#define zl380tw_DAI_RATES            (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_48000)
#define zl380tw_DAI_FORMATS          (SNDRV_PCM_FMTBIT_S16_LE)
#define zl380tw_DAI_CHANNEL_MIN      1
#define zl380tw_DAI_CHANNEL_MAX      2

static int zl380tw_hw_params(struct snd_pcm_substream *substream,
                            struct snd_pcm_hw_params *params,
                            struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_codec *codec = rtd->codec;
    struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);
    int sample_rate = params_rate(params), bits_per_frame = 0;

	if (sample_rate) {
		bits_per_frame = zl380tw->sysclk / sample_rate;   
        dev_info(codec->dev, "TDM clk = %d, bits_per_frame = %d, sample_rate = %d", zl380tw->sysclk, bits_per_frame, sample_rate);
    }
    return 0;
}

static int zl380tw_set_dai_fmt(struct snd_soc_dai *codec_dai,
                              unsigned int fmt)
{
    return 0;
}

static int zl380tw_set_dai_sysclk(struct snd_soc_dai *codec_dai, int clk_id,
                                    unsigned int freq, int dir)
{
    struct snd_soc_codec *codec = codec_dai->codec;
    struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);
    zl380tw->sysclk = freq;
    return 0;
}

static const struct snd_soc_dai_ops zl380tw_dai_ops = {
        .set_fmt        = zl380tw_set_dai_fmt,
        .set_sysclk     = zl380tw_set_dai_sysclk,
        .hw_params   	= zl380tw_hw_params,

};

static struct snd_soc_dai_driver zl380tw_dai = {
    .name = "zl380tw-hifi",
    .playback = {
        .stream_name = "Playback",
        .channels_min = zl380tw_DAI_CHANNEL_MIN,
        .channels_max = zl380tw_DAI_CHANNEL_MAX,
        .rates = zl380tw_DAI_RATES,
        .formats = zl380tw_DAI_FORMATS,
    },
    .capture = {
        .stream_name = "Capture",
        .channels_min = zl380tw_DAI_CHANNEL_MIN,
        .channels_max = zl380tw_DAI_CHANNEL_MAX,
        .rates = zl380tw_DAI_RATES,
        .formats = zl380tw_DAI_FORMATS,
    },
    .ops = &zl380tw_dai_ops,
};
EXPORT_SYMBOL(zl380tw_dai);

static int zl380tw_probe(struct snd_soc_codec *codec)
{
    printk(KERN_INFO"Probing zl380tw SoC CODEC driver\n");
    return 0;
}

static int zl380tw_remove(struct snd_soc_codec *codec)
{
    return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_zl380tw = {
    .probe =    zl380tw_probe,
    .remove =   zl380tw_remove,
};
EXPORT_SYMBOL(soc_codec_dev_zl380tw);

/*--------------------------------------------------------------------
 *    ALSA  SOC CODEC driver  - END
 *--------------------------------------------------------------------*/

/*IMPORTANT note: Change this controller string maching accordingly per your *.dts or *dtsi compatible definition file*/
static struct of_device_id zl380tw_of_match[] = {
    { .compatible = "ambarella,zl380snd0"},
    {},
};
MODULE_DEVICE_TABLE(of, zl380tw_of_match);

static int zl380xx_probe(struct platform_device *pdev)
{

    int err = 0;
    
    /* Allocate driver data */
    zl380tw_priv = devm_kzalloc(&pdev->dev, sizeof(*zl380tw_priv), GFP_KERNEL);
    if (zl380tw_priv == NULL)
        return -ENOMEM;

    printk(KERN_INFO"probing zl380tw device\n");
    dev_set_drvdata(&pdev->dev, zl380tw_priv);
    zl380tw_priv->dev = pdev;
    
    err = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_zl380tw, &zl380tw_dai, 1);
    if(err < 0) {
        kfree(zl380tw_priv);
        zl380tw_priv=NULL;
        printk(KERN_ERR"zl380tw I2c device not created!!!\n");
        return err;
    }

    printk(KERN_INFO"zl380xx codec device created...\n");
    return err;
}

static int zl380xx_remove(struct platform_device *pdev)
{
    snd_soc_unregister_codec(&pdev->dev);
    return 0;
}

static struct platform_driver zl380xx_codec_driver = {
    .probe      = zl380xx_probe,
    .remove     = zl380xx_remove,
    .driver = {
    .name   = "zl380snd0", 
    .owner  = THIS_MODULE,
    .of_match_table = zl380tw_of_match,
    },
};

module_platform_driver(zl380xx_codec_driver);

MODULE_AUTHOR("Jean Bony <jean.bony@microsemi.com>");
MODULE_DESCRIPTION(" Microsemi Timberwolf /alsa codec driver");
MODULE_LICENSE("GPL");
