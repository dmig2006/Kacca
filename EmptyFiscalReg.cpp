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

#include "EmptyFiscalReg.h"
#include "Utils.h"
#include "Cash.h"
#include "DbfTable.h"

#include "Payment.h"

#define	STR_PRINT
#define REG_DEBUG

enum {STX=0x02,ETX=0x03,EOT=0x04,ENQ=0x05,ACK=0x06,DLE=0x10,NAK=0x15,};

//Empty fiscal register implementation

EmptyFiscalReg::EmptyFiscalReg(void)
{
	cut_skip = 3;
	LineLength=0;
}

EmptyFiscalReg::~EmptyFiscalReg(void)
{
}

int EmptyFiscalReg::ResetReg(void)
{
	return 0;
}

int EmptyFiscalReg::Cancel(void)
{
	return 0;
}

int EmptyFiscalReg::CashIncome(double sum)
{
	return 0;
}

int EmptyFiscalReg::CashOutcome(double sum)
{
	return 0;
}

int EmptyFiscalReg::Close(double sum,double prepay, double discount,int CashDesk,int CheckNum,int PaymentType, long long CustomerCode, string CashmanName,string vote,string info,bool isHandDisc, string egais_info, string egais_url, string egais_sign, int prntQRCode)
{
	//PrintVote();// голосование
	return 0;
}

int EmptyFiscalReg::CreateInfoStr(int CashDesk,int CheckNum,int PaymentType,string CheckHeader,string info)
{

	PrintCheckHeader(CheckHeader);
	//PrintVote(PaymentType,vote);// голосование

//!переделать - добавить возможность вывода информации в заголовок чека
//!
   // PrintInfo(info);// дополнительная информация

    // + dms ====== для ЕНВД наименование документа задается в настройках программы
    string CheckStr = CheckName;

    string FirstStr,SecondStr,SpaceStr;
	FirstStr="Касса #"+Int2Str(CashDesk);

    if (CheckNum>=0)
    {
        if (CheckStr.empty())
        {
            SecondStr="Чек #"+Int2Str(CheckNum);
        }
        else
        {
            string ChStr = Int2Str(CheckNum);

            for (unsigned int i=0;i<6-ChStr.length();i++)
                ChStr="0"+ChStr;

            SecondStr=CheckStr+" #"+ChStr;
        }
    }
    // - dms ======


	if (LineLength>FirstStr.length()+SecondStr.length())
		for (unsigned int i=0;i<LineLength-FirstStr.length()-SecondStr.length();i++)
			SpaceStr+=' ';

	return PrintString((FirstStr+SpaceStr+SecondStr).c_str());
}

int EmptyFiscalReg::CreatePriceStr(const char *str,double price)
{
	string Capt=str;
	string Sum=" ="+GetRounded(price,0.01);
	int Len=Capt.length()+Sum.length();

	if (LineLength>Len){
		for (int i=0;i<LineLength-Len;i++)
			Capt+=' ';
	}else{
		Capt = Capt.substr(0, LineLength - Sum.length());
	}

	Capt+=Sum;

	return PrintString(Capt.c_str());
}

int EmptyFiscalReg::DateRangeReport(const char* passwd,DateShiftRange shift,bool type)
{
	return 0;
}

int EmptyFiscalReg::GetCurrentSumm(double& sum)
{
	sum=0;
	return 0;
}

int EmptyFiscalReg::GetDateShiftRange(const char *passwd,DateShiftRange& shift)
{
	shift.FirstShift=0;
	shift.LastShift=0;
	shift.FirstDate.tm_mday=31;
	shift.FirstDate.tm_mon=12;
	shift.FirstDate.tm_year=1899;
	shift.LastDate.tm_mday=31;
	shift.LastDate.tm_mon=12;
	shift.LastDate.tm_year=1899;
	return 0;
}

string EmptyFiscalReg::GetErrorStr(int er,int lang)

{
	return "";
}

int EmptyFiscalReg::InitReg(int port,int baud,int type,const char *passwd)
{
	return 0;
}

int EmptyFiscalReg::ChangeRegBaud(int port,int oldbaud, int baud,int type,const char *passwd)
{
	return 0;
}

bool EmptyFiscalReg::IsPaperOut(int er)
{
	return false;
}

bool EmptyFiscalReg::IsFullCheck(void)
{
	return false;
}

bool EmptyFiscalReg::RestorePaperBreak(void)
{
	return false;
}

void EmptyFiscalReg::OpenDrawer(void)
{
}

int EmptyFiscalReg::PrintHeader(void)
{
	return 0;
}

int EmptyFiscalReg::PrintString(const char* str,bool ctrl)
{
	return 0;
}

void EmptyFiscalReg::WaitNextHeader(void)
{
}

