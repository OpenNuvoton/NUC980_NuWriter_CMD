
#include "common.h"
#include <string.h>
#include <unistd.h>  /* getopt */
#include <ctype.h>  /* isprint */

int DDRtoDevice(unsigned char *buf,unsigned int len)
{

	unsigned int ack=0;
	AUTOTYPEHEAD head;
	head.address=DDRADDRESS;
	head.filelen=len;
	MSG_DEBUG("head.address=0x%08x,head.filelen=%d\n",head.address,head.filelen);
	NUC_WritePipe(0,(unsigned char*)&head,sizeof(AUTOTYPEHEAD));
	NUC_WritePipe(0,(unsigned char *)buf,len);
	NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
	if(ack==(BUF_SIZE+1))	return RUN_ON_XUSB;
	NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
	if(ack!=0x55AA55AA) return -1;
	MSG_DEBUG("ack=0x%08x\n",ack);
	return 0;
}

int XUSBtoDevice(unsigned char *buf,unsigned int len)
{
	unsigned char SIGNATURE[]= {'W','B',0x5A,0xA5};
	AUTOTYPEHEAD fhead;
	XBINHEAD *xbinhead;
	unsigned int scnt,rcnt,file_len,ack,total;
	unsigned char *pbuf;
	int bResult,pos;
	xbinhead=(XBINHEAD *)buf;
	pbuf=buf+sizeof(XBINHEAD);
	file_len=len-sizeof(XBINHEAD);
	fhead.filelen = file_len;
	MSG_DEBUG("sizeof(XBINHEAD)=%d\n",sizeof(XBINHEAD));
	if(xbinhead->sign==*(unsigned int *)SIGNATURE) {
		MSG_DEBUG("passed xbinhead->address=%x\n",xbinhead->address);
		fhead.address = xbinhead->address;//0x8000;
	} else {
		fhead.address = 0xFFFFFFFF;
	}
	if(fhead.address==0xFFFFFFFF) {
		return -1;
	}
	MSG_DEBUG("file_len=%d\n",file_len);
	NUC_WritePipe(0,(unsigned char*)&fhead,sizeof(AUTOTYPEHEAD));
	scnt=file_len/BUF_SIZE;
	rcnt=file_len%BUF_SIZE;
	total=0;
	while(scnt>0) {
		bResult=NUC_WritePipe(0,(unsigned char*)pbuf,BUF_SIZE);
		if(bResult<0)	return -1;
		pbuf+=BUF_SIZE;
		total+=BUF_SIZE;
		pos=(int)(((float)(((float)total/(float)file_len))*100));
		bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
		if(bResult<0 || ack!=BUF_SIZE) {
			return -1;
		}
		scnt--;
	}
	if(rcnt>0) {
		bResult=NUC_WritePipe(0,(unsigned char*)pbuf,rcnt);
		if(bResult<0) return -1;
		total+=rcnt;
		bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
		MSG_DEBUG("XUSBtoDevice rcnt=%d,ack=%d\n",rcnt,ack);
		if(bResult<0 || ack!=rcnt) return -1;
	}
	return 0;

}

int InfoFromDevice(void)
{
	int bResult;
	unsigned int ack;
	if(NUC_OpenUsb()<0) return -1;
	NUC_SetType(0,INFO);
	MSG_DEBUG("sizeof(INFO_T) %d INFO(%d)\n",sizeof(INFO_T),(long unsigned int)INFO);
	bResult=NUC_WritePipe(0,(UCHAR *)nudata.user_def, sizeof(INFO_T));
	if(bResult<0) goto EXIT;
	bResult=NUC_ReadPipe(0,(UCHAR *)nudata.user_def, sizeof(INFO_T));
	if(bResult<0) goto EXIT;
	bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
	if(bResult<0 || ack!=0x90) goto EXIT;
	return 0;
EXIT:

	return -1;
}
