/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <xbase/xbase.h>

#include "Display.h"
#include "Cash.h"
#include "Fiscal.h"
#include "DbfTable.h"
#include "Utils.h"
#include "WorkPlace.h"

//#include <egateapi.h>

extern "C"
{
#include <uuid/uuid.h>
}

extern CashRegister* CashReg;

#ifndef _WS_WIN_

	static void* dl = NULL;
	static bool (*CC_Init)(const int, const char*);
	static bool (*CC_ReadCard)(const time_t);
	static bool (*CC_LockSum)(const unsigned long);
	static bool (*CC_ConfirmSum)();
	static bool (*CC_RollbackSum)();
	static void (*CC_Release)();
//	static void (*CC_GetSum)(long *, long *, long *);
//	static DWORD (*CC_GetHR)();
//	static char * (*CC_GetCardNumber)();
//	static char * (*CC_GetErrorDesc)();
	static void (*CC_GetTrnProperties)(STrnProperties *);
/*
CardReader::dl = NULL;
CardReader::CC_Init = NULL;
CardReader::CC_ReadCard = NULL;
CardReader::CC_LockSum = NULL;
CardReader::CC_ConfirmSum = NULL;
CardReader::CC_RollbackSum = NULL;
CardReader::CC_Release = NULL;
//CardReader::CC_GetSum = NULL;
//CardReader::CC_GetHR = NULL;
//CardReader::CC_GetCardNumber = NULL;
//CardReader::CC_GetErrorDesc = NULL;
CardReader::CC_GetTrnProperties = NULL;
*/

// функции библиотеки libemvgate

    static void* idl = NULL;
    static int   (*egGetVersion)();
    static char* (*egGetVersionDescription)();
    static int   (*egGetLastError)(int);
    static char* (*egGetErrorDescription)(int);
    static int (*GetEncryptedPinBlock)(char*,char*,char*,char*,int,int,char*,char*);
    static int (*egInitInstance)(const char*);
    static bool (*egReleaseInstance)(int);
    static char* (*egAuthRequest)(int,int,const char*);
    static char* (*egGetAuthReceipt)(int);
    static char* (*egGetAuthResult)(int);

#endif

EmptyDisplay::EmptyDisplay(int port,int baud)
{
	LineLength=0;
}

EmptyDisplay::~EmptyDisplay()
{
}

void EmptyDisplay::ClearText(void)
{
}

void EmptyDisplay::DisplayLines(const char* first,const char* second)
{
}

void EmptyDisplay::DisplayText(const char* text)
{
}

DatecsDisplay::DatecsDisplay(int port,int baud)
{
	if (port*baud!=0)//open serial port ann clear settings
	    DisplayCom=OpenPort(port,baud,0,100);
	else
	    DisplayCom=0;
	ClearText();
	LineLength=20;
}

DatecsDisplay::~DatecsDisplay()
{
	ClearText();
	if (DisplayCom!=0)
		ClosePort(DisplayCom);
}

string DatecsDisplay::Convert(string str)//internal coding is something strange
{										//I never saw it before
	unsigned char UpperConvertTable[]={
		0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
		0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
		0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
		0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBd,0xBE,0xBF,
		0xB6,0xB0,0x42,0xB1,0xB2,0x45,0xB3,0xB4,0xB8,0xB9,0x4B,0xBA,0x4D,0x48,0x4F,0xBB,
		0x50,0x43,0x54,0xBC,0xBF,0x58,0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0x62,0xC8,0xC9,0xCA,
		0xB6,0xB0,0x42,0xB1,0xB2,0x45,0xB3,0xB4,0xB8,0xB9,0x4B,0xBA,0x4D,0x48,0x4F,0xBB,
		0x50,0x43,0x54,0xBC,0xBF,0x58,0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0x62,0xC8,0xC9,0xCA,};

	for (unsigned int i=0;i<str.length();i++)
	{
		unsigned char ch=str[i];
		if (ch>=128)
			str[i]=UpperConvertTable[ch-128];
	}

	return str;
}

void DatecsDisplay::DisplayText(const char* text)
{
	string str=text;
	if (DisplayCom!=0)
	{
		str=Convert(str);//convert string and write to serial port
		for(unsigned int i=0;i<str.length();i++)//display will show it
			WriteToPort(DisplayCom,&(str[i]),1);
	}
}

void DatecsDisplay::DisplayLines(const char* up,const char* down)
{
	unsigned int len,i;

	string first=up,second=down;

	if (DisplayCom==0)
		return;

	ClearText();
	first=Convert(first);
	if (first.length()>20)//cut off first string
		first=first.substr(0,20);
	else
	{
		len=first.length();//or add blanks
		for (i=0;i<20-len;i++)
			first+=' ';
	}

	second=Convert(second);
	if (second.length()>20)
		second=second.substr(0,20);//analoguesly
	else
	{
		len=second.length();
		for (i=0;i<20-len;i++)
			second+=' ';
	}

	for(i=0;i<first.length();i++)//write both strings to the serial port
		WriteToPort(DisplayCom,&(first[i]),1);

	for(i=0;i<second.length();i++)
		WriteToPort(DisplayCom,&(second[i]),1);
}

void DatecsDisplay::ClearText(void)//clear all
{
	unsigned char com=0x0c;

	if (DisplayCom!=0)
		WriteToPort(DisplayCom,&com,1);
}

VSComDisplay::VSComDisplay(int port,int baud) : DatecsDisplay(port,baud)
{
	unsigned char initbuffer[]={0x0c,0x1b,0x66,'R',0x1b,0x63,'R',};
	if (DisplayCom!=0)
		WriteToPort(DisplayCom,initbuffer,7);
}

VSComDisplay::~VSComDisplay(void)
{
}

string VSComDisplay::Convert(string str)
{
	unsigned char UpperConvertTable[]={
		0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
		0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
		0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
		0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
		0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
		0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
		0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
		0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,};

	for (unsigned int i=0;i<str.length();i++)
	{
		unsigned char ch=str[i];
		if (ch>=128)
			str[i]=UpperConvertTable[ch-128];
	}

	return str;
}




//////////////////////////////////////////////////////////////////////
// CardReader Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

typedef unsigned char BYTE;
typedef unsigned short WORD;

#ifndef _WS_WIN_

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qstringlist.h>
#include <stdio.h>
#include <qlabel.h>
#include <qlayout.h>

//class
CardReader::CardReader()// : QDialog( NULL, NULL, true )
{
	//msg = _msg;
}

CardReader::~CardReader()
{

}

const char * D2W(char str[])
{
char koi2win[] = {
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
0xfe,0xe0,0xe1,0xf6,0xe4,0xe5,0xf4,0xe3,0xf5,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,
0xef,0xff,0xf0,0xf1,0xf2,0xf3,0xe6,0xe2,0xfc,0xfb,0xe7,0xf8,0xfd,0xf9,0xf7,0xfa,
0xde,0xc0,0xc1,0xd6,0xc4,0xc5,0xd4,0xc3,0xd5,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,
0xcf,0xdf,0xd0,0xd1,0xd2,0xd3,0xc6,0xc2,0xdc,0xdb,0xc7,0xd8,0xdd,0xd9,0xd7,0xda };
	unsigned char * p = (unsigned char *)str - 1;
	while(*(++p)){
		*p = koi2win[*p];
	}
	return str;
}
QString D2U(char str[])
{
	return W2U(D2W(str));
	//return QString(str);
	//return QString::fromUtf8(str);
}

#include <qapplication.h>
#include <qsemimodal.h>
#include <qevent.h>
#include <qcursor.h>
#include <qstatusbar.h>
#include <qbutton.h>
#include <qvariant.h>
#include <signal.h>

char a_code[20];
char b_term[20];
char c_card[32];
char cc_card[32];
char l_card[32];
char c_track[250];

char a_sha1[250];

string BonusDiscTranID;
string BonusDiscTranDT;

int SberbankCard; // признак карты Сбербанка
int SBCardDiscountProgrammType; // номер программы лояльности Сбербанка
string CardSHA1;  // хэш карты



bool inline a_code_is_0()
{
	/*for(char* t = a_code+1; *t; t++)
		if(*t != '0')
			return false;
	return true;*/
	for(int i=0; a_code[i]!='\n'; i++)//
		if(a_code[i]!= '0')
			return false;
	return true;//~
}

/*
bool inline GetCertificate(){
	FILE *f;
	char num[20];
	char line[16];
	f=fopen("./upos/p","r");
	if(!f)
		return false;
	while(!feof(f)){
		fscanf(f,"%s",line);
		if (strstr(line,"CK:")){
			fscanf(f,"%s", num);
			fclose(f);
			a_code=num;
			QMessageBox::information( NULL, "Kacca",num, "Ok");
			return true;
		}
		else
		QMessageBox::information( NULL, "Kacca","No CK :(", "Ok");
	}
}
*/


int EmptyFiscalReg::PrintPCheck(const char* fn, bool dos)
{

	FILE * f = fopen( fn, "r");

	if(!f)
		return 0;

	int empty = 0;
	int any = 0;
	int emptycnt =0; //текущее кол-во пустых строк подряд
	int maxempty =4; //максимально-допустимое число пустых строк подряд (свыше не выводятся)
	bool flagprint = true;

	bool need_cut=true;

	bool lastCut=false;

/// +dms ==== Временно. На момент замены ПО терминалов на печать 1 слипа ====
    bool cut2check=true;
    FILE * fe = fopen("/Kacca/tmp/bankcopy.flg", "r");
    if(fe){
        cut2check=false;
        fclose(fe);
    }
/// -dms ==== Временно. На момент замены ПО терминалов на печать 1 слипа ====

	while(!feof(f))
	{
		char str[64];
		fgets(str, 64, f);
		char * nl = strrchr(str, 0x0d);
		if(nl) *nl = '\0';
		nl = strrchr(str, 0x0a);
		if(nl) *nl = '\0';

//		nl = strrchr(str, 0x1b);
//		if(nl) *nl = ' ';
//		nl = strrchr(str, 0x69);
//		if(nl) *nl = ' ';
//

/// +dms ==== Временно. На момент замены ПО терминалов на печать 1 слипа ====
        // проверка строки на спец.символ обрезки чека
/* Отключаем обрезку по спецсимволу
if (cut2check){
        if(NeedCutCheck(str))
        {
            if (!need_cut) continue;

            for(unsigned int j=0;j<3;j++)
             {
                PrintString("",false);
                empty++;
             }

            // Обрезка по спецсимволу
            CutCheck();
            need_cut=false;
        }
}
*/ //Отключаем обрезку по спецсимволу
/// -dms ==== Временно. На момент замены ПО терминалов на печать 1 слипа ====

		char* sbstr="CK:";
		if(strstr(str,sbstr)!=NULL && a_code_is_0())
		{
			sscanf(str,"CK: %s",a_code);
		}
		if(*str == '#')
		{
			if(sscanf(str + 1, "ACODE= %s", a_code) == 1 && a_code_is_0()){
				QMessageBox::information( NULL, "APM KACCA",
					W2U("Ошибка авторизации.\n"
					"Произведите оплату другим способом."), "Ok");
				fclose(f);
				return 2;
			}
			sscanf(str + 1, "TERM= %s", b_term);
			sscanf(str + 1, "CARD= %s", c_card);
			continue;
		}
		if(dos) D2W(str);

        flagprint = true;
		if(StringIsNull(str))
		{
		    emptycnt++;
		    if (emptycnt>maxempty)
                flagprint = false; // пропуск пустых строк
        }
		else
            emptycnt=0;

        if (flagprint)
        {
            int er=0;
            string _str = str;
           // _str="  "+_str;

            while(er = PrintString((char*)_str.c_str(),false))
            {
                string str_error = FiscalReg->GetErrorStr(er,0);

                int res = QMessageBox::information( NULL, "APM KACCA",
                    W2U("Ошибка при распечатке чека:\n\n\""+str_error+"\"\n\n"
                        "- Устраните неисправность\n"
                        "- Попробуйте выключить и включить регистратор\n"
                        "- Убедитесь, что на регистраторе не горит индикатор ошибки\n"
                        "\n"
                        "Нажмите \"Повторить чек\" для печати чека заново.\n"
                        "Для отмены печати чека нажмите \"Отмена\"."),
                        W2U("Повторить печать чека"),W2U("Отмена"));
                if(res==0)
                {
                    ResetReg();
                    //fclose(f);
                    //return PrintPCheck(fn, dos);
                }
                else if (res==1)
                {
                    fclose(f);
                    return 0;
                }
            }
        }

		if(empty >= 3 && any)
		{
			if(++any >= cut_skip)
			{
				any = empty = emptycnt = 0;
				if (need_cut)
				{
				    // Обрезка по признаку "пустых строк > 3"
				    //!CutCheck();
				    need_cut=false;
				}
			}
			continue;
		}

		if(!(*str))
		{// str is empty
			empty++;
			any = 0;
			continue;
		}
		any++;
		if(empty < 3)
			empty = 0;
	}

	fclose(f);

	// Промотаем чек вперед
	PrintString("",false);
	PrintString("",false);

	PrintHeader();
	CutCheck();
	return 0;
}

#include <unistd.h>
//#include "citycard.h"
int CRSBupos::exec(char **argv, bool closeday)
{
	char cmd[256] = "/Kacca/upos/sb_pilot";
	char cmdcpy[256] = "";
	int retsys;

	//+ dms ==== 2014-06-02 ==== Удалим файлы перед оплатой ======
	char efile[]="/Kacca/upos/e";
	char pfile[]="/Kacca/upos/p";

    remove(efile);
    remove(pfile);
    //- dms ==== 2014-06-02 ====

	//qApp->setOverrideCursor(QApplication::waitCursor);
	FILE* f;

	//+ dms ==== 2021-06-08 ==== Если файл 'e' по каким-либо причинам не удалось корректно удалить (нет доступа для записи/удаления),
	// то прерываем операцию из-за возможных ошибок в интерпретации ответа ======
	f = fopen("/Kacca/upos/e","r");
	if(f){
        fclose(f);
        return -1;
	}
	//- dms ==== 2021-06-08 ====


	QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
	QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
	sm.setFixedSize(250,100);
	msg.setText(W2U("Работа с терминалом Сбербанка...\nСмотрите на экран терминала."));
	msg.setFixedSize(250,100);
	msg.setAlignment( QLabel::AlignCenter );
	sm.show();
	thisApp->processEvents();

	argv[0] = cmd;//"sb_pilot";
	for(int i = 1; argv[i]; i++){
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	}


	for(int i = 1; argv[i]; i++)
	{
		strcat(cmdcpy, " ");
		if((4==i) && (strlen(argv[i])>8))
            for(int j=8;argv[i][j];j++) argv[i][j]='*';
	    strcat(cmdcpy, argv[i]);
	}

	chdir("/Kacca/upos");
	//FILE *lf;
	const char* tmpbuf; char logfile[]="sb.log";
	//strcpy(cmdcpy,cmd);
	f = fopen(logfile, "a");
	fprintf(f,"\n"); tmpbuf=CashReg->GetCurDate().c_str(); fwrite(tmpbuf,10,1,f);
	fprintf(f," ");  tmpbuf=CashReg->GetCurTime().c_str(); fwrite(tmpbuf,8,1,f);
	fprintf(f,"> %s === ",cmdcpy);
	fclose(f);

	retsys=system(cmd);

	f = fopen(logfile, "a");
	fprintf(f,"%i\n",retsys);
	fclose(f);
	chdir("/Kacca");
	LoggingUpos();

    // логирование банковских операций
	LoggingBankReports(PrintFileName(), closeday?7:1, 1);

	//FILE* f;
	f = fopen("./upos/e","r");
	if(!f) return -1;
	int e;
	fscanf(f,"%d,",&e);
	char sMsg[256];
	fgets(sMsg, 250, f);
	//LoggingUpos();

	if(e && (e!=4353)){
	    string errStr = sMsg;
	    ;;Log("  -> Error: " +Int2Str(e)+" ("+errStr+")");
	    if (e==4119)
	    {
	        ;;Log("   -> Ошибка: Нет связи с банком!");
            QMessageBox::information( NULL, "APM KACCA", W2U("Ошибка: Нет связи с банком!"), "Ok");
	    }
        else
            QMessageBox::information( NULL, "APM KACCA", D2U(sMsg), "Ok");
	}
	else
	{
        //QMessageBox::information( NULL, "APM KACCA", W2U("ОДОБРЕНО"), "Ok");
    }

/*
char a_code[16];
char b_term[16];
char c_card[32];
*/
	fgets(cc_card, 32, f);
	fgets(sMsg, 250, f);/// переделать бы
	fgets(a_code, 16, f);
	//fgets(b_term, 16, f);//
	fclose(f);
	GetTermNum();// определение b_term
	//LoggingUpos();

	if (e==4353) return e; // 4353 - штатный ответ промежуточного отказа (для первой команды оплаты ( код 1) по PayPass)

    if (closeday) return e; // сверку не печатаем

    // Печатаем 2 чека
    int prnres = 0;
    int cnt_prncheck=2;

    /// ==== Временно. На момент замены ПО терминалов на печать 1 слипа ====
    FILE * fe = fopen("/Kacca/tmp/bankcopy.flg", "r");
    if(fe)
        fclose(fe);
    else
        cnt_prncheck=1;
    ///==================

	if (!e)
        for(int i=1;i<=cnt_prncheck;i++)
        {
            if (i>1) FiscalReg->PrintString("Копия 1",false);
            prnres = prnres || FiscalReg->PrintPCheck(PrintFileName());
        }

	return e || prnres;

// Печать для 2-х слипов в p файле
//!	return e || FiscalReg->PrintPCheck(PrintFileName());
}

