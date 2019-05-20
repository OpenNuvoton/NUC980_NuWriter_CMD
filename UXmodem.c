#include "common.h"

int DataCompare(unsigned char* base,unsigned char* src,int len)
{
	int i=0;
	for(i=0; i<len; i++) {
		if(base[i]!=src[i])
			return 0;
	}
	return 1;
}

void show_progressbar(int pos)
{
	char progress[128];
	int i;
	progress[0]='|';
	for(i=0; i<50; i++) {
		if(i<((pos/2)))
			progress[i+1]='=';
		else
			progress[i+1]=' ';
	}
	progress[51]='|';
	progress[52]='\0';
	printf("%3d%c%s\r",pos,0x25,progress);
	if(pos==100) {
		memset(progress,' ',128);
		progress[127]='\0';
		printf("%s\r",progress);
	}

}

unsigned char *GetDDRFormat(unsigned int *len)
{
	char DDR[256];
	unsigned char *dbuf,*ddrbuf;
	unsigned int dlen;

	dbuf=load_ddr(nudata.ddr_path,&dlen);
	if(dbuf==NULL) return NULL;

	*len=((dlen+8+15)/16)*16;
	ddrbuf=(unsigned char *)malloc(sizeof(unsigned char)*(*len));
	memset(ddrbuf,0x0,*len);
	*(ddrbuf+0)=0x55;
	*(ddrbuf+1)=0xAA;
	*(ddrbuf+2)=0x55;
	*(ddrbuf+3)=0xAA;
	*((unsigned int *)(ddrbuf+4))=(dlen/8);        /* len */
	memcpy((ddrbuf+8),dbuf,dlen);
	return ddrbuf;
}

int UXmodem_DTB(void)
{
	FILE *fp=NULL;
	int bResult,pos;
	unsigned int scnt,rcnt,file_len,ack,total;
	unsigned char *pbuf,buf[BUF_SIZE];
	SDRAM_RAW_TYPEHEAD fhead;

	if(NUC_OpenUsb()<0) {
		printf("NUC_OpenUsb failed\n");
		return -1;
	}
	NUC_SetType(0,SDRAM);

	MSG_DEBUG("dtb_path = %s\n",nudata.sdram->dtb_path);
	fp=fopen(nudata.sdram->dtb_path, "rb");
	if(fp==NULL) {
		printf("Open write File Error(-w %s) \n",nudata.sdram->dtb_path);
		goto EXIT;
	}
	fseek(fp,0,SEEK_END);
	file_len=ftell(fp);
	fseek(fp,0,SEEK_SET);

	if(!file_len) {
		fclose(fp);
		printf("File length is zero\n");
		goto EXIT;
	}

	MSG_DEBUG("dtb_addr=%x\n",nudata.sdram->dtb_addr);
	fhead.flag=WRITE_ACTION;
	fhead.filelen=file_len;
	fhead.address=nudata.sdram->dtb_addr;
	fhead.dtbaddress = 0;
	//if(nudata.sdram->opetion==DOWNLOAD_RUN) {
	//	fhead.address |= NEED_AUTORUN;
	//}

	//if(nudata.sdram->dtb_addr!=0) {
	//	fhead.dtbaddress = nudata.sdram->dtb_addr | NEED_AUTORUN;
	//}
	NUC_WritePipe(0,(unsigned char*)&fhead,sizeof(SDRAM_RAW_TYPEHEAD));
	NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
	pbuf=buf;
	scnt=file_len/BUF_SIZE;
	rcnt=file_len%BUF_SIZE;
	total=0;
	while(scnt>0) {
		fread(pbuf,1,BUF_SIZE,fp);
		bResult=NUC_WritePipe(0,(unsigned char*)pbuf,BUF_SIZE);
		if(bResult<0)	goto EXIT;
		total+=BUF_SIZE;
		pos=(int)(((float)(((float)total/(float)file_len))*100));
		bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
		if(bResult<0 || ack!=BUF_SIZE) {
			goto EXIT;
		}
		scnt--;
		show_progressbar(pos);
	}
	if(rcnt>0) {
		fread(pbuf,1,rcnt,fp);
		bResult=NUC_WritePipe(0,(unsigned char*)pbuf,rcnt);
		if(bResult<0) goto EXIT;
		total+=rcnt;
		bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
		if(bResult<0) goto EXIT;
	}
	show_progressbar(100);
	fclose(fp);
	return 0;
EXIT:
	NUC_CloseUsb();
	if(fp!=NULL) fclose(fp);
	return -1;

}
int UXmodem_SDRAM(void)
{
	FILE *fp=NULL;
	int bResult,pos;
	unsigned int scnt,rcnt,file_len,ack,total;
	unsigned char *pbuf,buf[BUF_SIZE];
	SDRAM_RAW_TYPEHEAD fhead;

	if(nudata.sdram->dtb_addr!=0) {
		MSG_DEBUG("Burn DTB binary files\n");
		if(UXmodem_DTB()!=0)
			goto EXIT;
	}

	if(NUC_OpenUsb()<0) {
		printf("NUC_OpenUsb failed\n");
		return -1;
	}
	NUC_SetType(0,SDRAM);

	fp=fopen(nudata.sdram->sdram_path, "rb");
	if(fp==NULL) {
		printf("Open write File Error(-w %s) \n",nudata.sdram->sdram_path);
		goto EXIT;
	}
	fseek(fp,0,SEEK_END);
	file_len=ftell(fp);
	fseek(fp,0,SEEK_SET);


	if(!file_len) {
		fclose(fp);
		printf("File length is zero\n");
		goto EXIT;
	}


	fhead.flag=WRITE_ACTION;
	fhead.filelen=file_len;
	fhead.address=nudata.sdram->exe_addr;
	fhead.dtbaddress = 0;
	if(nudata.sdram->opetion==DOWNLOAD_RUN) {
		fhead.address |= NEED_AUTORUN;
	}

	if(nudata.sdram->dtb_addr!=0) {
		fhead.dtbaddress = nudata.sdram->dtb_addr | NEED_AUTORUN;
	}
	NUC_WritePipe(0,(unsigned char*)&fhead,sizeof(SDRAM_RAW_TYPEHEAD));
	NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
	pbuf=buf;
	scnt=file_len/BUF_SIZE;
	rcnt=file_len%BUF_SIZE;
	total=0;
	while(scnt>0) {
		fread(pbuf,1,BUF_SIZE,fp);
		bResult=NUC_WritePipe(0,(unsigned char*)pbuf,BUF_SIZE);
		if(bResult<0)	goto EXIT;
		total+=BUF_SIZE;
		pos=(int)(((float)(((float)total/(float)file_len))*100));
		bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
		if(bResult<0 || ack!=BUF_SIZE) {
			goto EXIT;
		}
		scnt--;
		show_progressbar(pos);
	}
	if(rcnt>0) {
		fread(pbuf,1,rcnt,fp);
		bResult=NUC_WritePipe(0,(unsigned char*)pbuf,rcnt);
		if(bResult<0) goto EXIT;
		total+=rcnt;
		if(nudata.sdram->opetion!=DOWNLOAD_RUN) {
			bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
			if(bResult<0) goto EXIT;
		}
	}
	show_progressbar(100);
	fclose(fp);
	return 0;
EXIT:
	NUC_CloseUsb();
	if(fp!=NULL) fclose(fp);
	return -1;
}

