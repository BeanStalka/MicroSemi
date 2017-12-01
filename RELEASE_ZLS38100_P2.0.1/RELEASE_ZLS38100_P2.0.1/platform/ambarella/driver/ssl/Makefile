include $(ROOTDIR)/Makefile.globals
include $(ROOTDIR)/config.mk

LOCAL_PWD=$(ROOTDIR)/platform/$(PLATFORM)/driver/ssl

obj-y := libssl.o

ifeq ($(HBI),I2C)
	libssl-y := hal_i2c.o 
else
	libssl-y := hal_spi.o
endif

libssl-y += ssl.o

all:help
	make -C $(KSRC) M=$(LOCAL_PWD)
	mv libssl.o $(INSTALL_LIB_PATH)/ssl.o_shipped

help:
	@echo "$(VPROC_DEV_BUS)="$(VPROC_DEV_BUS)

clean:
	rm -rf   *.o *.ko .*cmd *.mod.c .tmp_versions modules.order Module.symvers *.a ../*.o ../*.mod.* ../.*cmd
