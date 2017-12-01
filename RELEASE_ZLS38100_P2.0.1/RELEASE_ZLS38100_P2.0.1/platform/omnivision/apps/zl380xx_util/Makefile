OBJS =  except_int.o task_one.o main.o
EXTRA_OBJS += $(BLDDIR)/share/libfs_scif.o
SUBDIRS += $(CODEDIR)/share/zls38130/
IN_CFLAGS += $(CODEDIR)/share/zls38130
CFLAGS += -I$(CODEDIR)/share/zls38130

#FWR_C_FILE := zl38051_firmware126
FWR_C_FILE := zl38051_firmwaresvn5032
#CFGREC_C_FILE := zl38051_config126
#CFGREC_C_FILE := zl38051_config_default

ifeq ($(CONFIG_ZL380XX_HBI_LOAD_FWRCFG), 1)
OBJS += hbi_ldfwcfg.o
OBJS += app_util.o
ifeq ($(CONFIG_ZL380XX_HBI_BOOT_STATIC),1)
ifneq ($(FWR_C_FILE),)
#OBJS += $(FWR_C_FILE).o
IN_CFLAGS += -DFWR_C_FILE=$(FWR_C_FILE).h -DLOAD_FWR_STATIC
endif
endif
ifeq ($(CONFIG_ZL380XX_HBI_LOAD_CFGREC_STATIC),1)
ifneq ($(CFGREC_C_FILE),)
OBJS += $(CFGREC_C_FILE).o
IN_CFLAGS += -DCFGREC_C_FILE=$(CFGREC_C_FILE).h -DLOAD_CFGREC_STATIC
endif
endif
endif
ifeq ($(CONFIG_ZL380XX_TEST_HBI),1)
OBJS += hbi_test.o
endif
