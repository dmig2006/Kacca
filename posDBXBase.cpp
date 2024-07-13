#include <math.h>
#include <qthread.h>
#include <qmutex.h>
#include <posDBXBase.h>
#include <xbase/xbase.h>
#include <DbfTable.h>
#include <Utils.h>
#include "Payment.h"
#include "PriceSel.h"

//#define	IN_SCAN		1
//#define	IN_CODE		2
//#define	IN_PRICE	3
//#define	IN_NAME		4
//#define	IN_GR		5
//#define	IN_RSCAN	6

#define BLOCK_CIGAR
#define ALCO_VES
#define PIVO_BOTTLE
#define BIODOBAVKA


posDBXBase::posDBXBase()
{
	DbfMutex = new QMutex(true);

	//create internal dbf tables
	StaffTable = new DbfTable;
	SalesTable = new DbfTable;
	NonSentTable = new DbfTable;
	CurrentCheckTable = new DbfTable;
	GoodsTable = new DbfTable;
	GoodsPriceTable = new DbfTable;
	BarInfoTable = new DbfTable;
	KeyboardTable = new DbfTable;
	GroupsTable = new DbfTable;
	ActionsTable = new DbfTable;
	ActreportTable = new DbfTable;

	BankActionsTable = new DbfTable;
	BankActreportTable = new DbfTable;

	CustomersTable = new DbfTable;
	CustRangeTable = new DbfTable;
	CustomersSalesTable = new DbfTable;
	CustomersValueTable = new DbfTable;
	DiscountsTable = new DbfTable;
	BankDiscountTable = new DbfTable;
	MsgLevelTable = new DbfTable;
#ifdef RET_CHECK
	ReturnTable = new DbfTable;
#endif
	PaymentTable = new DbfTable;
	PaymentToSendTable = new DbfTable;
	CrPaymentTable = new DbfTable;

	BankBonusTable = new DbfTable;
	BankBonusToSendTable = new DbfTable;

	BonusTable = new DbfTable;
	BonusToSendTable = new DbfTable;

	ActsizTable = new DbfTable;
	ActsizToSendTable = new DbfTable;

}

posDBXBase::~posDBXBase()
{
	//delete ConfigTable;
	delete StaffTable;
	delete SalesTable;
	delete NonSentTable;
	delete CurrentCheckTable;
	delete GoodsTable;
	delete GoodsPriceTable;
	delete BarInfoTable;
	delete KeyboardTable;
	delete GroupsTable;
	delete ActionsTable;
	delete ActreportTable;

	delete BankActionsTable;
	delete BankActreportTable;

	delete CustomersTable;
	delete CustomersSalesTable;
	delete CustomersValueTable;
	delete DiscountsTable;
	delete BankDiscountTable;
	delete MsgLevelTable;
#ifdef RET_CHECK
	delete ReturnTable;
#endif
	delete CrPaymentTable;
	delete PaymentTable;
	delete PaymentToSendTable;

	delete BankBonusTable;
	delete BankBonusToSendTable;

	delete BonusTable;
	delete BonusToSendTable;

	delete DbfMutex;
}



int posDBXBase::LoadCodesFromDB(vector <int> *Codes)
{
	try
	{
		DbfMutex->lock();

		if (KeyboardTable->GetFirstRecord())
		{
			do
				Codes->insert(Codes->end(),KeyboardTable->GetLongField("KEYCODE"));
			while(KeyboardTable->GetNextRecord());
		}

		DbfMutex->unlock();
	}
	catch(cash_error &er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}
}

int posDBXBase::LoadBarcodeTableFromDB(vector<BarcodeInfo*> *tmpBarcodeTable)
{
	try
	{
		DbfMutex->lock();

		if (BarInfoTable->GetFirstRecord())
		{//read id/name of barcode's systems
			do
			{
				BarcodeInfo *tmpInfo=new BarcodeInfo;
				tmpInfo->CodeID=BarInfoTable->GetStringField("CODEID");
				tmpInfo->CodeName=BarInfoTable->GetStringField("CODENAME");

				tmpBarcodeTable->insert(tmpBarcodeTable->end(),tmpInfo);
			}
			while(BarInfoTable->GetNextRecord());
		}

		DbfMutex->unlock();
		return 0;
	}
	catch(cash_error &er)
	{
		DbfMutex->unlock();
		LoggingError(er);
		return -1;
	}
}

int posDBXBase::LoadFRSoundFromDB(char *frSound,const char* name)
{
	try
	{
		DbfMutex->lock();

		if (KeyboardTable->Locate("NAME",name))
			*frSound =(char)KeyboardTable->GetLongField("KEYCODE");

		DbfMutex->unlock();
	}
	catch(cash_error &er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}
}

int posDBXBase::LoadDiscountTypeFromDB(long long id,string *DiscountType)
{
	try
	{
		DbfMutex->lock();

		*DiscountType="";
		if(CustomersTable->Locate("ID",id))
		{
			*DiscountType += CustomersTable->GetField("DISCOUNTS");
		}
		else
		{
		    for(int i=0;i<MAXDISCOUNTLEN;i++) *DiscountType += "X";
		}
		DbfMutex->unlock();
	}
	catch(cash_error &er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}
}

int posDBXBase::AddGoods(GoodsInfo *CurrentGoods,int key)
// возвращаемое значение
// если все нормально то 0
// если ошибка то >0
// 1 ---
// если совсем ошибка то <0

// key ключ для поиска в случае dbf номер поля
{
	try
	{

		DbfMutex->lock();
		//if (GoodsTable->GetDoubleField("PRICE")<epsilon)
		if (GoodsTable->GetDoubleField("PRICE")<0)
		{
			DbfMutex->unlock();
			return 1; //неправильная цена товара
		}

        CurrentGoods->ItemBarcode   =GoodsTable->GetStringField("BARCODE");

		CurrentGoods->ItemCode		=GoodsTable->GetLongField("CODE");
		CurrentGoods->ItemName		=GoodsTable->GetStringField("NAME");
		CurrentGoods->ItemDBPos		=GoodsTable->GetRecNo();
		CurrentGoods->ItemFullPrice	=CurrentGoods->ItemPrice=GoodsTable->GetDoubleField("PRICE");
		CurrentGoods->ItemCalcPrice	=CurrentGoods->ItemFullPrice;
		CurrentGoods->ItemExcessPrice	=CurrentGoods->ItemFullPrice;
		CurrentGoods->ItemFixedPrice	=GoodsTable->GetDoubleField("FIXEDPRICE");
        CurrentGoods->ItemMinimumPrice	=GoodsTable->GetDoubleField("MINPRICE");

		if(GoodsTable->GetFieldNo("ALCO") != -1)
		{
            CurrentGoods->ItemAlco		= GoodsTable->GetLongField("ALCO");
		}
		else
		{
            CurrentGoods->ItemAlco = 0;
        }

		if(GoodsTable->GetFieldNo("NDS") != -1)
		{
			CurrentGoods->NDS = GoodsTable->GetDoubleField("NDS");
		}
		if (GoodsTable->GetLogicalField("GTYPE"))
		{
			CurrentGoods->ItemPrecision=1;//if true one has single goods
		}
		else
		{
			CurrentGoods->ItemPrecision=0.001;
		}

	
		if
		(
			GoodsTable->GetFieldNo("MULT") != -1 &&
			GoodsTable->GetDoubleField("MULT") > .0001
		)
		{
			CurrentGoods->ItemCount = GoodsTable->GetDoubleField("MULT");
			CalcCountPrice(CurrentGoods);
		}
		else
		{
			CurrentGoods->ItemCount=1;
		}
			
		CurrentGoods->CalcSum();

		CurrentGoods->ItemDiscountsDesc=GoodsTable->GetStringField("SKIDKA");
		CurrentGoods->Lock=GoodsTable->GetStringField("LOCK");

		DbfMutex->unlock();

		return 0;
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
		return -1;
	}
}

int posDBXBase::FindGoodsByCode(int code)
{
	//DbfMutex->lock();
	return GoodsTable->Locate("CODE",code);
	//DbfMutex->unlock();

}

int posDBXBase::CalcCountPrice(GoodsInfo *CurrentGoods)
{
//	if(PriceChecker!=NULL)
//		return 0;
	int i = 0;
	if(GoodsTable->GetRecNo() != CurrentGoods->ItemDBPos)
	{
		if(CurrentGoods->ItemBarcode.empty())
		{
			GoodsTable->Locate("CODE", CurrentGoods->ItemCode);
		}
		else
		{
			GoodsTable->Locate("BARCODE", CurrentGoods->ItemBarcode.c_str());
		}
		GoodsTable->SetRecNo(CurrentGoods->ItemDBPos);
	}
	CurrentGoods->ItemPrice=GoodsTable->GetDoubleField("PRICE");
	
	//Log("Колличество=" + CurrentGoods->ItemBarcode);
	
	while(1){
		//Log("TESTTESTETSE");
		char fname[64];
		sprintf(fname, "Q%d", i);
		if(GoodsTable->GetFieldNo(fname) == -1 || GoodsTable->GetDoubleField(fname) < .001)
			break;
		if(CurrentGoods->ItemCount >= GoodsTable->GetDoubleField(fname)){
			sprintf(fname, "P%d", i);
			if(GoodsTable->GetFieldNo(fname) == -1)
				break;
			double p = GoodsTable->GetDoubleField(fname);
			if(p > .01)
				CurrentGoods->ItemPrice = p;
		}
		i++;
	}
	return 0;
	
	//Log("TESTTESTGOOD");
}

