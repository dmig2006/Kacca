/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qrencode.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <qstring.h>
#include <qapplication.h>
#include <qinputdialog.h>
#include <xbase/xbase.h>

#include "ElvisFiscalRegVer3.h"
#include "Utils.h"
#include "Cash.h"
#include "DbfTable.h"
#include "Payment.h"

#define	STR_PRINT
//!#define REG_DEBUG

#define DEBUG_FR

///d.gusakov
#define TESTLOG
/// d.gusakov

enum {STX=0x02,ETX=0x03,EOT=0x04,ENQ=0x05,ACK=0x06,DLE=0x10,NAK=0x15,};

//Shtrich-M version 3

unsigned long ElvisRegVer3::SerialNumber(void)
{
	if(GetMode())
		return 0;
    return *(unsigned long *)(DataN + 32);
}

ElvisRegVer3::ElvisRegVer3(void)
{
#ifndef TESTLOG
        ;;Log("*****************************************************************************");
        ;;Log("NameGoods=" + Sales[0].Name);
        ;;Log("******************************************************************************");
#endif
	
	RegCom=0;
	MCU=100;
	// + ==== 2013-09-22 =====
	// ����
	//MinDuration=50;//20;//50;
	MinDuration=25;//50;//20;//50;
	// - ==== 2013-09-22 =====
	//DataNLen=128;

	DataNLen=200;


 	StartClosing=false;
	fclose(fopen("tmp/fr.log","a"));
	log = fopen("tmp/fr.log","r+");
	fseek(log, 0, SEEK_END);
	setbuf(log,NULL);

   // control
	fclose(fopen("tmp/control.log","a"));
	cLog = fopen("tmp/control.log","r+");
	fseek(cLog, 0, SEEK_END);
	setbuf(cLog,NULL);

	sum = 0;

    sum0=sum1=sum2=sum3=sum4=sum5=sum6=0;
    for(unsigned int i=0;i<11;i++) SumSec[i]=0;

    DBArchPath = "arch";
}

ElvisRegVer3::~ElvisRegVer3(void)
{
	if (RegCom!=0)
		ClosePort(RegCom);
}

int ElvisRegVer3::InitReg(int port,int baud,int type,const char *passwd)
{
    ConnectionStatus = true;

	WaitPeriod=MinDuration;

	RegPort=port;
	RegBaud=baud;
	RegType=type;
	// + ==== 2013-09-22 =====
	// ����
	//RegTimeout=2000;//100;//2000;
	RegTimeout=1000;//2000;//100;//2000;
	// - ==== 2013-09-22 =====
	RegPassword=atoi(passwd);

	if (RegCom!=0)
	{
		SetInstrHeader(0x14);
		DataN[5]=(port-1)%255;
		switch (baud)
		{
			case   2400:DataN[6]=0;break;
			case   4800:DataN[6]=1;break;
			case   9600:DataN[6]=2;break;
			case  19200:DataN[6]=3;break;
			case  38400:DataN[6]=4;break;
			case  57600:DataN[6]=5;break;
			case 115200:DataN[6]=6;break;
			default:    DataN[6]=0;break;
		}
		DataN[7]=MinDuration;
		JustDoIt(8,MinDuration);
		ClosePort(RegCom);
		RegCom=0;
	}

	return ResetReg();
}

int ElvisRegVer3::JustDoIt(unsigned int len,int timeout)
{
	memmove(DataN+2,DataN,len);
	DataN[0]=STX;
	DataN[1]=len;

	DataN[len+2] = 0;

	for (int i=0;i<=len;i++)
		DataN[len+2]^=DataN[i+1];

#ifdef DEBUG_FR
    int _n=0;
	for(int _i=0;_i<len+3;_i++)
    {
       if (_n%20==0) {
            fprintf(log,"\n%s\t -> ", GetCurDateTime());
            fflush(log);
            _n=0;
       }
       fprintf(log,"%0*hhX ",2,DataN[_i]);
       fflush(log);
       _n++;
    }
#endif

	ClearPort(RegCom);

	if (timeout>0)
		WaitPeriod=timeout;
	else
		WaitPeriod=MinDuration;

	int er;
	char store[512];
	static char call = 0;
	static bool yes = true;
	memcpy(store, DataN, DataNLen);

	for(char count = 2;;count--)
	{
	    ConnectionStatus = true;
		er = ProcessInstruction();
#ifdef DEBUG_FR
       fprintf(log,"\n%s\t ProcessInstruction result = %0*hhX (%s)", GetCurDateTime(),2,er,GetErrorStr(er,0).c_str());
       fflush(log);
#endif
		if(er >= 0)
			yes = true;
		if(call || !yes)
			break;

		if((er<0) || (er == 0x67) || (er == 0x80) || (er == 0x81) || (er == 0x82) || (er == 0x83))
		{

// + test
           fprintf(log,"\n%s\t ERROR CODE: %d\n", GetCurDateTime(), er);
           fflush(log);
// - test

		    ConnectionStatus = false;
		    if (!(yes = ShowQuestion(NULL, "������ ����� � ��.\n��������� �������?", true)))
		    {
		        break;
            }
        }

		if(!(((er < 0) || (er == 0x50) || (er == 0x58)) &&
			( count > 0 || (yes = ShowQuestion(NULL, "������ ������������.\n��������� �������?", true)))))
		{
			break;
		}

        if(er==0x6B)
        {
            // ��� ������� �����
            Log("FISCAL ERROR: ��� ������� �����!");

            if (!(yes = ShowQuestion(NULL, "��� ������� �����.\n��������� ����� � ���������� ������!\n���������� ������?", true)))
                break;
        }


//		Log("�����������: �������", store, DataNLen);
//		Log("�����������: �����", DataN, DataNLen);

		fprintf(log,"%s\t er = %d; sent:\n", GetCurDateTime(), er);
		fwrite(store, sizeof(char), DataNLen, log);
		fprintf(log,"\nanswer:\n");
		fwrite(DataN, sizeof(char), DataNLen, log);
		fflush(log);

		call = 1;
		ResetReg();
		call = 0;

		fprintf(log,"\nResetReg:\n");
		fwrite(DataN, sizeof(char), DataNLen, log);
		fprintf(log,"\n");
//		Log("�����������: ResetReg", DataN, DataNLen);

		memcpy(DataN, store, DataNLen);
	}

	WaitPeriod=MinDuration;
	return er;
}

int ElvisRegVer3::ProcessInstruction(int rev)
{
    int rpos = 1;
	int i = 3;
    int RetCode = 0;
	bool prev = true;
	do{
		unsigned char b;

		ClearPort(RegCom);

// +dms =========== �������� ������ ������ ========

// �������� ENG (0x05) - �������� ���������� ��
// �����: NAK (0x15 = 21) -  �� ����� � ������ ��������� �������
// ACK ( 0x06 ) - �� �����, ������� �����
// ��� ������ - ��� ����� � ��

		b=ENQ;
		WriteToPort(RegCom,&b,1);
#ifdef REG_DEBUG
		long savepos = ftell(log);

       fprintf(log,"\n\n%s\t -> %0*hhX ENG �������� ���������� ��", GetCurDateTime(),2,b);
       fflush(log);

		//fprintf(log,"\n\nStart\n%s; WriteToPort5 %d (%04X)\n", GetCurDateTime(), b, b);
		//fflush(log);
#endif
		{
			for(int j = 6;;)
			{//10 ������ ������� ����
				if(!--j)
					return -1; // ��� ������ �� ENQ - ������� � ������� "��� ����� � ��"
#ifdef REG_DEBUG

				fprintf(log,"\n%s\t j = %d", GetCurDateTime(), j);
				fflush(log);
#endif
				if(ReadFromPort(RegCom,&b,1))
				{
                    // ����� ����� USB
                    if (b == 0xFF) {
#ifdef REG_DEBUG
                        fprintf(log,"\n%s\t <- %0*hhX", GetCurDateTime(),2,b);
#endif
                        ReadFromPort(RegCom,&b,1);
                    }
                    // ����� ����� USB

#ifdef REG_DEBUG
                    // ���� ����� (������ ���� = 15h (21)), ������� ��� ������� ������
                    fprintf(log,"\n%s\t <- %0*hhX ����� ���������� �� (������ ���� NAK=15h)", GetCurDateTime(),2,b,b);
					//fprintf(log,"%s\t ReadFromPort21 %d (%04X)\n", GetCurDateTime(), b, b);
					fflush(log);
#endif
					break;
                }
            } // for
		}

		// ������ ������ �� ENG (�������� ���������� �� � ������)
		if(b == NAK)
		{
		    // ��� ��, �� ����� � ������
		    //������ 11 ������� �������� �������
			prev = false;
			for(int j = 11;;)
			{
				if(!--j)
					return -2;

				// �������� �������
				// ������ - "STX <�������>"
				// �����: ACK = ������� ������� ���������
				//      NAK - ������ �������
				//      ��� ������ - ��� ����� � ��, ������� �� �������
#ifdef REG_DEBUG
                fprintf(log,"\n%s\t -> ���������� �������: ", GetCurDateTime());
                fprintf(log,"\n%s\t -> ", GetCurDateTime());
                fflush(log);

                int len = DataN[1]+3;
                for(int _i=0;_i<len;_i++)
                {
            //! �������� ����������� ���
                   fprintf(log,"%0*hhX ",2,DataN[_i]);
                }
                fflush(log);
#endif

				WriteToPort(RegCom,DataN,DataN[1]+3);

#ifdef REG_DEBUG
				//fprintf(log,"\n%s\t WriteToPort DataN\n", GetCurDateTime());
				//fwrite(DataN, sizeof(char), DataN[1]+3, log);
				fprintf(log,"<end, bytes: %d>\n", DataN[1]+3);
				fflush(log);
#endif
				for(int ii = 5; --ii;)
				{// 10 ������ ���� ������������� ������


					if(ReadFromPort(RegCom,&b,1))
					{
					    // ������ ����� �� ��
					    // ��� �������� ���������� ������ ������ ACK (6)
#ifdef REG_DEBUG
                        fprintf(log,"\n%s\t <- %0*hhX ����� �� ������� (������ ���� ACK=06h)", GetCurDateTime(),2,b);
						//fprintf(log,"%s\t ReadFromPort6 %d (%04X)\n", GetCurDateTime(), b, b);
						fflush(log);
#endif
						break;

					}

				} // for
				if(b == ACK)
					break; // �������� ����������, ����� �� �����
			}// for


		}
		else
            if(b != ACK)
            {
                // ������ ���������� ��, ��������� �����  � �������� �������
                while(ReadFromPort(RegCom,&b,1))
                {
    #ifdef REG_DEBUG
                    fprintf(log,"\n%s\t <- %0*hhX ������ ���������� �� (������ ���� ACK=06h)", GetCurDateTime(),2,b);
 //                   fprintf(log,"%s\t ReadFromPort %d (%04X)\n", GetCurDateTime(), b, b);
                    fflush(log);
    #endif
                }
                continue;
            }


		{
        for(int j = 10;;)
        {
            if(!--j)
                goto while_end;
#ifdef REG_DEBUG
            fprintf(log,"\n%s\t j = %d", GetCurDateTime(), j);
            fflush(log);
#endif
            for(int ii = 5; --ii;){// 10 ������ ���� ����
#ifdef REG_DEBUG
                fprintf(log,"\n%s\t ii = %d", GetCurDateTime(), ii);
                fflush(log);
#endif
                if(ReadFromPort(RegCom,&b,1))
                {
#ifdef REG_DEBUG
                    fprintf(log,"\n%s\t <- %0*hhX ����� STX (������ ���� STX=02h)", GetCurDateTime(),2,b);
                   // fprintf(log,"\n%s\t ReadFromPort2 %d (%04X)", GetCurDateTime(), b, b);
                    fflush(log);
#endif
                    break;
                }
            }
            if(b == STX)
            {
#ifdef REG_DEBUG
                fprintf(log,"\n%s\t ReadedFromPortSTX", GetCurDateTime());
                fflush(log);
#endif
                break;
            }
        }
		}

		unsigned char len;
		if (ReadFromPort(RegCom,&len,1)==0){
#ifdef REG_DEBUG
			fprintf(log,"\n%s\t return -3;", GetCurDateTime());
			fflush(log);
#endif
			return -3;//-1;
		}
#ifdef REG_DEBUG
		fprintf(log,"\n%s\t <- %0*hhX", GetCurDateTime(), 2, len);
        fflush(log);
#endif
        fprintf(log,"\n%s\t <- ", GetCurDateTime());
		{
			char buf[1024];
			unsigned char CRC=len;
			for (int j=0;j<len;j++){
				if (ReadFromPort(RegCom,&b,1)==0){
#ifdef REG_DEBUG
					fprintf(log,"\n%s\t return -4;", GetCurDateTime());
					fflush(log);
#endif
					return -4;//-1;
				}

				fprintf(log,"%0*hhX ", 2, b);

				CRC ^= (buf[j] = b);

			}
			//!fwrite(buf, sizeof(char), len, log);

			if(ReadFromPort(RegCom,&b,1)==0 || CRC!=b)
			{
			    //fprintf(log,"\n%s\t <- %0*hhX", GetCurDateTime(), b, 2, b);
			    fprintf(log," (CRC is Error (%0*hhX != %0*hhX))", 2, b, 2, CRC);
                fflush(log);

#ifdef REG_DEBUG
				fprintf(log,"\n%s\t continue -5;", GetCurDateTime());
				fflush(log);
#endif
				b=NAK;
				WriteToPort(RegCom,&b,1);
				continue;
//				return -5;//-2;
			}

            fprintf(log,"%0*hhX ", 2, b);
            fprintf(log," (CRC is Ok (%0*hhX = %0*hhX))", 2, b, 2, CRC);
            fflush(log);

//			switch(buf[1]){};
			memset(DataN,0,DataNLen);
			memcpy(DataN,buf,len);
			DataN[len]=0;
		}


		b=ACK;
		if (WriteToPort(RegCom,&b,1)) {
#ifdef REG_DEBUG
		    fprintf(log,"\n%s\t -> 06 (ACK)", GetCurDateTime());
#endif
		} else {
#ifdef REG_DEBUG
		    fprintf(log,"\n%s\t -> ERROR write to port 06 (ACK)", GetCurDateTime());
#endif
		}

		if(prev){
			fprintf(log,"CONTINUE");
			continue;
			}

		//WaitSomeTime(WaitPeriod);

        rpos = 1;
		if (DataN[0] == 0xFF) {
		    rpos = 2;
		}

#ifdef REG_DEBUG
        //fprintf(log,"\n%s\t <- %d (%0*hhX) ����� STX (������ ���� STX=2)", GetCurDateTime(),b,2,b);
        //RetCode = DataN[1];
        RetCode = DataN[rpos];
		fprintf(log,"\n%s\t ��� ������ FR (DataN[1]): %0*hhX %s", GetCurDateTime(), 2, RetCode, GetErrorStr(RetCode,0).c_str());
		if(! DataN[rpos]){
			//fsetpos(log, &savepos);
			fseek(log, savepos, SEEK_SET);
		}
		fflush(log);
#endif
		return DataN[rpos];//0;
while_end:;
	}while(--i);

#ifdef REG_DEBUG
	fprintf(log,"\n%s\t return -6;", GetCurDateTime());
	fflush(log);
#endif
	return -6;
}

int ElvisRegVer3::ResetReg(void)
{
	if (RegCom!=0)
		ClosePort(RegCom);

	if (RegPort*RegBaud!=0)
	    RegCom=OpenPort(RegPort,RegBaud,0,RegTimeout);
	else
	    RegCom=0;
	if (RegCom==0)
		return -1;

	switch (RegType)//according to register's type set line length and speed of printing
	{
		case ShtrichVer3Type:
			LineLength=getLineWidthDefault();
			RegSpeed=75;
			EntireCheck=false;
		break;
		case ShtrichVer3EntireType:case ShtrichCityType:
			LineLength=getLineWidthDefault();
			RegSpeed=75;
			EntireCheck=true;
		break;
		default:
			LineLength=getLineWidthDefault();
			RegSpeed=75;
			EntireCheck=false;
		break;
	}

	ClearPort(RegCom);

	bool restore;
	int er = RestorePrinting(restore);

	if (er!=0)
		return er;

	if (restore)
		return ContinuePrinting();
	else
		return 0;
}

