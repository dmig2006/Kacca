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

#include "ElvisFiscalRegVer2.h"
#include "Utils.h"
#include "Cash.h"
#include "DbfTable.h"

#include "Payment.h"


#define	STR_PRINT
#define REG_DEBUG

enum {STX=0x02,ETX=0x03,EOT=0x04,ENQ=0x05,ACK=0x06,DLE=0x10,NAK=0x15,};


// Elvis KKM implementation
ElvisRegVer2::ElvisRegVer2(void)
{
	RegCom=0;
	DataNLen=47;//default buffer's length
	MCU=100;//minimal currency unit is equal 100 kop.
	memset(DataN,0,DataNLen);
	sum = 0;
}

ElvisRegVer2::~ElvisRegVer2(void)
{
	if (RegCom!=0)
		ClosePort(RegCom);
}

void ElvisRegVer2::RemoveDLE(void)
{
	for (unsigned int i=0;i<DataNLen-1;i++)//remove mask symbols
		if ((DataN[i]==DLE)&&((DataN[i+1]==DLE)||(DataN[i+1]==ETX)))
		{
			memcpy(DataN+i,DataN+i+1,DataNLen-i-1);
			DataN[DataNLen-1]=0;
		}
}

int ElvisRegVer2::InitReg(int port,int baud,int type,const char *passwd)
{
	mult=10;

	unsigned int tmp[]={0, mult*1, mult*4, mult*1, mult*1, mult*4, mult*1, mult*1, mult*2,};
	memcpy(t,tmp,9*sizeof(unsigned int));//set standart timeouts

	WaitPeriod=t[5];

	RegPort=port;
	RegBaud=baud;
	RegType=type;
	RegTimeout=10000;//wait response from the serial port fewer than 10 sec.
	RegPassword=passwd;

	if (RegCom!=0)
	{
		char val;
		switch (baud)
		{
			case   1200:val=1;break;
			case   2400:val=2;break;
			case   4800:val=3;break;
			case   9600:val=4;break;
			case  19200:return -1;
			case  38400:val=6;break;
			case  57600:val=7;break;
			case 115200:val=8;break;
			default:    val=3;break;
		}
		SetTable(9,1,2,&val,1,false);
		ClosePort(RegCom);
		RegCom=0;
	}

	return ResetReg();
}

int ElvisRegVer2::ResetReg(void)
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
		case ElvesMiniType:LineLength=24;RegSpeed=400;EntireCheck=false;break;
		case ShtrichVer2Type:LineLength=34;RegSpeed=75;EntireCheck=false;break;
		case ElvesMiniEntireType:LineLength=24;RegSpeed=400;EntireCheck=true;break;
		case ShtrichVer2EntireType:LineLength=34;RegSpeed=75;EntireCheck=true;break;
		default:LineLength=24;RegSpeed=400;EntireCheck=false;break;
	}

	return SetMode(1,RegPassword.c_str());
}