int UXmodem_Pack(void)
{
	int idx=0;
	FILE *fp;
	int i;
	int bResult,pos;
	unsigned int scnt,rcnt,file_len,ack,total;
	unsigned char *pbuf;
	unsigned int magic;
	char* lpBuffer;
	PACK_HEAD *ppackhead;
	PACK_CHILD_HEAD child;
	int posnum,burn_pos;
	unsigned int blockNum;
	NORBOOT_MMC_HEAD *m_fhead;
	m_fhead=malloc(sizeof(NORBOOT_MMC_HEAD));

	fp=fopen(nudata.image[idx].image_path, "rb");
	if(fp==NULL) {
		printf("Open read File Error(-w %s) \n",nudata.image[idx].image_path);
		return -1;
	}
	fread((unsigned char *)&magic,4,1,fp);
	if(magic!=0x5) {
		fclose(fp);
		printf("Pack Image Format Error\n");
		goto EXIT;
	}

	fseek(fp,0,SEEK_END);
	file_len=ftell(fp);
	fseek(fp,0,SEEK_SET);
	if(!file_len) {
		fclose(fp);
		printf("File length is zero\n");
		goto EXIT;
	}
	lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*file_len); //read file to buffer
	memset(lpBuffer,0xff,file_len);
	memset((unsigned char *)m_fhead,0,sizeof(NORBOOT_MMC_HEAD));
	if(nudata.mode.id!=MODE_SD) {
		((NORBOOT_NAND_HEAD *)m_fhead)->flag=PACK_ACTION;
		((NORBOOT_NAND_HEAD *)m_fhead)->type=nudata.image[idx].image_type;
		((NORBOOT_NAND_HEAD *)m_fhead)->initSize=0;
		((NORBOOT_NAND_HEAD *)m_fhead)->filelen=file_len;
		bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_NAND_HEAD));
		bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
		fread(lpBuffer,1,((NORBOOT_NAND_HEAD *)m_fhead)->filelen,fp);
	} else {
		((NORBOOT_MMC_HEAD *)m_fhead)->flag=PACK_ACTION;
		((NORBOOT_MMC_HEAD *)m_fhead)->type=nudata.image[idx].image_type;
		((NORBOOT_MMC_HEAD *)m_fhead)->initSize=0;
		((NORBOOT_MMC_HEAD *)m_fhead)->filelen=file_len;
		bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_MMC_HEAD));
		bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
		fread(lpBuffer,1,((NORBOOT_MMC_HEAD *)m_fhead)->filelen,fp);
	}


	fclose(fp);

	pbuf = lpBuffer;
	ppackhead=(PACK_HEAD *)lpBuffer;
	bResult=NUC_WritePipe(0,(UCHAR *)pbuf, sizeof(PACK_HEAD));
	bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
	total+= sizeof(PACK_HEAD);
	pbuf+= sizeof(PACK_HEAD);
	posnum=0;
	for(i=0; i<(int)(ppackhead->num); i++) {
		total=0;
		memcpy(&child,(char *)pbuf,sizeof(PACK_CHILD_HEAD));
		bResult=NUC_WritePipe(0,(UCHAR *)pbuf, sizeof(PACK_CHILD_HEAD));
		if(bResult<0) goto EXIT;
		if(nudata.mode.id==MODE_NAND || nudata.mode.id==MODE_SD) {
			bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
			if(bResult<0) goto EXIT;
		}
		pbuf+= sizeof(PACK_CHILD_HEAD);

		scnt=child.filelen/BUF_SIZE;
		rcnt=child.filelen%BUF_SIZE;
		total=0;

		while(scnt>0) {
			bResult=NUC_WritePipe(0,(UCHAR *)pbuf, BUF_SIZE);
			if(bResult<0) goto EXIT;
			pbuf+=BUF_SIZE;
			total+=BUF_SIZE;
			pos=(int)(((float)(((float)total/(float)child.filelen))*100));
			printf("Pack image%d ... ",i);
			show_progressbar(pos);
			bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
			if(bResult<0) goto EXIT;
			scnt--;
		}
		if(rcnt>0) {

			bResult=NUC_WritePipe(0,(UCHAR *)pbuf,rcnt);
			if(bResult<0) goto EXIT;
			pbuf+=rcnt;
			total+=rcnt;
			bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
			if(bResult<0) goto EXIT;
			pos=(int)(((float)(((float)total/(float)child.filelen))*100));
			printf("Pack image%d ... ",i);
			show_progressbar(pos);
		}
		posnum+=100;
		if(nudata.mode.id==MODE_NAND || nudata.mode.id==MODE_SD) {
			bResult=NUC_ReadPipe(0,(UCHAR *)&blockNum,4);
			if(bResult<0) goto EXIT;
		} else if(nudata.mode.id==MODE_SPINOR) {
			burn_pos=0;
			while(burn_pos!=100) {
				bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
				if(bResult<0) goto EXIT;
				if(!((ack>>16)&0xffff)) {
					burn_pos=(UCHAR)(ack&0xffff);
				} else {
					goto EXIT;
				}
			}
		}
	} //for(i=0; i<(int)(ppackhead->num); i++) end
	free(lpBuffer);
	free(m_fhead);
	show_progressbar(100);
	printf("Pack image ... Passed\n");
	return 0;
EXIT:
	free(lpBuffer);
	free(m_fhead);
	return -1;
}