int CardReader::exec(char **argv, bool closeday)
{
	char cmd[] = "/Kacca/sb_pilot/sb_pilot";
	//qApp->setOverrideCursor(QApplication::waitCursor);

	QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
	QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
	sm.setFixedSize(250,100);
	msg.setFixedSize(250,100);
	msg.setAlignment( QLabel::AlignCenter );
	msg.setText(W2U("Работа с картридером..."));
	sm.show();
	thisApp->processEvents();

	argv[0] = cmd;//"sb_pilot";
	pid_t sb_pilot_pid = fork();
//	printf("fork: %d\n", sb_pilot_pid);
	switch(sb_pilot_pid){
	case -1:
		ShowMessage(NULL, "fork error");
		exit(-1);// error
	case  0: // child
		chdir("/Kacca/sb_pilot");
		exit(execv(cmd, argv));
	default: break;// parent
	}

	int OK;
	struct sockaddr_un addr;

	int srvsock;

	srvsock = socket(PF_LOCAL,SOCK_STREAM,0);
	if(srvsock < 0){
		return 0;
	}

	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path,"/tmp/sb_pilot");
	double t = clock() + 3 * (CLOCKS_PER_SEC);
	while(connect(srvsock,(struct sockaddr*)(&addr),SUN_LEN(&addr))){
		thisApp->processEvents();
		if(clock() > t){
			ShowMessage(NULL, "Error opening socket.");
			kill(sb_pilot_pid,9);
			FILE* f;
			f = fopen("./sb_pilot/e","r");
			if(!f) return -1;
			int e;
			fscanf(f,"%d",&e);
			fclose(f);
			return e;
		}
	}

	for(;;){
		BYTE buf[1024];
		BYTE cmd,flags;
		WORD len;
		int pos = 0;
		BYTE res;

		if(read(srvsock,buf,3)!=3){
			continue;
		}
		cmd = buf[0];
		len = *((WORD*)(buf+1));
		if(read(srvsock,buf,len)!=len){
			continue;
		}

		msg.clear();

		//qApp->restoreOverrideCursor();

		printf("switch( %d )\n", cmd);
		switch(cmd){
		case 0x06://ShowStatusWnd
			{
				//qApp->setOverrideCursor(QApplication::waitCursor);
				char str[128];
				len = *((WORD*)(buf));
				memmove(str,buf+2,len);
				str[len] = 0;

				QString s = D2U(str);
				msg.setText(s);
				sm.show();
				thisApp->processEvents();
				memset(buf,0,3);
				write(srvsock,buf,3);
				break;
			}
		case 0x07:
			//msg.setText("");
			memset(buf,0,3);
			write(srvsock,buf,3);
			break;
		case 0x02://Done
			{
				//addr.sun_family = AF_UNSPEC;
				//connect(srvsock,(struct sockaddr*)(&addr),SUN_LEN(&addr));
				kill(sb_pilot_pid,9);
				FILE* f;
				f = fopen("./sb_pilot/e","r");
				if(!f) return -1;
				int e;
				fscanf(f,"%d",&e);
				fclose(f);
//				FiscalReg->PrintPCheck();
				return e || FiscalReg->PrintPCheck();
			}
		case 0x04: //Dialog
			{
				char buf2[128],text[64];
				len= *((WORD*)buf);
				pos = sizeof(WORD);
				char sCaption[128] = "APM KACCA";
				//memmove(sCaption,buf+pos,len);
				//sCaption[len] = 0;
				pos += len;
				//printf("-- %s --\n",str);

				len= *((WORD*)(buf+pos));
				pos += sizeof(WORD);
				char sMsg[128];
				memmove(sMsg,buf+pos,len);
				sMsg[len] = 0;
				pos += len;
				//printf("%s\n",str);

				//printf("%s\n%s\n", sCaption, sMsg);

				flags = buf[pos++];
				len = buf[pos];

				text[0]=0;
				if(flags & 0x10){
					bool ok;
					QString s;
					do{
#						if QT_VERSION < 0x030000
						s = QInputDialog::getText(sCaption,D2U(sMsg), QString::null, &ok);
#						endif
#						if QT_VERSION >= 0x030000
						s = QInputDialog::getText(sCaption,D2U(sMsg),QLineEdit::Normal,QString::null, &ok);
#						endif
					}while(s.length() > len);
					strcpy(text, s.ascii());
					res = ok ? 1 : 2;//flags
				}else{
					switch(flags){
					case 12: // yes/no
						res = QMessageBox::information( NULL, sCaption,
							D2U(sMsg), W2U("Да"), W2U("Нет")) ==
							0 ? 4 : 8;
						break;
					case  1: // ok
						QMessageBox::information( NULL, sCaption,
							D2U(sMsg), "Ok");
						res = 1;
						break;
					case  2: // ok/cancel
					default: // хер его знает, что еще может быть...
						res = QMessageBox::information( NULL, sCaption,
							D2U(sMsg), "Ok", W2U("Отмена")) ==
							0 ? 1 : 2;
					}
				}

				buf[0] = 0;
				len = strlen(text);
				*((WORD*)(buf+1)) = 3+strlen(text);
				buf[3]=res;
				*((WORD*)(buf+4))=strlen(text);
				memmove(buf+6,text,strlen(text));
				write(srvsock,buf,6+strlen(text));
				break;
			}
		case 0x05://Menu
			{
				pos = 0;
				len= (WORD)buf[pos];
				pos += sizeof(WORD);
				char sCaption[128] = "APM KACCA";
				//memmove(sCaption,buf+pos,len);
				//sCaption[len] = 0;
				pos += len;

				//printf("-- %s --\n",sCaption);

				int max = buf[pos++];
				pos += sizeof(WORD);

				QStringList l;
				for(int i=0;i<max;i++){
					char str[128];
					strcpy(str,(char*)(buf+pos));
					pos += strlen(str) + 1;
					l += D2U(str);
				}

				QString s = QInputDialog::getItem("APM KACCA",
					D2U(sCaption),l,0,false);
				buf[3] = l.findIndex(s);
				buf[0] = 0;
				*((WORD*)(buf+1)) = 1;
				write(srvsock,buf,4);
				break;
			}
		}
	}
}


int CardReader::ReadCardNumber_PayPass(unsigned long sum)
{

	char ss[16];
	sprintf(ss,"%u",sum);

	char *args[] = {NULL, "1", ss, NULL};

    int ret = exec(args);

    if (ret==4353) ret = 0;

     //ТЕСТ
	FILE* f = fopen("./upos/e","r");

	//ТЕСТ
	if(!f) return -1;
	int e;
	fscanf(f,"%d,",&e);
	char sMsg[256];
	fgets(sMsg, 250, f);
	fgets(sMsg, 250, f);

    char* card = strdup(sMsg);

    //QMessageBox::information( NULL, "APM KACCA0", D2U(card), "Ok");

    strcpy(c_track, sMsg);
    c_track[250] = 0;

    char* track =strchr(card, '=');
    if (track!=NULL)
    {
        *track = 0;
        strcpy(c_card, card);
        c_card[31] = 0;

        char* track_pos =strchr(c_track, '\r');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

        track_pos =strchr(c_track, '\n');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

    }
    else
    {
        strcpy(c_card, card);

        strcpy(c_track, card);

        // трек 2
        char* track_pos =strchr(c_track, '\r');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

        track_pos =strchr(c_track, '\n');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }


        // номер карты
        track_pos =strchr(c_card, '\r');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

        track_pos =strchr(c_card, '\n');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

    }

    // Признак банковской карты и хэш
    fgets(sMsg, 250, f);// срок действия карты
    fgets(sMsg, 250, f);// Код авторизации
    fgets(sMsg, 250, f);// Внутренний номер операции
    fgets(sMsg, 250, f);// Название типа карты

    fscanf(f,"%d\n,",&SberbankCard);//Признак карты Сбербанка (1) * возвращается 1, если карта Сбербанка, и 0 - в противном случае.

    fgets(sMsg, 250, f);// Номер терминала
    fgets(sMsg, 250, f);// Дата-время операции (ГГГГММДДччммсс)
    fgets(sMsg, 250, f);// Ссылочный номер операции (может быть пустым)
    fgets(sMsg, 250, f);// Хеш от номера карты

    strcpy(a_sha1, sMsg);
    a_sha1[250] = 0;

    QRegExp rn("\\n");
    QRegExp rrn("\\r");

    CardSHA1 = sMsg;
    CardSHA1 = U2W(W2U(CardSHA1).replace( rn, ""));
    CardSHA1 = U2W(W2U(CardSHA1).replace( rrn, ""));

    Log("хэш");
    ;;Log((char*)CardSHA1.c_str());


    ;;string tt = c_track;
    ;;Log("карта");
    ;;Log((char*)tt.c_str());


    fgets(sMsg, 250, f);// Трек 3 карты
    fgets(sMsg, 250, f);// Сумма оплченная бонусами спасибо
    fgets(sMsg, 250, f);// Номер мерчанта
    fgets(sMsg, 250, f);// Тип сообщения системы мониторинга  0 - обычное, 1 - срочное
    fgets(sMsg, 250, f);// Состояние ГПЦ. 0 - работает, 1 - возможны сбои, 2 - не рпботает
    fgets(sMsg, 250, f);// Сообщение системы мониторинга

    fscanf(f,"%d\n,",&SBCardDiscountProgrammType); // Номер программы лояльности в который попадает карта. Число
                        //Если программы не настроены на терминале
                        //или карта ни под одну из программ лояльности не попадает, возвращается 0

	fclose(f);

    //QMessageBox::information( NULL, "APM KACCA1", D2U(c_card), "Ok");
    //QMessageBox::information( NULL, "APM KACCA2", D2U(c_track), "Ok");

    return ret;
}


int CardReader::SB_ReadCardNumber_PayPass()
{

     //ТЕСТ
	FILE* f = fopen("./upos/e","r");

	//ТЕСТ
	if(!f) return -1;
	int e;
	fscanf(f,"%d,",&e);
	char sMsg[256];
	fgets(sMsg, 250, f);
	fgets(sMsg, 250, f);

    char* card = strdup(sMsg);

    //QMessageBox::information( NULL, "APM KACCA0", D2U(card), "Ok");

    strcpy(c_track, sMsg);
    c_track[250] = 0;

    char* track =strchr(card, '=');
    if (track!=NULL)
    {
        *track = 0;
        strcpy(c_card, card);
        c_card[31] = 0;

        char* track_pos =strchr(c_track, '\r');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

        track_pos =strchr(c_track, '\n');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

    }
    else
    {
        strcpy(c_card, card);

        strcpy(c_track, card);

        // трек 2
        char* track_pos =strchr(c_track, '\r');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

        track_pos =strchr(c_track, '\n');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }


        // номер карты
        track_pos =strchr(c_card, '\r');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

        track_pos =strchr(c_card, '\n');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

    }

    // Признак банковской карты и хэш
    fgets(sMsg, 250, f);// срок действия карты
    fgets(sMsg, 250, f);// Код авторизации
    fgets(sMsg, 250, f);// Внутренний номер операции
    fgets(sMsg, 250, f);// Название типа карты

    fscanf(f,"%d\n,",&SberbankCard);//Признак карты Сбербанка (1) * возвращается 1, если карта Сбербанка, и 0 - в противном случае.

    fgets(sMsg, 250, f);// Номер терминала
    fgets(sMsg, 250, f);// Дата-время операции (ГГГГММДДччммсс)
    fgets(sMsg, 250, f);// Ссылочный номер операции (может быть пустым)
    fgets(sMsg, 250, f);// Хеш от номера карты

    strcpy(a_sha1, sMsg);
    a_sha1[250] = 0;

    QRegExp rn("\\n");
    QRegExp rrn("\\r");

    CardSHA1 = sMsg;
    CardSHA1 = U2W(W2U(CardSHA1).replace( rn, ""));
    CardSHA1 = U2W(W2U(CardSHA1).replace( rrn, ""));

    Log("хэш");
    ;;Log((char*)CardSHA1.c_str());


    ;;string tt = c_track;
    ;;Log("карта");
    ;;Log((char*)tt.c_str());


    fgets(sMsg, 250, f);// Трек 3 карты
    fgets(sMsg, 250, f);// Сумма оплченная бонусами спасибо
    fgets(sMsg, 250, f);// Номер мерчанта
    fgets(sMsg, 250, f);// Тип сообщения системы мониторинга  0 - обычное, 1 - срочное
    fgets(sMsg, 250, f);// Состояние ГПЦ. 0 - работает, 1 - возможны сбои, 2 - не рпботает
    fgets(sMsg, 250, f);// Сообщение системы мониторинга

    fscanf(f,"%d\n,",&SBCardDiscountProgrammType); // Номер программы лояльности в который попадает карта. Число
                        //Если программы не настроены на терминале
                        //или карта ни под одну из программ лояльности не попадает, возвращается 0

    ;;Log("программа лояльности = "+Int2Str(SBCardDiscountProgrammType));

	fclose(f);

    //QMessageBox::information( NULL, "APM KACCA1", D2U(c_card), "Ok");
    //QMessageBox::information( NULL, "APM KACCA2", D2U(c_track), "Ok");

    return 0;
}