int ElvisRegVer2::ProcessInstruction(unsigned int len)
{
	if ((RegCom==0)||(len==0))
		return -1;

	unsigned int i;

	memmove(DataN+2,DataN,len);//add default password (0000)
	DataN[0]=DataN[1]=0x00;
	len+=2;

	i=0;
	while ((i<len)&&(i<DataNLen))//add mask symbol
		if ((DataN[i]==DLE)||(DataN[i]==ETX))
		{
			memmove(DataN+i+1,DataN+i,(len++) - i);
			DataN[i]=DLE;
			i+=2;
		}
		else
			i++;

	DataN[len++]=ETX;//add etx symbol to the end of array

	unsigned char CRC=0;//evaluate check sum
	for (i=0;i<len;i++)
		CRC^=DataN[i];

	memmove(DataN+1,DataN,len++);//final action
	DataN[0]=STX;
	DataN[len++]=CRC;

	int N=10,N1=100;
	int FRC=0,RC=0;
	unsigned char buf;

	while (RC<5)
	{
		buf=ENQ;WriteToPort(RegCom,&buf,1);WaitSomeTime(t[1]);//send request

		if (ReadFromPort(RegCom,&buf,1)==0)
		{
			buf=EOT;WriteToPort(RegCom,&buf,1);return -1;//if port keeps the silence
		}

		if (buf==NAK)
		{
			WaitSomeTime(t[1]);RC++;continue;//if register is not ready
		}

		if (buf==ENQ)
			WaitSomeTime(t[7]);

		if (buf!=ACK)


		{
			if (FRC<N1)
			{
				FRC++;RC=0;continue;
			}
			else
			{
				buf=EOT;WriteToPort(RegCom,&buf,1);return -1;
			}
		}
		else
			break;
	}

	if (RC==5)
	{
		buf=EOT;WriteToPort(RegCom,&buf,1);return -1;//if we did not receive confirm byte ACK
	}

	RC=0;

	while(RC<N)
	{
		WriteToPort(RegCom,DataN,len);WaitSomeTime(t[3]);//send instruction to the register
		if ((ReadFromPort(RegCom,&buf,1)==0)||(buf!=ACK)||((buf==ENQ)&&(RC==1)))
		{
			RC++;continue;//if something wrong we try again
		}

		if ((buf==ACK)||((buf==ENQ)&&(RC==1)))
			break;
	}

	if (RC==N)
	{
		buf=EOT;WriteToPort(RegCom,&buf,1);return -1;//if we failed to get correct answer
	}

	if (buf==ACK)
	{
		buf=EOT;WriteToPort(RegCom,&buf,1);RC=0;//send confirmation
	}

	while(RC<N1)
	{
		WaitSomeTime(WaitPeriod);//specific timeouts, they depend on the type of operation

		if (ReadFromPort(RegCom,&buf,1)==0)
		{
			return -2;
		}

		if (buf!=ENQ)
		{
			RC++;continue;
		}


		FRC=0;
		bool SendACK=true;

		while (FRC<N)
		{
			if (SendACK)
			{
				buf=ACK;WriteToPort(RegCom,&buf,1);
			}

			RC=0;

			while (RC<N1)
			{
				WaitSomeTime(t[2]);

				if (ReadFromPort(RegCom,&buf,1)==0)
				{
					return -2;
				}

				if ((buf!=STX)&&(buf!=ENQ))
				{
					RC++;continue;
				}

				break;
			}

			if (RC==N1)
				return -2;

			if (buf==ENQ)
			{
				FRC++;
				if (FRC<N)
					continue;
				else
					return -2;
			}

L1:		memset(DataN,0,DataNLen);

			unsigned int BC=0;
			bool DLE_Flag=false;

			while (BC<DataNLen)
			{
				WaitSomeTime(t[6]);
				if (ReadFromPort(RegCom,&buf,1)==0)
				{
					BC=DataNLen;break;
				}

				if (DLE_Flag)
					DLE_Flag=false;
				else
					if (buf==DLE)
						DLE_Flag=true;
					else
						if (buf==ETX)
						{
							WaitSomeTime(t[6]);
							if (ReadFromPort(RegCom,&buf,1)==0)
							{
								BC=DataNLen;break;
							}

							unsigned char CRC=ETX;
							unsigned int i;

							for (i=0;i<BC;i++)
								CRC^=DataN[i];

							if (CRC!=buf)
							{
								buf=NAK;WriteToPort(RegCom,&buf,1);BC=DataNLen;
								break;
							}

							buf=ACK;WriteToPort(RegCom,&buf,1);WaitSomeTime(t[4]);

							if((ReadFromPort(RegCom,&buf,1)==0)||(buf==EOT))
								return 0;

							if (buf==STX)
							{
								if (++FRC<N)
									goto L1;
								else
									return -2;
							}

							WaitSomeTime(200);

							if (ReadFromPort(RegCom,&buf,1)==0)
								return 0;

							BC=DataNLen;break;
						}

				if (BC<DataNLen)
				{
					DataN[BC]=buf;BC++;
				}
			}

			if (BC==DataNLen)
			{
				SendACK=false;FRC++;continue;
			}
		}
	}

	return -2;
}

int ElvisRegVer2::JustDoIt(unsigned int len,int timeout)
{
	ClearPort(RegCom);
	if (RegTimeout<timeout)
		WaitPeriod=timeout;
	else
		WaitPeriod=0;
	int er=ProcessInstruction(len);
	WaitPeriod=t[5];
	if (er!=0)
		return er;
	if (DataN[0]!=0x55)
		return -2;

	if (DataN[1]==DLE)
	{
		DataN[1]=DataN[2];
		DataN[2]=0;
	}

	return DataN[1];
}

void ElvisRegVer2::PrepNum(unsigned char *dest,int len,double val,int prec)
{
	char buf[255];
	memset(buf,0,255);
	string format_str="%0";
	format_str=format_str+Int2Str(len*2)+".0f";
	sprintf(buf,format_str.c_str(),val*prec);
	for (int i=0;i<len*2;i+=2)
	{
		char num[]={0,0,0};
		num[0]=buf[i];
		num[1]=buf[i+1];
		sscanf(num,"%x",(int*)(dest+i/2));
	}
}

int ElvisRegVer2::Sale(const char* Name,double Price,double Quantity,int Department)
{
	if (EntireCheck)
		return 0;
	else
		return SaleGoods(Name,Price,Quantity,Department,false);
}

int ElvisRegVer2::SaleGoods(const char* Name,double Price,double Quantity,int Department,bool wholeCheck)
{
	int er;

	if (!wholeCheck)
	{
		er = SetMode(1,RegPassword.c_str());
		if (er!=0)
			return er;
	}

	er=PrintString(Name);
	if (er!=0)
		return er;

	memset(DataN,0,DataNLen);
	DataN[0]=0x52;
	DataN[1]=0x00;
	PrepNum(DataN+2,5,Price,MCU);
	PrepNum(DataN+7,5,Quantity,1000);

	if (fabs(Quantity-1)<epsilon)
		return JustDoIt(13,RegSpeed);
	else
		return JustDoIt(13,RegSpeed*2);
}

int ElvisRegVer2::Storno(const char* Name,double Price,double Quantity,int Department)
{
	if (EntireCheck)
		return 0;
	else
		return StornoGoods(Name,Price,Quantity,Department,false);
}