int UXmodem_NAND(void)
{
	int idx=0;
	FILE *fp;
	int bResult,pos;
	unsigned int scnt,rcnt,file_len,ack,total;
	unsigned char *pbuf,buf[BUF_SIZE];
	NORBOOT_NAND_HEAD *m_fhead;
	char DDR[256];
	unsigned char *ddrbuf=NULL;
	unsigned int ddrlen;
	char* lpBuffer;
	int blockNum;

	m_fhead=malloc(sizeof(NORBOOT_NAND_HEAD));
	if((nudata.user_def->Nand_uPageSize==0 ||nudata.user_def->Nand_uPagePerBlock==0) && (nudata.user_def->Nand_uIsUserConfig==1) ) {
		printf("Cannot find NAND(%d,%d)\n",nudata.user_def->Nand_uPageSize,nudata.user_def->Nand_uPagePerBlock);
		return -1;
	}
	if(NUC_OpenUsb()<0) return -1;
	NUC_SetType(0,NAND);


	if(nudata.image[idx].image_type==PACK) {
		if(UXmodem_Pack()<0) goto EXIT;
	} else {

		if(nudata.run==RUN_READ) {
			unsigned char temp[BUF_SIZE];
			FILE* tempfp;
			//-----------------------------------
			fp=fopen(nudata.read->path,"w+b");
			if(fp==NULL) {
				printf("Open write File Error(-w %s) \n",nudata.read->path);
				goto EXIT;
			}
			file_len = nudata.read->offset_blocks*nudata.user_def->Nand_uPageSize*nudata.user_def->Nand_uPagePerBlock;
			m_fhead->flag=READ_ACTION;
			m_fhead->flashoffset=nudata.read->start_blocks;
			m_fhead->filelen=file_len;
			m_fhead->initSize=0; //read good block
			bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_NAND_HEAD));
			bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

			scnt=file_len/BUF_SIZE;
			rcnt=file_len%BUF_SIZE;
			total=0;
			while(scnt>0) {
				bResult=NUC_ReadPipe(0,(unsigned char*)temp, BUF_SIZE);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				total+=BUF_SIZE;
				pos=(int)(((float)(((float)total/(float)file_len))*100));
				printf("Read ... ");
				show_progressbar(pos);
				fwrite(temp,1,BUF_SIZE,fp);
				ack=BUF_SIZE;
				bResult=NUC_WritePipe(0,(UCHAR *)&ack,4);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				scnt--;
			}

			if(rcnt>0) {
				bResult=NUC_ReadPipe(0,(UCHAR *)temp,BUF_SIZE);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				total+=rcnt;
				fwrite(temp,1,rcnt,fp);
				ack=BUF_SIZE;
				bResult=NUC_WritePipe(0,(UCHAR *)&ack,4);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				pos=(int)(((float)(((float)total/(float)file_len))*100));
				printf("Read ... ");
				show_progressbar(pos);
			}
			fclose(fp);
			show_progressbar(100);
			printf("Read ... Passed\n");
		}

		if(nudata.run==RUN_PROGRAM || nudata.run==RUN_PROGRAM_VERIFY ) { //Burn Image
			for(idx=0; idx<nudata.image_num; idx++) {
				if(idx>0) {
					if(NUC_OpenUsb()<0) return -1;
					NUC_SetType(0,NAND);
					usleep(10);
				}
				fp=fopen(nudata.image[idx].image_path, "rb");
				if(fp==NULL) {
					printf("Open read File Error(-w %s) \n",nudata.image[idx].image_path);
					goto EXIT;
				}
				printf("image%d %s\n",idx,nudata.image[idx].image_path);
				fseek(fp,0,SEEK_END);
				file_len=ftell(fp);
				fseek(fp,0,SEEK_SET);

				if(!file_len) {
					fclose(fp);
					printf("File length is zero\n");
					goto EXIT;
				}
				memset((unsigned char *)m_fhead,0,sizeof(NORBOOT_NAND_HEAD));
				m_fhead->flag=WRITE_ACTION;
				m_fhead->initSize=0;
				m_fhead->filelen=file_len;
				switch(nudata.image[idx].image_type) {
				case DATA:
				case PACK:
					m_fhead->no=0;
					m_fhead->execaddr = 0x200;
					m_fhead->flashoffset = nudata.image[idx].image_start_offset;
					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*file_len); //read file to buffer
					memset(lpBuffer,0xff,file_len);
					m_fhead->macaddr[7]=0;
					m_fhead->type=nudata.image[idx].image_type;

					bResult=NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
					fread(lpBuffer,1,m_fhead->filelen,fp);
					break;
				case ENV:
					m_fhead->no=0;
					m_fhead->execaddr = 0x200;
					m_fhead->flashoffset = nudata.image[idx].image_start_offset;
					if(file_len>(0x10000-4)) {
						fclose(fp);
						printf("The environment file size is less then 64KB\n");
						goto EXIT;
					}
					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*0x10000); //read file to buffer
					memset(lpBuffer,0x00,0x10000);

					((NORBOOT_NAND_HEAD *)m_fhead)->macaddr[7]=0;

					m_fhead->filelen=0x10000;
					m_fhead->type=nudata.image[idx].image_type;

					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

					{
						char line[256];
						char* ptr=(char *)(lpBuffer+4);
						while (1) {
							if (fgets(line,256, fp) == NULL) break;
							if(line[strlen(line)-2]==0x0D && line[strlen(line)-1]==0x0A) {
								strncpy(ptr,line,strlen(line)-1);
								ptr[strlen(line)-2]=0x0;
								ptr+=(strlen(line)-1);
							}else if(line[strlen(line)-1]==0x0A) {
								strncpy(ptr,line,strlen(line));
								ptr[strlen(line)-1]=0x0;
								ptr+=(strlen(line));
							} else {
								strncpy(ptr,line,strlen(line));
								ptr+=(strlen(line));
							}
						}

					}
					*(unsigned int *)lpBuffer=CalculateCRC32((unsigned char *)(lpBuffer+4),0x10000-4);
					file_len=0x10000;
					break;
				case LOADER:
					m_fhead->no=0;
					m_fhead->execaddr = nudata.image[idx].image_exe_addr;
					m_fhead->flashoffset = 0;
					ddrbuf=GetDDRFormat(&ddrlen);
					file_len=file_len+ddrlen;
					m_fhead->initSize=ddrlen;

					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*file_len);
					memset(lpBuffer,0xff,file_len);
					m_fhead->macaddr[7]=0;

					m_fhead->type=nudata.image[idx].image_type;

					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

					memcpy(lpBuffer,ddrbuf,ddrlen);
					fread(lpBuffer+ddrlen,1,m_fhead->filelen,fp);
					break;
				}
				fclose(fp);

#if 1
				pbuf=lpBuffer;
				scnt=file_len/BUF_SIZE;
				rcnt=file_len%BUF_SIZE;
				total=0;
				while(scnt>0) {
					bResult=NUC_WritePipe(0,(unsigned char*)pbuf,BUF_SIZE);
					if(bResult<0)
						goto EXIT;
					pbuf+=BUF_SIZE;
					total+=BUF_SIZE;
					pos=(int)(((float)(((float)total/(float)file_len))*100));
					bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
					if(bResult<0 || ack!=BUF_SIZE) {
						goto EXIT;
					}
					scnt--;
					printf("Write image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
					show_progressbar(pos);
				}
				if(rcnt>0) {
					bResult=NUC_WritePipe(0,(unsigned char*)pbuf,rcnt);
					if(bResult<0) goto EXIT;
					total+=rcnt;
					bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
					if(bResult<0 || ack!=rcnt) goto EXIT;
				}
				printf("Write image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
				show_progressbar(pos);
				bResult=NUC_ReadPipe(0,(UCHAR *)&blockNum,4);
				if(bResult<0) goto EXIT;
				show_progressbar(100);
				printf("Write image%d %s ... Passed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
				if((nudata.run==RUN_PROGRAM_VERIFY)) {
					unsigned char temp[BUF_SIZE];

					if(NUC_OpenUsb()<0) return -1;
					NUC_SetType(0,NAND);
					m_fhead->flag=VERIFY_ACTION;
					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,sizeof(unsigned int));
					pbuf = lpBuffer+m_fhead->initSize;
					scnt=(file_len-m_fhead->initSize)/BUF_SIZE;
					rcnt=(file_len-m_fhead->initSize)%BUF_SIZE;
					total=0;
					while(scnt>0) {
						bResult=NUC_ReadPipe(0,(unsigned char*)temp, BUF_SIZE);
						if(bResult>=0) {
							total+=BUF_SIZE;
							if(DataCompare(temp,pbuf,BUF_SIZE))
								ack=BUF_SIZE;
							else
								ack=0;//compare error
							bResult=NUC_WritePipe(0,(unsigned char*)&ack,4);
							if(bResult<0) {
								printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
								goto EXIT;
							}
							pos=(int)(((float)(((float)total/(float)file_len))*100));
							printf("Verify %s ... ",TypeT[nudata.image[idx].image_idx].pName);
							show_progressbar(pos);
							pbuf+=BUF_SIZE;
						} else {
							printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
							goto EXIT;
						}
						scnt--;
					}

					if(rcnt>0) {
						bResult=NUC_ReadPipe(0,(UCHAR *)temp,BUF_SIZE);
						if(bResult>=0) {
							total+=rcnt;
							if(DataCompare(temp,pbuf,rcnt))
								ack=BUF_SIZE;
							else
								ack=0;//compare error
							bResult=NUC_WritePipe(0,(UCHAR *)&ack, 4);
							if((bResult<0)||(!ack)) {
								printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
								goto EXIT;
							}

						} else {
							printf("Verify image%d %s... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
							goto EXIT;
						}

						pos=(int)(((float)(((float)total/(float)file_len))*100));
						printf("Verify %s ... ",TypeT[nudata.image[idx].image_idx].pName);
						show_progressbar(pos);
					}
					show_progressbar(100);
					printf("Verify image%d %s ... Passed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
				} //Verify_tag end

				free(lpBuffer);
#endif
			}
		}

		if(nudata.run==RUN_ERASE) {  //Erase Nand
			int wait_pos=0;
			unsigned int erase_pos=0;
			m_fhead->flag=ERASE_ACTION;
			m_fhead->flashoffset = nudata.erase->start_blocks; //start erase block
			m_fhead->execaddr=nudata.erase->offset_blocks;  //erase block length

			/* Decide chip erase mode or erase mode                    */
			/* 0: chip erase, 1: erase accord start and length blocks. */
			if(nudata.erase->offset_blocks==0xFFFFFFFF)
				m_fhead->type=0;
			else
				m_fhead->type=1;

			//m_fhead->no=0xFFFFFFFF;//erase good block
			m_fhead->no=0xFFFFFFFE;//erase good block and bad block

			bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_NAND_HEAD));
			bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);

			erase_pos=0;
			while(erase_pos!=100) {

				bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
				if(bResult<0) goto EXIT;
				if(((ack>>16)&0xffff)) goto EXIT;
				erase_pos=ack&0xffff;
				printf("Erase ... ");
				show_progressbar(erase_pos);
				if(erase_pos==95) {
					wait_pos++;
					if(wait_pos>100) {
						goto EXIT;
					}
				}

			}
			show_progressbar(100);
			printf("Erase ... Passed\n");
		}
	}
	free(m_fhead);
	return 0;