int CardReader::ReadCardNumber(void)
{

	char *args[] = {NULL, "20", NULL};

	int ret = exec(args);

     //ТЕСТ
	FILE* f = fopen("./upos/e","r");
	//FILE* f = fopen("./upos/e1","r");
	//ТЕСТ
	if(!f) return -1;
	int e;
	fscanf(f,"%d,",&e);
	char sMsg[256];
	fgets(sMsg, 250, f);
	fgets(sMsg, 250, f);

    char* card = strdup(sMsg);

    //QMessageBox::information( NULL, "APM KACCA0", D2U(card), "Ok");

    strcpy(c_track, sMsg);
    c_track[250] = 0;

    char* track =strchr(card, '=');
    if (track!=NULL)
    {
        *track = 0;
        strcpy(c_card, card);
        c_card[31] = 0;

        char* track_pos =strchr(c_track, '\r');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

        track_pos =strchr(c_track, '\n');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

    }
    else
    {
        strcpy(c_card, card);

        strcpy(c_track, card);

        // трек 2
        char* track_pos =strchr(c_track, '\r');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

        track_pos =strchr(c_track, '\n');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }


        // номер карты
        track_pos =strchr(c_card, '\r');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

        track_pos =strchr(c_card, '\n');
        if (track_pos!=NULL)
        {
            *track_pos=0;
        }

    }

    // Признак банковской карты и хэш
    fgets(sMsg, 250, f);// срок действия карты
    fgets(sMsg, 250, f);// Код авторизации
    fgets(sMsg, 250, f);// Внутренний номер операции
    fgets(sMsg, 250, f);// Название типа карты

    fscanf(f,"%d\n,",&SberbankCard);//Признак карты Сбербанка (1) * возвращается 1, если карта Сбербанка, и 0 - в противном случае.

    fgets(sMsg, 250, f);// Номер терминала
    fgets(sMsg, 250, f);// Дата-время операции (ГГГГММДДччммсс)
    fgets(sMsg, 250, f);// Ссылочный номер операции (может быть пустым)
    fgets(sMsg, 250, f);// Хеш от номера карты

    QRegExp rn("\\n");
    QRegExp rrn("\\r");

    CardSHA1 = sMsg;
    CardSHA1 = U2W(W2U(CardSHA1).replace( rn, ""));
    CardSHA1 = U2W(W2U(CardSHA1).replace( rrn, ""));

    Log("хэш");
    ;;Log((char*)CardSHA1.c_str());


    ;;string tt = c_track;
    ;;Log("карта");
    ;;Log((char*)tt.c_str());


	fclose(f);

    //QMessageBox::information( NULL, "APM KACCA1", D2U(c_card), "Ok");
    //QMessageBox::information( NULL, "APM KACCA2", D2U(c_track), "Ok");

    return ret;
}

int CardReader::EG_ReadCardNumber(void)
{

    char ks[10];
    sprintf(ks,"%d", CashReg->GetCashRegisterNum());

    char *args[] = {NULL, ks, "20", "0", "1", NULL};

    int ret = eg_exec(args);

	FILE* f = fopen("./egate/e","r");
	if(!f) return -1;
	int e;
	fscanf(f,"%d \"",&e);
	char sMsg[256];
	fgets(sMsg, 250, f);
	fgets(sMsg, 250, f);

    char* card = strdup(sMsg);

    //QMessageBox::information( NULL, "APM KACCA0", D2U(card), "Ok");

    strcpy(c_track, sMsg);
    c_track[250] = 0;

    char* ap =strchr(c_track, '"');
    if (ap!=NULL) *ap = 0;

    char* track =strchr(card, '=');
    if (track!=NULL)
    {
        *track = 0;
        strcpy(c_card, card);
        c_card[31] = 0;
    }

	fclose(f);

    //QMessageBox::information( NULL, "APM KACCA1", D2U(c_card), "Ok");
    //QMessageBox::information( NULL, "APM KACCA2", D2U(c_track), "Ok");

    return ret;
}


int CardReader::VTB_ReadCardNumber(void)
{

    char ks[10];
    sprintf(ks,"%d", CashReg->GetCashRegisterNum());

    char *args[] = {NULL, "/o11", NULL, NULL, NULL};

    int ret = vtb_exec(args,0);

	FILE* f = fopen("./vtb/e","r");
	if(!f) return -1;
	int e;
	fscanf(f,"%d \"",&e);
	char sMsg[256];
	//fgets(sMsg, 250, f);
	fgets(sMsg, 250, f);

    char* card = strdup(sMsg);

    //QMessageBox::information( NULL, "APM KACCA0", D2U(card), "Ok");

    strcpy(c_track, sMsg);
    c_track[250] = 0;

    strcpy(c_card, c_track);
    c_card[250] = 0;

    char* ap =strchr(c_track, '"');
    if (ap!=NULL) *ap = 0;

    char* track =strchr(card, '=');
    if (track!=NULL)
    {
        *track = 0;
        strcpy(c_card, card);
        c_card[31] = 0;
    }

    // номер карты

	char * nl;
	nl = strrchr(c_card, 0x0d);//'\n');
	if(nl) *nl = '\0';
	nl = strrchr(c_card, 0x0a);//'\n');
	if(nl) *nl = '\0';

	nl = strrchr(c_track, 0x0d);//'\n');
	if(nl) *nl = '\0';
	nl = strrchr(c_track, 0x0a);//'\n');
	if(nl) *nl = '\0';


	fclose(f);

    //QMessageBox::information( NULL, "APM KACCA1", D2U(c_card), "Ok");
    //QMessageBox::information( NULL, "APM KACCA2", D2U(c_track), "Ok");

    return ret;
}

int CardReader::ExecDiscount(const char * card, unsigned long &sum)
{
    ;;Log("ExecDiscount");
	if(!CashReg->CurrentWorkPlace)
		return -1;

    //Добавление товара по номеру карты
    //!Акция закончилась
    //int Res = CashReg->CurrentWorkPlace->ProccessBankCardNumber(card);

    int Result = CashReg->CurrentWorkPlace->ProcessBankDiscounts(card);

    //if (!Result)
        sum = Double2Int(CashReg->CurrentWorkPlace->Cheque->GetSumToPay() * 100); // пересчитаем сумму чека


	return Result;
}

int CardReader::DefineType(const char * card, int &type)
{

	switch (type)
	{
	    case CL_SBERBANK:
            {
                switch (card[0])
                {
                case '4': type=CL_VISA;break;
                case '5': type=CL_MASTERCARD;break;
                case '6': type=CL_MAESTRO;break;
                case '9': type=CL_SBERCARD;break;
                default: type=CL_VISA;break;// по умолчанию пусть будет Visa
                }
                break;
            }
        case CL_GAZPROMBANK:
            {
                switch (card[0])
                {
                case '4': type=CL_VISA_GZ;break;
                case '5': type=CL_MASTERCARD_GZ;break;
                case '6': type=CL_MAESTRO_GZ;break;
                default: type=CL_UNKNOWN_GZ;break;
                }
                break;
            }
        case CL_VTB24:
            {
                switch (card[0])
                {
                case '4': type=CL_VISA_VTB;break;
                case '5': type=CL_MASTERCARD_VTB;break;
                case '6': type=CL_MAESTRO_VTB;break;
                default: type=CL_UNKNOWN_VTB;break;
                }
                break;
            }
	}
    return type;
}

string CardReader::GetPwdCardNumber(string number)
{
    string str_asteriks = "******************************************";
    string result = "";

    int len = number.length();
    if (len>4)
        result = str_asteriks.substr(0, len-4) + number.substr(len-4, 4);

    return result;
}


// Возврат баллов "Спасибо"
int CardReader::ExecBonusReturnDisc(string card, string sha1)
{
    Log("Возврат списания баллов \"Спасибо\" СберБанк");

    if(ReadCardNumber())
    {
        ;;cash_error* _err = new cash_error(11,0," ERROR_CARD_NUMBER (BONUS SBERBANK)");
        ;;CashReg->LoggingError(*_err);
        ;;delete _err;
    };


    if(SberbankCard!=1)
    {
        Log((char*)("Карта стороннего банка. Признак="+Int2Str(SberbankCard)).c_str());
        return -1;
    }

    card = c_card;
    sha1 = CardSHA1;

//    int e = 1;
//
//    if(e)
//    {// бонусные баллы списаны (возвращены) успешно
//       CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,BONUSRETDISC,sum*.01,CashReg->CurrentSaleman->Id,0, card, NULL, NULL, NULL, SberbankCard);// *
//    }
//    else
//    {
//        QMessageBox::information( NULL, "APM KACCA1", W2U(desc), "Ok");
//
//        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,ERRORBONUSRETDISC,sum*.01,CashReg->CurrentSaleman->Id,0, card, NULL, NULL, NULL, SberbankCard);// *
//
//    }
//

	return 0;
}



// Возврат баллов "Спасибо"
int CardReader::ExecBonusReverse(char * card)
{
	if(!CashReg->CurrentWorkPlace)
		return -1;

    Log("Возврат списания баллов \"Спасибо\" СберБанк");

    if(SberbankCard!=1)
    {
        Log((char*)("Карта стороннего банка. Признак="+Int2Str(SberbankCard)).c_str());
        //return -1;
    }

    string StrCard = card;

    int Result = CashReg->CurrentWorkPlace->ProcessBankBonusDiscountReverse(StrCard, CardSHA1);

	return Result;

}


// Начисление баллов "Спасибо"
int CardReader::ExecBonusPay(char * card, unsigned long &sum)
{
	if(!CashReg->CurrentWorkPlace)
		return -1;

    Log("Начисление баллов \"Спасибо\" СберБанк");

    if(SberbankCard!=1)
    {
        Log((char*)("Карта стороннего банка. Признак="+Int2Str(SberbankCard)).c_str());
        return -1;
    }

    string StrCard = card;

    int Result = CashReg->CurrentWorkPlace->ProcessBankBonusPay(StrCard, CardSHA1, sum);

	return Result;
}

int CardReader::BankBonusDiscount()
{
    ;;Log("Получение номера карты \"Спасибо\" СберБанк");

    if(ReadCardNumber())
    {
        ;;cash_error* _err = new cash_error(11,0," ERROR_CARD_NUMBER (BONUS SBERBANK)");
        ;;CashReg->LoggingError(*_err);
        ;;delete _err;
    };

    unsigned long sum=0;
    ExecBonusDisc(c_card, sum, false);

}

int CardReader::ExecBonusDisc(char * card, unsigned long &sum, bool uselimit)
{
    ;;Log("ExecBonusDisc");
	if(!CashReg->CurrentWorkPlace)
		return -1;

    string StrCard = card;

  //  strcpy(card, c_card);

    Log("Списание баллов \"Спасибо\" СберБанк");

    if(SberbankCard!=1)
    {
        Log((char*)("Карта стороннего банка. Признак="+Int2Str(SberbankCard)).c_str());
        return -1;
    }

    string pwd_card = GetPwdCardNumber(card);

    int Result = CashReg->CurrentWorkPlace->ProcessBankBonusDiscount(StrCard, CardSHA1, uselimit);

    sum = Double2Int(CashReg->CurrentWorkPlace->Cheque->GetSumToPay() * 100); // пересчитаем сумму чека

	return Result;
}


int CardReader::Pay(unsigned long sum, int &type, unsigned long check)
{


//!      тест

//CashReg->CurrentWorkPlace->NeedActionVTB = true;
  //  ;;;;return 0;

//        ;;FiscalReg->PrintPCheck(PrintFileName());
//    ;;;return -1;
    //int Res = CashReg->CurrentWorkPlace->ProccessBankCardNumber("42760149");
//!      тест

// инициализация параметров текущей карты
    BonusDiscTranID = "";
    BonusDiscTranDT = "";
    CardSHA1 = "";
    SberbankCard = 0;

    if (CashReg->CurrentWorkPlace)
       CashReg->CurrentWorkPlace->NeedActionVTB = false;

/*
/// ///////////////////////////////////////////////////////////////////////////////////
 {

    //заглушка

char* card = "4279380010004119";
string cardstr = card;

string pwd_card = GetPwdCardNumber(cardstr);

long Result = 1277;
CashReg->CurrentWorkPlace->ProcessBankBonusDiscount(pwd_card.c_str(), Result*.01);

sum = Double2Int(CashReg->CurrentWorkPlace->Cheque->GetSum() * 100); // пересчитаем сумму чека

CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,BONUSPAY,Result*.01,CashReg->CurrentSaleman->Id,0, card);// *
;;return -1;
}

/// ///////////////////////////////////////////////////////////////////////////////////
*/

    ;;Log("   > Pay /"+Int2Str(type)+"/ Sum = "+Int2Str(sum));

	*a_code = *b_term = *c_card = *l_card=0;
	if(type < 0)// CytyCard
		return CC_Pay(sum, check);

/*422580
	bool ok;
	QString s;
	do{
		s = QInputDialog::getText("Kacca",
			W2U("Введите первые 6 цифр номера карточки"), QString::null, &ok);
	}while(s.length() != 6);//ok
//	char text[8];
//	strcpy(text, s.ascii());

//	PD
	doDiscount(s.ascii());
	sum = Double2Int(CashReg->CurrentWorkPlace->Cheque->GetSum() * 100);
*/

    //long SummaBankBonus;

	if ((type==CL_VISA_GZ)||(type==CL_MASTERCARD_GZ)||(type==CL_GAZPROMBANK)) // GazPromBank
    {
        //;;Log("  >> GzB");
        c_track[0] = 0 ;

       if(EG_ReadCardNumber())
        {

/*
            ;;cash_error* _err = new cash_error(11,0," ERROR_CARD_NUMBER (GAZPROMBANK)");
            ;;CashReg->LoggingError(*_err);
            ;;delete _err;
*/
            ;;Log("ERR: Ошибка чтения карты, дальше на оплату не идем!");
            return -1;

        };

        ExecDiscount(c_card, sum);

        if (sum==0)
        {
            ;;Log("  Сумма чека равна нулю!");
            return -1;
        }

        DefineType(c_card, type);

        ;;Log((char*)("   > GB /"+Int2Str(type)+"/").c_str());
		//if(CardReaderType & EGCR)		{};


        char t[] = "0";
        *t += type;
        char ss[16];
        sprintf(ss,"%u",sum);

        char ks[10], ch[18];
        //sprintf(ks,"%d", CashReg->GetCashRegisterNum());
        sprintf(ks,"%d", 1);
        sprintf(ch,"%d", CashReg->GetLastCheckNum()+1);

        strcpy(l_card, c_card);
        l_card[250]=0;
        if (!strchr(l_card,'*') && strlen(l_card)>8 ) //"загасим" карту
        {
            for (int i=8; l_card[i];i++) l_card[i] ='*';
        }

        // начало транзакции
        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_DATA,sum*.01,CashReg->CurrentSaleman->Id,type, c_card, NULL, NULL, NULL, SberbankCard);// *
        //CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_DATA,sum*.01,CashReg->CurrentSaleman->Id,type, l_card,a_code,b_term);// *

        char *args[] = {NULL, ks, "1", ss, ch, NULL, NULL};

        if (c_track[0])  args[5] = c_track;

        int ret = eg_exec(args);

        // результат транзакции
        //* Логирование ошибок по безналу
        int e = -1;
        char sMsg[20]="";

        FILE* f = fopen("./egate/e","r");
        if(f)
        {
            fscanf(f,"%d ",&e);
            fgets(sMsg, 20, f);
            fclose(f);
        }

        char rcode[10];
        sprintf(rcode, "%d", e);
        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_DATA,sum*.01,CashReg->CurrentSaleman->Id,type, c_card, rcode, NULL, sMsg, SberbankCard);// *
        //CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_RESULT,sum*.01,CashReg->CurrentSaleman->Id,type, sMsg, rcode);
        //*/

        return ret;
    }


    //====================================================
    //=================  VTB 24 ==========================


    if ((type==CL_VTB24)) // ВТБ24
    {
        ;;Log("  >> BTБ");
        c_track[0] = 0 ;

       if(VTB_ReadCardNumber())
        {
/*
            ;;cash_error* _err = new cash_error(11,0," ERROR_CARD_NUMBER (VTB)");
            ;;CashReg->LoggingError(*_err);
            ;;delete _err;
*/

            ;;Log("ERR: Ошибка чтения карты, дальше на оплату не идем!");
            return -1;

        };

        ExecDiscount(c_card, sum);

        if (sum==0)
        {
            ;;Log("  Сумма чека равна нулю!");
            return -1;
        }

;;Log(c_card);

        DefineType(c_card, type);

        ;;Log((char*)("   > VTB24 /"+Int2Str(type)+"/").c_str());

	char t[] = "0";
	*t += type;
	char ss[16];
	sprintf(ss,"/a%u",sum);

    char *args[] = {NULL, "/o1", "/c643", ss, NULL, NULL};

    char cc_track[250];
    cc_track[0]=0;
    strcat(cc_track, "/t");
    strcat(cc_track, c_track);
    strcat(cc_track, "?");
    cc_track[250]=0;

    //! отключаем трек - ошибка при оплате на некоторых терминалах с 2020-04-27
	//if (c_track[0])  args[4] = cc_track;

;;Log("+ exec VTB pay");

// Задержка
;;Log("+ delay");

string tt = CashReg->GetCurTime();
int slp = 1; // 1 sec
while (slp)
{
    if (tt!=CashReg->GetCurTime())
    {
      slp--;
      tt = CashReg->GetCurTime();
    }
}

;;Log("- delay");

	int ret = vtb_exec(args,2);
;;Log("- exec VTB pay");
        strcpy(l_card, c_card);
        l_card[250]=0;
        if (!strchr(l_card,'*') && strlen(l_card)>8 ) //"загасим" карту
        {
            for (int i=8; l_card[i];i++) l_card[i] ='*';
        }


    int e = -1;
    char rcode[4]; rcode[0]=0;
    char sMsg[20]="";

    FILE* f = fopen("./vtb/e","r");
    if(f)
    {
        //fscanf(f,"%d,",&e);
        fgets(rcode, 4, f);

        fclose(f);
    }

    string strMsg = VTB_GetResulMsg(rcode);


        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_DATA,sum*.01,CashReg->CurrentSaleman->Id,type, c_card, a_code, b_term, strMsg.c_str(), SberbankCard);// *