void posDBXBase::ProcessDiscount(GoodsInfo *CurGoods,double CheckSum,long long CurrentCustomer,string DiscountType, long long AddonCustomer, double CheckSum_WithoutAction)
{
	CurGoods->ItemDSum=0;
	CurGoods->CustomerCode[0]=CurrentCustomer;
	CurGoods->CustomerCode[1]=AddonCustomer;
	CurGoods->DiscFlag=false;

    bool NoCheckStopPriceSumma = false;

    // специальный флаер, снимающий ограничения по сумме для СТОП-цены
    if (AddonCustomer==993000107028LL) NoCheckStopPriceSumma=true;


	DbfMutex->lock();
	int RecordsCount=DiscountsTable->RecordCount();
	DbfMutex->unlock();

	if (RecordsCount==0)
		return;

	time_t cur_time=time(NULL);
	tm *l_time=localtime(&cur_time);
	int Seconds=l_time->tm_hour*3600+l_time->tm_min*60+l_time->tm_sec;
	int Days=xbDate().JulianDays();

	double Percent=0,Charge=0,WholePercent=0,WholeCharge=0;
	long ByPrice=0;
	int i = 0;

	try
	{
		DbfMutex->lock();
		DiscountsTable->GetFirstRecord();
		DbfMutex->unlock();

		DbfMutex->lock();
		do
		{
			int type=DiscountsTable->GetLongField("DISCTYPE");
			int mode=DiscountsTable->GetLongField("MODE");
			int allgoodssign=DiscountsTable->GetLongField("ALLGOODS");

			if(type != 0 &&
				((CurGoods->ItemDiscountsDesc.size()<type)||
					(DiscountType.size()<type)||
					(
					(DiscountType[type-1]!= '.')
					&&
					(DiscountType[type-1]!= '0')
					&&
					(DiscountType[type-1]!= '1')
					&&
					(DiscountType[type-1]!= '2')
					&&
					(DiscountType[type-1]!= '3')
					&&
					(DiscountType[type-1]!= '4')
					&&
					(DiscountType[type-1]!= '5')
					&&
					(DiscountType[type-1]!= '6')
					&&
					(DiscountType[type-1]!= '7')
					&&
					(DiscountType[type-1]!= '8')
					&&
					(DiscountType[type-1]!= '9')
					)
					||
					((CurGoods->ItemDiscountsDesc[type-1]=='Z')&&(mode!=2))
					||
					((CurGoods->ItemDiscountsDesc[type-1]!='.')
					&& (CurGoods->ItemDiscountsDesc[type-1]!='1')
					&& (CurGoods->ItemDiscountsDesc[type-1]!='2')
					&& (CurGoods->ItemDiscountsDesc[type-1]!='3')
					&& (CurGoods->ItemDiscountsDesc[type-1]!='4')
					&& (CurGoods->ItemDiscountsDesc[type-1]!='5')
					&& (CurGoods->ItemDiscountsDesc[type-1]!='6')
					&& (CurGoods->ItemDiscountsDesc[type-1]!='7')
					&& (CurGoods->ItemDiscountsDesc[type-1]!='8')
					&& (CurGoods->ItemDiscountsDesc[type-1]!='9')
					&&(allgoodssign==0))
				))
				continue;
			int h,m,s;
			string date;

			sscanf(DiscountsTable->GetField("STARTTIME").c_str(),"%d:%d:%d",&h,&m,&s);


			if ((Seconds<h*3600+m*60+s)&&(h*3600+m*60+s>0))//if current time is less than
				continue;				//start time go to next schema
			sscanf(DiscountsTable->GetField("FINISHTIME").c_str(),"%d:%d:%d",&h,&m,&s);

			if ((Seconds>h*3600+m*60+s)&&(h*3600+m*60+s>0))//if current time is more than
				continue;				//finish time go to next schema
			int t = xbDate(DiscountsTable->GetField("STARTDATE").c_str()).JulianDays();
			if (Days<xbDate(DiscountsTable->GetField("STARTDATE").c_str()).JulianDays())
				continue;				//analogously with date restrictions
			if (Days>xbDate(DiscountsTable->GetField("FINISHDATE").c_str()).JulianDays())
				continue;
			int DayOfWeek=DiscountsTable->GetLongField("DAYOFWEEK");
			if (DayOfWeek!=0)
			{
				if (DayOfWeek==7)//sunday is 7th day, but xBase assumes that sunday is 0
					DayOfWeek=0;
				if (DayOfWeek!=xbDate().DayOf(XB_FMT_WEEK))//if current day of week does not
					continue;	//coincide with specified day of week go to next schema
			}

			if (type==0)
			{
				// округление
				double tmpTrunc1 =0 ,tmpTrunc2 = 0;
				tmpTrunc1 = DiscountsTable->GetDoubleField("CHARGE");
				tmpTrunc2 = ProcessDiscountStr(DiscountsTable->GetField("FCHARGE"),CheckSum);

				FiscalCheque::trunc = tmpTrunc1>tmpTrunc2 ? tmpTrunc1 : tmpTrunc2;
				continue;
			}

			if (mode==2)
			{
				// Суммовая скидка на весь чек
				if (FiscalCheque::DiscountSumm < DiscountsTable->GetDoubleField("CHARGE"))
				{
					FiscalCheque::DiscountSumm = DiscountsTable->GetDoubleField("CHARGE");
				}

			    double tmpDiscountSumm = ProcessDiscountStr(DiscountsTable->GetField("FCHARGE"),CheckSum);

				if (FiscalCheque::DiscountSumm < tmpDiscountSumm)
				{
					FiscalCheque::DiscountSumm = tmpDiscountSumm;
				}

				continue;
			}

			if (mode==1)
			{
				// Суммовая скидка на весь чек за исключением акционных товаров
				if (FiscalCheque::DiscountSummNotAction < DiscountsTable->GetDoubleField("CHARGE"))
				{
					FiscalCheque::DiscountSummNotAction = DiscountsTable->GetDoubleField("CHARGE");
				}

			    double tmpDiscountSumm = ProcessDiscountStr(DiscountsTable->GetField("FCHARGE"),CheckSum);

				if (FiscalCheque::DiscountSummNotAction < tmpDiscountSumm)
				{
					FiscalCheque::DiscountSummNotAction = tmpDiscountSumm;
				}

				continue;
			}

			if (mode == LOCKDISCOUNTMODE)
			{
				// Запрет
				if (CurGoods->ItemFlag == SL) FiscalCheque::Lock = true;
				continue;
			}

//;;Log((char*)("!!! Code="+Int2Str(CurGoods->ItemCode)+", Count="+Double2Str(CurGoods->ItemCount)+", disc="+CurGoods->ItemDiscountsDesc).c_str());

// Акция СТОП_ЦЕНА цена устанавливается на количество кратное заданной сумме
// Например. 2 товара на каждые 200р (без учета товаров по этой акции) в чеке
// - проходят по акционной цене, все что больше - по обычной


			if (mode==6)
			{

//;;bool is991= false;

				//
				if (GoodsPriceTable->GetFirstRecord())
				{

					double S0=0;
					do
					{

//Log

//;;if (GoodsPriceTable->GetLongField("CODE")==991) is991 = true;
//
;;Log((char*)("### Code="+Int2Str(GoodsPriceTable->GetLongField("CODE"))+
". Q0="+ Double2Str(GoodsPriceTable->GetDoubleField("Q0"))+
". P0="+ Double2Str(GoodsPriceTable->GetDoubleField("P0"))+
". S0="+ Double2Str(GoodsPriceTable->GetDoubleField("S0"))
).c_str());


					    S0 = GoodsPriceTable->GetDoubleField("S0");

                        if(
                        (GoodsPriceTable->GetLongField("CODE")==CurGoods->ItemCode)
                        &&
                        (S0>0)
                        )
                        {

                            if((S0<=CheckSum_WithoutAction) || (NoCheckStopPriceSumma))
                            {
                                if (NoCheckStopPriceSumma)
                                    CurGoods->Action_ItemCnt = 1000000;
                                else
                                    // Сколько товаров может быть продано по акционной цене
                                    CurGoods->Action_ItemCnt=int(CheckSum_WithoutAction / S0) * GoodsPriceTable->GetDoubleField("Q0");

                                // Цена по акции
                                CurGoods->Action_ItemPrice=GoodsPriceTable->GetDoubleField("P0");

                                CurGoods->DiscExcessFlag = true;

//Log
;;Log((char*)("### ACTION ### Code="+Int2Str(CurGoods->ItemCode)+
". Action_ItemCnt="+ Double2Str(CurGoods->Action_ItemCnt)+
". Action_ItemPrice="+ Double2Str(CurGoods->Action_ItemPrice)+
". ( "+
"CheckSumWA="+ Double2Str(CheckSum_WithoutAction)+
". Q0="+ Double2Str(GoodsPriceTable->GetDoubleField("Q0"))+
". S0="+ Double2Str(GoodsPriceTable->GetDoubleField("S0"))+
" ) "
).c_str());

                            }
                        }

					}while(GoodsPriceTable->GetNextRecord());

//;;if(!is991) Log("!!!ERROR991!!!");

					//break;
				}
				continue;
			}



			if (mode==4)
			{
				// Использовать GoodsPrice как шкалу на превышение (как скидка)
				if (GoodsPriceTable->GetFirstRecord())
				{

				    bool ExistBarcode = false;

					double oldQ0=0;
					do
					{
					    bool isFind = false;
					    bool FindWithBarcode = false;

                       if (GoodsPriceTable->GetDoubleField("S0")>0) continue;

						if(GoodsPriceTable->GetLongField("CODE")==CurGoods->ItemCode)
						{
						    string lPriceBarcode = trim(GoodsPriceTable->GetField("BARCODE"));


//;;Log((char*)("*"+lPriceBarcode+"*").c_str());

//;;Log((char*)("****"+trim(CurGoods->ItemBarcode).substr(0,8)+"****").c_str());


                            if (
                            (lPriceBarcode==trim(CurGoods->ItemBarcode))
                            ||
                            (
                                (CurGoods->ItemPrecision==0.001)
                                &&
                                (lPriceBarcode==trim(CurGoods->ItemBarcode).substr(0,8))
                            )
                            )

                            {
                                FindWithBarcode = true;
                                isFind = true;
//;;Log((char*)(">"+trim(GoodsPriceTable->GetField("BARCODE"))+"<").c_str());
                            }
                            else
                            {
                                if ((trim(GoodsPriceTable->GetField("BARCODE"))=="") && (!ExistBarcode))
                                {
                                    isFind = true;
//Log("Empty");
                                }
                            }
                        }

						if(isFind)
						{
						    if (GoodsPriceTable->GetDoubleField("P0")==0)
                                CurGoods->ActionGoods = (int)min(-GoodsPriceTable->GetDoubleField("Q0"), GoodsPriceTable->GetDoubleField("Q0"));

							if(GoodsPriceTable->GetDoubleField("Q0")>oldQ0)
							{
								oldQ0 = GoodsPriceTable->GetDoubleField("Q0");
								if(CurGoods->ItemCount >= oldQ0)
								{
                                    if (GoodsPriceTable->GetDoubleField("P0")==0)
									{
                                        int CntAction = (int)CurGoods->ItemCount/(int)oldQ0;
                                        CurGoods->ActionGoods = CntAction;
                                        CurGoods->DiscExcessFlag = true;
                                    }
									else if ((CurGoods->ItemExcessPrice>GoodsPriceTable->GetDoubleField("P0"))
                                        || (FindWithBarcode))
										{
										    CurGoods->ItemExcessPrice=GoodsPriceTable->GetDoubleField("P0");
										    CurGoods->DiscExcessFlag = true;

										    ExistBarcode = ExistBarcode || FindWithBarcode;
										}

								}
							}
						}
					}while(GoodsPriceTable->GetNextRecord());
				}
				continue;
			}
			if (mode==5)
			{
				// Использовать GoodsPrice как шкалу на превышение (как FullPrice)
				if (GoodsPriceTable->GetFirstRecord())
				{
					double oldQ0=0;
					do
					{
					   if(GoodsPriceTable->GetDoubleField("S0")>0) continue;

						if(GoodsPriceTable->GetLongField("CODE")==CurGoods->ItemCode)
						{
							if(GoodsPriceTable->GetDoubleField("Q0")>oldQ0)
							{
								oldQ0 = GoodsPriceTable->GetDoubleField("Q0");
								if(CurGoods->ItemCount >= oldQ0)
								{
									if(CurGoods->ItemCalcPrice>GoodsPriceTable->GetDoubleField("P0"))
									{
										CurGoods->ItemCalcPrice=GoodsPriceTable->GetDoubleField("P0");
										CurGoods->DiscExcessFlag = true;
                                    }
								}
							}
						}
					}while(GoodsPriceTable->GetNextRecord());

					break;
				}
				continue;
			}


			double tmpPercent=0,tmpCharge=0;
			long tmpByPrice=0;


            if (mode==7)
            {
               tmpPercent=ProcessDiscountStrByNumber(DiscountsTable->GetField("FPERCENT"), DiscountType[type-1]);
               //;;Log("Res=");
               //;;Log((char*)Int2Str(tmpPercent).c_str());
            }
            else
            {
                tmpPercent=ProcessDiscountStr(DiscountsTable->GetField("FPERCENT"),CheckSum);
            }

			if (mode==0) tmpCharge=ProcessDiscountStr(DiscountsTable->GetField("FCHARGE"),CheckSum);

			if (tmpPercent>Percent)//Строка
				Percent=tmpPercent;
			if (tmpCharge>WholeCharge)
				WholeCharge=tmpCharge;

			tmpPercent=DiscountsTable->GetDoubleField("PERCENT");//check parameters and
			if (mode==0) tmpCharge=DiscountsTable->GetDoubleField("CHARGE");//recalculate goods price

			if (tmpPercent>Percent)//Число
				Percent=tmpPercent;
			if (tmpCharge>Charge)
				Charge=tmpCharge;

			tmpByPrice=DiscountsTable->GetLongField("BYPRICE");
			if (tmpByPrice>ByPrice)
				ByPrice=tmpByPrice;

			// + dms ===== 2015-08-06 =======
			// Режим использования фиксированной цены в зависимости от суммы чека
			if (mode==8)
			{
                double tmpIsOnPrice = ProcessDiscountStr(DiscountsTable->GetField("FCHARGE"),CheckSum_WithoutAction);

                ByPrice=tmpIsOnPrice?1:0;
			}
			// - dms ===== 2015-08-06 =======

		}while(DiscountsTable->GetNextRecord());

		DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
		return;
	}

	if(CurGoods->ItemCalcPrice>CurGoods->ItemFullPrice)
	{
		CurGoods->ItemCalcPrice=CurGoods->ItemFullPrice;




		CurGoods->ItemDiscountsDesc = "ZZZZZZZZZZZZZZZZZZZZ";





	}
	CurGoods->ItemPrice=CurGoods->ItemCalcPrice;
	CurGoods->CalcSum();

    // + dms ======================
    if ((Percent==0)&&(Charge==0))
    {
        CurGoods->ItemDiscount=CurGoods->ItemDSum=0;
    }
    // - dms

 	if (CheckSum>0)
		Charge+=RoundToNearest(WholeCharge*(CurGoods->ItemCalcSum/CheckSum)/CurGoods->ItemCount,0.01);
	if ((Percent>epsilon)&&(Percent<100))
		CurGoods->ItemDiscount=Percent/100;
	if ((Charge>epsilon)&&(Charge<=CurGoods->ItemCalcPrice))
	{
		CurGoods->ItemDSum=Charge;
	}
	if ((CurGoods->ItemDiscount>epsilon)&&(CurGoods->ItemDiscount<100))
	{
		double p = RoundToNearest(CurGoods->ItemCalcPrice*(1-CurGoods->ItemDiscount),0.01);
		if(CurGoods->ItemPrice > p)
			CurGoods->ItemPrice = p;
	}
	if(CurGoods->ItemExcessPrice<CurGoods->ItemPrice)
	{
		CurGoods->ItemPrice=CurGoods->ItemExcessPrice;
	}
	if (ByPrice==1 && (CurGoods->ItemPrice > CurGoods->ItemFixedPrice))
	{
        CurGoods->ItemDiscount=CurGoods->ItemDSum=0;
        CurGoods->ItemPrice=CurGoods->ItemFixedPrice;
	}
	if ((CurGoods->ItemDSum>epsilon)&&(CurGoods->ItemDSum<CurGoods->ItemCalcPrice))
	{
		CurGoods->ItemPrice=RoundToNearest(CurGoods->ItemCalcPrice-CurGoods->ItemDSum,0.01);
	};
	// + dms ===== 2014-03-20 =======  цена не может быть больше минимальной  ========
	if ((CurGoods->ItemMinimumPrice>epsilon) && (CurGoods->ItemPrice < CurGoods->ItemMinimumPrice))
	{
		CurGoods->ItemPrice=CurGoods->ItemMinimumPrice;
	}
	// - dms ===== 2014-03-20 =======  цена не может быть больше минимальной  ========

	CurGoods->CalcSum();
	CurGoods->CustomerCode[0]=CurrentCustomer;//set flags
	CurGoods->CustomerCode[1]=AddonCustomer;
	CurGoods->DiscFlag=true;
}