int ElvisRegVer2::StornoGoods(const char* Name,double Price,double Quantity,int Department,bool wholeCheck)
{
	int er;

	if (!wholeCheck)
	{
		er = SetMode(1,RegPassword.c_str());
		if (er!=0)
			return er;
	}

	er=PrintString(Name);
	if (er!=0)
		return er;

	memset(DataN,0,DataNLen);
	DataN[0]=0x4E;
	DataN[1]=0x00;
	PrepNum(DataN+2,5,Price,MCU);
	PrepNum(DataN+7,5,Quantity,1000);

	if (fabs(Quantity-1)<epsilon)
		return JustDoIt(13,RegSpeed*3);
	else
		return JustDoIt(13,RegSpeed*4);
}

int ElvisRegVer2::Return(const char* Name,double Price,double Quantity,int Department)
{
	if (EntireCheck)
		return 0;
	else
		return ReturnGoods(Name,Price,Quantity,Department,false);
}

int ElvisRegVer2::ReturnGoods(const char* Name,double Price,double Quantity,int Department,bool wholeCheck)
{
	int er;

	if (!wholeCheck)
	{
		er = SetMode(1,RegPassword.c_str());
		if (er!=0)
			return er;
	}

	er=PrintString(Name);
	if (er!=0)
		return er;

	memset(DataN,0,DataNLen);
	DataN[0]=0x57;
	DataN[1]=0x00;
	PrepNum(DataN+2,5,Price,MCU);
	PrepNum(DataN+7,5,Quantity,1000);

	if (fabs(Quantity-1)<epsilon)
		return JustDoIt(12,RegSpeed*3);
	else
		return JustDoIt(12,RegSpeed*4);
}

int ElvisRegVer2::GetStatus(void)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x3F;

	WaitPeriod=t[5];
	int er=ProcessInstruction(1);
	if (er<0)
		return er;
	if (DataN[0]!=0x44)
		return -2;
	else
	{
		RemoveDLE();
		return 0;
	}
}

int ElvisRegVer2::Cancel(void)
{
	int er = GetStatus();
	if (er!=0)
		return er;

	if (DataN[0x16]&0x03!=0){
		memset(DataN,0,DataNLen);
		DataN[0]=0x59;
		int er = JustDoIt(1,RegSpeed*7);
		if (er!=0)
			return er;
	}else{
		if(CashRegister::ConfigTable->GetLogicalField("CHECKOPEN")){
			PrintString("ЧЕК АННУЛИРОВАН");
			PrintString("");
			PrintString("");
			PrintHeader();
			CutCheck();
			CashRegister::ConfigTable->PutField("CHECKOPEN","F");
		}
	}

	Sales.clear();
	sum = 0;
	return 0;
}

int ElvisRegVer2::Close(double Summ,double Disc,int CashDesk,int CheckNum,int PaymentType, long long CustomerCode, string CashmanName,string vote,string info)
{
	int er=0;
	int type_op;
	if (CashDesk < 0)
		CashDesk *= -1;
	//static double sum = 0;

	if (EntireCheck)
	{
		CashRegister::ConfigTable->PutField("CHECKOPEN","T");
		er = SetMode(1,RegPassword.c_str());
		if (er!=0)
			return er;

		while(!Sales.empty())
		{
			SalesRecord tmpSales=Sales[0];
#ifdef STR_PRINT
 			er = GoodsStr(tmpSales.Name.c_str(),tmpSales.Price,tmpSales.Quantity);
			if(er)
				return er;
			sum += RoundToNearest(tmpSales.Price * tmpSales.Quantity,.01);//floor(tmpSales.Price * tmpSales.Quantity * 100) / 100.f;
			type_op = tmpSales.Type;
#else
			switch (tmpSales.Type)
			{
				case SL:
					er=SaleGoods(tmpSales.Name.c_str(),tmpSales.Price,
						tmpSales.Quantity,tmpSales.Department,true);
					break;
				case ST:
					er=StornoGoods(tmpSales.Name.c_str(),tmpSales.Price,
						tmpSales.Quantity,tmpSales.Department,true);
					break;
				case RT:
					er=ReturnGoods(tmpSales.Name.c_str(),tmpSales.Price,
						tmpSales.Quantity,tmpSales.Department,true);
					break;
				default:er=-1;break;
			}

			if (er!=0)
				return er;
#endif

			Sales.erase(Sales.begin(),Sales.begin()+1);
		}
	}

#ifdef STR_PRINT
	switch(type_op){
	case SL:
		er = SaleGoods("------------------------------------",sum - FiscalCheque::tsum,1,0,false);
		break;
	case RT:
		er = ReturnGoods("------------------------------------",sum - FiscalCheque::tsum,1,0,false);
		break;
	default:
		return -1;
	}
	if(er)
		return er;
	sum = 0;
#endif

	er = CreateInfoStr(CashDesk,CheckNum,PaymentType,vote,info);
	CashmanName = "Кассир " + CashmanName;
	er = PrintString(CashmanName.c_str(), false);
	if (er!=0)
		return er;

	if (fabs(Disc)>epsilon)
	{
		if (EntireCheck)
		{
			er = GetStatus();
			if (er!=0)
				return er;

			char num[11];
			for (int i=0;i<5;i++)
				sprintf(num+i*2,"%02x",DataN[i+0x17]);

			float Result;
			sscanf(num,"%f",&Result);

			er = CreatePriceStr("ВСЕГО",Result/MCU+Disc);
			if (er!=0)
				return er;

			if(fabs(Disc - FiscalCheque::tsum) > 0.005)
			{
				string tmpStr;
				if(CustomerCode) tmpStr = "СКИДКА по карте " + LLong2Str(CustomerCode);
					else tmpStr = "СКИДКА";
				er = CreatePriceStr(tmpStr.c_str(),Disc - FiscalCheque::tsum);
				if (er!=0)
					return er;
			}

			if(FiscalCheque::tsum > .005){
				er = CreatePriceStr("СКИДКА-ОКРУГЛЕНИЕ",FiscalCheque::tsum);
				if (er!=0)
					return er;
			}
		}
		else
		{
			memset(DataN,0,DataNLen);
			DataN[0]=0x43;
			DataN[3]=1;
			PrepNum(DataN+5,5,Disc,MCU);
			er = JustDoIt(10,t[5]);
			if (er!=0)
				return er;
		}
	}

	memset(DataN,0,DataNLen);
	DataN[0]=0x4A;

	switch(PaymentType)//0x01 - cash, 0x02 - credit, 0x03 - bag, 0x04 - cashless
	{
		case CashPayment:DataN[2]=0x01;break;
//		case CashlessPayment:DataN[2]=0x04;break;
		default:DataN[2]=0x04;break;
	}

	int delay=0;
	if (fabs(Disc)>epsilon)
		delay+=RegSpeed*2;

	if (fabs(Summ)<epsilon)
		delay+=RegSpeed*4;
	else
		delay+=RegSpeed*6;

	CashRegister::ConfigTable->PutField("CHECKOPEN","F");
	PrepNum(DataN+3,5,Summ,MCU);
	return JustDoIt(8,delay);
}

