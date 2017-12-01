
OBJS = main.o except_int.o task_one.o task_two.o

SUBDIRS += $(CODEDIR)/share/zl380xx/
IN_CFLAGS += $(CODEDIR)/share/zl380xx

ifeq ($(CONFIG_AUDIO_ENC_EN), 1)

#EXTRA_OBJS += $(BLDDIR)/share/liba_enc.o
#EXTRA_OBJS += $(BLDDIR)/share/liba_ehw2.o
#EXTRA_OBJS += $(BLDDIR)/share/liba_ehw_codec.o
ifeq ($(CONFIG_AUDIO_EFMT_F1), 1)
#EXTRA_OBJS += $(BLDDIR)/share/liba_epost_f111k.o
IN_CFLAGS += -DAUDIO_REC_PCM
#OBJS += 8ks_1s.o
endif
ifeq ($(CONFIG_AUDIO_EFMT_F4), 1)
#EXTRA_OBJS += $(BLDDIR)/share/liba_epost_f4.o
IN_CFLAGS += -DAUDIO_REC_AAC
#OBJS += 44ks_aac.o
endif
ifeq ($(CONFIG_AUDIO_EFMT_F3), 1)
#EXTRA_OBJS += $(BLDDIR)/share/liba_epost_f3.o
IN_CFLAGS += -DAUDIO_REC_AF3
#OBJS += 44ks_af3.o
endif
ifeq ($(CONFIG_AUDIO_EFMT_F2), 1)
#EXTRA_OBJS += $(BLDDIR)/share/liba_epost_f2.o
IN_CFLAGS += -DAUDIO_REC_ADPCM
#OBJS += audio_adpcm.o
endif
ifeq ($(CONFIG_AUDIO_EFMT_F5), 1)
#EXTRA_OBJS += $(BLDDIR)/share/liba_epost_f5.o
IN_CFLAGS += -DAUDIO_REC_G711
endif
ifeq ($(CONFIG_AUDIO_EFMT_F6), 1)
#EXTRA_OBJS += $(BLDDIR)/share/liba_epost_f2.o
IN_CFLAGS += -DAUDIO_REC_ADPCM
#OBJS += audio_adpcm.o
endif
ifeq ($(CONFIG_AUDIO_EFMT_F7), 1)
#EXTRA_OBJS += $(BLDDIR)/share/liba_epost_f111k.o
IN_CFLAGS += -DAUDIO_REC_PCM
#OBJS += 8ks_1s.o
endif

endif

ifeq ($(CONFIG_AUDIO_DEC_EN), 1)
#EXTRA_OBJS += $(BLDDIR)/share/liba_dec.o
#EXTRA_OBJS += $(BLDDIR)/share/liba_dhw_codec.o
#EXTRA_OBJS += $(BLDDIR)/share/liba_dhw2.o
ifeq ($(CONFIG_AUDIO_DFMT_F1), 1)
IN_CFLAGS += -DAUDIO_PLAY_PCM
#EXTRA_OBJS += $(BLDDIR)/share/liba_dec_f1.o
#OBJS += 16km_pcm.o
endif
ifeq ($(CONFIG_AUDIO_DFMT_F2), 1)
IN_CFLAGS += -DAUDIO_PLAY_ADPCM
OBJS += audio_adpcm.o
#EXTRA_OBJS += $(BLDDIR)/share/liba_dec_f2.o
endif
ifeq ($(CONFIG_AUDIO_DFMT_F4), 1)
IN_CFLAGS += -DAUDIO_PLAY_AAC
OBJS += 44ks_aac.o
#EXTRA_OBJS += $(BLDDIR)/share/liba_dec_f4.o
endif
ifeq ($(CONFIG_AUDIO_DFMT_F3), 1)
IN_CFLAGS += -DAUDIO_PLAY_AF3
OBJS += 44ks_af3.o
#EXTRA_OBJS += $(BLDDIR)/share/liba_dec_f3.o
endif
ifeq ($(CONFIG_AUDIO_DFMT_F5), 1)
IN_CFLAGS += -DAUDIO_PLAY_G711
#EXTRA_OBJS += $(BLDDIR)/share/liba_dec_f5.o
endif
ifeq ($(CONFIG_AUDIO_DFMT_F6), 1)
IN_CFLAGS += -DAUDIO_PLAY_ADPCM_IMA
endif
ifeq ($(CONFIG_AUDIO_DFMT_F7), 1)
IN_CFLAGS += -DAUDIO_PLAY_PCM
endif
endif


ifeq ($(CONFIG_AUDIO_ENC_EN), 1)
ifneq ($(CONFIG_AUDIO_DEC_EN), 1)
IN_CFLAGS += -DAUDIO_REC
OBJS += arec_test.o
endif
endif

ifeq ($(CONFIG_AUDIO_DEC_EN), 1)
ifneq ($(CONFIG_AUDIO_ENC_EN), 1)
IN_CFLAGS += -DAUDIO_PLAY
OBJS += aplay_test.o
endif
endif

halfdulex:=1
#shally
#halfdulex:=0
ifeq ($(CONFIG_AUDIO_DEC_EN), 1)
ifeq ($(CONFIG_AUDIO_ENC_EN), 1)
IN_CFLAGS += -DAUDIO_REC_PLAY
ifeq ($(halfdulex), 1)
OBJS += half_aduplex_test.o
endif
ifeq ($(halfdulex), 0)
OBJS += aduplex_test.o
endif
ifeq ($(CONFIG_AUDIO_EFMT_F4), 1)
IN_CFLAGS += -DAUDIO_REC_AAC
endif
ifeq ($(CONFIG_AUDIO_EFMT_F1), 1)
IN_CFLAGS += -DAUDIO_REC_PCM
endif
endif
endif




EXTRA_OBJS += $(BLDDIR)/share/libfs_scif.o
