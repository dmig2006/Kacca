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
#include <qwidget.h>
#include <qmessagebox.h>
#include <qdir.h>
#include <qfile.h>
#include <qmutex.h>
#include <qsemimodal.h>
#include <xbase/xbase.h>

#include "Utils.h"

#include "Cash.h"

//FILE *state, *keepstate;

extern CashRegister* CashReg;

//names of dbf-tables
const char
		*GOODS="goods.dbf",
		*GOODSPRICE="goodsprice.dbf",
		*NONSENT="nonsent.dbf",
		*STAFF="staff.dbf",
		*SETTINGS="cashset.dbf",
		*DISCDESC="discount.dbf",
		*BANKDISC="bankdisc.dbf",
		*CUSTOMERS="customer.dbf",
		*CUSTRANGE="custrange.dbf",
		*NEWGOODS="newgoods.dbf",
		*KEYBOARD="keyboard.dbf",
		*GROUPS="groups.dbf",
		*BARCONF="barconf.dbf",
		*SALES="sale.dbf",
		*CURCHECK="curcheck.dbf",
		*ACTIONS="actions.dbf",
		*ACTREPORT = "actreport.dbf",
		*BANKACTIONS="bankactions.dbf",
		*BANKACTREPORT = "bankactreport.dbf",
		*CUSTSALES="custsales.dbf",
		*RETURN="return.dbf",
		*CRPAYMENT="crpayment.dbf",
		*PAYMENT="payment.dbf",
		*PAYMENTTOSEND="paymenttosend.dbf",
        *BANKBONUS="bankbonus.dbf",
        *BANKBONUSTOSEND="bankbonustosend.dbf",
        *BONUSFILE="bonus.dbf",
        *BONUSFILETOSEND="bonustosend.dbf",
        *ACTSIZFILE="actsiz.dbf",
        *ACTSIZFILETOSEND="actsiztosend.dbf",
		*CUSTVAL="custval.dbf", // сведения о накоплении
		*FLAG="ready",

//Connection messages
		*ServerError="Сервер недоступен! Сообщите администратору!",
		*XBaseError="Ошибка работы с DBF! Обратитесь к администратору!",

		*BankError="Ошибка связи с банком!",

		*ErrorLog="errors.log",*DateTemplate="DD.MM.YYYY",
//indices information
		*IndexFile[]={"code.ndx","price.ndx","barcode.ndx","grcode.ndx","name.ndx",},

#ifdef BUTTON_OLD_GOODS
        *indexFileOld[] = {"old_code.ndx","old_price.ndx","old_barcode.ndx","old_grcode.ndx","old_name.ndx",},
#endif

		*IndexField[]={"CODE"   ,"PRICE"    ,"BARCODE"    ,"GRCODE"    ,"NAME",};

int IndexNo=4;

//Tables description

xbSchema ConfigTableRecord[]=//main configuration's table
{
	{"DEPARTMENT",XB_NUMERIC_FLD,6,0},//store's number
	{"CASH_DESK",XB_NUMERIC_FLD,6,0},//cashdesk's number
	{"LAST_CHECK",XB_NUMERIC_FLD,10,0},//last check's number
	{"LAST_MAN",XB_CHAR_FLD,25,0},//last cashman's name
	{"MAIN_PATH",XB_CHAR_FLD,64,0},//main path of the program
	{"GOODS_PATH",XB_CHAR_FLD,64,0},//path of the new goods
	{"SERVER",XB_CHAR_FLD,64,0},//name of the SQL database
	{"S_LOGIN",XB_CHAR_FLD,64,0},//server's login
	{"S_PASSWD",XB_CHAR_FLD,64,0},//server's password
	{"SCALE_WAIT",XB_NUMERIC_FLD,6,0},//timeout (in sec) for the waiting responce from the scales
	{"IB_WAIT",XB_NUMERIC_FLD,6,0},//timeout (in sec) between connections to the SQL database
	{"REGPASSWD",XB_CHAR_FLD,8,0},//password for the fiscal register
	{"MAXAMOUNT",XB_NUMERIC_FLD,6,0},//maximum number of the goods in one sale
	{"MAXSUM",XB_NUMERIC_FLD,6,0},//maximum sum of the goods in one sale
	{"MAXSPACE",XB_NUMERIC_FLD,6,0},//maximum (in MB) disk space for the sale's archives
	{"RESOLUTION",XB_NUMERIC_FLD,1,0},//selected resolution (0=640x400,1=640x480)
	{"REGPORT",XB_NUMERIC_FLD,2,0},//serial port's number of the fiscal register
	{"SCANPORT",XB_NUMERIC_FLD,2,0},//serial port's number of the barcode's scanner
	{"SCALEPORT",XB_NUMERIC_FLD,2,0},//serial port's number of the scales
	{"DISPLPORT",XB_NUMERIC_FLD,2,0},//serial port's number of the customer's display
	{"CARDPORT",XB_NUMERIC_FLD,2,0},//serial port's number of the plastic card's reader
	{"REGBAUD",XB_NUMERIC_FLD,6,0},//serial port's speed of the fiscal register
	{"SCANBAUD",XB_NUMERIC_FLD,6,0},//serial port's speed of the barcode's scanner
	{"SCALEBAUD",XB_NUMERIC_FLD,6,0},//serial port's speed of the barcode's scales
	{"DISPLBAUD",XB_NUMERIC_FLD,6,0},//serial port's speed of the customer's display
	{"CARDBAUD",XB_NUMERIC_FLD,6,0},//serial port's speed of the plastic card's reader
	{"REGTYPE",XB_NUMERIC_FLD,2,0},//type of the fiscal register
	{"SCANTYPE",XB_NUMERIC_FLD,2,0},//type of the barcode's scanner
	{"SCALETYPE",XB_NUMERIC_FLD,2,0},//type of the scales
	{"DISPLTYPE",XB_NUMERIC_FLD,2,0},//type of the customer's display
	{"CARDTYPE",XB_NUMERIC_FLD,2,0},//type of the plastic card's reader
	{"CASHLESS",XB_LOGICAL_FLD,1,0},//признак безналичного расчета
	{"MSGLEVEL",XB_LOGICAL_FLD,1,0},//признак включения уровня сообщения
	{"CHECKOPEN",XB_LOGICAL_FLD,1,0},//признак открытого чека
	{"VEMSRV",XB_CHAR_FLD,32,0},// сервер видеонаблюдения
	{"VEMPORT",XB_NUMERIC_FLD,6,0},// порт видеонаблюдения
	{"S_ROLE",XB_CHAR_FLD,64,0},//server's role
	{"LAST_SMENA",XB_NUMERIC_FLD,10,0},//last smena
//	{"CHECKBANK",XB_LOGICAL_FLD,1,0},//проверять соединение с банком перед операцией безналичного расчета
//	{"CNT_TESTBANK",XB_NUMERIC_FLD,2,0},//кол-во попыток проверки связи с банком
	{"TESTBANK",XB_NUMERIC_FLD,2,0},//кол-во попыток проверки связи с банком
	{"CHECKNAME",XB_CHAR_FLD,32,0},//наименование документа в чеке ("Чек")
	{"PIC_PATH",XB_CHAR_FLD,64,0},//путь к каталогу картинок скринсейвера
	{"ARCH_PATH",XB_CHAR_FLD,64,0},//путь к архиву
	{"FRK_FONT",XB_LOGICAL_FLD,1,0},//печать 5м шрифтом
	{"SBPAYPASS",XB_LOGICAL_FLD,1,0},//paypass на терминалах сбербанка (другой формат команд)
	{"OFD",XB_LOGICAL_FLD,1,0},//режим работы ФР через ОФД
	{"",0,0,0}
};

xbSchema StaffTableRecord[]=//table of the staff description
{
	{"ID",XB_NUMERIC_FLD,6,0},//person's id
	{"NAME",XB_CHAR_FLD,40,0},//person's name
	{"MAN_LOGIN",XB_CHAR_FLD,10,0},//login
	{"SECURITY",XB_CHAR_FLD,20,0},//password
	{"ROLE_TYPE",XB_NUMERIC_FLD,6,0},//reserved
	{"RIGHTS",XB_CHAR_FLD,35,0},//if the n-th entry is . then this person have the corresponding right
	{"",0,0,0}
};

xbSchema SalesTableRecord[]=//table of the current sales
{
	{"CHECKNUM",XB_NUMERIC_FLD,10,0},//cheque's number
	{"CODE",XB_NUMERIC_FLD,10,0},//code of the goods
	{"QUANT",XB_NUMERIC_FLD,12,3},//quantity of the goods
	{"PRICE",XB_NUMERIC_FLD,15,2},//price of the goods
	{"FULLPRICE",XB_NUMERIC_FLD,15,2},//price of the goods without discount
	{"CALCPRICE",XB_NUMERIC_FLD,15,2},//price of the goods
	{"SUMMA",XB_NUMERIC_FLD,15,2},//price*quantity
	{"FULLSUMMA",XB_NUMERIC_FLD,15,2},//price*quantity without discount
	{"BANKBONUS",XB_NUMERIC_FLD,15,2},//bank bonus discount
	{"BANK",XB_NUMERIC_FLD,2,0},// flag card is sberbank card
	{"CALCSUMMA",XB_NUMERIC_FLD,15,2},//clacprice*quantity
	{"DISCOUNT",XB_NUMERIC_FLD,15,2},//discount's percent
	{"DSUM",XB_NUMERIC_FLD,15,2},//discount's sum
	{"OPERATION",XB_NUMERIC_FLD,2,0},//type of the operation (look at Utils.h for the description)
	{"PAYTYPE",XB_NUMERIC_FLD,4,0},//payment's type (cash,cashless etc.)
	{"DATE",XB_DATE_FLD,8,0},//sale's date
	{"TIME",XB_CHAR_FLD,8,0},//sale's time
	{"CASHMAN",XB_NUMERIC_FLD,6,0},//cashman's code
	{"CUSTOMER",XB_NUMERIC_FLD,18,0},//code of the customer with discount card
	{"INPUTKIND",XB_NUMERIC_FLD,3,0},// variant, how good add in the cheque
	{"KKMNO",XB_NUMERIC_FLD,10,0},//fiscal register serial number
	{"KKMSMENA",XB_NUMERIC_FLD,6,0},//last closed smena number
	{"KKMTIME",XB_NUMERIC_FLD,18,0},//fiscal register time
	{"NAME",XB_CHAR_FLD,100,0},//good`s name
	{"BARCODE",XB_CHAR_FLD,20,0},//good`s barcode
	{"FULLDSUMMA",XB_NUMERIC_FLD,15,2},//summa without certificate discount
	{"BANKDSUMMA",XB_NUMERIC_FLD,15,2},//summa without bank card discount
	{"ADDONCUST",XB_NUMERIC_FLD,18,0},//code of the customer with discount card
	{"RETCHECK",XB_NUMERIC_FLD,10,0},//cheque's number
	{"SBARCODE",XB_CHAR_FLD,20,0},//second goods barcode (employee`s info)
	{"GUARD",XB_NUMERIC_FLD,6,0},//cashman's code
	{"SECTION",XB_NUMERIC_FLD,6,0},//cashman's code
	{"MINPRICE",XB_NUMERIC_FLD,15,2},//price of the goods
	{"ALCO",XB_NUMERIC_FLD,6,0},// признак алкогольной продукции
	{"ACTSIZ",XB_CHAR_FLD,150,0},//штрихкод акцизной марки
	{"BONUSADD",XB_NUMERIC_FLD,15,2},//summa without certificate discount
	{"BONUSDISC",XB_NUMERIC_FLD,15,2},//summa without certificate discount
	{"BONUSFLAG",XB_NUMERIC_FLD,2,0},//summa without certificate discount
	{"NDS",XB_NUMERIC_FLD,18,2},//
	{"CIGAR",XB_NUMERIC_FLD,6,0},// признак алкогольной продукции
	{"AVANS",XB_NUMERIC_FLD,6,0},// признак продажи предоплаченных сертификатов (аванс)
	{"SUMMAPAY",XB_NUMERIC_FLD,15,2},//summa to pay
	{"ACTSIZTYPE",XB_NUMERIC_FLD,6,0},// признак алкогольной продукции
	{"",0,0,0}
};