int EmptyFiscalReg::Return(const char* Name,double Price,double Quantity,int Department)
{
	return 0;

}

int EmptyFiscalReg::Sale(const char* Name,double Price,double Quantity,int Department)
{
	return 0;
}

int EmptyFiscalReg::Storno(const char* Name,double Price,double Quantity,int Department)
{
	return 0;
}

int EmptyFiscalReg::SetFullZReport(void)
{
	return 0;
}

int EmptyFiscalReg::SetDate(int Day,int Month,int Year)
{
	return 0;
}


int EmptyFiscalReg::SetTime(int Hour,int Minute,int Second)
{
	return 0;
}

int EmptyFiscalReg::ShiftRangeReport(const char *passwd,DateShiftRange shift,bool type)
{
	return 0;
}

int EmptyFiscalReg::XReport(void)
{
	return 0;
}

int EmptyFiscalReg::ZReport(double& sum)
{
	sum=0;
	return 0;
}

int EmptyFiscalReg::OpenKKMSmena(void)
{
    return 0;
}

int EmptyFiscalReg::closeAdminCheck()
{
    return(0);
}

int EmptyFiscalReg::Reprint(void)
{
    ;;Log("[Proc] int EmptyFiscalReg::Reprint(void)");
	return 0;
}


void EmptyFiscalReg::SetDBArchPath(string path)
{
}

int EmptyFiscalReg::SetTable(int table,int row,int field,void* val,int len,bool asStr)
{
	return 0;
}

int EmptyFiscalReg::GetTable(int table,int row,int field,void* val,int len,bool asStr)
{
	memcpy(val,0,len);
	return 0;
}

void EmptyFiscalReg::InsertSales(vector<SalesRecord>)
{
}

void EmptyFiscalReg::InsertPayment(vector<PaymentInfo> data)
{
}

void EmptyFiscalReg::GetPayment(vector<PaymentInfo> **ptr)
{
//	*ptr=&Payment;
}

int EmptyFiscalReg::PrintVote(int PaymentType,string vote_)
{

	const char* vote = vote_.c_str();
	int len = strlen(vote), i = 0;
	char buf[36];
	if (len > 0){
		if(PaymentType!=0) {
			PrintString("");
			CutCheck();
		}
		memcpy(buf,vote,36);
		PrintString(buf,false);
		if (vote[36] == ' ') i++;
		if (len > (36 + i)){
			memcpy(buf, vote + 36 + i, 36);
			PrintString(buf,false);
		}
		if (vote[72] == ' ') i++;
		if (len > (72 + i)){
			memcpy(buf, vote + 72 + i, 28 - i);
			PrintString(buf,false);
			}
		/*
		PrintBarCode((__int64)(CashReg->GetLastCheckNum()+1)*100 +
			     (__int64) CashReg->GetCashRegisterNum());
		*/
		PrintString("");
		PrintString("");
		PrintHeader();
		CutCheck();
		return 1;
	}
	else
		return 0;
}

int EmptyFiscalReg::PrintCheckHeader(string CheckHeader)
{
	int oldpos=0,pos=0;

	pos = CheckHeader.find_first_of("\n\0x00",oldpos);
	while (pos!=string::npos)
	{
		string tmpstr;
		char buf[37];
		tmpstr = CheckHeader.substr(oldpos,pos-oldpos);
		if(strcmp(tmpstr.c_str(),"cutcheck") == 0)
		{
			CutCheck();
		}
		else
		if(strcmp(tmpstr.c_str(),"printheader") == 0)
		{
			PrintHeader();
		}
		else
		{
			memcpy(buf,tmpstr.c_str(),36);
			buf[36]=0;
			PrintString(buf,false);
		}
		oldpos=pos+1;
		pos = CheckHeader.find_first_of("\n\0x00",oldpos);
	}
	return 0;
}

int EmptyFiscalReg::PrintFormatText(string info_)
{
	int len = info_.length();
	if (len > 0)
	{
		PrintString("",false);

        string str, word;
        str="";word="";
		int i;
		for (i=0;i<len;i++)
		{
		    if (info_[i]!='\n') str+=info_[i];
		    if (((str).length()>34) || (info_[i]=='\n'))
		    {
                PrintString(GetFormatString(" "+str, "").c_str(),false);
                str="";
		    }

//		    if
//		    (
//		    (info_[i] == ' ')
//		    ||
//		    (info_[i] == '.')
//		    ||
//		    (info_[i] == ',')
//		    ||
//		    (info_[i] == '!')
//		    ||
//		    (info_[i] == '?')
//		    )
//		    {
//                str+=word;
//                word="";
//		    }

		}

        if (str.length()>0)
        {
            PrintString(GetFormatString(" "+str, "").c_str(),false);
        }

		PrintString("",false);
		return 1;
	} // ~информация в чеке
	else
		return 0;
}


