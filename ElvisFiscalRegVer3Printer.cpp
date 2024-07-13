/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <qstring.h>
#include <qapplication.h>
#include <qinputdialog.h>
#include <xbase/xbase.h>

#include "ElvisFiscalRegVer3Printer.h"
#include "Utils.h"
#include "Cash.h"
#include "DbfTable.h"
#include "Payment.h"

#define	STR_PRINT
#define REG_DEBUG

enum {STX=0x02,ETX=0x03,EOT=0x04,ENQ=0x05,ACK=0x06,DLE=0x10,NAK=0x15,};

//Shtrich-M version 3

unsigned long ElvisRegVer3Printer::SerialNumber(void)
{
	if(GetMode())
		return 0;
	return *(unsigned long *)(DataN + 32);
}

ElvisRegVer3Printer::ElvisRegVer3Printer(void)
{
	RegCom=0;
	MCU=100;
	MinDuration=50;
	DataNLen=128;
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

	sum = sum0 = sum1 = sum2 = 0;

	DBArchPath="arch";
}

ElvisRegVer3Printer::~ElvisRegVer3Printer(void)
{
	if (RegCom!=0)
		ClosePort(RegCom);
}

int ElvisRegVer3Printer::ChangeRegBaud(int port,int oldbaud, int baud,int type,const char *passwd)
{
    if (RegCom!=0)
    {
        ClosePort(RegCom);
        RegCom = 0;
    }

    InitReg(port, oldbaud, type, passwd);


	RegPort=port;
	RegBaud=baud;
	RegType=type;
	RegTimeout=2000;
	RegPassword=atoi(passwd);

	if (RegCom!=0)
	{
	    Log("+change port");
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
		Log("-change port");
	}

	return ResetReg();
}

int ElvisRegVer3Printer::InitReg(int port,int baud,int type,const char *passwd)
{
    ;;Log("InitReg");
    ConnectionStatus = true;

	WaitPeriod=MinDuration;

	RegPort=port;
	RegBaud=baud;
	RegType=type;
	RegTimeout=2000;
	RegPassword=atoi(passwd);

	if (RegCom!=0)
	{
	    ;;Log("if (RegCom!=0)");
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
		ProcessError(JustDoIt(8,MinDuration));
		ClosePort(RegCom);
		RegCom=0;
	}

	return ResetReg();
}

int ElvisRegVer3Printer::JustDoIt(unsigned int len,int timeout)
{
	memmove(DataN+2,DataN,len);
	DataN[0]=STX;
	DataN[1]=len;
	for (int i=0;i<=len;i++)
		DataN[len+2]^=DataN[i+1];

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

		if(er >= 0)
			yes = true;

		if(call || !yes)
			break;

		if((er<0) || (er == 0x67) || (er == 0x80) || (er == 0x81) || (er == 0x82) || (er == 0x83))
		{
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

		//;;Log("�����������: �������", store, DataNLen);
		//;;Log("�����������: �����", DataN, DataNLen);


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
		//;;Log("�����������: ResetReg", DataN, DataNLen);

		memcpy(DataN, store, DataNLen);
	}

	WaitPeriod=MinDuration;
	return er;
}

int ElvisRegVer3Printer::ProcessInstruction(int rev)
{
	int i = 3;

	bool prev = true;
	do{
		unsigned char b;

		ClearPort(RegCom);

		b=ENQ;
		WriteToPort(RegCom,&b,1);
#ifdef REG_DEBUG
		long savepos = ftell(log);


		fprintf(log,"\n\nStart\n%s\t WriteToPort5 %d\n", GetCurDateTime(), b);
		fflush(log);
#endif
		{
			for(int j = 6;;){//10 ������ ������� ����
				if(!--j)
					return -1;
#ifdef REG_DEBUG

				fprintf(log,"%s\t j = %d\n", GetCurDateTime(), j);
				fflush(log);
#endif
				if(ReadFromPort(RegCom,&b,1)){
#ifdef REG_DEBUG

					fprintf(log,"%s\t ReadFromPort21 %d\n", GetCurDateTime(), b);
					fflush(log);
#endif
					break;
				}
		}
		}
		if(b == NAK){
			prev = false;
			for(int j = 11;;){
				if(!--j)
					return -2;
				WriteToPort(RegCom,DataN,DataN[1]+3);
#ifdef REG_DEBUG

				fprintf(log,"%s\t WriteToPort DataN\n", GetCurDateTime());
				fwrite(DataN, sizeof(char), DataN[1]+3, log);
				fprintf(log,"<end, bytes: %d>\n", DataN[1]+3);
				fflush(log);
#endif
				for(int ii = 5; --ii;){// 10 ������ ���� ������������� ������
					if(ReadFromPort(RegCom,&b,1)){
#ifdef REG_DEBUG

						fprintf(log,"%s\t ReadFromPort6 %d\n", GetCurDateTime(), b);
						fflush(log);
#endif
						break;
					}
				}
				if(b == ACK)
					break;
			}
		}else if(b != ACK){
			while(ReadFromPort(RegCom,&b,1)){
#ifdef REG_DEBUG

				fprintf(log,"%s\t ReadFromPort %d\n", GetCurDateTime(), b);
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

				fprintf(log,"%s\t j = %d\n", GetCurDateTime(), j);
				fflush(log);
#endif
				for(int ii = 5; --ii;){// 10 ������ ���� ����
#ifdef REG_DEBUG

					fprintf(log,"%s\t ii = %d\n", GetCurDateTime(), ii);
					fflush(log);
#endif
					if(ReadFromPort(RegCom,&b,1)){
#ifdef REG_DEBUG

						fprintf(log,"%s\t ReadFromPort2 %d\n", GetCurDateTime(), b);
						fflush(log);
#endif
						break;
					}
				}
				if(b == STX){
#ifdef REG_DEBUG

					fprintf(log,"%s\t ReadedFromPortSTX\n", GetCurDateTime());
					fflush(log);
#endif
					break;
				}
			}
		}
		unsigned char len;
		if (ReadFromPort(RegCom,&len,1)==0){
#ifdef REG_DEBUG

			fprintf(log,"%s\t return -3;\n", GetCurDateTime());
			fflush(log);
#endif
			return -3;//-1;
		}
		{
			char buf[1024];
			unsigned char CRC=len;
			for (int j=0;j<len;j++){
				if (ReadFromPort(RegCom,&b,1)==0){
#ifdef REG_DEBUG

					fprintf(log,"%s\t return -4;\n", GetCurDateTime());
					fflush(log);
#endif
					return -4;//-1;
				}
				CRC ^= (buf[j] = b);
			}
			fwrite(buf, sizeof(char), len, log);
			if(ReadFromPort(RegCom,&b,1)==0 || CRC!=b)
			{
#ifdef REG_DEBUG

				fprintf(log,"%s\t return -5;\n", GetCurDateTime());
				fflush(log);
#endif
				b=NAK;
				WriteToPort(RegCom,&b,1);
				continue;
//				return -5;//-2;
			}
//			switch(buf[1]){};
			memset(DataN,0,DataNLen);
			memcpy(DataN,buf,len);
			DataN[len]=0;
		}
		b=ACK;
		WriteToPort(RegCom,&b,1);
		if(prev)
			continue;
		//WaitSomeTime(WaitPeriod);
#ifdef REG_DEBUG

		fprintf(log,"%s\t return %d;\n", GetCurDateTime(), DataN[1]);
		if(! DataN[1]){
			//fsetpos(log, &savepos);
			fseek(log, savepos, SEEK_SET);
		}
		fflush(log);
#endif
		return DataN[1];//0;
while_end:;
	}while(--i);

#ifdef REG_DEBUG

	fprintf(log,"%s\t return -6;\n", GetCurDateTime());
	fflush(log);
#endif
	return -6;
}