// + dms ========  Акция ВТБ24 ======
	if(type==CL_VISA_VTB)
	   if (CashReg->CurrentWorkPlace)
          CashReg->CurrentWorkPlace->NeedActionVTB = true;
// - dms ========  Акция ВТБ24 ======


    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_RESULT,sum*.01,CashReg->CurrentSaleman->Id,type,  c_card, rcode, b_term, strMsg.c_str(), SberbankCard);// *


    return ret;

    } // VTB 24

    //==================================================
    //==================================================


    //;;Log("  >> SB");
    c_track[0] = 0 ;
    a_sha1[0]  = 0;

    if (type!=9)
    {
//        if(ReadCardNumber())
//        {
//            ;;cash_error* _err = new cash_error(11,0," ERROR_CARD_NUMBER (SBERBANK)");
//            ;;CashReg->LoggingError(*_err);
//            ;;delete _err;
//
//            ;;Log("ERR: Ошибка чтения карты, дальше на оплату не идем!");
//            return -1;
//
//        };

//        int resRC=0;
//        if(CashReg->SBPayPass)
//        {
//            // Команда запроса параметров карты - (код 1 = Оплата)
//            // Следующая команда оплаты должна содержать хеш карты
//            resRC = ReadCardNumber_PayPass(sum);
//
//
//
//
//           	if (!a_sha1[0])
//           	{
//           	    Log("НЕ удалось определить SHA1");
//           	    return -1;
//           	}
//
//        }
//        else
//        {
//            // Команда чтения карты (код 20)
//            resRC = ReadCardNumber();
//        }

//        if(resRC)
//        {
///*
//            ;;cash_error* _err = new cash_error(11,0," ERROR_CARD_NUMBER (SBERBANK)");
//            ;;CashReg->LoggingError(*_err);
//            ;;delete _err;
//*/
//
//            ;;Log((char*)("resRC="+Int2Str(resRC)).c_str());
//
//            ;;Log("ERR: Ошибка чтения карты, дальше на оплату не идем!");
//            return -1;
//
//        };

// + dms ===== 2019-04-24 ====== Новая схема работы по сбербанку ======

//  Первая попытка оплаты. Если оплата пройдет успешно, то напечатается чек и вернет код = 0
//  При этом сразу завершаем работу - оплата прошла.
//  Если первая попытка оплаты вернет код 4353, то значит карта попадает в программу лояльности
//  Ищем скидку по коду программу из файла "е", применяем скидку и снова подаем новую сумму на оплату.
//  Если первая попытка вернула любой другой код  - значит произошла ошибка оплаты.

            ;;Log((char*)("   >(1) SB /"+Int2Str(type)+"/").c_str());

            char ss[16];
            sprintf(ss,"%u",sum);

            char *args[] = {NULL, "1", ss, NULL};

        ;;Log("+ exec 1 pay");
            int resRC = exec(args);
        ;;Log("- exec 1 pay");

            SB_ReadCardNumber_PayPass();

            if (resRC==4353)
            {
                // ищем и применяем скидку

                ;;Log((char*)("sum before disc="+Int2Str(sum)).c_str());

                if(SBCardDiscountProgrammType)
                {
                    string loyalcard = Int2Str(SBCardDiscountProgrammType);
                    while (loyalcard.length()<8)
                    {
                        loyalcard = loyalcard+"0";
                    }
                    ExecDiscount((const char *)loyalcard.c_str(), sum);

                    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_BANKDISC,sum*.01,CashReg->CurrentSaleman->Id,SBCardDiscountProgrammType,(const char *)loyalcard.c_str());

                }
                else
                {
                    ExecDiscount(c_card, sum);
                }

                ;;Log((char*)("sum after disc="+Int2Str(sum)).c_str());

            }
            else
            {

                DefineType(c_card, type);

                strcpy(l_card, c_card);
                l_card[250]=0;
                if (!strchr(l_card,'*') && strlen(l_card)>8 ) //"загасим" карту
                {
                    for (int i=8; l_card[i];i++) l_card[i] ='*';
                }

                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_DATA,sum*.01,CashReg->CurrentSaleman->Id,type, c_card, a_code, b_term, NULL, SberbankCard);// *

                 // результат транзакции
                //* Логирование ошибок по безналу
                int e = -1;
                char sMsg[20]="";

                FILE* f = fopen("./upos/e","r");
                if(f)
                {
                    fscanf(f,"%d,",&e);
                    fgets(sMsg, 20, f);
                    fclose(f);
                }

                char rcode[10];
                sprintf(rcode, "%d", e);
                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_RESULT,sum*.01,CashReg->CurrentSaleman->Id,type,  c_card, rcode, b_term, D2W(sMsg), SberbankCard);// *

                return resRC;
            }

           	if (!a_sha1[0])
           	{
           	    Log("НЕ удалось определить SHA1");
           	    return -1;
           	}

//        }
//        else
//        {
            // Команда чтения карты (код 20)
//            resRC = ReadCardNumber();
//        }

//        if(resRC)
//        {
///*
//            ;;cash_error* _err = new cash_error(11,0," ERROR_CARD_NUMBER (SBERBANK)");
//            ;;CashReg->LoggingError(*_err);
//            ;;delete _err;
//*/
//
//            ;;Log((char*)("resRC="+Int2Str(resRC)).c_str());
//
//            ;;Log("ERR: Ошибка чтения карты, дальше на оплату не идем!");
//            return -1;
//
//        };

//;;Log((char*)("sum before disc="+Int2Str(sum)).c_str());
//
//        ExecDiscount(c_card, sum);
//
//;;Log((char*)("sum after disc="+Int2Str(sum)).c_str());

//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========
/*
        if (CashReg->CurrentWorkPlace->Cheque->GetBankBonusSum()==0)
        {
           ExecBonusDisc(c_card, sum);
        }
*/
//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========

    }

    if (sum==0)
    {
        ;;Log("  Сумма чека равна нулю!");
        return -1;
    }

    DefineType(c_card, type);

    ;;Log((char*)("   >(2) SB /"+Int2Str(type)+"/").c_str());

	char t[] = "0";
	*t += type;
	char ss[16];
	sprintf(ss,"%u",sum);

    //char *args[] = {NULL, "1", ss, t, NULL, NULL};

//	if (c_track[0])  args[4] = c_track;

//    if(CashReg->SBPayPass)
//    {
//        args[3] = "0";
//        args[4] = a_sha1;
//    }
//    else
//        if (c_track[0])  args[4] = c_track;

    char *args[] = {NULL, "1", ss, "0", a_sha1, NULL};

;;Log("+ exec 2 pay");
	int ret = exec(args);
;;Log("- exec 2 pay");
    strcpy(l_card, c_card);
    l_card[250]=0;
    if (!strchr(l_card,'*') && strlen(l_card)>8 ) //"загасим" карту
    {
        for (int i=8; l_card[i];i++) l_card[i] ='*';
    }

	CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_DATA,sum*.01,CashReg->CurrentSaleman->Id,type, c_card, a_code, b_term, NULL, SberbankCard);// *

     // результат транзакции
    //* Логирование ошибок по безналу
    int e = -1;
    char sMsg[20]="";

    FILE* f = fopen("./upos/e","r");
    if(f)
    {
        fscanf(f,"%d,",&e);
        fgets(sMsg, 20, f);
        fclose(f);
    }

    char rcode[10];
    sprintf(rcode, "%d", e);
    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_RESULT,sum*.01,CashReg->CurrentSaleman->Id,type,  c_card, rcode, b_term, D2W(sMsg), SberbankCard);// *
    //CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_RESULT,sum*.01,CashReg->CurrentSaleman->Id,type, D2W(sMsg), rcode);
    //*/
//;;ret=0; // тест

//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========
/*
    if (!ret)
    {
        ;;Log("ret");
        ;;Log("+ExecBonusPay+");
        // команда начисления баллов "Спасибо" Сбербанка
         ExecBonusPay(c_card, sum);
         ;;Log("-ExecBonusPay-");
    }
    else
    {
        ;;Log("!ret");
        //Возврат списания, если был
        //if (CashReg->CurrentWorkPlace->Cheque->GetBankBonusSum()>0)
        //if (FiscalCheque::BankBonusSumm>=0)
//        {
//!            ;;Log("run ExecBonusReverse");
//!            ExecBonusReverse(c_card);
//        }
    }
*/
//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========

	return ret;
}

int CardReader::LoggingBankReports(const char *fn, int laction, int lbank)
{
    // Пишем данные в лог файл

    FILE * f = fopen( fn, "r");

    if(!f)
        return 0;

    char idreport[40];
    // guid операции
    uuid_t r_id;
    uuid_generate(r_id);
    uuid_unparse(r_id,idreport);

    int line = 0;
    while(!feof(f))
    {
        int k=5;
        string sum_str="";
        while(!feof(f) && (k>0))
        {

            char str[64];
            fgets(str, 64, f);
            char * nl = strrchr(str, 0x0d);
            if(nl) *nl = '\0';
            nl = strrchr(str, 0x0a);
            if(nl) *nl = '\0';

            // пакуем по 5 строк
            string tmp = (lbank==1)?D2W(str):(const char*)str;

            sum_str = sum_str + tmp + "\n";

            k--;
        }

       // CashReg->AddBankAction(CashReg->GetLastCheckNum()+1, laction, CashReg->CurrentSaleman->Id, lbank, b_term, (const char*)idreport, ++line, lbank==1?D2W(str):(const char*)str);
        CashReg->AddBankAction(CashReg->GetLastCheckNum()+1, laction, CashReg->CurrentSaleman->Id, lbank, b_term, (const char*)idreport, ++line, (char*)sum_str.c_str());
    }

   fclose(f);

   return 0;
}

int CardReader::CloseDay()
{
//	CC_CloseDay();
	//GetTermNum();
	char *args[] = {NULL, "7", NULL};
	int r = exec(args, true);

    //GetTermNum();
	//LoggingBankReports(PrintFileName(), 7, 1);

	ShowMessage(NULL,"Сверка итогов сформирована!\nДля печати выберите пункт Печать сверки итогов!");

	//FILE * f = fopen(PrintFileName(), "r");
	//if(!f)
//		return r;
//	fclose(f);
	//FiscalReg->PrintString("",false);
	//FiscalReg->CutCheck();
	return r;
}

int CardReader::Return(unsigned long sum, int &type, string RRN)
{

    // VTB24
    if ((type==CL_VTB24) || (type==CL_UNKNOWN_VTB) || (type==CL_VISA_VTB) || (type==CL_MASTERCARD_VTB) || (type==CL_MAESTRO_VTB)) // ВТБ24
    {
        // VTB24

       ;;Log("  >> BTБ");
        c_track[0] = 0 ;

        ;;Log((char*)("   > VTB24 /"+Int2Str(type)+"/").c_str());

        char t[] = "0";
        *t += type;
        char ss[16];
        sprintf(ss,"/a%u",sum);

        char *args[] = {NULL, "/o3", "/c643", ss, NULL, NULL};
        //char *args[] = {NULL, "/o3", "/c643", ss, "/r629216836915", NULL};

        char cc_track[250];
        cc_track[0]=0;

        char* c_rrn = (char*)RRN.c_str();

        strcat(cc_track, "/r");
        strcat(cc_track, c_rrn);
        cc_track[250]=0;

        if (cc_track[0])  args[4] = cc_track;

        ;;Log("+ exec VTB pay");

        // Задержка
        ;;Log("+ delay");

        string tt = CashReg->GetCurTime();
        int slp = 1; // 1 sec
        while (slp)
        {
            if (tt!=CashReg->GetCurTime())
            {
              slp--;
              tt = CashReg->GetCurTime();
            }
        }

        ;;Log("- delay");

        int ret = vtb_exec(args,2);

;;Log("- exec VTB pay");


        strcpy(l_card, c_card);
        l_card[250]=0;
        if (!strchr(l_card,'*') && strlen(l_card)>8 ) //"загасим" карту
        {
            for (int i=8; l_card[i];i++) l_card[i] ='*';
        }


        int e = -1;
        char rcode[4]; rcode[0]=0;
        char sMsg[20]="";

        FILE* f = fopen("./vtb/e","r");
        if(f)
        {
            //fscanf(f,"%d,",&e);
            fgets(rcode, 4, f);

            fclose(f);
        }

        string strMsg = VTB_GetResulMsg(rcode);

        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_RETURN,sum*.01,CashReg->CurrentSaleman->Id,type, c_card, a_code, b_term, strMsg.c_str(), SberbankCard);// *

       return ret;
    }// VTB24

    // Сбербанк
    if ((type==CL_SBERBANK) || (type==CL_VISA) || (type==CL_MASTERCARD) || (type==CL_MAESTRO)) // ВТБ24
    {

        //;;Log((char*)("   > Return /"+Int2Str(type)+"/").c_str());

        ;;Log(" > Return ");

        *a_code = *b_term = *c_card = *l_card=0;

        char s[16];
        sprintf(s,"%u",sum);
        char *args[] = {NULL, "3", s, NULL};

    ;;Log("+ exec return");
        int ret = exec(args);
    ;;Log("- exec return");

    ;;Log((char*)("ret="+Int2Str(ret)).c_str());

         // результат транзакции
        FILE* f = fopen("./upos/e","r");

        //ТЕСТ
        if(f)
        {
            int e;
            fscanf(f,"%d,",&e);
            char sMsg[256];
            fgets(sMsg, 250, f);
            fgets(sMsg, 250, f);

            char* card = strdup(sMsg);

            //QMessageBox::information( NULL, "APM KACCA0", D2U(card), "Ok");

            strcpy(c_track, sMsg);
            c_track[250] = 0;

            char* track =strchr(card, '=');
            if (track!=NULL)
            {
                *track = 0;
                strcpy(c_card, card);
                c_card[31] = 0;

                char* track_pos =strchr(c_track, '\r');
                if (track_pos!=NULL)
                {
                    *track_pos=0;
                }

                track_pos =strchr(c_track, '\n');
                if (track_pos!=NULL)
                {
                    *track_pos=0;
                }

            }
            else
            {
                strcpy(c_card, card);

                strcpy(c_track, card);

                // трек 2
                char* track_pos =strchr(c_track, '\r');
                if (track_pos!=NULL)
                {
                    *track_pos=0;
                }

                track_pos =strchr(c_track, '\n');
                if (track_pos!=NULL)
                {
                    *track_pos=0;
                }


                // номер карты
                track_pos =strchr(c_card, '\r');
                if (track_pos!=NULL)
                {
                    *track_pos=0;
                }

                track_pos =strchr(c_card, '\n');
                if (track_pos!=NULL)
                {
                    *track_pos=0;
                }

            }

            // Признак банковской карты и хэш
            fgets(sMsg, 250, f);// срок действия карты
            fgets(sMsg, 250, f);// Код авторизации
            fgets(sMsg, 250, f);// Внутренний номер операции
            fgets(sMsg, 250, f);// Название типа карты

            fscanf(f,"%d\n,",&SberbankCard);//Признак карты Сбербанка (1) * возвращается 1, если карта Сбербанка, и 0 - в противном случае.

            fgets(sMsg, 250, f);// Номер терминала
            fgets(sMsg, 250, f);// Дата-время операции (ГГГГММДДччммсс)
            fgets(sMsg, 250, f);// Ссылочный номер операции (может быть пустым)
            fgets(sMsg, 250, f);// Хеш от номера карты

            strcpy(a_sha1, sMsg);
            a_sha1[250] = 0;

            QRegExp rn("\\n");
            QRegExp rrn("\\r");

            CardSHA1 = sMsg;
            CardSHA1 = U2W(W2U(CardSHA1).replace( rn, ""));
            CardSHA1 = U2W(W2U(CardSHA1).replace( rrn, ""));

            fclose(f);

            //QMessageBox::information( NULL, "APM KACCA1", D2U(c_card), "Ok");
            //QMessageBox::information( NULL, "APM KACCA2", D2U(c_track), "Ok");

            ;;Log("=====");
            ;;Log("ОПЕРАЦИЯ ВОЗВРАТ:");

            ;;Log("хэш:");
            ;;Log((char*)CardSHA1.c_str());


            ;;string tt = c_track;
            ;;Log("карта:");
            ;;Log((char*)tt.c_str());

            ;;Log("сумма:");
            ;;Log((char*)Double2Str(sum*.01).c_str());
            ;;Log("=====");

            GetTermNum();// определение b_term

            char rcode[10];
            sprintf(rcode, "%d", e);
            CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_RETURN,sum*.01,CashReg->CurrentSaleman->Id,type,  c_card, rcode, b_term, D2W(sMsg),SberbankCard);// *

        }

        return ret;
   } // Сбербанк

    return -1;
}