int ElvisRegVer2::GetMode(char* mode,char* submode,char* paper,char* printer)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x45;

	WaitPeriod=t[5];
	int er=ProcessInstruction(1);
	if (er<0)
		return er;
	if (DataN[0]!=0x55)
	    return -2;

	RemoveDLE();

	if (mode!=NULL)
		*mode=DataN[1]&0x0F;
	if (submode!=NULL)
		*submode=(DataN[1]>>4)&0x0F;
	if (paper!=NULL)
		*paper=DataN[2]&0x01;
	if (printer!=NULL)
		*printer=(DataN[2]>>1)&0x01;

	return 0;
}

int ElvisRegVer2::SetMode(int mode,const char *passwd)
{
	if (mode!=6)
	{
		char md=-1;
		int er=GetMode(&md);
		if (er!=0)
			return er;

		if (md==mode)
			return 0;

		memset(DataN,0,DataNLen);
		DataN[0]=0x48;

		er = JustDoIt(1,t[5]);
		if (er!=0)
			return er;

		if (mode==0)
			return 0;
	}
	else
		mode=5;//try to unlock FR after tax officer password

	memset(DataN,0,DataNLen);
	DataN[0]=0x56;
	DataN[1]=mode;
	char high[]={0,0,};
	char low[]={0,0,};
	char code[]={0,0,0,0,0,0,0,0,};
	unsigned int i;
	for (i=0;(i<strlen(passwd))&&(i<8);i++)
		code[7-i]=passwd[strlen(passwd)-i-1];

	for (i=0;i<8;i+=2)
	{
		high[0]=code[7-i-1];
		low[0]=code[7-i];
		DataN[5-i/2]=atoi(high)*16+atoi(low);
	}

	return JustDoIt(6,t[5]);
}

void ElvisRegVer2::OpenDrawer(void)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x80;
	JustDoIt(1,t[5]);
}

int ElvisRegVer2::CashIncome(double Sum)
{
	int er=SetMode(1,RegPassword.c_str());
	if (er!=0)
		return er;
	memset(DataN,0,DataNLen);
	DataN[0]=0x49;
	PrepNum(DataN+2,5,Sum,MCU);
	return JustDoIt(7,RegSpeed*4);
}

int ElvisRegVer2::CashOutcome(double Sum)
{
	int er=SetMode(1,RegPassword.c_str());
	if (er!=0)
		return er;
	memset(DataN,0,DataNLen);
	DataN[0]=0x4F;
	PrepNum(DataN+2,5,Sum,MCU);
	return JustDoIt(7,RegSpeed*4);
}