int ElvisRegVer3Printer::ResetReg(void)
{

	if (RegCom!=0)
		ClosePort(RegCom);

	if (RegPort*RegBaud!=0)
	{
	    ;;Log("if (RegPort*RegBaud!=0)");
	    RegCom=OpenPort(RegPort,RegBaud,0,RegTimeout);
	    ;;Log("after OpenPort(RegPort,RegBaud,0,RegTimeout);");
	}
	else
	{
	    ;;Log("l1 FALSE");
	    RegCom=0;
	}

	if (RegCom==0)
		return -1;

;;Log("before switch (RegType)");
	switch (RegType)//according to register's type set line length and speed of printing
	{
		case ShtrichVer3Type:
			LineLength=36;
			RegSpeed=75;
			EntireCheck=false;
		break;
		case ShtrichVer3EntireType:
			LineLength=36;
			RegSpeed=75;
			EntireCheck=true;
		break;
		case ShtrichVer3EntirePrinterType:
			LineLength=36;
			//LineLength=40;
			RegSpeed=75;
			EntireCheck=true;
		break;
		case ShtrichVer3PrinterType:
			LineLength=36;
			RegSpeed=75;
			EntireCheck=false;
		break;
		default:
			LineLength=36;
			RegSpeed=75;
			EntireCheck=false;
		break;
	}

	ClearPort(RegCom);
;;Log("before RestorePrinting");
	bool restore;
	int er = RestorePrinting(restore);
;;Log("after RestorePrinting");
	if (er!=0)
		return er;

	if (restore)
	{
		return ContinuePrinting();
	}
	else
	{
		return 0;
	}
}

string ElvisRegVer3Printer::GetErrorStr(int er,int lang)
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
			case   -1: return "��� ����� � �������������";
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
			default:   return "Unknown error";
		}

	return "";
}

bool ElvisRegVer3Printer::IsPaperOut(int er)
{
	return ((er==0x6B)||(er==0x6C));
/*
	if ((er==0x6B)||(er==0x6C))
		return true;
	else
		return false;
*/
}

bool ElvisRegVer3Printer::IsFullCheck(void)
{
	return EntireCheck;
}

bool ElvisRegVer3Printer::RestorePaperBreak(void)
{
	return true;
}

int ElvisRegVer3Printer::Cancel(void)
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
		fprintf(log,"%s;%s;������������� ����\n", d, t);
*/


		fprintf(log,"%s\t������������� ����\n", GetCurDateTime());

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

int ElvisRegVer3Printer::GoodsStr(const char * n, double p, double v, double s)
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

int ElvisRegVer3Printer::PaymentStr(PaymentInfo payment)
{
	char str[41];
	memset(str, ' ', 40);
	str[40] = 0;

	if((payment.Type == 10) && (payment.Summa > 0))
	{
		snprintf(str,40,"���������� #%s", payment.Number.c_str(), payment.Summa);
		return CreatePriceStr(str, payment.Summa);
	}

	if((payment.Type == 11) && (payment.Summa > 0))
	{
		snprintf(str,40,"\"�������\" �� ��������� ", payment.Summa);
		return CreatePriceStr(str, payment.Summa);
	}

	return 0;
}

