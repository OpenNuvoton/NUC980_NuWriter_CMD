
AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS=nuwriter
nuwriter_SOURCES= common.h Serial.h main.c read_file.c device.c NucLibUsb.c parse.c UXmodem.c crc32.c minini/minIni.c minini/minIni.h minini/minGlue.h run.ini
EXTRA_DIST = nudata README.md

nudata_dir = $(prefix)/share/nudata/
nudata__DATA =	nudata/xusb64.bin \
		nudata/xusb128.bin \
		nudata/xusb.bin \
		nudata/xusb16.bin

syscfg_dir = $(prefix)/share/nudata/sys_cfg
syscfg__DATA =  nudata/sys_cfg/NUC980DF61YC.ini \
		nudata/sys_cfg/NUC980DK61Y.ini \
		nudata/sys_cfg/NUC980DK61YC.ini \
		nudata/sys_cfg/NUC980DR61Y.ini 

#image_dir = $(prefix)/share/nudata/image
#image__DATA = 	nudata/image/nand_uboot_spl.bin \
#		nudata/image/nand_uboot.bin \
#		nudata/image/nand_env.txt \
#		nudata/image/nand_pack.bin \
#		nudata/image/spi_uboot.bin \
#		nudata/image/spi_env.txt \
#		nudata/image/spi_pack.bin \
#		nudata/image/emmc_uboot.bin \
#		nudata/image/emmc_env.txt \
#		nudata/image/emmc_pack.bin \
#		nudata/image/kernel_970uimage

LIBS =$(LIBUSB_CFLAGS)  $(LIBUSB_LIBS) -lpthread
CFLAGS = $(LIBUSB_CFLAGS)