double posDBXBase::ProcessDiscountStrByNumber(string desc,char check_symbol)
{

//;;Log((char*)desc.c_str());

	if (!desc.empty())
	{
		vector<double> numbers;
		vector<char> sums;

		int prev_pos=0;
		for(int pos=0;pos<desc.size();pos++)
		{
			char buf[16];
			if (desc[pos]==':')
			{
				memset(buf,'\0',16);
				strncpy(buf,desc.c_str()+prev_pos,pos-prev_pos);
				prev_pos=pos+1;
				sums.insert(sums.end(),buf[0]);
			}

			if (desc[pos]==';')
			{
				memset(buf,'\0',16);
				strncpy(buf,desc.c_str()+prev_pos,pos-prev_pos);
				prev_pos=pos+1;
				numbers.insert(numbers.end(),atof(buf));
			}
		}

		if (sums.size()!=numbers.size())
			return 0;

		if (sums.empty())
			return 0;

		int min_pos=-1;
		double min_sum=-1;

		for (int i=0;i<sums.size();i++)
		{
//			if (sums[i]0)//((sums[i]<0)||(numbers[i]<0))

//				return 0;

			if (sums[i]==check_symbol)
			{
				min_pos=i;
				min_sum=sums[i];
			}
		}

		if (min_pos!=-1)
			return numbers[min_pos];
		else
			return 0;
	}
	else
		return 0;
}