string ElvisRegVer2::GetErrorStr(int er,int lang)
{
	if (lang==0)
		switch(er)
		{

			case  -3: return "Снятие отчета прервалось";
			case  -2: return "Нет ответа";
			case  -1: return "Нет связи";
			case   0: return "";
			case   1: return "Контрольная лента обработана без ошибок";
			case   8: return "Неверная цена (сумма)";
			case  10: return "Неверное количество";
			case  12: return "Невозможно сторно последней операции";
			case  13: return "Сторно по коду невозможно \n(в чеке зарегистрировано меньшее количество товаров с указанным кодом)";
			case  14: return "Невозможен повтор последней операции";
			case  15: return "Повторная скидка на операцию невозможна";
			case  16: return "Скидка/надбавка на предыдущую операцию невозможна";
			case  17: return "Неверный код товара";
			case  18: return "Неверный штрих-код товара";
			case  19: return "Неверный формат";
			case  20: return "Неверная длина";
			case  21: return "ККМ заблокирована в режиме ввода даты";
			case  22: return "Требуется подтверждение ввода даты";
			case  24: return "Нет больше данных для передачи ПО ККМ";
			case  50: return "Переполнение таблицы заказов";
			case  51: return "Невозможен возврат товара";
			case  61: return "Товар не найден";
			case  62: return "Весовой штрих-код с количеством <>1.000";
			case  63: return "Переполнение буфера чека";
			case  64: return "Недостаточное количество товара";
			case  65: return "Сторнируемое количество больше проданного";
			case  66: return "Заблокированный товар не найден в буфере чека";
			case  67: return "Данный товар не продавался в чеке, сторно невозможно";
			case  68: return "Memo Plus 3 заблокировано с ПК";
			case  69: return "Ошибка контрольной суммы таблицы настроек Memo PlusTM 3TM";
			case  70: return "Неверная команда от ККМ";
			case 102: return "Команда не реализуется в данном режиме ККМ";
			case 103: return "Нет бумаги";
			case 104: return "Нет связи с принтером чеков";
			case 115: return "Накопление меньше суммы возврата или аннулирования";
			case 122: return "Данная модель ККМ не может выполнить команду";
			case 123: return "Неверная величина скидки / надбавки";
			case 124: return "Операция после скидки / надбавки невозможна";
			case 125: return "Неверная секция";
			case 126: return "Неверный вид оплаты";
			case 127: return "Переполнение при умножении";
			case 128: return "Операция запрещена в таблице настроек";
			case 129: return "Переполнение итога чека";
			case 130: return "Открыт чек аннулирования - операция невозможна";
			case 132: return "Переполнение буфера контрольной ленты";
			case 134: return "Вносимая клиентом сумма меньше суммы чека";
			case 135: return "Открыт чек возврата - операция невозможна";
			case 136: return "Смена превысила 24 часа";
			case 137: return "Открыт чек продажи - операция невозможна";
			case 140: return "Неверный пароль";
			case 141: return "Буфер контрольной ленты не переполнен";
			case 142: return "Идет обработка контрольной ленты";
			case 143: return "Обнуленная касса (повторное гашение невозможно)";
			case 145: return "Неверный номер таблицы";
			case 146: return "Неверный номер ряда";
			case 147: return "Неверный номер поля";
			case 148: return "Неверная дата";
			case 149: return "Неверное время";
			case 150: return "Сумма чека по секции меньше суммы сторно";
			case 151: return "Подсчет суммы сдачи невозможен";
			case 152: return "В ККМ нет денег для выплаты";
			case 154: return "Чек закрыт - операция невозможна";
			case 155: return "Чек открыт - операция невозможна";
			case 156: return "Смена открыта, операция невозможна";
			case 157: return "ККМ заблокирована, ждет ввода пароля налогового инспектора";
			case 158: return "Заводской номер уже задан";
			case 159: return "Количество перерегистраций не может быть более 4";
			case 160: return "Ошибка Ф.П.";
			case 162: return "Неверная смена";
			case 163: return "Неверный тип отчета";
			case 164: return "Недопустимый пароль";
			case 165: return "Недопустимый заводской номер ККМ";
			case 166: return "Недопустимый РНМ";
			case 167: return "Недопустимый ИНН";
			case 168: return "ККМ не фискализирована";
			case 169: return "Не задан заводской номер";
			case 170: return "Нет отчетов";
			case 171: return "Режим не активизирован";
			case 172: return "Нет указанного чека в КЛ";
			case 173: return "Нет больше записей КЛ";
			case 174: return "Некорректный код или номер лицензии";
			case 176: return "Требуется выполнение общего гашения";
			case 177: return "Команда не разрешена введенными лицензиями";
			case 178: return "Невозможна отмена скидки/надбавки";
			case 179: return "Невозможно закрыть чек данным типом оплаты \n(в чеке присутствуют операции без контроля наличных)";
			case 180: return "Неверный номер маршрута";
			case 181: return "Неверный номер начальной зоны";
			case 182: return "Неверный номер конечной зоны";
			case 183: return "Неверный тип тарифа";
			case 184: return "Неверный тариф";
			case 200: return "Нет устройства, обрабатывающего данную команду";
			case 201: return "Нет связи с внешним устройством";
			case 202: return "Неверное состояние пульта ТРК";
			case 203: return "В чеке продажи топлива возможна только одна регистрация";
			case 204: return "Неверный номер пульта ТРК";
			case 255: return "Более одной клавиши ККМ нажаты одновременно";
			default:  return "Неизвестная ошибка";
		}

	if (lang==1)
		switch(er)
		{
			case  -3: return "The report was aborted";
			case  -2: return "No response";
			case  -1: return "No connection";
			case   0: return "";
			case   1: return "The reference tape was processed without errors";
			case   8: return "Wrong price (sum)";
			case  10: return "Wrong quantity";
			case  12: return "The storno of the last operation is impossible";
			case  13: return "The storno by code is impossible\n(insuffient quantity of goods with the selected code was registered in the cheque";
			case  14: return "Repeating of the last operation is impossible";
			case  15: return "Repeated discount for the operation is impossible";
			case  16: return "Discount/increase for the previous operation is impossible";
			case  17: return "Wrong goods code";
			case  18: return "Wrong goods barcode";
			case  19: return "Wrong format";
			case  20: return "Wrong length";
			case  21: return "Fiscal register is blocked in the mode of entering of the date";
			case  22: return "Confirmation of entering of date is required";
			case  24: return "No more data for the transmission to fiscal register";
			case  51: return "The return of the goods is impossible";
			case  61: return "The goods is not found";
			case  62: return "Weight barcode with the quantity <>1.000";
			case  63: return "Overflow of the cheque buffer";
			case  64: return "Insufficient quantity of the goods";
			case  65: return "The storno quantity is less than sale quantity";
			case  66: return "Blocked goods is not found in the buffer of cheque";
			case  67: return "This goods was not saled in the cheque, storno is impossible";
			case  68: return "Memo Plus 3 is blocked with PC";
			case  69: return "Error of the check sum in the setup table Memo PlusTM 3TM";
			case  70: return "Wrong instruction from fiscal register";
			case 102: return "Instruction is not released in this fiscal register's mode";
			case 103: return "Out of paper";
			case 104: return "No connection with cheque printer";
			case 115: return "Total sum is less than the sum of return or annulation";
			case 122: return "This type of the fiscal register can not process instruction";
			case 123: return "Wrong value of the discount/increase";
			case 124: return "Operation after discount/release is impossible";
			case 125: return "Wrong section";
			case 126: return "Wrong type of payment";
			case 127: return "Overflow during multiplication";
			case 128: return "Operation is inhibited in the setup table";
			case 129: return "Overflow of the cheque summary";
			case 130: return "Annulation cheque is opened - operation is impossible";
			case 132: return "Overflow of the reference tape's buffer";
			case 134: return "Income sum is less than cheque's sum";
			case 135: return "Return cheque is opened - operation is impossible";
			case 136: return "Shift is over 24 hours";
			case 137: return "Sale cheque is opened - operation is impossible";
			case 140: return "Wrong password";
			case 141: return "Buffer of the reference tape is not overflowed";
			case 142: return "Reference tape is processed";
			case 143: return "Cash register is empty (repeated z-report is impossible";
			case 145: return "Wrong number of the table";
			case 146: return "Wrong number of the row";
			case 147: return "Wrong number of the field";
			case 148: return "Wrong date";
			case 149: return "Wrong time";
			case 150: return "Cheque's sum by section is less than storno's sum";
			case 151: return "Calculation of the change's sum is impossible";
			case 152: return "Insufficient sum of money for payment in fiscal register";
			case 154: return "The cheque is closed - operation is impossible";
			case 155: return "The cheque is opened - operation is impossible";
			case 156: return "Shift is opened, operation is impossible";
			case 157: return "Fiscal register is blocked, it waits entering of the tax officer's password";
			case 158: return "Plant number is became";
			case 159: return "Quantity of the re-registrations can't be than four";
			case 160: return "Error of the fiscal memory";
			case 162: return "Wrong shift";
			case 163: return "Wrong report's type";
			case 164: return "Wrong password";
			case 165: return "Wrong plant's number of the fiscal register";
			case 166: return "Wrong RNM";
			case 167: return "Wrong INN";
			case 168: return "Fiscal register is not fiscalized";
			case 169: return "Plant's number is not set";
			case 170: return "No reports";
			case 171: return "Mode is not activated";
			case 172: return "No selected cheque in KL";
			case 173: return "No more records in KL";
			case 174: return "Wrong code or number of license";
			case 176: return "Common closing is required";
			case 177: return "Instruction is not permitted by entered licenses";
			case 178: return "Cancel of discount/charge is impossible";
			case 179: return "One can not close cheque with this payment's type\n(one exists operation without cash's control in the cheque)";
			case 180: return "Wrong path's number";
			case 181: return "Wrong number of the initial zone";
			case 182: return "Wrong number of the final zone";
			case 183: return "Wrong rate's type";
			case 184: return "Wrong rate";
			case 200: return "One has not device which executes this instruction";
			case 201: return "No connection with external device";
			case 202: return "Wrong status of the TRK's console";
			case 203: return "One can exists only one registration in the cheque of the fuel's sale";
			case 204: return "Wrong number of the TRK's console";
			case 255: return "More than one key of fiscal register is pressed simultaneously";
			default:  return "Unknown error";
		}

	return "";
}