EXIT:

	return -1;
}

int UXmodem_SPINAND(void)
{
	int idx;
	FILE *fp;
	int bResult,pos;
	unsigned int scnt,rcnt,file_len,ack,total;
	unsigned char *pbuf,buf[BUF_SIZE];
	NORBOOT_NAND_HEAD *m_fhead;
	char DDR[256];
	unsigned char *ddrbuf=NULL;
	unsigned int ddrlen;
	char* lpBuffer;
	int blockNum;
	m_fhead=malloc(sizeof(NORBOOT_NAND_HEAD));

	if(NUC_OpenUsb()<0) return -1;
	NUC_SetType(0,SPINAND);

	if(nudata.image[0].image_type==PACK) {
		if(UXmodem_Pack()<0) goto EXIT;
	} else {

		if(nudata.run==RUN_READ) {
			unsigned char temp[BUF_SIZE];
			FILE* tempfp;
			//-----------------------------------
			fp=fopen(nudata.read->path,"w+b");
			if(fp==NULL) {
				printf("Open write File Error(-w %s) \n",nudata.read->path);
				goto EXIT;
			}
			file_len = nudata.read->offset_blocks*(nudata.user_def->SPINand_PagePerBlock * nudata.user_def->SPINand_PageSize);
			m_fhead->flag=READ_ACTION;
			m_fhead->flashoffset=nudata.read->start_blocks*(nudata.user_def->SPINand_PagePerBlock * nudata.user_def->SPINand_PageSize);
			m_fhead->filelen=file_len;
			m_fhead->initSize=0; //read good block
			bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_NAND_HEAD));
			bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

			scnt=file_len/BUF_SIZE;
			rcnt=file_len%BUF_SIZE;
			total=0;
			while(scnt>0) {
				bResult=NUC_ReadPipe(0,(unsigned char*)temp, BUF_SIZE);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				total+=BUF_SIZE;
				pos=(int)(((float)(((float)total/(float)file_len))*100));
				printf("Read ... ");
				show_progressbar(pos);
				fwrite(temp,1,BUF_SIZE,fp);
				ack=BUF_SIZE;
				bResult=NUC_WritePipe(0,(UCHAR *)&ack,4);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				scnt--;
			}

			if(rcnt>0) {
				bResult=NUC_ReadPipe(0,(UCHAR *)temp,BUF_SIZE);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				total+=rcnt;
				fwrite(temp,1,rcnt,fp);
				ack=BUF_SIZE;
				bResult=NUC_WritePipe(0,(UCHAR *)&ack,4);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				pos=(int)(((float)(((float)total/(float)file_len))*100));
				printf("Read ... ");
				show_progressbar(pos);
			}
			fclose(fp);
			show_progressbar(100);
			printf("Read ... Passed\n");
		}

		if(nudata.run==RUN_PROGRAM || nudata.run==RUN_PROGRAM_VERIFY) { //Burn Image
			for(idx=0; idx<nudata.image_num; idx++) {
				if(idx>0) {
					if(NUC_OpenUsb()<0) return -1;
					NUC_SetType(0,SPINAND);
					usleep(10);
				}
				fp=fopen(nudata.image[idx].image_path, "rb");
				if(fp==NULL) {
					printf("Open read File Error(%s) \n",nudata.image[idx].image_path);
					goto EXIT;
				}
				printf("image%d %s\n",idx,nudata.image[idx].image_path);

				fseek(fp,0,SEEK_END);
				file_len=ftell(fp);
				fseek(fp,0,SEEK_SET);

				if(!file_len) {
					fclose(fp);
					printf("File length is zero\n");
					goto EXIT;
				}
				memset((unsigned char *)m_fhead,0,sizeof(NORBOOT_NAND_HEAD));
				m_fhead->flag=WRITE_ACTION;
				m_fhead->initSize=0;
				m_fhead->filelen=file_len;
				switch(nudata.image[idx].image_type) {
				case DATA:
				case PACK:
					m_fhead->no=0;
					m_fhead->execaddr = 0x200;
					m_fhead->flashoffset = nudata.image[idx].image_start_offset;
					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*file_len); //read file to buffer
					memset(lpBuffer,0xff,file_len);
					((NORBOOT_NAND_HEAD *)m_fhead)->macaddr[7]=0;
					m_fhead->type=nudata.image[idx].image_type;
					bResult=NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
					if(fread(lpBuffer,1,m_fhead->filelen,fp)!=m_fhead->filelen) {
						fclose(fp);
						printf("fjile length is longer\n");
						goto EXIT;
					}
					break;
				case ENV:
					m_fhead->no=0;
					m_fhead->execaddr = 0x200;
					m_fhead->flashoffset = nudata.image[idx].image_start_offset;
					if(file_len>(0x10000-4)) {
						fclose(fp);
						printf("The environment file size is less then 64KB\n");
						goto EXIT;
					}
					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*0x10000); //read file to buffer
					memset(lpBuffer,0x00,0x10000);

					((NORBOOT_NAND_HEAD *)m_fhead)->macaddr[7]=0;

					m_fhead->filelen=0x10000;
					m_fhead->type=nudata.image[idx].image_type;

					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

					{
						char line[256];
						char* ptr=(char *)(lpBuffer+4);
						while (1) {
							if (fgets(line,256, fp) == NULL) break;
							if(line[strlen(line)-2]==0x0D && line[strlen(line)-1]==0x0A) {
								strncpy(ptr,line,strlen(line)-1);
								ptr[strlen(line)-2]=0x0;
								ptr+=(strlen(line)-1);
							}else if(line[strlen(line)-1]==0x0A) {
								strncpy(ptr,line,strlen(line));
								ptr[strlen(line)-1]=0x0;
								ptr+=(strlen(line));
							} else {
								strncpy(ptr,line,strlen(line));
								ptr+=(strlen(line));
							}
						}

					}
					*(unsigned int *)lpBuffer=CalculateCRC32((unsigned char *)(lpBuffer+4),0x10000-4);
					file_len=0x10000;
					break;
				case LOADER:
					m_fhead->no=0;
					m_fhead->execaddr = nudata.image[idx].image_exe_addr;
					m_fhead->flashoffset = 0;
					ddrbuf=GetDDRFormat(&ddrlen);
					file_len=file_len+ddrlen;
					((NORBOOT_NAND_HEAD *)m_fhead)->initSize=ddrlen;

					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*file_len);
					memset(lpBuffer,0xff,file_len);
					((NORBOOT_NAND_HEAD *)m_fhead)->macaddr[7]=0;

					m_fhead->type=nudata.image[idx].image_type;

					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
					memcpy(lpBuffer,ddrbuf,ddrlen);
					fread(lpBuffer+ddrlen,1,m_fhead->filelen,fp);
					break;
				}
				fclose(fp);