double posDBXBase::ProcessDiscountStr(string desc,double check_sum)
{
	if (!desc.empty())
	{
		vector<double> sums,numbers;

		int prev_pos=0;
		for(int pos=0;pos<desc.size();pos++)
		{
			char buf[16];
			if (desc[pos]==':')
			{
				memset(buf,'\0',16);
				strncpy(buf,desc.c_str()+prev_pos,pos-prev_pos);
				prev_pos=pos+1;
				sums.insert(sums.end(),atof(buf));
			}
			if (desc[pos]==';')
			{
				memset(buf,'\0',16);
				strncpy(buf,desc.c_str()+prev_pos,pos-prev_pos);
				prev_pos=pos+1;
				numbers.insert(numbers.end(),atof(buf));
			}
		}

		if (sums.size()!=numbers.size())
			return 0;

		if (sums.empty())
			return 0;

		int min_pos=-1;
		double min_sum=-1;

		for (int i=0;i<sums.size();i++)
		{
			if (sums[i]<0)//((sums[i]<0)||(numbers[i]<0))

				return 0;

			if ((sums[i]<=check_sum)&&(sums[i]>min_sum))
			{
				min_pos=i;
				min_sum=sums[i];
			}
		}

		if (min_pos!=-1)
			return numbers[min_pos];
		else
			return 0;
	}
	else
		return 0;
}