int ElvisRegVer2::XReport(void)
{
	int er = SetMode(2,RegPassword.c_str());
	if (er!=0)
		return er;

	memset(DataN,0,DataNLen);
	DataN[0]=0x67;
	DataN[1]=1;

	er = JustDoIt(2,t[5]);
	if (er!=0)
		return er;

	char mode=-1,submode=-1,paper=-1,printer=-1;
	do
	{
		WaitSomeTime(2000);
		GetMode(&mode,&submode,&paper,&printer);
	}
	while((mode==2)&&(submode==2));

	if ((mode==2)&&(submode==0))
	{

		if (paper==1)
			er=103;
		else
			if (printer==1)
				er=-2;
			else
				er=0;
	}
	else
		er=-3;

	return er;
}

void ElvisRegVer2::SetDBArchPath(string path)
{

}

int ElvisRegVer2::ZReport(double& sum)
{
	int er;
/*
	er = GetCurrentSumm(sum);
	if(er)
		return er;
	if(sum > .01){
		er = CashOutcome(sum);
		if(er)
			return er;
	}
*/
	er = SetMode(3,RegPassword.c_str());
	if (er!=0)
		return er;

	char mode,submode,paper,printer;

	memset(DataN,0,DataNLen);
	DataN[0]=0x5A;

	er = JustDoIt(1,t[5]);
	if (er!=0)
		return er;

	do
	{
		WaitSomeTime(2000);
		GetMode(&mode,&submode,&paper,&printer);
	}
	while((mode==3)&&(submode==2));

	if ((mode!=7)||(submode!=1))
	{
		if (paper==1)

			return 103;
		else
			if (printer==1)
				return -2;
	}
	else
		do
		{
			WaitSomeTime(2000);
			GetMode(&mode,&submode,&paper,&printer);
		}
		while((mode==7)&&(mode==1));

	memset(DataN,0,DataNLen);
	DataN[0]=0x58;

	WaitPeriod=t[5];
	er=ProcessInstruction(1);
	if (er!=0)
		return er;

	if (DataN[0]!=0x55)
		return -2;

	RemoveDLE();

	if (DataN[1]!=0)
	    return DataN[1];

	char num[15];
	memset(num,0,15);

	for (int i=0;i<7;i++)
		sprintf(num+i*2,"%02x",DataN[i+2]);

	sum=atof(num)/MCU;

	return 0;
}