int CardReader::CC_write2log(STrnProperties * sTP, unsigned long check, char confirm)
{
	FILE * log = fopen("tmp/cc.log","a");
//	STrnProperties sTP;

	if(!log){
//		Log("tmp/cc.log: ошибка открытия");
		return 1;
	}

	CC_GetTrnProperties(sTP);
	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	fprintf(log, "%8d %4d %10d %16s %02d.%02d.%4d %02d:%02d %18.2f %10d %1d %10d\n",
		sTP->dwAcquirerID,
		sTP->dwTermID,
		sTP->dwAuthNumber,
		sTP->szCardNumber,
		t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min,
		sTP->lOperSum * .01,
		check,
		confirm,
		sTP->dwHR);

	fclose(log);
	return 0;
}



int CardReader::EG_Log(string logstr)
{
	FILE * log = fopen("egate/.egate.log","a");

	if(!log){
//		;;Log("egate/.egate.log: ошибка открытия");
		return 1;
	}

	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	fprintf(log, "%02d.%02d.%4d %02d:%02d:%02d> %s\n",
		t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec,
        logstr.c_str());

	fclose(log);
	return 0;
}

bool CardReader::EG_Init()
{
    //return true;

	if(idl)
		return true;
	idl = dlopen("libemvgate.so", RTLD_LAZY);//loadlibrary
	if(!idl){
	    fputs (dlerror(), stderr);
		return false;
	}

	egGetVersion            = (int (*)())dlsym(idl, "egGetVersion");
    egGetVersionDescription = (char* (*)()) dlsym(idl, "egGetVersionDescription");
    egGetLastError          = (int (*)(int))dlsym(idl, "egGetLastError");
    egGetErrorDescription   = (char* (*)(int))dlsym(idl, "egGetErrorDescription");
    GetEncryptedPinBlock    = (int (*)(char*,char*,char*,char*,int,int,char*,char*))dlsym(idl, "GetEncryptedPinBlock");
    egInitInstance          = (int (*)(const char*))dlsym(idl, "egInitInstance");
    egReleaseInstance       = (bool (*)(int))dlsym(idl, "egReleaseInstance");
    egAuthRequest           = (char* (*)(int,int,const char*))dlsym(idl, "egAuthRequest");
    egGetAuthReceipt        = (char* (*)(int))dlsym(idl, "egGetAuthReceipt");
    egGetAuthResult         = (char* (*)(int))dlsym(idl, "egGetAuthResult");

	return true;
}



bool CardReader::InitCC()
{
	if(dl)
		return true;
	dl = dlopen("libcitycard.so", RTLD_LAZY);//loadlibrary
	if(!dl){
		return false;
	}
	CC_Init = (bool (*)(const int, const char *))dlsym(dl, "CC_Init");
	CC_ReadCard = (bool (*)(const time_t))dlsym(dl, "CC_ReadCard");
	CC_LockSum = (bool (*)(const unsigned long))dlsym(dl, "CC_LockSum");
	CC_GetTrnProperties = (void (*)(STrnProperties *))dlsym(dl, "CC_GetTrnProperties");
	CC_Release = (void (*)())dlsym(dl, "CC_Release");
	CC_RollbackSum = (bool (*)())dlsym(dl, "CC_RollbackSum");
	CC_ConfirmSum = (bool (*)())dlsym(dl, "CC_ConfirmSum");
	CC_GetTrnProperties = (void (*)(STrnProperties *))dlsym(dl, "CC_GetTrnProperties");
	return true;
}

int CardReader::CC_CloseDay(unsigned long cash_desk, bool z)
{
	FILE * log = fopen("tmp/cc.log","r");
	if(!log)
		return 1;
	double s = 0;
	int rub, kop;
	int count = 0;
	char * fn = "./tmp/p";
	char * head = "           Сменный итог";
	if(z) head = "      Сменный итог с гашением";
	FILE * f = fopen( fn, "w");
	fprintf(f, "**          ПК СИТИКАРД           **\n"
			   "%s\n"
			   "  Касса # %d\n"
			   "        чек        сумма\n", head, cash_desk);
	int term;
	while(!feof(log)){
		unsigned long check;
		char confirm = 0;
		char sss[256];
		fgets(sss, 256, log);
		if(feof(log))
			break;
		if( 4 == sscanf(sss, "%*8d %4d %*10d %*16s %*2d.%*2d.%*4d %*2d:%*2d %d.%d %d %d %*10d\n", &term, &rub, &kop, &check, &confirm)
		  || confirm == 1){
			fprintf(f, " %10d  %8d.%02d\n", check, rub, kop);
			s += rub + kop * 0.01;
			count++;
		}
	}
	fclose(log);
	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	fprintf(f, "ТЕРМИНАЛ %12d %02d.%02d.%02d %02d:%02d\n",
				term, t->tm_mday, t->tm_mon + 1, t->tm_year % 100, t->tm_hour, t->tm_min);
	fprintf(f, "Платежей: %d\nНа сумму: %.2f\n", count, s);
	if(!z) fprintf(f, "\n\n");
	fclose(f);
	if(!count)
		return 1;
	FiscalReg->PrintPCheck(fn, false);
	if(!z) FiscalReg->CutCheck();
	return 0;
}

int CardReader::CC_Pay(unsigned long sum, unsigned long check)
{
	if(!InitCC()){
		QMessageBox::information( NULL, "APM KACCA", W2U("Ошибка инициализации библиотеки libcitycard.so!"), "Ok");
		return 1;
	}
	QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
	QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
	sm.setFixedSize(250,100);
	msg.setFixedSize(250,100);
	msg.setAlignment( QLabel::AlignCenter );
	msg.setText(W2U("Оплата CityCard..."));
	sm.show();
	thisApp->processEvents();

	STrnProperties sTP;

	//CC_Release();// из-за этого не работало
	if(!CC_Init(CashReg->ConfigTable->GetLongField("CARDPORT"), ".config")){
		QMessageBox::information( NULL, "APM KACCA", W2U("Ошибка инициализации!"), "Ok");
		//ShowMessage(NULL,W2U("Ошибка инициализации!"));
		return 1;
	}
	msg.setText(W2U("Проведите карту CityCard."));
	sm.show();
	thisApp->processEvents();
	while(!CC_ReadCard(10)){
		if(QMessageBox::information( NULL, "APM KACCA",
			W2U("Ошибка чтения карточки. Попробовать еще?"), W2U("Да"), W2U("Нет"))){
			CC_Release();//(200308)
			return 1;
		}
		msg.setText(W2U("Проведите карту CityCard."));
		sm.show();
		thisApp->processEvents();
	}
	//PD
	CC_GetTrnProperties(&sTP);
	doDiscount(sTP.szCardNumber);
	sum = Double2Int(CashReg->CurrentWorkPlace->Cheque->GetSumToPay() * 100);

/*	char *a, ca[10];
	DWORD dd=sTP.dwTermID;sprintf(ca,"%4d",dd);	a = ca;
	QMessageBox::information( NULL, "Kacca", a, "Ok");
	dd=sTP.dwAuthNumber;sprintf(ca,"%10d",dd);a = ca;
	QMessageBox::information( NULL, "Kacca", a, "Ok");*/


	Log("Lock summ...");
	msg.setText(W2U("Блокировка суммы..."));
	sm.show();
	thisApp->processEvents();
	CC_write2log(&sTP, check, 3);
	Log("Pre CC_LockSum(sum)");

	if(!CC_LockSum(sum)){
		Log("Post !CC_LockSum(sum)-LockSumError");
		CC_GetTrnProperties(&sTP);
		CC_write2log(&sTP, check, 0);
		CC_Release();//(200308)
		QMessageBox::information( NULL, "APM KACCA",
			W2U((QString("Ошибка блокирования суммы.\n"
			"Произведите оплату другим способом.\n") + sTP.szErrorDesc).ascii()), "Ok");
		return 1;
	}
	Log("Post CC_LockSum(sum)-LockSumOK");
	CC_write2log(&sTP, check, 2);
	msg.setText(W2U("Печать чека..."));
	sm.show();
	thisApp->processEvents();
	Log("print check");
	//print check
	{
		CC_GetTrnProperties(&sTP);
		Log("Post CC_GetTrnProperties(&sTP);");
		time_t tt = time(NULL);
		tm* t = localtime(&tt);
		char * fn = "./tmp/p";
		Log("Pre FILE * f = fopen( fn, w);");
		FILE * f = fopen( fn, "w");
		if(!f){
			QMessageBox::information( NULL, "APM KACCA",
				W2U((QString("Ошибка при распечатке чека.\n"
				"Произведите оплату другим способом.\n") + sTP.szErrorDesc).ascii()), "Ok");
			Log("Pre CC_Release();");
			CC_Release();
			Log("Post CC_Release();");
			return 1;
		}
		char b[4096];
		char p[] =
			"\n\n"
			"ПОДПИСЬ КАССИРА: ___________________\n"
			"\n\n"
			"ПОДПИСЬ КЛИЕНТА: ___________________";
		sprintf(b,
			"**          ПС СИТИКАРД           **\n"
			"ТЕРМИНАЛ %12d %02d.%02d.%02d %02d:%02d\n"
			"СЧЁТ %16s К.АВТ.%8d\n"
			"ОПЛАТА ПОКУПКИ     ВАЛЮТА      РУБЛИ\n"
			"СУММА:            %18.2f\n%s",
			sTP.dwTermID, t->tm_mday, t->tm_mon + 1, t->tm_year % 100, t->tm_hour, t->tm_min,
			sTP.szCardNumber, sTP.dwAuthNumber,
			sTP.lOperSum * .01,
			p);
		//(1)СКИДКА/НАЦЕНКА 99,99%       123 456 789 012,01р.
		//ИТОГ:            123 456 789 012,01р.
		//(2)КОМИССИЯ/ВОЗНАГРАЖДЕНИЕ:123 456 789 012,01р.

		fprintf(f, b);
		fprintf(f, "\n\n\n\n\n");//print vote here
		fprintf(f, b);
		fprintf(f, "\n\n\n\n");//print vote here
		fclose(f);
		Log("Pre FiscalReg->PrintPCheck(fn, false);");
		FiscalReg->PrintPCheck(fn, false);
	}
l_if:
	if(QMessageBox::information( NULL, "APM KACCA",
			W2U("Подтвердить оплату?"), W2U("Да"), W2U("Нет"))){
// надо подтвердить отмену оплаты
/*
		int GuardRes=NotCorrect;
		if (!CheckGuardCode(GuardRes))
			goto l_if;

		CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,QUANTITY,
						CurrentGoods->ItemSum,CashReg->CurrentSaleman->Id,GuardRes);
*/
		FiscalReg->PrintString("", false);
		FiscalReg->CutCheck();
		Log("Pre CC_RollbackSum();");
		CC_RollbackSum();
		CC_Release();// -> log
		CC_write2log(&sTP, check, 4);
		Log("Pre return 1;");
		return 1;
	}

	msg.setText(W2U("Подтверждение суммы, печать чека..."));
	sm.show();
	thisApp->processEvents();
	//CC_GetSum(&os, &ts, &tps);

	Log("Pre CC_ConfirmSum();");
	CC_ConfirmSum(); // -> log
	CC_write2log(&sTP, check);

	char *acode,*bterm,cacode[10],cbterm[10];// 25/05/07{
	//sprintf(cacode,"%10d",sTP.dwAuthNumber);
	//sprintf(cbterm,"%4d",sTP.dwTermID);
	sprintf(cacode,"%d",sTP.dwAuthNumber);
	sprintf(cbterm,"%d",sTP.dwTermID);
	acode = cacode;
	bterm = cbterm;
	CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_DATA,sum*.01,CashReg->CurrentSaleman->Id,-1,sTP.szCardNumber,acode,bterm);//} 25/05/07

//	CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_DATA,sum*.01,sTP.dwTermID,sTP.dwAuthNumber, sTP.szCardNumber);
	Log("Pre CC_Release();");
	CC_Release();// -> log
	Log("Pre return 0;");
	return 0;
}

#endif

//////////////////////////////////////////////////////////////////////
// Msgr Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Msgr::Msgr()
{
	m_type = m_number = 0;
	addr.sin_addr.s_addr = 0;
	//FILE
}

Msgr::~Msgr()
{

}