// �������� ������� �������� �������� ������������ (���������� ����)
// �� ���������� � �������� �������
bool ElvisRegVer3Printer::ControlPaymentTicket(string *PaymentHeader, int CashDesk, int CheckNum, string CashmanName )
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
            if((crpayment.Type == 10) && (crpayment.Summa > 0))
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

//int ElvisRegVer3Printer::CreateInfoStr(int CashDesk,int CheckNum,int PaymentType,string CheckHeader,string info)
//{
//
//	PrintCheckHeader(CheckHeader);
//	//PrintVote(PaymentType,vote);// �����������
//
//	PrintInfo(info);// �������������� ����������
//
//	string FirstStr,SecondStr,SpaceStr;
//	FirstStr="����� #"+Int2Str(CashDesk);
//	SecondStr="�������� ��� #"+Int2Str(CheckNum);
//	if (LineLength>FirstStr.length()+SecondStr.length())
//		for (unsigned int i=0;i<LineLength-FirstStr.length()-SecondStr.length();i++)
//			SpaceStr+=' ';
//
//	return PrintString((FirstStr+SpaceStr+SecondStr).c_str());
//}

int ElvisRegVer3Printer::Close(double Summ,double Disc,int CashDesk,int CheckNum,int PaymentType, long long CustomerCode, string CashmanName,string CheckHeader,string info, bool isHandDisc, string CheckEgaisHeader, string egais_url, string egais_sign, int ModePrintQRCode)
{
//;;  char fn[] = "./upos/p";
//;;	PrintPCheck(fn, false);

	int er=0;
	PaymentInfo tmpPayment;

	printf("Summ to close %.2f\n",Summ);

	switch(PaymentType)
	{
		case CashPayment:
			tmpPayment.Type=0;
			break;
		default:
			tmpPayment.Type=3;
			break;
	}
	tmpPayment.Summa=Summ;
	Payment.push_back(tmpPayment);

    string PaymentHeader;
    bool CrPaymentFlag = ControlPaymentTicket(&PaymentHeader, CashDesk, CheckNum, CashmanName);

    if (CrPaymentFlag) CheckHeader = PaymentHeader + CheckHeader;

   // CheckHeader = "printheader\n"  + CheckHeader;

	er = CreateInfoStr(CashDesk,CheckNum,PaymentType,CheckHeader,info);
	if (er) return er;

	return Close(Disc,CashDesk,CheckNum,CustomerCode,CashmanName,CheckHeader,info, isHandDisc);
}

