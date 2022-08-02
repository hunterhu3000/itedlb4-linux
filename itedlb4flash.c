/*-----------------------------------------------------------------------------------
 * Filename: itedlb4flash.c         For Chipset: ITE EC
 *
 * Function: ITE EC Flash Utility for BLB4
 *
 * Author  : Donald Huang <donald.huang@ite.com.tw> 
 * 
 * Copyright (c) 2021 - , ITE Tech. Inc. All Rights Reserved. 
 *
 * You may not present,reproduce,distribute,publish,display,modify,adapt,
 * perform,transmit,broadcast,recite,release,license or otherwise exploit
 * any part of this publication in any form,by any means,without the prior
 * written permission of ITE Tech. Inc.
 *
 * 2021.12.20 V1.0.1 <Donald Huang> First Release to Flash IT81302
 * 2022.02.24 V1.0.3 <Donald Huang> 1.Add parameter -f filename
 *                                                  -s skip check stage
 *                                  2.Add reset_ec()
 * 2022.04.12 V1.0.4 <Donald Huang> Reduce flash size to blk size                                 
 * 2022.04.25 V1.0.5 <Donald Huang> 1.Add parameter -u for SPI Flash Interface                             
 * 2022.06.24 V1.0.6 <Donald Huang> 1.Enable QE Bit After Flash to avoid write status clear                             
 *---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>
#include <unistd.h>

#include "libusb.h"
#include "itedlb4flash.h"

#include <time.h>

#define VERSION "1.0.6"

static int perr(char const *format, ...)
{
        va_list args;
        int r;

        va_start (args, format);
        r = vfprintf(stderr, format, args);
        va_end(args);

        return r;
}

void hexdump(unsigned char *buffer,int len)
{
        int i;
        for(i=0 ; i<len;i++)     {
                if((i%16==0)) {
                        printf(" %06X :",i);
                }
                printf(" %02x",buffer[i]);
                if((i%16==7)) {
                        printf(" - ");
                }
                if(i%16 == 15) {
                        printf("\n\r");
                }
        }

}

static int read_from_itedev(uint8_t *CMD,unsigned int ReadDataBytes, unsigned char* ReadData)
{
        static uint32_t tag = 1;
        uint8_t cdb_len;
        int i, r, size;
        DLB4_CBW CBW;
        DLB4_CSW CSW;
        uint8_t szBuffer[32];
        uint32_t bytesWritten,bytesRead;
        bool bResult;

	do {
        	CBW.dSignature=DLB4_CBW_Signature;
        	CBW.dTag=rand();
        	CBW.dDataLength=ReadDataBytes;
        	CBW.bmFlags=0x80;
        	CBW.bCBLength=DLB4_CBW_CBLength;
        	memcpy(CBW.CB,CMD,CBW.bCBLength);
        	memcpy(szBuffer,&CBW,sizeof(CBW));
		do {
        		bResult=libusb_bulk_transfer(devinfo.handle, devinfo.endpoint_out, (unsigned char*)&szBuffer, sizeof(CBW), &bytesWritten, 1000);
                        if(bResult!=LIBUSB_SUCCESS) {
                                ITE_DBG("bResult=%d\n",bResult);
                                return -1;
                        }


        		if (bResult == LIBUSB_ERROR_PIPE) {
                  		libusb_clear_halt(devinfo.handle, devinfo.endpoint_out);
        		}
			i++;
		} while ((bResult == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));

        	if(bytesWritten!=sizeof(CBW)) return -1;

        	if(ReadDataBytes>0) {
	  		bytesRead = 0;
			do {
          			bResult=libusb_bulk_transfer(devinfo.handle, devinfo.endpoint_in, ReadData, ReadDataBytes, &bytesRead, 5000);
	  
                        	if(bResult!=LIBUSB_SUCCESS) {
                                	ITE_DBG("\n\rbResult=%d",bResult);
                                	return -1;
                        	}
          			if (bResult == LIBUSB_ERROR_PIPE) {
                  			libusb_clear_halt(devinfo.handle, devinfo.endpoint_out);
          			}
				i++;
			} while ((bResult == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));
        	}

		do {
        		bResult=libusb_bulk_transfer(devinfo.handle, devinfo.endpoint_in, (unsigned char*)&szBuffer, sizeof(CSW), &bytesRead, 1000);

        		if (bResult == LIBUSB_ERROR_PIPE) {
                  		libusb_clear_halt(devinfo.handle, devinfo.endpoint_out);
        		}
			i++;
		} while ((bResult == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));
        	memcpy(&CSW,szBuffer,sizeof(CSW));

		if(CSW.dSignature!=DLB4_CSW_Signature) {
			printf("\n\r**Error Signature** (%08x)\n\r",CSW.dSignature);
		}	

    	} while ((bResult == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));


        return bResult;

}


static int write_to_itedev(uint8_t *CMD, unsigned int WriteDataBytes, unsigned char* WriteData)
{
        static uint32_t tag = 1;
        uint8_t cdb_len;
        int i, r, size;
        DLB4_CBW CBW;
        DLB4_CSW CSW;
        uint8_t szBuffer[32];
        uint32_t bytesWritten,bytesRead;
        bool bResult;

	do {	
        	CBW.dSignature=DLB4_CBW_Signature;
        	CBW.dTag=rand();
        	CBW.dDataLength=WriteDataBytes;
        	CBW.bmFlags=0x00;
        	CBW.bCBLength=DLB4_CBW_CBLength;
        	memcpy(CBW.CB,CMD,CBW.bCBLength);
        	memcpy(szBuffer,&CBW,sizeof(CBW));
		do {
        		bResult=libusb_bulk_transfer(devinfo.handle, devinfo.endpoint_out, (unsigned char*)&szBuffer, sizeof(CBW), &bytesWritten, 1000);
			if(bResult!=LIBUSB_SUCCESS) {
				ITE_DBG("bResult=%d\n",bResult);
				return -1;
			}	
        		if (bResult == LIBUSB_ERROR_PIPE) {
                  		libusb_clear_halt(devinfo.handle, devinfo.endpoint_out);
        		}
			i++;
		} while ((bResult == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));

        	if(bytesWritten!=sizeof(CBW)) return -1;

        	if(WriteDataBytes>0) {
			do {
          			bResult=libusb_bulk_transfer(devinfo.handle, devinfo.endpoint_out, WriteData, WriteDataBytes, &bytesWritten, 5000);
				if(bResult!=LIBUSB_SUCCESS) {
					ITE_DBG("\n\rbResult=%d",bResult);
					return -1;
				}	
          			if (bResult == LIBUSB_ERROR_PIPE) {
                  			libusb_clear_halt(devinfo.handle, devinfo.endpoint_out);
          			}
				i++;
			} while ((bResult == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));

        	}

		do {
        		bResult=libusb_bulk_transfer(devinfo.handle, devinfo.endpoint_in, (unsigned char*)&szBuffer, sizeof(CSW), &bytesRead, 1000);
			if(bResult!=LIBUSB_SUCCESS) {
				ITE_DBG("\n\rbResult=%d",bResult);
				return -1;
			}	
        		if (bResult == LIBUSB_ERROR_PIPE) {
                  		libusb_clear_halt(devinfo.handle, devinfo.endpoint_out);
        		}
			i++;
		} while ((bResult == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));

        	memcpy(&CSW,szBuffer,sizeof(CSW));
		if(CSW.dSignature!=DLB4_CSW_Signature) {
			printf("\n\r**Error Signature** (%08x)\n\r",CSW.dSignature);
			hexdump(szBuffer,32);
			return -1;
		}	
    	} while ((bResult == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));

        return bResult;
}

int DoCMD(DLB4_OP *cmd)
{
	int status=0;
	unsigned char cmdbuf[DLB4_CBW_CBLength];

	cmdbuf[0]=cmd->op_code;
	cmdbuf[1]=cmd->fun_code;
	cmdbuf[2]=cmd->p1;
	cmdbuf[3]=cmd->p2;
	cmdbuf[4]=cmd->p3;
	cmdbuf[5]=cmd->p4;
	cmdbuf[6]=cmd->p5;
	cmdbuf[7]=cmd->p6;
	cmdbuf[8]=cmd->p7;

	if(cmd->direction==ITE_DIR_IN) {
		status = read_from_itedev(cmdbuf, cmd->size,cmd->buffer);
	}	

        if(cmd->direction==ITE_DIR_OUT) {
                status = write_to_itedev(cmdbuf, cmd->size,cmd->buffer);
        }

	return status;
}	

// pin 	    : 1 => C1
//            2 => C2
//            3 => H1
//            4 => H2
// pin_data : 0 => ALT
//            1 => OUTPUT
//            2 => HIGH
//            3 => LOW
int Dlb4SetGPIO(uint8_t pin,uint8_t pin_data)
{
        unsigned char data[2];
        bool bResult;

	data[0]=pin;
	data[1]=pin_data;

        cmdParam.op_code=ITE_FW_CTL;
        cmdParam.fun_code=0x03;
        cmdParam.direction=ITE_DIR_OUT;
        cmdParam.buffer=data;
        cmdParam.size=4;

        bResult = DoCMD(&cmdParam);
        return bResult;
}


int GetDlb4FwVer(uint8_t *fwver)
{
        unsigned char data[4];
	bool bResult;

        cmdParam.op_code=ITE_FW_CTL;
        cmdParam.fun_code=ITE_FW_CTL_READ_FW_VER;
        cmdParam.direction=ITE_DIR_IN;
        cmdParam.buffer=data;
        cmdParam.size=4;

        bResult = DoCMD(&cmdParam);
	memcpy(fwver,data,4);
        return bResult;
}


//int SetPinDef(uint8_t *PinDef)
int SetPinDef()
{
        unsigned char data[16]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
				0x11,0x12,0x13,0x14,0x15,0x16,0x09};
        bool bResult;



        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_SetPinDef;
        cmdParam.direction=ITE_DIR_OUT;
        cmdParam.buffer=data;
        cmdParam.size=15;

        bResult = DoCMD(&cmdParam);
        return bResult;


}	

int GetChipID(uint8_t *chipid)
{
	unsigned char data[3];
	bool bResult;

	cmdParam.op_code=ITE_OP_CODE;
	cmdParam.fun_code=ITE_FUN_CODE_CHIPID_READ;
	cmdParam.direction=ITE_DIR_IN;
	cmdParam.buffer=data;
	cmdParam.size=3;

	bResult = DoCMD(&cmdParam);
	memcpy(chipid,data,3);
	return bResult;
}	

int GetFlashID(uint8_t *flashid,uint8_t mode)
{
        unsigned char data[5];
        bool bResult;

        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_FLASHID;
        cmdParam.direction=ITE_DIR_IN;
        cmdParam.buffer=data;
        cmdParam.size=5;
        cmdParam.p1=mode;

        bResult = DoCMD(&cmdParam);
        memcpy(flashid,data,5);
        return bResult;
}


int StartD2ec(uint8_t mode)
{
        unsigned char data[1];
	bool bResult;

        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_START_D2EC;
        cmdParam.direction=ITE_DIR_IN;
        cmdParam.buffer=data;
        cmdParam.size=1;
	cmdParam.p1=mode;

        bResult = DoCMD(&cmdParam);
        return bResult;
}

int RunCtrl(uint8_t p1,uint8_t p2,uint8_t p3)
{
        unsigned char data[1];
        bool bResult;

	data[0]=0x24;//dummy test
        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_RUN_CTRL;
        cmdParam.direction=ITE_DIR_OUT;
        cmdParam.buffer=data;
        cmdParam.size=1;
        cmdParam.p1=p1;
        cmdParam.p2=p2;
        cmdParam.p3=p3;

        bResult = DoCMD(&cmdParam);
        return bResult;
}

int RwDbgrCmdSet(uint8_t rw,uint8_t cmd,uint8_t *value)
{
        unsigned char data[1];
        bool bResult;

        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_DBGR_CMD_SET;
        cmdParam.direction=ITE_DIR_OUT;
        cmdParam.buffer=data;
        cmdParam.size=1;
        cmdParam.p1=rw;
        cmdParam.p2=cmd;
        cmdParam.p3=*value;

        bResult = DoCMD(&cmdParam);
	if(rw==0)
		*value=data[0];

        return bResult;
}



int WriteReg(uint8_t high,uint8_t low ,uint8_t data)
{
        unsigned char local[1];
	bool bResult;

	local[0]=data;

        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_WRITE_REG;
        cmdParam.direction=ITE_DIR_OUT;
        cmdParam.buffer=local;
        cmdParam.size=1;
	cmdParam.p1=high;
	cmdParam.p2=low;
	cmdParam.p7=0xf0;

        bResult = DoCMD(&cmdParam);
        return 0;
}

int ReadReg(uint8_t high,uint8_t low ,uint8_t *data)
{
        unsigned char local[1];
	bool bResult;

        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_READ_REG;
        cmdParam.direction=ITE_DIR_IN;
        cmdParam.buffer=local;
        cmdParam.size=1;
        cmdParam.p1=high;
        cmdParam.p2=low;
        cmdParam.p7=0xf0;

        bResult = DoCMD(&cmdParam);
	memcpy(data,local,1);

        return bResult;
}

int WriteNonSSTFlashStatus(uint8_t byte_count,uint8_t s1,uint8_t s2)
{

        unsigned char local[byte_count];
        bool bResult;

        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_FLASH_R_SPI_STATUS;
        cmdParam.direction=ITE_DIR_IN;
        cmdParam.buffer=local;
        cmdParam.size=byte_count;
        cmdParam.p1=byte_count;
        cmdParam.p2=s1;
        cmdParam.p3=s2;

        bResult = DoCMD(&cmdParam);
	printf("\n\rProtect Status: mode=%x data=%x %x\n\r",byte_count,local[0],local[1]);

        return bResult;

}

int eraseflash(int block_num,uint8_t sector_num,uint8_t erase_mode,uint8_t erase_type)
{
        unsigned char local[1];
        bool bResult;

        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_ERASE;
        cmdParam.direction=ITE_DIR_IN;
        cmdParam.buffer=local;
        cmdParam.size=1;
        cmdParam.p1=erase_mode;
        cmdParam.p2=erase_type;
        cmdParam.p3=(block_num%256);
        cmdParam.p4=sector_num;
        cmdParam.p5=(block_num/256);
        cmdParam.p6=0;//switch rom

        bResult = DoCMD(&cmdParam);

        return bResult;


}



int readflash(int block_num,uint8_t command_mode,uint8_t *data)
{
        bool bResult;

        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_READ;
        cmdParam.direction=ITE_DIR_IN;
        cmdParam.buffer=data;
        cmdParam.size=65536;
        cmdParam.p1=block_num%256;//BA
        cmdParam.p2=command_mode;//read mode
        cmdParam.p3=0;//dummy
        cmdParam.p4=0;//dummy
        cmdParam.p5=(block_num/256);//EBA
        cmdParam.p6=0;//switch rom

        bResult = DoCMD(&cmdParam);

        return bResult;


}	

int writeflash(int block_num,uint8_t command_mode,uint8_t program_type,uint8_t *data,int len)
{
        bool bResult;

        cmdParam.op_code=ITE_OP_CODE;
        cmdParam.fun_code=ITE_FUN_CODE_WRITE;
        cmdParam.direction=ITE_DIR_OUT;
        cmdParam.buffer=data;
        cmdParam.size=len;
        cmdParam.p1=command_mode;
        cmdParam.p2=block_num%256;
        cmdParam.p3=program_type;
        cmdParam.p5=(block_num/256);//EBA
        cmdParam.p6=0;//switch rom

        bResult = DoCMD(&cmdParam);

        return bResult;


}

int eraseall()
{

	int i,j,r;
	if((g_flag&ITE_USE_SPI)) {
		//chip erase
		// copy ini file set sector num as 4 
		
		eraseflash(g_blk_no,4,Flash.erase_mode,Flash.erase_type);
		printf("\rEraseing...      : 100%%\n\r");
                fflush(stdout);
		
	} else {
		//sector erase
		for(i=0;i<g_blk_no;i++) {
			for(j=0;j<0x10;j++) {
				r=eraseflash(i,((j<<4)+0xF),Flash.erase_mode,Flash.erase_type);
				if(r<0) return -1;

			}
			printf("\rEraseing...      : %d%%",(i+1)*100/(g_blk_no));
                	fflush(stdout);
		}
		printf("\n\r");
	}	
	return 0;

}	

int programall()
{
        int i,r;

	for(i=0;i<g_blk_no;i++) {

		r=writeflash(i,Flash.write_mode,Flash.write_type,(unsigned char *)(g_writebuf+(i*65536)),65536);
                printf("\rProgramng...     : %d%%",(i+1)*100/(g_blk_no));
                fflush(stdout);
		if(r<0) return -1;

	}
	printf("\n\r");
	return 0;

}

int checkall()
{
	int i,j,r;
        for(i=0;i<g_blk_no;i++) {
        	readflash(i,Flash.read_mode,(unsigned char *)(g_readbuf+i*65536));
		if(r<0) return -1;
        	for(j=0;j<65536;j++) {
			int l=i*65536+j;
                	if(g_readbuf[l]!=0xFF) {
                        	printf("\n\rCheck ERR on offset [%x]=%x",l,g_readbuf[l]);
                        	return 1;

                	} 
		}
		printf("\rChecking...      : %d%%",(i+1)*100/(g_blk_no));	
		fflush(stdout);
        }

	printf("\n\r");
	return 0;
}


int verifyall()
{
	int i,j,r;

        for(i=0;i<g_blk_no;i++) {
                readflash(i,Flash.read_mode,(unsigned char *)(g_readbuf+i*65536));
		if(r<0) return -1;
                for(j=0;j<65536;j++) {
			int l=i*65536+j;
                        if(g_readbuf[l]!=g_writebuf[l]) {
                        	printf("\n\rCheck ERR on offset r[%x]=%x w[%x]=%x",l,g_readbuf[l],l,g_writebuf[l]);
                                return 1;
         
                	}
        	}
                printf("\rVerifying...     : %d%%",(i+1)*100/(g_blk_no));
                fflush(stdout);
		
	}

	printf("\n\r");

	return 0;
}	

int init_dlb4_spi()
{
	int r;
	int i=0;

	ITE_OP_CODE = ITE_OP_CODE_DBGR_X;

	CALL_CHECK(StartD2ec(0x0b));
	CALL_CHECK(GetDlb4FwVer(g_fw_ver));
	enter_spi();
	CALL_CHECK(GetFlashID(g_flash_id,4));

	return 0;
}	

int init_dlb4()
{
	bool bResult;
        uint8_t value;
	int r=0,i=0;

	ITE_OP_CODE = ITE_OP_CODE_DBGR_O;

        Flash.read_mode = 3;
        Flash.erase_type = ITE_ERASE_TYPE_3_UNPROTECT_E;
        Flash.erase_mode = ITE_ERASE_MODE_1_SECTOR_ERASE ;
        Flash.write_type = 0; //ITE_PROGRAM_TYPE
        Flash.write_mode = 3; //ITE_PROGRAM_MODE

	CALL_CHECK(GetDlb4FwVer(g_fw_ver));
        CALL_CHECK(StartD2ec(7)); //Send Special
        msleep(50);
        CALL_CHECK(StartD2ec(0)); //Stop Special
        CALL_CHECK(StartD2ec(3)); //Enter Debug Mode

	value=0x04;
        RwDbgrCmdSet(0x01,0x1A,&value);
        RwDbgrCmdSet(0x00,0x1A,&value);

	CALL_CHECK(WriteReg(0x20,0x06,0x44));
        CALL_CHECK(WriteReg(0x10,0x63,0x00));
        CALL_CHECK(ReadReg(0x10,0x80,&value));

	CALL_CHECK(RunCtrl(0x81,0,0));
	//CALL_CHECK(StartD2ec(0x02)); //I2C Init & Enter Debug Mode //400K
	CALL_CHECK(StartD2ec(12)); //I2C Init & Enter Debug Mode //1M
        CALL_CHECK(StartD2ec(0x0a)); //Set Internal Flash
        //CALL_CHECK(StartD2ec(11)); //Set External Flash
        CALL_CHECK(GetChipID(g_chip_id));
	ReadReg(0x20,0x85,&g_chip_id[3]);
	ReadReg(0x20,0x86,&g_chip_id[4]);
	ReadReg(0x20,0x87,&g_chip_id[5]);
        CALL_CHECK(GetFlashID(g_flash_id,0x04));
	//CALL_CHECK(WriteNonSSTFlashStatus(0x82,0,0x2));
	CALL_CHECK(WriteNonSSTFlashStatus(0xff,0,0));

	//20220223 un-protect flash
	CALL_CHECK(WriteReg(0x1f,0x05,0x30));
	for(i=0;i<0x20;i++)
		CALL_CHECK(WriteReg(0x20,0xa0+i,0x00));

	return r;

}	

int enter_spi()
{

        int r;

        Dlb4SetGPIO(ITE_DLB_GPIO_G6,ITE_DLB_GPIO_LOW);
        Dlb4SetGPIO(ITE_DLB_GPIO_G6,ITE_DLB_GPIO_OUTPUT);
        msleep(100);

        Dlb4SetGPIO(ITE_DLB_GPIO_C1,ITE_DLB_GPIO_LOW);
        Dlb4SetGPIO(ITE_DLB_GPIO_C1,ITE_DLB_GPIO_OUTPUT);
        msleep(100);

        Dlb4SetGPIO(ITE_DLB_GPIO_C1,ITE_DLB_GPIO_HIGH);
        Dlb4SetGPIO(ITE_DLB_GPIO_C1,ITE_DLB_GPIO_ALT);
        msleep(100);

        Dlb4SetGPIO(ITE_DLB_GPIO_G6,ITE_DLB_GPIO_HIGH);
        Dlb4SetGPIO(ITE_DLB_GPIO_G6,ITE_DLB_GPIO_ALT);
        return r;

}	

int reset_ec()
{
	int r;

	CALL_CHECK(WriteReg(0x20,0x06,0x44));
	CALL_CHECK(RunCtrl(0x80,0,0));

	Dlb4SetGPIO(ITE_DLB_GPIO_C1,ITE_DLB_GPIO_LOW);
        Dlb4SetGPIO(ITE_DLB_GPIO_C1,ITE_DLB_GPIO_OUTPUT);
        //msleep(100);
        sleep(1);
        Dlb4SetGPIO(ITE_DLB_GPIO_C1,ITE_DLB_GPIO_HIGH);

	return r;
}	

void show_itedlb4()
{
        printf("\n\n\rITE DLB4 FW Version : %02x%02x",g_fw_ver[0],g_fw_ver[1]);
        printf("\n\r===================================");
	if(!(g_flag&ITE_USE_SPI)) {
        	printf("\n\rCHIP ID          : %x%02x%02x",g_chip_id[0],g_chip_id[1],g_chip_id[2]);
        	printf(" ( %x%02x%02x ) ",g_chip_id[3],g_chip_id[4],g_chip_id[5]);
	}
        printf("\n\rFlash ID         : %02x %02x %02x\n\r",g_flash_id[0],g_flash_id[1],g_flash_id[2]);
}	

int do_iteflash()
{
	int r=0;
	int loop=0;
	
	g_chip_id[0]=0x00;
	printf("\n\rConnecting ITE Device....");
	
	if((g_flag&ITE_USE_SPI)) {
	        printf("\n\rFlash via SPI interface...");
		init_dlb4_spi();
	} else {
		do {
			CALL_CHECK(init_dlb4());
			if(g_chip_id[0]!=0x00) {
				break;
			}
			printf(".");
                	fflush(stdout);
		}while(loop++ < 2000);

        	if(g_chip_id[0]==0) {
                	printf("\n\rGet Chip ERR! Please re-run the program");
                	return -1;
        	}

	}


	show_itedlb4();	

	CALL_CHECK(eraseall());
	if(!(g_flag&ITE_SKIP_CHECK))
		CALL_CHECK(checkall());
	CALL_CHECK(programall());
	if(!(g_flag&ITE_SKIP_VERIFY))
		CALL_CHECK(verifyall());

	//Enable QE Bit After flash
	CALL_CHECK(WriteNonSSTFlashStatus(0x82,0,0x2));

	reset_ec();

	return r;

}	



int ite_device(uint16_t vid, uint16_t pid)
{
	libusb_device_handle *handle;
	libusb_device *dev;
	int i, j, k, r=0;
	uint8_t endpoint_in = 0, endpoint_out = 0;	// default IN and OUT endpoints

	ITE_DBG("\n\rOpening device...\n");
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);

	if (handle == NULL) {
		perr("  Failed.\n");
		return -1;
	}

	endpoint_in = 0x81;
	endpoint_out = 0x02;
	devinfo.handle = handle;
	devinfo.endpoint_in = endpoint_in;
	devinfo.endpoint_out = endpoint_out;
	//CALL_CHECK(do_iteflash());
	r=do_iteflash();
	ITE_DBG("Closing device...\n");
	libusb_close(handle);

	return r;
}	


int init_usb()
{
	int r;
        const struct libusb_version* version;

        VID = 0x048D;
        PID = 0x8390;
        version = libusb_get_version();
        printf("Using libusb v%d.%d.%d.%d\n", version->major, version->minor, version->micro, version->nano);
        r = libusb_init(NULL);

	return r;
}	

int init_file(char* filename)
{
	int r=0;
	int len;
	int file_size;

	printf("\n\rOpen file: %s\n\r",filename);
        if ( (fi=fopen(filename,"rb"))!=NULL) {

                fseek(fi,0,SEEK_END);
                file_size = ftell(fi);
                fseek(fi,0,SEEK_SET);
		//printf("\n\rfile size = %d\n\r",file_size);
		g_blk_no= file_size / g_blk_size;
		if(file_size%g_blk_size)
			g_blk_no++;

        	g_flash_size=g_blk_size*g_blk_no;
		//printf("\n\rg_blk_no = %d g_flash_size=%d\n\r",g_blk_no,g_flash_size);



                g_writebuf = malloc(g_flash_size);
		if(g_writebuf==NULL) {
			printf("\n\ralloc g_writebuf fail");
		}	
                g_readbuf = malloc(g_flash_size);
		if(g_readbuf==NULL) {
			printf("\n\ralloc g_readbuf fail");
		}	
                len = fread(g_writebuf,1,g_flash_size,fi);
        } else {
                printf("open file error : %s \n",filename);
		r=ITE_ERR;
        }

	return r;
}	


void exit_file()
{
	free(g_writebuf);
	free(g_readbuf);
	fclose(fi);
}	

void show_time()
{
        time_t current_time;
        char* c_time_string;

        /* Obtain current time. */
        current_time = time(NULL);
        /* Convert to local time format. */
        c_time_string = ctime(&current_time);
         (void) printf("Current time is %s", c_time_string);
}	