int ElvisRegVer2::GetCurrentSumm(double& sum)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x4D;

	WaitPeriod=t[5];
	int er=ProcessInstruction(1);
	if (er!=0)
		return er;

	if (DataN[0]!=0x4D)
		return -2;

	char num[15];
	memset(num,0,15);

	for (int i=0;i<7;i++)
		sprintf(num+i*2,"%02x",DataN[i+1]);

	sum=atof(num)/MCU;

	return 0;
}

int ElvisRegVer2::SetTime(int Hour,int Minute,int Second)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x4B;

	PrepNum(DataN+1,1,Hour,1);
	PrepNum(DataN+2,1,Minute,1);
	PrepNum(DataN+3,1,Second,1);
	return JustDoIt(4,t[5]);
}

int ElvisRegVer2::SetDate(int Day,int Month,int Year)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x64;
	PrepNum(DataN+1,1,Day,1);
	PrepNum(DataN+2,1,Month,1);
	PrepNum(DataN+3,1,Year,1);
	return JustDoIt(4,RegSpeed*7);
}

bool ElvisRegVer2::IsPaperOut(int er)
{
	if (er==103)
		return true;
	else
		return false;
}

bool ElvisRegVer2::IsFullCheck(void)
{
	return EntireCheck;
}

bool ElvisRegVer2::RestorePaperBreak(void)
{
	return false;
}

int ElvisRegVer2::PrintString(const char* str,bool ctrl)
{
	extern unsigned char win2alt[];

	unsigned char *buf = new unsigned char[strlen(str)+1];
	memset(buf,0,strlen(str)+1);

	for (unsigned int i=0;i<strlen(str);i++)
		buf[i]=win2alt[(unsigned char)str[i]];
	memset(DataN,0,DataNLen);
	DataN[0]=0x4C;
	strncpy((char*)(DataN+1),(const char*)buf,LineLength);

	delete buf;

	return JustDoIt(strlen((char*)DataN),0);
}

void ElvisRegVer2::WaitNextHeader(void)
{
	WaitSomeTime(RegSpeed*7);
}

int ElvisRegVer2::PrintHeader(void)
{
	memset(DataN,0,DataNLen);
	DataN[0]=0x6C;

	return JustDoIt(1,RegSpeed*6);
}

int ElvisRegVer2::BCD2Int(char hexnum)
{
	char buffer[3];
	sprintf(buffer,"%02x",hexnum);
	return atoi(buffer);
}

char ElvisRegVer2::Int2BCD(int num)
{
	char buffer[3];
	char res;
	sprintf(buffer,"%02d",num%100);
	sscanf(buffer,"%02x",&res);
	return res;
}

int ElvisRegVer2::GetDateShiftRange(const char* password,DateShiftRange& range)
{
	SetMode(6,password);//try to unlock FR

	int er = SetMode(5,password);
	if (er!=0)
		return er;

	memset(DataN,0,DataNLen);
	DataN[0]=0x63;

	er = JustDoIt(1,1000);
	if (er!=0)
		return er;

	range.FirstDate.tm_mday=BCD2Int(DataN[2]);
	range.FirstDate.tm_mon=BCD2Int(DataN[3]);
	range.FirstDate.tm_year=BCD2Int(DataN[4]);

	range.LastDate.tm_mday=BCD2Int(DataN[5]);
	range.LastDate.tm_mon=BCD2Int(DataN[6]);
	range.LastDate.tm_year=BCD2Int(DataN[7]);

	range.FirstShift=BCD2Int(DataN[8])*100+BCD2Int(DataN[9]);
	range.LastShift=BCD2Int(DataN[10])*100+BCD2Int(DataN[11]);

	return SetMode(0,"");
}