string ElvisRegVer3::GetErrorStr(int er,int lang)
{
//	ResetReg();

	if (lang==0)
		switch(er)
		{
			case   -4:
			case   -6:
			case   -9:
			case   -2: return "��� ������";
			case   -3:
			case   -5:
			case   -7:
			case   -8:
			case   -1: return "��� �����";
			case 0x00: return "";
			case 0x01: return "���������� ���������� �� 1,�� 2 ��� ����";
			case 0x02: return "����������� �� 1";
			case 0x03: return "����������� �� 2";
			case 0x04: return "������������ ��������� � ������� ��������� � ��";
			case 0x05: return "��� ����������� ������ � ��";
			case 0x06: return "�� � ������ ������ ������";
			case 0x07: return "������������ ��������� � ������� ��� ������ ���������� ��";
			case 0x08: return "������� �� �������������� � ������ ���������� ��";
			case 0x09: return "������������ ����� ������� ��";
			case 0x0A: return "������ ������ �� �� BCD";
			case 0x0B: return "���������� ������ ������ �� ��� ������ �����";
			case 0x11: return "�� ������� ��������";
			case 0x12: return "��������� ����� ��� ������";
			case 0x13: return "������� ���� ������ ���� ��������� ������ � ��";
			case 0x14: return "������� ������� ������ �� �����������";
			case 0x15: return "����� ��� �������";
			case 0x16: return "����� �� �������";
			case 0x17: return "����� ������ ����� ������ ������ ��������� �����";
			case 0x18: return "���� ������ ����� ����� ���� ��������� �����";
			case 0x19: return "��� ������ � ��";
			case 0x1A: return "������� ��������������� � �� �����������";
			case 0x1B: return "��������� ����� �� ������";
			case 0x1C: return "� �������� ��������� ���� ������������ ������";
			case 0x1D: return "���������� ��������� ������ ������� ������";
			case 0x1E: return "������� ��������������� �� �����������";
			case 0x1F: return "����������� ������ ��������� � ��";
			case 0x20: return "������������ ��������� �������� �� ��� ����������";
			case 0x21: return "���������� ����� ������ ����������� ��������� �������� ��";
			case 0x22: return "������������ ����";
			case 0x23: return "��� ������ �����������";
			case 0x24: return "������� ����������� �����������";
			case 0x25: return "��� ����������� � ������������� �������";
			case 0x26: return "�������� �������� ����� ������ ����� ����";
			case 0x2B: return "���������� �������� ���������� �������";
			case 0x2C: return "���������� ����� (��������� ������� ����������)";
			case 0x2D: return "����� ���� �� ������ ������ ����� ������";
			case 0x2E: return "� �� ��� ����� ��� �������";
			case 0x30: return "�� ������������, ���� ����� ������ ���������� ����������";
			case 0x32: return "��������� ���������� ������ �������";
			case 0x33: return "������������ ��������� � �������";
			case 0x34: return "��� ����������� ������";
			case 0x35: return "�������� �������� ��� ������� ��������� ��";
			case 0x36: return "������������ ��������� � ������� ��� ������ ���������� ��";
			case 0x37: return "������� �� �������������� � ������ ���������� ��";
			case 0x38: return "������ � ������ �������� ��";
			case 0x39: return "���������� ������ �� ��";
			case 0x3A: return "������������ ���������� �� ��������� � �����";
			case 0x3B: return "������������ ���������� � �����";
			case 0x3C: return "����� ������� - �������� ����������";
			case 0x3D: return "����� �� ������� - �������� ����������";
			case 0x3E: return "������������ ���������� �� ������� � �����";
			case 0x3F: return "������������ ���������� �� ������� � �����";
			case 0x40: return "������������ ��������� ������";
			case 0x41: return "������������ ��������� ���������";
			case 0x42: return "������������ ��������� ��� 2";
			case 0x43: return "������������ ��������� ��� 3";
			case 0x44: return "������������ ��������� ��� 4";
			case 0x45: return "C���� ���� ����� ������ ������ ����� ����";
			case 0x46: return "�� ������� ���������� � �����";
			case 0x47: return "������������ ���������� �� ������� � �����";
			case 0x48: return "������������ ����� ����";
			case 0x49: return "�������� ���������� � �������� ���� ������� ����";
			case 0x4A: return "������ ��� - �������� ����������";
			case 0x4B: return "����� ���� ����������";
			case 0x4C: return "������������ ���������� �� ������� ������� � �����";
			case 0x4D: return "�������� ����������� ������� ����� ������ ����� ����";
			case 0x4E: return "����� ��������� 24 ����";
			case 0x4F: return "�������� ������";
			case 0x50: return "���� ������ ���������� �������";
			case 0x51: return "������������ ���������� ��������� � �����";
			case 0x52: return "������������ ���������� �� ���� ������ 2 � �����";
			case 0x53: return "������������ ���������� �� ���� ������ 3 � �����";
			case 0x54: return "������������ ���������� �� ���� ������ 4 � �����";
			case 0x55: return "��� ������ - �������� ����������";
			case 0x56: return "��� ��������� ��� �������";
			case 0x57: return "����: ���������� �������� ���� �� ��������� � ��";
			case 0x58: return "�������� ������� ����������� ������";
			case 0x59: return "�������� ������ ������ ����������";
			case 0x5A: return "������ ��������� ���������� � ����";
			case 0x5B: return "������������ ��������� ��������";
			case 0x5C: return "�������� ���������� 24�";
			case 0x5D: return "������� �� ����������";
			case 0x5E: return "������������ ��������";
			case 0x5F: return "������������� ���� ����";
			case 0x60: return "������������ ��� ���������";
			case 0x61: return "������������ ��������� ����";
			case 0x62: return "������������ ��������� ����������";
			case 0x63: return "������������ ��������� ������";
			case 0x64: return "�� �����������";
			case 0x65: return "�� ������� ����� � ������";
			case 0x66: return "������������ ����� � ������";
			case 0x67: return "������ ����� � ��";
			case 0x68: return "�� ������� ����� �� ������� �������";
			case 0x69: return "������������ ����� �� ������� �������";
			case 0x6A: return "������ ������� � ������ ������ �� I2C";
			case 0x6B: return "��� ������� �����";
			case 0x6C: return "��� ����������� �����";
			case 0x6D: return "�� ������� ����� �� ������";
			case 0x6E: return "������������ ����� �� ������";
			case 0x6F: return "������������ �� ������� � �����";
			case 0x70: return "������������ ��";
			case 0x71: return "������ ��������� ������";
			case 0x72: return "������� �� �������������� � ������ ���������";
			case 0x73: return "������� �� �������������� � ������ ������";
			case 0x74: return "������ ����������� ������ ��";
			case 0x75: return "������ �������";
			case 0x76: return "������ ��������: ��� ��������� � ��������������";
			case 0x77: return "������ ��������: ��� ������� � ��������";
			case 0x78: return "������������ �� ������� � �����";
			case 0x79: return "������ ��";
			case 0x7A: return "���� �� �������������";
			case 0x7B: return "������ ������������";
			case 0x7C: return "�� ��������� ����";
			case 0x7D: return "�������� ������ ����";
			case 0x7E: return "�������� �������� � ���� �����";
			case 0x7F: return "������������ ��������� ����� ����";
			case 0x80:
			case 0x81:
			case 0x82:
			case 0x83: return "������ ����� � ��";
			case 0x84: return "������������ ����������";
			case 0x85: return "������������ �� �������� � �����";
			case 0x86: return "������������ �� �������� � �����";
			case 0x87: return "������������ �� ��������� ������ � �����";
			case 0x88: return "������������ �� ��������� ������� � �����";
			case 0x89: return "������������ �� �������� � �����";
	                case 0x8A: return "������������ �� ��������� � ����";
	                case 0x8B: return "������������ �� ������� � ����";
	                case 0x8C: return "������������� ���� �������� � ����";
	                case 0x8D: return "������������� ���� ������ � ����";
	                case 0x8E: return "������� ���� ����";
	                case 0x8F: return "����� �� ���������������";
	                case 0x90: return "���� ��������� ������, ������������� � ����������";
	                case 0x91: return "����� �� ������� ���� ������ ��� ������ ���������� ������";
	                case 0x92: return "��������� �����";
	                case 0x93: return "�������������� ��� ������ �������";
	                case 0x94: return "�������� ����� �������� � ����";
	                case 0xA0: return "������ ����� � ����";
	                case 0xA1: return "���� �����������";
	                case 0xA2: return "����: ������������ ������ ��� �������� �������";
	                case 0xA3: return "������������ ��������� ����";
	                case 0xA4: return "������ ����";
	                case 0xA5: return "������ �� � ������� ����";
	                case 0xA6: return "�������� ��������� ������ ����";
	                case 0xA7: return "���� �����������";
	                case 0xA8: return "����: �������� ���� � �����";
	                case 0xA9: return "����: ��� ����������� ������";
	                case 0xAA: return "������������ ���� (������������� ���� ���������)";
	                case 0xB0: return "����: ������������ � ��������� ����������";
	                case 0xB1: return "����: ������������ � ��������� �����";
	                case 0xB2: return "����: ��� ��������������";
	                case 0xC0: return "�������� ���� � ������� (����������� ���� � �����)";
	                case 0xC1: return "����: �������� ����� � �������� �������� ������";
	                case 0xC2: return "���������� ���������� � ����� �������";
	                case 0xC3: return "������������ ������ ���� � ����";
	                case 0xC4: return "������������ ������� ����";
	                case 0xC5: return "����� ����������� ��������� ����";
	                case 0xC6: return "���������� �������� �����������";
	                case 0xC7: return "���� �� ������������� � ������ ������";
                    case 0xD3: return "��� ������ �� ���������";
			default:   return "����������� ������";
		}

	if (lang==1)
		switch(er)
		{
			case   -4: return "4 No response";
			case   -6: return "6 No response";
			case   -9: return "9 No response";
			case   -2: return "2 No response";
			case   -3: return "3 No connection";
			case   -5: return "5 No connection";
			case   -7: return "7 No connection";
			case   -8: return "8 No connection";
			case   -1: return "1 No connection";
			case 0x00: return "";
			case 0x01: return "The storage device in FM 1, FM 2 or clock is damaged";
			case 0x02: return "FM 1 not exists";
			case 0x03: return "FM 2 not exists";
			case 0x04: return "Wrong parameters in the instruction referenced to FM";
			case 0x05: return "Selected data in FM does not exists";
			case 0x06: return "FM is in the mode of the data output";
			case 0x07: return "Wrong parameters in the instruction for the current release of FM";
			case 0x08: return "The instruction is not supported in the current release of FM";
			case 0x09: return "Wrong length of the FM's instruction";
			case 0x0A: return "Data format of FM is not BCD";
			case 0x0B: return "Faulty cell of memory FM when writing a total";
			case 0x11: return "The license is not entered";
			case 0x12: return "The plant number has been already entered";
			case 0x13: return "The current date is fewer than the date of the last writing in FM";
			case 0x14: return "The field of the shift's results is overflowed";
			case 0x15: return "The shift has been already opened";
			case 0x16: return "The shift has not been opened yet";
			case 0x17: return "The first shift's number is more than the number of the last shift";
			case 0x18: return "The first shift's date is more than the date of the last shift";
			case 0x19: return "No data in FM";
			case 0x1A: return "The range of the reregistrations in FR is overflowed";
			case 0x1B: return "The plant number is not entered";
			case 0x1C: return "The given range contains damaged record";
			case 0x1D: return "The last record of shift's results is damaged";
			case 0x1E: return "The range of the FR's reregistrations is overflowed";
			case 0x1F: return "The register's memory in FM does not exists";
			case 0x20: return "Overflow of the money register in FM after addition";
			case 0x21: return "Subtrahended sum is more than value of the FR's money register";
			case 0x22: return "Wrong date";
			case 0x23: return "No writing an activation";
			case 0x24: return "Area of activations is overfilled";
			case 0x25: return "No activations with required number";
			case 0x26: return "The customer's payment is less than check's sum";
			case 0x2B: return "Can not cancel the previous instruction";
			case 0x2C: return "Empty cash register (can not do Z-report again)";
			case 0x2D: return "The check's sum by sections is less then 'storno's' sum";
			case 0x2E: return "No money in FR for the payment";
			case 0x30: return "FR is locked and wait for the password of the tax officer";
			case 0x32: return "The Z-report is required";
			case 0x33: return "Wrong parameters of the instruction";
			case 0x34: return "No data";
			case 0x35: return "Wrong parameter for current parameters of FR";
			case 0x36: return "Wrong parameters in the instruction for the current release of FR";
			case 0x37: return "The instruction is not supported in the current release of FR";
			case 0x38: return "Error in the FR's memory";
			case 0x39: return "Internal error of the FR's software";
			case 0x3A: return "Overflow of the charge's sum in the shift";
			case 0x3B: return "Overflow of the shift's sum";
			case 0x3C: return "The shift is opened - the operation is impossible";
			case 0x3D: return "The shift is not opened - the operation is impossible";
			case 0x3E: return "Overflow of the sum by sections in the shift";
			case 0x3F: return "Overflow of the sum by discounts in the shift";
			case 0x40: return "Overflow of the discount range";
			case 0x41: return "Overflow of the cash range";
			case 0x42: return "Overflow of the range type 2";
			case 0x43: return "Overflow of the range type 3";
			case 0x44: return "Overflow of the range type 4";
			case 0x45: return "The sum of payments of all types is less than check's sum";
			case 0x46: return "Not enough cash in the register";
			case 0x47: return "Overflow of the tax's sum in the shift";
			case 0x48: return "Overflow of the check's result";
			case 0x49: return "The operation is impossible in the opened check of such a type";
			case 0x4A: return "The check is opened - the operation is impossible";
			case 0x4B: return "Check's buffer is overflowed";
			case 0x4C: return "Overflow of the sum by tax circulation in the shift";
			case 0x4D: return "The cashless payment is more than sum of check";
			case 0x4E: return "The shift is over 24 hours";
			case 0x4F: return "Wrong password";
			case 0x50: return "The previous instruction is printing";
			case 0x51: return "Overflow of the sum in cash in the shift";
			case 0x52: return "Overflow of the sum in payment type 2 in the shift";
			case 0x53: return "Overflow of the sum in payment type 3 in the shift";
			case 0x54: return "Overflow of the sum in payment type 4 in the shift";
			case 0x55: return "Check is closed - the operation is impossible";
			case 0x56: return "No document for repetition";
			case 0x58: return "Wait for the instruction of the printing continue";
			case 0x59: return "The document is opened by another operator";
			case 0x5A: return "The discount is more than sum of check";
			case 0x5B: return "Overflow of the charge's range";
			case 0x5D: return "The table is not defined";
			case 0x5E: return "Wrong instruction";
			case 0x5F: return "Negative result of the check";
			case 0x60: return "Multiplication overflow";
			case 0x61: return "Price's range overflow";
			case 0x62: return "Quantity's range overflow";
			case 0x63: return "Section's range overflow";
			case 0x64: return "FM does not exists";
			case 0x65: return "Insufficient amount of money in the section";
			case 0x66: return "Money overflow in the section";
			case 0x67: return "Connection error of FM";
			case 0x68: return "Insufficient amount of money by tax's circulation";
			case 0x69: return "Money overflow by tax's circulation";
			case 0x6B: return "Out of check paper";
			case 0x6C: return "Out of control paper";
			case 0x6D: return "Insufficient amount of money by tax";
			case 0x6E: return "Money overflow by tax";
			case 0x70: return "Overflow of FM";
			case 0x71: return "Error of the paper's cutter";
			case 0x72: return "The instruction is not supported in the current submode";
			case 0x73: return "The instruction is not supported in the current mode";
			case 0x74: return "Error of the FR's RAM";
			case 0x75: return "Power error";
			case 0x76: return "Printer error: no impulse from taxogenerator";
			case 0x77: return "Printer error: no signal from detectors";
			case 0x78: return "Overflow of the payment in the shift";
			case 0x79: return "FR replacing";
			case 0x7A: return "Field is not edited";
			case 0x7B: return "Equipment error";
			case 0x7C: return "The date is not equal";
			case 0x7D: return "Wrong date format";
			case 0x7E: return "Wrong value in the length field";
			case 0x80:case 0x81:case 0x82:case 0x83: return "Error of the connection to FR";
			case 0x84: return "Cash overflow";
			case 0x85: return "Overflow of sales in the shift";
			case 0x86: return "Overflow of purchases in the shift";
			case 0x87: return "Overflow of sale's returns in the shift";
			case 0x88: return "Overflow of purchase's returns in the shiift";
			case 0x89: return "Overflow of the income in the shift";
	                case 0x8A: return "Overflow on additions in cheque";
	                case 0x8B: return "Overflow on discounts in cheque";
	                case 0x8C: return "Negative result of addition in cheque";
	                case 0x8D: return "Negative result of discount in cheque";
	                case 0x8E: return "Zero result of cheque";
	                case 0x8F: return "Pay-desk not fiscalized";
	                case 0x90: return "Field exceeds size, stated in adjustment";
	                case 0x91: return "Output for border of field of seal under given adjusting a font";
	                case 0x92: return "Imposition of floors";
	                case 0x93: return "Reconstruction RAM pass successfully";
	                case 0x94: return "Exhausted quota of operations in cheque";
	                case 0xA0: return "Relationship Mistake with EKLZ";
	                case 0xA1: return "EKLZ IS ABSENT";
	                case 0xA2: return "EKLZ: INCORRECT format or parameter of command";
		        case 0xA3: return "Incorrect condition EKLZ";
	                case 0xA4: return "Damage EKLZ";
	                case 0xA5: return "Damage in composition EKLZ";
	                case 0xA6: return "Exhausted time resource EKLZ";
	                case 0xA7: return "EKLZ OVERFILLED";
	                case 0xA8: return "EKLZ: UNFAITHFUL date and time";
	                case 0xA9: return "EKLZ: NO inquired data";
	                case 0xAA: return "Overflow EKLZ (negative result of the document)";
	                case 0xB0: return "EKLZ: OVERFLOW in parameter an amount";
	                case 0xB1: return "EKLZ: OVERFLOW in parameter an amount";
	                case 0xB2: return "EKLZ: ALREADY actuated";
	                case 0xC0: return "Checking a date and time (confirm date and time)";
	                case 0xC1: return "EKLZ: DAYLY report with extinguish to break it is impossible";
	                case 0xC2: return "Excess of voltage in power supply unit";
	                case 0xC3: return "Mismatch of results of cheque and EKLZ";
	                case 0xC4: return "Change number Mismatch";
	                case 0xC5: return "Buffer of under document start;put";
	                case 0xC6: return "Under document is absent";
	                case 0xC7: return "Field is not editted in given mode";
	                case 0xD3: return "Good code do not recognized";
			default:   return "Unknown error";
		}

	return "";
}

bool ElvisRegVer3::IsPaperOut(int er)
{
	return ((er==0x6B)||(er==0x6C));
/*
	if ((er==0x6B)||(er==0x6C))
		return true;
	else
		return false;
*/
}

bool ElvisRegVer3::IsFullCheck(void)
{
	return EntireCheck;
}

bool ElvisRegVer3::RestorePaperBreak(void)
{
	return true;
}

int ElvisRegVer3::Cancel(void)
{
	bool restore;

	sum = 0;
	Sales.clear();
	StartClosing=false;

	int er = RestorePrinting(restore);
	if (er!=0)
		return er;

	if (restore){
		er=ContinuePrinting();
		if (er!=0){
			return er;
		}
	}

	er=GetMode();
	if (er!=0)
		return er;

	if (DataN[0x0F]==8)
	{
		SetInstrHeader(0x88);
		er = ProcessError(JustDoIt(5,100));
		if (er!=0)
			return er;
		CashRegister::ConfigTable->PutField("CHECKOPEN","F");

/*
		char d[9], t[9];
		_strdate(d);
		_strtime(t);
		fprintf(log,"%s\t%s;������������� ����\n", d, t);
*/


		fprintf(log,"%s\t������������� ����\n", GetCurDateTime());

		CntLog("��� �����������");
		CntLog("");
		CntLog("");

		//CutCheck();
	}else{
		if(CashRegister::ConfigTable->GetLogicalField("CHECKOPEN")){
			PrintString("��� �����������");
			PrintString("");
			PrintString("");
			PrintHeader();
			CutCheck();
			CashRegister::ConfigTable->PutField("CHECKOPEN","F");
		}
	}
	return 0;
}


int EmptyFiscalReg::GoodsStr(const char * n, double p, double v, double s)
{
	if(fabs(s) < .01) s = p*v;
	char str[41];
	memset(str, ' ', 40);
	str[40] = 0;
	if(fabs(v - 1) > .001){
		int er = PrintString(n);
		if(er)
			return er;
		sprintf(str, "%-9.3f X %-9.2f", v, p);
	}else{
		strncpy(str,n,40);
	}
	return CreatePriceStr(str, s);
}

int EmptyFiscalReg::PaymentStr(PaymentInfo payment)
{
	char str[41];
	memset(str, ' ', 40);
	str[40] = 0;

	if((payment.Type == 10) && (payment.Summa > 0))
	{
		snprintf(str,40,"���������� #%s", payment.Number.c_str(), payment.Summa);
		return CreatePriceStr(str, payment.Summa);
	}

	if((payment.Type == 15) && (payment.Summa > 0))
	{
		snprintf(str,40,"���������� #%s", payment.Number.c_str(), payment.Summa);
		return CreatePriceStr(str, payment.Summa);
	}

	if((payment.Type == 11) && (payment.Summa > 0))
	{
		snprintf(str,40,"\"�������\" �� ��������� ", payment.Summa);
		return CreatePriceStr(str, payment.Summa);
	}

	if((payment.Type == 12) && (payment.Summa > 0))
	{
		snprintf(str,40,"������ #%s", payment.Number.c_str(), payment.Summa);
		return CreatePriceStr(str, payment.Summa);
	}

	return 0;
}


// �������� ������� �������� �������� ������������ (���������� ����)
// �� ���������� � �������� �������
bool ElvisRegVer3::ControlPaymentTicket(string *PaymentHeader, int CashDesk, int CheckNum, string CashmanName )
{
    bool flag = false;

    if (Payment.size()>0)
    {
        // ����
		*PaymentHeader += xbDate().FormatDate("DD.MM.YYYY").c_str();

		// �����
        time_t cr_time = time(NULL);
        tm *l_time = localtime( &cr_time );
        *PaymentHeader +=" ";
        *PaymentHeader +=QTime(l_time->tm_hour,l_time->tm_min,l_time->tm_sec).toString().latin1();
        *PaymentHeader += "\n";

		//  ����� , ���
        string FirstStr,SecondStr,SpaceStr;
        FirstStr="����� #"+Int2Str(CashDesk);

        // + dms ====== ��� ���� ������������ ��������� �������� � ���������� ���������
        string CheckStr = CheckName;
        if (CheckStr.empty()) CheckStr = "���";

        SecondStr=CheckStr+" #"+Int2Str(CheckNum);
        // - dms ======

        *PaymentHeader += GetFormatString(FirstStr, SecondStr, LineLength) + "\n";
        *PaymentHeader += "������ " + CashmanName + "\n";
        *PaymentHeader += "������� c���������� � ������: \n";
        for(unsigned int i=0;i<Payment.size();i++)
		 {

		    PaymentInfo crpayment = Payment[i];
            // �����������
            if(((crpayment.Type == 10) || (crpayment.Type == 15)) && (crpayment.Summa >= 0))
                {
                    flag = true;

                    FirstStr  = " ���������� " + crpayment.Number;
                    SecondStr = "  ="+ GetRounded(crpayment.Summa,0.01);

                    *PaymentHeader+= GetFormatString(FirstStr, SecondStr, LineLength) + "\n";
                }
		 }

		*PaymentHeader+= "����������� ������:\n";
        *PaymentHeader+= "\n";
        *PaymentHeader+= "___________________________________\n";
		*PaymentHeader+= " \n";
        *PaymentHeader+= " \n";
        *PaymentHeader+= " \n";
	    *PaymentHeader += "printheader\n";
        *PaymentHeader += "cutcheck\n";

    }

    return flag;

}

int ElvisRegVer3::ChangeRegBaud(int port,int oldbaud, int baud,int type,const char *passwd)
{
    return 0;
}