int ElvisRegVer3Printer::Close(double Disc,int CashDesk,int CheckNum,long long CustomerCode, string CashmanName,string vote,string info, bool isHandDisc)
{
//	SetInstrHeader(0x13);
//	int err = JustDoIt(5,MinDuration);
//	fprintf(log,"Error sound: %d\n",err);
	int SummToClose;
	double CheckSumm;
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

        CheckSumm = 0;
        sum=0;

		bool FirstRecord=true;
		while(!Sales.empty())
		{
			SalesRecord tmpSales=Sales[0];
#ifdef STR_PRINT

            //er = CreatePriceStr(tmpSales.Name.c_str(), tmpSales.Sum);
#ifndef _PRINT_FULL_PRICE_
 			er = GoodsStr(tmpSales.Name.c_str(),tmpSales.Price,tmpSales.Quantity,tmpSales.Sum);
#else
            er = GoodsStr(tmpSales.Name.c_str(),tmpSales.FullPrice,tmpSales.Quantity,tmpSales.FullSum);
#endif
			if(er)
				return er;

            // ����� ���������� �� �������������
            if (!tmpSales.Manufacture.empty())
            {
                er = PrintString(tmpSales.Manufacture.c_str());
                if(er) return er;
            }

			sum += RoundToNearest(tmpSales.Sum,.01);//floor(tmpSales.Price * tmpSales.Quantity * 100) / 100.f;

			CheckSumm += RoundToNearest(tmpSales.Sum,.01);
			type_op = tmpSales.Type;
#else

			switch (tmpSales.Type)
			{
				case SL:

                    er = CreatePriceStr(tmpSales.Name.c_str(), tmpSales.Price * tmpSales.Quantity);
					//er=SaleGoods(tmpSales.Name.c_str(),tmpSales.Price,
					//	tmpSales.Quantity,tmpSales.Department,false);
					break;
				case ST:
					er=StornoGoods(tmpSales.Name.c_str(),tmpSales.Price,
						tmpSales.Quantity,tmpSales.Department);
					break;
				case RT:
					er=ReturnGoods(tmpSales.Name.c_str(),tmpSales.Price,
						tmpSales.Quantity,tmpSales.Department,false);
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
//		if (EntireCheck)
		{

			//er = GetCheckSum(CheckSum);
			//if (er!=0)
			//	return er;


			er=CreatePriceStr("����� ��� ������",sum + Disc);
			if (er!=0)
				return er;

			for(unsigned int i=0;i<Payment.size();i++)
			{
				er = PaymentStr(Payment[i]);
				if(er)
					return er;
			}


			if(fabs(Disc - FiscalCheque::tsum) > 0.005)
			{
				string tmpStr;
				if(CustomerCode) tmpStr = "������ �� ����� " + LLong2Str(CustomerCode);
					else tmpStr = "������";
				er=CreatePriceStr(tmpStr.c_str(), Disc - FiscalCheque::tsum- FiscalCheque::CertificateSumm);
				if (er!=0)
					return er;

                 if(isHandDisc) er = PrintString("*�������������� ������ �� �����", false);

			}

			if(FiscalCheque::tsum > .005)
			{
				er = CreatePriceStr("������-����������",FiscalCheque::tsum);
				if (er!=0)
					return er;
			}
		}
/*		else
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
*/
	}

#ifdef STR_PRINT
	//WaitSomeTime(200);
/*	switch(type_op)
	{
		case SL:
			er = SaleGoods("",sum,1,0,false);
			if(er == 6)
				er = SaleGoods("",sum,1,0,false);
		break;
		case RT:
			er = ReturnGoods("",sum,1,0,false);
			if(er == 6)
				er = ReturnGoods("",sum,1,0,false);
		break;
		default:
			return -1;
	}

	if(er)
		return er;
	sum = 0;
*/
	//WaitSomeTime(200);
#endif

	double PaymentSumm[4];
	int    PaymentSummToClose[4];
	PaymentSumm[0]=PaymentSumm[1]=PaymentSumm[2]=PaymentSumm[3]=0;
	PaymentSummToClose[0]=PaymentSummToClose[1]=PaymentSummToClose[2]=PaymentSummToClose[3]=0;

	for(unsigned int i=0;i<Payment.size();i++)
	{
		if((Payment[i].Type>=0) && (Payment[i].Type<=3))
			PaymentSumm[Payment[i].Type]+=Payment[i].Summa;
	}

	SummToClose=0;
	for(unsigned int i=0;i<4;i++)
	{
		SummToClose+=PaymentSummToClose[i]=Double2Int(PaymentSumm[i]*MCU);
		printf("PaymentSummToClose[%d]=%d\n",i,PaymentSummToClose[i]);
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

	// + dms  �� ��� �������
	//LogOpRegister();

// + dms  �� ��� �������
//	SetInstrHeader(0x85);
// -dms

	//LogOpRegister();
	//����� ����������� ���������� ����� ���

// + dms  �� ��� �������
//	for(unsigned int i=0;i<4;i++)
//	{
//		(*(int*)(DataN+5+i*5))=(PaymentSummToClose[i])%0xFFFFFFFF;
//	}
// -dms



   //! === information === !//
    PrintInfo(info);



	//fprintf(log,"%s;������ �������� ����; ������: %d; �����: %d; ���: %d; ��� ������: %d\n", ctime( &t ), SummToClose, CashDesk, CheckNum, PaymentType);
	fprintf(log,"%s\t������ �������� ����; ������: %d; �����: %d; ���: %d; ��� ������: -1\n", GetCurDateTime(), SummToClose, CashDesk, CheckNum);

// + dms  �� ��� �������
//	er = ProcessError(JustDoIt(71,RegSpeed*6));
	//er==0;
// - dms

    er = CloseCheck(CheckSumm, PaymentSumm, type_op);

// + dms  �� ��� �������
   // SpaceAndCutCheck();

   //! === information === !//
    //PrintInfo(info);

    PrintHeader();

    CutCheck();
//+ dms  �� ��� �������
	if (er==0)
	{
		CashRegister::ConfigTable->PutField("CHECKOPEN","F");


//		//fprintf(log,"%s;�������� ����; ������: %d; �����: %d; �����: %d; ���: %d; ��� ������: %d\n", ctime( &t ), SummToClose, (*(int*)(DataN+3)), CashDesk, CheckNum, PaymentType);
		fprintf(log,"%s\t�������� ����; ������: %d; �����: %d; �����: %d; ���: %d; ��� ������: -1\n", GetCurDateTime(), SummToClose, (*(int*)(DataN+3)), CashDesk, CheckNum);
		StartClosing=false;
	}
// + dms  �� ��� �������

//    GetMode();
	if (frSound){
		SetInstrHeader(0x13);
		JustDoIt(5,MinDuration);
	}



//	PrintVote();
// + dms  �� ��� �������
	//LogOpRegister();
// - dms
	return er;
}

int ElvisRegVer3Printer::CloseCheck(double CheckSumm, double PaymentSumm[], int type_op)
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


    //if(!INN) INN=9718015419;

// ���, ���
    sprintf(str, "��� %08d ��� %012d", KKM, INN);
    er = PrintString(str ,false);

// ����, �����
    sprintf(str, "%02d.%02d.%02d %02d:%02d%s%s", day, month, year, hour, minute, SpaceString,StringForPrinting);
    er = PrintString(str ,false);


	switch(type_op)
	{
		case SL:
			er = PrintString("�������" ,false);
			er = CreatePriceStr("", CheckSumm, true);
		break;
		case RT:
			er = PrintString("�������" ,false);
			er = CreatePriceStr("", CheckSumm, true);
		break;
		default:
			return -1;
	}

	if(er) return er;


	er = CreatePriceStr("����",CheckSumm, true, true);
	//PrintBoldString

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

            er = CreatePriceStr(PaymentName, PaymentSumm[i], true);
            if (er) return er;
        }
    }

    // ���� ���� �����
    if (PaymentSumm[0]>.01)
    {
        double nal = PaymentSumm[0] + PaymentSumm[1] + PaymentSumm[2] + PaymentSumm[3] - CheckSumm;
        if ( nal >.01) er = CreatePriceStr("�����", nal, true);
    }
	//CheckSumm = 0;

    //"====================================="
    char buf[LineLength+2];
    memset(buf,'=',LineLength);
    buf[LineLength+1] = '\0';

    er = PrintString(buf);

    er = PrintString("");
    er = PrintString("");
    // �������
    //"_____________________________________"
    memset(buf,'_',LineLength);
    buf[LineLength+1] = '\0';
    er = PrintString(buf);

    er = PrintString("");

    return er;
}