#if 1
				pbuf=lpBuffer;
				scnt=file_len/BUF_SIZE;
				rcnt=file_len%BUF_SIZE;
				total=0;
				while(scnt>0) {
					bResult=NUC_WritePipe(0,(unsigned char*)pbuf,BUF_SIZE);
					if(bResult<0) {
						goto EXIT;
					}
					pbuf+=BUF_SIZE;
					total+=BUF_SIZE;
					pos=(int)(((float)(((float)total/(float)file_len))*100));
					bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
					if(bResult<0 || ack!=BUF_SIZE) {
						goto EXIT;
					}
					scnt--;
					printf("Write image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
					show_progressbar(pos);
				}
				if(rcnt>0) {
					bResult=NUC_WritePipe(0,(unsigned char*)pbuf,rcnt);
					if(bResult<0) {
						goto EXIT;
					}
					total+=rcnt;
					bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
					if(bResult<0 || ack!=rcnt) {
						goto EXIT;
					}
				}
				printf("Write image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
				show_progressbar(pos);
				bResult=NUC_ReadPipe(0,(UCHAR *)&blockNum,4);
				if(bResult<0) goto EXIT;
				show_progressbar(100);
				printf("Write image%d %s ... Passed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
				if((nudata.run==RUN_PROGRAM_VERIFY)) {
					unsigned char temp[BUF_SIZE];

					if(NUC_OpenUsb()<0) return -1;
					NUC_SetType(0,SPINAND);
					m_fhead->flag=VERIFY_ACTION;
					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,sizeof(unsigned int));
					pbuf = lpBuffer+m_fhead->initSize;
					scnt=(file_len-m_fhead->initSize)/BUF_SIZE;
					rcnt=(file_len-m_fhead->initSize)%BUF_SIZE;
					total=0;
					while(scnt>0) {
						bResult=NUC_ReadPipe(0,(unsigned char*)temp, BUF_SIZE);
						if(bResult>=0) {
							total+=BUF_SIZE;
							if(DataCompare(temp,pbuf,BUF_SIZE))
								ack=BUF_SIZE;
							else
								ack=0;//compare error
							bResult=NUC_WritePipe(0,(unsigned char*)&ack,4);
							if(bResult<0) {
								printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
								goto EXIT;
							}
							pos=(int)(((float)(((float)total/(float)file_len))*100));
							printf("Verify image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
							show_progressbar(pos);
							pbuf+=BUF_SIZE;
						} else {
							printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
							goto EXIT;
						}
						scnt--;
					}

					if(rcnt>0) {
						bResult=NUC_ReadPipe(0,(UCHAR *)temp,BUF_SIZE);
						if(bResult>=0) {
							total+=rcnt;
							if(DataCompare(temp,pbuf,rcnt))
								ack=BUF_SIZE;
							else
								ack=0;//compare error
							bResult=NUC_WritePipe(0,(UCHAR *)&ack, 4);
							if((bResult<0)||(!ack)) {
								printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
								goto EXIT;
							}

						} else {
							printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
							goto EXIT;
						}

						pos=(int)(((float)(((float)total/(float)file_len))*100));
						printf("Verify image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
						show_progressbar(pos);
					}
					show_progressbar(100);
					printf("Verify image%d %s... Passed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
				} //Verify_tag end

				free(lpBuffer);
#endif
			}
			free(m_fhead);
		}


		if(nudata.run==RUN_ERASE) {  //Erase SPI
			int wait_pos=0;
			unsigned int erase_pos=0;
			m_fhead->flag=ERASE_ACTION;
			m_fhead->flashoffset = nudata.erase->start_blocks; //start erase block
			m_fhead->execaddr=nudata.erase->offset_blocks;  //erase block length

			/* Decide chip erase mode or erase mode                    */
			/* 0: chip erase, 1: erase accord start and length blocks. */
			if(nudata.erase->offset_blocks==0xFFFFFFFF) {
				m_fhead->type=0;
				m_fhead->no=0xffffffff;//erase all
			} else {
				m_fhead->type=1;
				m_fhead->no=0;
			}

			bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_NAND_HEAD));
			bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
			erase_pos=0;
			printf("Erase ... ");
			show_progressbar(erase_pos);
			while(erase_pos!=100) {
				bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
				if(bResult<0) goto EXIT;
				if(((ack>>16)&0xffff)) goto EXIT;
				erase_pos=ack&0xffff;
				printf("Erase ... ");
				show_progressbar(erase_pos);
				if(erase_pos==95) {
					wait_pos++;
					if(wait_pos>100) {
						goto EXIT;
					}
				}

			}
			show_progressbar(100);
			printf("Erase ... Passed\n");
		}
	}

	return 0;

EXIT:

	return -1;
}

