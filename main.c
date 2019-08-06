
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>  /* getopt */
#include <ctype.h>  /* isprint */
#include <dirent.h>
#include "minini/minIni.h"
#include "common.h"

MODE_T ModeT[]= {
	{MODE_SDRAM,		"SDRAM"		},
	{MODE_NAND,		"NAND"		},
	{MODE_SPINOR,		"SPINOR"	},
	{MODE_SPINAND,	"SPINAND"	},
	{MODE_SD,		"SD"		},
	{0,		NULL		},
};


MODE_T TypeT[]= {
	{DATA,		"DATA"		},
	{ENV,		"ENV"		},
	{LOADER,		"LOADER"	},
	{PACK,	"PACK"		},
	{0,		NULL		},
};

MODE_T RunT[]= {
	{RUN_PROGRAM,		"PROGRAM"		},
	{RUN_PROGRAM_VERIFY,		"PROGRAM_VERIFY"		},
	{RUN_READ,		"READ"	},
	{RUN_ERASE,	"ERASE"		},
	{RUN_FORMAT,	"FORMAT"	},
	{0,		NULL		},
};

char inifile[256];

int check_strlen(const struct dirent *dir)
{
	if(strlen(dir->d_name)>5)
		return 1;
	else
		return 0;
}

/* Using globel varible, nudata */
int ParsingIni(void)
{
	MODE_T * mode;
	char str[256],str2[100];
	long n,IsYes;
	int i,idx;

	nudata.user_def = (INFO_T *)malloc(sizeof(INFO_T));
	/* Parsing [RUN] */
	n = ini_gets("RUN", "pack", "dummy", str, sizearray(str), inifile);
	if(n==3) {
		pack.enable_pack=1;
		n = ini_gets("RUN", "pack_path", "dummy", str, sizearray(str), inifile);
		strcpy(pack.pack_path,str);
	}

	n = ini_gets("RUN", "all_device", "dummy", str, sizearray(str), inifile);
	if(n==3) enable_all_device=1;

	n = ini_gets("RUN", "mode", "dummy", str, sizearray(str), inifile);
	mode = (MODE_T *)ModeT;
	while(mode->pName!=NULL && n>1) {
		if(!strcasecmp(str,mode->pName))
			break;
		else
			mode++;
	}
	if(mode->pName==NULL || n==0) {
		fprintf(stderr,"Check [RUN] mode setting\n");
		return -1;
	} else {
		nudata.mode.id=mode->id;
		nudata.mode.pName =mode->pName;
		MSG_DEBUG("id=%d,mode=%s\n",nudata.mode.id,nudata.mode.pName);
	}
	n = ini_gets("DDR", "ddr", "dummy", str, sizearray(str), inifile);
	sprintf(nudata.ddr_path,"%s/sys_cfg/%s",NUDATA_PATH,str);
	MSG_DEBUG("DDR ini = %s\n",nudata.ddr_path);

	if(nudata.mode.id==SDRAM) {	/* Parsing [SDRAM] */
		nudata.sdram = (SDRAM_T *)malloc(sizeof(SDRAM_T));
		n = ini_gets(ModeT[nudata.mode.id].pName, "sdram_path", "dummy", str, sizearray(str), inifile);
		strcpy(nudata.sdram->sdram_path,str);

		n = ini_gets(ModeT[nudata.mode.id].pName, "option", "dummy", str, sizearray(str), inifile);
		if(n==3)
			nudata.sdram->opetion=DOWNLOAD_RUN;
		else
			nudata.sdram->opetion=DOWNLOAD;
		n = ini_getl(ModeT[nudata.mode.id].pName, "exe_addr", -1, inifile);
		nudata.sdram->exe_addr = n;
		n = ini_gets(ModeT[nudata.mode.id].pName, "using_dtb", "dummy", str, sizearray(str), inifile);
		if(n==2) {
			nudata.sdram->dtb_addr = 0;
		} else {
			n = ini_gets(ModeT[nudata.mode.id].pName, "dtb_path", "dummy", str, sizearray(str), inifile);
			strcpy(nudata.sdram->dtb_path,str);
			n = ini_getl(ModeT[nudata.mode.id].pName, "dtb_addr", -1, inifile);
			nudata.sdram->dtb_addr  = n;
		}
#if 0
		fprintf(stderr,"nudata.sdram->exe_addr=0x%08x\n",nudata.sdram->exe_addr);
		fprintf(stderr,"nudata.sdram->dtb_addr=0x%08x\n",nudata.sdram->dtb_addr);
#endif
	} else { 	/* Parsing [SPINAND],[SPINOR],[SD],[NAND] */

		/* Parsing run */
		n = ini_gets(ModeT[nudata.mode.id].pName, "run", "dummy", str, sizearray(str), inifile);
		mode = (MODE_T *)RunT;
		while(mode->pName!=NULL && n>1) {
			if(!strcasecmp(str,mode->pName))
				break;
			else
				mode++;
		}
		nudata.run = mode->id;
		n = ini_getl(ModeT[nudata.mode.id].pName, "image_num", -1, inifile);
		nudata.image_num = n;
		if(nudata.image_num==0) {
			fprintf(stderr,"Set image number\n");
			return -1;
		}
		for(i=0; i<nudata.image_num; i++) {
			sprintf(str2,"image%d_type",i);
			n = ini_gets(ModeT[nudata.mode.id].pName, str2, "dummy", str, sizearray(str), inifile);
			mode = (MODE_T *)TypeT;
			idx = 0;
			while(mode->pName!=NULL && n>1) {
				if(!strcasecmp(str,mode->pName)) {
					break;
				} else {
					mode++;
					idx++;
				}
			}
			if(mode->pName==NULL) {
				fprintf(stderr,"Cannot find image type\n");
				printf("============================>Cannot find image type\n");
				return -1;
			}

			nudata.image[i].image_idx=idx;
			nudata.image[i].image_type = mode->id;
			sprintf(str2,"image%d_path",i);
			n = ini_gets(ModeT[nudata.mode.id].pName, str2, "dummy", str, sizearray(str), inifile);
			if(n==0) {
				fprintf(stderr,"Cannot find image%d\n",i);
				return -1;
			}
			if( access( str, F_OK ) == -1 ) {
				fprintf(stderr,"Cannot access image%d [path:%s]\n",i,str);
				return -1;
			}
			strcpy(nudata.image[i].image_path,str);

			sprintf(str2,"image%d_exe_addr",i);
			n = ini_getl(ModeT[nudata.mode.id].pName, str2, -1, inifile);
			nudata.image[i].image_exe_addr = n;

			sprintf(str2,"image%d_start_offset",i);
			n = ini_getl(ModeT[nudata.mode.id].pName, str2, -1, inifile);
			nudata.image[i].image_start_offset = n;
		}

		MSG_DEBUG("nudata.image_num=%d\n",nudata.image_num);
		for(i=0; i<nudata.image_num; i++) {
			MSG_DEBUG("image%d_type=%d\n",i,nudata.image[i].image_type);
			MSG_DEBUG("image%d_path=%s\n",i,nudata.image[i].image_path);
			MSG_DEBUG("image%d_exe_addr=0x%08x\n",i,nudata.image[i].image_exe_addr);
			MSG_DEBUG("image%d_start_offset=0x%08x\n",i,nudata.image[i].image_start_offset);
		}

		/* Parsing User defined */
		IsYes = ini_gets(ModeT[nudata.mode.id].pName, "using_user_defined", "dummy", str, sizearray(str), inifile);

		nudata.user_def->EMMC_uReserved = 1000 ;
		nudata.user_def->Nand_uBlockPerFlash = 1024;
		nudata.user_def->Nand_uPagePerBlock =	64;


		if(nudata.mode.id==MODE_SD) {

			n = ini_getl(ModeT[nudata.mode.id].pName, "reserved_size", -1, inifile);
			nudata.user_def->EMMC_uReserved = n ;

			n = ini_gets(ModeT[nudata.mode.id].pName, "using_format", "dummy", str, sizearray(str), inifile);
			if(n==2) {
				mmc_head.PartitionNum = 0 ;
			} else {
				n = ini_getl(ModeT[nudata.mode.id].pName, "partition_num", -1, inifile);
				mmc_head.PartitionNum = n ;
			}

			n = ini_getl(ModeT[nudata.mode.id].pName, "partition1_size", -1, inifile);
			mmc_head.Partition1Size = n ;

			n = ini_getl(ModeT[nudata.mode.id].pName, "partition2_size", -1, inifile);
			mmc_head.Partition2Size = n ;

			n = ini_getl(ModeT[nudata.mode.id].pName, "partition3_size", -1, inifile);
			mmc_head.Partition3Size = n ;

			mmc_head.Partition4Size = 0 ;

		} else if(nudata.mode.id==MODE_NAND) {

			n = ini_getl(ModeT[nudata.mode.id].pName, "blockperflash", -1, inifile);
			nudata.user_def->Nand_uBlockPerFlash = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "pageperblock", -1, inifile);
			nudata.user_def->Nand_uPagePerBlock = n;

			if(IsYes == 3)
				nudata.user_def->Nand_uIsUserConfig =1;
			else
				nudata.user_def->Nand_uIsUserConfig =0;

		} else if(nudata.mode.id==MODE_SPINAND) {

			n = ini_getl(ModeT[nudata.mode.id].pName, "page_size", -1, inifile);
			nudata.user_def->SPINand_PageSize = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "spare_area", -1, inifile);
			nudata.user_def->SPINand_SpareArea = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "quad_read_command", -1, inifile);
			nudata.user_def->SPINand_QuadReadCmd = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "read_status_command", -1, inifile);
			nudata.user_def->SPINand_ReadStatusCmd = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "write_status_command", -1, inifile);
			nudata.user_def->SPINand_WriteStatusCmd = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "status_value", -1, inifile);
			nudata.user_def->SPINand_StatusValue = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "dummy_bytes", -1, inifile);
			nudata.user_def->SPINand_dummybyte = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "blockperflash", -1, inifile);
			nudata.user_def->SPINand_BlockPerFlash = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "pageperflash", -1, inifile);
			nudata.user_def->SPINand_PagePerBlock = n;

			if(IsYes == 3)
				nudata.user_def->SPINand_uIsUserConfig =1;
			else
				nudata.user_def->SPINand_uIsUserConfig =0;

		} else if(nudata.mode.id==MODE_SPINOR) {

			n = ini_getl(ModeT[nudata.mode.id].pName, "quad_read_command", -1, inifile);
			nudata.user_def->SPI_QuadReadCmd = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "read_status_command", -1, inifile);
			nudata.user_def->SPI_ReadStatusCmd = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "write_status_command", -1, inifile);
			nudata.user_def->SPI_WriteStatusCmd = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "status_value", -1, inifile);
			nudata.user_def->SPI_StatusValue = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "dummy_bytes", -1, inifile);
			nudata.user_def->SPI_dummybyte = n;

			if(IsYes == 3)
				nudata.user_def->SPI_uIsUserConfig =1;
			else
				nudata.user_def->SPI_uIsUserConfig =0;
		}

		if(nudata.run==RUN_READ) {
			nudata.read = (READ_T *)malloc(sizeof(READ_T));
			n = ini_gets(ModeT[nudata.mode.id].pName, "read_path", "dummy", str, sizearray(str), inifile);
			strcpy(nudata.read->path,str);

			n = ini_getl(ModeT[nudata.mode.id].pName, "read_start_blocks", -1, inifile);
			nudata.read->start_blocks = n;

			n = ini_getl(ModeT[nudata.mode.id].pName, "read_offset_blocks", -1, inifile);
			nudata.read->offset_blocks = n;
		}

		if(nudata.run==RUN_ERASE) {
			nudata.erase = (ERASE_T *)malloc(sizeof(ERASE_T));
			n = ini_gets(ModeT[nudata.mode.id].pName, "erase_all", "dummy", str, sizearray(str), inifile);
			if(n==2) {
				n = ini_getl(ModeT[nudata.mode.id].pName, "erase_start_blocks", -1, inifile);
				nudata.erase->start_blocks = n;
				n = ini_getl(ModeT[nudata.mode.id].pName, "erase_offset_blocks", -1, inifile);
				nudata.erase->offset_blocks = n;
			} else
				nudata.erase->offset_blocks = 0xffffffff;
		}
	}


	MSG_DEBUG("ParsingIni END\n");
	return 0;
}