int posDBXBase::FindCustomer(long long *res,long long CardNumber)
{

	try
	{
		DbfMutex->lock();
		if (CustomersTable->Locate("ID",CardNumber))
		{
			*res = CardNumber;
		}
		else
		{
			if (CustRangeTable->GetFirstRecord())
			{// ищем скидку в таблице диапазонов
				do{
					if(CustRangeTable->GetLLongField("IDFIRST") <= CardNumber &&
					CustRangeTable->GetLLongField("IDLAST") >= CardNumber)
					{
						*res=CardNumber;
						break;
					}
				}
				while(CustRangeTable->GetNextRecord());
			}
		}
		DbfMutex->unlock();

	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return 0;
}

bool posDBXBase::FindCardInRangeTable(CardInfoStruct *CardInfo,long long CardNumber)
{
    bool res = false;
	try
	{
		DbfMutex->lock();
		if (CustRangeTable->GetFirstRecord())
        {// ищем карту в таблице диапазонов
            do{
                if(CustRangeTable->GetLLongField("IDFIRST") <= CardNumber &&
                CustRangeTable->GetLLongField("IDLAST") >= CardNumber)
                {
                    CardInfo->CardNumber=CardNumber;
                    CardInfo->CardType=CustRangeTable->GetLLongField("CUSTTYPE");
                    CardInfo->CardSumma=CustRangeTable->GetDoubleField("SUMMA");
                    CardInfo->BonusCard=CustRangeTable->GetLLongField("BONUSCARD");
                    CardInfo->ParType=CustRangeTable->GetLLongField("PARTYPE");

                    CardInfo->isExpire = xbDate().JulianDays()>xbDate(CustRangeTable->GetField("EXPIRE").c_str()).JulianDays();

                    res = true;
                    break;
                }
            }
            while(CustRangeTable->GetNextRecord());
        }

		DbfMutex->unlock();

	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return res;
}


bool posDBXBase::FindCardInCustomerTable(CardInfoStruct *CardInfo,long long CardNumber)
{
    bool res = false;
	try
	{
		DbfMutex->lock();
		if (CustomersTable->Locate("ID",CardNumber))
		{
		    // если это бонусная карта, то отметим ее как бонусную
		    if (CustomersTable->GetLLongField("NEEDCERT"))
		    {
                //CardInfo->CardNumber=CardNumber;
                CardInfo->BonusCard=CustomersTable->GetLLongField("NEEDCERT");
		    }

            res = true;

		}
		DbfMutex->unlock();

	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return res;
}

int posDBXBase::FindStaff(int *res,string Barcode)
{
	try
	{
		DbfMutex->lock();
		if (StaffTable->Locate("SECURITY",Barcode.c_str()))
			if(StaffTable->GetStringField("SECURITY")==Barcode)
				*res=StaffTable->GetLongField("ID");
		DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return 0;
}

double GetDoubleNum(string strEditLine,bool* err)//auxililary function
{
	QString GoodsPrice(strEditLine.c_str());
	for (unsigned int i=0;i<GoodsPrice.length();i++)
		if (GoodsPrice.at(i)==',')
			GoodsPrice=GoodsPrice.replace(i,1,".");

	return GoodsPrice.toDouble(err);
}

int posDBXBase::LookingForGoods
(
	int Tag
	,string strEditLine
	,GoodsInfo *CurGoods
	,bool priceCheckerIsNull
	,QString GoodsCode
	,QString GoodsBarcode
	,QString GoodsName
	,QString GoodsPrice
	,bool LookInsideStatus
)
//in accordance with filter looking for barcode
{
//    ;;Log("+posDBXBase::LookingForGoods+");
	FiscalCheque FoundGoods;
	GoodsInfo tmpGoods;
	double Price=0;
	string Barcode;
	int Code=0;
	int Group=0;
	string KeyName;
	QString qstrEditLine(strEditLine.c_str());
	int lock_code = GL_NoLock;
    int BarcodeLength = 0;

	try
	{
		bool err, loc;
		double Mult=0;
		string tempLock ="";

		double tempPrice = 0;
//        string sig_actsiz = "";
        int cigarPrice = 0;

//        string milk_actsiz = "";

#ifdef ALCO_VES
        string AlcoVes_actsiz = "";
#endif

#ifndef BLOCK_CIGAR
        string blockCigar_actsiz = "";
#endif

		switch (Tag)
		{
		case PriceFilter:
			Price=GetDoubleNum(strEditLine,&err);//if we search by price then we have to check it

			if (!err)
			{
				return 1;
			}

			DbfMutex->lock();
			loc=GoodsTable->Locate("PRICE",Price);
			DbfMutex->unlock();

			if (!loc)//find the first goods with the given price
			{
				return 2;
			}

			KeyName="PRICE";
			CurGoods->InputType = IN_PRICE;
			lock_code = GL_FindByPrice;

		break;
		case GroupFilter:
			Group=qstrEditLine.toInt(&err);//if we search by group's code then we have to check it
			if ((Group<=0)||(!err))
			{
				return 3;
			}

			DbfMutex->lock();

			loc=GoodsTable->Locate("GRCODE",Group);
			DbfMutex->unlock();

			if (!loc)//find the first goods with the given group
			{
				return 4;
			}

			KeyName="GRCODE";
			CurGoods->InputType = IN_GR;
		break;

		case BarcodeFilter:
			Barcode=strEditLine;
///Хранится весь считанный код датаматрикс, без разделителей
            BarcodeLength = Barcode.length();
			;;Log("[FUNC] LookingForGoods [BarcodeFilter] = "+ Barcode + " Длина кода маркировки=" + Int2Str(Barcode.length()));
/// CIGAR BEGIN
            if (Barcode.length() == 29)
            {
//                sig_actsiz = Barcode;
//                ;;Log("[FUNC] LookingForGoods [SIG] "+ sig_actsiz);

                string MRCCode = Barcode.substr(21,4);
                cigarPrice = GetMRCFromCigarActsiz(MRCCode);

                Log("CIGAR MRC = "+Int2Str(cigarPrice));

                Barcode = Barcode.substr(0,14);
                int k = 0;
                while(k<Barcode.length())
                {
                    if (Barcode.substr(k,1) != "0") break;
                    k++;
                }
                if(k>0) Barcode = Barcode.substr(k,14);

            }
/// CIGAR END

/// MILK or WATER BEGIN
            else if (Barcode.length() == 30 || Barcode.length() == 37 || Barcode.length() == 32)
            {
//                milk_actsiz = Barcode;
                Barcode = Barcode.substr(3,13);
            }
///	MILK or WATER END

#ifdef BLOCK_CIGAR
            else if ((Barcode.length()==52) || (Barcode.length()==41) || Barcode.length()==55)
            {
//                Log("Блок сигарет вход");
//                milk_actsiz = Barcode;
                Barcode = Barcode.substr(3,13);
            }
#endif

#ifdef ALCO_VES
            else if (Barcode.length()==44)
            {
                Log("Пиво разлив вход");
                AlcoVes_actsiz = Barcode;
                Barcode = Barcode.substr(3,13);
            }
#endif

#ifdef PIVO_BOTTLE
            else if (Barcode.length()==31)
            {
                Log("Пиво в таре вход");
                Barcode = Barcode.substr(3,13);
            }
#endif

#ifdef BIODOBAVKA
            else if (Barcode.length()==83)
            {
                Log("Биодобавка вход");
                Barcode = Barcode.substr(3,13);
            }
#endif

            else{
                    ;;Log("EEE Barcode.length() = " + Int2Str(Barcode.length()));
            }
            // - MILK or WATER

            ;;Log("Searching BARCODE " + Barcode + "");

			DbfMutex->lock();

			loc = GoodsTable->Locate("BARCODE",Barcode.c_str());

			if(!loc)
			{
				Barcode = "0" + Barcode;
				loc = GoodsTable->Locate("BARCODE",(Barcode).c_str());
			}

			DbfMutex->unlock();

			if (!loc)//find the first goods with the given barcode
			{			
                return 5;
			}

			KeyName="BARCODE";
			CurGoods->InputType = IN_SCAN;

			lock_code = GL_FindByBarcode;


// + dms ===== 2014-03-11 ======== Спец. режим выбора товаров ===================
//  При считывании ШК, если у товара стоит метка-запрет № 15, то выводим все товары
// с данным КОДОМ независимо от штрихкода (с равным количеством)
// Это необходимо для выбора кассиром цены товара
            ;;Log("[FUNC] CASHREG LookingForGoods [4]");
            DbfMutex->lock();
            tempLock = GoodsTable->GetStringField("LOCK");
            tempPrice = GoodsTable->GetDoubleField("PRICE");
            DbfMutex->unlock();
            Log("Длина считанного кода=" + Int2Str(BarcodeLength));

            // + dms ===== 2020-07-01 С 1 июля 2020 запрещено продавать сигареты без акциза
            // С 2020-07-01 Действует Запрет продажи сигарет без акциза
            // Остальная табачная продукция с 2021-07-01 (через год)
            if (
                (GoodsLock(tempLock, GL_SpecBarcodefilter) == GL_SpecBarcodefilter)
                &&
                (GoodsLock(tempLock, GL_CigarLock) == GL_CigarLock)
                &&
                ((BarcodeLength != 29) && (BarcodeLength != 52)
                 && (BarcodeLength != 41) && (BarcodeLength != 55) )
                ) {

#ifdef BLOCK_CIGAR
                Log("ZAPRET PRODAGI CIGARET");
#endif
                    return 13;

                }

            // ===============

            if (
                (GoodsLock(tempLock, GL_SpecBarcodefilter) == GL_SpecBarcodefilter)
                &&
                !(
                    (fabs(cigarPrice) > 0.01)
                    &&
                    (fabs(tempPrice*100 - cigarPrice) < 0.01)
                )
            )
            {

#ifndef BLOCK_CIGAR
                Log("ZAPRET PRODAGI CIGARET____1");
#endif

                DbfMutex->lock();
                Code        = GoodsTable->GetLongField("CODE");
                Mult        = GoodsTable->GetDoubleField("MULT");

                loc=GoodsTable->Locate("CODE",Code);
                DbfMutex->unlock();

                if (!loc)//find the first goods with the given group
                {
                    return 7;
                }

                Tag=SpecBarcodeFilter;
                KeyName="CODE";
#ifdef BLOCK_CIGAR
                Log("end block cigarete=====Exit");
#endif
            }
// - dms ===== 2014-03-11 ======== Спец. режим выбора товаров ===================
		break;

		case CodeFilter:
			Code=qstrEditLine.toInt(&err);//if we search by group's code then we have to check it
			if ((Code<=0)||(!err))
			{
				return 6;
			}

			DbfMutex->lock();

			loc=GoodsTable->Locate("CODE",Code);
			DbfMutex->unlock();

			if (!loc)//find the first goods with the given group
			{
				return 7;
			}

			KeyName="CODE";
			CurGoods->InputType = IN_CODE;
			lock_code = GL_FindByCode;
		break;
		case PropertiesFilter:
			CurGoods->InputType = IN_NAME;
            if (GoodsCode.toInt()>0) lock_code = GL_FindByCode;
            else if (!GoodsName.isEmpty()) lock_code = GL_FindByName;
               else if (GoodsPrice.toDouble()>0) lock_code = GL_FindByPrice;

		break;
		}

		DbfMutex->lock();
		FoundGoods.Clear();

		bool GoNext;

		if(Tag==PropertiesFilter)
		{
			GoNext=GoodsTable->GetFirstRecord();
		}
		else
		{
			GoNext=TRUE;//GoodsTable->GetFirstKey(KeyName.c_str());
		}

		if (GoNext)
		do   //load all goods that satisfy filter's condition
		{

            //Log("tmpGoods.itemPivoRazliv____Test");

			tmpGoods.ItemPrice	= GoodsTable->GetDoubleField("PRICE");
			tmpGoods.ItemCode	= GoodsTable->GetLongField("CODE");
			tmpGoods.ItemGroup	= GoodsTable->GetLongField("GRCODE");
			tmpGoods.ItemName	= GoodsTable->GetStringField("NAME");
			tmpGoods.ItemBarcode = GoodsTable->GetStringField("BARCODE");

			tmpGoods.ItemDBPos	= GoodsTable->GetRecNo();
			if(GoodsTable->GetFieldNo("NDS") != -1){
				tmpGoods.NDS = GoodsTable->GetDoubleField("NDS");
			}
			tmpGoods.ItemMult = GoodsTable->GetDoubleField("MULT");

            tmpGoods.Lock	  = GoodsTable->GetStringField("LOCK");

            tmpGoods.LockCode = GoodsLock(tmpGoods.Lock, GL_TimeLock);

            if (tmpGoods.LockCode != GL_TimeLock)
                tmpGoods.LockCode = GoodsLock(tmpGoods.Lock, lock_code);


			if(Tag==PropertiesFilter)
			{
				GoNext=GoodsTable->GetNextRecord();
			}
			else
			{
				GoNext=GoodsTable->GetNextKey(KeyName.c_str());
			}

// + dms ===== 2014-03-11 ======== Спец. режим выбора товаров
//  При считывании ШК, если у товара стоит метка-запрет № 15, то выводим все товары
// с данным КОДОМ независимо от штрихкода (с равным количеством)
// Это необходимо для выбора кассиром цены товара

			if (Tag==SpecBarcodeFilter)
			{
			    if (tmpGoods.ItemCode!=Code)
                    break;

                if (tmpGoods.ItemMult!=Mult)
                {
                    continue;
                }
			}

// - dms ===== 2014-03-11 ========

			if ((Tag==BarcodeFilter)&&(tmpGoods.ItemBarcode!=Barcode))
				break;
			if ((Tag==PriceFilter)&&(tmpGoods.ItemPrice!=Price))
				break;
			if ((Tag==GroupFilter)&&(tmpGoods.ItemGroup!=Group))
				break;
			if ((Tag==CodeFilter)&&(tmpGoods.ItemCode!=Code))
				break;
			if(Tag==PropertiesFilter)
			{
				if ((GoodsCode.toInt()>0)&&(GoodsCode.toInt()!=tmpGoods.ItemCode))
					continue;

				if ((!GoodsBarcode.isEmpty())&&
					(GoodsBarcode!=tmpGoods.ItemBarcode.c_str()))
					continue;

				if ((GoodsPrice.toDouble()>0)&&
					(fabs(GoodsPrice.toDouble()-tmpGoods.ItemPrice)>epsilon))
					continue;

				if (!GoodsName.isEmpty())
				{
					int index;

					if (LookInsideStatus)
						index=W2U(tmpGoods.ItemName).find(GoodsName,0,false);
					else
						if (W2U(tmpGoods.ItemName).left(GoodsName.length()).upper()==GoodsName.upper())
							index=0;
						else
							index=-1;

					if (index==-1)
						continue;
				}
			}

// + dms ===== 2014-03-11 ======== Спец. режим выбора товаров
//  При считывании ШК, если у товара стоит метка-запрет № 15, то выводим все товары
// с данным КОДОМ независимо от штрихкода (с равным количеством)
// Это необходимо для выбора кассиром цены товара

// Поправка: выводим только первые 3 позиции с максимальной ценой

			if (Tag==SpecBarcodeFilter)
			{

                int cnt_price=0; // тек. количество
                int max_cnt = 3; // Не больше 3х строк
                double min_price=9999999; // минимальная цена
                int min_pos=-1;  // позиция с минимальной ценой, кот. нужно заменить


                for (int j=0;j<FoundGoods.GetCount();j++)
                {
                    GoodsInfo g = FoundGoods[j];

                    if (g.ItemPrice == tmpGoods.ItemPrice)
                    {
                        // есть уже такая цена, пропускаем
                        cnt_price=max_cnt;
                        break;
                    }
                    else
                    {
                        if (g.ItemPrice < tmpGoods.ItemPrice)
                        {
                            // нашли с меньшей ценой
                            if (min_price>g.ItemPrice)
                            {
                                min_price=g.ItemPrice;
                                min_pos = j;
                            }
                        }
                        else
                        {
                            // Кол-во товаров с большей ценой
                            // не должно превышать заданное число (3 поз.)
                            cnt_price++;
                        }
                    }
                }

                if (cnt_price>=max_cnt) continue;

                if ((FoundGoods.GetCount()>=max_cnt) && (min_pos>=0) && (min_pos<FoundGoods.GetCount()))
                { // нашли с меньшей ценой - заменим и выйдем
                    FoundGoods.SetAt(min_pos,tmpGoods);
                    continue;
                }

			}
// - dms ===== 2014-03-11 ======== Спец. режим выбора товаров


		    FoundGoods.Insert(tmpGoods);

		}
		while(GoNext);

		DbfMutex->unlock();

		if (FoundGoods.GetSize()==0)
		{
			return 8;
		}

		if (FoundGoods.GetSize()==1)   //if we have found unique goods
		{

			DbfMutex->lock();
			if(GoodsTable->GetRecNo() != FoundGoods[0].ItemDBPos)
			{
				GoodsTable->SetRecNo(FoundGoods[0].ItemDBPos);
			}
			string StrLock = GoodsTable->GetStringField("LOCK");

			DbfMutex->unlock();
			CurGoods->ItemCount = 1;

            if (GoodsLock(StrLock, GL_TimeLock)==GL_TimeLock) return GL_TimeLock;

            if (lock_code!=GL_NoLock) // надо проверить на запрет
            {
                CurGoods->LockCode = GoodsLock(StrLock, lock_code);
                if (CurGoods->LockCode!=GL_NoLock) return CurGoods->LockCode;
            }

			if (AddGoods(CurGoods,0)==0)
			{
				return 0;
			}
			else
				return 9;
		}

		if (FoundGoods.GetSize()>1 && priceCheckerIsNull)//select goods from the list
		{

			int SelPos;
			PriceSelForm *dlg = new PriceSelForm(NULL);
			dlg->SelRow=&SelPos;
			dlg->ShowGoodsList(&FoundGoods);

			int res=dlg->exec();
			delete dlg;
			if (res==QDialog::Accepted)
			{
				DbfMutex->lock();
				if(GoodsTable->GetRecNo() != FoundGoods[SelPos].ItemDBPos)
				{
					GoodsTable->SetRecNo(FoundGoods[SelPos].ItemDBPos);
				}
				CurGoods->ItemCount = 1;//CashReg->GoodsTable->GetLogicalField("TYPE") ? 1 : 0;


				DbfMutex->unlock();
				if (AddGoods(CurGoods,0)==0)
				{
					return 0;
				}
				else
				{
					return 10;
				}
			}
			else {
			    if (Tag==SpecBarcodeFilter) {
                    return 11;
                } else {
                    return 12;
                }
			}
		}
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return 12;
}


int posDBXBase::AddPayment(int Type,string Number,double *Summa)
{
    ;;Log("+AddPayment+");
	try
	{
		DbfMutex->lock();
        // добавим запись в оплаты
		CrPaymentTable->BlankRecord();
        CrPaymentTable->PutLongField("TYPE",Type);
        CrPaymentTable->PutField("NUMBER",Number.c_str());
        CrPaymentTable->PutDoubleField("SUMMA",*Summa);
        CrPaymentTable->AppendRecord();

        char tmplog[1024];
        switch (Type) {
            case 11: sprintf(tmplog, "Карта \"Спасибо\" Сбербанк № %s (%.2f) добавлена в таблицу оплат (Type = %d)", Number.c_str(), *Summa, Type); break;
            case 12: sprintf(tmplog, "Карта № %s (%.2f) добавлена в таблицу оплат (Type = %d)", Number.c_str(), *Summa, Type); break;
            case 10: sprintf(tmplog, "Сертификат № %s (%.2f) добавлен в таблицу оплат (Type = %d)", Number.c_str(), *Summa, Type); break;
            case 15: sprintf(tmplog, "Сертификат (Аванс) № %s (%.2f) добавлен в таблицу оплат (Type = %d)", Number.c_str(), *Summa, Type); break;
        }
        Log(tmplog);

		DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
		return -1;
	}
    ;;Log("-AddPayment-");
	return 0;
}

int posDBXBase::CheckPayment(int Type,string Number,string DiscountStr,double *Summa,double CheckSum)
{
	DbfMutex->lock();
	int RecordsCount=CrPaymentTable->RecordCount();
	DbfMutex->unlock();

	if (RecordsCount>0)
	{
		try
		{
			DbfMutex->lock();
			CrPaymentTable->GetFirstRecord();
			DbfMutex->unlock();

			DbfMutex->lock();
			do
			{
				int    crType = CrPaymentTable->GetLongField("TYPE");
				string crNum  = CrPaymentTable->GetStringField("NUMBER");

				if ((crNum == Number) && (crType==Type) && (!CrPaymentTable->RecordDeleted()))
				{
					DbfMutex->unlock();
					return 1;//ФБЛПК УЕТФЙЖЙЛБФ ХЦЕ ЕУФШ
				}
			}
			while(CrPaymentTable->GetNextRecord());

			DbfMutex->unlock();
		}
		catch(cash_error& er)
		{
			DbfMutex->unlock();
			LoggingError(er);
			return -1;
		}
	};

    bool IsCert = false;
	try
	{
		//ДПВБЧМСЕН ЪБРЙУШ Ч ФБВМЙГХ
		//Log("Добавляем запись в таблицу оплат");
		DbfMutex->lock();

		//ЙЭЕН УХННХ УЕТФЙЖЙЛБФБ
		DiscountsTable->GetFirstRecord();
		time_t cur_time=time(NULL);
		tm *l_time=localtime(&cur_time);
		int Seconds=l_time->tm_hour*3600+l_time->tm_min*60+l_time->tm_sec;
		int Days=xbDate().JulianDays();

		double Charge=0,WholeCharge=0;

		do
		{
			int type=DiscountsTable->GetLongField("DISCTYPE");
			int mode=DiscountsTable->GetLongField("MODE");

			if(mode != 3) continue;

			if(
				(DiscountStr.size()<type)
				||
				(DiscountStr[type-1]!='.')
			)
				continue;

			int h,m,s;
			string date;

			sscanf(DiscountsTable->GetField("STARTTIME").c_str(),"%d:%d:%d",&h,&m,&s);


			if ((Seconds<h*3600+m*60+s)&&(h*3600+m*60+s>0))//if current time is less than
				continue;				//start time go to next schema
			sscanf(DiscountsTable->GetField("FINISHTIME").c_str(),"%d:%d:%d",&h,&m,&s);

			if ((Seconds>h*3600+m*60+s)&&(h*3600+m*60+s>0))//if current time is more than
				continue;				//finish time go to next schema
			int t = xbDate(DiscountsTable->GetField("STARTDATE").c_str()).JulianDays();
			if (Days<xbDate(DiscountsTable->GetField("STARTDATE").c_str()).JulianDays())
				continue;				//analogously with date restrictions
			if (Days>xbDate(DiscountsTable->GetField("FINISHDATE").c_str()).JulianDays())
				continue;
			int DayOfWeek=DiscountsTable->GetLongField("DAYOFWEEK");
			if (DayOfWeek!=0)
			{
				if (DayOfWeek==7)//sunday is 7th day, but xBase assumes that sunday is 0
					DayOfWeek=0;
				if (DayOfWeek!=xbDate().DayOf(XB_FMT_WEEK))//if current day of week does not
					continue;	//coincide with specified day of week go to next schema
			}

			double tmpCharge;
            IsCert = true; // это сертификат... хороший сертификат

			if (mode==3) tmpCharge=ProcessDiscountStr(DiscountsTable->GetField("FCHARGE"),CheckSum);

			if (tmpCharge>WholeCharge)
				WholeCharge=tmpCharge;

			if (mode==3) tmpCharge=DiscountsTable->GetDoubleField("CHARGE");//recalculate goods price
			if (tmpCharge>Charge)
				Charge=tmpCharge;
		}while(DiscountsTable->GetNextRecord());
		*Summa=Charge;

        if (IsCert) // Если это сертификат, то добавим запись в оплаты
		{
//		    CrPaymentTable->BlankRecord();
//            CrPaymentTable->PutLongField("TYPE",Type);
//            CrPaymentTable->PutField("NUMBER",Number.c_str());
//            CrPaymentTable->PutDoubleField("SUMMA",*Summa);
//            CrPaymentTable->AppendRecord();
//            Log("Сертификат добавлен в таблицу оплат");
		}
		else
		{
		    DbfMutex->unlock();
		    return 2; // че то не то считали
        }

		DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
		return -1;
	}

	return 0;
}

int posDBXBase::LoadPayment(vector<PaymentInfo> *Payment)
{
    int RecordsCount;

	DbfMutex->lock();

	RecordsCount=CrPaymentTable->RecordCount();

	DbfMutex->unlock();


	if (RecordsCount>0)
	{

		try
		{

			DbfMutex->lock();
			CrPaymentTable->GetFirstRecord();
			DbfMutex->unlock();

			DbfMutex->lock();
			do
			{

				if(!CrPaymentTable->RecordDeleted())
				{

					PaymentInfo  info;
					info.Type   = CrPaymentTable->GetLongField("TYPE");
					info.Number = CrPaymentTable->GetStringField("NUMBER");
					info.Summa  = CrPaymentTable->GetDoubleField("SUMMA");
					Payment->push_back(info);

				}
			}
			while(CrPaymentTable->GetNextRecord());

			DbfMutex->unlock();
		}
		catch(cash_error& er)
		{
			DbfMutex->unlock();
			LoggingError(er);
			return -1;
		}
	}

	return 0;
}

int posDBXBase::ProcessPayment()
{
	FiscalCheque::CertificateSumm = 0;
    FiscalCheque::PrePayCertificateSumm = 0;
    FiscalCheque::CertificateCount = 0;
	FiscalCheque::CertificateString = "";

    FiscalCheque::BonusSumm = 0;

	FiscalCheque::BankBonusSumm = 0;
	FiscalCheque::BankBonusString = "";

	DbfMutex->lock();
	int RecordsCount=CrPaymentTable->RecordCount();
	DbfMutex->unlock();

	if (RecordsCount>0)
	{
		try
		{
			DbfMutex->lock();
			CrPaymentTable->GetFirstRecord();
			DbfMutex->unlock();

			DbfMutex->lock();
			do
			{
				int    crType = CrPaymentTable->GetLongField("TYPE");

				if (!CrPaymentTable->RecordDeleted())
				{
				    switch(crType) {
                        case 10: // сертификаты
                            {
                                FiscalCheque::CertificateSumm += CrPaymentTable->GetLongField("SUMMA");

                                string curNumber =CrPaymentTable->GetStringField("NUMBER");

                                if (FiscalCheque::CertificateString!="")
                                    FiscalCheque::CertificateString += ", ";

                                FiscalCheque::CertificateString += curNumber.substr(6,12);

                                FiscalCheque::CertificateCount++;

                                break;
                            }
                        case 11: // бонусы программы "Спасибо" Сбербанка
                            {

                                FiscalCheque::BankBonusSumm += CrPaymentTable->GetDoubleField("SUMMA");

                                string curNumber =CrPaymentTable->GetStringField("NUMBER");

    //                          if (FiscalCheque::BankBonusString!="")
    //                                FiscalCheque::BankBonusString += ", ";

                                int len = curNumber.length();
                                int pos=len-4;
                                int n = 4;
                                if (pos < 0)
                                {
                                    pos=0;
                                    n= len;
                                }

                                //FiscalCheque::BankBonusString += "***"+curNumber.substr(pos,n);

                                break;

                            }
                        case 12:  // бонусная программы Гастроном
                            {

                                FiscalCheque::BonusSumm += CrPaymentTable->GetDoubleField("SUMMA");

                                string curNumber =CrPaymentTable->GetStringField("NUMBER");

    //                          if (FiscalCheque::BankBonusString!="")
    //                                FiscalCheque::BankBonusString += ", ";

                                //FiscalCheque::BankBonusString += "***"+curNumber.substr(pos,n);

                                break;

                            }
                        case 15: // предоплата сертификаты (аванс)
                        {
                            FiscalCheque::PrePayCertificateSumm += CrPaymentTable->GetLongField("SUMMA");

                            string curNumber =CrPaymentTable->GetStringField("NUMBER");

                            if (FiscalCheque::CertificateString!="")
                                FiscalCheque::CertificateString += ", ";

                            FiscalCheque::CertificateString += "*" + curNumber.substr(6,12);

                            FiscalCheque::CertificateCount++;

                            break;
                        }

				    }
				}
			}
			while(CrPaymentTable->GetNextRecord());

			DbfMutex->unlock();

			if (FiscalCheque::CertificateCount > 0) {
			    FiscalCheque::CertificateString = Int2Str(FiscalCheque::CertificateCount)+" шт.";
			}


		}
		catch(cash_error& er)
		{
			DbfMutex->unlock();
			LoggingError(er);
			return -1;
		}
	};

	return 0;
}

int posDBXBase::ClearPayment()
{
	FiscalCheque::CertificateSumm = 0;
	FiscalCheque::PrePayCertificateSumm = 0;
	FiscalCheque::CertificateCount = 0;
	FiscalCheque::CertificateString = "";
	try
	{
		DbfMutex->lock();
	    	CrPaymentTable->Zap();
		DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
		return -1;
	}
	return 0;
}

int posDBXBase::DeletePayment(int Type,string Number, bool all)
{
	DbfMutex->lock();
	int RecordsCount=CrPaymentTable->RecordCount();
	DbfMutex->unlock();

	if (RecordsCount>0)
	{
		try
		{
			DbfMutex->lock();
			if (CrPaymentTable->GetFirstRecord())
			{
				do
				{
					int    crType = CrPaymentTable->GetLongField("TYPE");
					string crNum  = CrPaymentTable->GetStringField("NUMBER");

					if (((crNum == Number)||all) && (crType==Type) && (!CrPaymentTable->RecordDeleted()))
					{
						CrPaymentTable->DeleteRecord();
						string tmpstr = "Удалена запись из таблицы оплат "+Number;
						Log((char*)tmpstr.c_str());
					}
				}
				while(CrPaymentTable->GetNextRecord());
			}
			DbfMutex->unlock();
		}
		catch(cash_error& er)
		{
			DbfMutex->unlock();
			LoggingError(er);
			return -1;
		}
	};
}

int posDBXBase::SavePayment(vector<PaymentInfo> Payment,int checknum,int KKMNo,int KKMSmena,time_t KKMTime, int cursaleman, long long idcust)
{
	int err;

	err = SavePaymentToTable(PaymentTable,Payment,checknum,KKMNo,KKMSmena,KKMTime,cursaleman,idcust);
	if (err) return err;

	err = SavePaymentToTable(PaymentToSendTable,Payment,checknum,KKMNo,KKMSmena,KKMTime,cursaleman,idcust);
	if (err) return err;

	return 0;
}

int posDBXBase::SavePaymentToTable(DbfTable *tbl,vector<PaymentInfo> Payment,int checknum,int KKMNo,int KKMSmena,time_t KKMTime, int cursaleman, long long idcust)
{
	try
	{
		DbfMutex->lock();
		for(unsigned int i=0;i<Payment.size();i++)
		{
			tbl->BlankRecord();
			tbl->PutLongField("TYPE",Payment[i].Type);
			tbl->PutField("NUMBER",Payment[i].Number.c_str());
			tbl->PutDoubleField("SUMMA",Payment[i].Summa);
			tbl->PutLongField("CHECKNUM",checknum);
			tbl->PutField("DATE",xbDate().GetDate().c_str());
			tbl->PutField("TIME",GetCurTime().c_str());
            tbl->PutLongField("CASHMAN",cursaleman);
            tbl->PutLLongField("CUSTOMER",idcust);
//	{"CUSTOMER",XB_NUMERIC_FLD,10,0},//code of the customer with discount card
//	{"INPUTKIND",XB_NUMERIC_FLD,3,0},//code of the customer with discount card
			if(KKMNo>0)
			tbl->PutLongField("KKMNO",KKMNo);
			if(KKMSmena>0)
			tbl->PutLongField("KKMSMENA",KKMSmena);
			if(KKMTime>0)
			tbl->PutLongField("KKMTime",KKMTime);

			tbl->AppendRecord();
		}
		DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
		return -1;
	}

	return 0;
}

int posDBXBase::CreatePrintFile(const char* fn, int kolstr)
{
    string str;
	FILE * f = fopen( fn, "w");
	if(!f)
		return 0;

	try
	{

        DbfMutex->lock();

        int kol = BankActreportTable->RecordCount();
        int n=0;
        if(BankActreportTable->GetFirstRecord())
        {
            do
            {
                if (kol-(++n)<=kolstr)
                {
                    str = BankActreportTable->GetStringField("TEXT");
                    fprintf(f,"%s\n", str.c_str());
                }
            }
            while(BankActreportTable->GetNextRecord());
        }
		DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	fclose(f);
    return 1;
}

string posDBXBase::GetCustomerInfo(long long crCustomer)
{
	string info = "";
	string info_date = "";
	//"Информация о покупателе\n";
	if (crCustomer==0) return info;

	try
	{
		DbfMutex->lock();
		if(CustomersValueTable->GetFirstRecord())
		{
			do
			{
				if(CustomersValueTable->GetLLongField("IDCUST")==crCustomer)
				{
					info_date+=(CustomersValueTable->GetStringField("DATESALE")).substr(6,2);
					info_date+=".";
					info_date+=(CustomersValueTable->GetStringField("DATESALE")).substr(4,2);
					info_date+=".";
					info_date+=(CustomersValueTable->GetStringField("DATESALE")).substr(0,4);
					info_date+=" ";
					info_date+=CustomersValueTable->GetStringField("TIMESALE");

					info+=GetFormatString("Последний чек", info_date);
					info+="\n"+GetFormatString("Сумма последнего чека", trim(CustomersValueTable->GetStringField("LASTSUMMA")));

//+ dms ==== 2017-01-26 ======
//					info+="Общее количество покупок    ";
//					info+=CustomersValueTable->GetStringField("COUNTSALE");
//					info+="\n";
//- dms ==== 2017-01-26 ======

                    info+="\n"+GetFormatString("ВЫ СЭКОНОМИЛИ", CustomersValueTable->GetStringField("DISCSUMMA"));
                    info+="\n"+GetFormatString("ВЫ НАКОПИЛИ", CustomersValueTable->GetStringField("SUMMA"));

					DbfMutex->unlock();
					return info;
				}
			}
			while(CustomersValueTable->GetNextRecord());
		}
		DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return info;
}

bool posDBXBase::CompareKey(int Key,string Pattern,string *ptrKeyName)//try to understand the type of the operation
{
	DbfMutex->lock();
	bool loc=KeyboardTable->Locate("KEYCODE",Key);
	DbfMutex->unlock();

	if (loc)
	{
		DbfMutex->lock();
		string KeyName=KeyboardTable->GetStringField("NAME").c_str();
		if(ptrKeyName)
		{
			*ptrKeyName=KeyName;
		}
		DbfMutex->unlock();
		return (KeyName.substr(0,Pattern.length())==Pattern);
	}

	return false;
}
bool posDBXBase::GetGroup(vector<int> *GrCodes,	vector<string> *S_Names)
{
	GrCodes->empty();
	S_Names->empty();

	DbfMutex->lock();

	if (GroupsTable->GetFirstRecord())
	do
	{
		GrCodes->insert(GrCodes->end(),GroupsTable->GetLongField("GRCODE"));
		S_Names->insert(S_Names->end(),GroupsTable->GetStringField("S_NAME"));
	}
	while(GroupsTable->GetNextRecord());

	DbfMutex->unlock();

	return true;
}

bool posDBXBase::GoodInAction(GoodsInfo* Good)
{
    bool result=false;
    DbfMutex->lock();
    if (GoodsPriceTable->GetFirstRecord())
    {
		do
		{
            if(GoodsPriceTable->GetLongField("CODE")==Good->ItemCode)
			    if (GoodsPriceTable->GetDoubleField("P0")==0)
			    {
			        result = true;
			        break;
                };

		}while(GoodsPriceTable->GetNextRecord());
    }

    DbfMutex->unlock();

    return result;
}



bool posDBXBase::CheckTimeLock()
{
	DbfMutex->lock();
	int RecordsCount=DiscountsTable->RecordCount();
	DbfMutex->unlock();

	if (RecordsCount==0)
		return false;

	time_t cur_time=time(NULL);
	tm *l_time=localtime(&cur_time);
	int Seconds=l_time->tm_hour*3600+l_time->tm_min*60+l_time->tm_sec;
	int Days=xbDate().JulianDays();

	int i = 0;

    bool result = false;

	try
	{
		DbfMutex->lock();
		DiscountsTable->GetFirstRecord();
		DbfMutex->unlock();

		DbfMutex->lock();
		do
		{
			int type=DiscountsTable->GetLongField("DISCTYPE");
			int mode=DiscountsTable->GetLongField("MODE");

			if(type != 0 && mode == LOCKDISCOUNTMODE)
			{
                    int h,m,s;
                    string date;

                    sscanf(DiscountsTable->GetField("STARTTIME").c_str(),"%d:%d:%d",&h,&m,&s);

                    if ((Seconds<h*3600+m*60+s)&&(h*3600+m*60+s>0))//if current time is less than
                        continue;				//start time go to next schema
                    sscanf(DiscountsTable->GetField("FINISHTIME").c_str(),"%d:%d:%d",&h,&m,&s);

                    if ((Seconds>h*3600+m*60+s)&&(h*3600+m*60+s>0))//if current time is more than
                        continue;				//finish time go to next schema
                    int t = xbDate(DiscountsTable->GetField("STARTDATE").c_str()).JulianDays();
                    if (Days<xbDate(DiscountsTable->GetField("STARTDATE").c_str()).JulianDays())
                        continue;				//analogously with date restrictions
                    if (Days>xbDate(DiscountsTable->GetField("FINISHDATE").c_str()).JulianDays())
                        continue;
                    int DayOfWeek=DiscountsTable->GetLongField("DAYOFWEEK");
                    if (DayOfWeek!=0)
                    {
                        if (DayOfWeek==7)//sunday is 7th day, but xBase assumes that sunday is 0
                            DayOfWeek=0;
                        if (DayOfWeek!=xbDate().DayOf(XB_FMT_WEEK))//if current day of week does not
                            continue;	//coincide with specified day of week go to next schema
                    }

                    result = true;

             }
		}while(!result && DiscountsTable->GetNextRecord());

		DbfMutex->unlock();

	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return result;
}

//проверяет коды запрета товара
int posDBXBase::GoodsLock(string StrLock, int LockCode)
{
    if (LockCode == GL_NoLock)
    {   // проверить все коды запрета и вернуть первый

        // первый код обрабатывается в особом порядке
        if ((StrLock.size()>0) && (StrLock[0]=='.'))
        {
            if (CheckTimeLock()) return GL_TimeLock;
        }

        for (int i=1; i<StrLock.size(); i++)
        {
            if (StrLock[i]=='.') return GL_NoLock + (i+1);
        }
    }
    else
    { // проверить установлен ли конкретный код - LockCode
        if (LockCode == GL_TimeLock)
        {
            // первый код обрабатывается в особом порядке
            if ((StrLock.size()>0) && (StrLock[0]=='.'))
            {
                if (CheckTimeLock()) return GL_TimeLock;
            }
        }
        else
        {   // проверить установлен ли конкретный код - LockCode
            int code = LockCode - GL_NoLock;
            if ((StrLock.size()>=code) && (StrLock[code-1]=='.')) return LockCode;
        }
    }

    return GL_NoLock;
}

// проверяет штрих-код на принадлежность карте сотрудника
bool posDBXBase::isGuard(string barcode)
{
    bool res = false;
    try
    {
		DbfMutex->lock();
		res = StaffTable->Locate("SECURITY",barcode.c_str());
        DbfMutex->unlock();
    }
    catch(cash_error &ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
        res = false;
    }

    return res;

}

bool posDBXBase::IsFlyer(long long CardNumber)
{
    bool Result = false;
	try
	{
		DbfMutex->lock();

        if (CustRangeTable->GetFirstRecord())
        {// ищем скидку в таблице диапазонов
            do{
                if(CustRangeTable->GetLLongField("IDFIRST") <= CardNumber &&
                CustRangeTable->GetLLongField("IDLAST") >= CardNumber)
                {
                    Result = CustRangeTable->GetLongField("CUSTTYPE")==20; //
                    break;
                }
            }
            while(CustRangeTable->GetNextRecord());
        }

		DbfMutex->unlock();

	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return Result;
}

bool posDBXBase::IsDobryeSosedy(long long CardNumber)
{
    bool Result = false;
	try
	{
		DbfMutex->lock();

        if (CustRangeTable->GetFirstRecord())
        {// ищем скидку в таблице диапазонов
            do{
                if(CustRangeTable->GetLLongField("IDFIRST") <= CardNumber &&
                CustRangeTable->GetLLongField("IDLAST") >= CardNumber)
                {
                    Result = CustRangeTable->GetLongField("CUSTTYPE")==4; //
                    break;
                }
            }
            while(CustRangeTable->GetNextRecord());
        }

		DbfMutex->unlock();

	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return Result;
}

int posDBXBase::ProcessBankDiscounts(const char *card,string *disc,long *idcust)
{
    int Res=-1;
	try
	{
	    string s_card = card;

		DbfMutex->lock();
        if (BankDiscountTable->GetFirstRecord())
        {// ищем карту в таблице диапазонов
            do{

                string t_card = s_card.substr(0, Int2Str(BankDiscountTable->GetLongField("BANKID")).length());

//+ dms === 2014-06-19 ==== Новые терминалы: у них номер забивается звездочкой
                for (unsigned int i=0;i<t_card.length();i++)
                {
                        if (t_card[i]=='*')
                            t_card[i]='0';
                }
//- dms === 2014-06-19 ==== Новые терминалы: у них номер забивается звездочкой
           	    long CardNumber = Str2Int(t_card);

                if(BankDiscountTable->GetLongField("BANKID") <= CardNumber &&
                BankDiscountTable->GetLongField("BANKIDEND") >= CardNumber)
                {
                    *disc   = BankDiscountTable->GetField("DISCOUNTS");
                    *idcust = BankDiscountTable->GetLongField("IDCUST");

                    Res=0;
                    break;
                }
            }
            while(BankDiscountTable->GetNextRecord());
        }
		DbfMutex->unlock();

	}
	catch(cash_error& er)
	{
	    Res=-2;
		DbfMutex->unlock();
		LoggingError(er);
	}


	return Res;
}


bool posDBXBase::FindCustomerInfo(long long crCustomer)
{

    bool res = false;
	if (crCustomer==0) return res;

	try
	{
		DbfMutex->lock();
		if(CustomersValueTable->GetFirstRecord())
		{
			do
			{
				if(CustomersValueTable->GetLLongField("IDCUST")==crCustomer)
				{

					DbfMutex->unlock();
					res = true;
					return res;
				}
			}
			while(CustomersValueTable->GetNextRecord());
		}
		DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		DbfMutex->unlock();
		LoggingError(er);
	}

	return res;
}


double posDBXBase::GetItemCountP0Q0(GoodsInfo* CurGoods, FiscalCheque* CurCheque, int type)
{
//    ;;Log("double posDBXBase::GetItemCountP0Q0(GoodsInfo* "
//          +Int2Str(CurGoods->ItemCode)+", FiscalCheque* "
//          +Int2Str(CurCheque->Cheque)+", int "
//          +Int2Str(type)+")"
//          );
    if (
            ((CurGoods->ItemFlag!=SL) && (CurGoods->ItemFlag!=RT))
            || (CurGoods->ItemCode==TRUNC)
            || (CurGoods->ItemCode==EXCESS_PREPAY)
            ) {
//        ;;Log("return 1 ="+Double2Str(CurGoods->ItemCount));
        return CurGoods->ItemCount;
    }

//    ;;Log("CurGoods->ItemDiscountsDesc="+CurGoods->ItemDiscountsDesc);

    if (
            (CurGoods->ItemDiscountsDesc.size()<type)
      || (
                (CurGoods->ItemDiscountsDesc[type-1]!='1')
                && (CurGoods->ItemDiscountsDesc[type-1]!='2')
                && (CurGoods->ItemDiscountsDesc[type-1]!='3')
                && (CurGoods->ItemDiscountsDesc[type-1]!='4')
                && (CurGoods->ItemDiscountsDesc[type-1]!='5')
                && (CurGoods->ItemDiscountsDesc[type-1]!='6')
                && (CurGoods->ItemDiscountsDesc[type-1]!='7')
                && (CurGoods->ItemDiscountsDesc[type-1]!='8')
                && (CurGoods->ItemDiscountsDesc[type-1]!='9')
                //&& (CurGoods->ItemDiscountsDesc[type-1]!='.')
                )
            ) {
//        ;;Log("return 2 ="+Double2Str(CurGoods->ItemCount));
        return CurGoods->ItemCount;
    }

    if (!CurCheque) {
//        ;;Log("return 3 ="+Double2Str(CurGoods->ItemCount));
        return CurGoods->ItemCount;
    }

    double res = 0;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (
                ((g.ItemFlag!=SL) && (g.ItemFlag!=RT))
                || (g.ItemCode==TRUNC)
                || (g.ItemCode==EXCESS_PREPAY)
                ) continue;

        if (g.ItemDiscountsDesc.size()<type) continue;

        if (CurGoods->ItemDiscountsDesc[type-1]=='.')
        {
//            ;;Log("(CurGoods->ItemAlco="+Int2Str(CurGoods->ItemAlco)
//                  +" <= 0) || (CurGoods->ItemCode="
//                  +Int2Str(CurGoods->ItemCode)+" != g.ItemCode="+Int2Str(g.ItemCode)+")"
//                  );
            if ((CurGoods->ItemAlco <= 0) || (CurGoods->ItemCode != g.ItemCode))
//                ;;Log("--- skip ---");
                continue;
        }
        else
        {
            if (g.ItemDiscountsDesc[type-1]!= CurGoods->ItemDiscountsDesc[type-1])
            {
                continue;
            }
        }
//        ;;Log("res += g.ItemCount="+Double2Str(g.ItemCount));
        res += g.ItemCount;
    }
//    ;;Log("return 4 ="+Double2Str(res));
    return res;

}

int posDBXBase::GetMRCFromCigarActsiz(string MRCCode)
{
    int MRC = 0;
    string Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!\"%&'*+-./_,:;=<>?";

	for(int i=0; i<4; i++)
	{
	    string CurSimbol = MRCCode.substr(i,1);
	    int pos = Alphabet.find(CurSimbol);

	    if (pos==string::npos) return 0;

		MRC += (int)pow(80, 3-i)*pos;
	}

	return MRC;

}