int UXmodem_SPI(void)
{
	int idx;
	FILE *fp;
	int bResult,pos;
	unsigned int scnt,rcnt,file_len,ack,total;
	unsigned char *pbuf,buf[BUF_SIZE];
	NORBOOT_NAND_HEAD *m_fhead;
	char DDR[256];
	unsigned char *ddrbuf=NULL;
	unsigned int ddrlen;
	char* lpBuffer;
	unsigned int burn_pos;
	m_fhead=malloc(sizeof(NORBOOT_NAND_HEAD));

	if(NUC_OpenUsb()<0) return -1;
	NUC_SetType(0,SPI);

	if(nudata.image[0].image_type==PACK) {
		if(UXmodem_Pack()<0) goto EXIT;
	} else {

		if(nudata.run==RUN_READ) {
			unsigned char temp[BUF_SIZE];
			FILE* tempfp;
			//-----------------------------------
			fp=fopen(nudata.read->path,"w+b");
			if(fp==NULL) {
				printf("Open write File Error(-w %s) \n",nudata.read->path);
				goto EXIT;
			}
			file_len = nudata.read->offset_blocks*(SPI64K);//*m_info.Nand_uPageSize*m_info.Nand_uPagePerBlock;
			m_fhead->flag=READ_ACTION;
			m_fhead->flashoffset=nudata.read->start_blocks*(SPI64K);
			m_fhead->filelen=file_len;
			m_fhead->initSize=0; //read good block
			bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_NAND_HEAD));
			bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

			scnt=file_len/BUF_SIZE;
			rcnt=file_len%BUF_SIZE;
			total=0;
			while(scnt>0) {
				bResult=NUC_ReadPipe(0,(unsigned char*)temp, BUF_SIZE);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				total+=BUF_SIZE;
				pos=(int)(((float)(((float)total/(float)file_len))*100));
				printf("Read ... ");
				show_progressbar(pos);
				fwrite(temp,1,BUF_SIZE,fp);
				ack=BUF_SIZE;
				bResult=NUC_WritePipe(0,(UCHAR *)&ack,4);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				scnt--;
			}

			if(rcnt>0) {
				bResult=NUC_ReadPipe(0,(UCHAR *)temp,BUF_SIZE);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				total+=rcnt;
				fwrite(temp,1,rcnt,fp);
				ack=BUF_SIZE;
				bResult=NUC_WritePipe(0,(UCHAR *)&ack,4);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				pos=(int)(((float)(((float)total/(float)file_len))*100));
				printf("Read ... ");
				show_progressbar(pos);
			}
			fclose(fp);
			show_progressbar(100);
			printf("Read ... Passed\n");
		}

		if(nudata.run==RUN_PROGRAM || nudata.run==RUN_PROGRAM_VERIFY) { //Burn Image
			for(idx=0; idx<nudata.image_num; idx++) {
				if(idx>0) {
					if(NUC_OpenUsb()<0) return -1;
					NUC_SetType(0,SPI);
					usleep(10);
				}
				fp=fopen(nudata.image[idx].image_path, "rb");
				if(fp==NULL) {
					printf("Open read File Error(%s) \n",nudata.image[idx].image_path);
					goto EXIT;
				}
				printf("image%d %s\n",idx,nudata.image[idx].image_path);

				fseek(fp,0,SEEK_END);
				file_len=ftell(fp);
				fseek(fp,0,SEEK_SET);

				if(!file_len) {
					fclose(fp);
					printf("File length is zero\n");
					goto EXIT;
				}
				memset((unsigned char *)m_fhead,0,sizeof(NORBOOT_NAND_HEAD));
				m_fhead->flag=WRITE_ACTION;
				m_fhead->initSize=0;
				m_fhead->filelen=file_len;
				switch(nudata.image[idx].image_type) {
				case DATA:
				case PACK:
					m_fhead->no=0;
					m_fhead->execaddr = 0x200;
					m_fhead->flashoffset = nudata.image[idx].image_start_offset;
					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*file_len); //read file to buffer
					memset(lpBuffer,0xff,file_len);
					((NORBOOT_NAND_HEAD *)m_fhead)->macaddr[7]=0;
					m_fhead->type=nudata.image[idx].image_type;
					bResult=NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
					if(fread(lpBuffer,1,m_fhead->filelen,fp)!=m_fhead->filelen) {
						fclose(fp);
						printf("fjile length is longer\n");
						goto EXIT;
					}
					break;
				case ENV:
					m_fhead->no=0;
					m_fhead->execaddr = 0x200;
					m_fhead->flashoffset = nudata.image[idx].image_start_offset;
					if(file_len>(0x10000-4)) {
						fclose(fp);
						printf("The environment file size is less then 64KB\n");
						goto EXIT;
					}
					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*0x10000); //read file to buffer
					memset(lpBuffer,0x00,0x10000);

					((NORBOOT_NAND_HEAD *)m_fhead)->macaddr[7]=0;

					m_fhead->filelen=0x10000;
					m_fhead->type=nudata.image[idx].image_type;

					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

					{
						char line[256];
						char* ptr=(char *)(lpBuffer+4);
						while (1) {
							if (fgets(line,256, fp) == NULL) break;
							if(line[strlen(line)-2]==0x0D && line[strlen(line)-1]==0x0A) {
								strncpy(ptr,line,strlen(line)-1);
								ptr[strlen(line)-2]=0x0;
								ptr+=(strlen(line)-1);
							}else if(line[strlen(line)-1]==0x0A) {
								strncpy(ptr,line,strlen(line));
								ptr[strlen(line)-1]=0x0;
								ptr+=(strlen(line));
							} else {
								strncpy(ptr,line,strlen(line));
								ptr+=(strlen(line));
							}
						}

					}
					*(unsigned int *)lpBuffer=CalculateCRC32((unsigned char *)(lpBuffer+4),0x10000-4);
					file_len=0x10000;
					break;
				case LOADER:
					m_fhead->no=0;
					m_fhead->execaddr = nudata.image[idx].image_exe_addr;
					m_fhead->flashoffset = 0;
					ddrbuf=GetDDRFormat(&ddrlen);
					file_len=file_len+ddrlen;
					((NORBOOT_NAND_HEAD *)m_fhead)->initSize=ddrlen;

					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*file_len);
					memset(lpBuffer,0xff,file_len);
					((NORBOOT_NAND_HEAD *)m_fhead)->macaddr[7]=0;

					m_fhead->type=nudata.image[idx].image_type;

					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

					memcpy(lpBuffer,ddrbuf,ddrlen);
					fread(lpBuffer+ddrlen,1,m_fhead->filelen,fp);
					break;
				}
				fclose(fp);
