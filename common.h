
#ifndef _HAVE_COMMON_H
#define _HAVE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>  /* sleep/usleep*/
/* change if your libusb.h is located elswhere */
#include <libusb-1.0/libusb.h>
#include "config.h"
#include "Serial.h"

#define SPINAND_ENV_LEN 0x20000

/* for upgrade */
#define USBD_FLASH_SDRAM			0x0
#define USBD_FLASH_NAND				0x3
#define USBD_FLASH_NAND_RAW		0x4
#define USBD_FLASH_MMC				0x5
#define USBD_FLASH_MMC_RAW		0x6
#define USBD_FLASH_SPI				0x7
#define USBD_FLASH_SPI_RAW		0x8
#define USBD_MTP							0x9
#define USBD_INFO							0xA
#define USBD_BURN_TYPE 				0x80


#define USB_VENDOR_ID		0x0416	/* USB vendor ID used by the device */
#define USB_PRODUCT_ID		0x5963	/* USB product ID used by the device */

#define USB_ENDPOINT_IN		(LIBUSB_ENDPOINT_IN  | 1)	/* endpoint address */
#define USB_ENDPOINT_OUT	(LIBUSB_ENDPOINT_OUT | 2)	/* endpoint address */
#define USB_TIMEOUT		(10000)	/* Connection timeout (in ms) */



#define DDRADDRESS	16
#define BUF_SIZE 	4096

#define MAX_IMAGE 12

#define MODE_SDRAM		0
#define MODE_NAND			1
#define MODE_SPINOR		2
#define MODE_SPINAND	3
#define MODE_SD				4

#define DOWNLOAD			0x10
#define DOWNLOAD_RUN	0x11

#define RUN_PROGRAM					0x21
#define RUN_PROGRAM_VERIFY	0x24
#define RUN_READ						0x22
#define RUN_ERASE						0x23
#define RUN_FORMAT						0x25

typedef struct _PACK_T {
	unsigned int enable_pack;
	char pack_path[256];
} PACK_T,*PPACK_T;

typedef struct _MODE_T {
	int id;
	char *pName;
} MODE_T,*PMODE_T;

typedef struct _IMAGE_T {
	int image_idx;
	int image_type;
	char image_path[256];
	unsigned int image_exe_addr;
	unsigned int image_start_offset;
} IMAGE_T,*PIMAGE_T;

typedef struct _READ_T {
	char path[256];
	unsigned int start_blocks;
	unsigned int offset_blocks;
} READ_T, *PREAD_T;

typedef struct _ERASE_T {
	unsigned int isall;  //0: yes, 1: no
	unsigned int start_blocks;
	unsigned int offset_blocks;
} ERASE_T, *PERASE_T;

typedef struct _SDRAM_T {
	char sdram_path[256];
	char dtb_path[256];
	unsigned int opetion;
	unsigned int dtb_addr;
	unsigned int exe_addr;
} SDRAM_T, *PSDRAM_T;


typedef struct _NU_DATA_T {
	MODE_T mode;
	unsigned int run;
	SDRAM_T *sdram;
	unsigned int image_num;
	char ddr_path[256];
	IMAGE_T image[MAX_IMAGE];
	INFO_T *user_def;
	ERASE_T *erase;
	READ_T *read;

} NU_DATA_T;

extern struct _NU_DATA_T nudata;
extern NORBOOT_MMC_HEAD mmc_head;


#define RUN_ON_XUSB 0x08FF0001

#if 0
#define MSG_DEBUG	printf
#else
#define MSG_DEBUG(...)
#endif

/* load_file.c */
extern char* load_ddr(char *FilePath,int *len);
extern char * load_xusb(char *FilePath,int *len);


/* NuclibUsb.c */
extern int NUC_OpenUsb(void);
extern void NUC_CloseUsb(void);
extern int NUC_SetType(int id,int type);
extern int NUC_ReadPipe(int id,unsigned char *buf,int len);
extern int NUC_WritePipe(int id,unsigned char *buf,int len);
extern int get_device_num_with_vid_pid(libusb_context *ctx,unsigned int vendor_id,unsigned int product_id);

/* device.c */
extern int XUSBtoDevice(unsigned char *buf,unsigned int len);
extern int DDRtoDevice(unsigned char *buf,unsigned int len);
int InfoFromDevice(void);

/* parse.c */
extern int ParseFlashType(void);
extern int init_xusb(void);

/* UXmodem.c */
extern int UXmodem_SDRAM(void);
extern int UXmodem_SPINAND(void);
extern int UXmodem_NAND(void);
extern int UXmodem_SPI(void);
extern int UXmodem_SD(void);
extern int UXmodem_PackImage(void);

/* crc32.c */
unsigned int CalculateCRC32(unsigned char * buf,unsigned int len);

/* gloabel */
extern libusb_context *ctx;
extern libusb_device_handle *handle;

extern MODE_T TypeT[];

extern unsigned int csg_usb_index;
#define MAX_DEV 8
extern libusb_device *dev_arr[MAX_DEV];
extern unsigned int dev_count;
extern unsigned int enable_all_device;
extern PACK_T pack;
#endif