xbSchema ActsizTableRecord[]=//table of the current sales
{
    {"CHECKNUM",XB_NUMERIC_FLD,10,0},//cheque's number
	{"CHECKNUM",XB_NUMERIC_FLD,10,0},//cheque's number
	{"DATE",XB_DATE_FLD,8,0},//sale's date
	{"TIME",XB_CHAR_FLD,8,0},//sale's time
	{"CODE",XB_NUMERIC_FLD,10,0},//code of the goods
	{"QUANT",XB_NUMERIC_FLD,12,3},//quantity of the goods (always = 1)
	{"PRICE",XB_NUMERIC_FLD,15,2},//price of the goods
	{"OPERATION",XB_NUMERIC_FLD,2,0},//type of the operation (look at Utils.h for the description)
	{"TRY",XB_NUMERIC_FLD,2,0}, //try of operation
	{"ACTSIZ",XB_CHAR_FLD,150,0},//штрихкод акцизной марки
	{"BARCODE",XB_CHAR_FLD,20,0},//good`s barcode
	{"NAME",XB_CHAR_FLD,100,0},//good`s name
	{"CASHMAN",XB_NUMERIC_FLD,6,0},//cashman's code
	{"RESCODE",XB_NUMERIC_FLD,2,0},//результат отражения в ЕГАИС
	{"RESULT",XB_CHAR_FLD,50,0},//Результат отражения в ЕГАИС
	{"",0,0,0}
};

xbSchema ReturnRecord[]=//table of the current sales
{
	{"CHECKNUM",XB_NUMERIC_FLD,10,0},//cheque's number
	{"CASHDESK",XB_NUMERIC_FLD,2,0},//cheque's number
	{"DATE",XB_DATE_FLD,8,0},//sale's date
	{"TIME",XB_CHAR_FLD,8,0},//sale's time
	{"OPERATION",XB_NUMERIC_FLD,2,0},//type of the operation (look at Utils.h for the description)
	{"KKM",XB_NUMERIC_FLD,8,0},//номер ККМ (для более точной идентификации чека)
	{"CHECK",XB_NUMERIC_FLD,8,0},//номер чека в ККМ
	{"SENT",XB_LOGICAL_FLD,1,0},//отравленно на сервер
	{"",0,0,0}
};

xbSchema GoodsTableRecord[]=//table of the goods description
{
	{"CODE",XB_NUMERIC_FLD,10,0},//code of the goods
	{"BARCODE",XB_CHAR_FLD,20,0},//barcode of the goods
	{"GRCODE",XB_NUMERIC_FLD,6,0},//group code of the goods
	{"NAME",XB_CHAR_FLD,100,0},//name of the goods
	{"PRICE",XB_NUMERIC_FLD,18,2},//price of the goods
	{"FIXEDPRICE",XB_NUMERIC_FLD,18,2},//fixed price of the goods
	{"GTYPE",XB_LOGICAL_FLD,1,0},//true - piece-goods, false - weighting goods
	{"SKIDKA",XB_CHAR_FLD,MAXDISCOUNTLEN,0},//if n-th position is . then this goods belongs
	{"NDS",XB_NUMERIC_FLD,18,2},//
	{"MULT",XB_NUMERIC_FLD,18,3},//
	//{"P0",XB_NUMERIC_FLD,18,2},//
	//{"Q0",XB_NUMERIC_FLD,18,2},//
	{"ALCO",XB_NUMERIC_FLD,5,0},//
	{"LOCK",XB_CHAR_FLD,MAXLOCKLEN,0},//if n-th position is . then this goods n-th lock-flag present
	{"MINPRICE",XB_NUMERIC_FLD,18,2},// minimum price. can`t sell this goods with price lower then minprice
	{"",0,0,0}
};

xbSchema GoodsPriceTableRecord[]=//table of the goods description
{
	{"CODE",XB_NUMERIC_FLD,10,0},//code of the goods
	{"BARCODE",XB_CHAR_FLD,20,0},//barcode of the goods
	{"Q0",XB_NUMERIC_FLD,18,3},//
	{"P0",XB_NUMERIC_FLD,18,2},//
	{"S0",XB_NUMERIC_FLD,18,2},//
	{"",0,0,0}
};

xbSchema BarInfoTableRecord[]=//table of barcode's settings
{
	{"CODENAME",XB_CHAR_FLD,20,0},//barcode's name
	{"CODEID",XB_CHAR_FLD,4,0},//barcode's prefix
	{"LEN",XB_NUMERIC_FLD,3,0},//length of the prefix
	{"",0,0,0}
};

xbSchema KeyboardTableRecord[]=//table of keyboard's settings
{
	{"NAME",XB_CHAR_FLD,40,0},//name of the key
	{"KEYCODE",XB_NUMERIC_FLD,6,0},//unicode of this key
	{"",0,0,0}
};

xbSchema GroupsTableRecord[]=//table of section's description
{
	{"GRCODE",XB_NUMERIC_FLD,7,0},//group's code
	{"S_NAME",XB_CHAR_FLD,80,0},//group's name
	{"",0,0,0}
};

xbSchema ActionsTableRecord[]=//table of the critical actions
{
	{"CHECKNUM",XB_NUMERIC_FLD,10,0},//cheque's number
	{"ACT_DATE",XB_DATE_FLD,8,0},//action's date
	{"ACT_TIME",XB_CHAR_FLD,8,0},//action's time
	{"ACT_TYPE",XB_NUMERIC_FLD,2,0},//action's type (look at Utils.h for the description)
	{"ACT_SUM",XB_NUMERIC_FLD,12,2},//action's sum
	{"SALEMAN_ID",XB_NUMERIC_FLD,6,0},//saleman's code
	{"GUARD_ID",XB_NUMERIC_FLD,11,0},//guard's code
	{"ACODE",XB_CHAR_FLD,20,0},//код авторизации
	{"BTERM",XB_CHAR_FLD,10,0},//номер терминала
	{"CARD",XB_CHAR_FLD,20,0},//карта
	{"PAYBACK",XB_NUMERIC_FLD,12,2},//сдача
	{"INFO",XB_CHAR_FLD,250,0},//информация
	{"BANK_ID",XB_NUMERIC_FLD,2,0},// признак банка 1-Сбербанк, 0 - прочие
	{"",0,0,0}
};


xbSchema CustomersTableRecord[]=//table of customers with discount catds
{
	{"ID",XB_NUMERIC_FLD,18,0},//customer's code
	{"DISCOUNTS",XB_CHAR_FLD,MAXDISCOUNTLEN,0},//type of the customer's discount
	{"ISSUE",XB_DATE_FLD,8,0},//issue date of the card
	{"EXPIRE",XB_DATE_FLD,8,0},//expiration's date of the card
	{"INFO",XB_CHAR_FLD,100,0},//информация
	{"NEEDCERT",XB_NUMERIC_FLD,1,0},// флаг проверки на выдачу сертификатов вкл./выкл.
	{"",0,0,0}
};

//
xbSchema CustRangeTableRecord[]=//table of customers with discount catds
{
	{"IDFIRST",XB_NUMERIC_FLD,18,0},//customer'scode first in range Int64
	{"IDLAST",XB_NUMERIC_FLD,18,0},//customer's code last in range Int64
	{"DISCOUNTS",XB_CHAR_FLD,MAXDISCOUNTLEN,0},//type of the customer's discount
	{"ISSUE",XB_DATE_FLD,8,0},//issue date of the card
	{"EXPIRE",XB_DATE_FLD,8,0},//expiration's date of the card
    {"CUSTTYPE",XB_NUMERIC_FLD,5,0},//тип карт 10-Сертификат, 20-Флаер на доп. скидку, 1-ВИП, 2 -Серебряная,3 -золотая, 0-прочии карты
    {"SUMMA",XB_NUMERIC_FLD,15,2},//Номинал сертификата
    {"BONUSCARD",XB_NUMERIC_FLD,5,0},//новый тип карт - Бонусный диапазон карт
    {"PARTYPE",XB_NUMERIC_FLD,5,0},//тип карт партнеров 3 - соотвествует 2% карте, 2  - 5%, 1 - ВИП
	{"",0,0,0}
};

xbSchema BankDiscountRecord[]=//table of customers with discount catds
{
    {"BANKID",XB_NUMERIC_FLD,10,0},//bankcard prefix begin
    {"BANKIDEND",XB_NUMERIC_FLD,10,0},//bankcard prefix end
	{"DISCOUNTS",XB_CHAR_FLD,MAXDISCOUNTLEN,0},//type of the customer's discount
	{"IDCUST",XB_NUMERIC_FLD,8,0},//customer's code
	{"",0,0,0}
};

xbSchema CustomersSalesRecord[]=//table of the sales with discount
{
	{"IDCUST",XB_NUMERIC_FLD,18,0},//customer's code
	{"DATESALE",XB_DATE_FLD,8,0},//date of this sale
	{"TIMESALE",XB_CHAR_FLD,8,0},//time of last sale
	{"CASHDESK",XB_NUMERIC_FLD,6,0},//cashdesk's number
	{"CHECKNUM",XB_NUMERIC_FLD,10,0},//check's number
	{"SUMMA",XB_NUMERIC_FLD,15,2},//sum of this sale
	{"DISCSUMMA",XB_NUMERIC_FLD,15,2},//discount's sum of this sale
	{"",0,0,0}
};

xbSchema CustomersValueRecord[]=//table of the sales with discount
{
	{"IDCUST",XB_NUMERIC_FLD,18,0},//customer's code
	{"DATESALE",XB_DATE_FLD,8,0},//date of last sale
	{"TIMESALE",XB_CHAR_FLD,8,0},//time of last sale
	{"COUNTSALE",XB_NUMERIC_FLD,8,0},//total count of sale
	{"SUMMA",XB_NUMERIC_FLD,15,2},//sum
	{"DISCSUMMA",XB_NUMERIC_FLD,15,2},//discount's sum
	{"LASTSUMMA",XB_NUMERIC_FLD,15,2},//last summ
	{"",0,0,0}
};


