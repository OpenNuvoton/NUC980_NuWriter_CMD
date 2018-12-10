
#include "common.h"
#include <unistd.h> /* sleep */
#include <string.h>
char * read_xusb_name(char * FilePath)
{

	FILE *fp;
	char *name=NULL;
	unsigned int address,value;
	char *ptmp,tmp[256],cvt[128];
	fp=fopen(FilePath, "r");
	if(fp==NULL) {
		printf("cannot open %s\n",FilePath);
		return NULL;
	}
	name=(char *)malloc(sizeof(char)*20);
	while(fgets(tmp, sizeof(tmp), fp)) {
		ptmp=strchr(tmp,'=');
		if(ptmp==NULL) {
			printf("DDR initial format error\n");
			break;
		}
		strncpy((char *)cvt,(char *)tmp,(unsigned long)ptmp-(unsigned long)tmp);
		cvt[(unsigned long)ptmp-(unsigned long)tmp]='\0';
		address=strtoul(cvt,NULL,0);

		strncpy(cvt,++ptmp,strlen(ptmp));
		cvt[strlen(ptmp)]='\0';
		value=strtoul(cvt,NULL,0);

		if(address==0xB0002010) {
			switch(value&0x7) {
			case 4:
				strcpy(name,"xusb16.bin");
				break;
			case 5:
				strcpy(name,"xusb.bin");
				break;
			case 6:
				strcpy(name,"xusb64.bin");
				break;
			case 7:
				strcpy(name,"xusb128.bin");
				break;
			}
		}
	}
	fclose(fp);
	return (char *)name;
}
int init_xusb(void)
{

	int ret=0;
	char DDR[256];
	char XUSB[256];
	int bResult,dlen,xlen;
	unsigned char *dbuf=NULL,*xbuf=NULL;

	ret=NUC_OpenUsb();
	if(ret<0) return -1;

	dbuf=load_ddr(nudata.ddr_path,&dlen);
	if(dbuf==NULL) {
		ret=-1;
		goto EXIT;
	}
	bResult=DDRtoDevice(dbuf,dlen);
	if(bResult==RUN_ON_XUSB) {
		ret=bResult;
		goto EXIT;
	}
	if(bResult<0) {
		printf("init_xusb:Burn DDR to Device failed,bResult=%d\n",bResult);
		ret=bResult;
		goto EXIT;
	}

	sprintf(XUSB,"%s/%s",NUDATA_PATH,read_xusb_name(nudata.ddr_path));
	xbuf=load_xusb(XUSB,&xlen);
	MSG_DEBUG("XUSB=%s,len=%d\n",XUSB,xlen);
	if(xbuf==NULL) {
		printf("Cannot find xusb.bin\n");
		ret=-1;
		goto EXIT;
	}
	if(XUSBtoDevice(xbuf,xlen)<0) {
		printf("Burn XUSB to Device failed\n");
		ret=-1;
		goto EXIT;
	}
	sleep(2);
	NUC_CloseUsb();
	ret=NUC_OpenUsb();
	if(ret<0) return -1;
EXIT:
	usleep(100);
	return ret;
}

int ParseFlashType(void)
{
	int ret;
	if((ret=init_xusb())<0) {
		printf("initail xusb failed %d\n",ret);
		return -1;
	}

	MSG_DEBUG("InfoFromDevice\n");
	if(InfoFromDevice()<0) {
		printf(" Get information failed for Device\n");
		return -1;
	}

	MSG_DEBUG("Parsing Run mode\n");
	switch(nudata.mode.id) {
	case MODE_SDRAM:
		MSG_DEBUG("ParseFlashType-SDRAM_M\n");
		if(UXmodem_SDRAM()<0) return -1;
		break;
	case MODE_NAND:
		MSG_DEBUG("ParseFlashType-NAND_M\n");
		if(UXmodem_NAND()<0) return -1;
		break;
	case MODE_SPINOR:
		MSG_DEBUG("ParseFlashType-SPI_M\n");
		if(UXmodem_SPI()<0) return -1;
		break;
	case MODE_SD:
		MSG_DEBUG("ParseFlashType-SD_M\n");
		if(UXmodem_SD()<0) return -1;
		break;
	case MODE_SPINAND:
		MSG_DEBUG("ParseFlashType-SPINAND_M\n");
		if(UXmodem_SPINAND()<0) return -1;
		break;
	}

	return 0;
}