#if 1
				pbuf=lpBuffer;
				scnt=file_len/BUF_SIZE;
				rcnt=file_len%BUF_SIZE;
				total=0;
				while(scnt>0) {
					bResult=NUC_WritePipe(0,(unsigned char*)pbuf,BUF_SIZE);
					if(bResult<0)
						goto EXIT;
					pbuf+=BUF_SIZE;
					total+=BUF_SIZE;
					pos=(int)(((float)(((float)total/(float)file_len))*100));
					bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
					if(bResult<0 || ack!=BUF_SIZE) {
						goto EXIT;
					}
					scnt--;
					printf("Write image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
					show_progressbar(pos);
				}
				if(rcnt>0) {
					bResult=NUC_WritePipe(0,(unsigned char*)pbuf,rcnt);
					if(bResult<0) goto EXIT;
					total+=rcnt;
					bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
					if(bResult<0 || ack!=rcnt) goto EXIT;
				}
				printf("Write image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
				show_progressbar(pos);
				burn_pos=0;
				while(burn_pos!=100) {
					bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
					if(bResult<0) goto EXIT;
					if(!((ack>>16)&0xffff)) {
						burn_pos=(UCHAR)(ack&0xffff);
					} else {
						goto EXIT;
					}
				}
				show_progressbar(100);
				printf("Write image%d %s ... Passed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
				if((nudata.run==RUN_PROGRAM_VERIFY)) {
					unsigned char temp[BUF_SIZE];

					if(NUC_OpenUsb()<0) return -1;
					NUC_SetType(0,SPI);
					m_fhead->flag=VERIFY_ACTION;
					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_NAND_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,sizeof(unsigned int));
					pbuf = lpBuffer+m_fhead->initSize;
					scnt=(file_len-m_fhead->initSize)/BUF_SIZE;
					rcnt=(file_len-m_fhead->initSize)%BUF_SIZE;
					total=0;
					while(scnt>0) {
						bResult=NUC_ReadPipe(0,(unsigned char*)temp, BUF_SIZE);
						if(bResult>=0) {
							total+=BUF_SIZE;
							if(DataCompare(temp,pbuf,BUF_SIZE))
								ack=BUF_SIZE;
							else
								ack=0;//compare error
							bResult=NUC_WritePipe(0,(unsigned char*)&ack,4);
							if(bResult<0) {
								printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
								goto EXIT;
							}
							pos=(int)(((float)(((float)total/(float)file_len))*100));
							printf("Verify image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
							show_progressbar(pos);
							pbuf+=BUF_SIZE;
						} else {
							printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
							goto EXIT;
						}
						scnt--;
					}

					if(rcnt>0) {
						bResult=NUC_ReadPipe(0,(UCHAR *)temp,BUF_SIZE);
						if(bResult>=0) {
							total+=rcnt;
							if(DataCompare(temp,pbuf,rcnt))
								ack=BUF_SIZE;
							else
								ack=0;//compare error
							bResult=NUC_WritePipe(0,(UCHAR *)&ack, 4);
							if((bResult<0)||(!ack)) {
								printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
								goto EXIT;
							}

						} else {
							printf("Verify image%d %s ... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
							goto EXIT;
						}

						pos=(int)(((float)(((float)total/(float)file_len))*100));
						printf("Verify image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
						show_progressbar(pos);
					}
					show_progressbar(100);
					printf("Verify image%d %s... Passed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
				} //Verify_tag end

				free(lpBuffer);
#endif
			}
			free(m_fhead);
		}


		if(nudata.run==RUN_ERASE) {  //Erase SPI
			int wait_pos=0;
			unsigned int erase_pos=0;
			m_fhead->flag=ERASE_ACTION;
			m_fhead->flashoffset = nudata.erase->start_blocks; //start erase block
			m_fhead->execaddr=nudata.erase->offset_blocks;  //erase block length

			/* Decide chip erase mode or erase mode                    */
			/* 0: chip erase, 1: erase accord start and length blocks. */
			if(nudata.erase->offset_blocks==0xFFFFFFFF) {
				m_fhead->type=0;
				m_fhead->no=0xffffffff;//erase all
			} else {
				m_fhead->type=1;
				m_fhead->no=0;
			}

			bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_NAND_HEAD));
			bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
			erase_pos=0;
			printf("Erase ... ");
			show_progressbar(erase_pos);
			while(erase_pos!=100) {
				bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
				if(bResult<0) goto EXIT;
				if(((ack>>16)&0xffff)) goto EXIT;
				erase_pos=ack&0xffff;
				printf("Erase ... ");
				show_progressbar(erase_pos);
				if(erase_pos==95) {
					wait_pos++;
					if(wait_pos>100) {
						goto EXIT;
					}
				}

			}
			show_progressbar(100);
			printf("Erase ... Passed\n");
		}
	}

	return 0;

EXIT:

	return -1;
}
int UXmodem_SD(void)
{
	int idx;
	FILE *fp;
	int bResult,pos;
	unsigned int scnt,rcnt,file_len,ack,total;
	unsigned char *pbuf,buf[BUF_SIZE];
	NORBOOT_MMC_HEAD *m_fhead;
	char DDR[256];
	unsigned char *ddrbuf=NULL;
	unsigned int ddrlen;
	char* lpBuffer;
	int blockNum;
	m_fhead=malloc(sizeof(NORBOOT_MMC_HEAD));
	m_fhead->ReserveSize = mmc_head.ReserveSize;
	m_fhead->PartitionNum = mmc_head.PartitionNum;
	m_fhead->Partition1Size = mmc_head.Partition1Size;
	m_fhead->Partition2Size = mmc_head.Partition2Size;
	m_fhead->Partition3Size = mmc_head.Partition3Size;
	m_fhead->Partition4Size = mmc_head.Partition4Size;

	if(m_fhead->PartitionNum==0 || m_fhead->PartitionNum > 4) {
		printf("PartitionNum , it should be 1 ~ 4\n");
		goto EXIT;
	}
	if(NUC_OpenUsb()<0) return -1;
	NUC_SetType(0,MMC);

	if(nudata.image[idx].image_type==PACK) {
		if(UXmodem_Pack()<0) goto EXIT;
	} else {
		if(nudata.run==RUN_FORMAT) {
			int wait_pos=0;
			unsigned int format_pos=0;
			m_fhead->flag=FORMAT_ACTION;
			if(m_fhead->PartitionNum==1) {
				m_fhead->Partition1Size = nudata.user_def->EMMC_uBlock/(2*1024) - \
				                          nudata.user_def->EMMC_uReserved;
			} else if(m_fhead->PartitionNum==2) {
				m_fhead->Partition2Size = nudata.user_def->EMMC_uBlock/(2*1024) - \
				                          nudata.user_def->EMMC_uReserved - \
				                          m_fhead->Partition1Size ;
			} else if(m_fhead->PartitionNum==3) {
				m_fhead->Partition3Size = nudata.user_def->EMMC_uBlock/(2*1024) - \
				                          nudata.user_def->EMMC_uReserved - \
				                          m_fhead->Partition1Size - \
				                          m_fhead->Partition2Size;
			} else {
				m_fhead->Partition4Size = nudata.user_def->EMMC_uBlock/(2*1024) - \
				                          nudata.user_def->EMMC_uReserved - \
				                          m_fhead->Partition1Size - \
				                          m_fhead->Partition2Size - \
				                          m_fhead->Partition3Size;
			}
			MSG_DEBUG("num=%d p1=%d,p2=%d,p3=%d\n",m_fhead->PartitionNum,m_fhead->Partition1Size,m_fhead->Partition2Size,m_fhead->Partition3Size);
			m_fhead->ReserveSize = m_fhead->ReserveSize * (2 * 1024);
			bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_MMC_HEAD));
			bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
			format_pos=0;
			printf("Format ... ");
			show_progressbar(format_pos);
			while(format_pos!=100) {
				bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
				if(bResult<0) goto EXIT;
				if(((ack>>16)&0xffff)) goto EXIT;
				format_pos=ack&0xffff;
				printf("Format ... ");
				show_progressbar(format_pos);
				if(format_pos==95) {
					wait_pos++;
					if(wait_pos>100) {
						goto EXIT;
					}
				}

			}
			show_progressbar(100);
			printf("Format ... Passed\n");
		}

		if(nudata.run==RUN_READ) {
			unsigned char temp[BUF_SIZE];
			FILE* tempfp;
			//-----------------------------------
			fp=fopen(nudata.read->path,"w+b");
			if(fp==NULL) {
				printf("Open write File Error(-w %s) \n",nudata.read->path);
				goto EXIT;
			}
			file_len = nudata.read->offset_blocks*(MMC512B);//*m_info.Nand_uPageSize*m_info.Nand_uPagePerBlock;
			m_fhead->flag=READ_ACTION;
			m_fhead->flashoffset=nudata.read->start_blocks*(MMC512B);
			m_fhead->filelen=file_len;
			m_fhead->initSize=0; //read good block
			bResult=NUC_WritePipe(0,(UCHAR *)m_fhead, sizeof(NORBOOT_MMC_HEAD));
			bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
			scnt=file_len/BUF_SIZE;
			rcnt=file_len%BUF_SIZE;
			total=0;
			while(scnt>0) {
				bResult=NUC_ReadPipe(0,(unsigned char*)temp, BUF_SIZE);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				total+=BUF_SIZE;
				pos=(int)(((float)(((float)total/(float)file_len))*100));
				printf("Read ... ");
				show_progressbar(pos);
				fwrite(temp,BUF_SIZE,1,fp);
				ack=BUF_SIZE;
				bResult=NUC_WritePipe(0,(UCHAR *)&ack,4);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				scnt--;
			}

			if(rcnt>0) {
				bResult=NUC_ReadPipe(0,(UCHAR *)temp,BUF_SIZE);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				total+=rcnt;
				fwrite(temp,rcnt,1,fp);
				ack=BUF_SIZE;
				bResult=NUC_WritePipe(0,(UCHAR *)&ack,4);
				if(bResult<0) {
					fclose(fp);
					goto EXIT;
				}
				pos=(int)(((float)(((float)total/(float)file_len))*100));
				printf("Read ... ");
				show_progressbar(pos);
			}
			fclose(fp);
			show_progressbar(100);
			printf("Read ... Passed\n");
		}

		if(nudata.run==RUN_PROGRAM || nudata.run==RUN_PROGRAM_VERIFY) { //Burn Image
			for(idx=0; idx<nudata.image_num; idx++) {
				if(idx>0) {
					int i;
					if(NUC_OpenUsb()<0) return -1;
					NUC_SetType(0,MMC);
					usleep(10);
				}
				fp=fopen(nudata.image[idx].image_path, "rb");
				if(fp==NULL) {
					printf("Open read File Error(-w %s) \n",nudata.image[idx].image_path);
					goto EXIT;
				}
				printf("image%d %s\n",idx,nudata.image[idx].image_path);
				fseek(fp,0,SEEK_END);
				file_len=ftell(fp);
				fseek(fp,0,SEEK_SET);

				if(!file_len) {
					fclose(fp);
					printf("File length is zero\n");
					goto EXIT;
				}
				memset((unsigned char *)m_fhead,0,sizeof(NORBOOT_MMC_HEAD));
				m_fhead->flag=WRITE_ACTION;
				m_fhead->initSize=0;
				m_fhead->filelen=file_len;
				switch(nudata.image[idx].image_type) {
				case DATA:
				case PACK:
					m_fhead->no=0;
					m_fhead->execaddr = 0x200;
					m_fhead->flashoffset = nudata.image[idx].image_start_offset;
					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*file_len); //read file to buffer
					memset(lpBuffer,0xff,file_len);
					((NORBOOT_MMC_HEAD *)m_fhead)->macaddr[7]=0;
					m_fhead->type=nudata.image[idx].image_type;

					bResult=NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_MMC_HEAD));
					bResult=NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));
					fread(lpBuffer,1,m_fhead->filelen,fp);
					break;
				case ENV:
					m_fhead->no=0;
					m_fhead->execaddr = 0x200;
					m_fhead->flashoffset = nudata.image[idx].image_start_offset;
					if(file_len>(0x10000-4)) {
						fclose(fp);
						printf("The environment file size is less then 64KB\n");
						goto EXIT;
					}
					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*0x10000); //read file to buffer
					memset(lpBuffer,0x00,0x10000);

					((NORBOOT_NAND_HEAD *)m_fhead)->macaddr[7]=0;

					m_fhead->filelen=0x10000;
					m_fhead->type=nudata.image[idx].image_type;
					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_MMC_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

					{
						char line[256];
						char* ptr=(char *)(lpBuffer+4);
						while (1) {
							if (fgets(line,256, fp) == NULL) break;
							if(line[strlen(line)-2]==0x0D && line[strlen(line)-1]==0x0A) {
								strncpy(ptr,line,strlen(line)-1);
								ptr[strlen(line)-2]=0x0;
								ptr+=(strlen(line)-1);
							}else if(line[strlen(line)-1]==0x0A) {
								strncpy(ptr,line,strlen(line));
								ptr[strlen(line)-1]=0x0;
								ptr+=(strlen(line));
							} else {
								strncpy(ptr,line,strlen(line));
								ptr+=(strlen(line));
							}
						}

					}
					*(unsigned int *)lpBuffer=CalculateCRC32((unsigned char *)(lpBuffer+4),0x10000-4);
					file_len=0x10000;
					break;
				case LOADER:
					m_fhead->no=1;
					m_fhead->execaddr = nudata.image[idx].image_exe_addr;
					m_fhead->flashoffset = 0x400;
					ddrbuf=GetDDRFormat(&ddrlen);
					file_len=file_len+ddrlen+32;
					m_fhead->initSize=ddrlen;
					lpBuffer = (unsigned char *)malloc(sizeof(unsigned char)*file_len);
					memset(lpBuffer,0xff,file_len);
					m_fhead->macaddr[7]=0;

					m_fhead->type=nudata.image[idx].image_type;

					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_MMC_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,(int)sizeof(unsigned int));

					memcpy(lpBuffer,ddrbuf,ddrlen);
					fread(lpBuffer+ddrlen,1,m_fhead->filelen,fp);
					break;
				}
				fclose(fp);