xbSchema DiscountsRecord[]=//table of the dicount's description
{
	{"DISCTYPE",XB_NUMERIC_FLD,6,0},	//type of the discount
	{"DESCRIPT",XB_CHAR_FLD,100,0}, 	//(05.02.2008)
	{"PERCENT",XB_NUMERIC_FLD,6,2},		//percent of the discount
	{"CHARGE",XB_NUMERIC_FLD,8,2},		//sum of the discount
	{"FPERCENT",XB_CHAR_FLD,100,0},		//percents of the discount, it's string like 100:2;150:3; etc
	{"FCHARGE",XB_CHAR_FLD,100,0},		//sum of the discount, it's string like 100:5;150:10; etc
	{"STARTTIME",XB_CHAR_FLD,8,0},		//period of time while discount is valid
	{"FINISHTIME",XB_CHAR_FLD,8,0},		//
	{"STARTDATE",XB_DATE_FLD,8,0},		//
	{"FINISHDATE",XB_DATE_FLD,8,0},		//
	{"DAYOFWEEK",XB_NUMERIC_FLD,1,0,},	//day of week when discount is valid
	{"ALLGOODS",XB_NUMERIC_FLD,1,0,},	//if ==1 then the schema is active for all goods
	{"BYPRICE",XB_NUMERIC_FLD,1,0,},	//if ==1 then one will be used the fixed price of the goods
	{"MODE",XB_NUMERIC_FLD,1,0,},		//if ==0 для товара в чеке ==1 суммы CHARGE и FCHARGE влияют на сумму товара а не на цену ==2 на весь чек влияет CHARGE и FCHARGE тогда это скидка не на цену товара а на весь чек
	{"",0,0,0}
};

xbSchema MsgLevelRecord[]=//table of the dicount's description
{
    {"MODE",XB_NUMERIC_FLD,8,0},
	{"LEVEL",XB_NUMERIC_FLD,10,2},
	{"MSG",XB_CHAR_FLD,200,0},
	{"STARTDATE",XB_DATE_FLD,8,0},//
	{"FINISHDATE",XB_DATE_FLD,8,0},//
    {"CUSTTYPE",XB_NUMERIC_FLD,5,0},//тип карт 10-Сертификат, 20-Флаер на доп. скидку, 1-ВИП, 2 -Серебряная,3 -золотая, 0-прочии карты
	{"",0,0,0}
};

xbSchema PaymentTableRecord[]=//
{
	{"TYPE",XB_NUMERIC_FLD,2,0},
	{"NUMBER",XB_CHAR_FLD,32,0},
	{"SUMMA",XB_NUMERIC_FLD,12,2},
	{"CHECKNUM",XB_NUMERIC_FLD,10,0},//cheque's number
	{"DATE",XB_DATE_FLD,8,0},//sale's date
	{"TIME",XB_CHAR_FLD,8,0},//sale's time
	{"CASHMAN",XB_NUMERIC_FLD,6,0},//cashman's code
	{"CUSTOMER",XB_NUMERIC_FLD,18,0},//code of the customer with discount card
	{"INPUTKIND",XB_NUMERIC_FLD,3,0},//code of the customer with discount card
	{"KKMNO",XB_NUMERIC_FLD,10,0},//fiscal register serial number
	{"KKMSMENA",XB_NUMERIC_FLD,6,0},//last closed smena number
	{"KKMTIME",XB_NUMERIC_FLD,18,0},//fiscal register time
//	{"TRANID",XB_CHAR_FLD,30,0},//
//	{"TRANDT",XB_CHAR_FLD,30,0},//
	{"",0,0,0}
};

xbSchema BankBonusTableRecord[]=//
{
	{"ACTION",XB_NUMERIC_FLD,3,0},
	{"CASHDESK",XB_NUMERIC_FLD,10,0},//cheque's number
	{"CHECKNUM",XB_NUMERIC_FLD,10,0},//cheque's number
	{"CARD",XB_CHAR_FLD,32,0},
	{"PAN4",XB_CHAR_FLD,32,0},
	{"CARD_STAT",XB_CHAR_FLD,3,0},
	{"TRAN_SUMMA",XB_NUMERIC_FLD,15,2},
	{"SUMMA",XB_NUMERIC_FLD,15,2},
	{"ACTIVE_BNS",XB_NUMERIC_FLD,15,2},
	{"CHANGE_BNS",XB_NUMERIC_FLD,15,2},
	{"DATE",XB_DATE_FLD,8,0},//sale's date
	{"TIME",XB_CHAR_FLD,8,0},//sale's time
	{"CASHMAN",XB_NUMERIC_FLD,6,0},//cashman's code
	{"GUARD",XB_NUMERIC_FLD,6,0},//cashman's code
	{"CUSTOMER",XB_NUMERIC_FLD,18,0},//code of the customer with discount card
	{"MODE",XB_NUMERIC_FLD,3,0},//code of the customer with discount card
	{"RETCODE",XB_NUMERIC_FLD,10,0},//code of the customer with discount card
	{"TRAN_ID",XB_CHAR_FLD,40,0},//
	{"TRAN_DT",XB_CHAR_FLD,14,0},//
	{"SHA1",XB_CHAR_FLD,40,0},//
	{"GUID",XB_CHAR_FLD,40,0},//
	{"RETCHECK",XB_NUMERIC_FLD,10,0},//cheque's number
	{"RETTRAN_ID",XB_CHAR_FLD,40,0},//
	{"TERMINAL",XB_CHAR_FLD,20,0},//id терминала
	{"INFO",XB_CHAR_FLD,250,0},//
	{"LIBVER",XB_CHAR_FLD,25,0},//
	{"",0,0,0}
};


xbSchema BonusTableRecord[]=//
{
	{"ACTION",XB_NUMERIC_FLD,3,0},
	{"CASHDESK",XB_NUMERIC_FLD,10,0},//cheque's number
	{"CHECKNUM",XB_NUMERIC_FLD,10,0},//cheque's number
	{"CARD",XB_CHAR_FLD,32,0},
	{"OPER",XB_CHAR_FLD,30,0},
	{"PAN4",XB_CHAR_FLD,32,0},
	{"CARD_STAT",XB_CHAR_FLD,3,0},
	{"ACTIVE_BNS",XB_NUMERIC_FLD,15,2},
	{"HOLD_BNS",XB_NUMERIC_FLD,15,2},
	{"SUMMA_ADD",XB_NUMERIC_FLD,15,2},
	{"SUMMA_REM",XB_NUMERIC_FLD,15,2},
	{"SUMMA_CHG",XB_NUMERIC_FLD,15,2},
	{"DATE",XB_DATE_FLD,8,0},//sale's date
	{"TIME",XB_CHAR_FLD,8,0},//sale's time
	{"CASHMAN",XB_NUMERIC_FLD,6,0},//cashman's code
	{"GUARD",XB_NUMERIC_FLD,6,0},//cashman's code
	{"CUSTOMER",XB_NUMERIC_FLD,18,0},//code of the customer with discount card
	{"MODE",XB_NUMERIC_FLD,3,0},//code of the customer with discount card
	{"RETCODE",XB_NUMERIC_FLD,10,0},//code of the customer with discount card
	{"ERRORCODE",XB_NUMERIC_FLD,10,0},//code of the customer with discount card
	{"TRAN_ID",XB_CHAR_FLD,40,0},//
	//{"TRAN_DT",XB_CHAR_FLD,14,0},//
	//{"SHA1",XB_CHAR_FLD,40,0},//
	{"GUID",XB_CHAR_FLD,40,0},//
	{"RETCHECK",XB_NUMERIC_FLD,10,0},//cheque's number
	{"RETTRAN_ID",XB_CHAR_FLD,40,0},//
	{"TERMINAL",XB_CHAR_FLD,20,0},//id терминала
	{"INFO",XB_CHAR_FLD,250,0},//
	//{"LIBVER",XB_CHAR_FLD,25,0},//
	{"OLDCARD",XB_NUMERIC_FLD,18,0},//Замена карты (старая карта)
	{"NEWCARD",XB_NUMERIC_FLD,18,0},//Замена карты (новая карта)
	{"",0,0,0}
};


xbSchema BankActionsTableRecord[]=//table of the bank actions
{
	{"CHECKNUM",XB_NUMERIC_FLD,10,0},//cheque's number
	{"DATESALE",XB_DATE_FLD,8,0},//action's date
	{"TIMESALE",XB_CHAR_FLD,8,0},//action's time
	{"ACTION",XB_NUMERIC_FLD,2,0},//action's type 0- продажа, 1 - возврат, 7 - сверка с банком (закрытие смены)
	{"BANK",XB_NUMERIC_FLD,2,0},// признак банка 1 - Сбербанк, 2 - ВТБ, 3 - Газпром, 0 - неопределено
    {"TERMINAL",XB_CHAR_FLD,20,0},//номер терминала
	{"IDREPORT",XB_CHAR_FLD,40,0},// GUID
	{"LINE",XB_NUMERIC_FLD,10,0},//номер строки
	{"TEXT",XB_CHAR_FLD,250,0},//информация
	{"SALEMAN_ID",XB_NUMERIC_FLD,6,0},//saleman's code
	{"",0,0,0}
};


//this convertion table is needed by internal keyboard switcher
unsigned short RecodeTable[RecodeTableSize][2]={
	{'q',1081}, {'w',1094}, {'e',1091}, {'r',1082}, {'t',1077}, {'y',1085},
	{'u',1075}, {'i',1096}, {'o',1097}, {'p',1079}, {'[',1093}, {']',1098},
	{'a',1092}, {'s',1099}, {'d',1074}, {'f',1072}, {'g',1087}, {'h',1088},
	{'j',1086}, {'k',1083}, {'l',1076}, {';',1078}, {'\'',1101}, {'z',1103},
	{'x',1095}, {'c',1089}, {'v',1084}, {'b',1080}, {'n',1090}, {'m',1100},
	{',',1073}, {'.',1102}, {'/',46  }, {'Q',1049}, {'W',1062}, {'E',1059},
	{'R',1050}, {'T',1045}, {'Y',1053}, {'U',1043}, {'I',1064}, {'O',1065},
	{'P',1047}, {'{',1061}, {'}',1066}, {'A',1060}, {'S',1067}, {'D',1042},
	{'F',1040}, {'G',1055}, {'H',1056}, {'J',1054}, {'K',1051}, {'L',1044},
	{':',1046}, {'"',1069}, {'Z',1071}, {'X',1063}, {'C',1057}, {'V',1052},
	{'B',1048}, {'N',1058}, {'M',1068}, {'<',1041}, {'>',1070}, {'?',44  },};

unsigned char win2alt[]= {
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
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xf0,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xf1,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef };


void setLineWidthDefault(int cnt) {
    LineWidthDefault = cnt;
}

int getLineWidthDefault() {
    return LineWidthDefault;
}