//из Windows (cp1251) в DOS (cp866)
char* Win2DOS(char * sstr)
{
   char win[]=
    {0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
    0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
    0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
    0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
    0xA8,0xB8,'\0'};
   char dos[]=
    {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
    0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
    0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
    0xF0,0xF1,'\0'};

   char str[1024];
   memset(str,0,strlen(str));
   strcpy(str, sstr);

   char l=0;
   int len=0;

   for(char k=0; k<strlen(str); k++)
   {
       if(!strchr(" 0123456789!?/|+-=(){}[]NY", str[k])) //ускоримся
       {
           l=0;
           len = strlen(win);
           while((l<len) && ((unsigned char)(str[k])!=(unsigned char)(win[l])))
           {
               l++;
           }

           if(l<len) str[k]=(unsigned char)(dos[l]);

       }
   }

   return str;
}

int Msgr::initHead(char *msg, unsigned int code)
{
	time_t ltime;
	time( &ltime );
	struct tm *now;
	now = localtime( &ltime );
//	int ret = sprintf(msg, "#CREP;%d;%s;%s;;%d;%02d%02d%04d%02d%02d%02d;%d;\"%s\";%d;",
//		code, Manufactured, ModelID, CashRegisterID,
//		now->tm_mday, now->tm_mon + 1, now->tm_year + 1900, now->tm_hour, now->tm_min, now->tm_sec,
//		CashierID, CashierName, CashierType);

    string StrCashierName = CashierName;

	int ret = sprintf(msg,"%-*s%-*d%-*d%-*d%-*s",
		3, "KKM",
		6, CashRegisterID,
		4, code,
		20, CashierID,
		30, Win2DOS((char*)StrCashierName.substr(0, 30).c_str())
		);

	return ret;
}

int Msgr::initTail(char *msg, bool addtime)
{

    int ret = 0;

    int len = strlen(msg);
    int delta = LengthMsg-len;

    if (delta>0)
    {
        ret = sprintf(msg+len,"%-*s",delta, " ");

        addtime=false; // не надо дату-время

        if (addtime)
        {
            time_t ltime;
            time( &ltime );
            struct tm *now;
            now = localtime( &ltime );

            ret = sprintf(msg+LengthMsg,"%02d%02d%02d%03d%02d%02d%02d",
            now->tm_mday,
            now->tm_mon + 1,
            (now->tm_year + 1900)%100,
            0,
            now->tm_sec,
            now->tm_min,
            now->tm_hour
            );
        }
    }

	return ret;
}

int Msgr::SetConnect(const char* host, unsigned short port)
{
	if(!port){
		addr.sin_addr.s_addr = 0;
		return -1;
	}
	hostent *hp;
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);//21845);

	unsigned long lAddr=inet_addr(host);

	if (lAddr==INADDR_NONE) // Or, is it a hostname?
	{
		hp = gethostbyname(host);
		if (!hp)
			return -1;
		else // Hostname was resolved, save the address
			memcpy(&addr.sin_addr.s_addr,hp->h_addr,hp->h_length);
	}
	else
		addr.sin_addr.s_addr = lAddr;

	return 0;
}

int Msgr::Init(const char *Mnfd, const char* Model, unsigned long sn)
{
	strcpy(Manufactured, Mnfd);
	strcpy(ModelID, Model);
	CashRegisterID = sn;
	LengthMsg = 262;
	return 0;
}

int Msgr::InitMan(unsigned long CID, const char * CName, char CType)
{
	CashierID = CID;
	strcpy(CashierName, CName);
	CashierType = CType;
	return 0;
}

int Msgr::Reg()
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 3);
	sprintf(msg + strlen(msg), "%-*d", 10, -1);
    initTail(msg);
	return send(msg);
}

int Msgr::Unreg()
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 4);
	sprintf(msg + strlen(msg), "%-*d", 10, -1);
    initTail(msg);
	return send(msg);
}

int Msgr::StartDoc(int type, unsigned int number)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 4);
	m_type = type;
	m_number = number;
	sprintf(msg + strlen(msg), "%-*d", 10, m_number);
	initTail(msg);
	return send(msg);
}

int Msgr::EndDoc()
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 5);
	sprintf(msg + strlen(msg), "%-*d", 10, m_number);
	initTail(msg);
	return send(msg);
}

int Msgr::StartKassa()
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 1003);

    initTail(msg);
	return send(msg);
}

int Msgr::FinishKassa()
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 1004);
    initTail(msg);
	return send(msg);
}


int Msgr::AddGoods(int pos, GoodsInfo * g, double sum, int input)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	//initHead(msg, 2002);
	initHead(msg, 6);

	sprintf(msg + strlen(msg), "%-*d%-*d%-*s%-*d%-*s%-*.3f%-*.3f%-*.3f%-*.3f",
		//m_type,
		10, m_number,
		3,  g->ItemNumber,
		13, g->ItemBarcode.substr(0,13).c_str(),
		30, g->ItemCode,
		30, Win2DOS((char*)(g->ItemName.substr(0,30).c_str())),
		15, g->ItemPrice,
		15, g->ItemCount,
		15, g->ItemSum,
		15, sum
		//g->InputType,
		//g->ItemCalcSum - g->ItemSum,
		//0.0
		);// скидка на чек

    initTail(msg);

	return send(msg);
}

int Msgr::EditGoods(unsigned int pos, GoodsInfo * g, double sum)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 12);

	sprintf(msg + strlen(msg), "%-*d%-*d%-*s%-*d%-*s%-*.3f%-*.3f%-*.3f%-*.3f",
		//m_type,
		10, m_number,
		3,  g->ItemNumber,
		13, g->ItemBarcode.substr(0,13).c_str(),
		30, g->ItemCode,
		30, Win2DOS((char*)g->ItemName.substr(0,30).c_str()),
		15, g->ItemPrice,
		15, g->ItemCount,
		15, g->ItemSum,
		15, sum
		);

    initTail(msg);
    return send(msg);
}

int Msgr::DeleteGoods(unsigned int pos, GoodsInfo * g, double sum)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 18);

	sprintf(msg + strlen(msg), "%-*d%-*d%-*s%-*d%-*s%-*.3f%-*.3f%-*.3f%-*.3f",
		//m_type,
		10, m_number,
		3,  g->ItemNumber,
		13, g->ItemBarcode.substr(0,13).c_str(),
		30, g->ItemCode,
		30, Win2DOS((char*)(g->ItemName.substr(0, 30).c_str())),
		15, g->ItemPrice,
		15, g->ItemCount,
		15, g->ItemSum,
		15, sum
		);

    initTail(msg);
	return send(msg);
}

int Msgr::CancelCheck(double sum, double dsum)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 25);
	sprintf(msg + strlen(msg), "%-*d%-*s%-*s%-*s%-*s%-*s%-*s%-*s%-*.3f%-*s%-*s%-*s%-*.3f",
		//m_type,
		10, m_number,
		3,  " ",
		13, " ",
		30, " ",
		30, " ",
		15, " ",
		15, " ",
		15, " ",
		15, sum,
		3,  " ",
		20, " ",
		15, " ",
		15, dsum
		);

    initTail(msg);

	sprintf(msg + strlen(msg), "%d;%d;%.2f;%.2f;", m_type, m_number, sum, dsum);
	return send(msg);
}

int Msgr::Calc(double sum, double dsum, int paytype)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 2021);
	sprintf(msg + strlen(msg), "%d;%d;%.2f;%.2f;", m_type, m_number, sum, dsum, paytype);
	initTail(msg);
	return send(msg);
}

int Msgr::Sell(double sum, double dsum, int paytype)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 37);
	//sprintf(msg + strlen(msg), "%d;%d;1;%.2f;%.2f;", m_type, m_number, sum, dsum, paytype);
	sprintf(msg + strlen(msg), "%-*d%-*s%-*s%-*s%-*s%-*s%-*s%-*s%-*.3f%-*s%-*s%-*s%-*.3f",
		//m_type,
		10, m_number,
		3,  " ",
		13, " ",
		30, " ",
		30, " ",
		15, " ",
		15, " ",
		15, " ",
		15, sum,
		3,  " ",
		20, " ",
		15, " ",
		15, dsum
		);
	initTail(msg);
	return send(msg);
}

int Msgr::Pay(double sum, double dsum, int paytype, double pay, double payback)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	//initHead(msg, 2024);
	initHead(msg, 50);
	sprintf(msg + strlen(msg), "%-*d%-*s%-*s%-*s%-*s%-*s%-*.3f%-*.3f%-*.3f%-*s%-*s%-*s%-*.3f",
		//m_type,
		10, m_number,
		3,  " ",
		13, " ",
		30, " ",
		30, " ",
		15, " ",
		15, pay,
		15, payback,
		15, sum,
		3,  " ",
		20, " ",
		15, " ",
		15, dsum
		);

    initTail(msg);
	return send(msg);
}

int Msgr::Discount(double sum, int cardtype, int number, double dsum)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 50);
	sprintf(msg + strlen(msg), "%-*d%-*s%-*s%-*s%-*s%-*s%-*s%-*s%-*.3f%-*d%-*d%-*s%-*.3f",
		//m_type,
		10, m_number,
		3,  " ",
		13, " ",
		30, " ",
		30, " ",
		15, " ",
		15, " ",
		15, " ",
		15, sum,
		3,  cardtype,
		20, number,
		15, " ",
		15, dsum
		);

    initTail(msg);

	return send(msg);
}

int Msgr::OpenMoney()
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 40);
	//sprintf(msg + strlen(msg), "-1;-1;");
	initTail(msg);
	return send(msg);
}

int Msgr::InCash(int number, int rmode, double sum)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	//initHead(msg, 3002);
	initHead(msg, 205);
	sprintf(msg + strlen(msg), "4;%d;%d;%.2f;", number, rmode, sum);
	return send(msg);
}

int Msgr::OutCash(int number, int rmode, double sum)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	//initHead(msg, 3001);
	initHead(msg, 206);
	sprintf(msg + strlen(msg), "4;%d;%d;%.2f;", number, rmode, sum);
	return send(msg);
}

int Msgr::Return(unsigned int pos, int rmode, GoodsInfo * g, double sum)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 46);

	sprintf(msg + strlen(msg), "%-*d%-*d%-*s%-*d%-*s%-*.3f%-*.3f%-*.3f%-*.3f%-*.3f",
		//m_type,
		10, m_number,
		3,  g->ItemNumber,
		13, g->ItemBarcode.substr(0,13).c_str(),
		30, g->ItemCode,
		30, Win2DOS((char*)g->ItemName.substr(0,30).c_str()),
		15, g->ItemPrice,
		15, g->ItemCount,
		15, g->ItemSum,
		15, sum,
		15, g->ItemCalcSum - g->ItemSum
		);

//	sprintf(msg + strlen(msg), "%d;%d;%d;%d;%s;%d;\"%s\";%.2f;%.3f;%.2f;%.2f;;;%.2f;;",
//		m_type,
//		m_number,
//		rmode,
//		pos,
//		g->ItemBarcode.c_str(),
//		g->ItemCode,
//		g->ItemName.c_str(),
//		g->ItemPrice,
//		g->ItemCount,
//		g->ItemSum,
//		sum,
//		g->ItemCalcSum - g->ItemSum,
//		0.0);// скидка на чек

	return send(msg);
}

int Msgr::Msg(const char * str)
{

    string msgstr = str;

	if(!addr.sin_addr.s_addr) return -1;
	char msg[4096];
	initHead(msg, 9000);//
	sprintf(msg + strlen(msg), "%-*s%-*s%-*s%-*s%-*s%-*d",
		//m_type,
		10, " ",
		3,  " ",
		13, " ",
		30, " ",
		30, Win2DOS((char*)msgstr.c_str()),
		15, msgstr.length()
		);
//	sprintf(msg + strlen(msg), "%d;%s;", strlen(str), str);
	initTail(msg);
	return send(msg);
}

#include "qtextcodec.h"
int Msgr::send(char *msg)
{
	if(!addr.sin_addr.s_addr)
		return -1;
	unsigned int sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (sock==-1)
		return -1;
/*
	unsigned char cc = 0x00;
	char * p = msg + 6;
	for(; *p; p++){
		cc ^= *p;
	}
	cc ^= (*(p++) = '0');
	cc ^= (*(p++) = '*');
	sprintf( p, "%2X\x0d\x0a", cc);
//*/

sendto(sock,msg,strlen(msg),0,(const sockaddr *)&addr,sizeof(sockaddr_in));
close(sock);

;;FILE * f = fopen( "./tmp/msg.log", "a");
if(f)
{
;;fprintf(f,"%4d|", strlen(msg));
;;fprintf(f,msg);
;;fprintf(f,"\n");
;;fclose(f);
}

return 0;
}

int Msgr::Check()
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 6000);
	sprintf(msg + strlen(msg), "4;0;1");
	return send(msg);
}

int CardReader::doDiscount(const char *card)
{
	if(!CashReg->CurrentWorkPlace)
		return 0;
	char prefix[8];
	strncpy(prefix, card, 6);
	prefix[6] = 0;
	if(!CashReg->BankDiscountTable->Locate("BANKID", prefix))
		return 0;
	if(CashReg->CurrentWorkPlace->DiscountType.size() == 0)
	{
		CashReg->CurrentWorkPlace->CurrentCustomer = CashReg->BankDiscountTable->GetLongField("IDCUST");
		CashReg->CurrentWorkPlace->DiscountType = CashReg->BankDiscountTable->GetField("DISCOUNTS");
		CashReg->CurrentWorkPlace->ProcessDiscount();
		return 1;
	}
	for(int i = 0; i < CashReg->CurrentWorkPlace->DiscountType.size(); i++)
	{
		if(CashReg->CurrentWorkPlace->DiscountType[i] == '.')
			continue;
		if(CashReg->BankDiscountTable->GetField("DISCOUNTS")[i] == '.')
			CashReg->CurrentWorkPlace->DiscountType[i] = '.';
	}
	CashReg->CurrentWorkPlace->ProcessDiscount();
	return 1;
}

//////////////////////////////////////////////////////////////////////
// CRSBupos Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRSBupos::CRSBupos()
{

}

CRSBupos::~CRSBupos()
{

}


const char* CardReader::PrintFileName()
{
	static char p[] = "./sb_pilot/p";
	return p;
}

const char* EGate::PrintFileName()
{
	static char p[] = "./egate/p";
	return p;
}

const char* CRSBupos::PrintFileName()
{
	static char p[] = "./upos/p";//cheque.txt";
	return p;
}

const char* CardReader::EG_PrintFileName()
{
	static char p[] = "./egate/p";
	return p;
}

const char* CardReader::VTB_PrintFileName()
{
	static char p[] = "./vtb/p";
	return p;
}

const char* CardReader::PCX_PrintFileName()
{
	static char p[] = "./pcx/p";
	return p;
}


EGate::EGate()
{

}

EGate::~EGate()
{

}


bool EGate::LoggingEvent()
{
    return true;
}

void EGate::GetTermNum()
{

}


int EGate::exec(char **argv)
{
    return 0;
}


string ReadLine(FILE* f)
{
    char buf[1024];

    fgets(buf, 1024, f);

    char * nl;
	nl = strrchr(buf, 0x0d);//'\n');
	if(nl) *nl = '\0';
	nl = strrchr(buf, 0x0a);//'\n');
	if(nl) *nl = '\0';

    string ret = buf;
    return ret;
}

//string trim(string str)
//{
//    string resstr = "";
//
//    int beg=0, end=0, i=0;
//    bool stopbeg = false;
//
//    while (i<str.length())
//    {
//        if (str[i]==' ')
//        {
//            if (!stopbeg) beg=i;
//        }
//        else
//        {
//            stopbeg = true;
//            end = i;
//        }
//
//        i++;
//    }
//
//    for(i=beg;i<=end;i++) resstr += str[i];
//
//    return resstr;
//}

int CardReader::EG_Pay(unsigned long sum, unsigned long check)
{
    return 0;
}