int ElvisRegVer3::PrintCheckEgais(string CheckEgaisHeader, string egais_url, string egais_sign, int ModePrintQRCode)
{

	PrintFormatText(CheckEgaisHeader);

    if (ModePrintQRCode>0) PrintQRCode(egais_url, ModePrintQRCode);

    PrintFormatText(egais_url);

    PrintFormatText(egais_sign);

	return 0;
}

int ElvisRegVer3::Close(double Summ,double SummToPay,double Disc,int CashDesk,int CheckNum,int PaymentType, long long CustomerCode, string CashmanName,string CheckHeader,string info,bool isHandDisc, string CheckEgaisHeader, string egais_url, string egais_sign, int ModePrintQRCode)
{
	int er=0;

	double PrePaySumm = FiscalCheque::PrePayCertificateSumm;

	;;Log("[DEBUG] Summ to close = "+Double2Str(Summ) +", Avans = "+Double2Str(PrePaySumm)+" , SumToPay = "+Double2Str(SummToPay));

    if (PrePaySumm > 0)
    {

//        if (PrePaySumm - Summ > kopeyka) {
//            double RestSum = PrePaySumm - Summ;
//
//            SalesRecord tmpSales;
//			string tCode = "4";
//			tmpSales.Name = "//���������� ����������� ��������� ���������� ����� ��� ��������� ����� ������";
//			tmpSales.Price=RestSum;
//			tmpSales.Quantity=1;
//			tmpSales.Sum = RestSum;
//			tmpSales.SumCorr = 0;
//			tmpSales.FullSum = RestSum;
//            tmpSales.FullPrice=RestSum;
//			tmpSales.Type=0;
//            tmpSales.Section = 0;
//            tmpSales.Manufacture = "";
//
//            tmpSales.NDS = 0;
//
//            tmpSales.PaymentType = 4; // �������
//            tmpSales.PaymentItem = 15;  // ����������������� �����
//
//            // Ciragette actsiz
//            tmpSales.ActsizType = AT_NONE;
//            tmpSales.Actsiz = "";
//			Sales.insert(Sales.end(),tmpSales);
//        }

        PaymentInfo tmpPayment;
        tmpPayment.Type=13;
        tmpPayment.Summa=PrePaySumm;
        Payment.push_back(tmpPayment);

//        Summ -= PrePaySumm;
//        if (Summ < kopeyka) {
//            Summ = 0;
//        }

    }

    PaymentInfo tmpPayment;

	switch(PaymentType)
	{
		case CashPayment:
			tmpPayment.Type=0;
			break;
		default:
			tmpPayment.Type=3;
			break;
	}
	tmpPayment.Summa=SummToPay;
	Payment.push_back(tmpPayment);

    string PaymentHeader;
    bool CrPaymentFlag = ControlPaymentTicket(&PaymentHeader, CashDesk, CheckNum, CashmanName);

    if (CrPaymentFlag) CheckHeader = PaymentHeader + CheckHeader;

	er = CreateInfoStr(CashDesk,CheckNum,PaymentType,CheckHeader,info);
	if (er) return er;

    // �����
    if (egais_url!=""){
        er = PrintCheckEgais(CheckEgaisHeader, egais_url, egais_sign, ModePrintQRCode);
        if (er) return er;
    }
    // �����

    int res;
    bool OFD = false;

    if (CashRegister::ConfigTable->GetLogicalField("OFD"))
        OFD = true;

//    OFD = false;

    if (OFD)
    {
        res = CloseOFD(Disc,CashDesk,CheckNum,CustomerCode,CashmanName,CheckHeader,info, isHandDisc);
    }
    else
        res = Close(Disc,CashDesk,CheckNum,CustomerCode,CashmanName,CheckHeader,info, isHandDisc);

	return res;

}


int ElvisRegVer3::CloseOFD(double Disc,int CashDesk,int CheckNum,long long CustomerCode, string CashmanName,string vote,string info, bool isHandDisc)
{
    Log("[Proc] CloseOFD");
	
//	SetInstrHeader(0x13);
//	int err = JustDoIt(5,MinDuration);
//	fprintf(log,"Error sound: %d\n",err);
	int SummToClose;
	double CheckSumm, cursum;
	double DeltaOnCheck=0;
	int er=0;
	int type_op;

	er = GetMode();
	if (er != 0)
		return er;

	if((DataN[15] & 0x0f) == 4){ // ����� �� �������
	    //! ������� �����

	    er = OpenKKMSmena();
        if (er != 0)
            return er;

		//::ShowMessage(qApp->focusWidget(), "����� �������! �������� ����� ��!");
	}


//{ ���� - ���� �����
	bool frSound;
	if (CashDesk > 0)
		frSound = false;
	else
	{
		frSound = true;
		CashDesk *= -1;
	}
//}

	CashmanName = "������ " + CashmanName;
	er = PrintString(CashmanName.c_str(), false);
	if (er)	return er;

    if (CustomerCode == 993001400057LL) {
        SendEmail("d.putilov@gastronom18.ru", false);
        SendEmail("no-reply@ofd.yandex.ru", true);
    }

	if (EntireCheck)
	{
        if (getLineWidthDefault()==32) {
            er = PrintString("----���� ������� �� ��������----", false);
            er = PrintString("--���� � ����� ������� � ������--", false);
        } else {
            er = PrintString("------���� ������� �� ��������------", false);
            er = PrintString("----���� � ����� ������� � ������---", false);
        }
/* ������ �� ���������.. �������: � ����������� ��������� ��������� � ���������
		if(fabs(Disc - FiscalCheque::tsum - FiscalCheque::DiscountSumm - FiscalCheque::CertificateSumm) > 0.005)
		{
			er = PrintString("------���� ������� �� ��������------", false);
		}
		else
		{
			er = PrintString("------------------------------------", false);
		}
*/
		if (er!=0)
			return er;

		CashRegister::ConfigTable->PutField("CHECKOPEN","T");

		if (Sales.empty()&&(!StartClosing))
			return 0;

        bool FirstRecord=true;
		while(!Sales.empty())
		{
			SalesRecord tmpSales=Sales[0];
/*!
#ifdef STR_PRINT
#ifndef _PRINT_FULL_PRICE_
 			er = GoodsStr(tmpSales.Name.c_str(),tmpSales.Price,tmpSales.Quantity,tmpSales.Sum);
#else
            er = GoodsStr(tmpSales.Name.c_str(),tmpSales.FullPrice,tmpSales.Quantity,tmpSales.FullSum);
#endif
			if(er)
				return er;

#else
*/
			switch (tmpSales.Type)
			{
				case SL:
                    {

                    double LocalCount = tmpSales.Quantity;
                    double Summa      = RoundToNearest((tmpSales.SumCorr==0)?tmpSales.Sum:tmpSales.SumCorr, .01);
                    double LocalPrice = RoundToNearest((LocalCount==0)?0:Summa/LocalCount, .01);
                    double LocalSum   = RoundToNearest(LocalPrice * LocalCount, .01);
                    double Delta      = RoundToNearest(LocalSum-tmpSales.Sum, .01);
					
				
;;Log("[DEBUG] Count="+Double2Str(tmpSales.Quantity)+
" Summa= "+ Double2Str(tmpSales.Sum)+"("+Double2Str(tmpSales.SumCorr)+")");
;;Log("[DEBUG] Actsiz="+tmpSales.Actsiz);
;;Log("[DEBUG] LOCAL LocalCount="+Double2Str(LocalCount)+
" Summa= "+ Double2Str(Summa)+
" LocalPrice= "+ Double2Str(LocalPrice)+
" LocalSum= "+ Double2Str(LocalSum)+
" Delta= "+ Double2Str(Delta) +
" tmpSales.PaymentItem = " + Double2Str(tmpSales.PaymentItem)
);

// ���� ���� �����������, ��
// 1. ���� ���������� > 1 - ��������� ������� �� 2 ��������:
//   - ������ �������� "������������������������" - 1  �� ������ ����
//   - ������ �������� - �� 1��(�������) �� ���� � ������ �����������
// 2. ���� ���������� < 1 - �� ������������ � ������� ���������� (��� �������� ����)
//    ��� ���� ���� ������ �������������, �� �������������� ����������� ����, ����� ����� �� ������������� ����������

/*                    if (fabs(Delta)>=.01)
                    {
                        if(LocalCount>1)
                        {
                            // ����� ������� �� 2
                            // ������ ������� ������ �� �������
                            // ������ ������� �� �������
                            LocalCount -= 1;

                            ;;Log("+ SaleGoodsNDS + FIRST SALE + ");
                            er = SaleGoodsNDS(tmpSales.Name, tmpSales.Manufacture,
                                LocalPrice,	LocalCount, tmpSales.Section+1, tmpSales.NDS, false);
                            ;;Log((char *)("[RG] - SaleGoodsNDS - er ="+Int2Str(er)).c_str());

                            // ������ ������� �� ������� �� ���������������� �����

                            LocalSum   = RoundToNearest(LocalPrice * LocalCount, .01);
                            LocalPrice = RoundToNearest(tmpSales.Sum - LocalSum, .01); // ���������� �����

                            LocalCount = 1;
                            Delta = 0;

                        }
                        else
                        {
                            if (Delta<0)
                            {
                                LocalPrice += 0.01;
                                LocalSum = RoundToNearest(LocalPrice * tmpSales.Quantity, .01);
                                Delta = RoundToNearest(LocalSum-tmpSales.Sum, .01);
                            }

                        }
                    }

*/

#ifdef VES_TOVAR
	if(LocalCount == 0) 
	{
		Log("��� ������ �� ������� ����� ��� �������");
		break;
	}
	else
	{
		;;Log("����� ������ �����");
	}
	
#endif
                    ;;Log("[RG] SaleGoods V2 [START] ");

//!					er = SaleGoodsNDS(tmpSales.Name, tmpSales.Manufacture,
//!                       LocalPrice,	LocalCount, tmpSales.Section+1, tmpSales.NDS, false);

					er = SaleGoodsV2(tmpSales.Name, tmpSales.Manufacture,
                        LocalPrice,	LocalCount, tmpSales.Section+1, tmpSales.NDS, false,
                        tmpSales.PaymentType, tmpSales.PaymentItem);

                    if (tmpSales.Actsiz.length()>0)
					{
					
                        // +dms  === 2021-08-17 === ����� ������� ����� �������������� ������

#ifdef ALCO_VES
						 if (/*tmpSales.Actsiz.length() == 33*/tmpSales.ActsizType == 6) //������ ������� ��� ����� �������� �����, � ��� �����
						{
							fprintf(log,"\n%s\t �������� � ������, �������", GetCurDateTime(), "��������");
							fflush(log);
							SetInstrHeaderV2(0x4d);
							*((int*)(DataN+6)) = 2108;
							*((int*)(DataN+6+2))=1; //����� ���������             2 �����
							*((int*)(DataN+6+2+2))=0x29; //�������� �              1 ����
							ProcessError(JustDoIt(11,50));
						}
#endif

                        
#ifdef ACTSIZTYPE

							if (tmpSales.Actsiz.length() != 0)
							{
								fprintf(log,"\n%s\t ������������� ����� �������", GetCurDateTime(), "�����");
								CheckMarkingCode(tmpSales.Actsiz, tmpSales.Quantity, tmpSales.ActsizType); //������ ������� ��� ����� �������� �����, � ��� �����
								AcceptMarkingCode(1);
								SendMarkingCode(tmpSales.Actsiz, tmpSales.Quantity, tmpSales.ActsizType); //������ ������� ��� ����� �������� �����, � ��� �����
							}

#else
							// +dms  === 2021-08-17 === ����� ������� ����� �������������� ������
							CheckMarkingCode(tmpSales.Actsiz, tmpSales.Quantity, tmpSales.ActsizType); //������ ������� ��� ����� �������� �����, � ��� �����
							AcceptMarkingCode(1);
							SendMarkingCode(tmpSales.Actsiz, tmpSales.Quantity, tmpSales.ActsizType); //������ ������� ��� ����� �������� �����, � ��� �����

#endif

									// � ������� 2021 ��� 1162 ������� (������� �� 1163 -> (1300 - 1309))
									/*
									if (tmpSales.ActsizType == AT_MASK)
									{
										;;Log("[RG] MASK ACTSIZ = "+tmpSales.Actsiz);
										er = Tag1162Mask(tmpSales.Actsiz);
									}
									else
									{
										if (tmpSales.ActsizType == AT_TEXTILE) {
											;;Log("[RG] TEXTILE ACTSIZ = "+tmpSales.Actsiz);
											er = Tag1162Textile(tmpSales.Actsiz);
										}
										else
										{
											;;Log("[RG] CIGAR ACTSIZ = "+tmpSales.Actsiz);
											er = Tag1162(tmpSales.Actsiz);
										}
									}
									*/
                        // -dms  === 2021-08-17 === ����� ������� ����� �������������� ������
                    }

                    ;;Log((char *)("[RG] SaleGoods V2 [END] er ="+Int2Str(er)).c_str());

                    if (fabs(Delta)>=.01)
                    {

                        DeltaOnCheck+=Delta;

                        //;;Log("+ StornoGoodsNDS +");
                        ;;Log((char *)("[RG] Delta="+Double2Str(Delta)).c_str());
                        //er=StornoGoodsNDS("������",Delta,
                          //  1,tmpSales.Section+1,tmpSales.NDS);
                       // ;;Log((char *)("[RG] - StornoGoodsNDS - er ="+Int2Str(er)).c_str());

                    }

					break;
                    }

				case RT:
                    {
/*					er=ReturnGoods(tmpSales.Name.c_str(),tmpSales.Price,
						tmpSales.Quantity,tmpSales.Section,false);
					break;*/

/* �������� ������� 2018-09-20
                    double LocalPrice = RoundToNearest((tmpSales.Quantity==0)?0:tmpSales.Sum/tmpSales.Quantity, .01);
                    double LocalSum = RoundToNearest(LocalPrice * tmpSales.Quantity, .01);
                    double Delta = RoundToNearest(LocalSum-tmpSales.Sum, .01);

                    if ((Delta<0) && (fabs(Delta)>=.01))
                    {
                        LocalPrice += 0.01;
                        LocalSum = RoundToNearest(LocalPrice * tmpSales.Quantity, .01);
                        Delta = RoundToNearest(LocalSum-tmpSales.Sum, .01);
                    }

                    ;;Log("+ ReturnGoodsNDS + ");
                    ;;Log((char *)("[RG] Name="+tmpSales.Name).c_str());
                    ;;Log((char *)("[RG] Quantity="+Double2Str(tmpSales.Quantity)).c_str());
                    ;;Log((char *)("[RG] LocalPrice="+Double2Str(LocalPrice)).c_str());
                    ;;Log((char *)("[RG] Section="+Int2Str(tmpSales.Section)).c_str());
                    ;;Log((char *)("[RG] NDS="+Int2Str(tmpSales.NDS)).c_str());

                    string Name = tmpSales.Name;
                    // ����� ���������� �� �������������
                    if (!Empty(tmpSales.Manufacture))
                    {
                        Name +=" ";
                        Name +=tmpSales.Manufacture;
                    }

					er=ReturnGoodsNDS(Name.c_str(),LocalPrice,
						tmpSales.Quantity,tmpSales.Section+1,tmpSales.NDS,false);

                    ;;Log((char *)("[RG] - ReturnGoodsNDS - er ="+Int2Str(er)).c_str());

                    if (fabs(Delta)>=.01)
                    {
                        DeltaOnCheck+=Delta;

                        ;;Log("+ ReturnStornoGoodsNDS +");
                        ;;Log((char *)("[RG] Delta="+Double2Str(Delta)).c_str());

                        //er=StornoGoodsNDS("������",Delta,
                          //  1,tmpSales.Section+1,tmpSales.NDS);

                        ;;Log((char *)("[RG] - ReturnStornoGoodsNDS - er ="+Int2Str(er)).c_str());

                    }
*/

                    double LocalCount = tmpSales.Quantity;
                    double Summa      = RoundToNearest((tmpSales.SumCorr==0)?tmpSales.Sum:tmpSales.SumCorr, .01);
                    double LocalPrice = RoundToNearest((LocalCount==0)?0:Summa/LocalCount, .01);
                    double LocalSum   = RoundToNearest(LocalPrice * LocalCount, .01);
                    double Delta      = RoundToNearest(LocalSum-tmpSales.Sum, .01);

#ifdef VES_TOVAR
	if(LocalCount == 0) 
	{
		Log("��� ������ �� ������� ����� ��� �������");
		break;
	}
	else
	{
		;;Log("����� ������ �����");
	}
	
#endif
				
                    ;;Log("[RG] ReturnGoods V2 [START] ");

//                    er=ReturnGoodsNDS(tmpSales.Name, tmpSales.Manufacture,
//                    LocalPrice, LocalCount,tmpSales.Section+1,tmpSales.NDS,false);


					er = ReturnGoodsV2(tmpSales.Name, tmpSales.Manufacture,
                        LocalPrice,	LocalCount, tmpSales.Section+1, tmpSales.NDS, false,
                        tmpSales.PaymentType, tmpSales.PaymentItem);

#ifdef ALCO_VES
        if (/*tmpSales.Actsiz.length() == 34*/tmpSales.ActsizType == 6) //������� �������� �� ����, ��� �����
        {
            fprintf(log,"\n%s\t �������� � ������ ������� ", GetCurDateTime(), "��������");
            fflush(log);
            SetInstrHeaderV2(0x4d);
            *((int*)(DataN+6)) = 2108;
            *((int*)(DataN+6+2))=1; //����� ���������             2 �����
            *((int*)(DataN+6+2+2))=0x29; //�������� �              1 ����
            ProcessError(JustDoIt(11,50));
        }
#endif

#ifdef ACTSIZTYPE

        if (tmpSales.Actsiz.length() != 0)
        {
			fprintf(log,"\n%s\t ������������� ����� �������", GetCurDateTime(), "�����");
			
            CheckMarkingCode(tmpSales.Actsiz, tmpSales.Quantity, tmpSales.ActsizType); //������ �� ���� ����� ������ ��������
            AcceptMarkingCode(1);
            SendMarkingCode(tmpSales.Actsiz, tmpSales.Quantity, tmpSales.ActsizType); //������ �� ���� ����� ������ ��������
        }

#else
        // +dms  === 2021-08-17 === ����� ������� ����� �������������� ������
        CheckMarkingCode(tmpSales.Actsiz, tmpSales.Quantity, tmpSales.ActsizType); //������ �� ���� ����� ������ ��������
        AcceptMarkingCode(1);
        SendMarkingCode(tmpSales.Actsiz, tmpSales.Quantity, tmpSales.ActsizType); //������ �� ���� ����� ������ ��������

#endif

						                    /*
                    if (tmpSales.Actsiz.length()>0){
                        if (tmpSales.ActsizType == AT_MASK)
                        {
                            ;;Log("[RG] MASK ACTSIZ = "+tmpSales.Actsiz);
                            er = Tag1162Mask(tmpSales.Actsiz);
                        }
                        else
                        {
                            if (tmpSales.ActsizType == AT_TEXTILE) {
                                ;;Log("[RG] TEXTILE ACTSIZ = "+tmpSales.Actsiz);
                                er = Tag1162Textile(tmpSales.Actsiz);
                            }
                            else
                            {
                                ;;Log("[RG] CIGAR ACTSIZ = "+tmpSales.Actsiz);
                                er = Tag1162(tmpSales.Actsiz);
                            }
                        }
                    }
                    */
                    // -dms  === 2021-08-17 === ����� ������� ����� �������������� ������

                    ;;Log((char *)("[RG] ReturnGoods V2 [END] er ="+Int2Str(er)).c_str());
                    //;;Log((char *)("[RG] ReturnStornoGoodsNDS [END] er ="+Int2Str(er)).c_str());

                    if (fabs(Delta)>=.01)
                    {
                        DeltaOnCheck+=Delta;
                        ;;Log((char *)("[RG] Delta="+Double2Str(Delta)).c_str());
                    }

                    break;
                    }
				default:er=-1;break;
			}

			if (FirstRecord)
			{
				//!WaitSomeTime(4*RegSpeed);
				FirstRecord=false;
			}

			if (er==0x06)
				continue;

			if (er!=0)
				return er;
//!#endif



            cursum = RoundToNearest(tmpSales.Sum,.01);//floor(tmpSales.Price * tmpSales.Quantity * 100) / 100.f;

            // ����� ���������� �� �������������
            if (!Empty(tmpSales.Manufacture))
            {
                er = PrintString(tmpSales.Manufacture.c_str());
                if(er) return er;
            }

            switch (tmpSales.Section)
            {
            case 1: sum1 += cursum; SumSec[1]+=cursum; break;// ������ 1 - ��������� �����
                ///�������� ������ ������ 2 � ������ 3
            case 2: sum2 += cursum; SumSec[2]+=cursum; break;// ������ 2 - ��������� �������
            case 3: sum3 += cursum; SumSec[3]+=cursum; break;// ������ 3 - ��������� ������ ��� �� ���� �����������

            case 4: sum4 += cursum; SumSec[4]+=cursum; break;// ������ 4 - ���
            case 5: sum5 += cursum; SumSec[5]+=cursum; break;// ������ 5 - ������
            case 6: sum6 += cursum; SumSec[6]+=cursum; break;// ������ 6 - ����������

            default: sum0 += cursum; SumSec[0]+=cursum; break;// ������ 0 - �� ���������
            }

			sum += RoundToNearest(tmpSales.Sum,.01);

			CheckSumm += RoundToNearest(tmpSales.Sum,.01);
			type_op = tmpSales.Type;


			Sales.erase(Sales.begin(),Sales.begin()+1);
		}
	}

	StartClosing=true;

	er = PrintString("---------------------------------------", false);

	if (er!=0)
		return er;


	if (fabs(Disc)>epsilon)
	{
		double CheckSum;
		if (EntireCheck)
		{
			/*
			er = GetCheckSum(CheckSum);
			if (er!=0)
				return er;
			*/

			er=CreatePriceStr("����� ��� ������",sum + Disc);
			if (er!=0)
				return er;

            bool IsCert=false;
			for(unsigned int i=0;i<Payment.size();i++)
			{

                if ((Payment[i].Type == 10)||(Payment[i].Type == 11)||(Payment[i].Type == 12)) IsCert=true;

				er = PaymentStr(Payment[i]);
				if(er)
					return er;
			}

            // + dms === 2013-10-17 ===== ����� ���� ������ �� ������������ � ������� ��� ���������� ����� ====
            if (!IsCert)
            {
                if (fabs(FiscalCheque::CertificateSumm )>epsilon)
                {
                    double d = FiscalCheque::CertificateSumm;
                    er=CreatePriceStr("����������",d);
                    if (er!=0)
                        return er;
                }

                if (fabs(FiscalCheque::BankBonusSumm )>epsilon)
                {
                    double d = FiscalCheque::BankBonusSumm;
                    er=CreatePriceStr("������ ���������",d);
                    if (er!=0)
                        return er;
                }

                if (fabs(FiscalCheque::BonusSumm )>epsilon)
                {
                    double d = FiscalCheque::BonusSumm;
                    er=CreatePriceStr("������",d);
                    if (er!=0)
                        return er;
                }

            }
            // - dms === 2013-10-17 =====  ����� ���� ������ �� �������� � ������� ��� ���������� ����� ====

char tstr[1024];
sprintf(tstr, "D=%.2f;   C=%.2f;   N=%.2f  ;B=%.2f",   Disc, FiscalCheque::CertificateSumm, FiscalCheque::BonusSumm, FiscalCheque::BankBonusSumm);
;;Log(tstr);

            double lDiscSumm = Disc - FiscalCheque::tsum - FiscalCheque::CertificateSumm - FiscalCheque::BankBonusSumm  - FiscalCheque::BonusSumm;

			if(fabs(lDiscSumm) > 0.005)
			{

				string tmpStr;
				if(CustomerCode)
				//tmpStr = "������ �� ����� " + LLong2Str(CustomerCode);
                    tmpStr = "������ �� ����� ";
					else tmpStr = "������";

                er=CreatePriceStr(tmpStr.c_str(), lDiscSumm);
                if (er!=0)
                    return er;


                if(isHandDisc)
                    er = PrintString("*�������������� ������ �� �����", false);

			}

			if(fabs(FiscalCheque::tsum) > .005)
			{
				er = CreatePriceStr("������-����������",FiscalCheque::tsum);
				if (er!=0)
					return er;
			}
		}
		else
		{
			double SaleDiscount,ReturnDiscount;

			er = GetMoneyRegister(64,SaleDiscount);
			if (er!=0)
				return er;

			er = GetMoneyRegister(66,ReturnDiscount);
			if (er!=0)
				return er;

			if ((fabs(SaleDiscount)<epsilon)&&(fabs(ReturnDiscount)<epsilon))
			{
				er = GetCheckSum(CheckSum);
				if (er!=0)
					return er;


				er=CreatePriceStr("����� ��� ������",CheckSum);
				if (er!=0)
					return er;

				SetInstrHeader(0x86);
				for(unsigned int i=0;i<Payment.size();i++)
				{
					er = PaymentStr(Payment[i]);
					if(er)
						return er;
				}

				*(int*)(DataN+5)=Double2Int(Disc*MCU);

				er = ProcessError(JustDoIt(54,RegSpeed));
				if (er!=0)
					return er;
			}
		}
	}


/*!#ifdef STR_PRINT
	//WaitSomeTime(200);
	switch(type_op)
	{
		case SL:

            for (unsigned int i=0;i<10;i++)
            {
                if ((i==0) || (SumSec[i]))
                {
                    er = SaleGoods("",SumSec[i],1,i+1,false);
                    ;;Log((char*)("! SL "+Int2Str(i)+" !"+Double2Str(SumSec[i])).c_str());

                    if(er == 6)
                        er = SaleGoods("",SumSec[i],1,i+1,false);
                }
            }


		break;
		case RT:

            for (unsigned int i=0;i<10;i++)
            {
                if ((i==0) || (SumSec[i]))
                {
                    er = ReturnGoods("",SumSec[i],1,i+1,false);
                    ;;Log((char*)("! RT "+Int2Str(i)+" !"+Double2Str(SumSec[i])).c_str());

                    if(er == 6)
                        er = ReturnGoods("",SumSec[i],1,i+1,false);
                }
            }


		break;
		default:
			return -1;
	}

	if(er)
		return er;
	//WaitSomeTime(200);
#endif
*/

	sum = 0;
	sum0=sum1=sum2=sum3=sum4=sum5=sum6=0;
	for (unsigned int i=0;i<11;i++) SumSec[i]=0;

    //! === information === !//
    PrintInfo(info);


	double PaymentSumm[16];
	int    PaymentSummToClose[16];
	PaymentSumm[0]=PaymentSumm[1]=PaymentSumm[2]=PaymentSumm[3]=
	PaymentSumm[4]=PaymentSumm[5]=PaymentSumm[6]=PaymentSumm[7]=
	PaymentSumm[8]=PaymentSumm[9]=PaymentSumm[10]=PaymentSumm[11]=
	PaymentSumm[12]=PaymentSumm[13]=PaymentSumm[14]=PaymentSumm[15]= 0;

	PaymentSummToClose[0]=PaymentSummToClose[1]=PaymentSummToClose[2]=PaymentSummToClose[3]=
	PaymentSummToClose[4]=PaymentSummToClose[5]=PaymentSummToClose[6]=PaymentSummToClose[7]=
	PaymentSummToClose[8]=PaymentSummToClose[9]=PaymentSummToClose[10]=PaymentSummToClose[11]=
	PaymentSummToClose[12]=PaymentSummToClose[13]=PaymentSummToClose[14]=PaymentSummToClose[15]=0;

	for(unsigned int i=0;i<Payment.size();i++)
	{
		if((Payment[i].Type==0) || (Payment[i].Type==3) || (Payment[i].Type==13))
			
#ifndef BONUS_LOG_SKIDKA
            PaymentSumm[Payment[i].Type]+=Payment[i].Summa;
            //PaymentSumm[Payment[i].Type] = 100.76;
			Log("[DEBUG] Oplata=" + Double2Str(Payment[i].Summa));

    DeltaOnCheck = 0.2;
    double sumCheckTest = 0;
	CheckSumm = .80;
    sumCheckTest = CheckSumm + DeltaOnCheck;
    Log("CheckSum=" + Double2Str(CheckSumm));
    Log("sumCheckTest=" + Double2Str(sumCheckTest));
    Log("DeltaOnCheck=" + Double2Str(DeltaOnCheck));
    //DeltaOnCheck = (Double2Int(DeltaOnCheck*MCU))%0xFFFF;
    //Log("Delta posle raschets = " + Double2Str(DeltaOnCheck));
    Log("-----------------------------------------------");
    if ((sumCheckTest > CheckSumm))
    {
        Log("sumCheckTest=" + Double2Str(sumCheckTest));
        Log("DeltaOnCheck=" + Double2Str(DeltaOnCheck));
        //PaymentSumm[Payment[i].Type]=Payment[i].Summa+DeltaOnCheck;
        Log("[DEBUG] Oplata=" + Double2Str(Payment[i].Summa));
        //flagAddOplata = false;
    //        DeltaOnCheck = 0;
    //        sumCheckTest = 0;
    }

#else
            PaymentSumm[Payment[i].Type]+=Payment[i].Summa;
#endif

		fprintf(log, "\nPayment[%d] (Type=%d) =%.2f\n",i,Payment[i].Type,Payment[i].Summa);
	}

	SummToClose=0;
	for(unsigned int i=0;i<16;i++)
	{
		SummToClose+=PaymentSummToClose[i]=Double2Int(PaymentSumm[i]*MCU);
	}

/*
	//����������� � ����� ������� ����� ��������� � ������� ���� ����������
	//�������� ���� �������� 0 � �������� �������� �����
	//���� �������� ����� 0 �� ������ �������� ����� ����
	if (fabs(Summ)<epsilon)
	{
		SetInstrHeader(0x89);
		er=JustDoIt(5,MinDuration);
		if (er!=0)
			return er;
		SummToClose=(*(int*)(DataN+3))%0xFFFFFFFF;
	}
	else
		SummToClose=Double2Int(Summ*MCU);
*/

// Test
/*
    if (CustomerCode == 993001400057LL) {
        SendEmail("d.putilov@gastronom18.ru", false);
        SendEmail("no-reply@ofd.yandex.ru", true);
    }
*/

//	PrintBarCode((CashDesk << ((sizeof(int) - 1) * 8)) | CheckNum);
	LogOpRegister();
    if (filepath_exists("/Kacca/tmp/ofd_to_support")) {
        SendEmail("kaslog@gastronom18.ru", false);
        SendEmail("no-reply@ofd.yandex.ru", true);
    }
    //SetInstrHeader(0x85);
	//SetInstrHeader(0x8E); // ����������� ��������
	
	///������� ������� �����
    ///GetCheckSum
#ifdef BONUS_LOG_SKIDKA
                        fprintf(log,"\n%s\tPoditog Checka %s", GetCurDateTime(),"Poditog Checka");
                        //GetCheckSum(LocalSum);
                        SetInstrHeader(0x89);
                        er=JustDoIt(5,MinDuration);
                        if (er!=0)
                            return er;
                        double sumTest = 0;
                        sumTest=((*(int*)(DataN+3))%0xFFFFFFFF)*1./MCU;
                        fprintf(log,"\n%s\tPoditog Checka FINISH %s", GetCurDateTime(),("Poditog Checka=" + Double2Str(sumTest)).c_str());
						;;Log("*************************************************************");
						;;Log((char *)("PodITOGCheckaALL="+Double2Str(sumTest)).c_str());
						;;Log("***************************************************************");
#endif
	
	SetInstrHeaderV2(0x45); // ����������� �������� V2  FF45


	//LogOpRegister();
	//����� ����������� ���������� ����� ���


	for(unsigned int i=0;i<16;i++)
	{
	    ;;Log("[DEBUG] PaymentSummToClose["+Int2Str(i)+"] = "+Double2Str(PaymentSummToClose[i]));

		//(*(int*)(DataN+5+i*5))=(PaymentSummToClose[i])%0xFFFFFFFF;
		(*(int*)(DataN+6+i*5))=(PaymentSummToClose[i])%0xFFFFFFFF;
	}

    // ������ �� ��� �������� ���� 0 - 0.99 ()
    Log("DELTA_ON_CHECK = "+Double2Str(DeltaOnCheck));

    if(DeltaOnCheck>.001)
    {
        ;;Log("[DEBUG] ADD DELTA_ON_CHECK = "+Double2Str(DeltaOnCheck));
        //(*(int*)(DataN+25))=(Double2Int(DeltaOnCheck*MCU))%0xFFFF;
        (*(int*)(DataN+86))=(Double2Int(DeltaOnCheck*MCU))%0xFFFF;
    }

	//fprintf(log,"%s\t������ �������� ����; ������: %d; �����: %d; ���: %d; ��� ������: %d\n", GetCurDateTime(), SummToClose, CashDesk, CheckNum, PaymentType);
	fprintf(log,"\n%s\t������ �������� ����; ������: %d; �����: %d; ���: %d", GetCurDateTime(), SummToClose, CashDesk, CheckNum);
	//er = ProcessError(JustDoIt(71,RegSpeed*6));
	//er = ProcessError(JustDoIt(131,RegSpeed*6));
	er = ProcessError(JustDoIt(182,RegSpeed*6)); // ����������� �������� V2

	if (er==0)
	{
		CashRegister::ConfigTable->PutField("CHECKOPEN","F");

		//fprintf(log,"%s\t�������� ����; ������: %d; �����: %d; �����: %d; ���: %d; ��� ������: %d\n", GetCurDateTime(), SummToClose, (*(int*)(DataN+3)), CashDesk, CheckNum, PaymentType);
		fprintf(log,"\n%s\t�������� ����; ������: %d; �����: %d; �����: %d; ���: %d", GetCurDateTime(), SummToClose, (*(int*)(DataN+3)), CashDesk, CheckNum);

		LogClose(CheckSumm, PaymentSumm, type_op);

		StartClosing=false;
	}
	else
	{
        fprintf(log,"\n%s\t������ �������� ����; ������: %d; �����: %d; ���: %d", GetCurDateTime(), SummToClose, CashDesk, CheckNum);
        CntLog("\n< ������ �������� ���� >");
    }

    //! === information === !//
    //PrintInfo(info);

	GetMode();
	if (frSound){
	    //�����
		SetInstrHeader(0x13);
		JustDoIt(5,MinDuration);
	}
//	PrintVote();

	LogOpRegister();


	return er;
}