double RoundToNearest(double num,double prec)//round given number up to specified precision
{                                            //prec has form 0.001 etc.
	double rest=num/prec-floor(num/prec);
	
	if (fabs(rest-0.5)<1e-6) //if 'rest' is very close to 1/2 we get the smallest number not less
		return ceil(num/prec)*prec;//than 'num/prec'

	if (rest<0.5)
		return floor(num/prec)*prec;
	else
		return ceil(num/prec)*prec;
}

string GetRounded(double num,double prec)//as above but function return formated string
{
	if(prec < .001)
		prec = .001;
	string str=Double2Str(RoundToNearest(num,prec));
	unsigned int i;

	for (i=0;i<str.length();i++)//replace dot symbol by comma
		if (str[i]=='.')
			str[i]=',';

	if (str.find(',')==str.npos)//append/remove comma and zeros according to the given precision
		str+=',';

	for (i=0;i<-log10(prec);i++)
		str+='0';

	str=str.substr(0,str.find(',')+Double2Int(log10(1/prec))+1);

	if (prec==1)  //if number is integer we have to remove last comma symbol
		str.erase(str.find(','));

	return str;
}

string Double2Str(double num)//convert double number to the string
{
	char buffer[64];
	sprintf(buffer,"%f",num);
	return buffer;
}

int Double2Int(double num)//cut off float part of number
{
	char buffer[64];
	sprintf(buffer,"%.0f",num);
	return atoi(buffer);
}

string Int2Str(int number)//convert integer number to the string
{
	char buffer[64];
	sprintf(buffer,"%d",number);
	return buffer;
}

string LLong2Str(long long number)//convert integer number to the string
{
	char buffer[64];
	sprintf(buffer,"%lld",number);
	return buffer;
}

string Card2Str(long long number)//convert integer number to the string
{
    string strCardNumber = LLong2Str(number);
    if (strCardNumber.length() > 5)
    {
        strCardNumber = strCardNumber.substr(5, 7);
    }
	return strCardNumber;
}

double Str2Double(string number)//convert string to the double number
{
	for (unsigned int i=0;i<number.length();i++)
		if (number[i]==',')
			number[i]='.';

	return atof(number.c_str());
}

long Str2Int(string number)//convert string to the int number
{
	return atol(number.c_str());
}

string TruncZeros(string number)
{
	unsigned int i;  //format number (remove non-necessary zeros)

	if ((number.find(',')==number.npos)&&(number.find('.')==number.npos))//append comma if necessary
		number+=',';

	for(i=0;i<number.length();i++)
		if (number[i]!='0')
			break;

	number=number.substr(i);

	for(i=number.length()-1;i>=0;i--)
		if (number[i]!='0')
			break;

	number=number.substr(0,i+1);

	if ((number.length()>0)&&((number[0]==',')||(number[0]=='.')))
		number='0'+number;

	if (number.empty())
		number='0';

	return number;
}

QString W2U(string str)//convert windows coding to the Unicode
{
	QString result="";
	for (unsigned int i=0;i<str.size();i++)
	{        //both scheme contains russian letter in order a-ya A-YA
		QChar ch;//but windows's offset is 0xC0 and Unicode offset is 0x410
		unsigned char b2c=str[i];//for each scheme symbols in the area 0x00-0x80 coincide
		if ((b2c>=0xC0)&&(b2c<=0xFF))
			ch=0x0410+b2c-0xC0;
		if (b2c<=0x80)
			ch=b2c;
		result+=ch;
	}

	return result;
}

#include <iconv.h>
QString KOI82U(string str)//convert windows coding to the Unicode
{
	iconv_t cd;

	cd = iconv_open("CP1251","KOI8-R");


	char *inbuf;
	char *outbuf;
	char *inbuf2,*outbuf2;
	size_t len1,len2;

	inbuf  = new(char [str.size()+1]);
	outbuf = new(char [(str.size()+1)*2]);

	inbuf2 =inbuf;
	outbuf2=outbuf;

	strcpy(inbuf,str.c_str());
	len1 = strlen(inbuf)+1;
	len2 = len1*2;

	iconv(cd,(char **)(&inbuf2),&len1,&outbuf2,&len2);

	QString result="";
	result=W2U(outbuf);

	iconv_close(cd);

	return result;
}

string U2W(QString str)//convert cyrillic part of Unicode to the the cp1251
{
	string result="";
	for (unsigned int i=0;i<str.length();i++)
	{
		unsigned short ch=str.at(i).unicode();
		if (ch<=0x80)
		{
			result+=ch%256;//add latin letters and various symbols
			continue;
		}

		if ((ch>=0x0400)&&(ch<=0x04FF))
		{
			result+=(ch-0x0410+0xC0)%256;//add cyrillic letters
			continue;
		}

		result+="?";//can not convert
	}

	return result;
}

void ShowMessage(QWidget* parent,string str)//auxililary functions
{
	if(CashReg) CashReg->m_vem.Msg(str.c_str());
	{
		char s[256];
		sprintf(s, "ShowMessage(%s)", str.c_str());
		Log(s);
	}
	QMessageBox::warning(parent,"APM KACCA",W2U(str),"OK","","",0);
}


bool ShowQuestion(QWidget* parent,string str,bool IsYesDefault)
{
	if(CashReg) CashReg->m_vem.Msg(str.c_str());
	int DefaultAnswer;
	if (IsYesDefault)
		DefaultAnswer=0;
	else
		DefaultAnswer=1;
	bool rez = (QMessageBox::warning(parent,"APM KACCA",W2U(str),W2U("Да"),W2U("Нет"),"",DefaultAnswer)==0);
	{
		char s[256];
		sprintf(s, "ShowQuestion(%s, %c)", str.c_str(), (rez?'Y':'N'));
		Log(s);
	}
	if(CashReg) CashReg->m_vem.Msg(rez?"Y":"N");
	return rez;
/*
	if (QMessageBox::warning(parent,"Kacca",W2U(str),W2U("Да"),W2U("Нет"),"",DefaultAnswer)==0)
		return true;
	else
		return false;
*/
}

void Beep(void)
{
#ifdef _WS_WIN_
	Beep(1500,100);
#else
	int fd=open("/dev/console",O_RDWR);
	char bp='\a';
	if (fd>=0)
	{
		write(fd,&bp,1);
		close(fd);
	}
#endif
}

port_handle OpenPort/*Scan*/(int port,int speed,int parity,int timeout)
{//open serial port with given number,speed,byte parity and timeout to wait for response (in ms)
#ifdef _WS_WIN_
	HANDLE fd=CreateFile(("COM"+Int2Str(port)).c_str(),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);

	DCB dcb;
	COMMTIMEOUTS timeouts;

	if (fd == INVALID_HANDLE_VALUE)
		return 0;

	GetCommState(fd, &dcb);

	switch(speed)
	{
		case   1200:dcb.BaudRate=CBR_1200;break;
		case   2400:dcb.BaudRate=CBR_2400;break;
		case   4800:dcb.BaudRate=CBR_4800;break;
		case   9600:dcb.BaudRate=CBR_9600;break;
		case  19200:dcb.BaudRate=CBR_19200;break;
		case  38400:dcb.BaudRate=CBR_38400;break;
		case  57600:dcb.BaudRate=CBR_57600;break;
		case 115200:dcb.BaudRate=CBR_115200;break;
		default:	dcb.BaudRate=0;break;
	}

	dcb.ByteSize = 8;
	switch (parity)
	{
		case 0:dcb.Parity = NOPARITY;break;
		case 1:dcb.Parity = ODDPARITY;break;
		case 2:dcb.Parity = EVENPARITY;break;
		default:dcb.Parity = NOPARITY;break;
	}

	dcb.StopBits = ONESTOPBIT;

	SetCommState(fd, &dcb);

	memset(&timeouts,0,sizeof(timeouts));
	SetCommTimeouts(fd,&timeouts);

	timeouts.ReadIntervalTimeout=MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier=MAXDWORD;
	timeouts.ReadTotalTimeoutConstant=timeout;

	SetCommTimeouts(fd,&timeouts);
#else
	//int fd=open(("/dev/ttyS"+Int2Str(port-1)).c_str(),O_RDWR | O_NOCTTY | O_NONBLOCK );
string s_port = ("/dev/ttyS"+Int2Str(port-1));
;;Log("PORT:"+s_port);
//	int fd=open(("/dev/ttyS"+Int2Str(port-1)).c_str(),O_RDWR | O_NOCTTY | O_NONBLOCK );
	int fd=open(s_port.c_str(),O_RDWR | O_NOCTTY | O_NONBLOCK );

	if (fd<0)
	{
	    ;;Log("ERROR OPEN FR PORT:"+s_port);
	    ;;Log("ERROR CODE:"+Int2Str(fd));
		return 0;
	}
//*
	int fdflags = fcntl(fd, F_GETFL);
	if(fdflags == -1 || fcntl(fd, F_SETFL, fdflags & ~O_NONBLOCK) < 0){
		close(fd);
		puts("not open ");
		return 0;
	}
//*/
	struct termios p;
	tcgetattr(fd,&p);

	speed_t cur_speed;
	switch(speed)
	{
		case 1200:cur_speed=B1200;break;
		case 2400:cur_speed=B2400;break;
		case 4800:cur_speed=B4800;break;
		case 9600:cur_speed=B9600;break;
		case 19200:cur_speed=B19200;break;
		case 38400:cur_speed=B38400;break;
		case 57600:cur_speed=B57600;break;
		case 115200:cur_speed=B115200;break;
		default:cur_speed=B0;break;
	}

	cfsetispeed(&p,cur_speed);
	cfsetospeed(&p,cur_speed);

	p.c_cflag=(p.c_cflag & ~CSIZE) | CS8 | CLOCAL | CREAD;
	p.c_cflag &= ~(PARENB | PARODD);

	switch (parity)
	{
	    case 1:p.c_cflag |= (PARENB | PARODD);break;
	    case 2:p.c_cflag |= PARENB;break;
	}

	p.c_iflag = IGNBRK;
	p.c_lflag = 0;
	p.c_oflag = 0;
	p.c_cc[VMIN]= 0;
	p.c_cc[VTIME]= timeout/100;

	tcsetattr(fd,TCSANOW,&p);
#endif

	ClearPort(fd);
	return fd;
}

