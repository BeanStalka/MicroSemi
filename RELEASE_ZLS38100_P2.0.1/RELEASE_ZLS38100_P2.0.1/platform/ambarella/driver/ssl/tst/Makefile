EXTRA_CFLAGS += -I$(ROOTDIR)/include
EXTRA_CFLAGS += -D$(BUILD_TYPE) 
EXTRA_LDFLAGS += -zmuldefs
#obj-y := ssl.o
#ssl-y := hal_i2c.o ssl.o

ssl_test-y := ../ssl.o ../hal_i2c.o test.o
obj-m := ssl_test.o
#lib-y := hal_i2c.o ssl.o

all:
	make -C $(KSRC) M=$(PWD) modules

clean:
	rm -rf   *.o *.ko .*cmd *.mod.c .tmp_versions modules.order Module.symvers *.a ../*.o ../*.mod.* ../.*cmd