int EmptyFiscalReg::PrintInfo(string info_)
{

	int len = info_.length();
	if (len > 0)
	{
		PrintString("",false);
		PrintString("************************************",false);
		PrintString(GetFormatString("*","*").c_str(),false);

        string str, word;
		int i;
		for (i=0;i<len;i++)
		{
		    if (info_[i]!='^') word+=info_[i];
		    if (((str+word).length()>34) || (info_[i]=='^'))
		    {
                PrintString(GetFormatString("*"+GetCenterString(str).substr(0,34), "*").c_str(),false);
                str="";
		    }

		    if
		    (
		    (info_[i] == ' ')
		    ||
		    (info_[i] == '.')
		    ||
		    (info_[i] == ',')
		    ||
		    (info_[i] == '!')
		    ||
		    (info_[i] == '?')
		    )
		    {
                str+=word;
                word="";
		    }

		}

        str+=word;

        if (str.length()>0)
        {
            PrintString(GetFormatString("*"+GetCenterString(str).substr(0,34), "*").c_str(),false);
        }


/*
		for (i=0;i<(len/34)+1;i++)
		{
			PrintString(GetFormatString("*"+GetCenterString(info_.substr(i*34, 34)).substr(0,34), "*").c_str(),false);
		}
//*/


//
//		if((len%34)>0)
//		{
//			memcpy(buf,info+i*34,(len%34));
//			buf[len%34]=0;
//			PrintString(PrintString(GetFormatString("*"+info_.substr(i*34), "*").c_str(),false);,false);
//		}
		PrintString(GetFormatString("*","*").c_str(),false);
		PrintString("************************************",false);
		PrintString("",false);
		return 1;
	} // ~информация в чеке
	else
		return 0;



/*
	const char* info = info_.c_str();// информация в чеке
	int len = strlen(info);
	if (len > 0)
	{

		//PrintString("",false);
		PrintString("************************************",false);
		PrintString("",false);
		char buf[37];
		int i;
		for (i=0;i<(len/36);i++)
		{
			memcpy(buf,info+i*36,36);
			PrintString(buf,false);
		}
		if((len%36)>0)
		{
			memcpy(buf,info+i*36,(len%36));
			buf[len%36]=0;
			PrintString(buf,false);
		}
		PrintString("",false);
		PrintString("************************************",false);
		PrintString("",false);
		return 1;
	} // ~информация в чеке
	else
		return 0;
*/


/*	if (len > 0){
		memcpy(buf,info,36);//1я строка
		int i = 0;
		buf[36] = '\0';
		//PrintString("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",false);
		PrintString(buf,false);
		if (info[36] == ' ') i++;
		if (len > (36 + i)){
			memcpy(buf, info + 36 + i, 36);
			PrintString(buf,false);
		}

		if (info[72] == ' ') i++;
		if (len > (72 + i)){
			for (int j = 0; j < 36; buf[j++] = ' ');
			memcpy(buf, info + 72 + i, 28 - i);
			PrintString(buf,false);
			}
		PrintString("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",false);
		return 1;
	} // ~информация в чеке
	else
		return 0;*/
}

int EmptyFiscalReg::AtoI(string ln)
{
	int mnum = 0;
	for (int i = 2; i < 6; i++)
		mnum =  mnum*10 + ln[i] - 48;
	return mnum;
}
int EmptyFiscalReg::GetSmenaNumber(unsigned long *KKMNo,unsigned short *KKMSmena,time_t * KKMTime)
{
	*KKMNo=0;
	*KKMSmena=0;

	return 0;
}

//!bool EmptyFiscalReg::StringIsNull(char* str, int strSize=0)
bool EmptyFiscalReg::StringIsNull(char* str, int strSize)
{
    if (!strSize) strSize = strlen(str);

    for (int i=0;i<strSize;i++)
    {
        if ((str[i] ==0) || (str[i] =='\n')) return true;  // если конец строки
        if ((str[i] !=' ')&& (str[i] !='\t'))
        return false; // если символ отличный от пробела и табуляции
    }
    return true;
}

bool EmptyFiscalReg::NeedCutCheck(char* str)
{
    bool res = false;
    int strSize = strlen(str);

    if(strSize>=2)
    {

        if ((str[0]==0x1b) || (str[1]==0x69))
        {
            str[0]=' ';
            str[1]=' ';

            res = true;
        }


        if ((str[0]==0x7e) || (str[1]==0x53))
        {
            str[0]=' ';
            str[1]=' ';

            res = true;
        }

    }

    return res;
}

void EmptyFiscalReg::SyncDateTime(void)
{
}