void check_parameter()
{
        g_blk_size=65536;
        g_blk_no=16;
        g_flash_size=g_blk_size*g_blk_no;

        if((g_flag&ITE_USE_SPI)) {
                printf("\n\rFlash via SPI interface...");
                ITE_CONNECT_MODE        = ITE_CONNECT_MODE_NODBGR;
                ITE_OP_CODE             = ITE_OP_CODE_DBGR_X;
                ITE_FUN_CODE_FLASHID    = ITE_FUN_CODE_FLASHID_READ_SPI;
		ITE_FUN_CODE_READ	= ITE_FUN_CODE_FLASH_READ_SPI;
		ITE_FUN_CODE_ERASE	= ITE_FUN_CODE_FLASH_ERASE_SPI;
		ITE_FUN_CODE_WRITE	= ITE_FUN_CODE_FLASH_WRITE_SPI;

		Flash.read_mode = 3;
        	Flash.erase_type = ITE_ERASE_TYPE_3_UNPROTECT_E;
        	Flash.erase_mode = ITE_ERASE_MODE_0_CHIP_ERASE ;
                Flash.write_type = 0; //ITE_PROGRAM_TYPE
                Flash.write_mode = 3; //ITE_PROGRAM_MODE


        } else {
                printf("\n\rFlash via I2C interface...");
                ITE_CONNECT_MODE        = ITE_CONNECT_MODE_DBGR;
                ITE_OP_CODE             = ITE_OP_CODE_DBGR_O;
                ITE_FUN_CODE_FLASHID    = ITE_FUN_CODE_FLASHID_READ;
		ITE_FUN_CODE_READ	= ITE_FUN_CODE_FLASH_READ;
		ITE_FUN_CODE_ERASE	= ITE_FUN_CODE_FLASH_ERASE;
		ITE_FUN_CODE_WRITE	= ITE_FUN_CODE_FLASH_WRITE;

        	Flash.read_mode = 3;
        	Flash.erase_type = ITE_ERASE_TYPE_3_UNPROTECT_E;
        	Flash.erase_mode = ITE_ERASE_MODE_1_SECTOR_ERASE ;
        	Flash.write_type = 0; //ITE_PROGRAM_TYPE
        	Flash.write_mode = 3; //ITE_PROGRAM_MODE

        }



}	