/*
port_handle OpenPort(int port,int speed,int parity,int timeout)
{//open serial port with given number,speed,byte parity and timeout to wait for response (in ms)
#ifdef _WS_WIN_
	HANDLE fd=CreateFile(("COM"+Int2Str(port)).c_str(),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);

	DCB dcb;
	COMMTIMEOUTS timeouts;

	if (fd == INVALID_HANDLE_VALUE)
		return 0;

	GetCommState(fd, &dcb);

	switch(speed)
	{
		case   1200:dcb.BaudRate=CBR_1200;break;
		case   2400:dcb.BaudRate=CBR_2400;break;
		case   4800:dcb.BaudRate=CBR_4800;break;
		case   9600:dcb.BaudRate=CBR_9600;break;
		case  19200:dcb.BaudRate=CBR_19200;break;
		case  38400:dcb.BaudRate=CBR_38400;break;
		case  57600:dcb.BaudRate=CBR_57600;break;
		case 115200:dcb.BaudRate=CBR_115200;break;
		default:	dcb.BaudRate=0;break;
	}

	dcb.ByteSize = 8;
	switch (parity)
	{
		case 0:dcb.Parity = NOPARITY;break;
		case 1:dcb.Parity = ODDPARITY;break;
		case 2:dcb.Parity = EVENPARITY;break;
		default:dcb.Parity = NOPARITY;break;
	}

	dcb.StopBits = ONESTOPBIT;

	SetCommState(fd, &dcb);

	memset(&timeouts,0,sizeof(timeouts));

	// как в драйвере
	timeouts.ReadIntervalTimeout = timeout;
	timeouts.ReadTotalTimeoutMultiplier = timeout;
	timeouts.ReadTotalTimeoutConstant = timeout;
	timeouts.WriteTotalTimeoutMultiplier = 50;
	timeouts.WriteTotalTimeoutConstant = 100;

	SetCommTimeouts(fd,&timeouts);
#else
//	puts("not open");
	int fd=open(("/dev/ttyS"+Int2Str(port-1)).c_str(),O_NONBLOCK | O_RDWR | O_NOCTTY);
	if (fd<0)
		return 0;

	int fdflags = fcntl(fd, F_GETFL);
	if(fdflags == -1 || fcntl(fd, F_SETFL, fdflags & ~O_NONBLOCK) < 0){
		close(fd);
		return 0;
	}

//	puts("open");

	struct termios p;
	tcgetattr(fd,&p);

	speed_t cur_speed;
	switch(speed)
	{
		case 1200:cur_speed=B1200;break;
		case 2400:cur_speed=B2400;break;
		case 4800:cur_speed=B4800;break;
		case 9600:cur_speed=B9600;break;
		case 19200:cur_speed=B19200;break;
		case 38400:cur_speed=B38400;break;
		case 57600:cur_speed=B57600;break;
		case 115200:cur_speed=B115200;break;
		default:cur_speed=B0;break;
	}

	cfsetispeed(&p,cur_speed);
	cfsetospeed(&p,cur_speed);

//	p.c_cflag |= CLOCAL | CREAD | CS8;// | (ICANON | ECHO | ECHOE);
//	p.c_cflag &= ~(CSIZE | PARENB | (ICANON | ECHO | ECHOE | ISIG));

//	p.c_cflag=(p.c_cflag & ~CSIZE) | CS8 | CLOCAL | CREAD;
//	p.c_cflag &= ~(PARENB | PARODD);

	p.c_cflag &= ~(CSIZE | PARENB | CLOCAL);//| CSTOPB
	p.c_cflag |= CS8 | CREAD | HUPCL;


	p.c_iflag = IGNBRK | IGNPAR;
	p.c_lflag = 0;
	p.c_oflag = 0;
	p.c_cc[VMIN]= 0;//1;//
	p.c_cc[VTIME]= timeout/100;

	tcsetattr(fd,TCSAFLUSH,&p);

//	puts("setattr");
#endif

	ClearPort(fd);
//	puts("clearport");
	return fd;
}
*/

bool ClosePort(port_handle port)//close serial port
{
#ifdef _WS_WIN_
	return CloseHandle(port);
#else
	if (close(port)==0)
		return true;
	else
		return false;
#endif
}

void ClearPort(port_handle port)//clear buffer of the serial port
{
#ifdef _WS_WIN_
	PurgeComm(port,PURGE_TXCLEAR|PURGE_RXCLEAR);
#else
	tcflush(port,TCIOFLUSH);
#endif
}

int WriteToPort(port_handle port,const void* buf,size_t size)
{//write buffer 'buf[size]' to the serial port and return number of the written bytes
#ifdef _WS_WIN_
	DWORD k;
	WriteFile(port,buf,size,&k,NULL);
	return k;
#else
	return write(port,buf,size);
#endif
}

int ReadFromPort(port_handle port,void* buf,size_t size)
{//read data from the serial port to buffer 'buf[size]' and return number of the read bytes
#ifdef _WS_WIN_
	DWORD k;
	ReadFile(port,buf,size,&k,NULL);
	return k;
#else
	int i;
	for(i = 0; i < size; i++){
		if(read(port, (char*)buf + i, 1) <= 0)
		{
			//Log((char *)buf);
			return i;
		}
	}
	return i;
/*
	return read(port,buf,size);
*/
#endif
}

void WaitSomeTime(int msec)//wait some time (in ms)
{
#ifdef _WS_WIN_
	Sleep(msec);
#else
	double curclock=clock();
	while((clock()-curclock)*1000/CLOCKS_PER_SEC<msec);
#endif
}

void CreateDir(string path)//create directory
{
	if (!path.empty())
	{
		if (path[path.size()-1]!=QDir::separator())
			path+=QDir::separator();
	}
	else
		return;

	for (unsigned int pos=1;pos<path.size();pos++)
		if (path[pos]==QDir::separator())
			QDir().mkdir(path.substr(0,pos).c_str());
}

bool MoveFile(QFile* fromFile,QFile* toFile)//move file
{
	remove((toFile->name() + "_t").ascii());
	rename((toFile->name()).ascii(),(toFile->name() + "_t").ascii());
	if(rename((fromFile->name()).ascii(),(toFile->name()).ascii())){
		rename((toFile->name() + "_t").ascii(),(toFile->name()).ascii());
		return false;
	}
	remove((toFile->name() + "_t").ascii());
	return true;;
}

bool MoveFileSys(string fromFile,string toFile)//move file
{
    int ret = system(("mv "+fromFile + " "+toFile).c_str());
	return ret;
}

bool Empty(string str)
{
    bool res=true;

    for(int i=0; res && (i<str.length());i++)
    {
        if(str[i]!=' ') res=false;
    }

    return res;
}

/*
{
	if (!fromFile->exists())
		return false;

	if (!fromFile->open(IO_ReadOnly))
		return false;
	if (!toFile->open(IO_WriteOnly))
		return false;

	for(;;)
	{
		char buffer[512];
		memset(buffer,0,512);
		unsigned int len=fromFile->readBlock(buffer,512);
		if (len==-1)
			return false;
		if (len==0)
			break;
		if (toFile->writeBlock(buffer,len)==-1)
			return false;
	};

	fromFile->close();
	toFile->close();

	if (toFile->size()!=fromFile->size())
		return false;

	if (fromFile->remove())
		return true;
	else
		return false;
}
*/

GoodsInfo * FiscalCheque::Insert(GoodsInfo g)//insert goods to the check
{
	cheque.insert(cheque.end(),g);
	return & cheque.back();
};

void FiscalCheque::Remove(void)
{
	cheque.erase(cheque.end());
};

void FiscalCheque::SetAt(unsigned int pos,GoodsInfo g)//insert goods into the given position
{
	cheque[pos]=g;
};

GoodsInfo FiscalCheque::operator[](unsigned int pos)//get goods from the given position
{
	return cheque[pos];
}

void FiscalCheque::Clear(void)//remove all goods in the check
{
    NumPrecheck=0;

	trunc = 0;
	tsum = 0;
	DiscountSumm = 0;
	DiscountSummNotAction = 0;
	CertificateSumm = 0;
	PrePayCertificateSumm = 0;
	CertificateCount = 0;
	BankBonusSumm = 0;
	CertificateString = "";
	BankBonusString = "";

	egais_url="";
	egais_sign="";

    BonusSumm=0;

	cheque.clear();
};

double FiscalCheque::GetSum(void)//get sum of all goods in the check
{
	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
		if ((cheque[i].ItemFlag==SL) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY))
			TotalSum+=cheque[i].ItemSum;
		if ((cheque[i].ItemFlag==RT) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY))
			TotalSum-=cheque[i].ItemSum;
	}
/*
	if(trunc > .01)
	{
		// округление
		tsum = TotalSum - trunc * floor((TotalSum + .005) / trunc);
		TotalSum -= tsum;
	}
	else tsum = 0;
*/

#ifndef BONUS_LOG_SKIDKA //TestProdag
    TotalSum = 100.80;
#endif

	return TotalSum;
};

double FiscalCheque::GetSumToPay(void)//get sum of all goods in the check
{
	double TotalSum = GetSum() - PrePayCertificateSumm;

    if (TotalSum < 0.01) {
        TotalSum = 0;
    }

	return TotalSum;
};

double FiscalCheque::GetSumBonusAdd(void)//get sum of all goods in the check
{
	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
		if ((cheque[i].ItemFlag==SL) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY))
			TotalSum+=cheque[i].ItemBonusAdd;
		if ((cheque[i].ItemFlag==RT) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY))
			TotalSum-=cheque[i].ItemBonusAdd;
	}

	return TotalSum;
};


double FiscalCheque::GetSumBonusDisc(void)//get sum of all goods in the check
{
	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
		if ((cheque[i].ItemFlag==SL) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY))
			TotalSum+=cheque[i].ItemBonusDisc;
		if ((cheque[i].ItemFlag==RT) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY))
			TotalSum-=cheque[i].ItemBonusDisc;
	}

	return TotalSum;
};


double FiscalCheque::GetSumNotAction(void)//get sum of all goods in the check
{
	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
		if ((cheque[i].ItemFlag==SL) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY) && (cheque[i].ItemName.substr(0,1)!="*"))
			TotalSum+=cheque[i].ItemSum;
		if ((cheque[i].ItemFlag==RT) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY) && (cheque[i].ItemName.substr(0,1)!="*"))
			TotalSum-=cheque[i].ItemSum;
	}

	return TotalSum;
};

bool FiscalCheque::GoodInActionByName(string str)
{
    bool res=false;

    if ( (str.substr(0,1)=="*") || (str.substr(0,1)=="@") ) {

        res = true;

    }

    return res;

}


double FiscalCheque::GetSumNotAction_New(void)//get sum of all goods in the check
{
	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
		if ((cheque[i].ItemFlag==SL)
		&& (cheque[i].ItemCode!=TRUNC)
        && (cheque[i].ItemCode!=EXCESS_PREPAY)
		&& (!GoodInActionByName(cheque[i].ItemName))
		)

			TotalSum+=cheque[i].ItemSum;

		if ((cheque[i].ItemFlag==RT)
		&& (cheque[i].ItemCode!=TRUNC)
		&& (cheque[i].ItemCode!=EXCESS_PREPAY)
		&& (!GoodInActionByName(cheque[i].ItemName))
		)

			TotalSum-=cheque[i].ItemSum;

	}

	return TotalSum;
};

double FiscalCheque::GetBankBonusSum(void)//get sum of all goods in the check
{
	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
		if ((cheque[i].ItemFlag==SL) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY))
			TotalSum+=cheque[i].ItemBankBonusDisc;
		if ((cheque[i].ItemFlag==RT) && (cheque[i].ItemCode!=TRUNC) && (cheque[i].ItemCode!=EXCESS_PREPAY))
			TotalSum-=cheque[i].ItemBankBonusDisc;
	}