int CardReader::eg_exec(char** argv)
{

	char cmd[256] = "/Kacca/egate/emvgate";
	char cmdcpy[256];
	int retsys;
	//qApp->setOverrideCursor(QApplication::waitCursor);
	FILE* f;

	QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
	QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
	sm.setFixedSize(250,100);
	msg.setText(W2U("Работа с терминалом Газпромбанка...\nСмотрите на экран терминала."));
	msg.setFixedSize(250,100);
	msg.setAlignment( QLabel::AlignCenter );
	sm.show();
	thisApp->processEvents();

	argv[0] = cmd;//"emvgate";
	for(int i = 1; argv[i]; i++){
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	}
	chdir("/Kacca/egate");
	//FILE *lf;
	const char* tmpbuf;

	char logfile[]="./.egate.log";
	strcpy(cmdcpy,cmd);
	f = fopen(logfile, "a");
	tmpbuf=CashReg->GetCurDate().c_str();
	fwrite(tmpbuf,10,1,f);
    fprintf(f, " ");
	tmpbuf=CashReg->GetCurTime().c_str();
	fwrite(tmpbuf,8,1,f);
	fprintf(f, "> =KKM= start =\n");
//	fprintf(f,"%s -- ",cmdcpy+(strchr(cmdcpy,' ')-cmdcpy));
	fclose(f);


    ;;printf("%s", cmd);

	//! start !//
	retsys=system(cmd);

	f = fopen(logfile, "a");
	tmpbuf=CashReg->GetCurDate().c_str();
	fwrite(tmpbuf,10,1,f);
    fprintf(f, " ");
	tmpbuf=CashReg->GetCurTime().c_str();
	fwrite(tmpbuf,8,1,f);
	fprintf(f, "> =KKM= finish (%d) =\n",retsys);
	fprintf(f, "\n",retsys);
	fclose(f);
	chdir("/Kacca");
//	LoggingUpos();
	//FILE* f;
	f = fopen("./egate/e","r");
	if(!f) return -1;
	int e;
	fscanf(f,"%d ",&e);
	char sMsg[256];
	fgets(sMsg, 250, f);
	//LoggingUpos();
	if(e){
		QMessageBox::information( NULL, "APM KACCA", W2U(sMsg), "Ok");
	}

/*
char a_code[16];
char b_term[16];
char c_card[32];
*/
//	fgets(c_card, 32, f);
//	fgets(sMsg, 250, f);/// переделать бы
//	fgets(a_code, 16, f);
	//fgets(b_term, 16, f);//
	fclose(f);
	//GetTermNum();// определение b_term
	//LoggingUpos();
	return e || FiscalReg->PrintPCheck(EG_PrintFileName(), false);
}

// cnt_prncheck - кол-во чеков выводимых на печать
// от 0 до 2х
int CardReader::vtb_exec(char** argv, int cnt_prncheck=0, bool closeday)
{
	char cmd[256] = "/Kacca/vtb/cashreg";
	char cmdcpy[256];
	int retsys;


	//+ dms ==== 2014-12-17 ==== Удалим файлы перед оплатой ======
	char efile[]="/Kacca/vtb/e";
	char pfile[]="/Kacca/vtb/p";

    remove(efile);
    remove(pfile);
    //- dms ==== 2014-12-17 ====


	//qApp->setOverrideCursor(QApplication::waitCursor);
	FILE* f;

	QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
	QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
	sm.setFixedSize(250,100);
	msg.setText(W2U("Работа с терминалом ВТБ24...\nСмотрите на экран терминала."));
	msg.setFixedSize(250,100);
	msg.setAlignment( QLabel::AlignCenter );
	sm.show();
	thisApp->processEvents();

	argv[0] = cmd;//"emvgate";
	for(int i = 1; argv[i]; i++){
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	}

	chdir("/Kacca/vtb");

	const char* tmpbuf;

;;Log((char*)cmd);

	char logfile[]="./vtb.log";
	strcpy(cmdcpy,cmd);
	f = fopen(logfile, "a");
    fprintf(f, "\n");
	tmpbuf=CashReg->GetCurDate().c_str();	fwrite(tmpbuf,10,1,f);  	fprintf(f, " ");
	tmpbuf=CashReg->GetCurTime().c_str();	fwrite(tmpbuf,8,1,f);
	//fprintf(f, "> = start = %d %d %d \n", CashReg->GetMagNum(), CashReg->GetCashRegisterNum(), CashReg->GetLastCheckNum()+1);
	fprintf(f, "> = start = %d = \n", CashReg->GetLastCheckNum()+1);

	tmpbuf=CashReg->GetCurDate().c_str();	fwrite(tmpbuf,10,1,f);    fprintf(f, " ");
	tmpbuf=CashReg->GetCurTime().c_str();	fwrite(tmpbuf,8,1,f);
	fprintf(f, "> %s\n", cmd);
//	fprintf(f,"%s -- ",cmdcpy+(strchr(cmdcpy,' ')-cmdcpy));
	fclose(f);

	//! start !//
	retsys=system(cmd);

	chdir("/Kacca");

	f = fopen("./vtb/e","r");
	if(!f) return -1;
	int e;
	fscanf(f,"%d ",&e);
	char sMsg[256];
	fgets(sMsg, 250, f);
	fgets(sMsg, 250, f);
	fgets(a_code, 20, f);

    fclose(f);


	f = fopen("./vtb/e","r");
	if(!f) return -1;
	char ret_code[4];
	fgets(ret_code, 4, f);
    fclose(f);

    string resStr = VTB_GetResulMsg(ret_code);

	if(e){
		QMessageBox::information( NULL, "APM KACCA", W2U(resStr), "Ok");
	}

	GetTermNumVTB();// определение b_term
	LoggingVTB();

    f = fopen("./vtb/vtb.log", "a");
	if (f)
	{
        tmpbuf=CashReg->GetCurDate().c_str();
        fwrite(tmpbuf,10,1,f);
        fprintf(f, " ");
        tmpbuf=CashReg->GetCurTime().c_str();
        fwrite(tmpbuf,8,1,f);
        fprintf(f, "> = finish (%d=%s) =\n",retsys, resStr.c_str());
        fclose(f);
	}

    int prnres = 0;

    // Частая ошибка при сверке итогов = 402 (ВТБшники проблему не решают)
    // =>  просто печатаем сверку
    if(closeday && (e==402)) e=0;

    LoggingBankReports(VTB_PrintFileName(), closeday?7:1, 2);

    if(closeday){
        return e; // сверку не печатаем
    }

	// Печатаем 2 чека
	if (!e)
        for(int i=1;i<=cnt_prncheck;i++)
        {
            if (i>1) FiscalReg->PrintString("Копия 1",false);
            prnres = prnres || FiscalReg->PrintPCheck(VTB_PrintFileName(), false);
        }

	return e || prnres || retsys;
}


string CardReader::VTB_GetResulMsg(string ret_code)
{

    char resStr[256];
    resStr[255]=0;


    string ret_code_str = ret_code;
    char* sbstr=(char*)(ret_code_str+"=").c_str();

    ;;Log((char*)(sbstr));

	FILE* frs = fopen("./vtb/rc_res_8583.INI","r");
	if(frs)
	{
        while(!feof(frs))
        {
            char str[250];
            fgets(str, 250, frs);

            if(strstr(str,sbstr)!=NULL)
            {

                strcpy(resStr, str);
            }
        }

        fclose(frs);
	}


	char * nl;
	nl = strrchr(resStr, 0x0d);//'\n');
	if(nl) *nl = '\0';
	nl = strrchr(resStr, 0x0a);//'\n');
	if(nl) *nl = '\0';

    return resStr;
}


int CardReader::pcx_exec(char** argv)
{

	//char cmd[256] = "sudo /Kacca/pcx/pcx";
	char cmd[256] = "/Kacca/pcx/pcx";
	char cmdcpy[256];
	int retsys;
	//qApp->setOverrideCursor(QApplication::waitCursor);
	FILE* f;

	QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
	QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
	sm.setFixedSize(430,50);
	msg.setText(W2U("Выполнение операции с ПЦ \"Лояльность\"..."));
	msg.setFixedSize(430,50);
	msg.setAlignment( QLabel::AlignCenter );
	sm.show();
	thisApp->processEvents();


	argv[0] = cmd;//"emvgate";
	for(int i = 1; argv[i]; i++){
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	}
	chdir("/Kacca/pcx");
	//FILE *lf;
	const char* tmpbuf;

	char logfile[]="./.pcx.log";
	strcpy(cmdcpy,cmd);
	f = fopen(logfile, "a");
	tmpbuf=CashReg->GetCurDate().c_str();
	fwrite(tmpbuf,10,1,f);
    fprintf(f, " ");
	tmpbuf=CashReg->GetCurTime().c_str();
	fwrite(tmpbuf,8,1,f);
	fprintf(f, "> =KKM= start =\n");
//	fprintf(f,"%s -- ",cmdcpy+(strchr(cmdcpy,' ')-cmdcpy));
	fclose(f);


   // ;;printf("%s", cmd);

	//! start !//
	retsys=system(cmd);

	f = fopen(logfile, "a");
	tmpbuf=CashReg->GetCurDate().c_str();
	fwrite(tmpbuf,10,1,f);
    fprintf(f, " ");
	tmpbuf=CashReg->GetCurTime().c_str();
	fwrite(tmpbuf,8,1,f);
	fprintf(f, "> =KKM= finish (%d) =\n",retsys);
	fprintf(f, "\n",retsys);
	fclose(f);
	chdir("/Kacca");
//	LoggingUpos();
	//FILE* f;


	f = fopen("./pcx/e","r");
	if(!f) return -1;

	int e;
	fscanf(f,"%d ",&e);
	char sMsg[256];
	fgets(sMsg, 250, f);
	//LoggingUpos();
	if(e){
//		QMessageBox::information( NULL, "APM KACCA", W2U(sMsg), "Ok");
	}

/*
char a_code[16];
char b_term[16];
char c_card[32];
*/
//	fgets(c_card, 32, f);
//	fgets(sMsg, 250, f);/// переделать бы
//	fgets(a_code, 16, f);
	//fgets(b_term, 16, f);//
	fclose(f);
	//GetTermNum();// определение b_term
	//LoggingUpos();
	return e || FiscalReg->PrintPCheck(PCX_PrintFileName(), false);
}


int CardReader::VTB_CloseDay(unsigned long cashdesk, bool z)
{

// 4 - сверка итоговых сумм
// 6 - запрос журнала операций

    char ks[10], ch[18];
    sprintf(ks,"%d", CashReg->GetCashRegisterNum());
    sprintf(ch,"%d", CashReg->GetLastCheckNum()+1);

    char *args[] = {NULL, "/o7", NULL, NULL, NULL, NULL};
    //char *args[] = {NULL, ks, "6", "0", "0", NULL};
	int ret = vtb_exec(args,1,true); // сверка итогов

    //GetTermNumVTB();
    //LoggingBankReports(VTB_PrintFileName(), 7, 2);

    ShowMessage(NULL,"Сверка итогов сформирована!\nДля печати выберите пункт Печать сверки итогов!");

        // результат транзакции
    //* Логирование ошибок по безналу
    int e = -1;
    char sMsg[20]="";

    FILE* f = fopen("./vtb/e","r");
    if(f)
    {
        fscanf(f,"%d ",&e);
        fgets(sMsg, 20, f);
        fclose(f);
    }

    double summ=0;
    int success = 0;
    FILE* fs = fopen("./vtb/p","r");
    if(fs)
    {
        if (1==fscanf(fs,"%f          \n          ИТОГИ СОВПАЛИ",&summ))
        {
            success = 1;
        }
        else
        {
            if(1==fscanf(fs,"%f\nИТОГИ НЕ СОВПАЛИ",&summ))
                success = -1;
        }

        fclose(fs);
    }

    char rcode[10];
    sprintf(rcode, "%d", e);
    CashReg->LoggingAction(0,CL_CLOSEDAY,summ,CashReg->CurrentSaleman->Id,success, sMsg, rcode);
    //*/

    return ret;

}

int CardReader::EG_CloseDay(unsigned long cashdesk, bool z)
{


// 4 - сверка итоговых сумм
// 6 - запрос журнала операций

    char ks[10], ch[18];
    sprintf(ks,"%d", CashReg->GetCashRegisterNum());
    sprintf(ch,"%d", CashReg->GetLastCheckNum()+1);

    char *args[] = {NULL, ks, "4", "0", "0", NULL};
    //char *args[] = {NULL, ks, "6", "0", "0", NULL};
	int ret = eg_exec(args);

        // результат транзакции
    //* Логирование ошибок по безналу
    int e = -1;
    char sMsg[20]="";

    FILE* f = fopen("./egate/e","r");
    if(f)
    {
        fscanf(f,"%d ",&e);
        fgets(sMsg, 20, f);
        fclose(f);
    }

    double summ=0;
    int success = 0;
    FILE* fs = fopen("./egate/p","r");
    if(fs)
    {
        if (1==fscanf(fs,"%f          \n          ИТОГИ СОВПАЛИ",&summ))
        {
            success = 1;
        }
        else
        {
            if(1==fscanf(fs,"%f\nИТОГИ НЕ СОВПАЛИ",&summ))
                success = -1;
        }

        fclose(fs);
    }

    char rcode[10];
    sprintf(rcode, "%d", e);
    CashReg->LoggingAction(0,CL_CLOSEDAY,summ,CashReg->CurrentSaleman->Id,success, sMsg, rcode);
    //*/

    return ret;

}

int CardReader::EG_Return(unsigned long sum)
{
        return 0;
}



bool CRSBupos::LoggingUpos(/*int sysret*/)//
{

	//time_t ltime;
//	CashRegister CR;
	FILE *f1, *f2, *f3;
	bool existpe = false;
	char tmp[160], ch, prech;
	char pfile[]="./upos/p";
	char efile[]="./upos/e";
	char logfile[]="./upos/sb.log";
	const char* tmpbuf;
	char* retbuf;

	f3 = fopen(logfile, "ab");
	tmpbuf=CashReg->GetCurDate().c_str();
	fwrite(tmpbuf,10,1,f3);
	fprintf(f3, " ");
	tmpbuf=CashReg->GetCurTime().c_str();
	fwrite(tmpbuf,8,1,f3);
	//sprintf(retbuf,"(%i)\n",3110);
	//fwrite(retbuf,strlen(retbuf)-1,1,f3);
	fprintf(f3, " ");

	if (f2 = fopen(efile, "rb")) {
		for(ch = fgetc(f2); !feof(f2) ; ch = fgetc(f2))
			if (!feof(f2))
			// &&  ch != 10 && ch != 13 && ch != 32)
				fputc(ch, f3);
		existpe = true;
		fclose(f2);}
	else
		fwrite("no'e'", 5, 1, f3);
/*	if (f1 = fopen(pfile, "rb")) {
		fread(tmp,160,1,f1);
		for(ch = fgetc(f1); !feof(f1) && ch!='_'; prech = ch, ch = fgetc(f1)){
			if (!feof(f1) &&  ch==0x69 && prech==0x1b){
				fread(tmp,160,1,f1);
				ch  = fgetc(f1);
			}
			if (!feof(f1) &&  ch!=10 && ch!=13 && ch!=32 && ch!='-' && ch!='=' && ch!='*' && ch!=0x1b)
				fputc(ch, f3);
		}
		existpe = true;
		fclose(f1); }
	else
		fwrite("no'p'", 5, 1, f3);
*/

	if (f1 = fopen(pfile, "rb")) {
		for(ch = fgetc(f1); !feof(f1) ; ch = fgetc(f1))
			if (!feof(f1))
                //if (ch != 10 && ch != 13 && ch != 32)
                    fputc(ch, f3);
                //else
                  //  fputc(';', f3);


//		fread(tmp,160,1,f1);
//		for(ch = fgetc(f1); !feof(f1) && ch!='_'; prech = ch, ch = fgetc(f1)){
//			if (!feof(f1) &&  ch==0x69 && prech==0x1b){
//				fread(tmp,160,1,f1);
//				ch  = fgetc(f1);
//			}
//			if (!feof(f1) &&  ch!=10 && ch!=13 && ch!=32 && ch!='-' && ch!='=' && ch!='*' && ch!=0x1b)
//				fputc(ch, f3);
//		}

		existpe = true;
		fclose(f1); }
	else
		fwrite("no'p'", 5, 1, f3);


	fputc('\n', f3);
	fclose(f3);
	return existpe;

}