int main(int argc, char **argv)
{
	/* Initial */
	char *path;
	int cmd_opt = 0;

	//memcpy(Data_Path,argv[0],strlen(argv[0]));
	//path=strrchr(Data_Path,'/');
	//fprintf(stderr,"path=%s\n",path);
	//printf("syscfg_dir=%s\n",syscfg_dir);
	csg_usb_index = 1;
	enable_all_device = 0;
	if(path==NULL) {
#ifndef _WIN32
		//Data_Path[0]='.';
		//Data_Path[1]='\0';
#else
		getcwd(Data_Path, 256);
#endif
	} else {
		*path='\0';
	}
#ifndef _WIN32
	//sprintf(Data_Path,"%s",NUDATA_PATH);
	//fprintf(stderr,"Data_Path=%s\n",Data_Path);
#else
	sprintf(Data_Path,"%s%s",Data_Path,"/nudata");
#endif

	while(1) {
		//fprintf(stderr, "proces index:%d\n", optind);
		cmd_opt = getopt(argc, argv, "h");

		/* End condition always first */
		if (cmd_opt == -1) {
			break;
		}

		/* Lets parse */
		switch (cmd_opt) {
		/* No args */
		case 'h':
			fprintf(stderr, "============================================\n");
			fprintf(stderr, "==   Nuvoton NuWriter Command Tool V%s   ==\n",PACKAGE_VERSION);
			fprintf(stderr, "============================================\n");
			fprintf(stderr, "NuWriter [File]\n\n");
			fprintf(stderr, "[File]      Set ini file\n");
			return 0;

		/* Error handle: Mainly missing arg or illegal option */
		case '?':
			fprintf(stderr, "Illegal option:-%c\n", isprint(optopt)?optopt:'#');
			break;
		default:
			fprintf(stderr, "Not supported option\n");
			fprintf(stderr, "Try 'nuwriter -h' for more information.\n");
			break;
		}
	}

	if(argc>1)
		MSG_DEBUG("argv[1]=%s\n",argv[1]);

	if( access( argv[1], F_OK ) == -1 ) {
		fprintf(stderr,"Cannot access %s ini file\n",argv[1]);
		return 0;
	}
	strcpy(inifile,argv[1]);
	if(ParsingIni()<0) {
		return -1;
	}

	if(pack.enable_pack==1) {
		MSG_DEBUG("UXmodem_PackImage\n");
		return UXmodem_PackImage();
	} else {
		libusb_init(NULL);
		if((dev_count=get_device_num_with_vid_pid(ctx,USB_VENDOR_ID, USB_PRODUCT_ID))==0) {
			printf("Device not found\n");
			libusb_exit(NULL);
			return -1;
		}
		if(enable_all_device==1) {
			printf("Number of devices: %d\n",dev_count);
		}
		do {
			if(enable_all_device==1) {
				printf("Burn device %d ...\n",csg_usb_index);
			}
			if(ParseFlashType()< 0) {
				printf("Failed\n");
				NUC_CloseUsb();
				libusb_exit(NULL);
				return -1;
			}
			NUC_CloseUsb();

			csg_usb_index++;
		} while(csg_usb_index<=dev_count && enable_all_device==1);
		libusb_exit(NULL);
	}
	return 0;
}