/*
	if(trunc > .01)
	{
		// округление
		tsum = TotalSum - trunc * floor((TotalSum + .005) / trunc);
		TotalSum -= tsum;
	}
	else tsum = 0;
*/
	return TotalSum;
};

double FiscalCheque::GetFullSum(void)//get sum of all goods in the check
{
	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
		if (cheque[i].ItemFlag==SL)
			TotalSum+=cheque[i].ItemFullSum;
		if (cheque[i].ItemFlag==RT)
			TotalSum-=cheque[i].ItemFullSum;
	}
	return TotalSum;
};

double FiscalCheque::GetCalcSum(void)//get sum of all goods in the check
{
	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
		if (cheque[i].ItemFlag==SL)
			TotalSum+=cheque[i].ItemCalcSum;
		if (cheque[i].ItemFlag==RT)
			TotalSum-=cheque[i].ItemCalcSum;
	}

	return TotalSum;
};

bool FiscalCheque::GoodInAction9(GoodsInfo g)
{
    int act_type=9;

    // g.ItemName.left(1) = '@';
    bool res = (g.ItemDiscountsDesc.length()>act_type)
               &&
               (g.ItemDiscountsDesc[act_type-1]=='.');

    return res;

}


double FiscalCheque::GetCalcSum_Action9(void)//get sum of all goods in the check
{
    // Функция считает сумму товаров в акции (спец. позиция №9 - фиксированная скидка "СТОП-цена" mode=6)
    //смотри по наличию точки в 9й позиции строки скидок для товара

	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
	    if (GoodInAction9(cheque[i]))
        {
            if (cheque[i].ItemFlag==SL)
                TotalSum+=cheque[i].ItemCalcSum;
            if (cheque[i].ItemFlag==RT)
                TotalSum-=cheque[i].ItemCalcSum;
        }
	}

	return TotalSum;
}

double FiscalCheque::GetCalcSum_NotAction9(void)//get sum of all goods in the check
{
    // Функция считает сумму товаров в акции (спец. позиция №9 - фиксированная скидка "СТОП-цена" mode=6)
    //смотри по наличию точки в 9й позиции строки скидок для товара

	double TotalSum=0; //don't take into account 'storno' goods
	for (unsigned int i=0;i<cheque.size();i++)
	{
	    if (!GoodInAction9(cheque[i]))
        {
            if (cheque[i].ItemFlag==SL)
                TotalSum+=cheque[i].ItemFullSum;
            if (cheque[i].ItemFlag==RT)
                TotalSum-=cheque[i].ItemFullSum;
        }
	}

	return TotalSum;
}


double FiscalCheque::GetDiscountSum()
{
	double Sum = 0;
	for (int i=0;i<cheque.size();i++)
	{
		if
		(
		    (
			(cheque[i].ItemFlag==SL)||(cheque[i].ItemFlag==RT)
		    )
		    &&
		    (
			(cheque[i].ItemCalcPrice - cheque[i].ItemPrice > .015)
			||
			(cheque[i].ItemCalcSum   - cheque[i].ItemSum   > .005)
		    )

		)
		{
			Sum += cheque[i].ItemCalcSum - cheque[i].ItemSum;
		}
	}
	return RoundToNearest(Sum,0.01);
}


double FiscalCheque::GetDiscountSum_NotAction9()
{
	double Sum = 0;
	for (int i=0;i<cheque.size();i++)
	{
		if
		(
		    (
			(cheque[i].ItemFlag==SL)||(cheque[i].ItemFlag==RT)
		    )
		    &&
		    (
			(cheque[i].ItemCalcPrice - cheque[i].ItemPrice > .015)
			||
			(cheque[i].ItemCalcSum   - cheque[i].ItemSum   > .005)
		    )

		)
		{
		    //if (!GoodInAction9(cheque[i]))
		    if(cheque[i].ItemName.substr(0,1)!="@") // без СТОП-цены
                Sum += cheque[i].ItemCalcSum - cheque[i].ItemSum;
		}
	}
	return RoundToNearest(Sum,0.01);
}



unsigned int FiscalCheque::GetSize(void)//number of goods in the check without storno goods
{
	unsigned int Size=0; //get actual size of cheque
	for (unsigned int i=0;i<cheque.size();i++)
		if (cheque[i].ItemFlag!=ST)//don't pay attention to storno
			Size++;
	return Size;
};

unsigned int FiscalCheque::GetCount(void)//number of goods in the check
{
	return cheque.size();
};

double FiscalCheque::trunc = 0;
double FiscalCheque::tsum = 0;
double FiscalCheque::DiscountSumm = 0;
double FiscalCheque::DiscountSummNotAction = 0;
double FiscalCheque::CertificateSumm = 0;
double FiscalCheque::PrePayCertificateSumm = 0;
int FiscalCheque::CertificateCount = 0;
double FiscalCheque::BankBonusSumm = 0;
string FiscalCheque::CertificateString = "";
string FiscalCheque::BankBonusString = "";
bool FiscalCheque::Lock = false;
int FiscalCheque::NumPrecheck=0;

string FiscalCheque::egais_url = "";
string FiscalCheque::egais_sign = "";

bool FiscalCheque::PayBank = false;

double FiscalCheque::BonusSumm = 0;

cash_error::cash_error(int type,int code,string msg)//it is clear
{
	error_type=type;
	error_code=code,
	error_text=msg;
	showmsg=true;
};

const char* cash_error::what()
{
	return error_text.c_str();
};

int cash_error::type()
{
	return error_type;
}

int cash_error::code()
{
	return error_code;
}

int FiscalCheque::SetPaymentType(int pt)
{
	for(int i = 0; i < cheque.size(); i++)
		cheque[i].PaymentType = pt;
	return PaymentType = pt;
}

class MyMutex{
	QMutex * pm;
public:
	MyMutex(QMutex & m){m.lock(); pm = &m;};
	~MyMutex(){pm->unlock();};
};

int LogEC(string msg)
{
    LogEC((char*)msg.c_str());
}

int LogEC(char * msg, char * data, int len)
{
    if (msg[0]==0) return 0;

    static QMutex m;
    MyMutex mm(m);

    FILE * log = fopen( "tmp/log_EC", "a");// "r");
    if(!log){
        return 2;
    }

    time_t tt = time(NULL);
    tm* t = localtime(&tt);

    fprintf(log, "%02d.%02d.%02d %02d:%02d:%02d\t%s\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year % 100,
            t->tm_hour, t->tm_min, t->tm_sec, msg);

    if(data){
        if(!len){
            len = strlen(data);
        }
        fwrite(data, sizeof(char), len, log);
        fprintf(log, "<*end of data*>\n");
    }

    fclose(log);
    return 0;
}

bool UTM_is_owner(const QString* barcode) {

#define EC_res_file "/Kacca/tmp/EC_r"

    int timeout = 3;
    int maxtime = 5;
    int try_number = 1;
    char curl_cmd[2048];
    bool is_owner = false;
    bool no_answer = true;
    QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
    QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
    sm.setFixedSize(250,100);
    msg.setText(W2U("Проверка ЕГАИС... "+Int2Str(try_number)));
    msg.setFixedSize(250,100);
    msg.setAlignment( QLabel::AlignCenter );

    while (try_number<=5 && no_answer) {
        sprintf(curl_cmd, "curl --max-time %d "
                          "--connect-timeout %d "
                          "http://%s/api/mark/check?code=%s -o " EC_res_file,
                maxtime,
                timeout,
                CashReg->EgaisSettings.IP.c_str(),
                barcode->ascii());

        LogEC("curl_cmd="+(string)curl_cmd);

//        QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
//        QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
//        sm.setFixedSize(250,100);
        msg.setText(W2U("Проверка ЕГАИС... "+Int2Str(try_number)+"\n"
                        +"ждем "+Int2Str(timeout)+" сек."));
        msg.setFixedSize(250,100);
        msg.setAlignment( QLabel::AlignCenter );
        sm.show();
        thisApp->processEvents();

        unlink(EC_res_file);
        int retcode = system(curl_cmd);
        LogEC("retcode="+Int2Str(retcode));

        QFile file(EC_res_file);
        if(!file.open(IO_ReadOnly)) {
            LogEC("error open file " EC_res_file" = " + file.errorString());
            try_number += 1;
//            timeout = timeout + timeout;
//            maxtime = maxtime + maxtime;
            no_answer = true;
        } else{
            QTextStream in(&file);
            while(!in.atEnd()) {
                is_owner = false;
                QString line = in.readLine();
                if (line.contains("\"owner\":true")) {
                    LogEC("UTM answer = "+line);
                    is_owner = true;
                    no_answer = false;
                    break;
                } else if (line.contains("\"owner\":false")) {
                    LogEC("UTM answer = "+line);
                    no_answer = false;
                    break;
                } else {
                    LogEC("UTM answer = "+line);
                    try_number += 1;
//                    timeout = timeout + timeout;
//                    maxtime = maxtime + maxtime;
                    no_answer = true;
                    break;
                }
            }
            file.close();
        }
    }

    LogEC("UTM_is_owner("
          + (string)barcode->ascii()
          + ") = "
          + Int2Str(is_owner)
          + "\n"
          );
    return is_owner;
}


int Log(string msg)
{
    Log((char*)msg.c_str());
}