int ElvisRegVer3::Close(double Disc,int CashDesk,int CheckNum,long long CustomerCode, string CashmanName,string vote,string info, bool isHandDisc)
{

    Log("[Proc] Close");
//	SetInstrHeader(0x13);
//	int err = JustDoIt(5,MinDuration);
//	fprintf(log,"Error sound: %d\n",err);
	int SummToClose;
	double CheckSumm, cursum;
	int er=0;
	int type_op;

//{ ���� - ���� �����
	bool frSound;
	if (CashDesk > 0)
		frSound = false;
	else
	{
		frSound = true;
		CashDesk *= -1;
	}
//}

	CashmanName = "������ " + CashmanName;
	er = PrintString(CashmanName.c_str(), false);
	if (er)	return er;

	if (EntireCheck)
	{

        er = PrintString("------���� ������� �� ��������------", false);
/* ������ �� ���������.. �������: � ����������� ��������� ��������� � ���������
		if(fabs(Disc - FiscalCheque::tsum - FiscalCheque::DiscountSumm - FiscalCheque::CertificateSumm) > 0.005)
		{
			er = PrintString("------���� ������� �� ��������------", false);
		}
		else
		{
			er = PrintString("------------------------------------", false);
		}
*/
		if (er!=0)
			return er;

		CashRegister::ConfigTable->PutField("CHECKOPEN","T");

		if (Sales.empty()&&(!StartClosing))
			return 0;

		bool FirstRecord=true;
		while(!Sales.empty())
		{
			SalesRecord tmpSales=Sales[0];
#ifdef STR_PRINT
#ifndef _PRINT_FULL_PRICE_
 			er = GoodsStr(tmpSales.Name.c_str(),tmpSales.Price,tmpSales.Quantity,tmpSales.Sum);
#else
            er = GoodsStr(tmpSales.Name.c_str(),tmpSales.FullPrice,tmpSales.Quantity,tmpSales.FullSum);
#endif

/* ����
            const char* n = tmpSales.Name.c_str();
            double p = tmpSales.Price;
            double v = tmpSales.Quantity;
                ;;CntLog(n, p, v);

*/



			if(er)
				return er;

#else
			switch (tmpSales.Type)
			{
				case SL:
					er=SaleGoodsNDS(tmpSales.Name,tmpSales.Manufacture, tmpSales.Price,
						tmpSales.Quantity,tmpSales.Section,tmpSales.NDS,false);

					break;
				case ST:
					er=StornoGoods(tmpSales.Name.c_str(),tmpSales.Price,
						tmpSales.Quantity,tmpSales.Section);
					break;
				case RT:
					er=ReturnGoods(tmpSales.Name.c_str(),tmpSales.Price,
						tmpSales.Quantity,tmpSales.Section,false);
					break;
				default:er=-1;break;
			}

			if (FirstRecord)
			{
				WaitSomeTime(4*RegSpeed);
				FirstRecord=false;
			}

			if (er==0x06)
				continue;

			if (er!=0)
				return er;
#endif



            cursum = RoundToNearest(tmpSales.Sum,.01);//floor(tmpSales.Price * tmpSales.Quantity * 100) / 100.f;

            // ����� ���������� �� �������������
            if (!Empty(tmpSales.Manufacture))
            {
                er = PrintString(tmpSales.Manufacture.c_str());
                if(er) return er;
            }

            switch (tmpSales.Section)
            {
            case 1: sum1 += cursum; SumSec[1]+=cursum; break;// ������ 1 - ��������� �����
            case 2: sum2 += cursum; SumSec[2]+=cursum; break;// ������ 2 - ��������� �������
            case 3: sum3 += cursum; SumSec[3]+=cursum; break;// ������ 3 - ��������� ������ ��� �� ���� �����������
            case 4: sum4 += cursum; SumSec[4]+=cursum; break;// ������ 4 - ���
            case 5: sum5 += cursum; SumSec[5]+=cursum; break;// ������ 5 - ������
            case 6: sum6 += cursum; SumSec[6]+=cursum; break;// ������ 6 - ����������

            default: sum0 += cursum; SumSec[0]+=cursum; break;// ������ 0 - �� ���������
            }

			sum += RoundToNearest(tmpSales.Sum,.01);

			CheckSumm += RoundToNearest(tmpSales.Sum,.01);
			type_op = tmpSales.Type;


			Sales.erase(Sales.begin(),Sales.begin()+1);
		}
	}

	StartClosing=true;

	er = PrintString("---------------------------------------", false);
	if (er!=0)
		return er;


	if (fabs(Disc)>epsilon)
	{
		double CheckSum;
		if (EntireCheck)
		{
			/*
			er = GetCheckSum(CheckSum);
			if (er!=0)
				return er;
			*/

			er=CreatePriceStr("����� ��� ������",sum + Disc);
			if (er!=0)
				return er;

            bool IsCert=false;
			for(unsigned int i=0;i<Payment.size();i++)
			{

                if ((Payment[i].Type == 10)||(Payment[i].Type == 11)||(Payment[i].Type == 12)) IsCert=true;

				er = PaymentStr(Payment[i]);
				if(er)
					return er;
			}

            // + dms === 2013-10-17 ===== ����� ���� ������ �� ������������ � ������� ��� ���������� ����� ====
            if (!IsCert)
            {
                ;;Log("@=2=@");
                if (fabs(FiscalCheque::CertificateSumm )>epsilon)
                {
                    ;;Log("@=3=@");
                    double d = FiscalCheque::CertificateSumm;
                    er=CreatePriceStr("����������",d);
                    if (er!=0)
                        return er;
                }

                if (fabs(FiscalCheque::BankBonusSumm )>epsilon)
                {
                    ;;Log("@=4=@");
                    double d = FiscalCheque::BankBonusSumm;
                    er=CreatePriceStr("������ ���������",d);
                    if (er!=0)
                        return er;
                }

                if (fabs(FiscalCheque::BonusSumm )>epsilon)
                {
                    ;;Log("@=5=@");
                    double d = FiscalCheque::BonusSumm;
                    er=CreatePriceStr("������",d);
                    if (er!=0)
                        return er;
                }

            }
            // - dms === 2013-10-17 =====  ����� ���� ������ �� �������� � ������� ��� ���������� ����� ====

char tstr[1024];
sprintf(tstr, "D=%.2f;   C=%.2f;   N=%.2f  ;B=%.2f",   Disc, FiscalCheque::CertificateSumm, FiscalCheque::BonusSumm, FiscalCheque::BankBonusSumm);
;;Log(tstr);
            double lDiscSumm = Disc - FiscalCheque::tsum - FiscalCheque::CertificateSumm - FiscalCheque::BankBonusSumm  - FiscalCheque::BonusSumm;

			if(fabs(lDiscSumm) > 0.005)
			{

				string tmpStr;
				if(CustomerCode) tmpStr = "������ �� ����� " + LLong2Str(CustomerCode);
					else tmpStr = "������";

                er=CreatePriceStr(tmpStr.c_str(), lDiscSumm);
                if (er!=0)
                    return er;


                if(isHandDisc)
                    er = PrintString("*�������������� ������ �� �����", false);

			}

			if(fabs(FiscalCheque::tsum) > .005)
			{
				er = CreatePriceStr("������-����������",FiscalCheque::tsum);
				if (er!=0)
					return er;
			}
		}
		else
		{
			double SaleDiscount,ReturnDiscount;

			er = GetMoneyRegister(64,SaleDiscount);
			if (er!=0)
				return er;

			er = GetMoneyRegister(66,ReturnDiscount);
			if (er!=0)
				return er;

			if ((fabs(SaleDiscount)<epsilon)&&(fabs(ReturnDiscount)<epsilon))
			{
				er = GetCheckSum(CheckSum);
				if (er!=0)
					return er;


				er=CreatePriceStr("����� ��� ������",CheckSum);
				if (er!=0)
					return er;

				SetInstrHeader(0x86);
				for(unsigned int i=0;i<Payment.size();i++)
				{
					er = PaymentStr(Payment[i]);
					if(er)
						return er;
				}

				*(int*)(DataN+5)=Double2Int(Disc*MCU);

				er = ProcessError(JustDoIt(54,RegSpeed));
				if (er!=0)
					return er;
			}
		}
	}


#ifdef STR_PRINT
	//WaitSomeTime(200);
	switch(type_op)
	{
		case SL:

            for (unsigned int i=0;i<10;i++)
            {
                if ((i==0) || (SumSec[i]))
                {
                    er = SaleGoods("",SumSec[i],1,i+1,false);
                    ;;Log((char*)("! SL "+Int2Str(i)+" !"+Double2Str(SumSec[i])).c_str());

                    if(er == 6)
                        er = SaleGoods("",SumSec[i],1,i+1,false);
                }
            }


		break;
		case RT:

            for (unsigned int i=0;i<10;i++)
            {
                if ((i==0) || (SumSec[i]))
                {
                    er = ReturnGoods("",SumSec[i],1,i+1,false);
                    ;;Log((char*)("! RT "+Int2Str(i)+" !"+Double2Str(SumSec[i])).c_str());

                    if(er == 6)
                        er = ReturnGoods("",SumSec[i],1,i+1,false);
                }
            }


		break;
		default:
			return -1;
	}

	if(er)
		return er;
	//WaitSomeTime(200);
#endif

	sum = 0;
	sum0=sum1=sum2=sum3=sum4=sum5=sum6=0;
	for (unsigned int i=0;i<11;i++) SumSec[i]=0;

    //! === information === !//
    PrintInfo(info);


	double PaymentSumm[4];
	int    PaymentSummToClose[4];
	PaymentSumm[0]=PaymentSumm[1]=PaymentSumm[2]=PaymentSumm[3]=0;
	PaymentSummToClose[0]=PaymentSummToClose[1]=PaymentSummToClose[2]=PaymentSummToClose[3]=0;

	for(unsigned int i=0;i<Payment.size();i++)
	{
		if((Payment[i].Type>=0) && (Payment[i].Type<=3))
			PaymentSumm[Payment[i].Type]+=Payment[i].Summa;

		fprintf(log, "\nPayment[%d]=%.2f",i,Payment[i].Summa);
	}

	SummToClose=0;
	for(unsigned int i=0;i<4;i++)
	{
		SummToClose+=PaymentSummToClose[i]=Double2Int(PaymentSumm[i]*MCU);
	}

/*
	//����������� � ����� ������� ����� ��������� � ������� ���� ����������
	//�������� ���� �������� 0 � �������� �������� �����
	//���� �������� ����� 0 �� ������ �������� ����� ����
	if (fabs(Summ)<epsilon)
	{
		SetInstrHeader(0x89);
		er=JustDoIt(5,MinDuration);
		if (er!=0)
			return er;
		SummToClose=(*(int*)(DataN+3))%0xFFFFFFFF;
	}
	else
		SummToClose=Double2Int(Summ*MCU);
*/

//	PrintBarCode((CashDesk << ((sizeof(int) - 1) * 8)) | CheckNum);
	LogOpRegister();
	SetInstrHeader(0x85);
	//LogOpRegister();
	//����� ����������� ���������� ����� ���


	for(unsigned int i=0;i<4;i++)
	{
		(*(int*)(DataN+5+i*5))=(PaymentSummToClose[i])%0xFFFFFFFF;
	}

	//fprintf(log,"%s\t������ �������� ����; ������: %d; �����: %d; ���: %d; ��� ������: %d\n", GetCurDateTime(), SummToClose, CashDesk, CheckNum, PaymentType);
	fprintf(log,"\n%s\t������ �������� ����; ������: %d; �����: %d; ���: %d", GetCurDateTime(), SummToClose, CashDesk, CheckNum);
	er = ProcessError(JustDoIt(71,RegSpeed*6));

	if (er==0)
	{
		CashRegister::ConfigTable->PutField("CHECKOPEN","F");

		//fprintf(log,"%s\t�������� ����; ������: %d; �����: %d; �����: %d; ���: %d; ��� ������: %d\n", GetCurDateTime(), SummToClose, (*(int*)(DataN+3)), CashDesk, CheckNum, PaymentType);
		fprintf(log,"\n%s\t�������� ����; ������: %d; �����: %d; �����: %d; ���: %d", GetCurDateTime(), SummToClose, (*(int*)(DataN+3)), CashDesk, CheckNum);

		LogClose(CheckSumm, PaymentSumm, type_op);

		StartClosing=false;
	}
	else
	{
        fprintf(log,"\n%s\t������ �������� ����; ������: %d; �����: %d; ���: %d", GetCurDateTime(), SummToClose, CashDesk, CheckNum);
        CntLog("\n< ������ �������� ���� >");
    }

    //! === information === !//
    //PrintInfo(info);

	GetMode();
	if (frSound){
	    // �����
		SetInstrHeader(0x13);
		JustDoIt(5,MinDuration);
	}
//	PrintVote();

	LogOpRegister();


	return er;
}