bool CardReader::LoggingVTB(/*int sysret*/)//
{

	//time_t ltime;
//	CashRegister CR;
	FILE *f1, *f2, *f3;
	bool existpe = false;
	char tmp[160], ch, prech;
	char pfile[]="./vtb/p";
	char efile[]="./vtb/e";
	char logfile[]="./vtb/vtb.log";
	const char* tmpbuf;
	char* retbuf;

	f3 = fopen(logfile, "ab");
	tmpbuf=CashReg->GetCurDate().c_str();
	fwrite(tmpbuf,10,1,f3);
	fprintf(f3, " ");
	tmpbuf=CashReg->GetCurTime().c_str();
	fwrite(tmpbuf,8,1,f3);
	//sprintf(retbuf,"(%i)\n",3110);
	//fwrite(retbuf,strlen(retbuf)-1,1,f3);
	fprintf(f3, ">");

	if (f2 = fopen(efile, "rb")) {
		for(ch = fgetc(f2); !feof(f2) ; ch = fgetc(f2))
			if (!feof(f2))
                if (ch != 10 && ch != 13 && ch != 32)
                    fputc(ch, f3);
                else
                    fputc(';', f3);

		existpe = true;
		fclose(f2);}
	else
		fwrite("no'e'", 5, 1, f3);

    fprintf(f3, "\n");

	if (f1 = fopen(pfile, "rb")) {
		for(ch = fgetc(f1); !feof(f1) ; ch = fgetc(f1))
			if (!feof(f1))
                //if (ch != 10 && ch != 13 && ch != 32)
                    fputc(ch, f3);
                //else
                  //  fputc(';', f3);


//		fread(tmp,160,1,f1);
//		for(ch = fgetc(f1); !feof(f1) && ch!='_'; prech = ch, ch = fgetc(f1)){
//			if (!feof(f1) &&  ch==0x69 && prech==0x1b){
//				fread(tmp,160,1,f1);
//				ch  = fgetc(f1);
//			}
//			if (!feof(f1) &&  ch!=10 && ch!=13 && ch!=32 && ch!='-' && ch!='=' && ch!='*' && ch!=0x1b)
//				fputc(ch, f3);
//		}

		existpe = true;
		fclose(f1); }
	else
		fwrite("no'p'", 5, 1, f3);
	fputc('\n', f3);
	fputc('\n', f3);
	fclose(f3);
	return existpe;

}


int CardReader::CC_Test(unsigned long sum, unsigned long check)
{
	if(!InitCC()){
		QMessageBox::information( NULL, "Test", W2U("Ошибка инициализации библиотеки libcitycard.so!"), "Ok");
		return 1;
	}
//	QLabel *wnd = new QLabel;
//	wnd->setText(W2U(" Тест оплаты "));
//	wnd->show();

	QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
	QLabel msg(&sm);//, NULL, Qt::W	Type_TopLevel);
	msg.setFixedSize(250,100);
	msg.setFixedSize(250,100);
	msg.setAlignment( QLabel::AlignCenter );
	msg.setText(W2U(" Тест оплаты CityCard..."));
	sm.show();
	thisApp->processEvents();

	STrnProperties sTP;
	if(!CC_Init(CashReg->ConfigTable->GetLongField("CARDPORT"), ".config")){
		QMessageBox::information( NULL, "Test", W2U("Ошибка инициализации!"), "Ok");
		return 1;
	}
	msg.setText(W2U("Тест:Проведите карту CityCard."));
	sm.show();
	thisApp->processEvents();
	while(!CC_ReadCard(10)){
		if(QMessageBox::information( NULL,"Test",W2U("Ошибка чтения карточки. Попробовать еще?"), W2U("Да"), W2U("Нет")))
			return 1;
		msg.setText(W2U("Тест:Проведите карту CityCard."));
		sm.show();
		thisApp->processEvents();
	}
	//PD
	CC_GetTrnProperties(&sTP);
	doDiscount(sTP.szCardNumber);
	sum = 1;

	Log("Start Test:Lock summ...");
	msg.setText(W2U("Тест:Блокировка суммы..."));
	sm.show();
	thisApp->processEvents();
	CC_write2log(&sTP, check, 3);

	Log("Test:Pre CC_LockSum(sum)");
	if(!CC_LockSum(sum)){
		Log("Test:Post !CC_LockSum(sum)");
		CC_GetTrnProperties(&sTP);
		CC_write2log(&sTP, check, 0);
		QMessageBox::information( NULL, "Test",
			W2U((QString("Ошибка блокирования суммы.\n") + sTP.szErrorDesc).ascii()), "Ok");
		return 1;
	}
	Log("Test:Post CC_LockSum(sum)");
	CC_write2log(&sTP, check, 2);
	{
		QMessageBox::information( NULL, "Test",
				W2U((QString("Далее отмена продажи")).ascii()), "Ok");
		Log("Pre CC_RollbackSum();");
		CC_RollbackSum();
		CC_Release();// -> log
		CC_write2log(&sTP, check, 4);
		Log("Test OK");
		QMessageBox::information( NULL, "Test",	W2U((QString("Test OK")).ascii()), "Ok");
		return 1;
	}

}

void CRSBupos::GetTermNum()
{
	FILE *f;
	char num[20];
	f=fopen("./upos/p","r");
	if (!f) {//если нет чека
		f=fopen("./upos/bterm","r");
		if (!f){//если нет файла с номером терминала
			strcpy(b_term,"-1");
			return;
		}
		else{// если есть файл
			fscanf(f,"%s",num);
			fclose(f);
			strncpy(b_term,num,20);
			return;
		}
	}
	else{//если есть чек
		char term[40];
		while(!feof(f)){
			fscanf(f,"%s",term);
			if (strstr(term,"етнйобм") || strstr(term,"ЕТНЙОБМ") || strstr(term,"ерминал") || strstr(term,"ЕРМИНАЛ")){
				fscanf(f,"%s", num);
				fclose(f);
				if(f=fopen("./upos/bterm","w")){
					fprintf(f,"%s",num);
					fclose(f);
					return;
				}
			}
		}
		fclose(f);
				f=fopen("./upos/bterm","r");
				if (!f){//если нет файла с номером терминала
					strcpy(b_term,"-2");
					return;
				}
				else{// если есть файл
					fscanf(f,"%s",num);
					strncpy(b_term,num,20);
					fclose(f);
					return;
				}
	}
	strncpy(b_term,num,20);
	return;
}

void CardReader::GetTermNumVTB()
{
	FILE *f;
	char num[20];
	f=fopen("./vtb/p","r");
	if (!f) {//если нет чека
		f=fopen("./vtb/bterm","r");
		if (!f){//если нет файла с номером терминала
			strcpy(b_term,"-1");
			return;
		}
		else{// если есть файл
			fscanf(f,"%s",num);
			fclose(f);
			strncpy(b_term,num,20);
			return;
		}
	}
	else{//если есть чек
		char term[40];
		while(!feof(f)){
			fscanf(f,"%s",term);
			if (strstr(term,"етнйобм") || strstr(term,"ЕТНЙОБМ") || strstr(term,"ерминал") || strstr(term,"ЕРМИНАЛ")){
				fscanf(f,"%s", num);
				fclose(f);
				if(f=fopen("./vtb/bterm","w")){
					fprintf(f,"%s",num);
					fclose(f);
					return;
				}
			}
		}
		fclose(f);
				f=fopen("./vtb/bterm","r");
				if (!f){//если нет файла с номером терминала
					strcpy(b_term,"-2");
					return;
				}
				else{// если есть файл
					fscanf(f,"%s",num);
					strncpy(b_term,num,20);
					fclose(f);
					return;
				}
	}
	strncpy(b_term,num,20);
	return;
}


void CardReader::GetTermNum()
{
	return ;
}

//**************************************
/*MsgrP::MsgrP()
{

	m_type = m_number = 0;
	addr.sin_addr.s_addr = 0;
	//FILE
}

MsgrP::~MsgrP()
{

}

int MsgrP::initHead(char *msg, unsigned int code)//
{

	return sprintf(msg, "KKM;%d;%d;%d;\"%s\"",
		CashRegisterID, code, CashierID, CashierName);
}

int MsgrP::SetConnect(const char* host, unsigned short port)//
{
	if(!port){
		addr.sin_addr.s_addr = 0;
		return -1;
	}
	m_port=port;
	hostent *hp;
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);//21845-prizma);

	unsigned long lAddr=inet_addr(host);

	if (lAddr==INADDR_NONE) // Or, is it a hostname?
	{
		hp = gethostbyname(host);
		if (!hp)
			return -1;
		else // Hostname was resolved, save the address
			memcpy(&addr.sin_addr.s_addr,hp->h_addr,hp->h_length);
	}
	else
		addr.sin_addr.s_addr = lAddr;

	return 0;
}

int MsgrP::Init(const char *Mnfd, const char* Model, unsigned long sn)//
{
	strcpy(Manufactured, Mnfd);
	strcpy(ModelID, Model);
	CashRegisterID = sn;
	return 0;
}

int MsgrP::InitMan(unsigned long CID, const char * CName, char CType)//
{
	CashierID = CID;
	strcpy(CashierName, CName);
	CashierType = CType;
	return 0;
}

int MsgrP::Reg()//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 3);
	sprintf(msg + strlen(msg), "0;0;;;;;;;0;;;;;");
	return send(msg);
}

int MsgrP::Unreg()//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 80);
	sprintf(msg + strlen(msg), "0;0;;;;;;;0;;;;;");
//	sprintf(msg + strlen(msg), "0*1C\x0d\x0a");
	return send(msg);
}

int MsgrP::StartDoc(int type, unsigned int number)//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	m_type=type;
	switch (type){
		case 1:initHead(msg, 4);
		case 2:initHead(msg, 46);
		default: return -1;
	}
	sprintf(msg + strlen(msg), "%d;0;;;;;;;0;;;;;", m_number=number);
	return send(msg);
}

int MsgrP::EndDoc()//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	switch (m_type){
		case 1:initHead(msg, 5);
		case 2:initHead(msg, 47);
		default: return -1;
	}
	sprintf(msg + strlen(msg), "%d;%d;;;;;;;%d;;;;;",m_number,m_pos,m_sum);
	return send(msg);
}

int MsgrP::AddGoods(int pos, GoodsInfo * g, double sum, int input)//
{
	if(!addr.sin_addr.s_addr) return -1;
	m_pos=g->ItemNumber;
	char msg[1024];
	initHead(msg, 6);
	sprintf(msg + strlen(msg), "%d;%d;%s;%d;\"%s\";%.2f;%.3f;%.2f;%.2f;",
		//m_type,
		m_number,
		g->ItemNumber,
		g->ItemBarcode.c_str(),
		g->ItemCode,
		g->ItemName.c_str(),
		g->ItemPrice,
		g->ItemCount,
		g->ItemSum,
		m_sum=sum
		//g->InputType,
		//g->ItemCalcSum - g->ItemSum,
		//0.0
		);//
	return send(msg);
}

int MsgrP::EditGoods(unsigned int pos, GoodsInfo * g, double sum)//
{
	if(!addr.sin_addr.s_addr) return -1;
	m_pos=g->ItemNumber;
	char msg[1024];
	initHead(msg, 12);
	sprintf(msg + strlen(msg), "%d;%d;%s;%d;\"%s\";%.2f;%.3f;%.2f;%.2f;;;;;",
		//m_type,
		m_number,
		g->ItemNumber,
		g->ItemBarcode.c_str(),
		g->ItemCode,
		g->ItemName.c_str(),
		g->ItemPrice,
		g->ItemCount,
		g->ItemSum,
		m_sum=sum
		//g->InputType,
		//g->ItemCalcSum - g->ItemSum,
		//0.0
		);//
	return send(msg);
}

int MsgrP::DeleteGoods(unsigned int pos, GoodsInfo * g, double sum)//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 18);
		sprintf(msg + strlen(msg), "%d;%d;%s;%d;\"%s\";%.2f;%.3f;%.2f;%.2f;;;;;",
		//m_type,
		m_number,
		g->ItemNumber,
		g->ItemBarcode.c_str(),
		g->ItemCode,
		g->ItemName.c_str(),
		g->ItemPrice,
		g->ItemCount,
		g->ItemSum,
		m_sum=sum
		//g->InputType,
		//g->ItemCalcSum - g->ItemSum,
		//0.0
		);

	return send(msg);
}

int MsgrP::CancelCheck(double sum, double dsum)//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 25);
	sprintf(msg + strlen(msg), "%d;m_pos;%.2f;;;;;", m_number, sum);
	return send(msg);
}

int MsgrP::Calc(double sum, double dsum, int paytype)//
{
	return 0;
}

int MsgrP::Sell(double sum, double dsum, int paytype)//
{
	return 0;
}

int MsgrP::Pay(double sum, double dsum, int paytype, double pay, double payback)//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	if (paytype==0)
		initHead(msg, 37);
	else
		initHead(msg, 38);
	sprintf(msg + strlen(msg), "%d;%d;%.2f;%.2f;", m_number, m_pos, payback, sum);
	return send(msg);
}

int MsgrP::Discount(double sum, int cardtype, int number, double dsum)//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 35);
	sprintf(msg + strlen(msg), "%d;;;;;;;;;%.2f;%d;%d;%.2f;", m_number, sum, cardtype, number, dsum);
	return send(msg);
}

int MsgrP::OpenMoney()//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 40);
	sprintf(msg + strlen(msg), "%d;%d;;;;;;;0;;;;;",m_number,m_pos);
	return send(msg);
}

int MsgrP::InCash(int number, int rmode, double sum)//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 78);
	sprintf(msg + strlen(msg), "0;;;;;;;;%d;;;;;", sum);
	return send(msg);
}

int MsgrP::OutCash(int number, int rmode, double sum)//
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 79);
	sprintf(msg + strlen(msg), "0;;;;;;;;%d;;;;;", sum);
	return send(msg);
}

int MsgrP::Return(unsigned int pos, int rmode, GoodsInfo * g, double sum)//
{
	return AddGoods(pos, g, sum, 4);
}

int MsgrP::Msg(const char * str)
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[4096];
	initHead(msg, 9000);//
	sprintf(msg + strlen(msg), "%d;%s;", strlen(str), str);
	return send(msg);
}

int MsgrP::send(char *msg)//
{
	if(!addr.sin_addr.s_addr)
		return -1;
	unsigned int sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (sock==-1)
		return -1;
		sendto(sock,msg,strlen(msg),0,(const sockaddr *)&addr,sizeof(sockaddr_in));
	close(sock);
	return 0;
}

int MsgrP::Check()
{
	if(!addr.sin_addr.s_addr) return -1;
	char msg[1024];
	initHead(msg, 81);
	sprintf(msg + strlen(msg), "0;0;;;;;;;;;;;;");
	return send(msg);
}
*/