#if 1
				pbuf=lpBuffer;
				scnt=file_len/BUF_SIZE;
				rcnt=file_len%BUF_SIZE;
				total=0;
				while(scnt>0) {
					bResult=NUC_WritePipe(0,(unsigned char*)pbuf,BUF_SIZE);
					if(bResult<0)
						goto EXIT;
					pbuf+=BUF_SIZE;
					total+=BUF_SIZE;
					pos=(int)(((float)(((float)total/(float)file_len))*100));
					bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
					if(bResult<0 || ack!=BUF_SIZE) {
						goto EXIT;
					}
					scnt--;

					printf("Write image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
					show_progressbar(pos);
				}
				if(rcnt>0) {
					bResult=NUC_WritePipe(0,(unsigned char*)pbuf,rcnt);
					if(bResult<0) goto EXIT;
					total+=rcnt;
					bResult=NUC_ReadPipe(0,(UCHAR *)&ack,4);
					if(bResult<0) goto EXIT;
				}
				printf("Write image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
				show_progressbar(pos);
				bResult=NUC_ReadPipe(0,(UCHAR *)&blockNum,4);
				if(bResult<0) goto EXIT;
				show_progressbar(100);
				printf("Write image%d %s ... Passed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
				if((nudata.run==RUN_PROGRAM_VERIFY)) {
					unsigned char temp[BUF_SIZE];
					if(NUC_OpenUsb()<0) return -1;
					NUC_SetType(0,MMC);
					m_fhead->flag=VERIFY_ACTION;
					if(nudata.image[idx].image_type==LOADER)
						file_len = file_len - 32;
					NUC_WritePipe(0,(unsigned char*)m_fhead,sizeof(NORBOOT_MMC_HEAD));
					NUC_ReadPipe(0,(unsigned char *)&ack,sizeof(unsigned int));
					pbuf = lpBuffer+m_fhead->initSize;
					scnt=(file_len-m_fhead->initSize)/BUF_SIZE;
					rcnt=(file_len-m_fhead->initSize)%BUF_SIZE;
					total=0;
					while(scnt>0) {
						bResult=NUC_ReadPipe(0,(unsigned char*)temp, BUF_SIZE);
						if(bResult>=0) {
							total+=BUF_SIZE;
							if(DataCompare(temp,pbuf,BUF_SIZE))
								ack=BUF_SIZE;
							else
								ack=0;//compare error
							bResult=NUC_WritePipe(0,(unsigned char*)&ack,4);
							if(bResult<0) {
								printf("Verify image%d %s... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
								goto EXIT;
							}
							pos=(int)(((float)(((float)total/(float)file_len))*100));
							printf("Verify image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
							show_progressbar(pos);
							pbuf+=BUF_SIZE;
						} else {
							printf("Verify image%d %s... Failed,Result %d\n",idx,TypeT[nudata.image[idx].image_idx].pName,bResult);
							printf("2 scnt=%d\n",scnt);
							goto EXIT;
						}
						scnt--;
					}

					if(rcnt>0) {
						bResult=NUC_ReadPipe(0,(UCHAR *)temp,BUF_SIZE);
						if(bResult>=0) {
							total+=rcnt;
							if(DataCompare(temp,pbuf,rcnt))
								ack=BUF_SIZE;
							else
								ack=0;//compare error
							bResult=NUC_WritePipe(0,(UCHAR *)&ack, 4);
							if((bResult<0)||(!ack)) {
								printf("Verify image%d %s... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
								goto EXIT;
							}

						} else {
							printf("Verify image%d %s... Failed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
							goto EXIT;
						}

						pos=(int)(((float)(((float)total/(float)file_len))*100));
						printf("Verify image%d %s ... ",idx,TypeT[nudata.image[idx].image_idx].pName);
						show_progressbar(pos);
					}
					show_progressbar(100);
					printf("Verify image%d %s... Passed\n",idx,TypeT[nudata.image[idx].image_idx].pName);
				} //Verify_tag end
#endif

				free(lpBuffer);
			}
		}
	}
	free(m_fhead);
	return 0;

EXIT:

	printf("\neMMC failed\n");
	return -1;
}