int main(int argc, char** argv)
{
	int r=0;
	int option_index = 0;
	int c;
	char *filename=NULL;
	//char *optstring = "f:s:";
	char *optstring = "f:s:u";
	char *skip=NULL;
	char skip_check[]="check";
	char skip_verify[]="verify";
	struct option long_options[] = {
        	{ "filename",       required_argument,      NULL, 'f' },
        	{ "skip",           required_argument,      NULL, 's' },
        	{ "usespi",        no_argument,      NULL, 'u' },
        	{ 0, 0, 0, 0}
    	};


    	while (1) {
        	c = getopt_long(argc, argv, optstring, long_options, &option_index);
        	if (c == -1)
            		break;

        	switch (c) {
			//use -f to skip check stage
			case 'f': filename=optarg; 		break;
            		case 's': skip = optarg; 
				  if(strcmp(skip,skip_check)==0)
					g_flag |= ITE_SKIP_CHECK;
				  if(strcmp(skip,skip_verify)==0)
					g_flag |= ITE_SKIP_VERIFY;

				  break;
                        case 'u': 
				  g_flag |= ITE_USE_SPI;
                                  break;
            		default:
                		printf("Usage: %s [...]\n", argv[0]);
                		exit(1);
        	}
    	}

	check_parameter();

	printf("\n\rITE DLB4 Linux Flash Tool: Version %s\n\r",VERSION);
	show_time();
	if(filename == NULL) {
		printf("\n\rchoose a file to flash..\n\r");
		return 0;
	}	

	r=init_file(filename);
	if(r) {
                printf("Open file error\n\r");
                exit(1);
	}	

	r=init_usb();
	if (r < 0)
                return r;


	r=ite_device(VID,PID);
	if(r<0) {
		printf("\n\rFlash Fail...");
		printf("\n\rPlease re-plug the 8390 download board or ");
		printf("\n\rpower on the ec...\n\r");
	}

        libusb_exit(NULL);

	exit_file();
	show_time();
	return r;
}