int ElvisRegVer3Printer::CreateBoldSummaStr(const char *str,double price)
{
	string Capt=str;
	string Sum=" ="+GetRounded(price,0.01);

	for(int i=0;i<Sum.length();i++)
	   if (Sum[i]==',') Sum[i] = '.';

	int Len=Capt.length()+Sum.length();
    int LineLength = 18;

	if (LineLength>Len){
		for (int i=0;i<LineLength-Len;i++)
			Capt+=' ';
	}else{
		Capt = Capt.substr(0, LineLength - Sum.length());
	}

	Capt+=Sum;

	return PrintBoldString(Capt.c_str(), false);
}

int ElvisRegVer3Printer::PrintBoldString(const char* str,bool ctrl)
{
	SetInstrHeader(0x12);
	DataN[5] = ctrl ? 0x03 : 0x02;
	strncpy((char*)DataN+6,*str?str:" ",20);



	fprintf(log,"%s\t PrintBoldString %s\n", GetCurDateTime(), str);
	fflush(log);

	return ProcessError(JustDoIt(26,0));
}


//!int ElvisRegVer3Printer::CreatePriceStr(const char *str,double price, bool withdot=false, bool bold=false)
int ElvisRegVer3Printer::CreatePriceStr(const char *str,double price, bool withdot, bool bold)
{
	string Capt=str;
	string Sum=" ="+GetRounded(price,0.01);

	if (withdot)
	{
	for(int i=0;i<Sum.length();i++)
	   if (Sum[i]==',') Sum[i] = '.';
	}

    int tLineLength = LineLength;
    if (bold) tLineLength = 18;

	int Len=Capt.length()+Sum.length();

	if (tLineLength>Len){
		for (int i=0;i<tLineLength-Len;i++)
			Capt+=' ';
	}else{
		Capt = Capt.substr(0, tLineLength - Sum.length());
	}

	Capt+=Sum;

    int er=0;
    if (bold)
    	er = PrintBoldString(Capt.c_str(), false);
    else
    	er = PrintString(Capt.c_str());

    return er;
}

int ElvisRegVer3Printer::Sale(const char* Name,double Price,double Quantity,int Department)
{
	if (EntireCheck)
		return 0;
	return SaleGoods(Name,Price,Quantity,Department,true);
}