int Log(char * msg, char * data, int len)
{
	if (msg[0]==0) return 0;

	static QMutex m;
	MyMutex mm(m);

	FILE * log = fopen( "tmp/log", "a");// "r");
	if(!log){
		return 2;
	}

	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	fprintf(log, "%02d.%02d.%02d %02d:%02d:%02d\t%s\n", t->tm_mday, t->tm_mon + 1, t->tm_year % 100, t->tm_hour, t->tm_min, t->tm_sec, msg);
	if(data){
		if(!len){
			len = strlen(data);
		}
		fwrite(data, sizeof(char), len, log);
		fprintf(log, "<*end of data*>\n");
	}

	unsigned long size[4];
	fgetpos(log, (fpos_t*)size);
//	printf("%d\n", size[0]);
	if((false) && (size[0] > 3048576)){
		fclose(log);

        // + dms === логи не нужно удалять, их нужно беречь ====
		time_t ltime;		time( &ltime );
		struct tm *now;		now = localtime( &ltime );
		char str[256];
		//sprintf( str, "log%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
		//rename("tmp/log",(CashReg->DBArchPath+"/"+str).c_str());

		sprintf( str, "log%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
	    MoveFileSys("tmp/log", CashReg->DBArchPath+"/"+str);

        // - dms

		//remove("tmp/log");
		return 3;
	}

	fclose(log);
	return 0;
}

void GoodsInfo::CalcSum()
{
	ItemCount = RoundToNearest(ItemCount,ItemPrecision);
	/*
	if(NDS > .01){
		double s = RoundToNearest(ItemPrice*100/(100+NDS) - .005,0.01);
		s *= ItemCount;
		s = RoundToNearest(s, .01);
		ItemSum = s + RoundToNearest(s * NDS / 100,0.01);
	}else{
	*/
		ItemSum = RoundToNearest(ItemPrice*ItemCount,0.01);
	//}
	ItemCalcSum = RoundToNearest(ItemCalcPrice*ItemCount,0.01);
	ItemFullSum = RoundToNearest(ItemFullPrice*ItemCount,0.01);

}

long long FiscalCheque::GetCustomerCode()
{
	return cheque[0].CustomerCode[0];
}

double FiscalCheque::Reduction(int KKMNo,int KKMSmena,time_t KKMTime,bool SumWeight, bool SumPrice)//
{
	tmpcheque.clear();
	unsigned int k;
	bool wasfind;
	for (unsigned int i=0;i<cheque.size();i++)
	{
	    	if (cheque[i].ItemFlag==SL)
		{
			wasfind = false;
			for (unsigned int j=0;j<tmpcheque.size();j++)
			{
				if
				(

                    // + dms === 2020-10-13 ==== На время акции Виски+Сок =====
                    (cheque[i].ItemCode!=24979)
                    &&
                    // - dms

                    // + dms
                    (!cheque[i].ItemAlco)
                    &&
                    (!cheque[i].ItemCigar)
                    &&
                    (!cheque[i].ItemTextile)
                    &&
                    (!cheque[i].ItemMilk)
                    &&
                    // - dms
                    // + dms
                    (cheque[i].ItemCode!=FreeSumCode)
                    // - dms
                    &&
					(cheque[i].ItemCode==tmpcheque[j].ItemCode)
					&&
					(
						(
                            // + dms ===\ могут быть товары с разными ценами по разным штрихкодам
                            (cheque[i].ItemBarcode==tmpcheque[j].ItemBarcode)
                            &&
                            // - dms ===/
                            // + dms ===\ товары с штрих-кодами менеджеров
                            (cheque[i].ItemSecondBarcode==tmpcheque[j].ItemSecondBarcode)
                            &&
                            // - dms ===/
							((cheque[i].ItemPrecision == 1)	&& (tmpcheque[j].ItemPrecision == 1))
						)

						||

						(
                            // + dms ===\ для весовых товаров штрихкод учитывать не нужно
                            SumWeight
                            &&
                            ((cheque[i].ItemPrecision < 1 )	&& (tmpcheque[j].ItemPrecision < 1))
						)

					)
//					&&
//					(
//					// Складываем только товары с одинаковой ценой,
//					// если не стоит специальный флаг
//					SumPrice
//					||
//					(cheque[i].ItemPrice==tmpcheque[j].ItemPrice)
//					)
					&&
					(
					// Товары по акции 9 не складываем
					// если не стоит специальный флаг
					SumPrice
					||
					!GoodInAction9(cheque[i])
					)
				)
				{
					wasfind=true;
					k=j;
					j=tmpcheque.size();
				}
			}
			if(wasfind)
			{
				tmpcheque[k].ItemCount+=cheque[i].ItemCount;
			}
			else
			{
				cheque[i].KKMNo    = KKMNo;
				cheque[i].KKMSmena = KKMSmena;
				cheque[i].KKMTime  = KKMTime;
				tmpcheque.insert(tmpcheque.end(),cheque[i]);
			}
			//tmpcheque.prepend(cheque[i]);
		}
	}

	for (unsigned int i=0;i<cheque.size();i++)
	{
	    	if (cheque[i].ItemFlag!=SL)
		{
			cheque[i].KKMNo    = KKMNo;
			cheque[i].KKMSmena = KKMSmena;
			cheque[i].KKMTime  = KKMTime;
			tmpcheque.insert(tmpcheque.end(),cheque[i]);
		}
	}

	cheque.clear();
	for (unsigned int i=0;i<tmpcheque.size();i++)
	{
		cheque.insert(cheque.end(),tmpcheque[i]);
	}
}


double FiscalCheque::ReductionByCode(int KKMNo,int KKMSmena,time_t KKMTime,bool SumWeight)//
{
	tmpcheque.clear();
	unsigned int k;
	bool wasfind;
	for (unsigned int i=0;i<cheque.size();i++)
	{
	    	if (cheque[i].ItemFlag==SL)
		{
			wasfind = false;
			for (unsigned int j=0;j<tmpcheque.size();j++)
			{
				if
				(
                    // + dms === 2020-10-13 ==== На время акции Виски+Сок =====
                    (cheque[i].ItemCode!=24979)
                    &&
                    // - dms

                    // + dms
                    (!cheque[i].ItemAlco)
                    &&
                    (!cheque[i].ItemCigar)
                    &&
                    (!cheque[i].ItemTextile)
                    &&
                    (!cheque[i].ItemMilk)
                    &&
                    // - dms
                    // + dms
                    (cheque[i].ItemCode!=FreeSumCode)
                    // - dms
                    &&
					(cheque[i].ItemCode==tmpcheque[j].ItemCode)
					&&
					(
                       ((cheque[i].ItemPrecision == 1)	&& (tmpcheque[j].ItemPrecision == 1))
                       ||
                       SumWeight
                    )

				)
				{
					wasfind=true;
					k=j;
					j=tmpcheque.size();
				}
			}
			if(wasfind)
			{
				tmpcheque[k].ItemCount+=cheque[i].ItemCount;
			}
			else
			{
				cheque[i].KKMNo    = KKMNo;
				cheque[i].KKMSmena = KKMSmena;
				cheque[i].KKMTime  = KKMTime;
				tmpcheque.insert(tmpcheque.end(),cheque[i]);
			}
			//tmpcheque.prepend(cheque[i]);
		}
	}

	for (unsigned int i=0;i<cheque.size();i++)
	{
	    	if (cheque[i].ItemFlag!=SL)
		{
			cheque[i].KKMNo    = KKMNo;
			cheque[i].KKMSmena = KKMSmena;
			cheque[i].KKMTime  = KKMTime;
			tmpcheque.insert(tmpcheque.end(),cheque[i]);
		}
	}

	cheque.clear();
	for (unsigned int i=0;i<tmpcheque.size();i++)
	{
		cheque.insert(cheque.end(),tmpcheque[i]);
	}
}

string GetCurTime(void)//get current time as a string
{
	time_t cur_time=time(NULL);
	tm *l_time=localtime(&cur_time);
	return QTime(l_time->tm_hour,l_time->tm_min,l_time->tm_sec).toString().latin1();
}

string GetCurDate(void)//get current date as a string
{
	return xbDate().FormatDate(DateTemplate).c_str();
}

// форматирует строку в чеке по принципу
// |FistrStr...............SecondStr|
//!string GetFormatString(string FirstStr, string SecondStr, int LineWidth = 0)
string GetFormatString(string FirstStr, string SecondStr, int LineWidth)
{
    if (!LineWidth) LineWidth = getLineWidthDefault();

    string SpaceStr="";
    if (LineWidth>FirstStr.length()+SecondStr.length())
        for (unsigned int i=0;i<LineWidth-FirstStr.length()-SecondStr.length();i++)
                SpaceStr+=' ';

    return FirstStr + SpaceStr + SecondStr;
}

char* GetCurDateTime(void)
{
    char s[255];
	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	sprintf(s, "%02d.%02d.%02d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
	return s;
}

//!string GetCenterString(string Str, string sp = " ", int LineWidth = 0)
string GetCenterString(string Str, string sp, int LineWidth)
{
    if (!LineWidth) LineWidth = getLineWidthDefault();

    string ResStr=Str;
    int midl = (int) (LineWidth-Str.length())/2;
    if (midl>0)
    {
        for (unsigned int i=0;i<midl-1;i++)
            ResStr=sp+ResStr;

        for (unsigned int i=0;i<midl;i++)
            ResStr+=sp;
    }
    return ResStr;
}

int LogMessage(string Pref, string MessageStr)
{
    FILE* f = fopen("./tmp/history.msg", "a");
    if(f)
    {
        time_t tt = time(NULL);
        tm* t = localtime(&tt);
        fprintf(f,"\n%s%02d:%02d:%02d>%s", Pref.c_str(),
        t->tm_hour, t->tm_min, t->tm_sec,
        MessageStr.c_str());
        fclose(f);
    }
}


string StrReplaceAll(string str, string s_str, string d_str)
{
  for(unsigned i=0; i=str.find(s_str, i), i!=std::string::npos;)
  {
    str.replace(i, s_str.length(), d_str);
    i+=d_str.length();
  }
  return str;
}


string trim(string str)
{
    string resstr = "";

    int beg=0, end=0, i=0;
    bool stopbeg = false;

    while (i<str.length())
    {
        if (str[i]==' ')
        {
            if (!stopbeg) beg=i;
        }
        else
        {
            stopbeg = true;
            end = i;
        }

        i++;
    }

    for(i=beg;i<=end;i++) resstr += str[i];

    return resstr;
}

long Int100(double s)
{
    return Double2Int(s*100);
}




///////////////////////////////////////////////////////////
///////   BASE 64
///////////////////////

typedef
unsigned
char    uchar;                              // Define unsigned char as uchar.

typedef
unsigned
int     uint;                               // Define unsigned int as uint.


// Macro definitions:

#define b64is7bit(c)  ((c) > 0x7f ? 0 : 1)  // Valid 7-Bit ASCII character?
#define b64blocks(l) (((l) + 2) / 3 * 4 + 1)// Length rounded to 4 byte block.
#define b64octets(l)  ((l) / 4  * 3 + 1)    // Length rounded to 3 byte octet.

// Note:    Tables are in hex to support different collating sequences

static
const                                       // Base64 Index into encoding
uchar  pIndex[]     =   {                   // and decoding table.
                        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
                        0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
                        0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
                        0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
                        0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
                        0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33,
                        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f
                        };

static
const                                       // Base64 encoding and decoding
uchar   pBase64[]   =   {                   // table.
                        0x3e, 0x7f, 0x7f, 0x7f, 0x3f, 0x34, 0x35, 0x36,
                        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x7f,
                        0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x01,
                        0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                        0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
                        0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                        0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x1a, 0x1b,
                        0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
                        0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
                        0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33
                        };


/*---------------------------------------------------------------------------*/
/* b64encode - Encode a 7-Bit ASCII string to a Base64 string.               */
/* ===========================================================               */
/*                                                                           */
/* Call with:   char *  - The 7-Bit ASCII string to encode.                  */
/*                                                                           */
/* Returns:     bool    - True (!0) if the operation was successful.         */
/*                        False (0) if the operation was unsuccessful.       */
/*---------------------------------------------------------------------------*/
bool b64encode(char *s, string *res)
{
    int     l   = strlen(s);                // Get the length of the string.
    int     x   = 0;                        // General purpose integers.
    char   *b, *p;                          // Encoded buffer pointers.

    while   (x < l)                         // Validate each byte of the string
    {                                       // ...to ensure that it's 7-Bit...
        if (!b64is7bit((uchar) *(s + x)))   // ...ASCII.
        {
//            printf("\"%s\" is not a 7-Bit ASCII string.\n", s);
            return false;                   // Return false if it's not.
        }
        x++;                                // Next byte.
    }

    if (!(b = b64buffer(s, true)))          // Allocate an encoding buffer.
        return false;                       // Can't allocate encoding buffer.

    memset(b, 0x3d, b64blocks(l) - 1);      // Initialize it to "=".

    p = b;                                  // Save the buffer pointer.
    x = 0;                                  // Initialize string index.

    while   (x < (l - (l % 3)))             // encode each 3 byte octet.
    {
        *b++   = pIndex[  s[x]             >> 2];
        *b++   = pIndex[((s[x]     & 0x03) << 4) + (s[x + 1] >> 4)];
        *b++   = pIndex[((s[x + 1] & 0x0f) << 2) + (s[x + 2] >> 6)];
        *b++   = pIndex[  s[x + 2] & 0x3f];
         x    += 3;                         // Next octet.
    }

    if (l - x)                              // Partial octet remaining?
    {
        *b++        = pIndex[s[x] >> 2];    // Yes, encode it.

        if  (l - x == 1)                    // End of octet?
            *b      = pIndex[ (s[x] & 0x03) << 4];
        else
        {                                   // No, one more part.
            *b++    = pIndex[((s[x]     & 0x03) << 4) + (s[x + 1] >> 4)];
            *b      = pIndex[ (s[x + 1] & 0x0f) << 2];
        }
    }

    *res = p;

    b64stats(s, p, true);                   // Display some encoding stats.
    free(p);                                // De-allocate the encoding buffer.
    //printf("Base64 encoding complete.\n");
    return true;                            // Return to caller with success.
}

/*---------------------------------------------------------------------------*/
/* b64decode - Decode a Base64 string to a 7-Bit ASCII string.               */
/* ===========================================================               */
/*                                                                           */
/* Call with:   char *  - The Base64 string to decode.                       */
/*                                                                           */
/* Returns:     bool    - True (!0) if the operation was successful.         */
/*                        False (0) if the operation was unsuccessful.       */
/*---------------------------------------------------------------------------*/
bool b64decode(char *s, string *res)
{
    int     l = strlen(s);                  // Get length of Base64 string.
    char   *b, *p;                          // Decoding buffer pointers.
    uchar   c = 0;                          // Character to decode.
    int     x = 0;                          // General purpose integers.
    int     y = 0;

    static                                  // Collating sequence...
    const                                   // ...independant "===".
    char    pPad[]  =   {0x3d, 0x3d, 0x3d, 0x00};

    if  (l % 4)                             // If it's not modulo 4, then it...
        return b64isnot(s, NULL);           // ...can't be a Base64 string.

    if  (b = strchr(s, pPad[0]))            // Only one, two or three equal...
    {                                       // ...'=' signs are allowed at...
        if  ((b - s) < (l - 3))             // ...the end of the Base64 string.
            return b64isnot(s, NULL);       // Any other equal '=' signs are...
        else                                // ...invalid.
            if  (strncmp(b, (char *) pPad + 3 - (s + l - b), s + l - b))
                return b64isnot(s, NULL);
    }

    if  (!(b = b64buffer(s, false)))        // Allocate a decoding buffer.
        return false;                       // Can't allocate decoding buffer.

    p = s;                                  // Save the encoded string pointer.
    x = 0;                                  // Initialize index.

    while ((c = *s++))                      // Decode every byte of the...
    {                                       // Base64 string.
        if  (c == pPad[0])                  // Ignore "=".
            break;

        if (!b64valid(&c))                  // Valid Base64 Index?
            return b64isnot(s, b);          // No, return false.

        switch(x % 4)                       // Decode 4 byte words into...
        {                                   // ...3 byte octets.
        case    0:                          // Byte 0 of word.
            b[y]    =  c << 2;
            break;
        case    1:                          // Byte 1 of word.
            b[y]   |=  c >> 4;

            if (!b64is7bit((uchar) b[y++])) // Is 1st byte of octet valid?
                return b64isnot(s, b);      // No, return false.

            b[y]    = (c & 0x0f) << 4;
            break;
        case    2:                          // Byte 2 of word.
            b[y]   |=  c >> 2;

            if (!b64is7bit((uchar) b[y++])) // Is 2nd byte of octet valid?
                return b64isnot(s, b);      // No, return false.

            b[y]    = (c & 0x03) << 6;
            break;
        case    3:                          // Byte 3 of word.
            b[y]   |=  c;

            if (!b64is7bit((uchar) b[y++])) // Is 3rd byte of octet valid?
                return b64isnot(s, b);      // No, return false.
        }
        x++;                                // Increment word byte.
    }

    *res = b;

    b64stats(p, b, false);                  // Display some decoding stats.
    free(b);                                // De-allocate decoding buffer.
    //printf("Base64 decoding complete.\n");
    return true;                            // Return to caller with success.
}

/*---------------------------------------------------------------------------*/
/* b64valid - validate the character to decode.                              */
/* ============================================                              */
/*                                                                           */
/* Checks whether the character to decode falls within the boundaries of the */
/* Base64 decoding table.                                                    */
/*                                                                           */
/* Call with:   char    - The Base64 character to decode.                    */
/*                                                                           */
/* Returns:     bool    - True (!0) if the character is valid.               */
/*                        False (0) if the character is not valid.           */
/*---------------------------------------------------------------------------*/
bool b64valid(uchar *c)
{
    if ((*c < 0x2b) || (*c > 0x7a))         // If not within the range of...
        return false;                       // ...the table, return false.

    if ((*c = pBase64[*c - 0x2b]) == 0x7f)  // If it falls within one of...
        return false;                       // ...the gaps, return false.

    return true;                            // Otherwise, return true.
}

/*---------------------------------------------------------------------------*/
/* b64isnot - Display an error message and clean up.                         */
/* =================================================                         */
/*                                                                           */
/* Call this routine to display a message indicating that the string being   */
/* decoded is an invalid Base64 string and de-allocate the decoding buffer.  */
/*                                                                           */
/* Call with:   char *  - Pointer to the Base64 string being decoded.        */
/*              char *  - Pointer to the decoding buffer or NULL if it isn't */
/*                        allocated and doesn't need to be de-allocated.     */
/*                                                                           */
/* Returns:     bool    - True (!0) if the character is valid.               */
/*                        False (0) if the character is not valid.           */
/*---------------------------------------------------------------------------*/
bool b64isnot(char *p, char *b)
{
    //printf("\"%s\" is not a Base64 encoded string.\n", p);

    if  (b)                                 // If the buffer pointer is not...
        free(b);                            // ...NULL, de-allocate it.

    return  false;                          // Return false for main.
}

/*---------------------------------------------------------------------------*/
/* b64buffer - Allocate the decoding or encoding buffer.                     */
/* =====================================================                     */
/*                                                                           */
/* Call this routine to allocate an encoding buffer in 4 byte blocks or a    */
/* decoding buffer in 3 byte octets.  We use "calloc" to initialize the      */
/* buffer to 0x00's for strings.                                             */
/*                                                                           */
/* Call with:   char *  - Pointer to the string to be encoded or decoded.    */
/*              bool    - True (!0) to allocate an encoding buffer.          */
/*                        False (0) to allocate a decoding buffer.           */
/*                                                                           */
/* Returns:     char *  - Pointer to the buffer or NULL if the buffer        */
/*                        could not be allocated.                            */
/*---------------------------------------------------------------------------*/
char *b64buffer(char *s, bool f)
{
    int     l = strlen(s);                  // String size to encode or decode.
    char   *b;                              // String pointers.

    if  (!l)                                // If the string size is 0...
        return  NULL;                       // ...return null.

   if (!(b = (char *) calloc((f ? b64blocks(l) : b64octets(l)),
               sizeof(char))))
      //  printf("Insufficient real memory to %s \"%s\".\n",
        //      (f ? "encode" : "decode"), s);
    return  b;                              // Return the pointer or null.
}

/*---------------------------------------------------------------------------*/
/* b64stats - Display encoding or decoding statistics.                       */
/* ===================================================                       */
/*                                                                           */
/* Call this routine to display some encoding or decoding statistics.        */
/*                                                                           */
/* Call with:   char *  - Pointer to original input string.                  */
/*              char *  - Pointer to the resultant encoded or decoded string.*/
/*              bool    - True (!0) to display encoding statistics.          */
/*                        False (0) to display decoding statistics.          */
/*                                                                           */
/* Returns:     void    - Nothing.                                           */
/*---------------------------------------------------------------------------*/
void b64stats(char *s, char *b, bool f)
{
    char   *e, *d, *p;                      // General purpose string pointers.

    e   = "Encoded";                        // Point to the "Encoded" string.
    d   = "Decoded";                        // Point to the "Decoded" string.

    if  (!f)                                // If we are decoding...
    {                                       // ...switch the pointers.
        p   = e;                            // Save, the "Encoded" pointer.
        e   = d;                            // Point to the "Decoded" string.
        d   = p;                            // Point to the "Encoded" string.
    }

    /*                                        // Display the statistics.
    printf("%s string length = %d\n", d, strlen(s));
    printf("%s string        = \"%s\"\n", d, s);
    printf("%s buffer size   = %d\n", e,
          (f ? b64blocks(strlen(s)) : b64octets(strlen(s))));
    printf("%s string length = %d\n", e, strlen(b));
    printf("%s string        = \"%s\"\n", e, b);
    */
}


#include <openssl/sha.h>

// возвращает хэш по номеру карты
string GetSHA1(string card)
{

    unsigned char md[20];

	//unsigned char* ClientIDstr =
	SHA1( (unsigned char *)card.c_str(),	strlen((const char*)card.c_str()),	md); // хэш SHA1

    string result = "";
    char tmp[1];

    for (int i=0;i<20;i++)
    {
        sprintf(tmp,"%.2x",md[i]);
        result += tmp;
    }

    // заглушка
    //;;result = "C20BBF80524A22A480BC171F536EF1507F53E757";

    return result;
}



double FiscalCheque::GetBonusAddSum()
{
	double Sum = 0;
	for (int i=0;i<cheque.size();i++)
	{
		if ((cheque[i].ItemFlag==SL)||(cheque[i].ItemFlag==RT))
		{
			Sum += cheque[i].ItemBonusAdd;
		}
	}
	return RoundToNearest(Sum,0.01);
}


double FiscalCheque::GetBonusDiscSum()
{
	double Sum = 0;
	for (int i=0;i<cheque.size();i++)
	{
		if ((cheque[i].ItemFlag==SL)||(cheque[i].ItemFlag==RT))
		{
			Sum += cheque[i].ItemBonusDisc;
		}
	}
	return RoundToNearest(Sum,0.01);
}


string DelAllStr(string str, string substr)
{
    string res = str;
    size_t pos = 0;
    size_t i = 0;

    while ((i = res.find(substr, pos)) != string::npos)
    {
        res.erase(i, 1);
        pos+=i;
    }


   return res;

}


string RightSymbol(string str, int num)
{
  string res="";
  for(int i=1;(i<=str.length()) && (i<=num) ;i++)
    res = str[str.length()-i]+res;

  return res;
}

bool filepath_exists(string filename) {
    ifstream ifile;
    ifile.open(filename.c_str());
    if(ifile) {
        ifile.close();
        return true;
    } else {
        return false;
    }
}