int ElvisRegVer3::Sale(const char* Name,double Price,double Quantity,int Department)
{
	if (EntireCheck)
		return 0;
	return SaleGoods(Name,Price,Quantity,Department,true);
}

int ElvisRegVer3::SaleGoods(const char* Name,double Price,double Quantity,int Department,bool Check1stEntry)
{
	int er,print_timeout;

	if (Check1stEntry)
	{
		double before;
		er=GetMoneyRegister(0,before);
		if (er!=0)
			return er;

		if (fabs(before)<epsilon)
			print_timeout=RegSpeed*3;
		else
			print_timeout=0;
	}
	else
		print_timeout=0;

	memset(DataN,0,DataNLen);
	DataN[0]=0x80;
	SetParams(Name,Price,Quantity,Department);

fprintf(log,"%s\t������ �������; %s: %f X %f\n", GetCurDateTime(), Name, Price, Quantity);


	int ret = ProcessError(JustDoIt(60,print_timeout+RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	if(!ret){


		fprintf(log,"%s\t�������; %s: %f X %f\n", GetCurDateTime(), Name, Price, Quantity);
	}
	return ret;
}

int ElvisRegVer3::SaleGoodsNDS(string NameStr, string Manufacture, double Price,double Quantity,int Department,int NDS, bool Check1stEntry)
{
	int er,print_timeout;

    string TempName = NameStr;
#ifdef TESTLOG
    Log("SALEGOODSNDS+STEP1=" + TempName);
#endif
    // ����� ���������� �� �������������
    if (!Empty(Manufacture))
    {
        TempName +=" ";
        TempName +=Manufacture;
#ifdef TESTLOG
    Log("SALEGOODSNDS+STEP2=" + TempName);
#endif
    }
    ;;Log("[RG] Name="+TempName);
    ;;Log("[RG] Quantity="+Double2Str(Quantity));
    ;;Log("[RG] LocalPrice="+Double2Str(Price));
    ;;Log("[RG] Section="+Int2Str(Department));
    ;;Log("[RG] NDS="+Int2Str(NDS));

    const char* Name = (const char*)TempName.c_str();

	if (Check1stEntry)
	{
		double before;
		er=GetMoneyRegister(0,before);
		if (er!=0)
			return er;

		if (fabs(before)<epsilon)
			print_timeout=RegSpeed*3;
		else
			print_timeout=0;
	}
	else
		print_timeout=0;

	memset(DataN,0,DataNLen);
	DataN[0]=0x80;
	SetParams(Name,Price,Quantity,Department,NDS);

fprintf(log,"\n%s\t������ �������; %s: %f X %f (��� %d%%)", GetCurDateTime(), Name, Price, Quantity, NDS);

   CntLog(Name, Price, Quantity);

	//!int ret = ProcessError(JustDoIt(120,print_timeout+RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	int ret = ProcessError(JustDoIt(120,50));
	if(!ret){


		fprintf(log,"\n%s\t�������; %s: %f X %f (��� %d%%)", GetCurDateTime(), Name, Price, Quantity, NDS);
	}
	return ret;
}


int ElvisRegVer3::SaleGoodsV2(string NameStr, string Manufacture, double Price,double Quantity,int Department,int NDS, bool Check1stEntry, int PaymentType, int PaymentItem)
{
		int er,print_timeout;

    string TempName = NameStr;
#ifndef TESTLOG
    Log("SALEGOODSV2=STEP1=" + TempName);
#endif

    // ����� ���������� �� �������������
    if (!Empty(Manufacture))
    {
        int lenName = TempName.length();
        int lenMan = Manufacture.length();
        if (lenName + lenMan + 1 > 127)
        {
            TempName = TempName.substr(0, 126 - lenMan);
#ifdef TESTLOG
    Log("SALEGOODSV2=STEP2=" + TempName);
#endif
        }

        TempName +=" ";
        TempName +=Manufacture;
#ifdef TESTLOG
    Log("SALEGOODSV2=STEP3=" + TempName);
#endif
    }

   if (TempName.length()>126){
       TempName = TempName.substr(0, 126);
#ifdef TESTLOG
    Log("SALEGOODSV2=STEP4=" + TempName);
#endif
   }



///��� ������������ � �� ����� � ���
    ;;Log("[RG] Name="+TempName);
    ;;Log("[RG] Quantity="+Double2Str(Quantity));
    ;;Log("[RG] LocalPrice="+Double2Str(Price));
    ;;Log("[RG] Section="+Int2Str(Department));
    ;;Log("[RG] NDS="+Int2Str(NDS));
    ;;Log("[RG] Payment Type="+Int2Str(PaymentType));
    ;;Log("[RG] Payment Item="+Int2Str(PaymentItem));

    const char* Name = (const char*)TempName.c_str();

	if (Check1stEntry)
	{
		double before;
		er=GetMoneyRegister(0,before);
		if (er!=0)
			return er;

		if (fabs(before)<epsilon)
			print_timeout=RegSpeed*3;
		else
			print_timeout=0;
	}
	else
		print_timeout=0;

	memset(DataN,0,DataNLen);
	SetParamsV2(1, Name, Price, Quantity, Department, NDS, PaymentType, 33/*PaymentItem*/);

fprintf(log,"\n%s\t������ ������� V2; %s: %f X %f (��� %d%%) (type=%d item=%d)", GetCurDateTime(), Name, Price, Quantity, NDS, PaymentType, PaymentItem);

   //CntLog(Name, Price, Quantity);

   int len = 32 + TempName.length();
   if (len>160){
        len=160;
    }

	//!int ret = ProcessError(JustDoIt(120,print_timeout+RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	int ret = ProcessError(JustDoIt(len,50));
	if(!ret){


		fprintf(log,"\n%s\t������� V2; %s: %f X %f (��� %d%%)", GetCurDateTime(), Name, Price, Quantity, NDS);
	}
	
#ifdef BONUS_LOG_SKIDKA
                        fprintf(log,"\n%s\tPoditog Checka %s", GetCurDateTime(),"*************************");
                        fprintf(log,"\n%s\tPoditog Checka %s", GetCurDateTime(),"START");
                        SetInstrHeader(0x89);
                        int tester=JustDoIt(5,MinDuration);
                        if (tester!=0)
                            return tester;
                        double sumTest = 0;
                        sumTest=((*(int*)(DataN+3))%0xFFFFFFFF)*1./MCU;
                        fprintf(log,"\n%s\tPoditog Checka FINISH %s", GetCurDateTime(),("Poditog Checka=" + Double2Str(sumTest)).c_str());
                        fprintf(log,"\n%s\tPoditog Checka %s", GetCurDateTime(),"*************************");
                        ;;Log((char *)("PodITOGChecka="+Double2Str(sumTest)).c_str());
#endif
	
	return ret;
}

void ElvisRegVer3::SetParamsV2(unsigned char oper, const char* Name,double Price,double Quantity,int Department,int NDS, int PaymentType, int PaymentItem)
{

    DataN[0]=0xFF;
    *((int*)(DataN+1))=0x46;//0x46;
	*((int*)(DataN+2))=RegPassword;
	*((int*)(DataN+6))=oper; //0x01 - ������, 0x02 - ������� �������
	*((int*)(DataN+7))=Double2Int(Quantity*1000000);
	*((int*)(DataN+13))=Double2Int(Price*MCU);
	*((int*)(DataN+18))=0xFF; // ����� 5���� ������������� ������������
	*((int*)(DataN+19))=0xFFFFFFFF; // ����� 5���� ������������� �������������
	*((int*)(DataN+23))=0xFF; // ����� 5���� ������������� ������������
	*((int*)(DataN+24))=0xFFFFFFFF; // ����� 5���� ������������� �������������
	*((int*)(DataN+28))=NDS;
	*((int*)(DataN+29))=Department%16;
//	*((int*)(DataN+30))=0x04; // 4 - ������ ������
//	*((int*)(DataN+31))=0x01; // �����

	*((int*)(DataN+30))=PaymentType; // 4 - ������ ������, 3 - �����
	*((int*)(DataN+31))=PaymentItem; // 1 - �����, 15 - �����������. ����� (������� �� �����������)

	strncpy((char*)(DataN+32),Name,128);
}

int ElvisRegVer3::ReturnGoodsV2(string NameStr, string Manufacture, double Price,double Quantity,int Department,int NDS, bool Check1stEntry, int PaymentType, int PaymentItem)
{
		int er,print_timeout;

    string TempName = NameStr;
#ifdef TESTLOG
    Log("Name proizv=STEP1=" + NameStr);
#endif
    // ����� ���������� �� �������������
    if (!Empty(Manufacture))
    {
        int lenName = TempName.length();
        int lenMan = Manufacture.length();
        if (lenName + lenMan + 1 > 127)
        {
            TempName = TempName.substr(0, 126 - lenMan);
#ifdef TESTLOG
            Log("Name proizv=STEP2=" + NameStr);
#endif
        }

        TempName +=" ";
        TempName +=Manufacture;
#ifdef TESTLOG
        Log("Name proizv=STEP3=" + NameStr);
#endif
    }

   if (TempName.length()>126){
#ifdef TESTLOG
       Log("Name proizv=STEP4=" + NameStr);
#endif
       TempName = TempName.substr(0, 126);
   }

    ;;Log("[RG] Name="+TempName);
    ;;Log("[RG] Quantity="+Double2Str(Quantity));
    ;;Log("[RG] LocalPrice="+Double2Str(Price));
    ;;Log("[RG] Section="+Int2Str(Department));
    ;;Log("[RG] NDS="+Int2Str(NDS));
    ;;Log("[RG] Payment Type="+Int2Str(PaymentType));
    ;;Log("[RG] Payment Item="+Int2Str(PaymentItem));

    const char* Name = (const char*)TempName.c_str();

	if (Check1stEntry)
	{
		double before;
		er=GetMoneyRegister(0,before);
		if (er!=0)
			return er;

		if (fabs(before)<epsilon)
			print_timeout=RegSpeed*3;
		else
			print_timeout=0;
	}
	else
		print_timeout=0;

	memset(DataN,0,DataNLen);
	    SetParamsV2(2, Name,Price,Quantity,Department,NDS,PaymentType, 33/*PaymentItem*/);
//SetParamsV2(2, Name,Price,Quantity,Department,NDS,PaymentType, 1/*PaymentItem*/);

fprintf(log,"\n%s\t������ �������� V2; %s: %f X %f (��� %d%%) (type=%d item=%d)", GetCurDateTime(), Name, Price, Quantity, NDS, PaymentType, 33/*PaymentItem*/);

   CntLog(Name, Price, Quantity);

   int len = 32 + TempName.length();
   if (len>160){
        len=160;
    }

	//!int ret = ProcessError(JustDoIt(120,print_timeout+RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	int ret = ProcessError(JustDoIt(len,50));
	if(!ret){


		fprintf(log,"\n%s\t������� V2; %s: %f X %f (��� %d%%)", GetCurDateTime(), Name, Price, Quantity, NDS);
	}
	
#ifdef BONUS_LOG_SKIDKA
                        fprintf(log,"\n%s\tPoditog Checka %s", GetCurDateTime(),"*************************");
                        fprintf(log,"\n%s\tPoditog Checka %s", GetCurDateTime(),"START");
                        SetInstrHeader(0x89);
                        int tester=JustDoIt(5,MinDuration);
                        if (tester!=0)
                            return tester;
                        double sumTest = 0;
                        sumTest=((*(int*)(DataN+3))%0xFFFFFFFF)*1./MCU;
                        fprintf(log,"\n%s\tPoditog Checka FINISH %s", GetCurDateTime(),("Poditog Checka=" + Double2Str(sumTest)).c_str());
                        fprintf(log,"\n%s\tPoditog Checka %s", GetCurDateTime(),"*************************");
                        ;;Log((char *)("PodITOGChecka="+Double2Str(sumTest)).c_str());
#endif
	
	return ret;
}


int ElvisRegVer3::Old_Tag1162(string Actsiz)
{

    const char* GTINStr = (const char*)Actsiz.substr(0,14).c_str();
    const char* Serial = (const char*)Actsiz.substr(14,7).c_str();

    unsigned long long GTINCode = atoll(GTINStr);

    fprintf(log,"\n%s\tCIGAR. ACTSIZ %s", GetCurDateTime(), Actsiz.c_str());
    //fprintf(log,"\n%s\tGTIN = %lld (%0*hhX) Serial = %s", GetCurDateTime(), GTINCode, 12, GTINCode, Serial);
    fprintf(log,"\n%s\tGTIN = %lld Serial = %s", GetCurDateTime(), GTINCode, Serial);

    fflush(log);

    int lenGTIN = 50;
    unsigned char GTIN[lenGTIN];
    memset(GTIN,0,lenGTIN);
    *((unsigned long long*)GTIN) = GTINCode;


	memset(DataN,0,DataNLen);
	DataN[0]=0xFF;
	*((int*)(DataN+1))=0x4D;

	*((int*)(DataN+2))=RegPassword;
	*((int*)(DataN+6))=1162; // 0x8A04
	//*((int*)(DataN+6))=0x8A; // ��� 1162
	//*((int*)(DataN+7))=0x04; // ��� 1162
	*((int*)(DataN+8))=0x0F; // ����� ��������� = 15
	*((int*)(DataN+9))=0x00; // ����� ���������
	*((int*)(DataN+10))=0x00; // ��� ���� ���������� = 00 05
	*((int*)(DataN+11))=0x05; // ��� ���� ���������� = 00 05
	//*((unsigned long*)(DataN+12))=GTIN%0xFFFFFFFFFFFFLL; // GTIN 6 ����

	for(int i=0; i<6; i++ ){
        *((int*)(DataN+12+i))=(int)(GTIN[5-i]); // GTIN 6 ����
	}
	strncpy((char*)(DataN+18),Serial, 7);  // Serial 7 ����

	int ret = ProcessError(JustDoIt(25,50));
	return ret;

}


// �������� e-mail ��� ������������ ����
int ElvisRegVer3::SendEmail(string email, bool isSender)
{

    fprintf(log,"\n%s\t SEND E-MAIL %s", GetCurDateTime(), email.c_str());
    fflush(log);

    int len = email.length();

	memset(DataN,0,DataNLen);
	DataN[0]=0xFF;
	*((int*)(DataN+1))=0x0C;
	*((int*)(DataN+2))=RegPassword;
	if (isSender)
	{ // �����������
        *((int*)(DataN+6))=1117; // 0x5D04
	}
	else
	{
	    // ����������
        *((int*)(DataN+6))=1008; // 0xF003
	}
	*((int*)(DataN+8))=len;
	*((int*)(DataN+9))=0x00; // ����� ���������
	strncpy((char*)(DataN+10),(const char*)email.c_str(), len);  // ������

    int ret = ProcessError(JustDoIt(6 + 4 + len, 50));
	return ret;

}

// �� ����� �������� � ������� 2021
// �������� �������������� ������ � �������
int ElvisRegVer3::SendMarkingCode(string Actsiz, double Quantity, int ActsizType) // ������ ������ �������� �� ���� ����� � �� �����
{
    int ret = 0;
    const char* MarkingBarcode = (const char*)Actsiz.c_str();

    fprintf(log,"\n%s\tMARKING BARCODE. ACTSIZ %s", GetCurDateTime(), Actsiz.c_str());
    fflush(log);

    if (/*(Actsiz.length() == 33) || (Actsiz.length() == 34)*/ActsizType == 6)    
	{
        fprintf(log,"\nPIVO_RAZLIV");
        fflush(log);
        SetInstrHeaderV2(0x67);
/*
        int len = 0;
        if (Actsiz.length()==33) {len = Actsiz.length()-1;fprintf(log,"\n������� %s", GetCurDateTime(), "PivoRazliv");}
        else {len = Actsiz.length()-2;fprintf(log,"\n������� %s", GetCurDateTime(), "PivoRazliv");}
*/	
		int len = Actsiz.length();
		fprintf(log,"\n������ %s", GetCurDateTime(), "PivoRazliv");
	
        *((int*)(DataN+6))=len; // ����� ���� ����������
        strncpy((char*)(DataN+7),MarkingBarcode, len);  // ��� ����������
        //*((int*)(DataN+7+len+1))=0x2F;
        ret = ProcessError(JustDoIt(7+len,50));
    }
    else
    {
        fprintf(log,"\nMarkirovka");
        fflush(log);
        SetInstrHeaderV2(0x67);
        int len = Actsiz.length();
        *((int*)(DataN+6))=len; // ����� ���� ����������
        strncpy((char*)(DataN+7),MarkingBarcode, len);  // ��� ����������
        ret = ProcessError(JustDoIt(7+len,50));
    }

	return ret;
}

// �� ����� �������� � ������� 2021
// �������� ���� �������������� ������
int ElvisRegVer3::CheckMarkingCode(string Actsiz, double Quantity, int ActsizType) //������ �������� ������ �� ���� �������� �����
{
	int ret = 0;
    const char* MarkingBarcode = (const char*)Actsiz.c_str();

    fprintf(log,"\n%s\tCHECK BARCODE. ACTSIZ %s", GetCurDateTime(), Actsiz.c_str());
    fflush(log);

    //bool flafPivoRazliv = false;
    if (/*(Actsiz.length() == 33) || (Actsiz.length() == 34)*/ActsizType == 6) //������ �������� ������ �� ���� �������� �����
    {
		fprintf(log,"\nPivoRazliv", GetCurDateTime(), "PivoRazliv");
		
        SetInstrHeaderV2(0x61);

/*        int len = 0; 
        if (Actsiz.length()==33) {len = Actsiz.length()-1;fprintf(log,"\n������� %s", GetCurDateTime(), "PivoRazliv");}
        else {len = Actsiz.length()-2;fprintf(log,"\n������� %s", GetCurDateTime(), "PivoRazliv");}*/

		int len = Actsiz.length();
		fprintf(log,"\n������ %s", GetCurDateTime(), "PivoRazliv");
		
        *((int*)(DataN+6))=2; // ����� ������ ������ (����������)
        *((int*)(DataN+7))=0; //
        *((int*)(DataN+8))=len;
        *((int*)(DataN+9))=17; // 0-������ ��������, 1-������-��������, 2-���������
        strncpy((char*)(DataN+10),MarkingBarcode, len);  // ��� ����������
        *((int*)(DataN+10+len)) = 2108;
        *((int*)(DataN+10+len+2))=1; //����� ���������             2 �����
        *((int*)(DataN+10+len+2+2))=0x29; //�������� �              1 ����
        *((int*)(DataN+10+len+2+2+1))=1023; //����� ���� 1023 0xff03  2 �����
        *((int*)(DataN+10+len+2+2+1+2))=8; //����� ���������            2 �����
        if(Quantity == 1.5)  ///��������� �������, ����������� � ����������
        {
            fprintf(log,"\n%s\tPivoRazliv 1_5 %s", GetCurDateTime(), "PivoRazliv");
            *((int*)(DataN+10+len+2+2+1+2+2))=0x01; //�������� ����������� 1  8 ����� (����� �� 15 �� 22)
            *((int*)(DataN+10+len+9+1))=0x0F;
        }
        else
        {
            fprintf(log,"\n%s\tPivoRazliv 1%s", GetCurDateTime(), "PivoRazliv");
            *((int*)(DataN+10+len+2+2+1+2+2))=0x00; //�������� ����������� 1  8 ����� (����� �� 15 �� 22)
            *((int*)(DataN+10+len+9+1))=0x01;
        }
        *((int*)(DataN+10+len+9+2))=0x00;
        *((int*)(DataN+10+len+9+3))=0x00;
        *((int*)(DataN+10+len+9+4))=0x00;
        *((int*)(DataN+10+len+9+5))=0x00;
        *((int*)(DataN+10+len+9+6))=0x00;
        *((int*)(DataN+10+len+9+7))=0x00;
        ret = ProcessError(JustDoIt(10+len+17,50));
    }
    else
    {
		fprintf(log,"\nMarkirovka", GetCurDateTime(), "Markirovka");
		
        SetInstrHeaderV2(0x61);
        int len = Actsiz.length();
        *((int*)(DataN+6))=1; // ����� ������ ������ (����������)
        *((int*)(DataN+7))=0; //
        *((int*)(DataN+8))=len; // ����� ���� ����������
        *((int*)(DataN+9))=0; // 0-������ ��������, 1-������-��������, 2-���������
        strncpy((char*)(DataN+10),MarkingBarcode, len);  // ��� ����������
        ret = ProcessError(JustDoIt(10+len,50));
    }

    return ret;
	
}

// �� ����� �������� � ������� 2021
// �������/���������� ���������� ����� ��������
// Status= 0-����������, 1- �������
int ElvisRegVer3::AcceptMarkingCode(int Status)
{
    if (Status) {
        fprintf(log,"\n%s\tACCEPT BARCODE. Status=%d", GetCurDateTime(), Status);
    } else {
        fprintf(log,"\n%s\tREJECT BARCODE. Status=%d", GetCurDateTime(), Status);
    }
    fflush(log);

    SetInstrHeaderV2(0x69);

	*((int*)(DataN+6))=Status; // ������

	int ret = ProcessError(JustDoIt(7,50));
	return ret;
}

// �� ����� �������� � ����� 2020
int ElvisRegVer3::Tag1162(string Actsiz)
{
Log("TAG1162_TEST");
    string GTINStrAsString = Actsiz.substr(0,14);
    string SerialAsString = Actsiz.substr(14,7);
    string MRCAsString = Actsiz.substr(21,4);

    const char* GTINStr = (const char*)GTINStrAsString.c_str();
    const char* Serial = (const char*)SerialAsString.c_str();
    const char* MRC = (const char*)MRCAsString.c_str();

//    const char* GTINStr = (const char*)Actsiz.substr(0,14).c_str();
//    const char* Serial = (const char*)Actsiz.substr(14,11).c_str(); // Serial + ���

    unsigned long long GTINCode = atoll(GTINStr);

    fprintf(log,"\n%s\tCIGAR. ACTSIZ %s", GetCurDateTime(), Actsiz.c_str());
    //fprintf(log,"\n%s\tGTIN = %lld (%0*hhX) Serial = %s", GetCurDateTime(), GTINCode, 12, GTINCode, Serial);
    //fprintf(log,"\n%s\tGTIN = %lld Serial = %s", GetCurDateTime(), GTINCode, Serial);
    fprintf(log,"\n%s\tGTIN = %s (%lld) Serial = %s  MRC = %s", GetCurDateTime(), GTINStr, GTINCode, Serial, MRC);

    fflush(log);

    int lenGTIN = 50;
    unsigned char GTIN[lenGTIN];
    memset(GTIN,0,lenGTIN);
    *((unsigned long long*)GTIN) = GTINCode;


	memset(DataN,0,DataNLen);
	DataN[0]=0xFF;
	*((int*)(DataN+1))=0x4D;

	*((int*)(DataN+2))=RegPassword;
	*((int*)(DataN+6))=1162; // 0x8A04
	//*((int*)(DataN+6))=0x8A; // ��� 1162
	//*((int*)(DataN+7))=0x04; // ��� 1162
	*((int*)(DataN+8))=0x15; // ����� ��������� = 21
	*((int*)(DataN+9))=0x00; // ����� ���������
	*((int*)(DataN+10))=0x44; // ��� ���� ���������� (444D) = 44
	*((int*)(DataN+11))=0x4D; // ��� ���� ���������� (444D) = 4D
	//*((unsigned long*)(DataN+12))=GTIN%0xFFFFFFFFFFFFLL; // GTIN 6 ����

	for(int i=0; i<6; i++ ){
        *((int*)(DataN+12+i))=(int)(GTIN[5-i]); // GTIN 6 ����
	}
	strncpy((char*)(DataN+18),Serial, 7);  // Serial 7 ����
	strncpy((char*)(DataN+25),MRC, 4);  // ��� 4 �����

    *((int*)(DataN+29))=0x20; // ������
    *((int*)(DataN+30))=0x20; // ������

	int ret = ProcessError(JustDoIt(31,50));
	return ret;

}


int ElvisRegVer3::Tag1162Textile(string Actsiz)
{

    string GTINStrAsString = Actsiz.substr(0,14);
    string SerialAsString = "";

    if (Actsiz.length() >= 27)
    {
        SerialAsString = Actsiz.substr(14,13);
    }
    else
    {
        SerialAsString = Actsiz.substr(14,6) + "       ";
    }
    //string MRCAsString = Actsiz.substr(21,4);

    const char* GTINStr = (const char*)GTINStrAsString.c_str();
    const char* Serial = (const char*)SerialAsString.c_str();

//    const char* GTINStr = (const char*)Actsiz.substr(0,14).c_str();
//    const char* Serial = (const char*)Actsiz.substr(14,11).c_str(); // Serial + ���

    unsigned long long GTINCode = atoll(GTINStr);

    fprintf(log,"\n%s\tTEXTILE. ACTSIZ %s", GetCurDateTime(), Actsiz.c_str());
    //fprintf(log,"\n%s\tGTIN = %lld (%0*hhX) Serial = %s", GetCurDateTime(), GTINCode, 12, GTINCode, Serial);
    //fprintf(log,"\n%s\tGTIN = %lld Serial = %s", GetCurDateTime(), GTINCode, Serial);
    fprintf(log,"\n%s\tGTIN = %s (%lld) Serial = %s ", GetCurDateTime(), GTINStr, GTINCode, Serial);

    fflush(log);

    int lenGTIN = 50;
    unsigned char GTIN[lenGTIN];
    memset(GTIN,0,lenGTIN);
    *((unsigned long long*)GTIN) = GTINCode;


	memset(DataN,0,DataNLen);
	DataN[0]=0xFF;
	*((int*)(DataN+1))=0x4D;

	*((int*)(DataN+2))=RegPassword;
	*((int*)(DataN+6))=1162; // 0x8A04
	//*((int*)(DataN+6))=0x8A; // ��� 1162
	//*((int*)(DataN+7))=0x04; // ��� 1162
	*((int*)(DataN+8))=0x15; // ����� ��������� = 21
	*((int*)(DataN+9))=0x00; // ����� ���������
	*((int*)(DataN+10))=0x44; // ��� ���� ���������� (444D) = 44
	*((int*)(DataN+11))=0x4D; // ��� ���� ���������� (444D) = 4D
	//*((unsigned long*)(DataN+12))=GTIN%0xFFFFFFFFFFFFLL; // GTIN 6 ����

	for(int i=0; i<6; i++ ){
        *((int*)(DataN+12+i))=(int)(GTIN[5-i]); // GTIN 6 ����
	}
	strncpy((char*)(DataN+18),Serial, 13);  // Serial 13 ����
//	strncpy((char*)(DataN+24),MRC, 5);  // ��� 5 �����
//
//    *((int*)(DataN+29))=0x20; // ������
//    *((int*)(DataN+30))=0x20; // ������

	int ret = ProcessError(JustDoIt(31,50));
	return ret;

}


int ElvisRegVer3::Tag1162Mask(string Actsiz)
{

    string EAN13AsString = Actsiz.substr(0,13);

    const char* EAN13Str = (const char*)EAN13AsString.c_str();
    unsigned long long EAN13Code = atoll(EAN13Str);

    fprintf(log,"\n%s\tMASK. ACTSIZ %s", GetCurDateTime(), Actsiz.c_str());
    fprintf(log,"\n%s\tEAN13 = %s (%lld) ", GetCurDateTime(), EAN13Str, EAN13Code);

    fflush(log);

    int lenEAN13 = 13;
    unsigned char EAN13[lenEAN13];
    memset(EAN13,0,lenEAN13);
    *((unsigned long long*)EAN13) = EAN13Code;


	memset(DataN,0,DataNLen);
	DataN[0]=0xFF;
	*((int*)(DataN+1))=0x4D;

	*((int*)(DataN+2))=RegPassword;
	*((int*)(DataN+6))=1162; // 0x8A04
	*((int*)(DataN+8))=0x08; // ����� ��������� = 8
	*((int*)(DataN+9))=0x00; // ����� ���������
	*((int*)(DataN+10))=0x45; // ��� ���� ���������� (450D) = 45
	*((int*)(DataN+11))=0x0D; // ��� ���� ���������� (450D) = 0D
	for(int i=0; i<6; i++ ){
        *((int*)(DataN+12+i))=(int)(EAN13[5-i]); // EAN13 6 ����
	}
//	strncpy((char*)(DataN+18),Serial, 7);  // Serial 7 ����
//	strncpy((char*)(DataN+25),MRC, 4);  // ��� 4 �����
//
//    *((int*)(DataN+29))=0x20; // ������
//    *((int*)(DataN+30))=0x20; // ������

	int ret = ProcessError(JustDoIt(18,50));
	return ret;

}

int ElvisRegVer3::StornoGoodsNDS(const char* Name,double Price,double Quantity,int Department, int NDS)
{

	memset(DataN,0,DataNLen);
	DataN[0]=0x84;
	SetParams(Name,Price,Quantity,Department,NDS);

	CntLog(Name, Price, Quantity);

	int ret = ProcessError(JustDoIt(60,RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));

	if(!ret){


		fprintf(log,"%s\t������; %s: %f X %f (��� %d%%)\n", GetCurDateTime(), Name, Price, Quantity, NDS);
	}
	return ret;
}

int ElvisRegVer3::StornoGoods(const char* Name,double Price,double Quantity,int Department)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x84;
	SetParams(Name,Price,Quantity,Department);
	int ret = ProcessError(JustDoIt(60,RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	if(!ret){


		fprintf(log,"%s\t������; %s: %f X %f\n", GetCurDateTime(), Name, Price, Quantity);
	}
	return ret;
}

int ElvisRegVer3::Storno(const char* Name,double Price,double Quantity,int Department)
{
	if (EntireCheck)
		return 0;
	return StornoGoods(Name,Price,Quantity,Department);
}

int ElvisRegVer3::ReturnGoodsNDS(string NameStr, string Manufacture,double Price,double Quantity,int Department,int NDS, bool Check1stEntry)
{
	int er,print_timeout;

    string TempName = NameStr;
    // ����� ���������� �� �������������
    if (!Empty(Manufacture))
    {
        TempName +=" ";
        TempName +=Manufacture;
    }

    ;;Log("[RG] Name="+TempName);
    ;;Log("[RG] Quantity="+Double2Str(Quantity));
    ;;Log("[RG] LocalPrice="+Double2Str(Price));
    ;;Log("[RG] Section="+Int2Str(Department));
    ;;Log("[RG] NDS="+Int2Str(NDS));

    const char* Name = (const char*)TempName.c_str();

	if (Check1stEntry)
	{
		double before;
		er=GetMoneyRegister(2,before);
		if (er!=0)
			return er;

		if (fabs(before)<epsilon)
			print_timeout=RegSpeed*3;
		else
			print_timeout=0;
	}
	else
		print_timeout=0;


	memset(DataN,0,DataNLen);
	DataN[0]=0x82;
	SetParams(Name,Price,Quantity,Department,NDS);

    CntLog(Name, Price, Quantity);

	int ret = ProcessError(JustDoIt(120,print_timeout+RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	if(!ret)
	{
		fprintf(log,"%s\t�������; %s: %f X %f\n (��� %d)", GetCurDateTime(), Name, Price, Quantity, NDS);
	}
	return ret;
}

int ElvisRegVer3::ReturnGoods(const char* Name,double Price,double Quantity,int Department,bool Check1stEntry)
{
	int er,print_timeout;

	if (Check1stEntry)
	{
		double before;
		er=GetMoneyRegister(2,before);
		if (er!=0)
			return er;

		if (fabs(before)<epsilon)
			print_timeout=RegSpeed*3;
		else
			print_timeout=0;
	}
	else
		print_timeout=0;


	memset(DataN,0,DataNLen);
	DataN[0]=0x82;
	SetParams(Name,Price,Quantity,Department);

	int ret = ProcessError(JustDoIt(60,print_timeout+RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	if(!ret)
	{
		fprintf(log,"%s\t�������; %s: %f X %f\n", GetCurDateTime(), Name, Price, Quantity);
	}
	return ret;
}

int ElvisRegVer3::Return(const char* Name,double Price,double Quantity,int Department)
{
	if (EntireCheck)
		return 0;
	return ReturnGoods(Name,Price,Quantity,Department,true);
}

void ElvisRegVer3::OpenDrawer(void)
{
	SetInstrHeader(0x28);
	JustDoIt(6,MinDuration);
}

int ElvisRegVer3::CashIncome(double sum)
{
	SetInstrHeader(0x50);
	*((int*)(DataN+5))=Double2Int(sum*MCU)%0xFFFFFFFF;
	int er = ProcessError(JustDoIt(10,RegSpeed*5));

//	if (er==0)
//		CutCheck();
	return er;
}

int ElvisRegVer3::CashOutcome(double sum)
{
	SetInstrHeader(0x51);
	*((int*)(DataN+5))=Double2Int(sum*MCU)%0xFFFFFFFF;
	int er = ProcessError(JustDoIt(10,RegSpeed*5));
//	if (er==0)
//		CutCheck();
	return er;
}

int ElvisRegVer3::GetCurrentSumm(double& sum)
{
	return GetMoneyRegister(241,sum);
}

int ElvisRegVer3::OpenKKMSmena(void)
{
    SetInstrHeader(0xE0);
	int er = JustDoIt(5,1000);

    WaitFR();

    return er;
}

int ElvisRegVer3::closeAdminCheck()
{
    SetInstrHeader(0x88); //�������� ���� �� �������� ����
    int er = JustDoIt(5,1000);

    WaitFR();

    return er;
}

int ElvisRegVer3::Reprint(void)
{

    SetInstrHeader(0x88);
	int er1 = JustDoIt(5,1000);

	Log("[Proc] Reprint");
    SetInstrHeader(0xB0);
	int er = JustDoIt(5,1000);

    return er;
}

int ElvisRegVer3::XReport(void)
{
   // FRReportBySmena();

	SetInstrHeader(0x40);
	int er = JustDoIt(5,1000);

	if (er!=0)
		return er;
	do
	{
		WaitSomeTime(2000);
		er = GetMode();
		if (er!=0)
			break;
	}
	while((DataN[0x10]==4)||(DataN[0x10]==5));

	er = ProcessError(er);
//	if (er==0)
//		CutCheck();
	return er;
}

int ElvisRegVer3::FRReportBySmena(void)
{
	SetInstrHeader(0x67);
	int er = JustDoIt(5,1000);

	if (er!=0)
		return er;
	do
	{
		WaitSomeTime(2000);
		er = GetMode();
		if (er!=0)
			break;
	}
	while((DataN[0x10]==4)||(DataN[0x10]==5));

	er = ProcessError(er);
//	if (er==0)
//		CutCheck();
	return er;
}

void ElvisRegVer3::SetDBArchPath(string path)
{
	DBArchPath = path;
}

int ElvisRegVer3::WaitFR() {
    int er;
	do{
		WaitSomeTime(2000);
		er = GetMode();
		if (er!=0)
			break;
	}while((DataN[0x10]==4)||(DataN[0x10]==5));

    return er;
}

int ElvisRegVer3::ZReport(double& sum)
{
	double val;
	sum = 0.;
	time_t ltime;
	time( &ltime );
	fprintf(log,"%s %s", ctime(&ltime),"Z.�������� ��������:\n");
	int er = GetMoneyRegister(193,val);
	fprintf(log,"193 - %f\t",val);
	if (er!=0)
		return er;
	sum += val;
//	nick{
	er = GetMoneyRegister(197,val);
	fprintf(log,"197 - %f\t",val);
	if (er!=0)
		return er;
	sum += val;
	er = GetMoneyRegister(201,val);
	fprintf(log,"201 - %f\t",val);
	if (er!=0)
		return er;
	sum += val;
	er = GetMoneyRegister(205,val);
	fprintf(log,"205 - %f\n",val);
	if (er!=0)
		return er;
	sum += val;
	er = GetMoneyRegisterV2(4180,val); // ��������������� ������ (������)
	fprintf(log,"4180 - %f\n",val);
	if (er!=0)
		return er;
	sum += val;
//	}nick

    if(CashRegister::ConfigTable->GetLogicalField("OFD")){
        //������ ������ �� �������
        SetInstrHeader(0x42);
        int er1 = JustDoIt(5,1000);

        WaitFR();
    }

	SetInstrHeader(0x41);
	er = JustDoIt(5,1000);
	if (er!=0)
		return er;

	time( &ltime );
	struct tm *now;
	now = localtime( &ltime );
	char str[256];
	sprintf( str, "fr%d-%02d-%02d %02d_%02d_%02d.log", now->tm_year + 1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
	fclose(log);

    MoveFileSys("Kacca/tmp/fr.log", DBArchPath+"/"+str);

    string s = str;

	rename("tmp/fr.log",("arch/"+s).c_str());

	fclose(fopen("tmp/fr.log","a"));
	log = fopen("tmp/fr.log","r+");
	fseek(log, 0, SEEK_END);

   // control
	char cstr[256];
	sprintf( cstr, "control_%d-%02d-%02d %02d_%02d_%02d.log", now->tm_year + 1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
	fclose(cLog);

    string cs = cstr;

    MoveFileSys("/Kacca/tmp/control.log", DBArchPath+"/"+cstr);


    int err = rename("tmp/control.log", ("arch/"+cs).c_str());


	fclose(fopen("tmp/control.log","a"));
	cLog = fopen("tmp/control.log","r+");
	fseek(cLog, 0, SEEK_END);

	do{
		WaitSomeTime(2000);
		er = GetMode();
		if (er!=0)
			break;
	}while((DataN[0x10]==4)||(DataN[0x10]==5));

	er = ProcessError(er);
//	if (er==0)
//		CutCheck();

	if(er)
		return er;

	bool t;
	RestorePrinting(t); // �������������� ������������� ������� ����� ������

	return er;
}

int ElvisRegVer3::SetTime(int hour,int minute,int sec)
{
	SetInstrHeader(0x21);
	DataN[5]=hour%24;
	DataN[6]=minute%60;
	DataN[7]=sec%60;
	return JustDoIt(8,150);
}

int ElvisRegVer3::SetDate(int day,int month,int year)
{
	SetInstrHeader(0x22);
	DataN[5]=day%32;
	DataN[6]=month%13;
	DataN[7]=year%100;

	int er = JustDoIt(8,100);
	if (er!=0 && er!=0x73)//Nick
		return er;

	SetInstrHeader(0x23);
	DataN[5]=day%32;
	DataN[6]=month%13;
	DataN[7]=year%100;
	return JustDoIt(8,150);
}

int ElvisRegVer3::PrintString(const char* str,bool ctrl)
{
    if (CashRegister::ConfigTable->GetLogicalField("FRK_FONT"))
    {
        return PrintStringWithFont(str, 5, ctrl);
    }
    else
    {

        /*
        SetInstrHeader(0x2F);
        DataN[5] = ctrl ? 0x03 : 0x02;
        DataN[6] = 0x01;
        strncpy((char*)DataN+7,*str?str:" ",40);
        int len = 47;
        */

        SetInstrHeader(0x17);
        DataN[5] = ctrl ? 0x03 : 0x02;
        strncpy((char*)DataN+6,*str?str:" ",40);
        int len = 46;


        /*
        int len = 6;
        if (*str)
        {
            int slen = min((int)strlen(str), 40);
            strncpy((char*)DataN+6,str, slen);
            len +=slen ;
        }
        */

        fprintf(log,"\n%s\t -> PrintString %s", GetCurDateTime(), str);
        fflush(log);

        CntLog(str);

        return ProcessError(JustDoIt(len,0));
    }
}

int ElvisRegVer3::PrintStringWithFont(const char* str, unsigned short curfont, bool ctrl)
{
	SetInstrHeader(0x2F);
	DataN[5] = ctrl ? 0x03 : 0x02;
	DataN[6] = curfont;
	strncpy((char*)DataN+7,*str?str:" ",40);

	fprintf(log,"\n%s\t -> PrintStringWithFont %s", GetCurDateTime(), str);
	fflush(log);

	CntLog(str);

	return ProcessError(JustDoIt(46,0));
}

void ElvisRegVer3::WaitNextHeader(void)
{
	WaitSomeTime(RegSpeed*7);
}

int ElvisRegVer3::PrintHeader(void)
{
	char StringForPrinting[41];
	int strSize=40;

    bool OFD = false;
    if (CashRegister::ConfigTable->GetLogicalField("OFD")) OFD = true;

    if (OFD)
    {
        memset(StringForPrinting,0,strSize+1);
        //int er = ReadTable(4,4,1);
        int er = ReadTable(4,12,1);
        if (er!=0)
            return er;

        strncpy(StringForPrinting,(char*)(DataN+2),strSize);
        er = PrintString(StringForPrinting);
        if (er!=0)
            return er;

        memset(StringForPrinting,0,strSize+1);
        //er = ReadTable(4,5,1);
        er = ReadTable(4,13,1);
        if (er!=0)
            return er;

        strncpy(StringForPrinting,(char*)(DataN+2),strSize);
        er = PrintString(StringForPrinting);
        if (er!=0)
            return er;

        memset(StringForPrinting,0,strSize+1);
        //er = ReadTable(4,6,1);
        er = ReadTable(4,14,1);
        if (er!=0)
            return er;

        strncpy(StringForPrinting,(char*)(DataN+2),strSize);
        er = PrintString(StringForPrinting);
        if (er!=0)
            return er;
    }
	return 0;
}

int ElvisRegVer3::CutCheck(void)
{
	SetInstrHeader(0x25);
	*(DataN+5) = 1; // �������� ������� ������
	return JustDoIt(6,100);
}

void ElvisRegVer3::SetParams(const char* Name,double Price,double Quantity,int Department,int NDS)
{
	*((int*)(DataN+1))=RegPassword;
	*((int*)(DataN+5))=Double2Int(Quantity*1000);
	*((int*)(DataN+10))=Double2Int(Price*MCU);
	*((int*)(DataN+15))=Department%16;
	*((int*)(DataN+16))=NDS;
	strncpy((char*)(DataN+20),Name,100);
}

void ElvisRegVer3::SetInstrHeader(unsigned char instr)
{
	memset(DataN,0,DataNLen);
	DataN[0]=instr;
	(*(int*)(DataN+1))=RegPassword;
}

void ElvisRegVer3::SetInstrHeaderV2(unsigned char instr)
{
	memset(DataN,0,DataNLen);
    DataN[0]=0xFF;
    *((int*)(DataN+1))=instr;
	*((int*)(DataN+2))=RegPassword;
}


int ElvisRegVer3::GetMoneyRegister(int num,double& sum)
{
	SetInstrHeader(0x1A);
	DataN[5]=num;
	int er = JustDoIt(6,100);
	if (er!=0)
		return er;

	sum=(*(int*)(DataN+3))%0xFFFFFFFF*1.0/MCU;
	return 0;
}

int ElvisRegVer3::GetMoneyRegisterV2(int num,double& sum)
{
	SetInstrHeader(0x1A);
	*((short*)(DataN+5))=num;
	int er = JustDoIt(7,100);
	if (er!=0)
		return er;

	sum=(*(int*)(DataN+3))%0xFFFFFFFF*1.0/MCU;
	return 0;
}

int ElvisRegVer3::GetOperationRegister(int num,int& sum)
{
	SetInstrHeader(0x1B);
	DataN[5]=num;
	int er = JustDoIt(6,100);
	if (er!=0)
		return er;
	sum=(*(int*)(DataN+3));//%0xFFFFFFFF*1.0;
	return 0;
}

int ElvisRegVer3::ReadTable(int TableNumber,int RowNumber,int FieldNumber)
{
	SetInstrHeader(0x1F);
	DataN[5]=TableNumber%0xFF;
	*((short*)(DataN+6))=RowNumber%0xFFFF;
	DataN[8]=FieldNumber%0xFF;
	return JustDoIt(9,100);
}

int ElvisRegVer3::GetDateShiftRange(const char* password,DateShiftRange& range)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x64;
	(*(int*)(DataN+1))=atoi(password);
	int er=JustDoIt(5,100);

	if (er!=0)
		return er;

	range.FirstDate.tm_mday=DataN[2];
	range.FirstDate.tm_mon=DataN[3];
	range.FirstDate.tm_year=DataN[4];

	range.LastDate.tm_mday=DataN[5];
	range.LastDate.tm_mon=DataN[6];
	range.LastDate.tm_year=DataN[7];

	range.FirstShift=*((short*)(DataN+8))%0xFFFF;
	range.LastShift=*((short*)(DataN+10))%0xFFFF;

	return 0;
}

int ElvisRegVer3::DateRangeReport(const char* password,DateShiftRange range,bool type)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x66;
	(*(int*)(DataN+1))=atoi(password);
	if (type)
		DataN[5]=1;
	DataN[6]=range.FirstDate.tm_mday%31;
	DataN[7]=range.FirstDate.tm_mon%12;
	DataN[8]=range.FirstDate.tm_year%100;
	DataN[9]=range.LastDate.tm_mday%31;
	DataN[10]=range.LastDate.tm_mon%12;
	DataN[11]=range.LastDate.tm_year%100;

	int er=JustDoIt(12,1000);
	if (er!=0)
		return er;

	do
	{
		er = GetMode();
		if (er!=0)
			break;
	}
	while((DataN[0x10]==4)||(DataN[0x10]==5)||(type&&((DataN[0x0F]&0x0F)==11)));

	er = ProcessError(er);
	if (er==0)
		CutCheck();
	return er;
}

int ElvisRegVer3::ShiftRangeReport(const char* password,DateShiftRange range,bool type)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x67;
	(*(int*)(DataN+1))=atoi(password);
	if (type)
		DataN[5]=1;
	*((short*)(DataN+6))=range.FirstShift%0xFFFF;
	*((short*)(DataN+8))=range.LastShift%0xFFFF;

	int er=JustDoIt(10,1000);
	if (er!=0)
		return er;

	do
	{
		er = GetMode();
		if (er!=0)
			break;
	}
	while((DataN[0x10]==4)||(DataN[0x10]==5)||(type&&((DataN[0x0F]&0x0F)==11)));

	er = ProcessError(er);
	if (er==0)
		CutCheck();
	return er;
}

#define P_CHECK	0x01
#define P_CNTRL	0x02
int ElvisRegVer3::GetMode(void)
{
	SetInstrHeader(0x11);
	int ret = JustDoIt(5,300);
	/*
	if(ret)
		return ret;
	static unsigned char flag = 0;
	unsigned char fr = (*(DataN + 14));// & (P_CHECK | P_CNTRL);
	//if()
	*/
	return ret;
}

int ElvisRegVer3::ContinuePrinting(void)
{
	SetInstrHeader(0xB0);
	int er = JustDoIt(5,500);
	if (er!=0)
		return er;

	do
	{
		er = GetMode();
		if (er!=0)
			break;
	}
	while((DataN[0x10]==4)||(DataN[0x10]==5));

	return er;
}

int ElvisRegVer3::RestorePrinting(bool& restore)
{
	int er = GetMode();
	if (er != 0)
		return er;

	restore = ((DataN[0x10]==0x02)||(DataN[0x10]==0x03));

//	nick
	if((DataN[15] & 0x0f) == 3){
		::ShowMessage(qApp->focusWidget(),
			"�������� ����� �����������!\n������� Z-�����!");
	}

//	������ ������� � ���� (����� GetMode())
	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	tm reg_tm;
	reg_tm.tm_year	= DataN[27] + 100; // c 1900
	reg_tm.tm_mon	= DataN[26] - 1; // c 0
	reg_tm.tm_mday	= DataN[25];
	reg_tm.tm_hour	= DataN[28];
	reg_tm.tm_min	= DataN[29];
	reg_tm.tm_sec	= DataN[30];
	reg_tm.tm_isdst = t->tm_isdst;
	time_t reg_time = mktime(&reg_tm);
	if( abs(reg_time - time(NULL)) > 300 ){// (������ ������ - ���� �����)
		FILE * f = fopen("tmp/timesync.log", "r");
		if(f){
			char m, d, y, hh, mm, ss, y_n;
			fscanf(f, "%2d/%2d/%4d %2d:%2d:%2d %c", &m, &d, &y, &hh, &mm, &ss, &y_n);
			fclose(f);
			if(y_n == 'Y'){
				tm sync_tm;
				sync_tm.tm_year	= y%100 + 100; // c 1900
				sync_tm.tm_mon	= m - 1; // c 0
				sync_tm.tm_mday	= d;
				sync_tm.tm_hour	= hh;
				sync_tm.tm_min	= mm;
				sync_tm.tm_sec	= ss;
				sync_tm.tm_isdst = t->tm_isdst;
				time_t sync_time = mktime(&sync_tm);
				long d = time(NULL) - sync_time;
//				Log("if(d < 0)");
				if(d < 0)
				{ // ������� ����� ������ ��������� ������� �������������
//					Log("d < 0");
					goto l_reg_sync;
				}
//				Log("if(d < 60*60*24 )");
				if(d < 60*60*24 )
				{//��������� ������������� �� ������ ����� �����
//					Log("d < 60*60*24");
					if((DataN[15] & 0x0f) == 4){// ����� ������� - ����� ����������������
//						Log("(DataN[15] & 0x0f) == 4)");
						time_t cur_time=time(NULL);
						tm *l_time=localtime(&cur_time);
						SetDate(l_time->tm_mday,l_time->tm_mon+1,l_time->tm_year-100);
						SetTime(l_time->tm_hour,l_time->tm_min,l_time->tm_sec);
					}
					else
					{
						::ShowMessage(qApp->focusWidget(),
							"����������� ������� �� ������������ � ���������� ����� ���� �����!");
					}
					goto l_sync_end;
				}
			}
		}
l_reg_sync:
		char cmd[256];
		sprintf( cmd, "date -s \"%02d/%02d/%02d %02d:%02d:%02d\"", reg_tm.tm_mon + 1, reg_tm.tm_mday, reg_tm.tm_year%100, reg_tm.tm_hour, reg_tm.tm_min, reg_tm.tm_sec );
//		Log(cmd);
		system(cmd);
	}
l_sync_end:

//	nick
//	��������� ���������� ��� Z-������
	char v;
	GetTable(1,1,2,&v,1,false);
	if(!v)
		SetTable(1,1,2,&(v=1),1,false);

//	�������������� ������� ���� (��������)
	GetTable(1,1,8,&v,1,false);
	if(v != 2)
		SetTable(1,1,8,&(v = 2),1,false);

//	������ ��������� ����� �� ����� ������������� �������
	GetTable(1,1,3,&v,1,false);
	if(v != 1)
		SetTable(1,1,3,&(v = 1),1,false);

//	������ ��������� ����
	GetTable(1,1,20,&v,1,false);
	if(v != 0)
		SetTable(1,1,20,&(v = 0),1,false);

//	������ ��� � ������ ��� �� ����� ������������� �������
	GetTable(1,1,24,&v,1,false);
	if(v != 1)
		SetTable(1,1,24,&(v = 1),1,false);

//	������ ������ �� ����� ������������� �������
	GetTable(1,1,31,&v,1,false);
	if(v != 1)
		SetTable(1,1,31,&(v = 1),1,false);

//	������ ��������� ��� ������� �����
	GetTable(1,1,42,&v,1,false);
	if(v != 0)
		SetTable(1,1,42,&(v = 1),1,false);

// �������� ��
	GetTable(1,1,28,&v,1,false);
	if(v != 4)
		SetTable(1,1,28,&(v = 4),1,false);

// ��������� �������� �����
	GetTable(1,1,25,&v,1,false);
	if(v != 0)
		SetTable(1,1,25,&(v = 0),1,false);

	return 0;
}

int ElvisRegVer3::ProcessError(int er)
{
	if (er!=0)
	{
		bool restore;
		int error=RestorePrinting(restore);
		if (error!=0)
			return error;
		if (restore)
			ContinuePrinting();//try to restore 'printing mode'
	}

	return er;
}

int ElvisRegVer3::SetFullZReport(void)
{
    int er;
	char v;
//	GetTable(1,1,40,&v,1,false);
//	if(!v)
//		er = SetTable(1,1,40,&(v=1),1,false);
//	else
//        er = SetTable(1,1,40,&(v=0),1,false);

    er = SetTable(1,1,40,&(v=0),1,false);

    return er;
}

int ElvisRegVer3::SetTable(int table,int row,int field,void* val,int len,bool asStr)
{
	SetInstrHeader(0x1E);
	DataN[5]=table%0xFF;
	*((short*)(DataN+6))=row%0xFFFF;
	DataN[8]=field%0xFF;
	memcpy(DataN+9,val,len);
	return JustDoIt(9+len,100);
}

int ElvisRegVer3::GetTable(int table,int row,int field,void* val,int len,bool asStr)
{
	int er=ReadTable(table,row,field);
	if (er!=0)
		return er;

	memcpy(val,DataN+2,len);
	return 0;
}

void ElvisRegVer3::InsertSales(vector<SalesRecord> data)
{
	if ((EntireCheck)&&(Sales.empty())&&(!StartClosing))
		Sales=data;
}

void ElvisRegVer3::InsertPayment(vector<PaymentInfo> data)
{
	if ((EntireCheck)&&(Payment.empty())&&(!StartClosing))
		Payment=data;
}

void ElvisRegVer3::GetPayment(vector<PaymentInfo>** ptr)
{
	*ptr=&Payment;
}

int ElvisRegVer3::GetCheckSum(double& sum)
{
	SetInstrHeader(0x89);
	int er=JustDoIt(5,MinDuration);
	if (er!=0)
		return er;

	sum=((*(int*)(DataN+3))%0xFFFFFFFF)*1./MCU;
	return 0;
}


int ElvisRegVer3::PrintBarCode(__int64 barcode)
{
	SetInstrHeader(0xC2);
//	barcode %= 0xFFFFFFFFFF;
	(*(__int64*)(DataN+5))=barcode;//%0xFFFFFFFFFF; // ���� ����
	return JustDoIt(10,MinDuration);
}

#define OUT_FILE_PIXEL_PRESCALER   1

#define PIXEL_COLOR_R   0x00
#define PIXEL_COLOR_G   0x00
#define PIXEL_COLOR_B   0x00

#define BI_RGB			0L

#define OUT_FILE			"tmp/QRCode.bmp"	 // Output file name


void SetBit(int BitNo, bool OnOff, char* Dat, int len)
{
    int ind;
    char Mask;

    ind=(int) (BitNo/8);  // ����� ����� � �������

    if (ind>=len) return;

    Mask = 1 << (BitNo % 8); // ����� ���� (�������� 00010000)- ���������� ����� ������ ��� � ����� ����� ����� 1 (��������� =0)

    if (OnOff) Dat[ind] |= Mask; // ���������� ���
    else       Dat[ind] &= (~Mask); // �������� ���

}

int ElvisRegVer3::PrintQRCode(string QRString, int PrintMode)
{
// ����� ������
// 0 - ��� ������
// 1 - ��� ������������� ��������������� ������� C4 - C3
// 2 - � ����������������, ������� C4 - 4F
// 3 - ����������� ������� ��� DD  - DE

    if (!PrintMode) return 0;

    const char* str = (const char*)QRString.c_str();


    if (PrintMode==3)
    {

        fprintf(log,"\nText:\t%s", QRString.c_str());
        fflush(log);

        fprintf(log,"\n:Length\t%d", QRString.length());
        fflush(log);

        int err=0;
        int cnt = int(QRString.length()/64) + 1;

        for(unsigned short i=0;i<cnt;i++)
        {
            string QRStringPart = QRString.substr(i*64,64);
            const char* str_i = (const char*)QRStringPart.c_str();

            SetInstrHeader(0xDD);
            *((char*)(DataN+5))=0;
            *((char*)(DataN+6))=(char)i;
            memcpy(DataN+7,str_i,strlen(str_i));

            int cmdlen = 7+strlen(str_i);

            err = JustDoIt(cmdlen,MinDuration);
        }

        SetInstrHeader(0xDE);
        *((char*)(DataN+5))=3; // ��� ��������� 3 = QRCode
        *((short*)(DataN+6))=strlen(str); // ����� ���������
        *((char*)(DataN+8))=0; // ��������� ����
        //*((char*)(DataN+9))=10; // ������ ( 0 - ����, ���� =30 )
        //*((char*)(DataN+9))=10; // �����
        *((char*)(DataN+11))=4; // ������ ����� 3-8
        *((char*)(DataN+13))=2; // ������� ������. ������ 0-3
        *((char*)(DataN+14))=1; // �� ������
        err = JustDoIt(15,MinDuration);

        return err;
    }

    unsigned int unWidth, unWidthAdjusted, unDataBytes, unResultBytes;
    unsigned int x, y, l, n;
    unsigned char* pRGBData, *pSourceData, *pDestData, *pResultData;
    QRcode *pQRC;

// ���������� QRcode
	if (pQRC = QRcode_encodeString(str, 0, QR_ECLEVEL_H, QR_MODE_8, 1))
	{
		unWidth = pQRC->width;


         fprintf(log,"\nText:\t%s", QRString.c_str());
         fflush(log);

         fprintf(log,"\nQR width:\t%d", pQRC->width);
         fflush(log);

         pSourceData = pQRC->data;
         unWidth = pQRC->width;


         //short offset=16;
         short offset=68;
         short ImageWidth = 320 - offset*2;

         short XSize=(int)(ImageWidth/unWidth);
         short YSize=XSize;

         fprintf(log,"\nImage Widht (px):\t%d", ImageWidth);

         fprintf(log,"\nXSize:\t%d", XSize);
         fprintf(log,"\nYSize:\t%d", YSize);

         fprintf(log,"\noffset:\t%d", offset);
         fflush(log);

        int row=0;
        int y=0;
        int pos=0;
        //while (j<unResultBytes)

        bool bit;

        for(int i=0;i<2;i++)
        {
            char img[40];
            memset(img, 0x00, 40);

            SetInstrHeader(0xC4);
            *((short*)(DataN+5))=++row;
            memcpy(DataN+7,img,40);
            int err = JustDoIt(47,MinDuration);
        }

        while (y<unWidth)
        {

            //memcpy(DataN+7,val,40);

            int x=0;
            char img[40];

            memset(img, 0x00, 40);

            while (x<unWidth)
            {

                bit = (*pSourceData & 1);

                for ( int i= x*XSize ; i< (x+1)*XSize; i++)
                {
                    pos = offset + i;
                    SetBit(pos, bit, img, 40);

                }

                //*((char*)(DataN+7+i)) = pQRC->data[j*40+i];
                x++;

                pSourceData++;
            }

            if (PrintMode==2)
            {
                // ������ � ����������������
                SetInstrHeader(0xC4);
                *((short*)(DataN+5))=++row;
                memcpy(DataN+7,img,40);
                int err = JustDoIt(47,MinDuration);

                if (err) break;
            }
            else
            {
                // ������ ��� ������������� ���������������, ������ ������� ����
                for(int i=0;i<YSize;i++)
                {
                    SetInstrHeader(0xC4);
                    *((short*)(DataN+5))=++row;
                    memcpy(DataN+7,img,40);
                    int err = JustDoIt(47,MinDuration);

                    if (err) break;
                }
            }
            y++;
        }


        for(int i=0;i<2;i++)
        {
            char img[40];
            memset(img, 0x00, 40);

            SetInstrHeader(0xC4);
            *((short*)(DataN+5))=++row;
            memcpy(DataN+7,img,40);
            int err = JustDoIt(47,MinDuration);
        }


        QRcode_free(pQRC);

        if (row>0)
        {
            if (PrintMode==2)
            {
                // ������ � ������������ ����������������
                SetInstrHeader(0x4F);
                *((char*)(DataN+5))=0x01;
                *((char*)(DataN+6))=row;
                *((char*)(DataN+7))=YSize;
                *((char*)(DataN+8))=0x01;
                return JustDoIt(9,MinDuration);
            }
            else
            {
                // ������ ��� ������������� ���������������
                SetInstrHeader(0xC3);
                *((short*)(DataN+5))=0x01;
                *((short*)(DataN+7))=row;

                return JustDoIt(9,MinDuration);
            }
        }
    }

}


FILE *f;
int ElvisRegVer3::LogOpRegister()
{
	//double
	int val,val1;
	int er;
	f = fopen("tmp/OR.log","a");
	//er = GetOperationRegister(152,val);
	er = GetOperationRegister(148,val);//�������
	if (er==0){
		er = GetOperationRegister(150,val1);//�������
		if (er==0)
			fprintf(f,"%i\n",val+val1);
		else
			fprintf(f,"err\n");
	}else
		fprintf(f,"err\n");
	fclose(f);
	return 0;
}

int ElvisRegVer3::Test()
{
	for(;;){
		int er = GetMode();
		if (er!=0)
			return er;
		short mask = 0x01;
		unsigned short fr = *(unsigned short *)(DataN + 13);
		QString msg = "����� ��: ";
		msg += QString::number(fr, 16) + '\n';
		msg += '\t' + QString("������� ����������� �����: ") + (fr & mask ? "����":"���") + '\n';
		msg += '\t' + QString("������� ������� �����: ") + (fr & (mask <<= 1) ? "����":"���") + '\n';
		msg += '\t' + QString("��������� ���������� �����: ") + (fr & (mask <<= 3) ? '2':'0') + " ������\n";
		msg += '\t' + QString("������� ����: ") + (fr & (mask <<= 1) ? "����":"���") + '\n';
		msg += QString("���������� �������:\n");
		msg += '\t' + QString("������� ����������� �����: ") + (fr & (mask <<= 1) ? "����":"���") + '\n';
		msg += '\t' + QString("������� ������� �����: ") + (fr & (mask <<= 1) ? "����":"���") + '\n';
		msg += QString("��������� ������:\n");
		msg += '\t' + QString("����������� �����: ") + (fr & (mask <<= 1) ? "������":"������") + '\n';
		msg += '\t' + QString("������� �����: ") + (fr & (mask <<= 1) ? "������":"������") + '\n';
		msg += "�����(/��������): ";
		char *mode_txt[] = {
			"������ ������",
			"������� �����",
			"�������� ����� ����������",
			"����� �������",
			"���������� �� ������������� ������ ���������� ����������",
			"�������� ������������� ����� ����",
			"���������� ��������� ��������� ���������� �����",
			"�������� ��������",
			"���������� ���.��������� � ������������� ������",
			"�������� ������",
			"������ ������� ����������� ������"
		};
		msg += mode_txt[(DataN[15] & 0x0f) - 1];
		if(DataN[16]){
			msg += " / ";
			char *submode_txt[] = {
				"��������� ���������� ������",
				"�������� ���������� ������",
				"�������� ������������� ������",
				"������ ������ ���������� �������",
				"������ ��������"
			};
			msg += submode_txt[(DataN[16] & 0x07) - 1];
		}
		msg += "\n����/�����: " +
			QString::number(DataN[25]) + '.' +
			QString::number(DataN[26]) + '.' +
			QString::number(DataN[27] + 2000) + '/' +
			QString::number(DataN[28]) + ':' +
			QString::number(DataN[29]) + ':' +
			QString::number(DataN[30]) + '\n';
		unsigned char fp = DataN[31];
		msg += "����� ��: " + QString::number(fp, 16) + "; ��1-";
		msg += (fp & 0x01) ? "����": "���";
		msg += ", ��2-";
		msg += (fp & 0x02) ? "����": "���";
		msg += ", �������� ";
		if(!(fp & 0x04))
			msg += "�� ";
		msg += "�������";
		if(fp & 0x08)
			msg += ", ������������ ��";
		msg += "\n��������� �����: ";
		msg += QString::number(*(unsigned long *)(DataN + 32));//����� ���
		msg += "\n����� ��������� �������� �����: ";
		msg += QString::number(*(unsigned short *)(DataN + 36));
		msg += "\n���������� ��������� ������� � ��: ";
		msg += QString::number(*(unsigned short *)(DataN + 38));
		msg += "\n���������� ������������ / ����������: ";
		msg += QString::number(*(unsigned char *)(DataN + 40));
		msg += " / ";
		msg += QString::number(*(unsigned char *)(DataN + 41));
		QStringList l;
		//l += W2U("�����");
		l += W2U("Exit");
		l += W2U("������������");
		l += W2U("���������� ������");
		l += W2U("���. ���������");
		int t[] = {0,1,2,3};

		QString s = QInputDialog::getItem("APM KACCA",
			W2U(msg.ascii()),l,0,false);
		int i = l.findIndex(s);
		if((!i) || i > 3)
			return 0;
		unsigned char cmd[] = {0x88, 0xb0, 0x16};
		SetInstrHeader(cmd[i-1]);
		JustDoIt(5,500);
		if(i == 3)
		{
			time_t cur_time=time(NULL);
			tm *l_time=localtime(&cur_time);
			SetDate(l_time->tm_mday,l_time->tm_mon+1,l_time->tm_year-100);
			SetTime(l_time->tm_hour,l_time->tm_min,l_time->tm_sec);
		}
	}
	return 0;
}

int ElvisRegVer3::GetSmenaNumber(unsigned long *KKMNo,unsigned short *KKMSmena,time_t * KKMTime)
{
	int er = GetMode();
	if (er!=0)
		return er;
	short mask = 0x01;

	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	tm reg_tm;
	reg_tm.tm_year	= DataN[27] + 100; // c 1900
	reg_tm.tm_mon	= DataN[26] - 1; // c 0
	reg_tm.tm_mday	= DataN[25];
	reg_tm.tm_hour	= DataN[28];
	reg_tm.tm_min	= DataN[29];
	reg_tm.tm_sec	= DataN[30];
	reg_tm.tm_isdst = t->tm_isdst;
	//����� ���
	*KKMTime        = mktime(&reg_tm);
	//��������� �����
	*KKMNo=*(unsigned long *)(DataN + 32);//����� ���
	//����� ��������� �������� �����
	*KKMSmena=*(unsigned short *)(DataN + 36);

	// +dms ======= ��� �� � �������� ����
	if(!*KKMSmena) *KKMSmena=CashRegister::ConfigTable->GetLongField("LAST_SMENA");
	// -dms =======

	return 0;
}

void ElvisRegVer3::SyncDateTime(void)
{
;;Log("Synchronization date/time");

    if (!ConnectionStatus)
        return;

	int er = GetMode();
	if (er != 0)
		return;

//	������ ������� � ���� (����� GetMode())
	time_t sys_time = time(NULL);
	tm* sys_tm = localtime(&sys_time);
	tm reg_tm;
	reg_tm.tm_year	= DataN[27] + 100; // c 1900
	reg_tm.tm_mon	= DataN[26] - 1; // c 0
	reg_tm.tm_mday	= DataN[25];
	reg_tm.tm_hour	= DataN[28];
	reg_tm.tm_min	= DataN[29];
	reg_tm.tm_sec	= DataN[30];
	reg_tm.tm_isdst = sys_tm->tm_isdst;
	time_t reg_time = mktime(&reg_tm);

    long delta = abs(reg_time - time(NULL)); // �����������
    if (delta <= 30) //  � �������� ����� ~30 ���
        return;
	if( delta <= 300 )
	{// (������ ������ - ���� �����)

          //��������� ����/������� ��
        if((DataN[15] & 0x0f) == 4)
        {// ����� ������� - ����� ����������������
            ;;Log("... ������������� ��");
            time_t cur_time=time(NULL);
            tm *l_time=localtime(&cur_time);
            SetDate(l_time->tm_mday,l_time->tm_mon+1,l_time->tm_year-100);
            SetTime(l_time->tm_hour,l_time->tm_min,l_time->tm_sec);
        }

//        //��������� ����/������� �������
//		char cmd[256];
//		sprintf( cmd, "echo \"12345\" | sudo -S date -s \"%02d/%02d/%02d %02d:%02d:%02d\"", reg_tm.tm_mon + 1, reg_tm.tm_mday, reg_tm.tm_year%100, reg_tm.tm_hour, reg_tm.tm_min, reg_tm.tm_sec );
//		;;Log(cmd);
//		system(cmd);
	}
    else
    {
        // ������ �� ������, ������ �������
		char msg[1024];

		sprintf( msg, "����������� ������� � �� ����� ���� �����:\n\n\
		 - ����������� : %02d.%02d.%02d     %02d:%02d:%02d\n\
		 - �������         : %02d.%02d.%02d     %02d:%02d:%02d\n\n�������������� ������������� ����������!",
		reg_tm.tm_mday, reg_tm.tm_mon + 1, reg_tm.tm_year%100, reg_tm.tm_hour, reg_tm.tm_min, reg_tm.tm_sec,
        sys_tm->tm_mday, sys_tm->tm_mon + 1, sys_tm->tm_year%100, sys_tm->tm_hour, sys_tm->tm_min, sys_tm->tm_sec);

		//;;Log(msg);

        ::ShowMessage(qApp->focusWidget(), msg );
    }

}

void ElvisRegVer3::CntLog(string str)
{

	time_t t;
	time( &t );
	struct tm *now = localtime( &t );

	fprintf(cLog,"%02d.%02d.%02d %02d:%02d:%02d> | %s \n", now->tm_mday, now->tm_mon+1, now->tm_year + 1900, now->tm_hour, now->tm_min, now->tm_sec, str.c_str());
	fflush(cLog);

}


void ElvisRegVer3::CntLog(const char * n, double p, double v)
{

	double s = RoundToNearest(p * v, .01);
	char str[251];
	memset(str, ' ', 250);
	str[250] = 0;
	if(fabs(v - 1) > .001){
		CntLog(n);
		sprintf(str, "%-9.3f X %-9.2f", v, p);
	}else{
		strncpy(str,n,40);
	}

	string Capt=str;
	string Sum=" ="+GetRounded(s,0.01);
	int Len=Capt.length()+Sum.length();

	if (LineLength>Len){
		for (int i=0;i<LineLength-Len;i++)
			Capt+=' ';
	}else{
		Capt = Capt.substr(0, LineLength - Sum.length());
	}

	Capt+=Sum;

	CntLog(Capt);

}



void ElvisRegVer3::LogClose(double CheckSumm, double PaymentSumm[], int type_op)
{

// =================== ������ ������ ���������� ��������� �� ======================= //
// ������ ��������� ������������
    GetMode();

    int er=0;

    char str[LineLength];

// ��������
    int Operator = (int)DataN[2]; //���������!!!!

    int day, month, year, hour, minute, second;
// ����
    day    = (int)DataN[25];
    month  = (int)DataN[26];
    year   = (int)DataN[27];
// �����
    hour   = (int)DataN[28];
    minute = (int)DataN[29];
    second = (int)DataN[30];

// � ���
    int KKM = (*(int*)(DataN+32));
// ���
    int INN = (*(int*)(DataN+42));


// =================== ����� ������ ���������� ��������� �� ======================= //

    char StringForPrinting[22];
    char SpaceString[LineLength];
	int strSize=21;

    memset(StringForPrinting,0,strSize+1);
    er = ReadTable(2, Operator, 2);     // ������ ���������� ���������
    //if (er!=0) return er;
    strncpy(StringForPrinting,(char*)(DataN+2),strSize);

    memset(SpaceString,' ',LineLength-2);
    SpaceString[LineLength-1]=0;

    int spacelen = LineLength -14- strlen(StringForPrinting);
    if (spacelen >=0) SpaceString [spacelen]=0;
    else SpaceString[1]=0;


// ���, ���
    sprintf(str, "��� %08d ��� %012d", KKM, INN);
    CntLog(str);

// ����, �����
    sprintf(str, "%02d.%02d.%02d %02d:%02d%s%s", day, month, year, hour, minute, SpaceString,StringForPrinting);
    CntLog(str);


	switch(type_op)
	{
		case SL:
			CntLog("�������");
			CntLog(GetPriceStr("", CheckSumm, true));
		break;
		case RT:
			CntLog("�������");
			CntLog(GetPriceStr("", CheckSumm, true));
		break;
		default:
			return ;
	}

//	if(er) return ;

	CntLog(GetPriceStr("����",CheckSumm, true));

// ������
    for(int i=0;i<4;i++)
    {
        if (fabs(PaymentSumm[i])>.01)
        {
            char PaymentName[40];
            int strSize=41;

            memset(PaymentName,0,strSize+1);
//{" ���������"," ��������"," �����"," ����. ������"};
            er = ReadTable(5, i+1, 1);     // ������������ ����� ������
            //if (er!=0) return er;
            strncpy(PaymentName,(char*)(DataN+2),strSize);

            CntLog(GetPriceStr(PaymentName, PaymentSumm[i], true));

        }
    }

    // ���� ���� �����
    if (PaymentSumm[0]>.01)
    {
        double nal = PaymentSumm[0] + PaymentSumm[1] + PaymentSumm[2] + PaymentSumm[3] - CheckSumm;
        if ( nal >.01) CntLog(GetPriceStr(" �����", nal, true));
    }
	//CheckSumm = 0;

    //"====================================="
    char buf[LineLength+1];
    memset(buf,'*',LineLength);
    buf[LineLength+1] = '\0';
    buf[(int)(LineLength)/2-1] = '�';
    buf[(int)(LineLength)/2] = '�';
    //"====================================="
    CntLog(buf);
    CntLog("");

}


//!string ElvisRegVer3::GetPriceStr(const char *str,double price, bool withdot=false)
string ElvisRegVer3::GetPriceStr(const char *str,double price, bool withdot)
{
	string Capt=str;
	string Sum=" ="+GetRounded(price,0.01);


	if (withdot)
	{
	for(int i=0;i<Sum.length();i++)
	   if (Sum[i]==',') Sum[i] = '.';
	}

    int tLineLength = LineLength;

	int Len=Capt.length()+Sum.length();

	if (tLineLength>Len){
		for (int i=0;i<tLineLength-Len;i++)
			Capt+=' ';
	}else{
		Capt = Capt.substr(0, tLineLength - Sum.length());
	}

	Capt+=Sum;

    return Capt;

}