int ElvisRegVer3Printer::SaleGoods(const char* Name,double Price,double Quantity,int Department,bool Check1stEntry)
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
	SetParams(Name,Price,Quantity,0);

	int ret = ProcessError(JustDoIt(60,print_timeout+RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	if(!ret){


		fprintf(log,"%s\t�������; %s: %f X %f\n", GetCurDateTime(), Name, Price, Quantity);
	}
	return ret;
}

int ElvisRegVer3Printer::StornoGoods(const char* Name,double Price,double Quantity,int Department)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x84;
	SetParams(Name,Price,Quantity,0);
	int ret = ProcessError(JustDoIt(60,RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	if(!ret){


		fprintf(log,"%s\t������; %s: %f X %f\n", GetCurDateTime(), Name, Price, Quantity);
	}
	return ret;
}

int ElvisRegVer3Printer::Storno(const char* Name,double Price,double Quantity,int Department)
{
	if (EntireCheck)
		return 0;
	return StornoGoods(Name,Price,Quantity,Department);
}

int ElvisRegVer3Printer::ReturnGoods(const char* Name,double Price,double Quantity,int Department,bool Check1stEntry)
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
	SetParams(Name,Price,Quantity,0);

	int ret = ProcessError(JustDoIt(60,print_timeout+RegSpeed*((fabs(Quantity-1)<epsilon)?2:3)));
	if(!ret)
	{


		fprintf(log,"%s\t�������; %s: %f X %f\n", GetCurDateTime(), Name, Price, Quantity);
	}
	return ret;
}

int ElvisRegVer3Printer::Return(const char* Name,double Price,double Quantity,int Department)
{
	if (EntireCheck)
		return 0;
	return ReturnGoods(Name,Price,Quantity,Department,true);
}

void ElvisRegVer3Printer::OpenDrawer(void)
{
	SetInstrHeader(0x28);
	JustDoIt(6,MinDuration);
}

int ElvisRegVer3Printer::CashIncome(double sum)
{
	SetInstrHeader(0x50);
	*((int*)(DataN+5))=Double2Int(sum*MCU)%0xFFFFFFFF;
	int er = ProcessError(JustDoIt(10,RegSpeed*5));

//	if (er==0)
//		CutCheck();
	return er;
}

int ElvisRegVer3Printer::CashOutcome(double sum)
{
	SetInstrHeader(0x51);
	*((int*)(DataN+5))=Double2Int(sum*MCU)%0xFFFFFFFF;
	int er = ProcessError(JustDoIt(10,RegSpeed*5));
//	if (er==0)
//		CutCheck();
	return er;
}

int ElvisRegVer3Printer::GetCurrentSumm(double& sum)
{
	return GetMoneyRegister(241,sum);
}

int ElvisRegVer3Printer::XReport(void)
{
    double sum;
    return Report(sum, false);
}

void ElvisRegVer3Printer::SetDBArchPath(string path)
{
	DBArchPath = path;
}

int ElvisRegVer3Printer::ZReport(double& sum)
{
    int er;
    return Report(sum, true);

    // FR Z-Report

	SetInstrHeader(0x41);
	er = JustDoIt(5,1000);
	if (er!=0)
		return er;

	do{
		WaitSomeTime(2000);
		er = GetMode();
		if (er!=0)
			break;
	}while((DataN[0x10]==4)||(DataN[0x10]==5));

	er = ProcessError(er);

	if(er)
		return er;

	bool t;
	RestorePrinting(t); // �������������� ������������� ������� ����� ������

	return er;
}

int ElvisRegVer3Printer::Report(double& sum, bool z)
{

	double val;
	sum = 0.;
	time_t ltime;
//	time( &ltime );
//	fprintf(log,"%s %s", ctime(&ltime),"Z.�������� ��������:\n");
//
/*	int er = GetMoneyRegister(193,val);
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
//	}nick
	SetInstrHeader(0x41);
	er = JustDoIt(5,1000);
	if (er!=0)
		return er;


	do{
		WaitSomeTime(2000);
		er = GetMode();
		if (er!=0)
			break;
	}while((DataN[0x10]==4)||(DataN[0x10]==5));
*/


   // ������ ��������� ������������
    GetMode();

    int er=0;

    char str[LineLength];

// ��������
    int Operator = (int)DataN[2]; //

    int day, month, year, hour, minute, second;
    if (report.Date.tm_mday)
    {
    // ����
        day    = report.Date.tm_mday;
        month  = report.Date.tm_mon+1;
        year   = report.Date.tm_year+1900;
    // �����
        hour   = report.Date.tm_hour;
        minute = report.Date.tm_min;
        second = report.Date.tm_sec;
    }
    else
    {
    // ����
        day    = (int)DataN[25];
        month  = (int)DataN[26];
        year   = (int)DataN[27];
    // �����
        hour   = (int)DataN[28];
        minute = (int)DataN[29];
        second = (int)DataN[30];
    }
// � ���
    int KKM = (*(int*)(DataN+32));
//���
    int INN = (*(int*)(DataN+42));

    //if(!INN) INN=09718015419;


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
    er = PrintString(str ,false);

// ����, �����
    sprintf(str, "%02d.%02d.%02d %02d:%02d%s%s", day, month, year, hour, minute, SpaceString,StringForPrinting);
    er = PrintString(str ,false);

    er = PrintString(GetFormatString(z?"�������� ����� � ��������":"�������� ����� ��� �������", "#"+Int2Str(report.KKMSmena), LineLength).c_str() ,false);

    int PaymentCnt = 4;
    string PaymentName[PaymentCnt];

// ������
    for(int i=0;i<PaymentCnt;i++)
    {
            char Name[40];
            int strSize=41;

            memset(Name,0,strSize+1);
//{" ���������"," ��������"," �����"," ����. ������"};
            er = ReadTable(5, i+1, 1);     // ������������ ����� ������
            if (er) return er;
            strncpy(Name,(char*)(DataN+2),strSize);

            string str(Name,40);
            PaymentName[i]+=" "+str;

    }


// ����� ������
    char strCnt[5];
    sprintf(strCnt, "%04d",report.TotalSalesCheckCount);
    string string_strCnt(strCnt);

    er = PrintString(GetFormatString("����� ������", string_strCnt, LineLength).c_str());
    if (er) return er;

    sprintf(strCnt, "%04d",report.SalesCheckCount);

    er = CreatePriceStr(strCnt, report.SalesSumma, true, true);
    if (er) return er;

    for ( int i=0; i<PaymentCnt;i++)
    {
        er = CreatePriceStr(PaymentName[i].c_str(), report.SalesDetail[i], true);
        sum+=report.SalesDetail[i]; // ����� ������
    }

// ����� �������
//    er = PrintString("����� �������" ,false);

// ����� ��������� ������
    sprintf(strCnt, "%04d",report.TotalReturnSalesCheckCount);
    string strRetCnt(strCnt);

    er =  PrintString(GetFormatString("����� ��������� ������",strRetCnt,LineLength).c_str());
    if (er) return er;

    sprintf(strCnt, "%04d", report.ReturnSalesCheckCount);

    er = CreatePriceStr(strCnt, report.ReturnSalesSumma, true);
    if (er) return er;

    for ( int i=0; i<PaymentCnt;i++)
    {
        er = CreatePriceStr(PaymentName[i].c_str(),  report.ReturnSalesDetail[i], true);
        ///sum+=report.SalesDetail[i]; // ����� ���������  (! �� ���� !)
    }

// ����� ��������� �������
//    er = PrintString("����� ��������� �������" ,false);

// ��������
    er = PrintString("��������",false);
    if (er) return er;

// ������
    er = PrintString("������",false);
    if (er) return er;

// �������������� �����
    er = PrintString(GetFormatString("�������������� �����","0000",LineLength).c_str());
    if (er) return er;

// ���������
    er = CreatePriceStr(z?"���������":"���. � �����",report.SalesDetail[0] - report.ReturnSalesDetail[0], false);
    if (er) return er;

// �������
    er = CreatePriceStr("�������",report.SalesSumma - report.ReturnSalesSumma, false);
    if (er) return er;

    //"====================================="
    char buf[LineLength+2];
    memset(buf,'=',LineLength);
    buf[LineLength+1] = '\0';

    er = PrintString(buf);

    if (z) er = PrintString("************����� �������***********");
    er = PrintString("");
//    // �������
//    //"_____________________________________"
//    memset(buf,'_',LineLength);
//    buf[LineLength+1] = '\0';
//    er = PrintString(buf);
//
//    er = PrintString("");


    if (z)
    {
        time( &ltime );
        struct tm *now;
        now = localtime( &ltime );
        char tstr[256];
        sprintf( tstr, "arch/fr%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
        fclose(log);
        rename("tmp/fr.log",tstr);
        fclose(fopen("tmp/fr.log","a"));
        log = fopen("tmp/fr.log","r+");
        fseek(log, 0, SEEK_END);

        // control
        char cstr[256];
        sprintf( cstr, "arch/control%d-%02d-%02d %02d_%02d_%02d.log", now->tm_year + 1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
        fclose(cLog);
        rename("tmp/control.log",cstr);
        fclose(fopen("tmp/control.log","a"));
        cLog = fopen("tmp/control.log","r+");
        fseek(cLog, 0, SEEK_END);
    }

    PrintHeader();
    CutCheck();

	er = ProcessError(er);

	if(er)
		return er;

	bool t;
	RestorePrinting(t); // �������������� ������������� ������� ����� ������

	return er;
}

int ElvisRegVer3Printer::SetTime(int hour,int minute,int sec)
{
	SetInstrHeader(0x21);
	DataN[5]=hour%24;
	DataN[6]=minute%60;
	DataN[7]=sec%60;
	return JustDoIt(8,150);
}

int ElvisRegVer3Printer::SetDate(int day,int month,int year)
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

int ElvisRegVer3Printer::PrintString(const char* str,bool ctrl)
{
    if (CashRegister::ConfigTable->GetLogicalField("FRK_FONT"))
    {
        return PrintStringWithFont(str, 5, ctrl);
    }
    else
    {

        SetInstrHeader(0x17);
        DataN[5] = ctrl ? 0x03 : 0x02;
        strncpy((char*)DataN+6,*str?str:" ",40);

        fprintf(log,"%s\t PrintString %s\n", GetCurDateTime(), str);
        fflush(log);

        CntLog(str);

        return ProcessError(JustDoIt(46,0));
    }
}

int ElvisRegVer3Printer::PrintStringWithFont(const char* str, unsigned short curfont, bool ctrl)
{
	SetInstrHeader(0x2F);
	DataN[5] = ctrl ? 0x03 : 0x02;
	DataN[6] = curfont;
	strncpy((char*)DataN+7,*str?str:" ",40);

	fprintf(log,"%s\t PrintString %s\n", GetCurDateTime(), str);
	fflush(log);

	CntLog(str);

	return ProcessError(JustDoIt(46,0));
}

void ElvisRegVer3Printer::WaitNextHeader(void)
{
	WaitSomeTime(RegSpeed*7);
}


int ElvisRegVer3Printer::PrintHeader(void)
{
    bool flag_print_null = true;
    bool is_nullstr = false;
    int er=0;
    int num_of_fields = 4;
    //int ar_fields_of_table [] = {4,5,6,8};
    int ar_fields_of_table [] = {6,7,8,9};

    char StringForPrinting[41];
	int strSize=40;

    //if (!flag_print_null) PrintString("");

    for (int i =0; i<num_of_fields; i++)
    {
        memset(StringForPrinting,0,strSize+1);
        int er = ReadTable(4, ar_fields_of_table[i], 1);
        if (er!=0) return er;

        strncpy(StringForPrinting,(char*)(DataN+2),strSize);

        is_nullstr = false;

        if (StringIsNull(StringForPrinting, strlen(StringForPrinting)))
        {
           if (flag_print_null)
            {
             er = PrintString(StringForPrinting);
             is_nullstr = true;
            }
        }
        else
        {
            er = PrintString(StringForPrinting);
        }

        if (er!=0) return er;
    }

    if (!is_nullstr) PrintString("");

	return er;
}

int ElvisRegVer3Printer::SpaceAndCutCheck(void)
{
    for (int i=0;i<4;i++) {PrintString("");}

    return CutCheck();
}

int ElvisRegVer3Printer::CutCheck(void)
{
	SetInstrHeader(0x25);
	*(DataN+5) = 1; // �������� ������� ������
	return JustDoIt(6,100);
}

void ElvisRegVer3Printer::SetParams(const char* Name,double Price,double Quantity,int Department)
{
	*((int*)(DataN+1))=RegPassword;
	*((int*)(DataN+5))=Double2Int(Quantity*1000);
	*((int*)(DataN+10))=Double2Int(Price*MCU);
	*((int*)(DataN+15))=Department%16;
	strncpy((char*)(DataN+20),Name,LineLength);
}

void ElvisRegVer3Printer::SetInstrHeader(unsigned char instr)
{
	memset(DataN,0,DataNLen);
	DataN[0]=instr;
	(*(int*)(DataN+1))=RegPassword;
}

int ElvisRegVer3Printer::GetMoneyRegister(int num,double& sum)
{
	SetInstrHeader(0x1A);
	DataN[5]=num;
	int er = JustDoIt(6,100);
	if (er!=0)
		return er;

	sum=(*(int*)(DataN+3))%0xFFFFFFFF*1.0/MCU;
	return 0;
}

int ElvisRegVer3Printer::GetOperationRegister(int num,int& sum)
{
	SetInstrHeader(0x1B);
	DataN[5]=num;
	int er = JustDoIt(6,100);
	if (er!=0)
		return er;
	sum=(*(int*)(DataN+3));//%0xFFFFFFFF*1.0;
	return 0;
}

int ElvisRegVer3Printer::ReadTable(int TableNumber,int RowNumber,int FieldNumber)
{
	SetInstrHeader(0x1F);
	DataN[5]=TableNumber%0xFF;
	*((short*)(DataN+6))=RowNumber%0xFFFF;
	DataN[8]=FieldNumber%0xFF;
	return JustDoIt(9,100);
}

int ElvisRegVer3Printer::GetDateShiftRange(const char* password,DateShiftRange& range)
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

int ElvisRegVer3Printer::DateRangeReport(const char* password,DateShiftRange range,bool type)
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

int ElvisRegVer3Printer::ShiftRangeReport(const char* password,DateShiftRange range,bool type)
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
int ElvisRegVer3Printer::GetMode(void)
{

	SetInstrHeader(0x11);
	int ret = JustDoIt(5,300);

// 1  ��� ������ (1 ����)
// 2  ���������� ����� ��������� (1 ����)
// 3  ������ �� �� (2 �����)
// 5  ������ �� �� (2 �����)
// 7  ���� �� �� (3 �����) ��-��-��
//10  ����� � ���� (1 ����)
//11  �������� ����� �������� ��������� (2 �����)
//13  ����� �� (2 �����)
//15  ����� �� (1 ����)
//16  �������� �� (1 ����)
//17  ���� �� (1 ����)
//18  ������ �� �� (2 �����)
//20  ������ �� �� (2 �����)
//22  ���� �� �� (3 �����) ��-��-��
//25  ���� (3 �����) ��-��-��
//28  ����� (3 �����) ��-��-��
//31  ����� �� (1 ����)
//32  ��������� ����� (4 �����)
//36  ����� ��������� �������� ����� (2 �����)
//38  ���������� ��������� ������� � �� (2 �����)
//40  ���������� ��������������� (������������) (1 ����)
//41  ���������� ���������� ��������������� (������������) (1 ����)
//42  ��� (6 ����)

	/*
	if(ret)
		return ret;
	static unsigned char flag = 0;
	unsigned char fr = (*(DataN + 14));// & (P_CHECK | P_CNTRL);
	//if()
	*/
	return ret;
}

int ElvisRegVer3Printer::ContinuePrinting(void)
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

int ElvisRegVer3Printer::RestorePrinting(bool& restore)
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
				//;;Log("if(d < 0)");
				if(d < 0)
				{ // ������� ����� ������ ��������� ������� �������������
					//;;Log("d < 0");
					goto l_reg_sync;
				}
				//;;Log("if(d < 60*60*24 )");
				if(d < 60*60*24 )
				{//��������� ������������� �� ������ ����� �����
					//;;Log("d < 60*60*24");
					if((DataN[15] & 0x0f) == 4){// ����� ������� - ����� ����������������
						//;;Log("(DataN[15] & 0x0f) == 4)");
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
		;;Log(cmd);
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
		SetTable(1,1,42,&(v = 0),1,false);
	return 0;
}

int ElvisRegVer3Printer::ProcessError(int er)
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

int ElvisRegVer3Printer::SetFullZReport(void)
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

int ElvisRegVer3Printer::SetTable(int table,int row,int field,void* val,int len,bool asStr)
{
	SetInstrHeader(0x1E);
	DataN[5]=table%0xFF;
	*((short*)(DataN+6))=row%0xFFFF;
	DataN[8]=field%0xFF;
	memcpy(DataN+9,val,len);
	return JustDoIt(9+len,100);
}


int ElvisRegVer3Printer::GetTable(int table,int row,int field,void* val,int len,bool asStr)
{
	int er=ReadTable(table,row,field);
	if (er!=0)
		return er;

	memcpy(val,DataN+2,len);
	return 0;
}

void ElvisRegVer3Printer::InsertSales(vector<SalesRecord> data)
{
	if ((EntireCheck)&&(Sales.empty())&&(!StartClosing))
		Sales=data;
}

void ElvisRegVer3Printer::InsertPayment(vector<PaymentInfo> data)
{
	if ((EntireCheck)&&(Payment.empty())&&(!StartClosing))
		Payment=data;
}

void ElvisRegVer3Printer::GetPayment(vector<PaymentInfo>** ptr)
{
	*ptr=&Payment;
}

int ElvisRegVer3Printer::GetCheckSum(double& sum)
{
	SetInstrHeader(0x89);
	int er=JustDoIt(5,MinDuration);
	if (er!=0)
		return er;

	sum=((*(int*)(DataN+3))%0xFFFFFFFF)*1./MCU;
	return 0;
}


int ElvisRegVer3Printer::PrintBarCode(__int64 barcode)
{
	SetInstrHeader(0xC2);
//	barcode %= 0xFFFFFFFFFF;
	(*(__int64*)(DataN+5))=barcode;//%0xFFFFFFFFFF; // ���� ����
	return JustDoIt(10,MinDuration);
}

int ElvisRegVer3Printer::LogOpRegister()
{
    FILE *f;
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

int ElvisRegVer3Printer::Test()
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
//		l += W2U("�����");
		l += W2U("EXIT");
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

int ElvisRegVer3Printer::GetSmenaNumber(unsigned long *KKMNo,unsigned short *KKMSmena,time_t * KKMTime)
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
///	*KKMSmena=*(unsigned short *)(DataN + 36);

	// ����� ����� ����� �� �������� ���������
    *KKMSmena=CashRegister::ConfigTable->GetLongField("LAST_SMENA");

	return 0;
}


void ElvisRegVer3Printer::SyncDateTime(void)
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
//		sprintf( cmd, "echo \"1\" | sudo -S date -s \"%02d/%02d/%02d %02d:%02d:%02d\"", reg_tm.tm_mon + 1, reg_tm.tm_mday, reg_tm.tm_year%100, reg_tm.tm_hour, reg_tm.tm_min, reg_tm.tm_sec );
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

void ElvisRegVer3Printer::CntLog(string str)
{

	time_t t;
	time( &t );
	struct tm *now = localtime( &t );

	fprintf(cLog,"%02d.%02d.%02d %02d:%02d:%02d> | %s \n", now->tm_mday, now->tm_mon+1, now->tm_year + 1900, now->tm_hour, now->tm_min, now->tm_sec, str.c_str());
	fflush(cLog);

}