int ElvisRegVer2::DateRangeReport(const char* password,DateShiftRange range,bool type)
{
	SetMode(6,password);//try to unlock FR

	int er = SetMode(5,password);
	if (er!=0)
		return er;

	memset(DataN,0,DataNLen);
	DataN[0]=0x65;
	if (type)
		DataN[1]=1;

	DataN[2]=Int2BCD(range.FirstDate.tm_mday);
	DataN[3]=Int2BCD(range.FirstDate.tm_mon);
	DataN[4]=Int2BCD(range.FirstDate.tm_year);
	DataN[5]=Int2BCD(range.LastDate.tm_mday);
	DataN[6]=Int2BCD(range.LastDate.tm_mon);
	DataN[7]=Int2BCD(range.LastDate.tm_year);

	er = JustDoIt(8,1000);
	if (er!=0)
		return er;

	char mode,submode,paper,printer;

	do
	{
		WaitSomeTime(1000);
		GetMode(&mode,&submode,&paper,&printer);
	}
	while((mode==5)&&(submode==2));

	if ((mode==5)&&(submode==0))
	{
		if (paper==1)
			return 103;
		else
			if (printer==1)
				return -2;
	}
	else
		return -2;

	return SetMode(0,"");
}

int ElvisRegVer2::ShiftRangeReport(const char* password,DateShiftRange range,bool type)
{
	SetMode(6,password);//try to unlock FR

	int er = SetMode(5,password);
	if (er!=0)
		return er;

	memset(DataN,0,DataNLen);
	DataN[0]=0x66;
	if (type)
		DataN[1]=1;

	DataN[2]=Int2BCD(range.FirstShift/100);
	DataN[3]=Int2BCD(range.FirstShift%100);
	DataN[4]=Int2BCD(range.LastShift/100);
	DataN[5]=Int2BCD(range.LastShift%100);

	er = JustDoIt(6,1000);
	if (er!=0)
		return er;

	char mode,submode,paper,printer;

	do
	{
		WaitSomeTime(1000);
		GetMode(&mode,&submode,&paper,&printer);
	}
	while((mode==5)&&(submode==2));

	if ((mode==5)&&(submode==0))
	{
		if (paper==1)
			return 103;
		else
			if (printer==1)
				return -2;
	}
	else
		return -2;

	return SetMode(0,"");
}

int ElvisRegVer2::SetTable(int table,int row,int field,void* val,int len,bool asStr)
{
	int er = SetMode(4,RegPassword.c_str());//try to change serial port's speed
	if (er!=0)
		return er;
	memset(DataN,0,DataNLen);
	DataN[0]=0x50;
	DataN[1]=table%0xFF;
	DataN[2]=0;
	DataN[3]=row&0xFF;
	DataN[4]=field%0xFF;

	memcpy(DataN+5,val,len);

	if (asStr)
	{
		extern unsigned char win2alt[];
		for (int i=0;i<LineLength;i++)
			DataN[5+i]=win2alt[DataN[5+i]];
	}

	return JustDoIt(5+len,t[5]);
}

int ElvisRegVer2::GetTable(int table,int row,int field,void* val,int len,bool asStr)
{
	int er = SetMode(4,RegPassword.c_str());//try to change serial port's speed
	if (er!=0)
		return er;
	memset(DataN,0,DataNLen);
	DataN[0]=0x46;
	DataN[1]=table%0xFF;
	DataN[2]=0;
	DataN[3]=row&0xFF;
	DataN[4]=field%0xFF;
	er = JustDoIt(5,t[5]);
	if (er!=0)
		return er;

	RemoveDLE();

	const char *CodesTable="АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ !\"#№%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	if (asStr)
		for (int i=0;i<LineLength;i++)
			if (DataN[2+i]<strlen(CodesTable))
				DataN[2+i]=CodesTable[DataN[2+i]];

	memcpy(val,DataN+2,len);
	return 0;
}

void ElvisRegVer2::InsertSales(vector<SalesRecord> data)
{
	if (EntireCheck)
		Sales=data;
}


void ElvisRegVer2::InsertPayment(vector<PaymentInfo> data)
{
//	if ((EntireCheck)&&(Payment.empty())&&(!StartClosing))
//		Payment=data;
}

unsigned long ElvisRegVer2::SerialNumber(void)
{
	if(GetStatus())
		return 0;
	return *(unsigned long *)(DataN + 10);//0;//
}

int ElvisRegVer2::CutCheck()
{
	memset(DataN,0,DataNLen);
	DataN[0] = 0x75;
	DataN[1] = 1;

	return JustDoIt(2,t[5]);
}

int ElvisRegVer2::GetSmenaNumber(unsigned long *KKMNo,unsigned short *smena,time_t * KKMTime)
{
/*
	int er = GetMode();
	if (er!=0)
		return er;
	short mask = 0x01;

	*smena=*(unsigned short *)(DataN + 36);
*/
	return 0;
}
