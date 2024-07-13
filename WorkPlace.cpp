/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qgroupbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <qprogressbar.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qinputdialog.h>
#include <qfile.h>

#include <qpainter.h>
#include <qtimer.h>
#include <qimage.h>
//#include <qmovie.h>

#include <algorithm>
#include <math.h>
#include <xbase/xbase.h>
#include <pthread.h>

#include "WorkPlace.h"
#include "Display.h"
#include "Scale.h"
#include "Fiscal.h"
#include "Guard.h"
//#include "Actsiz.h"
#include "PriceSel.h"
#include "SectionSel.h"
#include "Calc.h"
#include "Utils.h"
#include "Search.h"
#include "FreeSum.h"
#include "BankBonus.h"
#include "ScreenSaver.h"
#include "Cash.h"
#include "DbfTable.h"
//#include "Threads.h"
#include "Payment.h"
#include "PaymentType.h"
#include "Ping.h"
#include "Sum.h"
#include "ExtControls.h"
#include "Message.h"
#include "Display.h"
#include "Certificate.h"

FILE *storenum, *keepstate;

const char
			*INFO="ИНФО",
			*SEARCHBYCODE="ПОИСК ПО КОДУ",//names of commands
			*SEARCHBYBARCODE="ПОИСК ПО ШТРИХ-КОДУ",
			*SEARCHBYPRICE="ПОИСК ПО ЦЕНЕ",
			*SEARCHBYPROPERTIES="ПОИСК ПО РЕКВИЗИТАМ",
			*SEARCHBYGROUP="ПОИСК ПО ГРУППЕ",
			*SCALEGOODS="ВЗВЕСИТЬ",
			*ENTERERROR="ОШИБКА",
			*AMOUNT="ВВОД КОЛИЧЕСТВА",
			*DELETEFROMCHECK="УДАЛЕНИЕ ПОКУПКИ ИЗ ЧЕКА",
			*ANNUL="АHHУЛИРОВАТЬ ЧЕК",
			*OPENDRAWER="ОТКРЫТЬ ЯЩИК",
			*FINALIZE="РАСЧЕТ",
			*CALCULATOR="КАЛЬКУЛЯТОР",
			*GROUPCODE="КОД ГРУППЫ ",
			*DISCOUNT="СКИДКА",
			*MESSAGE="СООБЩЕНИЕ",
			*LASTCHECK="LASTCHECK",
			*PRECHECK="PRECHECK",
			*FRSOUND="FRSOUND",//
			*KEEP="ОТЛОЖИТЬ ЧЕК",
			*CERTIFICATE="СЕРТИФИКАТ",
			*FREESUMMA="СВОБОДНАЯ СУММА",
			*SCREENSAVER="SCREENSAVER",
            *ALTFINALIZE="БЕЗНАЛ",
            *SEND="ПЕРЕСЛАТЬ ЧЕК",
            *CHANGEBONUSCARD="ЗАМЕНА",
            *SEARCHBYPHONE="СКИДКА ПО ТЕЛЕФОНУ";

char frSound = 0;

const char *BarcodePrefix[]={"EAN8","EAN13","UPCE","UPCA","CODE39","CODE93","CODE128",
							 "CODABAR","INTERLEAVE25","MSI","DISCOUNTCARD","GASTRONOMCARD","EMPLOYEE"};
const int BarcodePrefixLen=13;

const char *CardPrefix="98";
const char *SPPPrefix="981";
string EmpPrefix="982"; // Employee`s prefix (SBARCODE)
const char *GastronomPrefix="993";
const char *CardPrefixN="922";
const char *ManagerPrefix="222";

//cheque visible properties:number,code,name,price,count,sum
enum {ListNo=0,ListCode,ListName,ListPrice,ListCount,ListSum,ListDscnt};

extern unsigned short RecodeTable[RecodeTableSize][2];
extern int revision;

GoodsInfo CurGoods;
QPainter painter;

static char * statefn = "tmp/state.dat";
static char * sendstatefn = "tmp/sendstate.dat";


//static LProc* WPCX;


/*
 *  Constructs a WorkPlace which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
WorkPlace::WorkPlace( QWidget* parent )  : QDialog( parent, "WorkPlace", true )
{

/* 	//============================
 * 	// При загрузке кассового ПО выполняется cоздание и инициализация объекта PCX
 * 	// Объект должен жить до завершения работы кассового ПО
 * 	// Создания и инициализации объекта на каждом запросе не должно быть
 *
 *
 *     Log(" INIT PCX... ");
 * 	try {
 *
 * 	    WPCX = new LProc();
 * 	    WPCX->Init();
 *         Log("  ..SUCCESS ");
 * 	} catch(exception& ex) {
 * 		// обработка ошибки создания объекта PCX
 * 		Log("  ..ERROR: ");
 * 		Log((char*)ex.what());
 * 	}
 */

    /* slair+ */
    dont_print_paper_check = false;
    /* slair- */

	int BaseSkip,xRes,yRes;
	int BottomLineHeight=12;

	FindInDB = false;

	ActionShowMsg=true;
    CustomerByPhone=false;

    CurrentActiveBonus=0;
    StatusBonusCard ="";
    SummaBonusAction = 0;

    MilkActsiz.clear();

#ifdef ALCO_VES
    AlcoVesActsiz.clear();
#endif

#ifdef BLOCK_CIGAR
    BlockCigarActsiz.clear();
#endif

	//bool BW;

    for(int i=0;i<MAXDISCOUNTLEN;i++) DefaultDiscountType+="X";

	CashReg = (CashRegister*)parent;

	switch (CashReg->ResolutionType)
	{
		case res640x400Color:
		    BW=false;
		case res640x400BW:
		    xRes = 640;
		    yRes = 400;
		    BaseSkip=0;
		    break;
		case res640x480Color:
		    BW=false;
		case res640x480BW:
		    xRes = 640;
		    yRes = 480;
		    BaseSkip=8;
		    break;
		case res800x600Color:
		    BW=false;
		case res800x600BW:
		    xRes = 800;
		    yRes = 600;
		    BaseSkip=8;
		    break;
		case res1024x768Color:
		    BW=false;
		case res1024x768BW:
		    xRes = 1024;
		    yRes = 768;
		    BaseSkip=8;
		    break;
		default: BaseSkip=8;break;
	}

	QColor SumColor;
	QColor CentralFieldColor;
	QColor TitleColor;
	QColor DiscColor;

	if (BW)
	{
		SumColor=black;
		CentralFieldColor=black;
		TitleColor=black;
		DiscColor=black;
	}
	else
	{
		SumColor=blue;
		CentralFieldColor=darkGreen ;
		TitleColor = darkGreen;
		DiscColor = darkBlue;
	}

	vector<BarcodeInfo*> tmpBarcodeTable;

	CashReg->LoadCodesFromDB(&Codes);
	CashReg->LoadBarcodeTableFromDB(&tmpBarcodeTable);

	char a;
	unsigned int i;

	string PreFX="PREFIX_",PostFX="POSTFIX_";
	for (a='1';a<='4';a++)
		for (i=0;i<tmpBarcodeTable.size();i++)
		{
			if ((tmpBarcodeTable[i]->CodeName==PreFX+a)&&(!tmpBarcodeTable[i]->CodeID.empty()))
				PrefixSeq.insert(PrefixSeq.begin(),QString(tmpBarcodeTable[i]->CodeID.c_str()).toInt(NULL,16));
			if ((tmpBarcodeTable[i]->CodeName==PostFX+a)&&(!tmpBarcodeTable[i]->CodeID.empty()))
				PostfixSeq.insert(PostfixSeq.begin(),QString(tmpBarcodeTable[i]->CodeID.c_str()).toInt(NULL,16));
		}

	for (i=0;i<tmpBarcodeTable.size();i++)
	{
		int k;

		string WP="W_PREFIX2"; //read weight prefices in the range [20..29]
		string WPNum;

		for (a='0';a<='9';a++)
			if ((tmpBarcodeTable[i]->CodeName==WP+a)&&(!tmpBarcodeTable[i]->CodeID.empty()))
                    WeightPrefices.insert(WeightPrefices.end(),atoi(tmpBarcodeTable[i]->CodeID.c_str()));

		W_Code_Length=6;  //weight code consists of four parts (prefix,goods code,
		if (tmpBarcodeTable[i]->CodeName=="W_CODE_LENGTH")//weight,checking number).
		{												//Here we read length of the goods code
			W_Code_Length=atoi(tmpBarcodeTable[i]->CodeID.c_str());
			if (W_Code_Length==0)
				W_Code_Length=6;
		}

		for (k=0;k<BarcodePrefixLen;k++)
			if ((tmpBarcodeTable[i]->CodeName==BarcodePrefix[k])&&(!tmpBarcodeTable[i]->CodeID.empty()))
			{
				BarcodeInfo *tmpInfo=new BarcodeInfo;
				tmpInfo->CodeID=tmpBarcodeTable[i]->CodeID;
				tmpInfo->CodeName=tmpBarcodeTable[i]->CodeName;

				BarcodeID.insert(BarcodeID.end(),tmpInfo);
;;Log((char*)tmpBarcodeTable[i]->CodeName.c_str());
			}

		delete tmpBarcodeTable[i];
	}



	setName( "WorkPlace" );
	setFixedSize(CashReg->size());
	move(0,0);

	//список товаров чека
	ChequeList = new SelQListView( this, "ChequeList" );
	ChequeList->addColumn( W2U( "No" ),25 );
	ChequeList->header()->setClickEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->header()->setResizeEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->addColumn( W2U( "КОД" ),55);
	ChequeList->header()->setClickEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->header()->setResizeEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->addColumn( W2U( "НАИМЕНОВАНИЕ" ),xRes-ChequeList->verticalScrollBar()->width()-75-60-75-75);
	ChequeList->header()->setClickEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->header()->setResizeEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->addColumn( W2U( "ЦЕНА" ),75 );
	ChequeList->header()->setClickEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->header()->setResizeEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->addColumn( W2U( "КОЛ-ВО" ),60 );
	ChequeList->header()->setClickEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->header()->setResizeEnabled( false, ChequeList->header()->count() - 1 );

	ChequeList->addColumn( W2U( "СУММА" ),75 );
	ChequeList->header()->setClickEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->header()->setResizeEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->addColumn( W2U( "СКИДКА" ),75 );
	ChequeList->header()->setClickEnabled( false, ChequeList->header()->count() - 1 );
	ChequeList->header()->setResizeEnabled( false, ChequeList->header()->count() - 1 );

	//ChequeList->setGeometry( QRect( 0, 0, width(), height()-70-70-BottomLineHeight-53) );
	ChequeList->setGeometry( QRect( 0, 0, width(), height()-70-70-BottomLineHeight-83) );
	QFont ChequeList_font(  ChequeList->font() );

	ChequeList_font.setPointSize( 11 );//10
	ChequeList_font.setBold(true);
	ChequeList->setFont( ChequeList_font );
	ChequeList->setProperty( "selectionMode", (int)QListView::Single );
	ChequeList->setProperty( "allColumnsShowFocus", QVariant( true, 0 ) );
	ChequeList->setSorting(-1);
	ChequeList->setShowSortIndicator(false);
	ChequeList->setHScrollBarMode(QListView::AlwaysOff);
	ChequeList->setVScrollBarMode(QListView::AlwaysOn);
	ChequeList->move( 0, 70);

	QFont TextLabel_font( font() );
	TextLabel_font.setPointSize(7);


	TextLabel_font.setBold( true );

    //Actsiz
    int CurPos = height()-70-BottomLineHeight-82;

	MActsizLabel = new QColorLabel(this, "MActsizLabel", red );
	MActsizLabel->setFixedSize(width(), 20);//(175, 25);
	MActsizLabel->move( 0, CurPos);
	QFont MActsizLabel_font( font() );
	MActsizLabel_font.setPointSize( 10 );
	MActsizLabel_font.setBold(true);
	MActsizLabel->setFont( MActsizLabel_font );
	MActsizLabel->setProperty( "frameShape", (int)QLabel::Panel );
	MActsizLabel->setProperty( "frameShadow", (int)QLabel::Sunken );
	MActsizLabel->setProperty( "text", tr( "" ) );
	MActsizLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

	MActsizLabel->setProperty( "echoMode", (int)QLineEdit::Password );


	DiscInfoBox = new QGroupBox(this,"DiscInfoBox");
	DiscInfoBox->setFixedSize(xRes,52);
	DiscInfoBox->move(0,height()-70-53-BottomLineHeight);

    CurPos = 10;

// Label discount card
	lbDiscCard = new QColorLabel(DiscInfoBox, "TextLabelDCard", DiscColor);
	lbDiscCard->setFixedSize(100,10);//(175,10);
	lbDiscCard->move( CurPos, 8);
	lbDiscCard->setFont( TextLabel_font );
	lbDiscCard->setProperty( "text", W2U( "Дисконтная карта:" ) );
	lbDiscCard->setProperty( "alignment", int( QLabel::AlignCenter ) );


	lbBonusCard = new QColorLabel(DiscInfoBox, "TextLabelDCard", DiscColor);
	lbBonusCard->setFixedSize(100,10);//(175,10);
	lbBonusCard->move( CurPos, 8);
	lbBonusCard->setFont( TextLabel_font );
	lbBonusCard->setProperty( "text", W2U( "БОНУСНАЯ КАРТА:" ) );
	lbBonusCard->setProperty( "alignment", int( QLabel::AlignCenter ) );
    lbBonusCard->hide();

// Label discount card
	lbHandDiscCard = new QColorLabel(DiscInfoBox, "TextLabelHandDiscountCard", red);
	lbHandDiscCard->setFixedSize(100,10);//(175,10);
	lbHandDiscCard->move( CurPos, 18);
	lbHandDiscCard->setFont( TextLabel_font );
	lbHandDiscCard->setProperty( "text", W2U( "" ) );
	lbHandDiscCard->setProperty( "alignment", int( QLabel::AlignLeft ) );

// Label add-on discount card
	lbAddonDiscCard = new QColorLabel(DiscInfoBox, "TextLabelAddonDCard", DiscColor);
	lbAddonDiscCard->setFixedSize(100,10);//(175,10);
	lbAddonDiscCard->move( CurPos, 32);
	lbAddonDiscCard->setFont( TextLabel_font );
	//lbAddonDiscCard->setProperty( "text", W2U( "Баланс бонусов:" ) );
	lbAddonDiscCard->setProperty( "text", W2U( "Доп. карта:" ) );
	lbAddonDiscCard->setProperty( "alignment", int( QLabel::AlignCenter ) );

    CurPos+= lbDiscCard->width() + 13;

// Edit current discount card number
	MDiscCard = new QColorLabel(DiscInfoBox, "MDiscCard", DiscColor );
	MDiscCard->setFixedSize(180, 22);//(175, 25);
	MDiscCard->move( CurPos, 3);
	QFont MDiscCard_font( font() );
	MDiscCard_font.setPointSize( 14 );
	//MDiscCard_font.setBold(true);
	MDiscCard->setFont( MDiscCard_font );
	MDiscCard->setProperty( "frameShape", (int)QLabel::Panel );
	MDiscCard->setProperty( "frameShadow", (int)QLabel::Sunken );
	MDiscCard->setProperty( "text", tr( "" ) );
	MDiscCard->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Edit current add-on discount card number
	MAddonDiscCard = new QColorLabel(DiscInfoBox, "MAddonDiscCard", Qt::darkMagenta );
	MAddonDiscCard->setFixedSize(180, 22);//(175, 25);
	MAddonDiscCard->move( CurPos, 26);
	QFont MAddonDiscCard_font( font() );
	MAddonDiscCard_font.setPointSize( 14 );
	//MDiscCard_font.setBold(true);
	MAddonDiscCard->setFont( MAddonDiscCard_font );
	MAddonDiscCard->setProperty( "frameShape", (int)QLabel::Panel );
	MAddonDiscCard->setProperty( "frameShadow", (int)QLabel::Sunken );
	MAddonDiscCard->setProperty( "text", tr( "" ) );
	MAddonDiscCard->setProperty( "alignment", int( QLabel::AlignCenter ) );

    CurPos+= MDiscCard->width() + 7;

// Label certificate
	lbCertCard = new QColorLabel(DiscInfoBox, "TextLabelCertCard", DiscColor);
	lbCertCard->setFixedSize(110,10);//(175,10);
	lbCertCard->move( CurPos , 8);
	lbCertCard->setFont( TextLabel_font );
	lbCertCard->setProperty( "text", W2U( "Сертификаты:" ) );
	lbCertCard->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Label need issue certificate
	lbNeedCert = new QColorLabel(DiscInfoBox, "TextLabelNeedcert", DiscColor);
	lbNeedCert->setFixedSize(100,10);//(175,10);
	lbNeedCert->move( CurPos, 18);
	lbNeedCert->setFont( TextLabel_font );
	lbNeedCert->setProperty( "text", W2U( "" ) );
	lbNeedCert->setProperty( "alignment", int( QLabel::AlignLeft ) );

    int CurPosBankBonus = CurPos;

    CurPos+= lbCertCard->width() + 5;

// Edit certificate numbers
	MCertCard = new QColorLabel(DiscInfoBox, "MCertCard", DiscColor );
	MCertCard->setFixedSize(130, 22);//(175, 25);
	MCertCard->move( CurPos, 3);
	MCertCard->setFont( MDiscCard_font );
	MCertCard->setProperty( "frameShape", (int)QLabel::Panel );
	MCertCard->setProperty( "frameShadow", (int)QLabel::Sunken );
	MCertCard->setProperty( "text", tr( "" ) );
	MCertCard->setProperty( "alignment", int( QLabel::AlignCenter ) );

    CurPos+= MCertCard->width();

// Edit certificate discount summa
    MCertDiscount = new QColorLabel(DiscInfoBox, "MCertDiscount", DiscColor );
	MCertDiscount->setFixedSize(120 , 22);//(175, 25);
	MCertDiscount->move( CurPos , 3);
	MCertDiscount->setFont( MDiscCard_font );
	MCertDiscount->setProperty( "frameShape", (int)QLabel::Panel );
	MCertDiscount->setProperty( "frameShadow", (int)QLabel::Sunken );
	MCertDiscount->setProperty( "text", tr( "" ) );
	MCertDiscount->setProperty( "alignment", int( QLabel::AlignCenter ) );

    CurPos+= MCertDiscount->width();

    MCertPrePay = new QColorLabel(DiscInfoBox, "MCertPrePay", Qt::darkMagenta );
	MCertPrePay->setFixedSize(width() - CurPos , 22);//(175, 25);
	MCertPrePay->move( CurPos , 3);
	MCertPrePay->setFont( MDiscCard_font );
	MCertPrePay->setProperty( "frameShape", (int)QLabel::Panel );
	MCertPrePay->setProperty( "frameShadow", (int)QLabel::Sunken );
	MCertPrePay->setProperty( "text", tr( "" ) );
	MCertPrePay->setProperty( "alignment", int( QLabel::AlignCenter ) );

    CurPos+= lbDiscCard->width() + 5;

	InfoCLPay = new QColorLabel(DiscInfoBox, "TextLabelInfoCLPay", Qt::darkMagenta);
	InfoCLPay->setFixedSize(200,20);//(175,10);
	InfoCLPay->move(width()-200 , 28);

	QFont InfoCLPay_font( font() );
	InfoCLPay_font.setPointSize( 12 );
	InfoCLPay_font.setBold(true);

	InfoCLPay->setFont( InfoCLPay_font );
	InfoCLPay->setProperty( "text", tr( "" ) );
	InfoCLPay->setProperty( "alignment", int( QLabel::AlignRight ) );


// Label bank bonus
	lbBankBonus = new QColorLabel(DiscInfoBox, "TextbBankBonus", DiscColor);
	lbBankBonus->setFixedSize(110,10);//(175,10);
	lbBankBonus->move( CurPosBankBonus ,32);
	lbBankBonus->setFont( TextLabel_font );
	lbBankBonus->setProperty( "text", W2U( "Бонусы :" ) );
	lbBankBonus->setProperty( "alignment", int( QLabel::AlignCenter ) );

    CurPosBankBonus+=lbBankBonus->width() + 5;

// Edit bank bonus card numbers
	MBankBonusCard = new QColorLabel(DiscInfoBox, "MBankBonusCard", DiscColor );
	MBankBonusCard->setFixedSize(130, 22);//(175, 25);
	MBankBonusCard->move( CurPosBankBonus, 26);
	MBankBonusCard->setFont( MDiscCard_font );
	MBankBonusCard->setProperty( "frameShape", (int)QLabel::Panel );
	MBankBonusCard->setProperty( "frameShadow", (int)QLabel::Sunken );
	MBankBonusCard->setProperty( "text", tr( "" ) );
	MBankBonusCard->setProperty( "alignment", int( QLabel::AlignCenter ) );

    CurPosBankBonus+= MBankBonusCard->width();

// Edit bank bonus discount summa
    MBankBonusDiscount = new QColorLabel(DiscInfoBox, "MCertDiscount", DiscColor );
	MBankBonusDiscount->setFixedSize(120 , 22);
	MBankBonusDiscount->move( CurPosBankBonus , 26);
	MBankBonusDiscount->setFont( MDiscCard_font );
	MBankBonusDiscount->setProperty( "frameShape", (int)QLabel::Panel );
	MBankBonusDiscount->setProperty( "frameShadow", (int)QLabel::Sunken );
	MBankBonusDiscount->setProperty( "text", tr( "" ) );
	MBankBonusDiscount->setProperty( "alignment", int( QLabel::AlignCenter ) );


// Goods info Box

	GoodsInfoBox = new QGroupBox(this,"GoodsInfoBox");
	GoodsInfoBox->setFixedSize(xRes,72);
	GoodsInfoBox->move(0,height()-72-BottomLineHeight);

// Label Code

	lbCode = new QColorLabel(GoodsInfoBox, "TextLabel1", TitleColor);
	lbCode->setFixedSize(100,7);//(175,10);
	lbCode->move( 0, 2);
	lbCode->setFont( TextLabel_font );
	lbCode->setProperty( "text", W2U( "Код" ) );// W2U( ";ljkasf" ) );//
	lbCode->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Edit Code
	MItemCode = new QColorLabel(GoodsInfoBox, "MItemCode", CentralFieldColor );
	MItemCode->setFixedSize(100, 25);//(175, 25);
	MItemCode->move( 0, 10);
	QFont MItemCode_font( font() );
	MItemCode_font.setPointSize( 18 );
	MItemCode_font.setBold(true);
	MItemCode->setFont( MItemCode_font );
	MItemCode->setProperty( "frameShape", (int)QLabel::Panel );
	MItemCode->setProperty( "frameShadow", (int)QLabel::Sunken );
	MItemCode->setProperty( "text", tr( "" ) );
	MItemCode->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Label Name

	TextLabel2 = new QColorLabel(GoodsInfoBox, "TextLabel2", TitleColor );
	TextLabel2->setFixedSize(width()-MItemCode->width()-100,7);//(width()-MItemCode->width()-160,10);
	TextLabel2->move(MItemCode->width(), 2);

	TextLabel2->setFont( TextLabel_font );
	TextLabel2->setProperty( "text", W2U( "Наименование" ) );
	TextLabel2->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Edit Name
	MItemName = new QColorLabel(GoodsInfoBox, "MItemName", CentralFieldColor );
	//MItemName->setFixedSize(ChequeList->width()-MItemCode->width()-197,25);
	MItemName->setFixedSize(ChequeList->width()-MItemCode->width()-195,25);
	MItemName->move( MItemCode->width(), 10 );
	QFont MItemName_font( font() );
	MItemName_font.setPointSize( 12 );//(18);

	MItemName_font.setBold(true);
	MItemName->setFont( MItemName_font );
	MItemName->setProperty( "frameShape", (int)QLabel::Panel );
	MItemName->setProperty( "frameShadow", (int)QLabel::Sunken );
	MItemName->setProperty( "text", "" );


// Label Num

	TextLabel3 = new QColorLabel(GoodsInfoBox, "TextLabel3", TitleColor );
	TextLabel3->setFixedSize(40, 10);
	TextLabel3->move( 0, 35);
	TextLabel3->setFont( TextLabel_font );
	TextLabel3->setProperty( "text", W2U( "No" ) );
	TextLabel3->setProperty( "alignment", int( QLabel::AlignCenter ) );

//Edit Num
	MItemNumber = new QColorLabel(GoodsInfoBox, "MItemNumber", CentralFieldColor );
	MItemNumber->setFixedSize(50, 25);
	MItemNumber->move( 0, 45);
	QFont MItemNumber_font( font() );
	MItemNumber_font.setPointSize( 18 );
	MItemNumber_font.setBold(true);
	MItemNumber->setFont( MItemNumber_font );
	MItemNumber->setProperty( "frameShape", (int)QLabel::Panel );
	MItemNumber->setProperty( "frameShadow", (int)QLabel::Sunken );
	MItemNumber->setProperty( "text", tr( "" ) );
	MItemNumber->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Label Kol-vo

	TextLabel4 = new QColorLabel(GoodsInfoBox, "TextLabel4", TitleColor );
	TextLabel4->setFixedSize(80, 10);
	TextLabel4->move(50+ MItemNumber->width(), 35 );
	TextLabel4->setFont( TextLabel_font );
	TextLabel4->setProperty( "text", W2U( "Количество" ) );
	TextLabel4->setProperty( "alignment", int( QLabel::AlignCenter ) );

//Edit Kol-vo
	MItemCount = new QColorLabel(GoodsInfoBox, "MItemCount", CentralFieldColor );
	MItemCount->setFixedSize(100, 25);
	MItemCount->move(50+MItemNumber->width(), 45);
	QFont MItemCount_font( font() );
	MItemCount_font.setPointSize( 18 );
	MItemCount_font.setBold(true);
	MItemCount->setFont( MItemCount_font );
	MItemCount->setProperty( "frameShape", (int)QLabel::Panel );
	MItemCount->setProperty( "frameShadow", (int)QLabel::Sunken );
	MItemCount->setProperty( "text", tr( "" ) );
	MItemCount->setProperty( "alignment", int( QLabel::AlignCenter ) );


// Label Price

	TextLabel5 = new QColorLabel(GoodsInfoBox, "TextLabel5", TitleColor );
	TextLabel5->setFixedSize(100, 7);
	TextLabel5->move( MItemCode->width() + MItemName->width(),2 );
	TextLabel5->setFont( TextLabel_font );
	TextLabel5->setProperty( "text", W2U( "Цена" ) );
	TextLabel5->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Edit Price
	MItemPrice = new QColorLabel(GoodsInfoBox, "MItemPrice", CentralFieldColor );
	MItemPrice->setFixedSize(100, 25);
	MItemPrice->move( MItemCode->width() + MItemName->width(),10);
	QFont MItemPrice_font( font() );
	MItemPrice_font.setBold(true);
	MItemPrice_font.setPointSize( 18 );
	MItemPrice->setFont( MItemPrice_font );
	MItemPrice->setProperty( "frameShape", (int)QLabel::Panel );
	MItemPrice->setProperty( "frameShadow", (int)QLabel::Sunken );
	MItemPrice->setProperty( "text", tr( "" ) );
	MItemPrice->setProperty( "alignment", int( QLabel::AlignCenter ) );


// Label Disc

	TextLabel7 = new QColorLabel(GoodsInfoBox, "TextLabel7", TitleColor );
	TextLabel7->setFixedSize(125, 10);
	TextLabel7->move( 50+MItemNumber->width()+MItemCount->width(), 35 );
	TextLabel7->setFont( TextLabel_font );
	TextLabel7->setProperty( "text", W2U( "Скидка" ) );
	TextLabel7->setProperty( "alignment", int( QLabel::AlignCenter ) );

//Edit Disc
	MItemDiscount = new QColorLabel(GoodsInfoBox, "MItemDiscount", CentralFieldColor );
	MItemDiscount->setFixedSize(145, 25);
	MItemDiscount->move( 50+MItemNumber->width()+MItemCount->width(), 45);
	QFont MItemDiscount_font( font() );
	MItemDiscount_font.setPointSize( 18 );
	MItemDiscount_font.setBold(true);
	MItemDiscount->setFont( MItemDiscount_font );
	MItemDiscount->setProperty( "frameShape", (int)QLabel::Panel );
	MItemDiscount->setProperty( "frameShadow", (int)QLabel::Sunken );
	MItemDiscount->setProperty( "text","");
	MItemDiscount->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Label Sum //Сумма по товару

	lbSum = new QColorLabel(GoodsInfoBox, "lbSum", TitleColor );
	lbSum->setFixedSize(130, 10);
	lbSum->move( 50+MItemNumber->width()+MItemCount->width()+MItemDiscount->width(), 35);
	lbSum->setFont( TextLabel_font );
	lbSum->setProperty( "text", W2U( "Сумма" ) );
	lbSum->setProperty( "alignment", int( QLabel::AlignCenter ) );

//Edit Sum //сумма по товару

	QFont MItemSum_font( font() );
	MItemSum_font.setPointSize( 18 );
	MItemSum_font.setBold(true);

	MItemSum = new QColorLabel(GoodsInfoBox, "MItemSum", CentralFieldColor );
	MItemSum->setFixedSize(130, 25);
	MItemSum->move(  50+MItemNumber->width()+MItemCount->width()+MItemDiscount->width(), 45 );
	MItemSum->setFont( MItemSum_font );
	MItemSum->setProperty( "frameShape", (int)QLabel::Panel );
	MItemSum->setProperty( "frameShadow", (int)QLabel::Sunken );
	MItemSum->setProperty( "text","");
	MItemSum->setProperty( "alignment", int( QLabel::AlignCenter ) );




// Edit Sbarcode
	MItemSBarcode = new QColorLabel(GoodsInfoBox, "MItemSbarcode", Qt::darkMagenta );
	MItemSBarcode->setFixedSize(50+MItemNumber->width()+MItemCount->width()+MItemDiscount->width()+MItemSum->width()-100,25);
	MItemSBarcode->move( 50+MItemNumber->width()+MItemCount->width()+MItemDiscount->width()+MItemSum->width(), 45 );
	QFont MItemSBarcode_font( font() );
	MItemSBarcode_font.setPointSize( 10 );//(18);

	//MItemSBarcode_font.setBold(true);
	MItemSBarcode->setFont( MItemSBarcode_font );
	MItemSBarcode->setProperty( "frameShape", (int)QLabel::Panel );
	MItemSBarcode->setProperty( "frameShadow", (int)QLabel::Sunken );
	MItemSBarcode->setProperty( "text", "" );

// Label Time


	QFont fntTime( font() );
	fntTime.setPointSize( 18 );
	fntTime.setBold(true);

	lbTime = new QColorLabel(GoodsInfoBox, "lbTime", CentralFieldColor );
	//lbTime = new QColorLabel(GoodsInfoBox, "lbTime", Qt::darkBlue );
	lbTime->setFont(fntTime);
	lbTime->setFixedSize(120, 30);
	lbTime->move( xRes-212, 39);
	lbTime->setProperty( "text", W2U( "ВРЕМЯ" ));
	lbTime->setProperty( "alignment", int( QLabel::AlignCenter ) );
// END GoodsInfoBox

// Label Kassir

	lbSaleman = new QColorLabel(this, "lbSaleman", CentralFieldColor );
	lbSaleman->setFixedSize(xRes/4,10);
	lbSaleman->move(0,0);
	lbSaleman->setFont( TextLabel_font );
	lbSaleman->setProperty( "text", W2U( "Вас обслуживает" ) );
	lbSaleman->setProperty( "alignment", int( QLabel::AlignCenter ) );

//Edit Kassir

	Saleman = new QColorLabel(this, "Saleman", darkGray );
	Saleman->setFixedSize(int(xRes/3.5),25);
	Saleman->move(0,10);
	//;;Saleman->setProperty( "frameShape", (int)QLabel::Panel );
	//;;Saleman->setProperty( "frameShadow", (int)QLabel::Sunken );
	Saleman->setProperty( "text", tr( "" ) );
	Saleman->setProperty( "alignment", int( QLabel::AlignLeft ) );

// Label Input Line
//	Log("lbEdit");
//	lbEdit = new QColorLabel( this, "TextLabel10",  CentralFieldColor );
//	lbEdit->setFixedSize(xRes/2, 10);
//	lbEdit->move( 0, 35);
//	lbEdit->setFont( TextLabel_font );
//	lbEdit->setProperty( "text", W2U("Ввод значений" ) );
//	lbEdit->setProperty( "alignment", int( QLabel::AlignCenter ) );


//Edit Input Line

	EditLine = new ExtQLineEdit( this, "EditLine", Codes, PrefixSeq, PostfixSeq );
	QFont EditLine_font(  EditLine->font() );
	EditLine_font.setPointSize( 10 );

	EditLine->setFixedSize(xRes/2, 25);
	EditLine->move(  0,45);
	EditLine->setFont( EditLine_font );

	EditLine->setFocus();

// Label Total Disc

	lbTotalDiscount = new QColorLabel( this, "lbTotalDiscount",  CentralFieldColor );
	lbTotalDiscount->setFixedSize(xRes/4, 10);
	lbTotalDiscount->move( xRes/4, 0);
	lbTotalDiscount->setFont( TextLabel_font );
	lbTotalDiscount->setProperty( "text", W2U( "Скидка по чеку" ) );
	lbTotalDiscount->setProperty( "alignment", int( QLabel::AlignCenter ) );
	lbTotalDiscount->hide();

//Edit Total Disc

	QFont fntTotalDiscount(  font() );
	fntTotalDiscount.setPointSize( 18 );
	fntTotalDiscount.setBold(true);

	eTotalDiscount = new QColorLabel( this, "eTotalDiscount", SumColor );
	eTotalDiscount->setFixedSize(xRes/4,25);
	eTotalDiscount->move(xRes/4,10);
	eTotalDiscount->setFont(fntTotalDiscount);
	eTotalDiscount->setProperty( "text","0,00");
	eTotalDiscount->setProperty( "alignment", int( QLabel::AlignCenter ) );
	eTotalDiscount->hide();

// Label Total SUM

	lbTotalSum = new QColorLabel( this, "lbTotalSum",  CentralFieldColor );
	lbTotalSum->setFixedSize(xRes/2, 10);
	lbTotalSum->move(xRes/2,0);
	lbTotalSum->setFont( TextLabel_font );
	lbTotalSum->setProperty( "text", W2U( "Cумма чека" ) );
	lbTotalSum->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Label Total Change

	lbTotalChange = new QColorLabel( this, "lbTotalChange",  CentralFieldColor );
	lbTotalChange->setFixedSize(xRes/2, 10);
	lbTotalChange->move(xRes/2,0);
	lbTotalChange->setFont( TextLabel_font );
	lbTotalChange->setProperty( "text", W2U( "Сдача" ) );
	lbTotalChange->setProperty( "alignment", int( QLabel::AlignCenter ) );

//Edit Total SUM

	QFont MTotalSum_font(  font() );
	MTotalSum_font.setPointSize( 48 );

	MTotalSum = new QColorLabel( this, "MTotalSum", SumColor );
	MTotalSum->setFixedSize(xRes/2,60);
	MTotalSum->move(xRes/2,lbTotalSum->height());
	MTotalSum->setFont( MTotalSum_font );
	MTotalSum->setProperty( "frameShape", (int)QLabel::Panel );
	MTotalSum->setProperty( "frameShadow", (int)QLabel::Sunken );
	MTotalSum->setProperty( "text","0,00");
	MTotalSum->setProperty( "alignment", int( QLabel::AlignCenter ) );

//Edit Total Change

	QFont MTotalChange_font(  font() );
	MTotalChange_font.setPointSize( 48 );

	MTotalChange = new QColorLabel( this, "MTotalChange", SumColor );
	MTotalChange->setFixedSize(xRes/2,60);
	MTotalChange->move(xRes/2,lbTotalChange->height());
	MTotalChange->setFont( MTotalChange_font );
	MTotalChange->setProperty( "frameShape", (int)QLabel::Panel );
	MTotalChange->setProperty( "frameShadow", (int)QLabel::Sunken );
	MTotalChange->setProperty( "text","0,00");
	MTotalChange->setProperty( "alignment", int( QLabel::AlignCenter ) );


	QFont ButtonFont(font());//GUI elements initialization
	ButtonFont.setPointSize(8);

	InfoBar = new QProgressBar(this);
	InfoBar->setFixedSize(135,BottomLineHeight);
	InfoBar->move(0,height()-InfoBar->height());

	InfoMessage = new QLabel(W2U("*"),this);
	InfoMessage->setFixedSize(265 - (CashReg->CheckMillenium?15:0),BottomLineHeight);
	InfoMessage->move(InfoBar->width(),height()-InfoMessage->height());
	InfoMessage->setFrameShape( QLabel::Panel );
	InfoMessage->setFrameShadow( QLabel::Sunken );
	InfoMessage->setFont(ButtonFont);

	if(CashReg->CheckMillenium){
		ActionInfo = new QLabel(W2U("A"),this);
		ActionInfo->setFixedSize(15,BottomLineHeight);
		ActionInfo->move(InfoBar->width()+InfoMessage->width(),height()-ActionInfo->height());
		ActionInfo->setFrameShape( QLabel::Panel );
		ActionInfo->setFrameShadow( QLabel::Sunken );
		ActionInfo->setFont(ButtonFont);
	}else{
		ActionInfo = NULL;
	}

	KeepCheckInfo = new QColorLabel(this, "KeepCheck",SumColor);
	KeepCheckInfo->setFixedSize(30,BottomLineHeight);
	KeepCheckInfo->move(InfoBar->width()+InfoMessage->width(),height()-KeepCheckInfo->height());
	QFont KeepCheckInfo_font( font() );
	KeepCheckInfo_font.setPointSize( 8 );
	KeepCheckInfo_font.setBold(true);
	//KeepCheckInfo_font.setStrikeOut(true);
	KeepCheckInfo->setFont( KeepCheckInfo_font );
	KeepCheckInfo->setProperty( "frameShape", (int)QLabel::Panel );
	KeepCheckInfo->setProperty( "frameShadow", (int)QLabel::Sunken );
	if(keepstate=fopen("tmp/keepstate.dat","r")){//
		fclose(keepstate);
		KeepCheckInfo->setProperty( "text",W2U("О.Ч."));
//		KeepCheckInfo->setProperty( "StrikeOut",true);
	}
	else
		KeepCheckInfo->setProperty( "text","");
	KeepCheckInfo->setProperty( "alignment", int( QLabel::AlignCenter ) );

    string stEgais = (CashReg->EgaisSettings.ModePrintQRCode>-1)?"ЕГАИС":"ОШИБКА";
    Bar1 = new QColorLabel(this, "Bar1",Qt::red);
	//Bar1 = new QLabel(W2U(""),this);
	QFont Bar1_font( font() );
	Bar1_font.setPointSize( 8 );
	Bar1_font.setBold(true);
	Bar1->setFont( Bar1_font );

    Bar1->setProperty( "text",W2U(stEgais));

	Bar1->setFixedSize(70,BottomLineHeight);
	Bar1->move(InfoBar->width()+InfoMessage->width()+KeepCheckInfo->width(),height()-Bar1->height());
	Bar1->setFrameShape( QLabel::Panel );
	Bar1->setFrameShadow( QLabel::Sunken );
	//ButtonFont.setStrikeOut(CashReg->Bar1->font().strikeOut());
	Bar1->setFont(ButtonFont);

	Bar2 = new QLabel(W2U(""),this);
	Bar2->setFixedSize(30,BottomLineHeight);
	Bar2->move(InfoBar->width()+InfoMessage->width()+KeepCheckInfo->width()+Bar1->width(),height()-Bar2->height());
	Bar2->setFrameShape( QLabel::Panel );
	Bar2->setFrameShadow( QLabel::Sunken );
	//ButtonFont.setStrikeOut(CashReg->Bar2->font().strikeOut());
	Bar2->setFont(ButtonFont);

	Bar3 = new QLabel(W2U(""),this);
	Bar3->setFixedSize(30,BottomLineHeight);
	Bar3->move(InfoBar->width()+InfoMessage->width()+KeepCheckInfo->width()+Bar1->width()+Bar2->width(),height()-Bar3->height());
	Bar3->setFrameShape( QLabel::Panel );
	Bar3->setFrameShadow( QLabel::Sunken );
	//ButtonFont.setStrikeOut(CashReg->Bar3->font().strikeOut());
	Bar3->setFont(ButtonFont);

	BankInfo = new QLabel(W2U(CashReg->TryBankConnect?"Банк":""),this);
	BankInfo->setFixedSize(40,BottomLineHeight);
	BankInfo->move(InfoBar->width()+InfoMessage->width()+KeepCheckInfo->width()+Bar1->width()+Bar2->width()+Bar3->width(),height()-BankInfo->height());
	BankInfo->setFrameShape( QLabel::Panel );
	BankInfo->setFrameShadow( QLabel::Sunken );

//	QFont tmpFont=ButtonFont;
//	tmpFont.setStrikeOut(CashReg->BankInfo->font().strikeOut());

//	BankInfo->setFont(tmpFont);
	//ButtonFont.setStrikeOut(CashReg->BankInfo->font().strikeOut());
	BankInfo->setFont(ButtonFont);
    UpdateBankStatus();

	ServerInfo = new QLabel(W2U("БАЗА"),this);
	ServerInfo->setFixedSize(50,BottomLineHeight);
	ServerInfo->move(InfoBar->width()+KeepCheckInfo->width()+InfoMessage->width()+(CashReg->CheckMillenium?15:0)
		+Bar1->width()+Bar2->width()+Bar3->width()+BankInfo->width(),height()-ServerInfo->height());
	ServerInfo->setFrameShape( QLabel::Panel );
	ServerInfo->setFrameShadow( QLabel::Sunken );
	//ButtonFont.setStrikeOut(CashReg->ServerInfo->font().strikeOut());
	ServerInfo->setFont(ButtonFont);

	RegisterInfo = new QLabel(W2U("ФР"),this);
	RegisterInfo->setFixedSize(30,BottomLineHeight);
	RegisterInfo->move(InfoBar->width()+KeepCheckInfo->width()+InfoMessage->width()+ServerInfo->width()+(CashReg->CheckMillenium?15:0)
		+Bar1->width()+Bar2->width()+Bar3->width()+BankInfo->width(),height()-RegisterInfo->height());
	RegisterInfo->setFrameShape( QLabel::Panel );
	RegisterInfo->setFrameShadow( QLabel::Sunken );
	//ButtonFont.setStrikeOut(CashReg->RegisterInfo->font().strikeOut());
	RegisterInfo->setFont(ButtonFont);

	Timer = new QTimer(this);//start clock
//	Timer2 = new QTimer(this);
	QTimer::singleShot(100, this, SLOT(CheckCheque()));//
//	if (!Cheque->GetCount()) QTimer::singleShot(100, this, SLOT(AutoRestoreKeepedCheque()));

	//PriceChecker = new PriceCheck(this);

//*
// + dms ===== New Year 2011 =====\

	LabelImage = new QLabel(this, "LabelImage");
	LabelImage->move(width()-80, height()-80);

	LabelImage->setFixedSize(70, 70);
	LabelImage->setProperty( "alignment", int( QLabel::AlignCenter ) );
	LabelImage->setBackgroundColor(Qt::lightGray);
    LabelImage->setMargin(1);

    QPixmap pic;

    QImage img("./logo2.gif");
    if (!pic.convertFromImage( img ))
    {
       QImage img_logo("./logo.jpg");
       pic.convertFromImage( img_logo );

    }

    LabelImage->setPixmap(pic);

    //QMovie *mv = new QMovie("./Xmas.gif");
    //LabelImage->setMovie(*mv);

    LabelImage->setScaledContents(true);

// - dms =====/
//*/

	connect( Timer, SIGNAL(timeout()), this, SLOT(timerDone()));
	connect( EditLine, SIGNAL(keyPressed(int)), this, SLOT(ProcessKey(int)));
	connect( EditLine, SIGNAL(barcodeFound()), this, SLOT(ProcessBarcode()));


	Timer->start(1000);

    // bank connection test timer
	TimerBank = new QTimer(this);//start clock
	connect( TimerBank, SIGNAL(timeout()), this, SLOT(TimerBankAction()));
//! выкл.
//!	TimerBank->start(3*60*1000);  // в милисекундах

    HandBarcode = false;

	bPriceCheck=false;
	PriceChecker=NULL;

	//bFreeSum=false;
	FreeSum=NULL;
    ScreenSaver=NULL;
    Messager=NULL;

    PictureNumber =0 ;
    //!ShowScreenSaver =true;
    ShowScreenSaver =false;
	TimerScreenSaver = new QTimer(this);//start ScreenSaver  timer
	connect( TimerScreenSaver, SIGNAL(timeout()), this, SLOT(StartScreenSaver()));

    //!TimerScreenSaver->start(WAITTIMESS);


	char* KEYNAME[0xffff];

	for(i=0;i<0xffff;i++)
		KEYNAME[i]="";

	KEYNAME[Key_F1]="F1";KEYNAME[Key_F2]="F2";KEYNAME[Key_F3]="F3";
	KEYNAME[Key_F4]="F4";KEYNAME[Key_F5]="F5";KEYNAME[Key_F6]="F6";
	KEYNAME[Key_F7]="F7";KEYNAME[Key_F8]="F8";KEYNAME[Key_F9]="F9";
	KEYNAME[Key_F10]="F10";KEYNAME[Key_F11]="F11";KEYNAME[Key_F12]="F12";
	KEYNAME[Key_Plus]="+";KEYNAME[Key_Return]=KEYNAME[Key_Enter]="ВВОД";
	KEYNAME[Key_Delete]="DEL";KEYNAME[Key_End]="END";
	KEYNAME[Key_Space]="ПРОБЕЛ";KEYNAME[Key_Minus]="--";
	KEYNAME[Key_Slash]=" /"; //key hints
	KEYNAME[Key_Asterisk]="*";

	CashReg->LoadFRSoundFromDB(&frSound,FRSOUND);

	try
	{
		IsCurrentGoodsEmpty=true;   //init workplace
		CurrentGoods = &CurGoods;//new GoodsInfo;
		Cheque = new FiscalCheque;

		Saleman->setText(W2U(CashReg->CurrentSaleman->Name));

		PaymentKeyPress=0;

		CashReg->DbfMutex->lock();

		switch (CashReg->DisplayType)//init driver of the customer's display
		{
			case EmptyDisplayType:CustomerDisplay=/*(AbstractDisplay*)*/new EmptyDisplay(0,0);break;
			case DatecsDisplayType:	CustomerDisplay=/*(AbstractDisplay*)*/new DatecsDisplay(
									CashReg->ConfigTable->GetLongField("DISPLPORT"),
									CashReg->ConfigTable->GetLongField("DISPLBAUD"));
									break;
			case VSComDisplayType:	CustomerDisplay=/*(AbstractDisplay*)*/new VSComDisplay(
									CashReg->ConfigTable->GetLongField("DISPLPORT"),
									CashReg->ConfigTable->GetLongField("DISPLBAUD"));
									break;
		}

		switch (CashReg->ScaleType)
		{
			case EmptyScaleType:
				Scales=/*(AbstractScales*)*/new EmptyScales(0,0,0);
				break;
			case MassaKScaleType:
				Scales=/*(AbstractScales*)*/new MassaKScales(//init scale's driver
					CashReg->ConfigTable->GetLongField("SCALEPORT"),
					CashReg->ConfigTable->GetLongField("SCALEBAUD"),
					CashReg->ConfigTable->GetLongField("SCALE_WAIT"));
				break;
			case MassaKPType:
				Scales=new MassaKP(//init scale's driver
					CashReg->ConfigTable->GetLongField("SCALEPORT"),
					CashReg->ConfigTable->GetLongField("SCALEBAUD"),
					CashReg->ConfigTable->GetLongField("SCALE_WAIT"));
				break;
		}

		Scanner = new ComPortScan(CashReg
					    ,CashReg->ConfigTable->GetLongField("SCANPORT")
					    ,CashReg->ConfigTable->GetLongField("SCANBAUD")
					    );//init scanner's driver

		//Scanner = CashReg->Scanner;

		MaxAmount=CashReg->ConfigTable->GetLongField("MAXAMOUNT");//limits of the
		if(MaxAmount < .001) MaxAmount = 100000;
		MaxSum=CashReg->ConfigTable->GetLongField("MAXSUM");//saleman's activity

		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		CashReg->DbfMutex->unlock();
		CashReg->LoggingError(er);
	}

	IsChequeInProgress=false;

	InitWorkPlace();
	BarcodeCard=NULL;
	CashReg->ActsizCard=NULL;

	timerDone();

}

/*
 *  Destroys the object and frees any allocated resources
 */
WorkPlace::~WorkPlace()
{
//    WPCX->~LProc();
    // no need to delete child widgets, Qt does it all for us
}

void WorkPlace::CloseWorkPlace(void)
{
	int GuardRes=NotCorrect;

	//if we have opened check then try to annul it
	if (((Cheque->GetSize()>0)&&(!CashReg->CurrentSaleman->Annul))||
			((!IsCurrentGoodsEmpty)&&(!CashReg->CurrentSaleman->CurrentGoods)))
		while(!CheckGuardCode(GuardRes));


	if (Cheque->GetCount()>0)
	{
		CashReg->ShowErrorCode(FiscalReg->Cancel());
		CashReg->AnnulCheck(Cheque,GuardRes);
	}
	keepstate=fopen("tmp/keepstate.dat","r");
	if(keepstate)
	{
		if(fclose(keepstate)){
			ShowMessage("Ошибка закрытия файла!\nСообщите администратору!");
			return;
		}
		if (!Cheque->GetSize())
		{//восстановить отложенный чек
			RestoreKeepedCheque();
			ShowMessage("Отложенный чек восстановлен для аннулирования!");
		}
		GuardRes=NotCorrect;
		//if we have opened check then try to annul it
		if (((Cheque->GetSize()>0)&&(!CashReg->CurrentSaleman->Annul))||
				((!IsCurrentGoodsEmpty)&&(!CashReg->CurrentSaleman->CurrentGoods)))
			while(!CheckGuardCode(GuardRes));
		if (!IsCurrentGoodsEmpty)
			CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CURRENTGOODS,
				CurrentGoods->ItemSum,CashReg->CurrentSaleman->Id,GuardRes);
		if (Cheque->GetCount()>0)
		{
			CashReg->ShowErrorCode(FiscalReg->Cancel());
			CashReg->AnnulCheck(Cheque,GuardRes);
		}
	}

	delete CustomerDisplay;
	delete Scales;

	delete Scanner;//destroy scanner's driver

	//delete CurrentGoods;
	delete Cheque;
	delete Timer;
	delete TimerBank;
    delete TimerScreenSaver;

	for (unsigned int i=0;i<BarcodeID.size();i++)
		delete BarcodeID[i];
}

void WorkPlace::ShowMessage(string str)
{
	::ShowMessage(thisApp->focusWidget(),str);
}

bool WorkPlace::ShowQuestion(string str,bool IsYesDefault)
{
	return ::ShowQuestion(thisApp->focusWidget(),str,IsYesDefault);
}

void WorkPlace::timerDone(void)
{
	//setCaption(W2U("АРМ КАССИРА")+W2U(CashReg->CurrentSaleman->Name)+" "+(CashReg->GetCurDate()+" "+CashReg->GetCurTime()).c_str());
    /* slair+ */
//    QString status_check;
//    if (dont_print_paper_check) {
//        status_check = W2U(" Без бумажного чека");
//    } else {
//        status_check = W2U(" С бумажным чеком");
//    }
//    setCaption(W2U("APM KACCA rev.")+W2U(Int2Str(revision))+W2U(" ")+(CashReg->GetCurDate()).c_str()+status_check);
    /* slair- */
    setCaption(W2U("APM KACCA rev.")+W2U(Int2Str(revision))+W2U(" ")+(CashReg->GetCurDate()).c_str());
	lbTime->setProperty( "text",CashReg->GetCurTime().c_str());
}

void WorkPlace::ShowSpecLabel(QLabel* CurLabel,char **KeyCode)
//show key hint
//if key is not active we disable it
{
/*
	CashReg->DbfMutex->lock();

	int Key=CashReg->KeyboardTable->GetLongField("KEYCODE");

	CashReg->DbfMutex->unlock();

	if (Key>0)
	{

		CurLabel->setText(CurLabel->text()+W2U(KeyCode[Key]));
		CurLabel->show();
	}
*/
}

void WorkPlace::ShowGoodsInfo(void)
{
    //;;Log("[FUNC] ShowGoodsInfo [START]");

	if((PriceChecker!=NULL) || (FreeSum!=NULL))
		return;

	if (Cheque->GetSize()==0)
	{
		ChequeList->clear();
		InfoMessage->setText(W2U("*"));
        RestartTimerSS(true);
	}
	else
	{
	    InfoMessage->setText(W2U(""));
        RestartTimerSS(false);
    }

    // пересчет скидок
	ProcessDiscount(Cheque);

	if (!IsCurrentGoodsEmpty)  //show properties of the current goods
	{
//;;Log("[FUNC] ShowGoodsInfo [+1]");
  		MItemCode->setNum(CurrentGoods->ItemCode);
		MItemName->setText(W2U(CurrentGoods->ItemName));
//;;Log("[FUNC] ShowGoodsInfo [+1.1]");
        if (CurrentGoods->ItemSecondBarcode.length()<=21)
            MItemSBarcode->setText(W2U(CurrentGoods->ItemSecondBarcode.substr(0, 21)));
        else
            MItemSBarcode->setText(tr(""));
//;;Log("[FUNC] ShowGoodsInfo [+1.2]");
		MItemNumber->setNum(CurrentGoods->ItemNumber);

		MItemCount->setText(GetRounded(CurrentGoods->ItemCount,CurrentGoods->ItemPrecision).c_str());
		MItemPrice->setText(GetRounded(CurrentGoods->ItemPrice,0.01).c_str());
		MItemSum->setText(GetRounded(CurrentGoods->ItemSum,0.01).c_str());
		MItemDiscount->setText(GetRounded(CurrentGoods->ItemCalcSum-CurrentGoods->ItemSum,0.01).c_str());

		//MActsizLabel->setText(W2U(CurrentGoods->ItemActsizBarcode));
//;;Log("[FUNC] ShowGoodsInfo [+2]");
        //int cntActsiz = CurrentGoods->Actsiz.size();
        //if (cntActsiz>0) MActsizLabel->setText(W2U(Int2Str(cntActsiz)+" - "+ CurrentGoods->Actsiz[cntActsiz-1]));
        //if (cntActsiz>0) MActsizLabel->setText(W2U("Акцизов: "+Int2Str(cntActsiz)));

        if (IsGoods18())
        {
            MActsizLabel->setText(W2U("СПРОСИТЕ УДОСТОВЕРЕНИЕ ЛИЧНОСТИ !"));
        } else
        {
            MActsizLabel->clear();
        }

//;;Log("[FUNC] ShowGoodsInfo [+3]");
	}
	else
	{
//	    ;;Log("[FUNC] ShowGoodsInfo [+4]");
		MItemCode->clear();
		MItemName->clear();
		MItemNumber->clear();
        MItemSBarcode->clear();
		MItemCount->clear();

		MItemPrice->clear();
		MItemSum->clear();
		MItemDiscount->clear();

		MDiscCard->clear();

		MActsizLabel->clear();

		MCertCard->clear();
		MCertDiscount->clear();
		MCertPrePay->clear();
	}
//;;Log("[FUNC] ShowGoodsInfo [+5]");
    if (CurrentCustomer!=0)
    {
        //string CardStr = LLong2Str(CurrentCustomer);
        string CardStr = Card2Str(CurrentCustomer);
        if (IsHandDiscounts())
        {
             CardStr="* "+CardStr;
             lbHandDiscCard->setText(W2U ("* настроенные скидки"));
        }
        else
        {
            lbHandDiscCard->setText(tr(""));
        }

        MDiscCard->setText( CardStr.c_str() );


        if (NeedCert)
        {
            lbDiscCard->hide();
            lbBonusCard->show();

            lbNeedCert->setText(W2U ("* бонусная карта"));
        }
        else
        {
            lbBonusCard->hide();
            lbDiscCard->show();

            lbNeedCert->setText(tr(""));
        }

    }
    else
    {
        MDiscCard->setText(tr( "" ));
        lbHandDiscCard->setText(tr(""));
        lbNeedCert->setText(tr(""));
    }
//;;Log("[FUNC] ShowGoodsInfo [+6]");
    if (CurrentAddonCustomer!=0)
        MAddonDiscCard->setText( LLong2Str(CurrentAddonCustomer).substr(6).c_str() );
    else
        MAddonDiscCard->setText(tr( "" ));


// Баланс текущей бонусной карты
/*    if (CurrentBonusBalans!=0)
        MAddonDiscCard->setText( GetRounded(CurrentBonusBalans,0.01).c_str() );
    else
        MAddonDiscCard->setText(tr( "" ));
*/
    if (Cheque->CertificateSumm!=0)
        MCertDiscount->setText( GetRounded(Cheque->CertificateSumm,0.01).c_str() );
    else
        MCertDiscount->setText(tr( "" ));

    if (Cheque->PrePayCertificateSumm!=0)
        MCertPrePay->setText( GetRounded(Cheque->PrePayCertificateSumm,0.01).c_str() );
    else
        MCertPrePay->setText(tr( "" ));

    if (Cheque->BankBonusSumm!=0)
        MBankBonusDiscount->setText( GetRounded(Cheque->BankBonusSumm,0.01).c_str() );
    else
        MBankBonusDiscount->setText(tr( "" ));

    // Бонусы
    if (Cheque->BonusSumm!=0)
        MBankBonusDiscount->setText( GetRounded(Cheque->BonusSumm,0.01).c_str() );
    else
        MBankBonusDiscount->setText(tr( "" ));


    //MBankBonusCard->setText( (Cheque->BankBonusString).c_str() );
    MBankBonusCard->setText( GetRounded(Cheque->GetSumBonusAdd(),0.01).c_str() );

    MCertCard->setText( W2U(Cheque->CertificateString) );


//;;Log("[FUNC] ShowGoodsInfo [+7]");

	MTotalSum->setText(GetTotalSum().c_str());

	eTotalDiscount->setText(GetRounded(Cheque->GetDiscountSum(),0.01).c_str());

	if (fabs(Cheque->GetDiscountSum())>epsilon)
	{
	    eTotalDiscount->show();
	    lbTotalDiscount->show();
	}
	else
	{
	    eTotalDiscount->hide();
	    lbTotalDiscount->hide();
	}

	if (fabs(Cheque->GetSum())>epsilon)
	{
		if (!IsCurrentGoodsEmpty)
			CustomerDisplay->DisplayLines(CurrentGoods->ItemName.c_str(),
														("Общая сумма "+GetTotalSum()).c_str());
		else
			CustomerDisplay->DisplayLines("",("Общая сумма "+GetTotalSum()).c_str());
	}
	else
	{
		if (!IsCurrentGoodsEmpty)
			CustomerDisplay->DisplayLines(CurrentGoods->ItemName.c_str(),"");
		else
			CustomerDisplay->DisplayLines("Добро пожаловать!","");
	}

	InfoCLPay->setText(tr( "" ));
	//;;Log("[FUNC] ShowGoodsInfo [END]");
}

string WorkPlace::GetTotalSum(void)
{
	return GetRounded(Cheque->GetSumToPay(),0.01);//get cheque sum+current goods sum
}

void WorkPlace::InitWorkPlace(bool clear)//erase cashman's workplace
{
    //;;Log("[FUNC] InitWorkPlace [START]");

	CashReg->GetNewGoods();
	if (!IsChequeInProgress)
	{
	    Log("InitWorkPlace");
		ChequeList->clear();
		CashReg->ClearCurrentCheck();
		Cheque->Clear();
		EditLine->clear();
		CashReg->ClearPayment();

		//
		CurrentGoods = &CurGoods;
		CurGoods.ItemCount = 1;

		IsCurrentGoodsEmpty=true;

        MDiscCard->setText(tr( "" ));

		MTotalSum->setText("0,00");
		lbTotalSum->show();
		MTotalSum->show();

		lbTotalChange->hide();
		MTotalChange->hide();
		MTotalChange->setText("0,00");

        SumRestoreBankBonus=0;
		PaymentKeyPress=0;
		IsChequeInProgress=true;
		GoodsWeightChecked=false;
		CurrentCustomer=0;
		CurrentCertificate=0;
		CurrentAddonCustomer=0;

		CurrentBonusBalans = 0;

		NeedCert=0;
		NeedActionVTB = false;

		//KeepCustomer=0;// 07/09/07
		DiscountType="";
		DiscountTypeCustomer="";
		CashReg->LoadDiscountTypeFromDB(CurrentCustomer,&DiscountType);
		DiscountTypeCustomer=DiscountType;
		BarcodeCard=NULL;

        bonus_pay_id="";
        CurrentActiveBonus=0;
        SummaBonusAction = 0;
        StatusBonusCard ="";

		ShowGoodsInfo();

//Log("++ InitWorkPlace PCX->InitResult()");
		CashReg->WPCX->InitResult();
//Log("-- InitWorkPlace PCX->InitResult()");

        FindInDB = false;

        ActionShowMsg=true;

        CustomerByPhone=false;

        NeedChangeCard2VIP = false;
        NeedChangeCard2SuperVIP = false;

	}
	else
    {
        //+dms
        if (clear) // просто очистим надписи на форме
        {

            ShowGoodsInfo();

//            MDiscCard->setText(tr( "" ));
//            MCertCard->setText(tr( "" ));
//            MCertDiscount->setText(tr( "" ));
//
//            MTotalSum->setText("0,00");
//            lbTotalSum->show();
//            MTotalSum->show();
//
//            lbTotalChange->hide();
//            MTotalChange->hide();
//            MTotalChange->setText("0,00");

        }
        //-dms
    }
    //;;Log("[FUNC] InitWorkPlace [END]");
}

bool WorkPlace::AddGoods(int code)
{
	int ret;
	if (b_uncorrectend) //
		return false; //
	if (!EditLineCheck(""))
		return false;

	ret=CashReg->AddGoods(&CurGoods,code);
	if (ret == 1 )//неправильная цена товара
	{
		ShowMessage("Неправильная цена товара! Сообщите товароведу!");
		return false;
	}

	if (ret==0)
	{
		try
		{

            string resCheckSugar = "";
			if(CheckSugar(&resCheckSugar, CurGoods.ItemCode, CurGoods.ItemCount))
			{
                bool oldscanersuspend = Scanner->suspend;
                Scanner->suspend = true;
			    ShowMessage("ТОВАР НЕ ДОБАВЛЕН\n\n"+resCheckSugar);
			    Scanner->suspend = oldscanersuspend;
			    return false;
            }


            long long CertNumber = 0;
            string strLock = CurGoods.Lock;
		    if (CashReg->GoodsLock(strLock, GL_Manager) == GL_Manager)
            { // продажа с запросом ШК менеджера

              CertNumber = NotCorrect;
              do{
                  CertNumber = CheckBarcode(Manager, "Продажа товара с кодом менеджера \n\n"+Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName+"\n\nВведите ШК менеджера!");

                  ;;Log((char*)("CertNumber="+LLong2Str(CertNumber)).c_str());

                  if (CertNumber==PressEscape) {
                      ;;Log("Отмена ввода ШК менеджера");
                      return false;
                  }


              }
              while ((CertNumber==NotCorrect) || (CertNumber==NoPasswd));
            }


            // + EGAIS
            int TextileCnt = 0;
            int MilkCnt = 0;
            int AlcoCnt = 0;
#ifdef ALCO_VES
            int PivoRazlivCnt = 0;
#endif
            string ActsizBarcode = "";
            strLock = CurGoods.Lock;

            vector<string> ListActsizBarcodes;
            ListActsizBarcodes.clear();

// Возможность отбития без акциза вводилась на переходный период
// Сейчас все алко-товары проходят по ЕГАИС со считываем акциза
// Проверку отключаю
//            if (CashReg->EgaisSettings.ModePrintQRCode>-1)
            {
                if (CashReg->GoodsLock(strLock, GL_AlcoEGAIS) == GL_AlcoEGAIS)
                { // Продажа алкогольной продукции

                    AlcoCnt = (CurGoods.ItemAlco>0)?CurGoods.ItemAlco:1;
                    int errAct=0;

                    for (int j=0;j<AlcoCnt;j++)
                    {
                      errAct=0;

                      ActsizBarcode = "";

                      errAct = CashReg->CheckActsizBarcode(&ActsizBarcode, "Позиция # "+Int2Str(j+1)+" из "+Int2Str(AlcoCnt)+" \n\n"+ Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName, Cheque, ListActsizBarcodes);

                      if (errAct) return false;

                      ListActsizBarcodes.insert(ListActsizBarcodes.end(), ActsizBarcode);

                    }

                    //int errAct = CashReg->CheckActsizBarcode(&ActsizBarcode, Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName, Cheque);
                    //
                }

                 else {
                     if (CashReg->GoodsLock(strLock, GL_TextileEGAIS) == GL_TextileEGAIS)
                        {

                              ActsizBarcode = "";

                              Scanner->allsymbols = true;
                              int errAct = CashReg->CheckActsizBarcodeDataMatrix(&ActsizBarcode, "Позиция # 1 из 1 \n\n"+ Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName, Cheque, ListActsizBarcodes, AT_TEXTILE);
                              Scanner->allsymbols = false;

                              if (errAct) return false;

                              ListActsizBarcodes.insert(ListActsizBarcodes.end(), ActsizBarcode);

                              TextileCnt = 1;

                        }
                     else {
                         if (CashReg->GoodsLock(strLock, GL_MilkEGAIS) == GL_MilkEGAIS)
                            {

                             Log("Milk Datamatruks");

                                  ActsizBarcode = "";

                                  Scanner->allsymbols = true;
                                  int errAct = CashReg->CheckActsizBarcodeDataMatrix(&ActsizBarcode, "Позиция # 1 из 1 \n\n"+ Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName, Cheque, ListActsizBarcodes, AT_MILK);
                                  Scanner->allsymbols = false;

                                  if (errAct) return false;

                                  ListActsizBarcodes.insert(ListActsizBarcodes.end(), ActsizBarcode);

                                  MilkCnt = 1;

                            }
#ifdef ALCO_VES
                          else{
                             if (CashReg->GoodsLock(strLock,GL_BeerEGAIS) == GL_BeerEGAIS)
                             {

                                 Log("ALCOVES Datamatruks");

                                 ActsizBarcode = "";

                                 Scanner->allsymbols = true;
                                 int errAct = CashReg->CheckActsizBarcodeDataMatrix(&ActsizBarcode, "Позиция # 1 из 1 \n\n"+ Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName, Cheque, ListActsizBarcodes, AT_PIVO_RAZLIV);
                                 Scanner->allsymbols = false;

                                 if (errAct) return false;

                                 ListActsizBarcodes.insert(ListActsizBarcodes.end(), ActsizBarcode);

                                 PivoRazlivCnt = 1;
                             }
                         }
#endif
                    }
                }
            }
            // - EGAIS

			CurrentGoods=&CurGoods;
            CurrentGoods->ItemSecondBarcode = CertNumber>0?LLong2Str(CertNumber):"";
			CurrentGoods->ItemDepartment	=CashReg->ConfigTable->GetLongField("DEPARTMENT");
			CurrentGoods->ItemNumber=Cheque->GetSize()+1;
	    		CurrentGoods->ItemFlag=SL;
			CurrentGoods->GuardCode=-1;
			CurrentGoods->PaymentType=CashPayment;
			time(&(CurrentGoods->tTime));

			CurrentGoods->KKMNo=0;
			CurrentGoods->KKMSmena=0;
			CurrentGoods->KKMTime=0;

            // + EGAIS
			//CurrentGoods->ItemAlco = isAclo;
			//CurrentGoods->ItemActsizBarcode = ActsizBarcode;

            CurrentGoods->ItemAlco = AlcoCnt;
            CurrentGoods->ItemTextile = TextileCnt;
            CurrentGoods->ItemMilk = MilkCnt;
#ifdef ALCO_VES
            CurrentGoods->ItemPivoRazliv = PivoRazlivCnt;
#endif
            CurrentGoods->Actsiz.clear();
            for (int j=0;j<ListActsizBarcodes.size();j++)
                CurrentGoods->Actsiz.insert(CurrentGoods->Actsiz.end(), ListActsizBarcodes[j]);

			CurrentGoods->ItemQRBarcode = "";
            // - EGAIS

			//CurrentGoods->Section=CashReg->GetSection(CurrentGoods->Lock);

			if(!Cheque->GetCount())
				CashReg->m_vem.StartDoc(1, CashReg->GetLastCheckNum()+1);

			if ((PriceChecker==NULL) && (FreeSum==NULL))
				IsCurrentGoodsEmpty=false;

            AppendRecord2Check();

			CashReg->m_vem.AddGoods(CurrentGoods->ItemNumber,CurrentGoods,Cheque->GetCalcSum(),CurGoods.InputType);

            //CashReg->LogAddGoods(CurrentGoods);

			return true;
		}
		catch(cash_error& er)
		{
			CashReg->LoggingError(er);
			return false;
		}
	}
}

void WorkPlace::ProcessDiscount(GoodsInfo *CurGoods,double CheckSum,double CheckSum_WithoutAction)
{
	CashReg->ProcessDiscount(CurGoods,CheckSum,CurrentCustomer,DiscountType, CurrentAddonCustomer, CheckSum_WithoutAction);
}

void WorkPlace::ProcessDiscount(FiscalCheque* CurCheque)
{
//;;Log("[Func] ProcessDiscount [Begin]");

	if (!CurCheque)
		CurCheque = Cheque;
	if (!CurCheque) return;



    SummaBonusAction=0;

    // + dms ===== 2014-11-14 =====  Пересчет суммовой скидки =======
    CurCheque->DiscountSumm = 0;
    CurCheque->DiscountSummNotAction = 0;
    // + dms ===== 2014-11-14 =====

	FiscalCheque *tmpCheque;
	tmpCheque = new FiscalCheque;
	try
	{


/*
// ====== ============================== =======
// ====== АКЦИЯ ЗАВЕРШИЛАСЬ (2014-11-20) =======
// ====== ============================== =======

            // Акция : при покупке 6 бутылок пива ( коды см. ниже)....
            // сумка-холодильник в подарок

            bool BUD=false;

//;;Log("= START DISC =");

                FiscalCheque *tmpCheque2;
                tmpCheque2 = new FiscalCheque;

                    //копируем в промежуточный чек для расчета скидок на превышение по весовому товару
                    for (int i=0;i<CurCheque->GetCount();i++)
                    {
                        tmpCheque2->Insert((*CurCheque)[i]);
                    }
                    tmpCheque2->ReductionByCode(CashReg->KKMNo,CashReg->KKMSmena,CashReg->KKMTime,true);

                    //расчитываем скидки по каждой позиции товара в промежуточном чеке
                    for (int i=0;i<tmpCheque2->GetCount();i++)
                    {
                        GoodsInfo g1=(*tmpCheque2)[i];

                        if (
                        (
                        (g1.ItemCode==789158)
                        ||
                        (g1.ItemCode==789159)
                        ||
                        (g1.ItemCode==778701)
                        ||
                        (g1.ItemCode==789161)
                        )

                        && (g1.ItemCount>=6)

                        && (g1.ItemFlag==SL)

                        )
                        {
                            ;;Log("= BUD =");
                             BUD=true;
                        }

                    }

            if ((ActionShowMsg)&&(BUD))
            {
               ShowMessage("            В честь празднования Октоберфеста\n\
      при покупке 6 единиц пива Францисканер \n\
      или Шпатен Мюнхен Вы получаете\n\n\
              СУМКУ-ХОЛОДИЛЬНИК БАД \n\n\
                        В ПОДАРОК!!!");

                   ActionShowMsg = false;
            }

// ====== ============================== =======
// КОНЕЦ.АКЦИЯ  СУМКА-ХОЛОДИЛЬНИК
// ====== ============================== =======
*/

/* Акция завершена
            // Акция : при покупке шпика
            //  - Шпик ПО-ВЕНГЕРСКИ ООО Гастроном
            //  - Шпик ПО-ДОМАШНЕМУ ООО Гастроном
            //  - Смалец с красным перцем ООО Гастроном
            //  - Смалец с черным перцем ООО Гастроном

            //  Водка КАЛАШНИКОВ 3 звезды 40% 0,7 (124986)
            //  считывается со специальной ценой (спец. штрих-код на кассе)

            bool shpik=false;
            bool vodka=false;



            if  (vodka)
            {
                ;;Log("=5.1=");

                for (int i=0;i<CurCheque->GetCount();i++)
                {
                    GoodsInfo g1=(*CurCheque)[i];

                    Log((char*)("["+g1.ItemBarcode+"]").c_str());
                    if (shpik)
                    {
                        if ((g1.ItemCode==124986)
                        && (g1.ItemBarcode.substr(0,8) != "27084274"))
                       // && (g1.ItemBarcode.substr(0,12) == "460703219338"))
                        {

                          g1.ItemBarcode="27084274";
                          CurCheque->SetAt(i,g1);

                            string LogStr;
                            LogStr = "АКЦИЯ ШПИК + ВОДКА КАЛАШНИКОВ = СПЕЦ. ЦЕНА";
                            Log((char*)LogStr.c_str());

                          //  ;;ShowMessage("АКЦИЯ ШПИК + ВОДКА КАЛАШНИКОВ = СПЕЦ. ЦЕНА");

                        }
                    }
                    else
                    {
                        if ((g1.ItemCode==124986)
                        && (g1.ItemBarcode.substr(0,8) == "27084274"))
                        {
                            Log("= УДАЛЕН ТОВАР ИЗ АКЦИИ ШПИК+ВОДКА=");
                          g1.ItemBarcode="4607032193388";
                          CurCheque->SetAt(i,g1);
                        }
                    }
                }


            }
*/
//;;Log("= END =");



/* АКЦИЯ ЗАВЕРШЕНА 2014-09-24
            // Акция : при покупке 2x товаров P&G выдать сообщение о подарке
            //  Подарок: вода ()

            bool voda=false;
            bool InAaction=false;

//;;Log("= START DISC =");

                FiscalCheque *tmpCheque1;
                tmpCheque1 = new FiscalCheque;

                    //копируем в промежуточный чек для расчета скидок на превышение по весовому товару
                    for (int i=0;i<CurCheque->GetCount();i++)
                    {
                        tmpCheque1->Insert((*CurCheque)[i]);
                    }

                    string ActCodes = "[762635][768571][768572][745410][745409][720208][736926][785099][785098][765912][765916][765911][765919][765910][765915][765918][765914][765921][765917][765913][765920][776546][776695][776696][776697][776698][776699][776700][776701][776702][776703][776704][776705][743558][743559][743560][743561][743562][743563][743564][743565][743566][743567][743568][743569][743570][743571][743572][743573][743574][55588][55589][55583][55584][50560][50565][50566][50567][55593][55596][768570][55597][84093][711266][711265][50563][50562][55592][43849][43743][110612][43647][43697][43653][43859][43661][142415][43829][43758][43676][43789][43739][43618][43608][43628][24007][743582][743188][745421][738465][738467][738470][123249][115341][139441][127046][125424][118734][104345][36890][33788][36887][36888][745414][706954][745422][738466][738471][774868][123248][118737][127047][36895][104346][745413][36894][706953][743580][118720][43984][55564][55551][768576][96475][104338][757753][774865][743548][121088][774848][782333][143336][143335][760858][760859][143337][774866][774869][743174][706950][743170][55540][743550][745420][757759][780876][780856][760862][760863][762634][118719][43968][55561][80343][55552][80351][96473][104336][118713][743169][757755][757754][774864][743549][50576][743555][743553][55529][743554][54455][706949][121086][706951][774849][782332][745415][776710][762633][738461][743176][738475][143332][757782][743576][743175][143331][760860][760861][743177][745419][143330][774867][774870][768577][743556][762629][743166][743167][743171][768573][774861][774862][774863][782334][757781][743425][706961][774846][96470][44186][776709][776708][139396][782335][147877][757780][745411][61252][768574][774854][774858][774850][774859][774851][774855][774852][774856][774853][774857][774860][757767][757762][768575][118743][782336][776706][776707][743229][743231][743228][743546][743227][743230][89909][706958][706957][706956][706959][774847][743224][743232][743233][706955][738483][738484][738490][738491][738492][738494]";

                    int CntPG=0;
                    //расчитываем скидки по каждой позиции товара в промежуточном чеке
                    for (int i=0;i<tmpCheque1->GetCount();i++)
                    {
                        GoodsInfo g1=(*tmpCheque1)[i];

                        if (g1.ItemFlag!=SL) continue;

                        string ItemStr = "["+Int2Str(g1.ItemCode)+"]";

                        string::size_type pos_found = ActCodes.find(ItemStr);

                        if(pos_found!=string::npos)
                        {
                            ;;Log((char*)Int2Str(pos_found).c_str());
                            CntPG+=g1.ItemCount;
                        }

                        if (CntPG>=2)
                        {
//                            ;;Log("= NEED VODA =");
                            InAaction=true;
                        }

                      if (

                         (g1.ItemCode==797549)

                         && (g1.ItemFlag==SL)

                         )
                        {
                            ;;Log("= VODA =");
                            voda = true;
                        }

                    }


            if ((InAaction) && (!voda))
            {
                ;;Log("= NEED VODA =");
                ShowMessage("ВЫДАТЬ ПОДАРОК AQUA MINERALE");
            }
//;;Log("= END =");
*/



// ====== ============================== =======
// АКЦИЯ При покупке 2-х разных товаров в чеке - акционная цена на оба товара
// ====== ============================== =======

///* Акция
            // Акция :
//> При единовременной покупке (в одном чеке) товаров:
//> 826578  Аперитив  АПЕРОЛЬ  11%  0,7л  с/б Давиде Кампари-Милано ИТАЛИЯ
//> (регулярная цена 1599, акционная цена 1199)
//> 731558  Вино  МАРТИНИ ПРОСЕКО игристое сухое белое 0,75л с/б MARTINI &
//> ROSSI S.P.A. ИТАЛИЯ (регулярная цена 1499, акционная цена 999,9)
//
//> покупатель  получает  скидку до цен 999,9 и 799,9 соответственно, т.е.
//> общая цена набора 1799,9 вместо 3099
//
// Товар считывается со специальной ценой (спец. штрих-код на кассе)
//826578 Аперитив АПЕРОЛЬ 11% 0,7л с/б Давиде Кампари-Милано ИТАЛИЯ - добавили шк 27209875  (8002230000302)
//731558 Вино МАРТИНИ ПРОСЕКО игристое сухое белое 0,75л с/б MARTINI & ROSSI S.P.A. ИТАЛИЯ - добавили шк 27209882  (8000570552505)
            bool action_aperol = true;

            int aperol=0;
            int martini=0;

            if  (action_aperol)
            {

                for (int i=0;i<CurCheque->GetCount();i++)
                {
                    GoodsInfo g1=(*CurCheque)[i];

                    if ((g1.ItemFlag!=SL) && (g1.ItemFlag!=RT)) continue;

                    if (g1.ItemCode==826578)
                    {
                        aperol++;
                    }

                    if (g1.ItemCode==731558)
                    {
                        martini++;
                    }
                }

                aperol = min(aperol, martini);
                martini = min(aperol, martini);

                for (int i=0;i<CurCheque->GetCount();i++)
                {
                    GoodsInfo g1=(*CurCheque)[i];

                    if ((g1.ItemFlag!=SL) && (g1.ItemFlag!=RT)) continue;

                    if (g1.ItemCode==826578)
                    {
                        if (aperol > 0)
                        {

                            if (g1.ItemBarcode.substr(0,8) != "27209875")
                            {
                                Log("= ДОБАВЛЕН ТОВАР АПЕРОЛЬ В АКЦИЮ АПЕРОЛЬ+МАРТИНИ =");

                                g1.ItemBarcode="27209875";
                                CurCheque->SetAt(i,g1);
                            }

                            aperol--;

                        }
                        else
                        {

                            if (g1.ItemBarcode.substr(0,8) == "27209875")
                            {
                                Log("= УДАЛЕН ТОВАР АПЕРОЛЬ ИЗ АКЦИИ АПЕРОЛЬ+МАРТИНИ =");

                                g1.ItemBarcode="8002230000302";
                                CurCheque->SetAt(i,g1);
                            }

                        }
                    }

// мартини
                    if (g1.ItemCode==731558)
                    {
                        if (martini > 0)
                        {

                            if (g1.ItemBarcode.substr(0,8) != "27209882")
                            {
                                Log("= ДОБАВЛЕН ТОВАР МАРТИНИ В АКЦИЮ АПЕРОЛЬ+МАРТИНИ =");

                                g1.ItemBarcode="27209882";
                                CurCheque->SetAt(i,g1);
                            }

                            martini--;

                        }
                        else
                        {

                            if (g1.ItemBarcode.substr(0,8) == "27209882")
                            {
                                Log("= УДАЛЕН ТОВАР МАРТИНИ ИЗ АКЦИИ АПЕРОЛЬ+МАРТИНИ =");

                                g1.ItemBarcode="8000570552505";
                                CurCheque->SetAt(i,g1);
                            }

                        }
                    }
                }



            }

// slair
/*
            bool action_IONOS = true;

            int IONOS_white=0;
            int IONOS_pink=0;

            if  (action_IONOS)
            {

                for (int i=0;i<CurCheque->GetCount();i++)
                {
                    GoodsInfo g1=(*CurCheque)[i];

                    if ((g1.ItemFlag!=SL) && (g1.ItemFlag!=RT)) continue;

                    if (g1.ItemCode==893831)
                    {
                        IONOS_white++;
                    }

                    if (g1.ItemCode==893833)
                    {
                        IONOS_pink++;
                    }
                }

                IONOS_white = min(IONOS_white, IONOS_pink);
                IONOS_pink = min(IONOS_white, IONOS_pink);

                for (int i=0;i<CurCheque->GetCount();i++)
                {
                    GoodsInfo g1=(*CurCheque)[i];

                    if ((g1.ItemFlag!=SL) && (g1.ItemFlag!=RT)) continue;

                    if (g1.ItemCode==893831)
                    {
                        if (IONOS_white > 0)
                        {

                            if (g1.ItemBarcode.substr(0, 13) != "7201015011547")
                            {
                                Log("= ДОБАВЛЕН ТОВАР IONOS_white("
                                    + g1.ItemBarcode.substr(0, 13)
                                    + ") В АКЦИЮ action_IONOS =");

                                g1.ItemBarcode="7201015011547";
                                CurCheque->SetAt(i,g1);
                            }

                            IONOS_white--;

                        }
                        else
                        {

                            if (g1.ItemBarcode.substr(0, 13) == "7201015011547")
                            {
                                Log("= УДАЛЕН ТОВАР IONOS_white("
                                    + g1.ItemBarcode.substr(0, 13)
                                    + ") ИЗ АКЦИИ action_IONOS =");

                                g1.ItemBarcode="5201015011547";
                                CurCheque->SetAt(i,g1);
                            }

                        }
                    }

// IONOS_pink
                    if (g1.ItemCode==893833)
                    {
                        if (IONOS_pink > 0)
                        {

                            if (g1.ItemBarcode.substr(0, 13) != "7201015011585")
                            {
                                Log("= ДОБАВЛЕН ТОВАР IONOS_pink("
                                    + g1.ItemBarcode.substr(0, 13)
                                    + ") В АКЦИЮ action_IONOS =");

                                g1.ItemBarcode="7201015011585";
                                CurCheque->SetAt(i,g1);
                            }

                            IONOS_pink--;

                        }
                        else
                        {

                            if (g1.ItemBarcode.substr(0, 13) == "7201015011585")
                            {
                                Log("= УДАЛЕН ТОВАР IONOS_pink("
                                    + g1.ItemBarcode.substr(0, 13)
                                    + ") ИЗ АКЦИИ action_IONOS =");

                                g1.ItemBarcode="5201015011585";
                                CurCheque->SetAt(i,g1);
                            }

                        }
                    }
                }



            }
*/
//;;Log("= END =");


// ====== ============================== =======
// АКЦИЯ При покупке 1-го товара - акционная цена на другой товар
// ====== ============================== =======

///* Акция
            // Акция :
//>> необходимо прогрузить акцию: при покупке  114521 Виски ФЕЙМОС ГРАУЗ
//>> 0,7л с/б Мэтью Глог Энд Сант ЛТД, ВЕЛИКОБРИТАНИЯ
//>> сок  024979  Сок РИЧ яблочный осветл. т/п 1л Мултон Rich за 1 РУБ


//ШК сгенерировала 27214503
//Обычный ШК (4607042439216) цена 99.90
            bool action_wiski_sok = true;

            int wiski=0;
            int sok=0;

            if  (action_wiski_sok)
            {

                for (int i=0;i<CurCheque->GetCount();i++)
                {
                    GoodsInfo g1=(*CurCheque)[i];

                    if ((g1.ItemFlag!=SL) && (g1.ItemFlag!=RT)) continue;

                    if (g1.ItemCode==114521)
                    {
                        wiski++;
                    }

                    if (g1.ItemCode==24979)
                    {
                        sok++;
                    }
                }

                wiski = min(wiski, sok);
                sok = min(wiski, sok);

                for (int i=0;i<CurCheque->GetCount();i++)
                {
                    GoodsInfo g1=(*CurCheque)[i];

                    if ((g1.ItemFlag!=SL) && (g1.ItemFlag!=RT)) continue;

                    if (g1.ItemCode==24979)
                    {
                        if (wiski > 0)
                        {

                            if (g1.ItemBarcode.substr(0,8) != "27214503")
                            {
                                Log("= ДОБАВЛЕН ТОВАР СОК В АКЦИЮ ВИСКИ+СОК =");

                                g1.ItemBarcode="27214503";
                                CurCheque->SetAt(i,g1);
                            }

                            wiski--;

                        }
                        else
                        {

                            if (g1.ItemBarcode.substr(0,8) == "27214503")
                            {
                                Log("= УДАЛЕН ТОВАР СОК ИЗ АКЦИИ ВИСКИ+СОК =");

                                g1.ItemBarcode="4607042439216";
                                CurCheque->SetAt(i,g1);
                            }

                        }
                    }
                }
            }

//;;Log("= END =");




		//копируем в промежуточный чек для расчета скидок на превышение по весовому товару
		for (int i=0;i<CurCheque->GetCount();i++)
		{
			tmpCheque->Insert((*CurCheque)[i]);
		}
		tmpCheque->Reduction(CashReg->KKMNo,CashReg->KKMSmena,CashReg->KKMTime,true,true);

		CashReg->ProcessPayment();

        double CheckSum_WithoutAction = CurCheque->GetCalcSum_NotAction9();
        if (CheckSum_WithoutAction<0) CheckSum_WithoutAction=0;


		//расчитываем скидки по каждой позиции товара в промежуточном чеке
		for (int i=0;i<tmpCheque->GetCount();i++)
		{
			GoodsInfo g=(*tmpCheque)[i];
			g.ItemExcessPrice = g.ItemFullPrice;
			g.ItemCalcPrice   = g.ItemFullPrice;
			g.ActionGoods     = 0;
			g.DiscExcessFlag  = false; // флаг скидки по превышению

// Задача: акция - при цена 59,9 при покупке ЛЮБЫХ 3-х наименований из заданного списка товаров
// Количество считаем общее по группе, например, если в знаке скидки 16 (P0Q0) стоит цифра
// то ItemCount = суммарное количество всех товаров в чеке с такой же цифрой
//например, Товар№1 = 1, Товар№2 = 2. В ItemCount запишем = 3, чтобы сработала скидка по превышению
            g.ItemCount       = CashReg->GetItemCountP0Q0(&g, CurCheque, 16);

			double lCheckSumma = tmpCheque->GetCalcSum() - tmpCheque->DiscountSumm - tmpCheque->DiscountSummNotAction - tmpCheque->CertificateSumm - tmpCheque->BankBonusSumm  - tmpCheque->BonusSumm;

			ProcessDiscount(&g,lCheckSumma>0?lCheckSumma:0, CheckSum_WithoutAction);
			tmpCheque->SetAt(i,g);
		}

/*
    Log("tmpCheque begin\n");
    for (int i=0;i<tmpCheque->GetCount();i++)
    {
        GoodsInfo g=(*tmpCheque)[i];
        char tmpStr[1024];
        sprintf(tmpStr,"--- %d --- Code=%d Count=%.2f FullPrice=%8.2f ExcessPrice=%8.2f Price=%8.2f   Action = %d",i,g.ItemCode,g.ItemCount,g.ItemFullPrice,g.ItemExcessPrice,g.ItemPrice,g.ActionGoods );
        Log(tmpStr);
    }
    Log("tmpCheque end\n");
*/
		//присваиваем цену по превышению из промежуточного чека
		//for (int i=0;i<CurCheque->GetCount();i++)
		for (int i=CurCheque->GetCount()-1;i>=0;i--)
		{
          GoodsInfo gi=(*CurCheque)[i];

          GoodsInfo gj;
          gi.ItemExcessPrice	=gi.ItemFullPrice;
          gi.ItemCalcPrice	    =gi.ItemFullPrice;

//if (gi.ItemName.substr(0,1)=="@"){
//;;Log((char*)("!!!TEST STEP _ 1 !!!  Code="+Int2Str(gi.ItemCode)+". Cnt="+ Double2Str(gi.ItemCount)).c_str());
//}

          // + dms === 2010-03-30 =====\ спец. код пропускаем
          if (gi.ItemCode != FreeSumCode)
		  {

			for(int j=0;j<tmpCheque->GetCount();j++)
			{
				gj=(*tmpCheque)[j];

				// сторно пропускаем!
				if(
                    gj.DiscExcessFlag
                    &&
                    (gi.ItemCode == gj.ItemCode)
                    &&
                    (
                        (gi.ItemBarcode == gj.ItemBarcode)
                        ||
                        ((gi.ItemPrecision<1) && (gi.ItemBarcode.substr(0,8) == gj.ItemBarcode.substr(0,8)))
                    )
                    &&
                    (gj.ItemFlag!=ST)
                  )
				{

//if (gi.ItemName.substr(0,1)=="@"){
//;;Log((char*)("!!!TEST STEP _ 2 !!!  Code="+Int2Str(gi.ItemCode)+". Cnt="+ Double2Str(gi.ItemCount)+". ActCnt="+ Double2Str(gj.Action_ItemCnt)).c_str());
//}

				if (
				(gi.ItemFlag!=ST)
				&&
				(gj.Action_ItemPrice>0)
				&&
				(gj.Action_ItemCnt>0)
				&&
				(gj.Action_ItemCnt>=gi.ItemCount)
				)
				{
//;;Log("B1");
//;;Log((char*)("!!!IN ACTION!!! Code="+Int2Str(gi.ItemCode)+". Cnt="+ Double2Str(gi.ItemCount)+". ActCnt="+ Double2Str(gj.Action_ItemCnt)).c_str());

                    //gi.ItemCalcPrice	= gj.Action_ItemPrice;

                    // Признак акции СТОП-ЦЕНА
                    gi.Action_ItemPrice	= gj.Action_ItemPrice;

                    gi.ItemExcessPrice	= gj.Action_ItemPrice;

                    gj.Action_ItemCnt   -= gi.ItemCount;

                    tmpCheque->SetAt(j, gj);

//;;Log("E2");
                }
                else
                {

                    gi.ItemCalcPrice	= gj.ItemCalcPrice;
                    gi.ItemExcessPrice	= gj.ItemExcessPrice;

                    // каждый n-й бесплатно!!!
					if ((gj.ActionGoods>0) && (gi.ItemFlag!=ST))
                    {
                        int CntAction       = min(gj.ActionGoods, Double2Int(gi.ItemCount));
                        // пересчитываем цену за вычетом кол-ва акционного (бесплатного) товара
                        //gi.ItemExcessPrice *= (gi.ItemCount-CntAction)/gi.ItemCount;
                        gi.ItemExcessPrice = RoundToNearest(gi.ItemExcessPrice*(gi.ItemCount-CntAction)/gi.ItemCount, 0.01);

                        gi.ItemCalcPrice    = gi.ItemExcessPrice;
                        gi.ActionGoods      = CntAction;

                        gj.ActionGoods     -= CntAction;
                        if (gj.ActionGoods==0) gj.ActionGoods=-1;

                        tmpCheque->SetAt(j, gj);

                    }
                    else //отметим как акционный
                        if (gj.ActionGoods<0)
                            gi.ActionGoods = gj.ActionGoods;
                }
				} //if
			 } // for

	  	   } // если не спец. код

			CurCheque->SetAt(i,gi);
		}

        CurCheque->Lock = false;

/*
    Log("CurCheque --- begin\n");
    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];
        char tmpStr[1024];
        sprintf(tmpStr,"--- %d --- Code=%d Count=%.2f FullPrice=%8.2f ExcessPrice=%8.2f Price=%8.2f   Action = %d",i,g.ItemCode,g.ItemCount,g.ItemFullPrice,g.ItemExcessPrice,g.ItemPrice,g.ActionGoods );
        Log(tmpStr);
    }
    Log("CurCheque --- end\n");
*/

		//расчитываем скидки по каждой позиции товара
		for (int i=0;i<CurCheque->GetCount();i++)
		{
			GoodsInfo g=(*CurCheque)[i];
			//ProcessDiscount(&g,(CurCheque->GetCalcSum()-CurCheque->DiscountSumm-CurCheque->CertificateSumm)>0?(CurCheque->GetCalcSum()-CurCheque->DiscountSumm-CurCheque->CertificateSumm):0);

            // ! Скидка без учета сертификата ! //

            double CheckSum_Full = CurCheque->GetCalcSum()-CurCheque->DiscountSumm- min(CurCheque->DiscountSummNotAction, CurCheque->GetSumNotAction_New());
            if (CheckSum_Full<0) CheckSum_Full=0;

            double CheckSum_WithoutAction = CheckSum_Full - CurCheque->GetCalcSum_Action9();
            if (CheckSum_WithoutAction<0) CheckSum_WithoutAction=0;
			//ProcessDiscount(&g,	(CurCheque->GetCalcSum()-CurCheque->DiscountSumm)>0?(CurCheque->GetCalcSum()-CurCheque->DiscountSumm):0	);

            ProcessDiscount(&g,	CheckSum_Full, CheckSum_WithoutAction);

			CurCheque->SetAt(i,g);
		}
//

/*
    Log("CurCheque +++ begin\n");
    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];
        char tmpStr[1024];
        sprintf(tmpStr,"--- %d --- Code=%d Count=%.2f FullPrice=%8.2f ExcessPrice=%8.2f Price=%8.2f   Action = %d",i,g.ItemCode,g.ItemCount,g.ItemFullPrice,g.ItemExcessPrice,g.ItemPrice,g.ActionGoods );
        Log(tmpStr);
    }
    Log("CurCheque +++ end\n");
*/

// + dms ===== Исправление распределения суммы скидки по позициям чека =====
/// {Было}
//		//размазываем скидки на весь чек по товарам
//		double crDiscountSumm = CurCheque->DiscountSumm + CurCheque->CertificateSumm;
//		for (int i=0;i<CurCheque->GetCount();i++)
//		{
//			double crGoodsDiscountSumm;
//
//			GoodsInfo crGoods = (*CurCheque)[i];
//
//			crGoodsDiscountSumm = RoundToNearest(crGoods.ItemCalcSum*(CurCheque->DiscountSumm+CurCheque->CertificateSumm)/CurCheque->GetCalcSum(),0.01);
//			if (crGoods.ItemSum > crGoodsDiscountSumm)
//			{
//				crGoods.ItemSum	-= crGoodsDiscountSumm;
//				crDiscountSumm 	-= crGoodsDiscountSumm;
//			}
//			else
//			{
//				crGoods.ItemSum	 = 0;
//				crDiscountSumm	-= crGoods.ItemSum;
//			}
//			CurCheque->SetAt(i,crGoods);
//		}

/// {Стало}


// + dms ====================/



// БОНУСЫ

    bool isBonusCard = CashReg->BonusMode(CurrentCustomer);
    {
        double Sum=0;
        for (int i=0;i<CurCheque->GetCount();i++)
        {

            GoodsInfo crGoods = (*CurCheque)[i];
            crGoods.ItemBonusAdd = 0;

            if (isBonusCard)
            {
                Sum = 0;

                if
                (
                    (
                    (crGoods.ItemFlag==SL)||(crGoods.ItemFlag==RT)
                    )
                    &&
                    (
                    (crGoods.ItemCalcPrice - crGoods.ItemPrice > .015)
                    ||
                    (crGoods.ItemCalcSum   - crGoods.ItemSum   > .005)
                    )
                    &&
                    !(crGoods.ItemFullPrice - crGoods.ItemExcessPrice>.005)  // акция по первышению
                )
                {
                    crGoods.ItemBonusAdd = crGoods.ItemCalcSum - crGoods.ItemSum;

                    crGoods.ItemPrice = crGoods.ItemCalcPrice;
                    crGoods.ItemSum   = crGoods.ItemCalcSum;
                }
            }

            CurCheque->SetAt(i,crGoods);

        }
    }



//! СУММОВАЯ СКИДКА НА ЧЕК, КРОМЕ АКЦИОННЫХ ТОВАРОВ
		//распределяем суммовую скидку на весь чек, кроме акционных товаров
		// на акционные товары скидка не распространяется

//		;;Log((char*)("CurCheque->DiscountSumm = "+Int2Str(CurCheque->DiscountSumm)).c_str());
{
		double BaseDiscountSum  = CurCheque->DiscountSummNotAction;
		double BaseCheckSum     = CurCheque->GetSumNotAction_New();

		double AllocDiscountSum = 0;
		double AllocCheckSum    = 0;
		double CurDiscount      = 0;

		for (int i=0;i<CurCheque->GetCount();i++)
		{
			GoodsInfo crGoods = (*CurCheque)[i];

//;;Log((char*)crGoods.ItemName.c_str());

            if (
            ((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT))
            || (crGoods.ItemCode==TRUNC)
            || (crGoods.ItemCode==EXCESS_PREPAY)
            || (CurCheque->GoodInActionByName(crGoods.ItemName))
            )
                continue; // на акционные товары скидка не распространяется

            //;;Log((char*)crGoods.ItemName.c_str());


            if (BaseCheckSum - AllocCheckSum > 0)
                CurDiscount = RoundToNearest((BaseDiscountSum - AllocDiscountSum)/(BaseCheckSum - AllocCheckSum)*crGoods.ItemSum, 0.01);
            else
                CurDiscount = 0;

//;;Log((char*)Int2Str(CurDiscount).c_str());

            AllocCheckSum += crGoods.ItemSum;

            if (crGoods.ItemSum	> CurDiscount)
            {
                crGoods.ItemSum	-= CurDiscount;
                //+ dms ===== сумма скидки по бонусам сбербанка
                //crGoods.ItemBankBonusDisc = CurDiscount;
                //- dms =====/
            }
            else
            {
                CurDiscount = crGoods.ItemSum;
                crGoods.ItemSum	= 0;

                //+ dms ===== сумма скидки по бонусам сбербанка
                //crGoods.ItemBankBonusDisc = CurDiscount;
                //- dms =====/
            }

            AllocDiscountSum += CurDiscount;

			CurCheque->SetAt(i,crGoods);
		}
}
// - dms ====================/


// + dms ====================/

// СУММОВАЯ СКИДКА НА ВЕСЬ ЧЕК, ВКЛЮЧАЯ АКЦИОННЫЕ ТОВАРЫ
		//распределяем суммовую скидку на весь чек

//		;;Log((char*)("CurCheque->DiscountSumm = "+Int2Str(CurCheque->DiscountSumm)).c_str());

		double BaseDiscountSum  = CurCheque->DiscountSumm;
		// + dms ===== 2014-11-14 ===== Скидку распространять на весь товар, включая акционный
		// Было:
		//double BaseCheckSum     = CurCheque->GetSumNotAction();
		// Стало:
        double BaseCheckSum     = CurCheque->GetSum();
		// - dms ===== 2014-11-14 ===== Скидку распространять на весь товар, включая акционный
		double AllocDiscountSum = 0;
		double AllocCheckSum    = 0;
		double CurDiscount      = 0;

		for (int i=0;i<CurCheque->GetCount();i++)
		{
			GoodsInfo crGoods = (*CurCheque)[i];

//;;Log((char*)crGoods.ItemName.c_str());

            if (
            ((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT))
            || (crGoods.ItemCode==TRUNC)
            || (crGoods.ItemCode==EXCESS_PREPAY)

            // + dms ===== 2014-11-14 ===== Скидку распространять на весь товар, включая акционный
            // Было:
            //|| ((crGoods.ItemName.substr(0,1)=="*"))
            // - dms ===== 2014-11-14 ===== Скидку распространять на весь товар, включая акционный
            )
                continue;

            //;;Log((char*)crGoods.ItemName.c_str());


            if (BaseCheckSum - AllocCheckSum > 0)
                CurDiscount = RoundToNearest((BaseDiscountSum - AllocDiscountSum)/(BaseCheckSum - AllocCheckSum)*crGoods.ItemSum, 0.01);
            else
                CurDiscount = 0;

//;;Log((char*)Int2Str(CurDiscount).c_str());

            AllocCheckSum += crGoods.ItemSum;

            if (crGoods.ItemSum	> CurDiscount)
            {
                crGoods.ItemSum	-= CurDiscount;
                //+ dms ===== сумма скидки по бонусам сбербанка
                //crGoods.ItemBankBonusDisc = CurDiscount;
                //- dms =====/
            }
            else
            {
                CurDiscount = crGoods.ItemSum;
                crGoods.ItemSum	= 0;

                //+ dms ===== сумма скидки по бонусам сбербанка
                //crGoods.ItemBankBonusDisc = CurDiscount;
                //- dms =====/
            }

            AllocDiscountSum += CurDiscount;

			CurCheque->SetAt(i,crGoods);
		}
// - dms ====================/

// + dms ====================/
		//распределяем суммовую скидку бонусов Сбербанка
		BaseDiscountSum  = CurCheque->BankBonusSumm;
		BaseCheckSum     = CurCheque->GetSum();
		AllocDiscountSum = 0;
		AllocCheckSum    = 0;
		CurDiscount      = 0;

		for (int i=0;i<CurCheque->GetCount();i++)
		{
			GoodsInfo crGoods = (*CurCheque)[i];

            if (((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT)) || (crGoods.ItemCode==TRUNC) || (crGoods.ItemCode==EXCESS_PREPAY)) continue;

            if (BaseCheckSum - AllocCheckSum > 0)
                CurDiscount = RoundToNearest((BaseDiscountSum - AllocDiscountSum)/(BaseCheckSum - AllocCheckSum)*crGoods.ItemSum, 0.01);
            else
                CurDiscount = 0;

            AllocCheckSum += crGoods.ItemSum;

            if (crGoods.ItemSum	> CurDiscount)
            {
                crGoods.ItemSum	-= CurDiscount;
                //+ dms ===== сумма скидки по бонусам сбербанка
                crGoods.ItemBankBonusDisc = CurDiscount;
                //- dms =====/
            }
            else
            {
                CurDiscount = crGoods.ItemSum;
                crGoods.ItemSum	= 0;

                //+ dms ===== сумма скидки по бонусам сбербанка
                crGoods.ItemBankBonusDisc = CurDiscount;
                //- dms =====/
            }

            AllocDiscountSum += CurDiscount;

			CurCheque->SetAt(i,crGoods);
		}
// - dms ====================/

		//распределяем суммовую скидку и скидку по сертификатам по всему чеку
		BaseDiscountSum  = CurCheque->CertificateSumm;
		BaseCheckSum     = CurCheque->GetSum();
		AllocDiscountSum = 0;
		AllocCheckSum    = 0;
		CurDiscount      = 0;

		for (int i=0;i<CurCheque->GetCount();i++)
		{
			GoodsInfo crGoods = (*CurCheque)[i];

            if (((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT)) || (crGoods.ItemCode==TRUNC) || (crGoods.ItemCode==EXCESS_PREPAY)) continue;

            if (BaseCheckSum - AllocCheckSum > 0)
                CurDiscount = RoundToNearest((BaseDiscountSum - AllocDiscountSum)/(BaseCheckSum - AllocCheckSum)*crGoods.ItemSum, 0.01);
            else
                CurDiscount = 0;

            //+ dms ===== сумма без учета скидки по сертификату
            crGoods.ItemFullDiscSum = crGoods.ItemSum;
			//crGoods.ItemFullDiscPrice=RoundToNearest(crGoods.ItemFullDiscSum/crGoods.ItemCount,0.01);
            //- dms =====/

            AllocCheckSum += crGoods.ItemSum;

            if (crGoods.ItemSum	> CurDiscount)
                crGoods.ItemSum	-= CurDiscount;
            else
            {
                CurDiscount = crGoods.ItemSum;
                crGoods.ItemSum	= 0;
            }

            AllocDiscountSum += CurDiscount;

			CurCheque->SetAt(i,crGoods);
		}
// - dms ====================/


// + dms ====================/
		//распределяем суммовую скидку бонусов
		BaseDiscountSum  = CurCheque->BonusSumm;
		
#ifndef BONUS_LOG_SKIDKA 
		//;;Log("********************************************************************");
		//;;Log("Summa skidki bonus BaseDiscountSum=" + Double2Str(BaseDiscountSum));
		BaseDiscountSum = 306.0;
		isBonusCard = true;
		//;;Log(" BaseCheckSum=" + Double2Str(CurCheque->GetSum()));
		//;;Log("********************************************************************");
#endif
		
		BaseCheckSum     = GetMaxBonusToPay(CurCheque);
		
#ifndef BONUS_LOG_SKIDKA //ПРоверка суммы с бонусами
        ;;Log("********************************************************************");
        ;;Log("BaseCheckSum=" + Double2Str(BaseCheckSum));
        ;;Log("********************************************************************");
        BaseCheckSum     = CurCheque->GetSum();
		double deltaSumBonusPoditog = 0.0;
		deltaSumBonusPoditog = (CurCheque->GetSum())*100 - BaseDiscountSum*100;
        ;;Log("BaseCheckSum new=" + Double2Str(BaseCheckSum));
		;;Log("delta Poditog checka-BonusSumm=" + Double2Str(deltaSumBonusPoditog));
		int deltaX = Double2Int(deltaSumBonusPoditog) % 100;
		;;Log("deltaX=" + Int2Str(deltaX));
        ;;Log("------------------------------------------------");
#endif
		
		//BaseCheckSum     = CurCheque->GetSum();
		AllocDiscountSum = 0;
		AllocCheckSum    = 0;
		CurDiscount      = 0;

		for (int i=0;i<CurCheque->GetCount();i++)
		{
			GoodsInfo crGoods = (*CurCheque)[i];

            if (((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT)) || (crGoods.ItemCode==TRUNC) || (crGoods.ItemCode==EXCESS_PREPAY)) continue;

            string strLock = crGoods.Lock;
            if (CashReg->GoodsLock(strLock, GL_NoBonus) == GL_NoBonus) continue;

            double curItemSum = crGoods.ItemSum;
            if(crGoods.ItemMinimumPrice>0)
            {
                curItemSum -= crGoods.ItemCount * crGoods.ItemMinimumPrice;
                if (curItemSum<=0) continue;
            }

            if (BaseCheckSum - AllocCheckSum > 0)
                //CurDiscount = RoundToNearest((BaseDiscountSum - AllocDiscountSum)/(BaseCheckSum - AllocCheckSum)*crGoods.ItemSum, 0.01);
                CurDiscount = RoundToNearest((BaseDiscountSum - AllocDiscountSum)/(BaseCheckSum - AllocCheckSum)*curItemSum, 0.01);
            else
                CurDiscount = 0;

            AllocCheckSum += curItemSum;

            if (crGoods.ItemSum	> CurDiscount)
            {
                crGoods.ItemSum	-= CurDiscount;
                //+ dms ===== сумма скидки по бонусам
                crGoods.ItemBonusDisc = CurDiscount;
                //- dms =====/
            }
            else
            {
                CurDiscount = crGoods.ItemSum;
                crGoods.ItemSum	= 0;
                //+ dms ===== сумма скидки по бонусам
                crGoods.ItemBonusDisc = CurDiscount;
                //- dms =====/
            }

            // уменьшаем сумму начисленных бонусов с учетом суммы списанных в счет оплаты
            if ((fabs(CurDiscount)>0.01) && (fabs(crGoods.ItemCalcSum)>0.01))
            {
                double newCalcSum = crGoods.ItemCalcSum-CurDiscount;
                if (newCalcSum<0) newCalcSum = 0;
                crGoods.ItemBonusAdd = RoundToNearest(crGoods.ItemBonusAdd * (newCalcSum / crGoods.ItemCalcSum), 0.01);
            }

            AllocDiscountSum += CurDiscount;

			CurCheque->SetAt(i,crGoods);
		}
// - dms ====================/

#ifndef BONUS_LOG_SKIDKA 
		;;Log("********************************************************************");
		//;;Log("Summa skidki bonus BaseDiscountSum=" + Double2Str(BaseDiscountSum));
		//BaseDiscountSum = 306.0;
		;;Log(" BaseCheckSum=" + Double2Str(CurCheque->GetSum()));
		;;Log("********************************************************************");
#endif

		//пересчитываем цену товаров с учетом скидок чтобы в чеке все выглядело красиво
		for (int i=0;i<CurCheque->GetCount();i++)
		{
			GoodsInfo g=(*CurCheque)[i];
			g.ItemPrice=RoundToNearest(g.ItemSum/g.ItemCount,0.01);
			if(fabs(g.ItemPrice-g.ItemCalcPrice)>0.015)
				CurCheque->SetAt(i,g);
		}
		

		// округление
		if( (FiscalCheque::trunc > .01)
            && (!FiscalCheque::PayBank) )
		{
		    int mag = Str2Int(trim(CashReg->GetMagNum()));
		    // Тестирование округения. Отключено
//		    if ((mag==77) || (mag==30))
//		    {
//                // Новые правила округления с Октября 2019г:
//                // - если никаких бонусов в чеке нет, то НЕ округляем вообще
//                // - если есть бонусы, то округляем до рубля в меньшую сторону (за cчет предоставляемых бонусов)
//;;Log("=D1=");
//                if ( isBonusCard
//                    && (CurCheque->GetBonusAddSum()>=FiscalCheque::trunc)
//                    && (CurCheque->GetSum()>=20)
//                   )
//                {
//                    ;;Log("=D2=");
//                    CurCheque->tsum = CurCheque->GetSum() - (FiscalCheque::trunc) * floor((CurCheque->GetSum() + .005) / (FiscalCheque::trunc));
//                }
//                else
//                {
//                    ;;Log("=D3=");
//                    CurCheque->tsum = 0;
//                    //;;Log("Пересчет без КОПЕЕК");
//                }
//		    }
//		    else
		    {
                if (CurCheque->GetSum()>=20)
                {
                    // Новые правила округления с Сентября 2016г:
                    // - если никаких скидок в чеке нет, то округляем до рубля меньшую сторону (за счет предприятия)
                    // - если есть скидки, то округляем до рубля в большую сторону (за cчет предоставляемых скидок)
                    // Добавлено Июнь 2017г.
                    // Для бонусных карт скидку округляем в меньшую сторону. При этом Уменьшаем сумму
                    // начисленных бонусов на Размер скидки-округления
                    if ((!isBonusCard)
                        && (CurCheque->GetDiscountSum_NotAction9()>=FiscalCheque::trunc)
                        && (CurrentCustomer > 0LL))
              {
                        CurCheque->tsum = CurCheque->GetSum() - (FiscalCheque::trunc) * ceil((CurCheque->GetSum() + .005) / (FiscalCheque::trunc));
#ifndef BONUS_LOG_SKIDKA
        ;;Log("Okruglenie v bolshui storonu=" + Double2Str(CurCheque->tsum));
#endif
              }
                        //CurCheque->tsum = CurCheque->GetSum() - ceil(CurCheque->GetSum());
                    else
              {
                        CurCheque->tsum = CurCheque->GetSum() - (FiscalCheque::trunc) * floor((CurCheque->GetSum() + .005) / (FiscalCheque::trunc));

#ifndef BONUS_LOG_SKIDKA
        ;;Log("Okruglenie v menshui storonu=" + Double2Str(CurCheque->tsum));
#endif


              }
                }
                else
                {
                    //если сумма чека меньше 20 рублей, то округляем до 10 коп. в меньшую сторону
                    CurCheque->tsum = CurCheque->GetSum() - (0.1) * floor((CurCheque->GetSum() + .005) / (0.1));
                }
		    }
		    //else
		    //{
            //    CurCheque->tsum = CurCheque->GetSum() - (FiscalCheque::trunc) * floor((CurCheque->GetSum() + .005) / (FiscalCheque::trunc));
            //}

			CurCheque->tsum = RoundToNearest(CurCheque->tsum,0.01);
		}
		else
		{
		    CurCheque->tsum = 0;
		    //;;Log("Пересчет без КОПЕЕК");
		}


        // Для бонусных карт скидку округляем в меньшую сторону.
        // При этом Уменьшаем сумму начисленных бонусов на Размер скидки-округления
        if (isBonusCard)
        {
            double crGoodsTSum=0;
            double crTSum=CurCheque->tsum;

            for (int i=0;(i<CurCheque->GetCount()) && (crTSum>0);i++)
            {
                GoodsInfo crGoods = (*CurCheque)[i];

                if (crGoods.ItemFlag == SL)
                {
#ifndef BONUS_LOG_SKIDKA
                    ;;Log("CrgoodsItemBonusAdd = " + Double2Str(crGoods.ItemBonusAdd));
                    ;;Log("crTsum = " + Double2Str(crTSum));
#endif

                    if (crGoods.ItemBonusAdd > crTSum)
                    {
                        crGoods.ItemBonusAdd	-= crTSum;
                        crTSum	=0;
                    }
                    else
                    {
                        crTSum	-= crGoods.ItemBonusAdd;
                        crGoods.ItemBonusAdd	 = 0;
                    }
                    CurCheque->SetAt(i,crGoods);
                }
            }

           // CurCheque->tsum = crTSum;

        }


/*
		double crGoodsTSum=0;
		double crTSum=CurCheque->tsum;
		for (int i=0;(i<CurCheque->GetCount()) && (crTSum>0);i++)
		{
			GoodsInfo crGoods = (*CurCheque)[i];
			if (crGoods.ItemFlag == SL)
			{
				//crGoodsTSum = RoundToNearest(crGoods.ItemCalcSum*CurCheque->tsum/CurCheque->GetCalcSum(),0.01);
				crGoodsTSum = RoundToNearest(crGoods.ItemSum*CurCheque->tsum/CurCheque->GetSum(),0.01);

				crGoods.ItemFullDiscSum	-= crGoodsTSum;

				if (crGoods.ItemSum > crGoodsTSum)
				{
					crGoods.ItemSum	-= crGoodsTSum;
					crTSum		-= crGoodsTSum;
				}
				else
				{
					crGoods.ItemSum	 = 0;
					crTSum	-= crGoods.ItemSum;
				}
				CurCheque->SetAt(i,crGoods);
			}
		}
		if (fabs(crTSum)>epsilon)
		{
			int i=CurCheque->GetCount()-1;

			GoodsInfo crGoods = (*CurCheque)[i];
			while((i>=0) && (crGoods.ItemFlag!=SL) || (crGoods.ItemSum<=0))
			{
				i--;
				crGoods = (*CurCheque)[i];
			}
			if (crGoods.ItemFlag == SL)
			{
			    crGoods.ItemFullDiscSum	-= crTSum;
				if (crGoods.ItemSum > crTSum)
				{
					crGoods.ItemSum	-= crTSum;
					crTSum		-= crTSum;
				}
				else
				{
					crGoods.ItemSum	 = 0;
					crTSum	-= crGoods.ItemSum;
				}
			}
			CurCheque->SetAt(i,crGoods);
		}
//*/
//*
        // ТЕСТ РАЗОВО - УБРАТЬ СКИДКУ-ОКРУГЛЕНИЕ
#ifndef BONUS_LOG_SKIDKA
		;;Log("tsum do obnulenia=" + Double2Str(CurCheque->tsum));
        CurCheque->tsum = 0;
		;;Log("Summa cheks = " + Double2Str(CurCheque->GetSum()));
#endif
        // ТЕСТ РАЗОВО - УБРАТЬ СКИДКУ-ОКРУГЛЕНИЕ

        //! Списание суммы округления только с округляемых позиций
		double crGoodsTSum=0;
		double crTSum=CurCheque->tsum;

		double crSum=0;

        bool minus = (CurCheque->tsum < 0); // признак отрицательной скидки-округления

        if (minus)
        {
            for (int i=0;(i<CurCheque->GetCount()) && (crTSum<0);i++)
            {
                GoodsInfo crGoods = (*CurCheque)[i];

                crSum = crGoods.ItemCalcSum - crGoods.ItemSum; // выделим скидку

                if ( (crGoods.ItemFlag == SL) && (crSum>0.001) && (crGoods.ItemName.substr(0,1)!="*") && (crGoods.ItemName.substr(0,1)!="@") ) // отрицательная скидка не может увеличивать акционный товар
                {
                    //crGoodsTSum = RoundToNearest(crGoods.ItemCalcSum*CurCheque->tsum/CurCheque->GetCalcSum(),0.01);
                    crGoodsTSum = RoundToNearest(max(-crSum, crTSum),0.01);

                    crGoods.ItemFullDiscSum	-= crGoodsTSum;

                    crGoods.ItemSum	-= crGoodsTSum;
                    crTSum		-= crGoodsTSum;

                    CurCheque->SetAt(i,crGoods);
                }
            }
        }
        else
        {
            for (int i=0;(i<CurCheque->GetCount()) && (crTSum>0);i++)
            {
                GoodsInfo crGoods = (*CurCheque)[i];

                crSum = (crGoods.ItemSum*10 - (int)(crGoods.ItemSum*10))/10; // выделим копейки

                if ((crGoods.ItemFlag == SL) && (crSum>0.001))
                {
                    //crGoodsTSum = RoundToNearest(crGoods.ItemCalcSum*CurCheque->tsum/CurCheque->GetCalcSum(),0.01);
                    crGoodsTSum = RoundToNearest(min(crSum, crTSum),0.01);

                    crGoods.ItemFullDiscSum	-= crGoodsTSum;   

                    if (crGoods.ItemSum > crGoodsTSum)
                    {
                        crGoods.ItemSum	-= crGoodsTSum;
                        crTSum		-= crGoodsTSum;
                    }
                    else
                    {
                        crGoods.ItemSum	 = 0;
                        crTSum	-= crGoods.ItemSum;
                    }
                    CurCheque->SetAt(i,crGoods);
                }
            }
        }
		
		if (fabs(crTSum)>epsilon)
		{
			int i=CurCheque->GetCount()-1;

			GoodsInfo crGoods = (*CurCheque)[i];
			
#ifndef BONUS_LOG_SKIDKA
            ;;Log("crGoodsItemSum=" + Double2Str(crGoods.ItemSum));
#endif
			
			while((i>=0) && (crGoods.ItemFlag!=SL) || (crGoods.ItemSum<=0))
			{
				i--;
				crGoods = (*CurCheque)[i];
			}
			if ((crGoods.ItemFlag == SL)
                && (!minus || ((crGoods.ItemName.substr(0,1)!="*") && (crGoods.ItemName.substr(0,1)!="@")))
               )
			{
			    crGoods.ItemFullDiscSum	-= crTSum;
				if (crGoods.ItemSum > crTSum)
				{
					crGoods.ItemSum	-= crTSum;
					crTSum		-= crTSum;
				}
				else
				{
					crGoods.ItemSum	 = 0;
					crTSum	-= crGoods.ItemSum;
				}
			}

			CurCheque->SetAt(i,crGoods);
		}

#ifndef BONUS_LOG_SKIDKA
        ;;Log("***************************************");
        ;;Log("CurCheque->tsum =" + Double2Str(CurCheque->tsum));
        ;;Log("Summa cheks = " + Double2Str(CurCheque->GetSum()));
        ;;Log("*****************************************************");
#endif
		
		
//*/

//  ====== Акции по бонусам ======
//        ExecBonusAction(CurCheque); //отключить
//  =======

// + dms ====================/
		//распределяем оплату сертификатами по всему чеку (АВАНС)
		BaseDiscountSum  = CurCheque->PrePayCertificateSumm;
		BaseCheckSum     = CurCheque->GetSum();
		AllocDiscountSum = 0;
		AllocCheckSum    = 0;
		CurDiscount      = 0;

		for (int i=0;i<CurCheque->GetCount();i++)
		{
			GoodsInfo crGoods = (*CurCheque)[i];

            if (((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT)) || (crGoods.ItemCode==TRUNC) || (crGoods.ItemCode==EXCESS_PREPAY)) continue;

            if (BaseCheckSum - AllocCheckSum > 0)
                CurDiscount = RoundToNearest((BaseDiscountSum - AllocDiscountSum)/(BaseCheckSum - AllocCheckSum)*crGoods.ItemSum, 0.01);
            else
                CurDiscount = 0;

            crGoods.ItemSumToPay = crGoods.ItemSum;

            AllocCheckSum += crGoods.ItemSum;

            if (crGoods.ItemSumToPay > CurDiscount)
                crGoods.ItemSumToPay -= CurDiscount;
            else
            {
                CurDiscount = crGoods.ItemSumToPay;
                crGoods.ItemSumToPay	= 0;
            }

            AllocDiscountSum += CurDiscount;

			CurCheque->SetAt(i,crGoods);
		}
// - dms ====================/

		//обновляем информацию на экране
		QListViewItem *CurItem=ChequeList->firstChild();
		if(CurItem)
		for (int i=0;i<CurCheque->GetCount();i++)
		{
			GoodsInfo g=(*CurCheque)[i];
			if (g.ItemFlag==SL)
			{
			    //CurItem-> TextColor = g.ActionGoods?Qt::blue:Qt::black;
			    CurItem->setText(ListName,W2U(g.ItemName));
				CurItem->setText(ListPrice,GetRounded(g.ItemPrice,0.01).c_str());
				CurItem->setText(ListSum,GetRounded(g.ItemSum,0.01).c_str());
				CurItem->setText(ListDscnt,GetRounded(g.ItemCalcSum-g.ItemSum,0.01).c_str());
				
				#ifndef BONUS_LOG_SKIDKA
					;;Log("g.ItemName = " + g.ItemName);
					;;Log("Price = " + Double2Str(g.ItemPrice));
					;;Log("Sum = " + Double2Str(g.ItemSum));
					;;Log("Delta = " + Double2Str(g.ItemCalcSum-g.ItemSum));
				#endif

				if (CurItem->itemBelow()!=NULL)
					CurItem=CurItem->itemBelow();
			}
		}
		CashReg->ClearCurrentCheck();

		CashReg->WriteChequeIntoTable(CurCheque,CashReg->GetCurTime());

		//ShowGoodsInfo();
		SaveSet();
		
#ifndef BONUS_LOG_SKIDKA
        ;;Log("All Tovar checks=" + Double2Str(CurCheque->GetSum()));
		;;Log("GetFullSumm=" + Double2Str(CurCheque->GetFullSum()));
		;;Log("GetCalcSum=" + Double2Str(CurCheque->GetCalcSum()));
		;;Log("GetDiscountSum();=" + Double2Str(CurCheque->GetDiscountSum()));
		;;Log("tsum=" + Double2Str(CurCheque->tsum));
		;;Log("trunc=" + Double2Str(CurCheque->trunc));
		;;Log("GetSumNotAction=" + Double2Str(CurCheque->GetSumNotAction()));
		//;;Log("GetCalcSum_Action9=" + Double2Str(GetCalcSum_Action9()));
		;;Log("GetCalcSum_NotAction9=" + Double2Str(CurCheque->GetCalcSum_NotAction9()));
		;;Log("GetDiscountSum_NotAction9=" + Double2Str(CurCheque->GetDiscountSum_NotAction9()));	
#endif

//		;;Log("[Func] ProcessDiscount [End]");

 	}
	catch(cash_error& er)
	{
		CashReg->LoggingError(er);
		return;
	}
}

void WorkPlace::SetBankDSum(void)
{
    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return;

    //сохранение суммы перед предоставлением скидки по банковской карте
    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];
        g.ItemBankDiscSum=g.ItemFullDiscSum;

        CurCheque->SetAt(i,g);
    }
}

bool WorkPlace::AppendRecord2Check(void)
{
	if (PriceChecker!=NULL)
	{
		StartPriceChecker(41480);

		return false;
	}
    if (FreeSum!=NULL)
    {
        FreeSum->NameEdit->setText(W2U(CurrentGoods->ItemName));
        FreeSum->PriceEdit->setText(W2U(Double2Str(CurrentGoods->ItemPrice)));
        FreeSum->CountEdit->setText(W2U(Double2Str(CurrentGoods->ItemCount)));

        FreeSum->InfoLabel->setText(W2U("Товар есть в базе!"));
        return false;
    }

	{
		try
		{

/*
                // Временная для маг 100 на переходный период
                // Не перемещенный Алкоголь можно продавать только на 11й кассе
                // На товар установлен признак 31 (алкоголь гастроном)
                // Товары домашнего гастронома отбивать на 11й кассе нельзя

                int mag = Str2Int(trim(CashReg->GetMagNum()));
                if (mag==100)
                {
                    string str= CurrentGoods->Lock;
                    bool res = CashReg->GoodsLock(str, GL_GastronomOnly) == GL_GastronomOnly;

                        if (
                        (CashReg->GetCashRegisterNum()==11) || (CashReg->GetCashRegisterNum()==9)
                        || (CashReg->GetCashRegisterNum()==2) || (CashReg->GetCashRegisterNum()==10)
                        )
                        {
                            if (!res) ShowMessage("Товар не может быть продан на этой кассе!");
                        }
                        else
                        {
                            if (res) ShowMessage("Товар не может быть продан на этой кассе! Только на кассах 11, 9");
                        }

                }
*/

		    // Акция 3+1 - каждый четвертый товар в подарок ( скидкой 25%)
		    string str= CurrentGoods->Lock;
		    bool action3_1 = CashReg->GoodsLock(str, GL_Action3_1) == GL_Action3_1;

		    if (action3_1)
		    {
                //ShowMessage("ТОВАР УЧАСТВУЕТ В АКЦИИ 3+1\nПРЕДЛОЖИТЬ ПОКУПАТЕЛЮ ПОЛУЧИТЬ 4-Й ТОВАР В ПОДАРОК");

                // ShowMessage("При покупке данного пива кратное 4,5 л\n выдается подарок:\n\n          5 пачек арахиса.\n\n ( за 9 л - 10 пачек и т.д.)");
		    }

		    //if (CurrentGoods->ItemCode==48297)
		    //{
		    //     ShowMessage("Выдать ПОДАРОК книгу \"Великие поэты. Иван Бунин\"");

            //    ShowMessage("Выдать ПОДАРОК книгу \"Музеи мира\"");

                // ShowMessage("При покупке данного пива кратное 4,5 л\n выдается подарок:\n\n          5 пачек арахиса.\n\n ( за 9 л - 10 пачек и т.д.)");
		    //}

		    bool IsGood18 = CashReg->GoodsLock(str, GL_Goods18) == GL_Goods18;


			CurrentGoods = Cheque->Insert(*CurrentGoods);

			QListViewItem *lastItem=ChequeList->firstChild();//update check's list

			if (lastItem!=NULL)
				for(;;)
					if (lastItem->itemBelow()!=NULL)

						lastItem=lastItem->itemBelow();
					else
						break;

//			QListViewItem *item=new QListViewItem(ChequeList,lastItem);//add goods to the cheque list

            QColor color = Qt::black;

            if ((CurrentGoods->ItemAlco) || IsGood18)
                color = Qt::darkMagenta;
            else
                if (CurrentGoods->ItemCode==FreeSumCode)
                    color = Qt::darkMagenta;
                else
                    if (action3_1 || CashReg->GoodInAction(CurrentGoods))
                        color = Qt::darkBlue;

            ColorListViewItem *item=new ColorListViewItem(ChequeList, lastItem, color);//add goods to the cheque list

			item->setText(ListNo,Int2Str(Cheque->GetSize()).c_str());
            // + dms === 2010-03-30 =====\ спец. код не выводим
			item->setText(ListCode, CurrentGoods->ItemCode==FreeSumCode? "" : Int2Str(CurrentGoods->ItemCode).c_str());

            if (CurrentGoods->ItemAlco)
                item->setText(ListName,W2U("~"+CurrentGoods->ItemName));
            else
                item->setText(ListName,W2U(CurrentGoods->ItemName));

			item->setText(ListPrice,GetRounded(CurrentGoods->ItemPrice,0.01).c_str());
			item->setText(ListCount,GetRounded(CurrentGoods->ItemCount,CurrentGoods->ItemPrecision).c_str());
			item->setText(ListSum,GetRounded(CurrentGoods->ItemSum,0.01).c_str());
			item->setText(ListDscnt,GetRounded(CurrentGoods->ItemCalcSum-CurrentGoods->ItemSum,0.01).c_str());

			ChequeList->ensureItemVisible(item);

			ChequeList->setSelected(item,true);
			ChequeList->setCurrentItem(item);

			//
			if(CurrentGoods->ItemSum < .01){
				ShowMessage(CurGoods.ItemCount < .001 ? "Введите количество товара!" : "Неверная сумма покупки!");
				return false;
			}
			ShowGoodsInfo();

            //+ dms ==== 2014-12-23 ==== Сообщения при считывании товара в чеке =====

            // 120-121й запрет
		    // 20-21й ИД для сообщения при добавлении товара в чек

		    str = CurrentGoods->Lock;
            for (unsigned short k=20;k<=21;k++)
            {
                if (CashReg->GoodsLock(str, GL_NoLock + k) == GL_NoLock + k)
                {
                    string msg = CashReg->GetMessageInfo(k);
                    if (msg!="") ShowMessage(msg);

                }
            }

            //- dms ==== 2014-12-23 ==== Сообщения при считывании товара в чеке =====

			return true;
		}
		catch(int &er)
		{
			CashReg->WriteChequeIntoTable(Cheque,CashReg->GetCurTime());

			ErrorHandler(er);
			return false;


		}
		catch(cash_error &er)
		{
			CashReg->DbfMutex->unlock();
			CashReg->LoggingError(er);
			return false;
		}
	}
	return false;
}

void WorkPlace::ErrorHandler(int er)
{
	if (er!=0)
	{
		CashReg->ShowErrorCode(er);

		if ((FiscalReg->RestorePaperBreak())&&(FiscalReg->IsPaperOut(er)))
		{
			int error_code;
			for(;;)
			{
				error_code=FiscalReg->ResetReg();
				if (error_code==0)
					return;
				if (FiscalReg->IsPaperOut(er))
					ShowMessage("Вставьте бумагу и нажмите OK!");
				else
					ErrorHandler(er);
			}
		}

		if (Cheque->GetCount()>0)    //check guard id
		{
			int GuardRes=NotCorrect;
			if (!CashReg->CurrentSaleman->Annul)
			do
			{
				GuardRes=CheckBarcode(Guard);

				if (GuardRes==NotCorrect)
					ShowMessage("Неправильный пароль СПП!");

				if (GuardRes==NoPasswd)
					ShowMessage("Введите пароль!");
			}
			while((GuardRes==NotCorrect)||(GuardRes==NoPasswd));

			CashReg->TryToCancelCheck();//anull current check

			if (FiscalReg->IsPaperOut(er))
				ShowMessage("Не забудьте снова пробить чек!");

			CashReg->AnnulCheck(Cheque,GuardRes);

			IsChequeInProgress=false;
			InitWorkPlace();
		}
	}
}


bool WorkPlace::WhoIsCert(int Who)
{
    return Who == CD_CERT100
        || Who == CD_CERT250
        || Who == CD_CERT500
        || Who == CD_CERT1000
        || Who == CD_CERT1500
        || Who == CD_CERT2000
        || Who == CD_CERT3000
        || Who == CD_CERT5000;
}

long long WorkPlace::CheckBarcode(int Who, string msg)
{
    //;;Log("[FUNC] CheckBarcode [START]");
    if (msg=="") BarcodeCard=new GuardForm(this, PrefixSeq, PostfixSeq);
	else BarcodeCard=new GuardForm(this, PrefixSeq, PostfixSeq, msg);
	if(!CashReg->CurrentSaleman->ZReport)
		BarcodeCard->EditLine->setReadOnly(true);
	long long res=NotCorrect;
	string Barcode;
    string BarcodeL; // Int64

	try
	{
		vector<string> prefices;

		for(unsigned int i=0;i<BarcodeID.size();i++)
		{
			string s = BarcodeID[i]->CodeName;
			if ( s=="EAN13" || s == "EAN8")
				prefices.insert(prefices.end(),BarcodeID[i]->CodeID);
		}


		if (BarcodeCard->exec()==QDialog::Accepted)
		{
			Barcode=U2W(BarcodeCard->EditLine->text().upper());//load barcode and cut off prefix
			if (!Barcode.empty())
				for (unsigned int i=0;i<prefices.size();i++)
				{
					string prefix=prefices[i];
					if (Barcode.substr(0,prefix.length())==prefix)
					{
						Barcode=Barcode.substr(prefix.length());
						BarcodeL=Barcode.substr(1);
					}
				};


			if (Barcode.substr(0,2)!=CardPrefix && Barcode.substr(0,3)!=CardPrefixN && Who != Guard && Who != Manager && !WhoIsCert(Who))
			{
				res = NotCorrect;
				if(Barcode.length() < 13)
				{
					int CardNumber = atoi(Barcode.substr(0,7).c_str());// + 9000000;
					// Код карты Int64
					long long CardNumberL = atoll(BarcodeL.substr(0,12).c_str());// + 9000000;

#ifdef _OLD_VARIANT_  // объявляется в utils.h

// test, find error customers range (9930008...)
					//if((CardNumber>=800000) && (CardNumber<900000))
					//{
                     //   ;;cash_error* _err = new cash_error(11,0, BarcodeL);
                     //   ;;CashReg->LoggingError(*_err);
                     //   ;;delete _err;
					//}


					if((CardNumberL>=993000700000LL) && (CardNumberL<993000800000LL))
					{
						CardNumber+=100000;
						CardNumberL+=100000;
					}

#endif
					//CashReg->FindCustomer(&res,CardNumber);
					CashReg->FindCustomer(&res,CardNumberL);

					delete BarcodeCard;
					BarcodeCard=NULL;
					return res;
				}
			}

			res=NotCorrect;

			int CardNumber;
			long long CardNumberL;

			switch(Who)
			{
			    case Flayer:
                    if(Barcode.substr(0,3) == GastronomPrefix)
                    {
                        CardNumber=atoi(Barcode.substr(6,6).c_str());
						// Customer code Int64
						CardNumberL=atoll(Barcode.substr(0,12).c_str());

#ifdef _OLD_VARIANT_
						if((CardNumberL>=993000700000LL) && (CardNumberL<993000800000LL))
						{
							CardNumber+=100000;
							CardNumberL+=100000;
						}
#endif
                        CardInfoStruct CardInfo;
                        if (CashReg->FindCardInRangeTable(&CardInfo,CardNumberL))
                        {
                           if(CardInfo.CardType==CD_FLAYER)
                           {
                               res = CardInfo.CardNumber;
                               break;
                           }
                        };
                    }
				case Guard://if card's owner is guard then we have to find his ID
					if(Barcode.substr(0,3) != SPPPrefix)
						break;//res = NotCorrect;
					CashReg->FindStaff((int*)&res,Barcode);
					break;
                case CD_CERT100:
                case CD_CERT250:
                case CD_CERT500:
                case CD_CERT1000:
                case CD_CERT1500:
                case CD_CERT2000:
                case CD_CERT3000:
                case CD_CERT5000:

					if(Barcode.substr(0,3) == SPPPrefix)
						break;//res = NotCorrect;
					if(Barcode.substr(0,3) == GastronomPrefix)
					{
						CardNumber=atoi(Barcode.substr(3,9).c_str());
						// Customer code Int64
						CardNumberL=atoll(Barcode.substr(0,12).c_str());

#ifdef _OLD_VARIANT_

						if((CardNumberL>=993000700000LL) && (CardNumberL<993000800000LL))
						{
							CardNumber+=100000;
							CardNumberL+=100000;
						}
#endif
                        CardInfoStruct CardInfo;
                        if (CashReg->FindCardInRangeTable(&CardInfo,CardNumberL))
                        {
                           if((CardInfo.CardType==CD_CERTIFICATE)
                           || (CardInfo.CardType==CD_PREPAY_CERTIFICATE))
                           {
                               ;;char log_str[100];
                               ;;sprintf(log_str, " = SaleCertificate #%lld was found (nominal=%0.2f)", CardInfo.CardNumber, CardInfo.CardSumma);
                               ;;Log(log_str);
                               if (CardInfo.CardSumma==Who)
                               {
                                    Log(" SUCCESS");
                                    res = CardInfo.CardNumber;
                               }
                               else
                               {
                                    Log(" ERROR (Wrong nominal)");
                                    res = NoPasswd;
                               }
                               break;
                           }
                        }

					}
                    break;
                case Manager:

                    if(Barcode.length()<12)
                        break;//res = NotCorrect;
					if(Barcode.substr(0,3) == SPPPrefix)
						break;//res = NotCorrect;
					if(Barcode.substr(0,3) == ManagerPrefix)
					{
						CardNumber=atoi(Barcode.substr(6,6).c_str());
						// Customer code Int64
						CardNumberL=atoll(Barcode.substr(0,12).c_str());

                       res = CardNumberL;

                       ;;char log_str[100];
                       ;;sprintf(log_str, " Manager #%lld ", res);
                       ;;Log(log_str);

					}else{
                        Log(" ERROR (Wrong Manager barcode)");
                        res = NoPasswd;
					}
                    break;
				case Customer://if card's owner is customer then we have to find his ID
				case Certificate:

					if(Barcode.substr(0,3) == SPPPrefix)
						break;//res = NotCorrect;
					if(Barcode.substr(0,3) == GastronomPrefix)
					{
						CardNumber=atoi(Barcode.substr(3,9).c_str());
						// Customer code Int64
						CardNumberL=atoll(Barcode.substr(0,12).c_str());

#ifdef _OLD_VARIANT_
						if((CardNumberL>=993000700000LL) && (CardNumberL<993000800000LL))
						{
						    Log("Update old card number: "+LLong2Str(CardNumberL));
							CardNumber+=100000;
							CardNumberL+=100000;
							Log("to "+LLong2Str(CardNumberL));
						}
#endif
                        if ((Who != Certificate) && IsCertificate(CardNumberL))
						{
						    ShowMessage("Невозможно использовать подарочную карту в качестве дисконтной !");

                            delete BarcodeCard;
                            BarcodeCard=NULL;

						    res=NoPasswd;
						    return res;
                        }
					}
					else
					{
						CardNumber=atoi(Barcode.substr(6,6).c_str());
                        // Customer code Int64
						CardNumberL=atoll(Barcode.substr(0,12).c_str());
					}
					//CashReg->FindCustomer(&res,CardNumber);
					CashReg->FindCustomer(&res,CardNumberL);

					break;
				default:break;
			}

		}
		else
		{
            if(Who==Manager) res = PressEscape;
            else res=NoPasswd;
		}
	}
	catch(cash_error& er)
	{
		CashReg->DbfMutex->unlock();
		CashReg->LoggingError(er);
	}

	delete BarcodeCard;
	BarcodeCard=NULL;

    //;;Log("[FUNC] CheckBarcode [END]");

//                       ;;char log_str[100];
//                       ;;sprintf(log_str, " Manager_1 #%lld ", res);
//                       ;;Log(log_str);

	return res;
}

bool WorkPlace::CheckGuardCode(int& code)
{
    //;;Log("[FUNC] CheckGuardCode [START]");
	int GuardRes=CheckBarcode(Guard);
	if (GuardRes==NotCorrect)
	{
		ShowMessage("Неправильный пароль СПП");
		return false;
	}
	if (GuardRes==NoPasswd)
		return false;
	code=GuardRes;
	//;;Log("[FUNC] CheckGuardCode [END]");
	return true;
}

long long WorkPlace::ScanBarcode()
{

	long long Res=-1;

	while ((Res==NotCorrect) || (Res==NoPasswd))
	{
        Res = CheckBarcode(Flayer);
	}

	return Res;
}

#include <qapplication.h>
#include <qsemimodal.h>
#include <qevent.h>
#include <qcursor.h>
#include <qstatusbar.h>
#include <qbutton.h>
#include <qvariant.h>
#include <signal.h>


bool WorkPlace::LookingForBarcode(void)
{
    ;;Log("[FUNC] LookingForBarcode [START]");

	QString tmpID;

    QString OrgBarcode= EditLine->text();
    QString CurBarcode= EditLine->text().upper();

    ;;Log("250322 OrgBarcode="+OrgBarcode+" ("+Int2Str(OrgBarcode.length())+")");
    ;;Log("250322 CurBarcode="+CurBarcode+" ("+Int2Str(CurBarcode.length())+")");

	if(PriceChecker!=NULL) CurBarcode=PriceChecker->SearchEdit->text().upper();
	if(FreeSum!=NULL)      CurBarcode=FreeSum->BarcodeEdit->text().upper();

	unsigned int i,k;

	wbarcode=0;

	if(CurBarcode.left(2) == "KG")
	{

        QString KGBarcode= EditLine->text();

        EditLine->clear();

        QString hash = KGBarcode.mid(15);

        bool resCheck=false;

        int pos = hash.find("|", 0);

        if (pos>0) {
            QString DateTimeStr = hash.mid(pos+1);
            hash = hash.left(pos);

            // проверка хеша карты
           string cardhash;
           if (b64decode((char*)hash.ascii(), &cardhash))
           {

               string comphash = CurBarcode.mid(2,12).ascii();

               if (GetSHA1(comphash+"CARD") == cardhash)
               {
                // Хеш верный, проверка на актуальность штрихкода
                // Проверка дата/время
                   string dtcard;
                   if (b64decode((char*)DateTimeStr.ascii(), &dtcard))
                   {

                        // Получим дату и время, до которого действует карта
                        long long llres= 0;
                        sscanf((const char*)(dtcard.c_str()), "%lld", &llres);

                        // Дата/время действия карты
                        QDateTime QDTCard;
                        QDTCard.setTime_t((unsigned int)(llres/1000));
                        ;;Log("Время карты: "+QDTCard.toString());

                        QDateTime QDTCardToCheck = QDTCard.addSecs(2*60*60)
                        ;;Log("Время карты (+2): "+QDTCardToCheck.toString());

                        // Текущее дата/время
                        QDateTime QDTCurrent = QDateTime::currentDateTime();
                        ;;Log("Текущее время: "+QDTCurrent.toString());


                        if (QDTCard>=QDTCurrent)
                        {
                            // Штрихкод актуален, передаем карту в чек
                            CurBarcode = "F"+CurBarcode.mid(2,12);
                            resCheck=true;
                        }
                        else
                        {
                            ShowMessage("Срок действия штрихкода истек!");
                            Log("Срок действия штрихкода истек!");
                        }
                   }
                   else
                   {
                        ShowMessage("Неправильный формат штрихкода!");
                        Log("Ошибка расшифровки даты действия!");
                   }
               }
               else
               {
                    ShowMessage("Неправильный формат штрихкода!");
                    Log("Неправильный хеш карты!");
               }
           }
           else
           {
               ShowMessage("Неправильный формат штрихкода!");
               Log("Ошибка чтения хеша карты");
           }
        }
        else
        {
           ShowMessage("Неправильный формат штрихкода!");
           Log("не найдена позиция |");
        }

        if (!resCheck)
        {
            return false;
        }

	}
///Предчек
	if(CurBarcode.left(2) == "PC")
	{
		IsCurrentGoodsEmpty = true;
		if(!ShowQuestion(string("Пречек ")+CurBarcode.mid(2).ascii(),true))
			return false;
		QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
		QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
		sm.setFixedSize(250,100);
		msg.setFixedSize(250,100);
		msg.setAlignment( QLabel::AlignCenter );
		msg.setText(W2U("Поиск пречека..."));
		sm.show();
		//thisApp->processEvents();

		int NumPrecheck = CurBarcode.mid(2).toInt(NULL);

		bool ret = CashReg->SQLServer->GetPreCheck(NumPrecheck, Cheque, CurrentCustomer);
		if(!ret)
		{
			EditLine->clear();
		}
		UpdateList();
		return ret;
	}


	CurGoods.InputType = FindInDB?IN_DBRSCAN:HandBarcode?IN_RSCAN:IN_SCAN;

    ;;Log("[FUNC] LookingForBarcode [1]");
	
#ifndef PIVO_BOTTLE
    ;;Log("[DEBUG] CurBarcode.length=" + Int2Str(CurBarcode.length()));
#endif

#ifdef BIODOBAVKA
    ;;Log("[DEBUG] CurBarcode.length=" + Int2Str(CurBarcode.length()));
#endif
	
//;;Log("Barcodefilter=" + Int2Str(BarcodeFilter));
    if (CurBarcode.length() == 29)
    {
        // DataMatrix cigarette
		Log("DataMatrix Cigarette");
        return LookingForGoods(BarcodeFilter);//find by barcode
    }
		else if ((CurBarcode.length()==30) || (CurBarcode.length()==37) || (CurBarcode.length() == 32))
    {
	
///d.gusakov
	Log("DataMatrix milk or water");
///d.gusakov
		
        MilkActsiz = OrgBarcode.local8Bit();
        return LookingForGoods(BarcodeFilter);//find by barcode
    }

#ifdef ALCO_VES
        else if (CurBarcode.length()==44)
        {
            Log("Пиво разлив вход");
            AlcoVesActsiz = OrgBarcode.local8Bit();
            return LookingForGoods(BarcodeFilter);
        }
#endif

#ifdef BLOCK_CIGAR
    else if ((CurBarcode.length()==52) ||(CurBarcode.length()==41) || (CurBarcode.length()==55) )
    {
        Log("Блок сигарет вход");
        BlockCigarActsiz = OrgBarcode.local8Bit();
        return LookingForGoods(BarcodeFilter);
    }
#endif

#ifdef PIVO_BOTTLE
    else if ( (CurBarcode.length()==31))
    {
        Log("Пиво в бутылке или банке");
        MilkActsiz = OrgBarcode.local8Bit();
        return LookingForGoods(BarcodeFilter);
    }
#endif

#ifdef BIODOBAVKA
    else if ( (CurBarcode.length()==83) )
    {
        Log("Биодобавка");
        MilkActsiz = OrgBarcode.local8Bit();
        return LookingForGoods(BarcodeFilter);
    }
#endif

    /* slair+ */
    string GS1DM_barcode = "";
    MilkActsiz = "";

#ifdef ALCO_VES
    AlcoVesActsiz = "";
#endif

#ifdef BLOCK_CIGAR
    BlockCigarActsiz = "";
#endif

    if (OrgBarcode.left(3) == "010")
    { // GS1-DataMatrix молочка
        ;;Log("250322 OrgBarcode="+OrgBarcode+" маркировка "+Int2Str(OrgBarcode.length()));
        GS1DM_barcode = OrgBarcode.mid(3, 13).local8Bit();
        CurBarcode = OrgBarcode.mid(3, 13).local8Bit();
        ;;Log("250322 GS1DM_barcode="+GS1DM_barcode);

        // молочный акциз
        if ((OrgBarcode.length()==30) || (OrgBarcode.length()==32) || (OrgBarcode.length()==37) || (OrgBarcode.length()==44)) {
           // Добавляем спецсимвол
           ;;Log("Origin Actsiz DataMatrix="+OrgBarcode);
           char spec = 0x1D;
//           MilkActsiz = OrgBarcode.mid(0, 24).local8Bit() + spec + OrgBarcode.mid(24, 6).local8Bit();
           MilkActsiz = OrgBarcode.local8Bit();
           ;;Log("250322 MilkActsiz="+MilkActsiz+" запомнили");
        }

#ifdef BLOCK_CIGAR
        if (OrgBarcode.length()==52 || OrgBarcode.length()==41 || OrgBarcode.length()==55 )
        {
            // Добавляем спецсимвол
            ;;Log("Origin Actsiz DataMatrix="+OrgBarcode);
            char spec = 0x1D;
            BlockCigarActsiz = OrgBarcode.local8Bit();
            ;;Log("250322 MilkActsiz="+BlockCigarActsiz+" запомнили");
        }
#endif

    }
	
    if (CurBarcode.left(4) == "F010")
    { // GS1-DataBar весовой товар
        ;;Log("230322 CurBarcode="+CurBarcode+" весовой "+Int2Str(CurBarcode.length()));
//        ShowMessage("Весовой товар");

        int GS1DB_weight;
        string GS1DB_barcode;
        int GS1DB_code;

        bool bad_barcode = false;

//        ShowMessage("CurBarcode.mid(2, 2)=="+CurBarcode.mid(2, 2));
//        if (CurBarcode.mid(2, 2) != "01") {
//            bad_barcode = false;
//        }
//        ShowMessage("CurBarcode.mid(4+13, 4)=="+CurBarcode.mid(4+13, 4));

        if (CurBarcode.mid(4+13, 4) != "3103") {
            

			bad_barcode = true;
        }

        if (! bad_barcode) {
            GS1DB_barcode = CurBarcode.mid(4, 13).ascii();
            GS1DB_weight = CurBarcode.mid(4+13+4, 6).toInt();

            bool loc=CashReg->GoodsTable->Locate("BARCODE", GS1DB_barcode.c_str());
            if (!loc) {
                ShowMessage("Штрих код " + GS1DB_barcode + " не найден в базе товаров!");
    //            ;;Log("Штрихкод " + GS1DB_barcode + " не найден в базе товаров!");
                return false; // fixme: не забудь включить, чтобы не было подмены товара
//                GS1DB_code = 121548;   // fixme: find CODE by BARCODE
            } else {
                GS1DB_code = CashReg->GoodsTable->GetLongField("CODE");
                loc = true;
            }
			
            ;;Log((char* )(
                      " >> Весовой товар. Префикс: 010 Штрихкод: " + GS1DB_barcode
                      + " Код: " + Int2Str(GS1DB_code)
                      + " Вес: " + Int2Str(GS1DB_weight)
                      ).c_str());

            if (GS1DB_weight<=0) {
                ShowMessage("Неправильное количество товара!");
                return false;
            }
            try {
                if(!Cheque->GetSize())
                    CashReg->GetNewGoods();

                if (CashReg->IsInload()) {
                    ShowMessage("Список товаров недоступен!\n"
                                "Дождитесь окончания загрузки товаров и\n"
                                "Считайте штрихкод еще раз!");
                    return false;
                }

    //            loc=CashReg->GoodsTable->Locate("BARCODE", GS1DB_barcode.c_str());
                string gtype;
                loc = CashReg->FindGoodsByCode(GS1DB_code);
                if (loc) {
                    gtype = CashReg->GoodsTable->GetStringField("GTYPE");
                    ;;Log("TOKAS->GTYPE==" + gtype);
                    if (gtype!="F") {
                        ;;Log("CurGoods:: А в базе у нас он не весовой. Пытаемся исправить.");
                        CurGoods.ItemCount = 1;
                    } else { // if (gtype!="F") таки весовой
                        CurGoods.ItemCount = GS1DB_weight*0.001;
                    }
                } else {
                    ShowMessage("Товар не найден!");
                    return false;
                }


                if ((loc)&&(AddGoods(GS1DB_code))) {
                    CurrentGoods->ItemBarcode = U2W(CurBarcode);
                    if (gtype!="F") {
                        ;;Log("CurrentGoods:: А в базе у нас он не весовой. Пытаемся исправить.");
                        CurrentGoods->ItemCount = 1;
                    } else { // if (gtype!="F") таки весовой
                        CurrentGoods->ItemCount=RoundToNearest(
                            GS1DB_weight*0.001,CurrentGoods->ItemPrecision);
                    }
                    CalcCountPrice();
                    CurrentGoods->CalcSum();
                            
							ShowGoodsInfo();

                    CashReg->LogAddGoods(CurrentGoods);

                    if(CurrentGoods->ItemCount < CurrentGoods->ItemPrecision) {
                        ShowMessage("Неправильное количество товара");
//                        return false;
                    }

                    if((PriceChecker==NULL)&&(FreeSum==NULL)) {
                        QListViewItem *item = ChequeList->firstChild();
                        if(item)
                            while(item->itemBelow())
                                item = item->itemBelow();
                        if(item) {
                            item->setText(ListNo,Int2Str(Cheque->GetSize()).c_str());
                            item->setText(ListCode,Int2Str(CurrentGoods->ItemCode).c_str());
                                item->setText(ListName,W2U(CurrentGoods->ItemName));
                            item->setText(ListPrice,GetRounded(CurrentGoods->ItemPrice,0.01).c_str());
                            item->setText(ListCount,GetRounded(CurrentGoods->ItemCount,CurrentGoods->ItemPrecision).c_str());
                                item->setText(ListSum,GetRounded(CurrentGoods->ItemSum,0.01).c_str());
                            item->setText(ListDscnt,GetRounded(CurrentGoods->ItemCalcSum-CurrentGoods->ItemSum,0.01).c_str());
                        }
                    }
                    return true;
                } else {
                    ShowMessage("Товар не найден!");
                    return false;
                }
            } // try {
            catch(cash_error& er) {
                CashReg->DbfMutex->unlock();
                CashReg->LoggingError(er);
                return false;
            } // catch(cash_error& er) {
        } // if (! bad_barcode) {
    }
    else if (CurBarcode.left(4) == "F011")
    { // GS1-DataBar штучный товар
        ;;Log("230322 CurBarcode="+CurBarcode+" штучный");
//        ShowMessage("Штучный товар");
//        return false;
    } // GS1-DataBar штучный товар
    /* slair- */

	for (i=0;i<BarcodeID.size();i++)
	//for (i=BarcodeID.size()-1;i>=0;i--)
	{

        tmpID=W2U(BarcodeID[i]->CodeID);//first of all we have to find barcode prefix
///d.gusakov
		//;;Log(BarcodeID[i]->CodeID + " "+BarcodeID[i]->CodeName+" === "+CurBarcode.ascii());
		//Log("Test1=" + CurBarcode.left(tmpID.length()));
        //Log("Test2=" + tmpID);
///d.gusakov
		if (CurBarcode.left(tmpID.length())==tmpID)
		{
	
			Log("Test=" + CurBarcode);
			
            long long CardNumberL;
			QString TempBarcode = CurBarcode;
            // код карты Int64 (8 байт)
			CardNumberL = atoll(U2W(CurBarcode).substr(1,12).c_str());

			CurBarcode.remove(0,tmpID.length());

			if(PriceChecker!=NULL) PriceChecker->SearchEdit->setText(CurBarcode);
			else if (FreeSum!=NULL) FreeSum->BarcodeEdit->setText(CurBarcode);
                 else EditLine->setText(CurBarcode);


            if (BarcodeID[i]->CodeName=="EMPLOYEE")
            {
                ;;Log((char*)(" <!> "+U2W(TempBarcode)).c_str());
                if ((!IsCurrentGoodsEmpty)
                &&(
                    (CurrentGoods->ItemSecondBarcode.empty())
                    || (W2U(CurrentGoods->ItemSecondBarcode).left(tmpID.length())==tmpID)
                 )
                )
                {
                   TempBarcode.remove(0,1);
                   CurrentGoods->ItemSecondBarcode = U2W(TempBarcode);

                   CashReg->LogEditGoods(CurrentGoods);
                }
                EditLine->clear();
                //UpdateList();
                ShowGoodsInfo();
                return true;
            }

			if (((BarcodeID[i]->CodeName=="DISCOUNTCARD") || (BarcodeID[i]->CodeName=="GASTRONOMCARD")) && (PriceChecker==NULL))
			{
				EditLine->setText("");

			    if(FreeSum!=NULL)
			    {
			        FreeSum->BarcodeEdit->setText("");
			        ShowMessage("Неправильный штрих-код товара!");

			        return false;
			    }

				int CardNumber;

				CardNumber = atoi(U2W(CurBarcode).substr(0,9).c_str());
				if((BarcodeID[i]->CodeName=="GASTRONOMCARD"))
				{
#ifdef _OLD_VARIANT_

// test, find error customers range (9930008...)
					//if((CardNumber>=800000) && (CardNumber<900000))
					//{
                        //;;cash_error* _err = new cash_error(11,0, LLong2Str(CardNumberL));
                        //;;CashReg->LoggingError(*_err);
                        //;;delete _err;
					//}

					if((CardNumberL>=993000700000LL) && (CardNumberL<993000800000LL))
					{
						CardNumber+=100000;
						CardNumberL+=100000; // Код карты Int64
					}
#endif

#ifndef CERTIFICATES
                    Log("-------------------------------------------------");
                    Log("Номер сертификата=" + Int2Str(IsCertificate(CardNumberL)));
                    Log("---------------------------------------------------");
#endif

					//запрещаем использовать сертификат как дисконтную карту
                if(IsCertificate(CardNumberL))
//					(
//						(CardNumber>=810000) && (CardNumber<=810099)
//						||
//						(CardNumber>=811000) && (CardNumber<=811099)
//						||
//						(CardNumber>=812000) && (CardNumber<=812049)
//						||
//						(CardNumber>=813000) && (CardNumber<=813049)
//					)
					{
                        //bool loc=CashReg->FindGoodsByCode(tempCode);

						ShowMessage("Невозможно использовать подарочную карту в качестве дисконтной !");
						return false;
                    }

				}

				string discount;
//				if (SearchDiscount(FindFromScaner,&discount,&CardNumber)==1)
				if (SearchDiscount(FindFromScaner,&discount,&CardNumberL)==1)
				{
					CurrentCustomer=0;
					NeedCert=0;

					CashReg->DbfMutex->lock();
					DiscountType = (CashReg->CustomersTable->Locate("ID",0)) ? CashReg->CustomersTable->GetField("DISCOUNTS") : DefaultDiscountType;
					CashReg->DbfMutex->unlock();
				}

				ShowGoodsInfo();
				return true;
			}

			if (BarcodeID[i]->CodeName=="EAN13")
			{ //if it is EAN13 then one checks weight prefix

				for (k=0;k<WeightPrefices.size();k++)
				{
					if ((CurBarcode.left(2).toInt()!=WeightPrefices[k])||(WeightPrefices[k]==0))
						continue;

					int CurWeight,tempCode;//decode weigth barcode and find a goods
					tempCode=CurBarcode.mid(2,W_Code_Length).toInt(NULL);
					if (tempCode<=0)
					{

						ShowMessage("Неправильный код товара!");
						return false;
					}



					CurWeight=CurBarcode.mid(2+W_Code_Length,10-W_Code_Length).toInt(NULL);
					;;Log((char*)(" >> Весовой товар. Префикс: "+Int2Str(WeightPrefices[k])+" Код: "+Int2Str(tempCode)+" Вес: "+Int2Str(CurWeight)).c_str());

					if (CurWeight<=0 )
					{
						
						ShowMessage("Неправильное количество товара!");
						return false;
					}

					try
					{
						if(!Cheque->GetSize())
							CashReg->GetNewGoods();

                        // + dms ==== 2019-09-03 ===== Ждем загрузки товаров =====
                        if (CashReg->IsInload()){
                            ShowMessage("Список товаров недоступен!\nДождитесь окончания загрузки товаров и\nСчитайте штрихкод еще раз!");
                            return false;
                        }
                        //WaitForLoadGoods();
                        // - dms ==== 2019-09-03 =====

						bool loc=CashReg->FindGoodsByCode(tempCode);

						CurGoods.ItemCount = CurWeight*0.001;

						if ((loc)&&(AddGoods(tempCode)))
						{
					
                            CurrentGoods->ItemBarcode = U2W(CurBarcode);
							CurrentGoods->ItemCount=RoundToNearest(
								CurWeight*0.001,CurrentGoods->ItemPrecision);					
								
							CalcCountPrice();
							CurrentGoods->CalcSum();

						ShowGoodsInfo();
	

							CashReg->LogAddGoods(CurrentGoods);

							if(CurrentGoods->ItemCount < CurrentGoods->ItemPrecision)
							{
								
								
								ShowMessage("Неправильное количество товара");
							}

							if((PriceChecker==NULL)&&(FreeSum==NULL))
							{
								QListViewItem *item = ChequeList->firstChild();
								if(item)
									while(item->itemBelow())
										item = item->itemBelow();
								if(item)
								{
									item->setText(ListNo,Int2Str(Cheque->GetSize()).c_str());
									item->setText(ListCode,Int2Str(CurrentGoods->ItemCode).c_str());
		    							item->setText(ListName,W2U(CurrentGoods->ItemName));
									item->setText(ListPrice,GetRounded(CurrentGoods->ItemPrice,0.01).c_str());
									item->setText(ListCount,GetRounded(CurrentGoods->ItemCount,CurrentGoods->ItemPrecision).c_str());
					    				item->setText(ListSum,GetRounded(CurrentGoods->ItemSum,0.01).c_str());
									item->setText(ListDscnt,GetRounded(CurrentGoods->ItemCalcSum-CurrentGoods->ItemSum,0.01).c_str());
								}
							}

							return true;
						}
						else
						{
							ShowMessage("Товар не найден!");
							return false;
						}
					}
					catch(cash_error& er)
					{
						CashReg->DbfMutex->unlock();
						CashReg->LoggingError(er);
						return false;
					}
				}
            }
			
			return LookingForGoods(BarcodeFilter);//find by barcode
		}
//        ;;Log("230322 i=="+Int2Str(i));
//        ;;Log("230322 BarcodeID[i]->CodeName=="+BarcodeID[i]->CodeName);
//        ;;Log("230322 BarcodeID[i]->CodeID=="+BarcodeID[i]->CodeID);
    }
	ShowMessage("Отсутствует информация о типе штрих-кода");
	return false;
}

double WorkPlace::GetDoubleNum(bool* err)//auxililary function
{
	QString GoodsPrice=EditLine->text();
	for (unsigned int i=0;i<GoodsPrice.length();i++)
		if (GoodsPrice.at(i)==',')
			GoodsPrice=GoodsPrice.replace(i,1,".");

	return GoodsPrice.toDouble(err);
}

bool WorkPlace::LookingForGoods
(
	 int Tag
	,QString GoodsCode
	,QString GoodsBarcode
	,QString GoodsName
	,QString GoodsPrice
	,bool LookInsideStatus
)
//in accordance with filter looking for barcode
{
    ;;Log("[FUNC] LookingForGoods [START]");
    if(!Cheque->GetSize()){
///d.gusakov
Log("GetNewGoods====STEP1====TEST");
///d.gusakov
		CashReg->GetNewGoods();
}

#ifdef ALCO_VES
    string testStringAlcoVes=EditLine->text();
#endif

#ifdef BLOCK_CIGAR
    string testString=EditLine->text();
///TestStroka	
//	testString=testString.substr(3,13);
//	Log("testString=" + testString);
///TestStroka
#endif

	QString QBarcode = EditLine->text();
	
#ifdef PIVO_BOTTLE
    ;;Log("QBarcode Test=" + QBarcode);
#endif
	
/*Log("QBarCode=" + QBarcode);
QBarcode = testString;
Log("QBarcode obrabotka=" + QBarcode);
*/	
	if (PriceChecker!=NULL) QBarcode = PriceChecker->SearchEdit->text();
	if (FreeSum!=NULL) QBarcode = FreeSum->BarcodeEdit->text();

    // + dms ==== 2019-09-03 ===== Ждем загрузки товаров =====
    if (CashReg->IsInload()){
        ShowMessage("Список товаров недоступен!\nДождитесь окончания загрузки товаров и\nСчитайте штрихкод еще раз!");
//        return false;
    }
    //WaitForLoadGoods();
    // - dms ==== 2019-09-03 =====

	int res=-1;

	if (FindInDB)
	{  // ищем в БД на сервере
        ;;Log("[FUNC] LookingForGoods [FindInDB]");
        res = CashReg->LookingForGoodsDB(
                Tag
                ,U2W(QBarcode)
                ,&CurGoods
                ,((PriceChecker==NULL)&&(FreeSum==NULL))
                ,GoodsCode
                ,GoodsBarcode
                ,GoodsName
                ,GoodsPrice
                ,LookInsideStatus);

	}
	else
	{   // ищем в dbf на кассе
        ;;Log("[FUNC] LookingForGoods [Find In Local DBF]");
	    res = CashReg->LookingForGoods(
			Tag
			,U2W(QBarcode)
			,&CurGoods
			,((PriceChecker==NULL)&&(FreeSum==NULL))
			,GoodsCode
			,GoodsBarcode
			,GoodsName
			,GoodsPrice
			,LookInsideStatus);
    }
#ifndef ZAPRET_ALCO
    QString strDate="";
    strDate=QDateTime::currentDateTime().toString("dd.MM");
    if (strDate == "16.06") res=101;
    Log("Date=" + strDate);

#endif


//;;Log("[FUNC] LookingForGoods [res]");
//Log("res=" + Int2Str(res));
	
	switch (res)
	{
		case 0:
			{
		try
		{

		    int Avans = 0;

            long long CertNumber = 0;
		    int cardbarcode = GetCertNominal(CurGoods.ItemBarcode);

		    if ((cardbarcode == CD_CERT100)
                || (cardbarcode == CD_CERT250)
                || (cardbarcode == CD_CERT500)
                || (cardbarcode == CD_CERT1000)
                || (cardbarcode == CD_CERT1500)
                || (cardbarcode == CD_CERT2000)
                || (cardbarcode == CD_CERT3000)
                || (cardbarcode == CD_CERT5000)
                )
            { // продажа сертификатов
              CertNumber = CheckBarcode(cardbarcode, "Продажа подарочной карты ("+CurGoods.ItemBarcode+")!\n\nВведите номер сертификата:");
              if (CertNumber==NotCorrect)
                {
                    ShowMessage("Неправильный номер сертификата!");
                    return false;
                }

              if (CertNumber==NoPasswd)
                {
                    ShowMessage("Неправильный НОМИНАЛ сертификата!");
                    return false;
                }

            // + dms ==== 2020-09-23 =====  Avans
                CardInfoStruct CardInfo;
                if (CashReg->FindCardInRangeTable(&CardInfo, CertNumber))
                {
                    if ( CardInfo.CardType == CD_PREPAY_CERTIFICATE)
                    {
                        Avans = 1;
                    }
                }
            // - dms ==== 2020-09-23 =====  Avans

            }

            string strLock = CurGoods.Lock;
		    if (CashReg->GoodsLock(strLock, GL_Manager) == GL_Manager)
            { // продажа с запросом ШК менеджера

              CertNumber = NotCorrect;
              do{
                CertNumber = CheckBarcode(Manager, "Продажа товара с кодом менеджера \n\n"+Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName+"\n\nВведите ШК менеджера!");
//                if ((CertNumber==NotCorrect) || (CertNumber==NoPasswd))
//                {
//                    ShowMessage("Неправильный штрихкод менеджера!");
//                }

                  ;;Log("CertNumber="+LLong2Str(CertNumber));

                  if (CertNumber==PressEscape) {
                      ;;Log("Отмена ввода ШК менеджера");
                      return false;
                  }

              }
              while ((CertNumber==NotCorrect) || (CertNumber==NoPasswd));
            }

            string resCheckSugar = "";
			if(CheckSugar(&resCheckSugar, CurGoods.ItemCode, CurGoods.ItemCount))
			{
                bool oldscanersuspend = Scanner->suspend;
                Scanner->suspend = true;
			    ShowMessage("ТОВАР НЕ ДОБАВЛЕН\n\n"+resCheckSugar);
			    Scanner->suspend = oldscanersuspend;
			    return false;
            }

            // + EGAIS
            int TextileCnt = 0;
            int MilkCnt = 0;
            int CigarCnt = 0;

#ifdef ALCO_VES
            int PivoRazlivCnt = 0;
#endif

            int AlcoCnt = 0;
            string ActsizBarcode = "";
            strLock = CurGoods.Lock;

            vector<string> ListActsizBarcodes;

// Возможность отбития без акциза вводилась на переходный период
// Сейчас все алко-товары проходят по ЕГАИС со считываем акциза
// Проверку отключаю
//            if (CashReg->EgaisSettings.ModePrintQRCode>-1)
            {
                if (CashReg->GoodsLock(strLock, GL_AlcoEGAIS) == GL_AlcoEGAIS)
                { // Продажа алкогольной продукции

                    int errAct=0;
                    AlcoCnt = (CurGoods.ItemAlco>0)?CurGoods.ItemAlco:1;

                    for (int j=0;j<AlcoCnt;j++)
                    {
                      errAct=0;

                      ActsizBarcode = "";

                      errAct = CashReg->CheckActsizBarcode(&ActsizBarcode, "Позиция # "+Int2Str(j+1)+" из "+Int2Str(AlcoCnt)+" \n\n"+ Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName, Cheque, ListActsizBarcodes);

                      if (errAct) return false;

                      ListActsizBarcodes.insert(ListActsizBarcodes.end(), ActsizBarcode);

                    }

                    //int errAct = CashReg->CheckActsizBarcode(&ActsizBarcode, Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName, Cheque);
                    //
                }
                else

#ifdef ALCO_VES
                    //Разливное пиво
                            //if (QBarcode.length()==44)
                    if (CashReg->GoodsLock(strLock, GL_BeerEGAIS) == GL_BeerEGAIS)
                    {
						
						 Log("TestPIVOPIVOPIVO");
						
                        ActsizBarcode = "";

                        Scanner->allsymbols = true;
                        int errAct = CashReg->CheckActsizBarcodeDataMatrix(&ActsizBarcode, "Позиция # 1 из 1 \n\n"+ Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName, Cheque, ListActsizBarcodes, AT_PIVO_RAZLIV);
                        Scanner->allsymbols = false;

                        if (errAct) return false;

                        ListActsizBarcodes.insert(ListActsizBarcodes.end(), ActsizBarcode);

                        PivoRazlivCnt = 1;
					}

                    else

#endif


                // + CIGAR
                // Табачная продукция
                    if (CashReg->GoodsLock(strLock, GL_CigarEGAIS) == GL_CigarEGAIS)
                    {
                        CigarCnt = 1;
											
                        if ((QBarcode.length()==29) || (QBarcode.length()==52) ||
                                (QBarcode.length()==41) || (QBarcode.length()==55)){

                            int errAct = CashReg->CheckCigarBarcode(U2W(QBarcode), Cheque);
                            if (errAct) return false;
							
							;;Log("CigarActsiz=" + QBarcode);
							
                            QBarcode = testString;
                            ListActsizBarcodes.insert(ListActsizBarcodes.end(), U2W(QBarcode));

							
#ifdef BLOCK_CIGAR
                            if (QBarcode.length()==52)
                            {
                               char spec = 0x1D;

                               testString=testString.substr(0,25)+spec+
                                       testString.substr(25,10)+spec+
                                       testString.substr(35,6)+spec+
                                       testString.substr(41,11);
                               QBarcode = testString;
                               ListActsizBarcodes.insert(ListActsizBarcodes.end(), U2W(QBarcode));

                            }

                            else if (QBarcode.length()==41)
                            {
                                char spec = 0x1D;
                                testString=testString.substr(0,25)+spec+
                                    testString.substr(25,10)+spec+
                                    testString.substr(35,6);
                                QBarcode = testString;
                                ListActsizBarcodes.insert(ListActsizBarcodes.end(), U2W(QBarcode));
                            }

                            else if(QBarcode.length()==55)
                            {
                                char spec = 0x1D;
                                testString=testString.substr(0,25)+spec+
                                    testString.substr(25,10)+spec+
                                    testString.substr(35,6)+spec+
                                    testString.substr(41,14);
                                QBarcode = testString;
                                ListActsizBarcodes.insert(ListActsizBarcodes.end(), U2W(QBarcode));
                            }
#endif

                        } else {
                            if (CashReg->GoodsLock(strLock, GL_CigarLock) == GL_CigarLock)
                            {
                                // С 2020-07-01 Действует Запрет продажи сигарет без акциза
                                // Остальная табачная продукция с 2021-07-01 (через год)
                                return false;
                            }
                        }

#ifdef BLOCK_CIGAR
                        // Проверка на повтор
                        bool isOk=true;
                        if (isOk)
                        {
                            FiscalCheque* CurCheque = Cheque;
                            if (!CurCheque)
                            {
                                return -1;
                            }
                            for (int i=0;i<CurCheque->GetCount();i++)
                            {
                                 GoodsInfo g=(*CurCheque)[i];

                                 if ((g.ItemFlag==SL) || (g.ItemFlag==RT))
                                 {
                                     for (int j=0;j<g.Actsiz.size();j++)
                                        if(testString==g.Actsiz[j])
                                        {
                                            isOk=false;
                                            break;
                                        }
                                 }
                                  if (!isOk)
                                  {
                                          break;
                                  }
                            }
                        }

                        if (isOk)
                        {

                        }
                        else
                        {
                            EditLine->clear();
                            return false;
                        }


#endif

                    }
                    else
                    {
                        // С 2021-01-01 Действует Запрет продажи товаров легкой промышленности без акциза

                        if (CashReg->GoodsLock(strLock, GL_TextileEGAIS) == GL_TextileEGAIS)
                        {

                              ActsizBarcode = "";

                              Scanner->allsymbols = true;
                              int errAct = CashReg->CheckActsizBarcodeDataMatrix(&ActsizBarcode, "Позиция # 1 из 1 \n\n"+ Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName, Cheque, ListActsizBarcodes, AT_TEXTILE);
                              Scanner->allsymbols = false;

                              if (errAct) return false;

                              ListActsizBarcodes.insert(ListActsizBarcodes.end(), ActsizBarcode);

                              TextileCnt = 1;

                        }
                        else
                        {

                            // С 2021-01-01 Действует Запрет продажи товаров легкой промышленности без акциза

                            if (CashReg->GoodsLock(strLock, GL_MilkEGAIS) == GL_MilkEGAIS)
                            {
                                if (MilkActsiz!="") {
                                    ;;Log("MilkActsiz = " + MilkActsiz);
#ifdef PIVO_BOTTLE
		;;Log("markirovka=" + QBarcode);
#endif
                                    char spec = 0x1D;
                                    if (MilkActsiz.length()==30) {
                                        // Добавляем спецсимвол
                                        ;;Log("Origin MilkActsiz DataMatrix (30)="+MilkActsiz);
                                        //char spec = 0x1D;
                                        ActsizBarcode = MilkActsiz.substr(0, 24) + spec
                                                + MilkActsiz.substr(24, 6);
                                    } else if (MilkActsiz.length()==32) {
                                        // Добавляем спецсимвол
                                        ;;Log("Origin MilkActsiz DataMatrix (32)="+MilkActsiz);
                                        //char spec = 0x1D;
                                        ActsizBarcode = MilkActsiz.substr(0, 26) + spec
                                                + MilkActsiz.substr(26, 6);
                                    } else if (MilkActsiz.length()==37) {
                                        ActsizBarcode = MilkActsiz.substr(0, 31) + spec
                                                + MilkActsiz.substr(31, 6);
                                    } else if (MilkActsiz.length()==38) {
                                        if (MilkActsiz[31]!=spec) {
                                            // Добавляем спецсимвол
                                            ;;Log("Origin MilkActsiz DataMatrix (38)="+MilkActsiz);
                                            //char spec = 0x1D;
                                            ActsizBarcode = MilkActsiz.substr(0, 32) + spec
                                                    + MilkActsiz.substr(32, 6);
                                        } else

                                        {
                                            ActsizBarcode = MilkActsiz;
                                        }
                                    } else
										
#ifdef PIVO_BOTTLE
                                        if ((QBarcode.length()==31) /*|| (QBarcode[25]==spec)*/)
                                        {
                                            Log("Получаем акцизную марку пива в таре");
											                                    ///Проверяем на символ разделителя
											if (MilkActsiz==MARKINGACTSIZ)
											{
												;;Log("MARKINGACTSIZ=111111111111111111111111111111");
												ActsizBarcode = "";
												//ResMatch = 2;
											}
                                            // Добавляем спецсимвол
                                            else if (MilkActsiz[25]!=spec)
                                            { 
                                                ActsizBarcode = MilkActsiz.substr(0, 25) + spec
                                                        + MilkActsiz.substr(25, 6);
												;;Log("Пиво в бутылке или банке - Origin Actsiz DataMatrix (31)=" + ActsizBarcode);
                                            }
                                            else Log("Пиво в бутылке или банке - Origin Actsiz DataMatrix (32)=" + ActsizBarcode);
                                        }
                                    else
#endif

#ifdef BIODOBAVKA
                                            if (QBarcode.length()==83)
                                            {
                                                Log("Получаем акцизную марку биодобавки");
                                                // Добавляем спецсимвол
                                                ;;Log("Origin MilkActsiz DataMatrix (83)="+MilkActsiz);
                                                ActsizBarcode = MilkActsiz.substr(0, 31) + spec
                                                        + MilkActsiz.substr(31, 6) + spec + MilkActsiz.substr(37, 46);
                                                Log("Акциз Биодобавки = " + MilkActsiz);
                                            }
                                        else
#endif

#ifndef ALCO_VES
                                            if (QBarcode.length()==46)
                                            {
                                                Log("Получаем акцизную марку Разливного пива");
                                               /* char spec = 0x1D;
                                                testStringAlcoVes=testStringAlcoVes.substr(0,25)+spec+
                                                          testStringAlcoVes.substr(25,6);
                                                QBarcode = testStringAlcoVes;
                                                ListActsizBarcodes.insert(ListActsizBarcodes.end(), U2W(QBarcode));*/
                                                Log("Актциз пива = " + MilkActsiz);
                                            }
                                    else
#endif

                                    {
                                        ;;Log("ERROR MilkActsiz DataMatrix length ("
                                              + Int2Str(MilkActsiz.length()) + ")");
                                    }
                                    // Проверка на повтор
                                    bool isOk=true;

                                    // Проверка на повтор в текущем наборе
                                    if (ListActsizBarcodes.size()>0)
                                    {

                                        for (int j=0;j<ListActsizBarcodes.size();j++)
                                           if(ActsizBarcode==ListActsizBarcodes[j]){

                                               isOk=false;
                                                break;
                                           }
                                    }

                                    // Проверка на повтор в текущем чеке
                                    if (isOk)
                                    {
                                        FiscalCheque* CurCheque = Cheque;
                                        if (!CurCheque)
                                            return -1;
                                        for (int i=0;i<CurCheque->GetCount();i++)
                                        {

                                            GoodsInfo g=(*CurCheque)[i];

                                            if ((g.ItemFlag==SL) || (g.ItemFlag==RT))
                                            {
                                                for (int j=0;j<g.Actsiz.size();j++)
                                                   if(ActsizBarcode==g.Actsiz[j]){

                                                       isOk=false;
                                                        break;
                                                   }
                                            }
#ifdef PIVO_BOTTLE
							if(ActsizBarcode == MARKINGACTSIZ) isOk=true;
#endif
                                            if (!isOk)
                                                break;
                                        }
                                    }

                                    if (isOk)
                                    {

                                    }
                                    else
                                    {
                                        // Это не акциз, продолжаем считывание

                                        EditLine->clear();
                                        return false;
                                    }
                                } else {
                                    ActsizBarcode = "";
#ifdef PIVO_BOTTLE
                                MilkActsiz = "";
#endif

                                    Scanner->allsymbols = true;
                                    int errAct = CashReg->CheckActsizBarcodeDataMatrix(
                                                &ActsizBarcode,
                                                "Позиция # 1 из 1 \n\n"
                                                + Int2Str(CurGoods.ItemCode)+" "+ CurGoods.ItemName,
                                                Cheque, ListActsizBarcodes, AT_MILK
                                                );
                                    Scanner->allsymbols = false;

                                    if (errAct) return false;
                                }

                                if (ActsizBarcode!=SERVICE_EAN_MILK) {
                                    ListActsizBarcodes.insert(ListActsizBarcodes.end(), ActsizBarcode);
                                    MilkCnt = 1;
                                } else {
                                    if (CashReg->GoodsLock(strLock, GL_CigarLock) == GL_CigarLock) {
                                        ShowMessage("Маркировка DataMatrix обязательна!");
                                        return false;
                                    }
                                }

                            }
                        }
                    }
                // - CIGAR
            }
            // - EGAIS

            CurrentGoods=&CurGoods;
			CurrentGoods->ItemSecondBarcode = CertNumber>0?LLong2Str(CertNumber):"";
			CurrentGoods->ItemDepartment	=CashReg->ConfigTable->GetLongField("DEPARTMENT");
			CurrentGoods->ItemNumber=Cheque->GetSize()+1;
	    		CurrentGoods->ItemFlag=SL;
			CurrentGoods->GuardCode=-1;
			CurrentGoods->PaymentType=CashPayment;
			time(&(CurrentGoods->tTime));

			CurrentGoods->KKMNo=0;
			CurrentGoods->KKMSmena=0;
			CurrentGoods->KKMTime=0;

            // + EGAIS
			//CurrentGoods->ItemAlco = isAlco;
			//CurrentGoods->ItemActsizBarcode = ActsizBarcode;

            CurrentGoods->ItemCigar = CigarCnt;
            CurrentGoods->ItemAlco = AlcoCnt;
            CurrentGoods->ItemTextile = TextileCnt;
            CurrentGoods->ItemMilk = MilkCnt;
#ifdef ALCO_VES
            CurrentGoods->ItemPivoRazliv = PivoRazlivCnt;
#endif

#ifdef PIVO_BOTTLE
                                MilkActsiz = "";
#endif
            CurrentGoods->ItemMask = IsMask(CurrentGoods->ItemCode);

            CurrentGoods->Actsiz.clear();
            for (int j=0;j<ListActsizBarcodes.size();j++)
                CurrentGoods->Actsiz.insert(CurrentGoods->Actsiz.end(), ListActsizBarcodes[j]);

			CurrentGoods->ItemQRBarcode = "";
            // - EGAIS

            CurrentGoods->Avans = Avans;

            if (FindInDB && CurrentGoods->InputType==IN_SCAN)
                    CurrentGoods->InputType=IN_DBRSCAN;
			else if (HandBarcode && CurrentGoods->InputType==IN_SCAN)
                    CurrentGoods->InputType=IN_RSCAN;

			if(!Cheque->GetCount())
				CashReg->m_vem.StartDoc(1, CashReg->GetLastCheckNum()+1);

			if ((PriceChecker==NULL)&&(FreeSum==NULL))
				IsCurrentGoodsEmpty=false;

            AppendRecord2Check();

			CashReg->m_vem.AddGoods(CurrentGoods->ItemNumber,CurrentGoods,Cheque->GetCalcSum(),CurGoods.InputType);

            CashReg->LogAddGoods(CurrentGoods);


			return true;
		}
		catch(cash_error& er)
		{
			CashReg->LoggingError(er);
			return false;
		}
				ShowGoodsInfo();
				return true;
			}
		case 1:
			{
				ShowMessage("Неправильная цена!");
				return false;
			}
		case 2:
			{
				ShowMessage("Товар с указанной ценой не найден!");
				return false;
			}
		case 3:
			{
				ShowMessage("Неправильный номер группы!");
				return false;
			}
		case 4:
			{
				ShowMessage("Товар с указанной группой не найден!");
				return false;
			}
		case 5:
			{

				string Barcode = U2W(EditLine->text());
                if (FreeSum!=NULL)
                {
                    Barcode = U2W(FreeSum->BarcodeEdit->text());
                    if (!CashReg->isGuard(Barcode))
                    {
                        FreeSum->InfoLabel->setText("");
                        FreeSum->valid = true;
                    }
                    else
                    {
                        FreeSum->BarcodeEdit->setText("");
                        ShowMessage("Неправильный штрих-код товара!");
                        FreeSum->valid = false;
                    }

                }
                else
                {
                    if (PriceChecker!=NULL)
                    {
                       Barcode = U2W(PriceChecker->SearchEdit->text());
                    }
                    else
                    {
                        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,
                            WRONG_BARCODE, 0, CashReg->CurrentSaleman->Id, -1, Barcode.c_str());
                    }
                }

				EditLine->clear();
                if (FreeSum==NULL) ShowMessage("Товар с указанным штрих-кодом не найден! ("
                                               + QBarcode + ")");
				return false;
			}
		case 6:
			{
				ShowMessage("Неправильный код товара!");
				return false;
			}
		case 7:
			{
				ShowMessage("Товар с указанным кодом не найден! (7)");
				return false;
			}
		case 8:
			{
				switch (Tag)
				{
					case BarcodeFilter:
						ShowMessage("Товар с указанным штрих-кодом не найден! (8)");
						break;
					case PriceFilter:
						ShowMessage("Товар с указанной ценой не найден! (8)");
						break;
					case GroupFilter:
						ShowMessage("Товар с указанной группой не найден! (8)");
						break;
					case CodeFilter:
						ShowMessage("Товар с указанным кодом не найден! (8)");
						break;
					case PropertiesFilter:
						ShowMessage("Товар с указанными свойствами не найден! (8)");
						break;
				}
				return false;
			}
		case 9:
			{
			    ShowMessage("Неправильная цена товара! Сообщите товароведу! (9)");
				return false;
			}
		case 10:
			{
				ShowMessage("Неправильная цена товара! Сообщите товароведу!");
				return false;
			}
		case 11: {
                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CANCELCIGARETTE, 0, CashReg->CurrentSaleman->Id, -1, (const char *)(U2W(QBarcode).c_str()));
                return false;
            }
		case 12:
			{
				return false;
			}
		case 13:
			{
                ShowMessage("Запрещена продажа сигарет без акциза!\nСчитайте акциз.");
				return false;
			}
		case GL_TimeLock:
			{
			    ShowMessage("Продажа товара запрещена");
				return false;
			}
		case GL_FindByCode:
			{
			    ShowMessage("Недостаточно прав (CD)");
				return false;
			}
		case GL_FindByPrice:
			{
			    ShowMessage("Недостаточно прав (PR)");
				return false;
			}
		case GL_FindByName:
			{
			    ShowMessage("Недостаточно прав (NM)");
				return false;
			}
        case GL_Lock:
            {
                ShowMessage("Lock");
                return false;
            }
        //default: ShowMessage((char*)Int2Str(res).c_str());

	};
}

bool WorkPlace::CompareKey(int Key,string Pattern,string* ptrKeyName)//try to understand the type of the operation
{

	return CashReg->CompareKey(Key,Pattern,ptrKeyName);
}

void WorkPlace::closeEvent(QCloseEvent* e)//close saleman's workplace
{
	InitWorkPlace();
/*
	if(Cheque->GetCount()>0){
		ShowMessage("Для завершения работы аннулируйте чек.");
		return;
	}
*/
	if (ShowQuestion("Завершить работу кассира?",false)==true)
	{
		CloseWorkPlace();
		QDialog::closeEvent(e);
	}
}

void WorkPlace::keyPressEvent(QKeyEvent* e)//analogously
{
    ;;Log("#keyPressEvent="+Int2Str(e->key()));

	InitWorkPlace();
	if ((e->key()==Qt::Key_Escape))
	{
	    QCloseEvent ev = QCloseEvent();
		closeEvent(&ev);
/*
		if (ShowQuestion("Завершить работу кассира?",false)==true)
		{
			CloseWorkPlace();
			QDialog::keyPressEvent(e);
		}
*/
	}
	else{
		if(ShiftButton & e->state())
			PaymentType = ShiftDown;
		//ShowMessage("keyPressEvent");

		QDialog::keyPressEvent(e);
	}
}

void WorkPlace::ProcessKey(int Key)//proccess command key
{

    //;;Log("#ProcessKey="+Int2Str(Key));

    AltFinal=false;
    RestartTimerSS(true);

	InitWorkPlace();

	if (Key==0)
		return;

    ;;Log("#ProcessKey="+Int2Str(Key));

	//if up or down arrows were pressed we send them to cheque form
	if ((Key==Key_Up)||(Key==Key_Down)||(Key==Key_PageUp)||(Key==Key_PageDown))
		thisApp->sendEvent(ChequeList,new QKeyEvent(QEvent::KeyPress,Key,0,0));

	try
	{
		string KeyName;

		if (CompareKey(Key,INFO))
		{
		    if(EditLine->ctrl)
		    {
                Log("PressKey>> INFO");
                EditLine->clear();
                if (CurrentCustomer==0LL) return;

                bPriceCheck=true;
                PriceChecker = new PriceCheck(this);
                PriceChecker->SearchEdit->setText(W2U(LLong2Str(CurrentCustomer)));
                PriceChecker->ProcessKey(12345); // Автопоиск
                //PriceChecker->ProcessKey(4148); // F5
                (PriceChecker->exec()==QDialog::Accepted);
                bPriceCheck=false;
                delete PriceChecker;
                PriceChecker=NULL;
		    }
		    else
		    {
                Log("PressKey>> INFO");
                EditLine->clear();
                if (!EditLineCheck(""))
                    return;
                if (ShowQuestion("Открыть окно информации о товаре?",false))
                {
                    bPriceCheck=true;
                    PriceChecker = new PriceCheck(this);
                    (PriceChecker->exec()==QDialog::Accepted);
                    bPriceCheck=false;
                    delete PriceChecker;
                    PriceChecker=NULL;
                }
		    }
		}

		if (CompareKey(Key,SEARCHBYCODE,&KeyName))//looking for goods by code
		{
            FindInDB = EditLine->ctrl;
		    string msg = "PressKey>> SEARCH BY CODE ";
            msg += (FindInDB?"IN BASE ":"");
		    msg += "("+U2W(EditLine->text())+")";
	        Log((char*)msg.c_str());

			if(EditLine->text().isEmpty())
				EditLine->setText(KeyName.substr(strlen(SEARCHBYCODE),KeyName.length()-strlen(SEARCHBYCODE)).c_str());
			if (!EditLineCheck("Введите код товара!"))
			{
			    FindInDB = false;
				return;
			}

			if (LookingForGoods(CodeFilter))//show all goods with the entered code
				EditLine->clear();
            FindInDB = false;

			return;
		}

		if (CompareKey(Key,SEARCHBYPRICE))//looking for goods by price
		{
            FindInDB = EditLine->ctrl;
		    string msg = "PressKey>> SEARCH BY PRICE ";
            msg += (FindInDB?"IN BASE ":"");
		    msg += "("+U2W(EditLine->text())+")";
	        Log((char*)msg.c_str());

			if (!EditLineCheck("Введите цену товара!"))
			{
                FindInDB = false;
				return;
			}

			if (LookingForGoods(PriceFilter))//show all goods with the entered price
				EditLine->clear();

            FindInDB = false;
			return;
		}

		if (CompareKey(Key,SEARCHBYGROUP))//looking for goods by group code
		{
		    Log("PressKey>> SEARCH BY GROUP");
			if (!EditLineCheck(""))
				return;

			if (!EditLine->text().isEmpty())
			{
				//show all goods with the selected group
				if (LookingForGoods(GroupFilter))
					EditLine->clear();
			}
			else
			{

				int SelPos;  //show list of groups
				vector<int> GrCodes;
				vector<string> S_Names;

				CashReg->GetGroup(&GrCodes,&S_Names);

				SectionSelForm *dlg = new SectionSelForm(this);
				dlg->SelRow=&SelPos;
				dlg->ShowSectionList(GrCodes,S_Names);
				if (dlg->exec()==QDialog::Accepted)
					EditLine->setText(Int2Str(GrCodes[SelPos]).c_str());
				delete dlg;

			}
			return;
		}

		if (CompareKey(Key,SEARCHBYPROPERTIES))//advanced goods search
		{
		    Log("PressKey>> SEARCH BY PROPERTIES");
			EditLine->clear();
			if (!EditLineCheck(""))
				return;

			SearchForm *dlg = new SearchForm(this);
			QString GoodsCode,GoodsBarcode,GoodsName,GoodsPrice;
			bool LookInsideStatus=false;
			for(;;)
			{
				dlg->CodeEdit->setText(GoodsCode);
				dlg->BarcodeEdit->setText(GoodsBarcode);
				dlg->NameEdit->setText(GoodsName);
				dlg->PriceEdit->setText(GoodsPrice);
				dlg->LookInside->setChecked(LookInsideStatus);
				if (dlg->exec()==QDialog::Accepted)
				{
					GoodsCode	= dlg->CodeEdit->text();
					GoodsBarcode	= dlg->BarcodeEdit->text();
					GoodsName	= dlg->NameEdit->text();
					LookInsideStatus= dlg->LookInside->isChecked();
					GoodsPrice	= dlg->PriceEdit->text();

					bool checknum;

					if (!GoodsCode.isEmpty())
					{
						GoodsCode.toInt(&checknum);
						if (!checknum)
						{
							ShowMessage("Неправильный код товара!");
							continue;
						}
					}

					if (!GoodsPrice.isEmpty())
					{
						GoodsPrice.toDouble(&checknum);
						if (!checknum)

						{

							ShowMessage("Неправильная цена товара!");
							continue;
						}
					}
					try
					{
						thisApp->setOverrideCursor(waitCursor);
						InfoMessage->setText(W2U("Поиск товара"));

						LookingForGoods(PropertiesFilter,GoodsCode,GoodsBarcode,GoodsName,GoodsPrice,LookInsideStatus);

						InfoMessage->clear();
						thisApp->restoreOverrideCursor();
					}
					catch(cash_error& er)
					{
						CashReg->LoggingError(er);
					}
				}
				break;
			}

			delete dlg;
			return;
		}

		if (CompareKey(Key,SEARCHBYBARCODE))//looking for goods by barcode
		{

            FindInDB = EditLine->ctrl;
		    string msg = "PressKey>> SEARCH BY BARCODE ";
            msg += (FindInDB?"IN BASE ":"");
		    msg += "("+U2W(EditLine->text())+")";
	        Log((char*)msg.c_str());

            if (!EditLineCheck("Введите штрих-код товара!"))
            {
                FindInDB = false;
                return;
            }

                if (EditLine->text().left(3)=="993") // ручной ввод карты
                {
                    //if(!CashReg->BonusMode(lcard))
                        return ;

/*
                    ;;cash_error* _err = new cash_error(11,0, "Barcode (F5): "+U2W(EditLine->text()));
                    ;;CashReg->LoggingError(*_err);
                    ;;delete _err;
*/
                }


                HandBarcode = true;

                ProcessBarcode();

                HandBarcode = false;
                FindInDB = false;

			return;
		}

		if (CompareKey(Key,SCALEGOODS)) //scaling
		{
		    Log("PressKey>> SCALEGOODS");
			if (!EditLineCheck(""))
				return;

			int w=0;

			if (Scales->GetWeight(w))
				EditLine->setText(GetRounded(w*0.001,0.001).c_str());
			else
				ShowMessage("Не могу определить вес товара!");

			return;
		}
		if (CompareKey(Key,ENTERERROR))//delete current goods
		{
		    Log("PressKey>> ENTERERROR");
			QListViewItem *item = ChequeList->firstChild();
			if(item){
				while(item->itemBelow())
					item = item->itemBelow();
				ChequeList->setSelected(item,true);
			}
		}
		if (CompareKey(Key,DELETEFROMCHECK) || CompareKey(Key,ENTERERROR))//remove goods from check
		{
		    Log("PressKey>> DELETEFROMCHECK");
			if (!EditLineCheck(""))
				return;

			if(!ChequeList->selectedItem()){
				ShowMessage("Не выбрана позиция.");
				return;
			}

			if (Cheque->GetSize()>0)
			{
				if (ShowQuestion("Удалить покупку из чека",false))
				{
					int GuardRes=NotCorrect;
					if ((!CashReg->CurrentSaleman->Storno)&&(!CheckGuardCode(GuardRes)))
						return;

					//screen form of the cheque doesn't show storno goods
					int ChequePos;//so we have to find internal index of the selected goods
					int ChequeCount=0;

					for (ChequePos=0;ChequePos<Cheque->GetCount();ChequePos++)
					{
						if ((*Cheque)[ChequePos].ItemFlag!=ST)
							ChequeCount++;
						if (ChequeCount==ChequeList->selectedItem()->text(ListNo).toInt())
							break;
					}

					if (ChequePos==Cheque->GetCount())
						return;

					//try to make 'storno' operation
					CashReg->ProcessRegError(FiscalReg->Storno((*Cheque)[ChequePos].ItemName.c_str(),
						(*Cheque)[ChequePos].ItemFullPrice,(*Cheque)[ChequePos].ItemCount,(*Cheque)[ChequePos].ItemDepartment));

					GoodsInfo g=(*Cheque)[ChequePos];

					g.ItemFlag=ST;
					g.GuardCode=GuardRes;

//					CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,ST,
//						g.ItemSum,CashReg->CurrentSaleman->Id,GuardRes);

					Cheque->SetAt(ChequePos,g);

					if (ChequeList->selectedItem()!=NULL)
						delete ChequeList->selectedItem();

					if(Cheque->GetSize()==0)
					{
						ShowMessage("Чек пуст и будет аннулирован!");
						CurrentCustomer = 0;
						NeedCert=0;
						CurrentCertificate=0;
						CashReg->LoadDiscountTypeFromDB(0,&DiscountType);
						CashReg->ProcessRegError(FiscalReg->Cancel());//annul cheque
						IsChequeInProgress=false;
						CashReg->AnnulCheck(Cheque,GuardRes);
						IsChequeInProgress=false;
						InitWorkPlace();
						AutoRestoreKeepedCheque();
					}
					ProcessDiscount(Cheque);

					CashReg->m_vem.DeleteGoods(-1, &g, Cheque->GetCalcSum());

					CashReg->LogDelGoods(&g);

					QListViewItem *lastItem=ChequeList->firstChild();

					if (lastItem!=NULL)//highlight the goods in the check
					{
						for(int i=1;;i++)
						{
							lastItem->setText(ListNo,Int2Str(i).c_str());

							if (ChequeCount==i)
							{
								ChequeList->setSelected(lastItem,true);
								ChequeList->setCurrentItem(lastItem);
							}

							if (lastItem->itemBelow()!=NULL)
								lastItem=lastItem->itemBelow();
							else
								break;
						}

						if (ChequeCount>ChequeList->childCount())
						{
							ChequeList->setSelected(lastItem,true);
							ChequeList->setCurrentItem(lastItem);
						}
					}

					if (!IsCurrentGoodsEmpty){
						IsCurrentGoodsEmpty = true;
						CurGoods.init();
						CurrentGoods = &CurGoods;
					}

//					CashReg->ClearCurrentCheck();
//					CashReg->WriteChequeIntoTable(CashReg->CurrentCheckTable,Cheque,CashReg->GetCurTime());

					ShowGoodsInfo();
				}
			}
			else
				ShowMessage("Пустой чек!");
			return;
		}

		if (CompareKey(Key,CALCULATOR))//show calculator
		{
		    Log("PressKey>> CALCULATOR");
			CalcForm *dlg = new CalcForm(this);

			dlg->exec();
			delete dlg;
			return;
		}

		if (CompareKey(Key,LASTCHECK))//последний чек
		{
		    Log("PressKey>> LASTCHECK");
			if(!ShowQuestion("Повторить чек?",false))
				return;
			if(Cheque->GetCount()){
				ShowMessage("Текущий чек не пуст.");
				return;
			}
			RestoreSet(false, true);
			UpdateList();
			return;
		}

		if (CompareKey(Key,KEEP))//отложенный чек
		{
		    Log("PressKey>> KEEP");
			keepstate=fopen("tmp/keepstate.dat","r");
			if(!keepstate)
			{
				if (!Cheque->GetSize())
				{//чеки пусты
					ShowMessage("Текущий и отложенный чеки пусты!!!");
					KeepCheckInfo->setProperty( "text","");
					return;
				}
				else
				{//отложить
					if(!ShowQuestion("Отложить текущий чек?",false))
						return;
					if (rename("tmp/state.dat","tmp/keepstate.dat")){
						ShowMessage("Ошибка откладывания чека!\nСообщите администратору!");
						return;
					}

					CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,19,Cheque->GetSum(),CashReg->CurrentSaleman->Id,-1);

                    // Реверс cписания баллов
                    if ((CashReg->WPCX!=NULL) && (Cheque->BankBonusSumm>0))
                    {// Cancel bank bonus discounts
                        ;;Log(" Cancel bank bonus discounts (KEEP)");
                        ProcessBankBonusDiscountReverse(CashReg->WPCX->PCX_Result.card, CashReg->WPCX->PCX_Result.sha1, true);
                    }

                    // Реверс cписания бонусов
                    if ((CashReg->BNS!=NULL) && (Cheque->BonusSumm>0))
                    {// Cancel bonus discounts
                        ;;Log(" Cancel bonus discounts (KEEP)");

                        ProcessBonusDiscountReverse(true);
                    }

					keepstate=fopen("tmp/keepstate.dat","a");
					fprintf(keepstate, "KCHNUM=%d\n",CashReg->GetLastCheckNum()+1);
					fprintf(keepstate, "KCHTIME=");
					fputs(CashReg->GetCurDate().c_str(), keepstate);
					fputs("/",keepstate);
					fputs(CashReg->GetCurTime().c_str(), keepstate);
					fputs("\n",keepstate);
					fclose(keepstate);

					storenum = fopen("./tmp/storenum.dat","r");
					if (storenum){
						int storechecknum;
						fscanf(storenum,"%d",&storechecknum);
						fclose(storenum);
						remove("./tmp/storenum.dat");
						if ((storechecknum-CashReg->GetLastCheckNum())>100 || (CashReg->GetLastCheckNum()-storechecknum)>100 || storechecknum>1000000000 || CashReg->GetLastCheckNum()>1000000000){
							char tmpstr[128];//="Ошибка нумерации чеков. Текущий чек: "+CashReg->GetLastCheckNum()+". Отложенный чек: "+storechecknum+".\n Сообщите администратору!!!"
							sprintf(tmpstr,"Ошибка нумерации чеков. Текущий чек: %d.\n Сообщите администратору!!!",storechecknum);
							ShowMessage((string)tmpstr);
						}
						CashReg->SetLastCheckNum(storechecknum-1);
					}
					else
						CashReg->SetLastCheckNum(CashReg->GetLastCheckNum()+1);

					CurrentCustomer = 0;
					NeedCert=0;
					CurrentAddonCustomer = 0;
					CurrentBonusBalans=0;
					CashReg->LoadDiscountTypeFromDB(CurrentCustomer,&DiscountType);

					Cheque->Clear();
					CashReg->ClearCurrentCheck();//
					ChequeList->clear();
					KeepCheckInfo->setProperty( "text",W2U("O.Ч."));
					remove("./tmp/storenum.dat");
					CashReg->ClearPayment();

                    IsCurrentGoodsEmpty=true;
                    InitWorkPlace(true);



					ShowMessage("Текущий чек отложен!");

					return;
				}
			}
			else
			{
				fclose(keepstate);
				if (!Cheque->GetSize())
				{//восстановить
					if(!ShowQuestion("Восстановить отложенный чек?",false))
						return;
					RestoreKeepedCheque();
					//painter.end();
					return;
				}
				else
				{//отложить и восстановить
					if(!ShowQuestion("Отложить текущий и восстановить отложенный чеки?",false))
						return;

					keepstate = fopen("./tmp/keepstate.dat","r");
					int storechecknum;
					if (keepstate){
						char buf[256];
						while(!feof(keepstate)){
							fscanf(keepstate,"%s",buf);
							sscanf(buf,"KCHNUM=%d", &storechecknum);
						}
						fclose(keepstate);
						storenum = fopen("./tmp/storenum.dat","w");
						fprintf(storenum,"%d",CashReg->GetLastCheckNum()+2);
						fclose(storenum);
					}
					if (rename("tmp/state.dat","tmp/tmpstate.dat") ||
						rename("tmp/keepstate.dat","tmp/state.dat") ||
						rename("tmp/tmpstate.dat","tmp/keepstate.dat")) {
						ShowMessage("Ошибка смены чеков!\nСообщите администратору!");
						return;
					}
					CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,19,Cheque->GetSum(),CashReg->CurrentSaleman->Id,-1);
					keepstate=fopen("tmp/keepstate.dat","a");
					if (keepstate){
						fprintf(keepstate, "KCHNUM=%d\n",CashReg->GetLastCheckNum()+1);
						fprintf(keepstate, "KCHTIME=");
						fputs(CashReg->GetCurDate().c_str(), keepstate);
						fputs("/",keepstate);
						fputs(CashReg->GetCurTime().c_str(), keepstate);
						fputs("\n",keepstate);
						fclose(keepstate);
					}
					CashReg->SetLastCheckNum(storechecknum-1);

					Cheque->Clear();
					EditLine->clear();
					CashReg->ClearPayment();

					RestoreSet();
					UpdateList();

					KeepCheckInfo->setProperty( "text",W2U("O.Ч."));
					//KeepCheckInfo_font.setStrikeOut(true);
					ShowMessage("Отложенный восстановлен и текущий отложен!");
					return;
				}
			}
		}

		if (CompareKey(Key,ENTERERROR))//delete current goods
		{
		    Log("PressKey>> ENTERERROR_2");
			if (!EditLineCheck(""))
				return;

			if (IsCurrentGoodsEmpty)
			{
				ShowMessage("Нечего удалять!");
				return;
			}

			if (ShowQuestion("Удалить покупку?",false))
			{
				int GuardRes=NotCorrect;
				if ((!CashReg->CurrentSaleman->CurrentGoods)&&(!CheckGuardCode(GuardRes)))
					return;

				if (Cheque->GetCount()>0)//clear current goods info
				{
					IsCurrentGoodsEmpty=true;
					ShowGoodsInfo();
				}
				else
				{
					IsChequeInProgress=false;
					InitWorkPlace();
				}

				CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CURRENTGOODS,
					CurrentGoods->ItemSum,CashReg->CurrentSaleman->Id,GuardRes);
			}

			return;
		}

		if (CompareKey(Key,AMOUNT))//change the goods quantity
		{
		    ;;Log((char*)("PressKey>> AMOUNT ("+U2W(EditLine->text())+")").c_str());
			//if(bPriceCheck)
			//	ShowMessage("search by enter");
			if (!EditLineCheck(""))
				return;

			if (!IsCurrentGoodsEmpty)
			{//change the goods amount by taking into account saleman's limits


                // + dms === 2020-10-13 ==== На время акции Виски+Сок =====
                if (CurrentGoods->ItemCode==24979)
                {
                    return;
                }
                // - dms


                if (CurrentGoods->ItemAlco)
				{
					//ShowMessage("Нельзя изменять количество алкогольной продукции");
					return;
				}

                if (CurrentGoods->ItemCigar)
				{
					//ShowMessage("Нельзя изменять количество алкогольной продукции");
					return;
				}

			    string str = CurrentGoods->Lock;

                if (CashReg->GoodsLock(str, GL_AmountLockAnyway) == GL_AmountLockAnyway)
				{
					//ShowMessage("Нельзя изменять количество товара");
					return;
				}

                if (CashReg->GoodsLock(str, GL_MilkEGAIS) == GL_MilkEGAIS)
                {
                    //ShowMessage("Нельзя изменять количество товара");
                    return;
                }

                if (CashReg->GoodsLock(str, GL_TextileEGAIS) == GL_TextileEGAIS)
                {
                    //ShowMessage("Нельзя изменять количество товара");
                    return;
                }

                int Res=NotCorrect;
                if ((CashReg->GoodsLock(str, GL_AmountLock) == GL_AmountLock)
                   &&(!CheckGuardCode(Res)))
				{
					//ShowMessage("Нельзя изменять количество товара");
					return;
				}


				bool err;
				double GoodsCount=GetDoubleNum(&err);
				if ((!err)||(GoodsCount<epsilon))
				{
					ShowMessage("Неправильное количество товара");
					return;
				}

				if (fabs(CurrentGoods->ItemPrecision-1)<epsilon)
					if (GoodsCount!=floor(GoodsCount))
					{
						ShowMessage("Неправильное количество штучного товара");
						return;
					}

				if ((MaxAmount>epsilon)&&(GoodsCount>=MaxAmount))
				{
					ShowMessage("Слишком большое количество товара!");
					return;
				}


				if ((MaxSum>epsilon)&&(GoodsCount*CurrentGoods->ItemPrice>=MaxSum))
				{
					ShowMessage("Слишком большая сумма товара!");
					return;
				}

				if ((RoundToNearest(GoodsCount*CurrentGoods->ItemPrice,0.01)<epsilon) && (!CurrentGoods->ActionGoods))
				{
					ShowMessage("Слишком мала сумма товара!");
					return;
				}

				int	GuardRes=NotCorrect;
				if ((GoodsCount<CurrentGoods->ItemCount)&&(fabs(CurrentGoods->ItemPrecision-1)<epsilon)//закоммент-запрет уменьшения весового товара ()
							&&(!CashReg->CurrentSaleman->Quantity)&&(!CheckGuardCode(GuardRes)))
					return;

				if ((GoodsCount<CurrentGoods->ItemCount)&&(fabs(CurrentGoods->ItemPrecision-1)>epsilon)	// весовой товар
							&&(!CashReg->CurrentSaleman->Weight)&&(!CheckGuardCode(GuardRes)))			//
					return;


            string resCheckSugar = "";
			if(CheckSugar(&resCheckSugar, CurrentGoods->ItemCode, GoodsCount-CurrentGoods->ItemCount))
			{
			    ShowMessage(resCheckSugar);
			    return;
            }
//

				if (GoodsCount<CurrentGoods->ItemCount)
					CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,QUANTITY,
						CurrentGoods->ItemSum,CashReg->CurrentSaleman->Id,GuardRes);

				CurrentGoods->ItemCount = RoundToNearest(GoodsCount,CurrentGoods->ItemPrecision);
				CalcCountPrice();
				CurrentGoods->CalcSum();

                bool divide;//флаг делимости
				double quant = -1.0;
				for (int i = 0; ;i++)//проверка полей Q
				{
					char fname[64];
					sprintf(fname, "Q%d", i);
					if(CashReg->GoodsTable->GetFieldNo(fname) == -1 || CashReg->GoodsTable->GetDoubleField(fname) < .001)
						break;
					quant = CashReg->GoodsTable->GetDoubleField(fname);
				}

				if (CashReg->GoodsTable->GetFieldNo("MULT") != -1
						&& quant != -1.0 //максимальное из полей Q
						//&& GoodsCount > 0
						&& fmod(GoodsCount,quant) != 0.0 //остаток от деления
						&& quant > CashReg->GoodsTable->GetDoubleField("MULT")//количество от которого начинается скидка
						&& quant < GoodsCount)
				{
					double tmpGoodsCount = GoodsCount;
					GoodsCount = tmpGoodsCount - fmod(GoodsCount,quant);
					CurrentGoods->ItemCount = RoundToNearest(GoodsCount,CurrentGoods->ItemPrecision);
					CalcCountPrice();
					CurrentGoods->CalcSum();
					QListViewItem *item = ChequeList->firstChild();
					while(item->itemBelow())
						item = item->itemBelow();
					item->setText(ListNo,Int2Str(Cheque->GetSize()).c_str());
		            // + dms === 2010-03-30 =====\ спец. код не выводим
                    item->setText(ListCode, CurrentGoods->ItemCode==FreeSumCode? "" : Int2Str(CurrentGoods->ItemCode).c_str());
                    // - dms === /
					item->setText(ListName,W2U(CurrentGoods->ItemName));
					item->setText(ListPrice,GetRounded(CurrentGoods->ItemPrice,0.01).c_str());
					item->setText(ListCount,GetRounded(CurrentGoods->ItemCount,CurrentGoods->ItemPrecision).c_str());
					item->setText(ListSum,GetRounded(CurrentGoods->ItemSum,0.01).c_str());
					item->setText(ListDscnt,GetRounded(CurrentGoods->ItemCalcSum-CurrentGoods->ItemSum,0.01).c_str());
					GoodsCount = fmod(tmpGoodsCount,quant);
					AddGoods();
					divide = true;
				}
				else
				{
					divide = false;
				}

				CurrentGoods->ItemCount = RoundToNearest(GoodsCount,CurrentGoods->ItemPrecision);
				CalcCountPrice();
				CurrentGoods->CalcSum();

				ProcessDiscount(Cheque);

				QListViewItem *item = ChequeList->firstChild();
				while(item->itemBelow())
					item = item->itemBelow();
				item->setText(ListNo,Int2Str(Cheque->GetSize()).c_str());
                // + dms === 2010-03-30 =====\ спец. код не выводим
                item->setText(ListCode, CurrentGoods->ItemCode==FreeSumCode? "" : Int2Str(CurrentGoods->ItemCode).c_str());
                // - dms === /
				item->setText(ListName,W2U(CurrentGoods->ItemName));
				item->setText(ListPrice,GetRounded(CurrentGoods->ItemPrice,0.01).c_str());
				item->setText(ListCount,GetRounded(CurrentGoods->ItemCount,CurrentGoods->ItemPrecision).c_str());
				item->setText(ListSum,GetRounded(CurrentGoods->ItemSum,0.01).c_str());
				item->setText(ListDscnt,GetRounded(CurrentGoods->ItemCalcSum-CurrentGoods->ItemSum,0.01).c_str());

				CashReg->m_vem.EditGoods(-1, CurrentGoods, Cheque->GetCalcSum());

				CashReg->LogEditGoods(CurrentGoods);

				ShowGoodsInfo();
				EditLine->clear();
			}
			return;
		}


        if (CompareKey(Key,ALTFINALIZE) && EditLine->ctrl)//alternative cashless complete current sale
        {
            Log("PressKey>> ALTFINALIZE");
            AltFinal = (PaymentKeyPress==0);
        }

		if (CompareKey(Key,FINALIZE) || AltFinal)//complete current sale
		{
            NeedChangeCard2VIP=false;
            NeedChangeCard2SuperVIP=false;

		    if (!AltFinal)
                Log((char*)("PressKey>> FINALIZE ("+U2W(EditLine->text())+")").c_str());

			if ((IsCurrentGoodsEmpty)&&(Cheque->GetSize()==0))//bool IsCurrentGoodsEmpty;//various flags &&
			{
				ShowMessage("Отсутствует товар в чеке!");
				return;
			}
			//{
			if (Cheque->GetSum() < 0)
			{
				ShowMessage("Недопустимая сумма в чеке!");
				return;
			}
			//}

            // Акция выдачи карты "Добрые соседи" при предъявлении флаера
//            if (InActionDobryeSosedy(CurrentCustomer, AddAddonDiscount))
//            {
//                 ShowMessageAndLogInfo("Необходимо считать карту \"Добрые соседи\" и флаер!");
//                 return;
//            }
            //==============


			if(GoodsLock(Cheque))
			{
			    return;
            }

			if(!GoodsWeightChecked && CheckWeightGoods(Cheque))
			{
			    return;
            }

			if(Cheque->Lock)
			{
			    ShowMessage("Продажа товаров запрещена!");
			    return;
            }


            string resCheckSugar = "";
			if(CheckSugar(&resCheckSugar))
			{
                ShowMessage(resCheckSugar);
			    return;
            }



/*			if(Check100_11())
			{
			    ShowMessage("Продажа данных товаров на этой кассе запрещена!");
			    return;
            }
*/

/* проверка на активность сертификатов
			if()
			{
			    ShowMessage("");
			    return;
            }
//*/

/*/
// АКЦИЯ ЗАВЕРШЕНА
// Акция Либеро: за каждые 2 товара Либеро выдается подарок
            int CntPresent = CheckLibero();
			if(CntPresent)
			{

                string msg = "Выдать мягкую игрушку на руку ("+Int2Str(CntPresent)+" шт.)";
                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CARDINFO,0,CashReg->CurrentSaleman->Id,-1,NULL,NULL,NULL, (char*)msg.c_str());

                int lKeyPressed = -1;
                while (lKeyPressed !=0)
                {
                    lKeyPressed = QMessageBox::information
                    ( NULL, "APM KACCA", W2U(msg)
                    ,W2U("ВЫДАТЬ ПОДАРОК"),W2U("Х") ,""
                    , 1, -1);
                }

            }
//*/

/*/
// АКЦИЯ ЗАВЕРШЕНА
// Акция Пиво Redhop: за каждые 3 товара пива Redhop выдается подарок - упаковка колбасок "Пиколини"
            CntPresent = 0;
            CntPresent = CheckRedhop();
			if(CntPresent)
			{
                string msg = "За покупку пива ТМ Redhop вручаем упаковку колбасок Пиколини ("+Int2Str(CntPresent)+" шт.)";
                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CARDINFO,0,CashReg->CurrentSaleman->Id,-1,NULL,NULL,NULL, (char*)msg.c_str());

                int lKeyPressed = -1;
                while (lKeyPressed !=0)
                {
                    lKeyPressed = QMessageBox::information
                    ( NULL, "APM KACCA", W2U(msg)
                    ,W2U("ВЫДАТЬ ПОДАРОК"),W2U("Х") ,""
                    , 1, -1);
                }

            }
//*/

/*/
// АКЦИЯ ЗАВЕРШЕНА
// Акция Кимберли: при покупке товаров Кимберли на 750 руб. выдается подарок
            int CntPresentKimberly = CheckKimberly();
			if(CntPresentKimberly)
			{

                string msg = "Выдать мягкую игрушку Кимберли ("+Int2Str(CntPresentKimberly)+" шт.)";
                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CARDINFO,0,CashReg->CurrentSaleman->Id,-1,NULL,NULL,NULL, (char*)msg.c_str());

                int lKeyPressed = -1;
                while (lKeyPressed !=0)
                {
                    lKeyPressed = QMessageBox::information
                    ( NULL, "APM KACCA", W2U(msg)
                    ,W2U("ВЫДАТЬ ПОДАРОК"),W2U("Х") ,""
                    , 1, -1);
                }

            }

//*/
/*
// АКЦИЯ ЗАВЕРШЕНА
//
            int CntPresentBeerCup = CheckBeerCup();
			if(CntPresentBeerCup)
			{

                string msg = "Выдать БОКАЛ в ПОДАРОК";
                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CARDINFO,0,CashReg->CurrentSaleman->Id,-1,NULL,NULL,NULL, (char*)msg.c_str());

                int lKeyPressed = -1;
                while (lKeyPressed !=0)
                {
                    lKeyPressed = QMessageBox::information
                    ( NULL, "APM KACCA", W2U(msg)
                    ,W2U("ВЫДАТЬ ПОДАРОК"),W2U("Х") ,""
                    , 1, -1);
                }

            }
//*/

/*
// АКЦИЯ ЗАВЕРШЕНА
           int CntPresentMonster = CheckMonster();
		   if(CntPresentMonster)
			{

                string msg = "Выдать скретч-карту в ПОДАРОК!";
                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CARDINFO,0,CashReg->CurrentSaleman->Id,-1,NULL,NULL,NULL, (char*)msg.c_str());

                int lKeyPressed = -1;
                while (lKeyPressed !=0)
                {
                    lKeyPressed = QMessageBox::information
                    ( NULL, "APM KACCA", W2U(msg)
                    ,W2U("ВЫДАТЬ ПОДАРОК"),W2U("Х") ,""
                    , 1, -1);
                }

            }
//*/



// АКЦИЯ
// Акция Frigovere: при покупке определенных товаров на 1500 руб. выдается подарок - Контейнер
            int CntPresentFrigovere = CheckFrigovere();
			if(CntPresentFrigovere)
			{

                string msg = "Выдать ПОДАРОК\n\nКонтейнер FRIGOVERE FUN Круглый 800мл. ("+Int2Str(CntPresentFrigovere)+" шт.)";
                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CARDINFO,0,CashReg->CurrentSaleman->Id,-1,NULL,NULL,NULL, (char*)msg.c_str());

                int lKeyPressed = -1;
                while (lKeyPressed !=0)
                {
                    lKeyPressed = QMessageBox::information
                    ( NULL, "APM KACCA", W2U(msg)
                    ,W2U("ВЫДАТЬ ПОДАРОК"),W2U("Х") ,""
                    , 1, -1);
                }

            }
//

// АКЦИЯ ЗАВЕРШЕНА
/*
// АКЦИЯ
// Акция : при покупке сыра -  для колки сыра в подарок
            int CntPresentCheese = CheckCheeseAction();
			if(CntPresentCheese)
			{

                string msg = "Выдать ПОДАРОК\n\nНож для колки сыра";
                CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CARDINFO,0,CashReg->CurrentSaleman->Id,-1,NULL,NULL,NULL, (char*)msg.c_str());

                int lKeyPressed = -1;
                while (lKeyPressed !=0)
                {
                    lKeyPressed = QMessageBox::information
                    ( NULL, "APM KACCA", W2U(msg)
                    ,W2U("ВЫДАТЬ ПОДАРОК"),W2U("Х") ,""
                    , 1, -1);
                }

            }
//*/


			FiscalReg->GetSmenaNumber(&(CashReg->KKMNo),&(CashReg->KKMSmena),&(CashReg->KKMTime));

			Cheque->Reduction(CashReg->KKMNo,CashReg->KKMSmena,CashReg->KKMTime,false,false);

    		if ((!IsCurrentGoodsEmpty)&&(PaymentKeyPress==0))
			{
				
                Log("= ПОДГОТОВКА ЧЕКА К ОТБИТИЮ (СЖАТИЕ, ПЕРЕСЧЕТ СКИДОК) =");
				IsCurrentGoodsEmpty = true;
				CurGoods.init();
                CurrentGoods = &CurGoods;
                ActionShowMsg=true;
                //Пересчет скидок (процедура ProcessDiscount) есть в ShowGoodsInfo
				//ProcessDiscount(Cheque);

                //Процедура ShowGoodsInfo есть в UpdateList
				//ShowGoodsInfo();

                
				UpdateList();
				return;
				/*
				if (AppendRecord2Check())//add last goods to cheque
    				ProcessDiscount(Cheque);
				return;
				*/
			}
			UpdateList();

            Log("= ОТБИТИЕ ЧЕКА = ");


            // Статус EGAIS
            // -1 - не было проверки
            // 0 - проверка пройдена успешно, получен слип
            // 1 - проверка прошла с ошибкой, либо не получен ответ, печать чека невозможна
            int StatusEGAIS = -1;

			//if (PaymentKeyPress==0)
			{
				//if (EditLine->text().isEmpty())
				{//без сдачи
					int res;
					bool oldscanersuspend = Scanner->suspend;
					Scanner->suspend = true;
					if (CashReg->Cashless)//CashReg->CurrentSaleman->Cashless)
					{
					
						if(PaymentType == ShiftDown || EditLine->shift || AltFinal)
						{
							PaymentTypeForm *dlg = new PaymentTypeForm(this);
							res=dlg->exec();

							PaymentType=dlg->PaymentType;
							delete dlg;
						}
						else
						{
							PaymentType = CashPayment;
							res=QDialog::Accepted;
						}
					}
					else
					{					
						res=QDialog::Accepted;
						PaymentType=CashPayment;
					}

					if (res==QDialog::Accepted)//ask for the type of payment
					{
						if (PaymentType==CertificatePayment)
						{
							if (!CashReg->CurrentSaleman->Discounts)
							{
								ShowMessage("Отсутствуют права на предоставление скидки!");
								return;
							}

							Log("Payment type is Certificate");
							PaymentForm *dlg = new PaymentForm(this);
							res=dlg->exec();

							delete dlg;
							Log("Расчитываем скидки");
							ProcessDiscount(Cheque);
							ShowGoodsInfo();

                            Scanner->suspend = false;

							return;

						}

//						if (PaymentType==CashPayment)
//						{
//							FiscalReg->OpenDrawer();//close check
//						}

						if (PaymentType==BankBonusPayment)
						{

						    ProcessBonusDiscount(CurrentCustomer);

                            Scanner->suspend = false;
//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========
/*
//                            if (OnlyCashlessMode)
//                            {
//                            //если чек восстановлен с бонусами, то повторно списывать не даем
//                               EditLine->ctrl=true;
//                               ProcessKey(4100);
//                               return
//                            }

                            //! +тест


//                        // + Повторное снятие бонусов
//                        SumForm *dlg = new SumForm(this);//show simple dialog and get check's number
//                        dlg->FormCaption->setText(W2U("Номер чека"));
//
//                        int res=dlg->exec();
//
//                        QString s = dlg->LineEdit->text();
//                        string LogStr = " = ПОВТОРНОЕ СПИСАНИЕ БОНУСОВ СБЕРБАНКА ЧЕК # "+U2W(s)+ " =";
//
//                        Log((char*)LogStr.c_str());
//
//                        delete dlg;
//
//                        int depart;
//
//                        int retcheck = s.toInt();
//                        // - Повторное снятие бонусов
//                        if (retcheck)
//                        {
//                            string StrCard="000"; // "<номер карты>";
//                            string CardSHA1 = "E6188F13ED6A057C1692D75C1403D85C86CE7947"; //"<хеш карты>";
//                            ProcessBankBonusDiscount(StrCard, CardSHA1);
//                        }
//                        else
//                        {
                            //! + Оригинал
                            CashReg->pCR->BankBonusDiscount();
                            //! - Оригинал
//                        }
                              //  ;;double val=155;
                              //;;CashReg->AddPayment(11,"TEST",&val);
                            //!- тест

                             ShowGoodsInfo();
*/
//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========

                           return;
						}

                        //Запомним сумму чека до списания бонусов
                        double OldSum = Cheque->GetSumToPay();

                        // БОНУСЫ. Инициализация и списание (опционально)
                        ProcessBonusDiscount(CurrentCustomer);
                        //
                        bool PaymentType_CashlessPayment = (PaymentType == CashlessPayment);

#ifndef _WS_WIN_
						if(PaymentType == CashlessPayment)
						{

                            FiscalCheque::PayBank = true;
//;;Log("+ ShowGoodsInfo #1 ");
                            ShowGoodsInfo();
//;;Log("- ShowGoodsInfo #1 ");
                            FiscalCheque::PayBank = false;

							//CardTypeDialog
							QStringList l, al;
							l += W2U("Без оплаты");
							al += W2U("Без оплаты");
							//int t[] = {0,1,2,3,9,10,11,-1};
							int t[] = {CL_NOPAY,CL_VISA,CL_MASTERCARD,CL_MAESTRO,CL_SBERCARD,CL_VISA_GZ,CL_MASTERCARD_GZ,CL_CITYCARD};
							int at[] = {CL_NOPAY,CL_VISA,CL_MASTERCARD,CL_MAESTRO,CL_SBERCARD,CL_VISA_GZ,CL_MASTERCARD_GZ,CL_CITYCARD};
							int set = 1;
							if(CashReg->CardReaderType & VFCR || CashReg->CardReaderType & VFUP){//
								al += "Visa";
								al += "Eurocard/Mastercard";
								al += "Cirrus/Maestro";
								al += W2U("СБЕРКАРТ");

								//set = 2;

                                //*
								l += W2U("Сбербанк");
								//l += W2U(" -> СБЕРКАРТ");

                                //t[l.count()-1] = CL_SBERCARD;
								t[l.count()-1] = CL_SBERBANK;

                                //*/
                                /*
                                //! полный список карт
								l += "Visa";
								l += "Eurocard/Mastercard";
								l += "Cirrus/Maestro";
								l += W2U("СБЕРКАРТ");
                                //*/
							}
							if(CashReg->CardReaderType & EGCR){//
								al += W2U("Visa (Газпромбанк)");
								al += W2U("Mastercard / Maestro (Газпромбанк)");
								//set += 1;
								at[al.count()-1] = CL_MASTERCARD_GZ;
								at[al.count()-2] = CL_VISA_GZ;

								l += W2U("Газпромбанк");
								t[l.count()-1] = CL_GAZPROMBANK;
							}
							if(CashReg->CardReaderType & CCCR){
                                al += "CityCard/IzhCard";
                                at[al.count()-1] = CL_CITYCARD;

								l += "CityCard/IzhCard";
								t[l.count()-1] = CL_CITYCARD;
								//if(!(CashReg->CardReaderType & VFCR || CashReg->CardReaderType & VFUP || CashReg->CardReaderType & EGCR))
									//t[1] = -1;
							}
							if(CashReg->CardReaderType & VTBCR){
								al += W2U("Visa (ВТБ24)");
								al += W2U("Mastercard (ВТБ24)");
								al += W2U("Maestro (ВТБ24)");
								//set += 1;
								at[al.count()-1] = CL_MAESTRO_VTB;
								at[al.count()-2] = CL_MASTERCARD_VTB;
								at[al.count()-3] = CL_VISA_VTB;

								l += W2U("ВТБ24");
								t[l.count()-1] = CL_VTB24;
								//if(!(CashReg->CardReaderType & VFCR || CashReg->CardReaderType & VFUP || CashReg->CardReaderType & EGCR))
									//t[1] = -1;

                                // По умолчанию ВТБ
//								set = l.count()-1;

                                // По умолчанию Сбербанк
								set = 1;
							}

							//t[l.count()] = -1;

							bool ok;//
							QString s = QInputDialog::getItem("APM KACCA",
								W2U("Выберите тип карты"),l,set,false,&ok);
							if (!ok) //
                            {

                                ShowGoodsInfo();
                                Scanner->suspend = false;

								return;//
                            }
							int i = l.findIndex(s);
							if(i == -1){

								ShowGoodsInfo();
								Scanner->suspend = false;

								return;
							}

                            Log("Выбор банковской карты: "+U2W(s)+" /"+Int2Str(t[i])+"/ ");



							if(!i){
							    // Без оплаты
								if(!ShowQuestion("Вы уверенны, что оплата уже произведена?", false)){// , то "Без оплаты"
									ShowGoodsInfo();
									Scanner->suspend = false;
									return;
								}

								Log(">> Продажа БЕЗ оплаты....");

								int GuardRes = NotCorrect;//{
								Scanner->suspend = false;
								while(!CheckGuardCode(GuardRes));
								Scanner->suspend = true;//}
								l[0] = W2U("Отмена");
								al[0] = W2U("Отмена");
								s = QInputDialog::getItem("APM KACCA",
									W2U("Выберите тип карты, которым произведена оплата."),al,set,false,&ok);
								if (!ok) //
								{
								    ShowGoodsInfo();
								    Scanner->suspend = false;
									return;//
								}
								int ii = al.findIndex(s);
								if(!ii)
								{
								    ShowGoodsInfo();
								    Scanner->suspend = false;
									return;
								}
								PaymentType = at[ii];
								// Чек без оплаты (в лог)
								CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CL_AGAIN,Cheque->GetSum(),CashReg->CurrentSaleman->Id,-1);

                                // Пересчет скидок без округления копеек
								FiscalCheque::PayBank = true;
								ShowGoodsInfo();
								FiscalCheque::PayBank = false;
							} // Без оплаты
							else
							{   // + dms ==== 2009-10-21 ==== тест связи с сервером банка
							    if ((CashReg->TryBankConnect)
							    &&(t[i] > 0)
							    &&(t[i]!=CL_GAZPROMBANK)) // если нужно проверять и это не CityCard
							    {
							        Log("Test bank connection ...");
                                    CashReg->TestBankConnection(0, 1);
                                    if (!CashReg->StatusBankConnection)
                                    {
                                        ShowMessage("Нет связи с банком: Оплата безналичным расчетом невозможна!");

                                        // Отмена бонусов
                                        ProcessBonusDiscountReverse();

                                        ShowGoodsInfo();

                                        Scanner->suspend = false;
                                        return;
                                    }
                                }
                                // - dms
                                Log(">> Cashless Pay....");
                                PaymentType = t[i];

                                if (StatusEGAIS==-1){
                                    Log("= ПРОВЕРКА ЕГАИС (Безнал) = ");
                                    StatusEGAIS = CashReg->CheckEGAIS(Cheque);
                                    if (StatusEGAIS)
                                    {
                                        ShowMessage("Ошибка проверки чека в ЕГАИС!");

                                        // Отмена бонусов
                                        ProcessBonusDiscountReverse();
                                        // пересчет скидок
                                        ShowGoodsInfo();

                                        Scanner->suspend = false;
                                        return;
                                    }
                                }

                                if(CashReg->pCR->Pay(Double2Int(Cheque->GetSumToPay() * 100), PaymentType, CashReg->GetLastCheckNum()+1))
                                {
                                    // ошибка безналичного расчета

                                    Log("   ....ERROR");


                                    // Отмена последней операции проверки ЕГАИС
                                    CashReg->CheckEGAIS(Cheque, true, true);
                                    StatusEGAIS=-1; // обнуляем флаг проверки, т.к. нужна повторная проверка ЕГАИС
                                    //

                                    // Отмена бонусов
                                    ProcessBonusDiscountReverse();
                                    //
                                    ShowGoodsInfo(); // пересчет скидок

                                    Scanner->suspend = false;
                                    return;
                                }

                                Log("   ....SUCCESS");
							}
						}//CashLess

#endif

                        // + EGAIS
                        if (StatusEGAIS==-1){
                            Log("ПРОВЕРКА ЕГАИС (наличные)");
                            StatusEGAIS = CashReg->CheckEGAIS(Cheque);
                            if (StatusEGAIS)
                            {
                                ShowMessage("Ошибка проверки чека в ЕГАИС!");

                                // Отмена бонусов
                                ProcessBonusDiscountReverse();
                                // пересчет скидок
                                ShowGoodsInfo();

                                Scanner->suspend = false;
                                return;
                            }
                        }
                        // - EGAIS
                        FullSum=0;
                        double RestSum=-1;

                        if(!PaymentType_CashlessPayment)
                        { // Оплата наличными
                          // Бонусы уже списаны, чек пересчитан, нужно просто показать новую сумму и окно ввода наличных

                            Log("= ОПЛАТА НАЛИЧНЫМИ = ");
                            {
                                // Если сумма чека изменилась (списаны бонусы"
                                // то нужно заново ввести сумму наличных

                                Log("Сумма чека ("+Double2Str(RoundToNearest(Cheque->GetSum(),0.01))+")");
                                if (FiscalCheque::PrePayCertificateSumm > 0)
                                {
                                    Log("Сумма чека к оплате("+Double2Str(RoundToNearest(Cheque->GetSumToPay(),0.01))+")");
                                }
                                Log("Введена сумма к оплате ("+U2W(EditLine->text())+")");

                                bool NeedEnterPaySum=false;

                                if (EditLine->text()!="")
                                {
                                    bool err;
                                    FullSum = EditLine->text().toDouble(&err);

                                    if ((!err) || (FullSum>10000000))  //calculate change
                                    {
                                        ShowMessage("Неправильно указана полученная сумма!");
                                        EditLine->setText("");
                                        NeedEnterPaySum=true;

                                        FullSum=0; RestSum=-1;
                                    }
                                    else
                                        if (FullSum>0)
                                        {
                                            // введена сумма наличных
                                            RestSum=RoundToNearest(FullSum-Cheque->GetSumToPay(),0.01);

                                            if (RestSum<-0.0015)
                                            {
                                                ShowMessage("Полученная сумма слишком мала!");
                                                NeedEnterPaySum=true;

                                                FullSum=0; RestSum=-1;
                                            }

                                         }
                                }

                                if (OldSum - Cheque->GetSumToPay() > 1){
                                    NeedEnterPaySum=true;
                                }

                                if (NeedEnterPaySum)
                                {

                                    Log("Нужно ввести сумму наличных повторно");

                                    SumForm *dlg = new SumForm(this);
                                    dlg->FormCaption->setText(W2U("Введите сумму наличных"));

                                    dlg->LineEdit->setText(EditLine->text());

                                    for(;;)
                                    {
                                        ;;Log(" -> Диалог ввода суммы <- ");
                                        if (dlg->exec() == QDialog::Accepted)
                                        {

                                            Log(" -> Введена сумма ("+U2W(EditLine->text())+")");

                                            if (dlg->LineEdit->text()=="")
                                            {
                                                Log("Введен ноль к оплате");
                                                FullSum=0;  RestSum=-1;

                                                break;
                                            }

                                            bool err;
                                            FullSum = dlg->LineEdit->text().toDouble(&err);

                                            if ((!err) || (FullSum>10000000))  //calculate change
                                            {
                                                ShowMessage("Неправильно указана полученная сумма!");
                                                continue;
                                            }

                                            if (FullSum>0)
                                            {
                                                // введена сумма наличных
                                                RestSum=RoundToNearest(FullSum-Cheque->GetSumToPay(),0.01);

                                                if (RestSum<-0.0015)
                                                {
                                                    ShowMessage("Полученная сумма слишком мала!");
                                                    continue;
                                                }

                                             }
                                        }
                                        else
                                        {
                                            Log("Нажата кнопка отказа от ввода полученной суммы наличных");
                                            FullSum=0;
                                            RestSum=-1;
                                        }

                                        ;;Log(" -> Выход <- ");
                                        break;
                                    }

                                    delete dlg;
                                }


                                if ((FullSum>0) && (RestSum>=0))
                                {
                                    ;;Log("= СДАЧА = ");

;;Log("FullSum="+Double2Str(FullSum));
;;Log("RestSum="+Double2Str(RestSum));

                                   // отобразим сдачу на экране
                                    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,PAY,FullSum,CashReg->CurrentSaleman->Id,-1);

                                    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,PAYBACK,RestSum,CashReg->CurrentSaleman->Id,-1);

                                    MTotalChange->setText(GetRounded(RestSum,0.01).c_str());

                                    lbTotalChange->show();
                                    MTotalChange->show();

                                    lbTotalSum->hide();
                                    MTotalSum->hide();

                                    MTotalChange->repaint();
                                    MTotalSum->repaint();

                                    ShowMessage("");
                                    //thisApp->processEvents();

                                    CustomerDisplay->DisplayLines("",("Сдача "+GetRounded(RestSum,0.01)).c_str());
                                    PaymentKeyPress=1;
                                    PaymentType=CashPayment;
                                    EditLine->clear();


                                }


                            }
                        } // наличными (со сдачей или без)

                        /* slair+ */

#ifndef PRINTPAPPER
                        dont_print_paper_check = true;
#else
                        dont_print_paper_check = false;
#endif

                        char no_print;
                        if (dont_print_paper_check) {
                            ;;Log("!= без печати бумажного чека");
                            no_print=1;
                            FiscalReg->SetTable(17,1,7,&(no_print=1),1,false);
                        } else {
                            ;;Log("!= с печатью бумажного чека");
                            no_print=0;
                            FiscalReg->SetTable(17,1,7,&(no_print=0),1,false);
                        }
                        /* slair- */

                        Log("= ЗАКРЫТИЕ ЧЕКА = ");

                        if (PaymentType==CashPayment)
                            FiscalReg->OpenDrawer();//close check with the change

;;Log("=> Начало отбития чека #"+Int2Str(CashReg->GetLastCheckNum()+1));


        CashReg->DeletePrepayExcess(Cheque);
        CashReg->AddPrepayExcess(Cheque);


						CashReg->m_vem.Calc(Cheque->GetCalcSum(),Cheque->GetDiscountSum(), PaymentType ? 2 : 1);

						CashReg->AddSalesToReg(Cheque);
						CashReg->AddPaymentToReg();
						//Saleman *CurSaleman;


						CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,STARTCHECK,Cheque->GetSum(),CashReg->GetCashmanCode(),0);//


                        ;;Log(" ===> Начало GetCheckHeader");
                        string strCheckHeader = CashReg->GetCheckHeader();
                        ;;Log(" ===> Конец GetCheckHeader");

                        ;;Log(" ===> Начало GetCheckEgaisHeader");
                        string strCheckEGAIS = CashReg->GetCheckEgaisHeader();
                        ;;Log(" ===> Конец GetCheckEgaisHeader");

                        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,STARTPRINTCHECK,Cheque->GetSum(),CashReg->CurrentSaleman->Id,0);//
                        ;;Log(" ===> Начало печати чека");
						int er = FiscalReg->Close(
							 Cheque->GetSum()
							,(FullSum>epsilon) ? FullSum : Cheque->GetSumToPay()
							,Cheque->GetDiscountSum()
							,!frSound ? CashReg->GetCashRegisterNum() : -1*CashReg->GetCashRegisterNum()
							,CashReg->GetLastCheckNum()+1
							,PaymentType
							,Cheque->GetCustomerCode()
							,CashReg->CurrentSaleman->Name
							,strCheckHeader
							,CashReg->GetCheckFutter(Cheque->GetSum())
							,IsHandDiscounts()
							,strCheckEGAIS
                            ,Cheque->egais_url
                            ,Cheque->egais_sign
                            ,CashReg->EgaisSettings.ModePrintQRCode
							);
                        ;;Log(" ===> Конец печати чека");
                        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,FINISHCHECK,Cheque->GetSum(),CashReg->CurrentSaleman->Id,0,"",(char*)Int2Str(er).c_str(),"",(char*)FiscalReg->GetErrorStr(er,0).c_str());//

                        if(er==0x6B) // Нет чековой ленты
                        {
                            Log("ERROR: НЕТ ЧЕКОВОЙ ЛЕНТЫ!");
                           // Отмена последней операции проверки ЕГАИС
                            CashReg->CheckEGAIS(Cheque, true, true);
                            StatusEGAIS=-1; // обнуляем флаг проверки, т.к. нужна повторная проверка ЕГАИС
                            //
                        }

                        ;;Log(" ===> Начало фиксации бонусов");
                        // Фиксация покупки в ПЦ бонусов
//                        clock_t start, end;
//                        start = clock();
//                        time_t start, end;
//                        time(&start);
                        struct timeval start, end;
                        gettimeofday(&start, NULL);
                        ProcessBonusFix(CurrentCustomer);
//                        end = clock();
//                        double duration_sec = double(end-start)/CLOCKS_PER_SEC;
//                        time(&end);
//                        double duration_sec = difftime(end, start);
                        gettimeofday(&end, NULL);
                        double duration_sec = ((end.tv_sec * 1000000 + end.tv_usec) -
                                               (start.tv_sec * 1000000 + start.tv_usec))/1000000.0;
                        Log("ProcessBonusFixDuration: " + Double2Str(duration_sec) + " sec.");
                        ;;Log(" ===> Конец фиксации бонусов");

                        ;;Log("=> Конец отбития чека #"+Int2Str(CashReg->GetLastCheckNum()+1));

						CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,SELL,Cheque->GetSum(),CashReg->CurrentSaleman->Id,-1);

						CashReg->m_vem.Sell(Cheque->GetCalcSum(),Cheque->GetDiscountSum(), PaymentType ? 2 : 1);
						CashReg->m_vem.Pay(Cheque->GetCalcSum(),Cheque->GetDiscountSum(), PaymentType ? 2 : 1, Cheque->GetSum(),0);

						Scanner->suspend = false;

						if (er!=0)
						{
							FullSum=0;
							PaymentKeyPress=1;
						}

						CashReg->ProcessRegError(er);

						SetPaymentType(Cheque,PaymentType);

						CashReg->FixCheque(Cheque);
						CashReg->SavePaymentFromReg();

						IsChequeInProgress=false;

//! Замена карт
                        CashReg->ChangeBonusCard(CurrentCustomer);
//! Замена карт

//! Замена карт на VIP
                        if(NeedChangeCard2SuperVIP)
                        {
                            CashReg->ChangeBonusCard2Vip(CurrentCustomer, CD_SUPER_VIP);
                        }
                        else
                        {
                            if(NeedChangeCard2VIP)
                            {
                                CashReg->ChangeBonusCard2Vip(CurrentCustomer, CD_VIP);
                            }
                        }

//! Замена карт на VIP

						//!!!InitWorkPlace();
						RestartTimerSS(true);
                        InfoMessage->setText(W2U("*"));
                        InfoCLPay->setText(W2U("ОДОБРЕНО"));

						PaymentKeyPress=0;
						AutoRestoreKeepedCheque();

						FiscalReg->GetSmenaNumber(&(CashReg->KKMNo),&(CashReg->KKMSmena),&(CashReg->KKMTime));

					} // Accepted

//					Scanner->suspend = false;
//					return;
				}
			
                Scanner->suspend = false;
                return;

/*
				//со сдачей
				bool err;

				FullSum=EditLine->text().toDouble(&err);

				if (!err)   //calculate change
				{
					ShowMessage("Неправильно указана полученная сумма!");
					PaymentKeyPress=0;
					return;
				}

				double RestSum=FullSum-Cheque->GetSum();

				if (RestSum>=0)
				{

                    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,PAY,FullSum,CashReg->CurrentSaleman->Id,-1);

					CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,PAYBACK,RestSum,CashReg->CurrentSaleman->Id,-1);

					MTotalChange->setText(GetRounded(RestSum,0.01).c_str());

					lbTotalSum->hide();
					MTotalSum->hide();

					lbTotalChange->show();
					MTotalChange->show();

					CustomerDisplay->DisplayLines("",("Сдача "+GetRounded(RestSum,0.01)).c_str());
					PaymentKeyPress=1;
					PaymentType=CashPayment;
					EditLine->clear();

					return;
				}
				ShowMessage("Полученная сумма слишком мала!");
				PaymentKeyPress=0;
				return;
*/

			} // PaymentKeyPress==0

			if (false && PaymentKeyPress==1)
			{
// Закрываем эту ветку как ненужную

        ;;Log("POINT_ PaymentKeyPress==1 ");


               // + EGAIS
                if (StatusEGAIS==-1){
                    StatusEGAIS = CashReg->CheckEGAIS(Cheque);
                    if (StatusEGAIS)
                    {
                        ShowMessage("Ошибка проверки чека в ЕГАИС!");
                        return;
                    }
                }

			    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,SELL,Cheque->GetSum(),CashReg->CurrentSaleman->Id,-1);

				if (PaymentType==CashPayment)
					FiscalReg->OpenDrawer();//close check with the change
				CashReg->m_vem.Calc(Cheque->GetCalcSum(),Cheque->GetDiscountSum(), PaymentType ? 2 : 1);
				CashReg->AddSalesToReg(Cheque);
				CashReg->AddPaymentToReg();

				CashReg->ProcessRegError(
					FiscalReg->Close(
                        Cheque->GetSum()
						,(FullSum>epsilon) ? FullSum : Cheque->GetSumToPay()
						,Cheque->GetDiscountSum()
						,!frSound ? CashReg->GetCashRegisterNum() : (-1*CashReg->GetCashRegisterNum())
						,CashReg->GetLastCheckNum()+1
						,PaymentType
						,Cheque->GetCustomerCode()
						,CashReg->CurrentSaleman->Name
						,CashReg->GetCheckHeader()
						,CashReg->GetCheckFutter(Cheque->GetSum())
						,IsHandDiscounts()
						,CashReg->GetCheckEgaisHeader()
						,Cheque->egais_url
                        ,Cheque->egais_sign
                        ,CashReg->EgaisSettings.ModePrintQRCode
					)
				);

                // Начисление бонусов

				ProcessBonusFix(CurrentCustomer);


				CashReg->m_vem.Sell(
					Cheque->GetCalcSum()
					,Cheque->GetDiscountSum()
					,PaymentType ? 2 : 1
				);
				CashReg->m_vem.Pay(
					Cheque->GetCalcSum()
					,Cheque->GetDiscountSum()
					,PaymentType ? 2 : 1
					,FullSum
					,FullSum-Cheque->GetSum()
				);

				SetPaymentType(Cheque,PaymentType);

				CashReg->FixCheque(Cheque);
				CashReg->SavePaymentFromReg();

				IsChequeInProgress=false;
				PaymentKeyPress=0;
				//!!!InitWorkPlace();
				RestartTimerSS(true);
				InfoMessage->setText(W2U("*"));

				AutoRestoreKeepedCheque();

				return;
			}
		}//if (CompareKey(Key,FINALIZE))

		if (CompareKey(Key,ANNUL))//annul opened check
		{
		    Log("PressKey>> ANNUL");
			if (!EditLineCheck(""))
				return;

			if (ShowQuestion("Аннулировать чек?",false))
			{
				int GuardRes=NotCorrect;   //check guard id
				if ((!CashReg->CurrentSaleman->Annul)&&(!CheckGuardCode(GuardRes)))
					return;

				CashReg->ProcessRegError(FiscalReg->Cancel());//annul cheque
				IsChequeInProgress=false;
				CashReg->AnnulCheck(Cheque,GuardRes);
				IsChequeInProgress=false;
				InitWorkPlace();
				AutoRestoreKeepedCheque();

				FiscalReg->GetSmenaNumber(&(CashReg->KKMNo),&(CashReg->KKMSmena),&(CashReg->KKMTime));
			}
			return;
		}

		if (CompareKey(Key,GROUPCODE))//process key attached to some goods group
		{
			CashReg->DbfMutex->lock();
			string code=CashReg->KeyboardTable->GetStringField("NAME");
			CashReg->DbfMutex->unlock();
			EditLine->setText(code.substr(strlen(GROUPCODE),code.length()-strlen(GROUPCODE)).c_str());
			LookingForGoods(GroupFilter);
			EditLine->clear();
		}

		if (CompareKey(Key,OPENDRAWER)){
		    Log("PressKey>> OPENDRAWER");
			//CashReg->LoggingAction(0,CASHBOX,0,CashReg->CurrentSaleman->Id,-1);
			CashReg->m_vem.OpenMoney();
			FiscalReg->OpenDrawer();
		}

		if (CompareKey(Key,MESSAGE))
		{
		    Log("PressKey>> MESSAGE");
            RunMessager();
		}

		if (CompareKey(Key,CHANGEBONUSCARD))
		{
		    Log("PressKey>> CHANGEBONUSCARD");
            CashReg->ChangeBonusCard(-1);
		}


		if (CompareKey(Key,SEARCHBYPHONE))
		{
		    Log("PressKey>> SEARCHBYPHONE");
			int SearchDiscountMode;
			string discount;

            string phone = U2W(EditLine->text());

;;Log("Поиск карты. Введен номер:"+phone);

            if ((phone.length()==10) && (phone.substr(0,1)=="9")) phone = "7"+phone;
            if (phone.substr(0,1)=="8") phone = "7"+phone.substr(1);
            if (phone.length()!=11)
            {
			    ShowMessage("Неправильно веден номер телефона! Введите номер в формате 89ХХХХХХХХХ");
                return;
            }

			long long number=0LL;

			int res = CashReg->GetBonusCardByPhone(phone, &number);


			if (res==1)
			{
			    ShowMessage("Карта не найдена!");
                return;
			}
			else
			{
                if (res==0) CustomerByPhone=true;
			}


//            EditLine->setText(W2U(LLong2Str(number)));
            SearchDiscountMode=FindFromScaner;

            int resDisc = SearchDiscount(SearchDiscountMode,&discount,&number);
            ;;Log("Поиск карты. Код ответа:"+Int2Str(resDisc));
			if (resDisc==1)
			{
				CurrentCustomer=0;
				CashReg->DbfMutex->lock();
				DiscountType = (CashReg->CustomersTable->Locate("ID",0)) ? CashReg->CustomersTable->GetField("DISCOUNTS") : DefaultDiscountType;
				CashReg->DbfMutex->unlock();
			}

            EditLine->setText(W2U(""));

			ShowGoodsInfo();
			return;

		}

		if (CompareKey(Key,DISCOUNT) || CompareKey(Key,CERTIFICATE))
		{
		    Log("PressKey>> DISCOUNT");
			int SearchDiscountMode;
			string discount;
			long long number=0;

			if (CompareKey(Key,DISCOUNT))    SearchDiscountMode=FindDiscnt;
			if (CompareKey(Key,CERTIFICATE)) SearchDiscountMode=FindCertificate;

			if (SearchDiscount(SearchDiscountMode,&discount,&number)==1)
			{
				if(CompareKey(Key,DISCOUNT))    CurrentCustomer=0;
				if(CompareKey(Key,CERTIFICATE)) CurrentCertificate=0;

				CashReg->DbfMutex->lock();
				DiscountType = (CashReg->CustomersTable->Locate("ID",0)) ? CashReg->CustomersTable->GetField("DISCOUNTS") : DefaultDiscountType;
				CashReg->DbfMutex->unlock();
			}

			ShowGoodsInfo();
			return;
		}

		if (CompareKey(Key,PRECHECK))
		{
		    Log("PressKey>> PRECHECK");
			if (!EditLineCheck("Введите код пречека!"))
				return;
			EditLine->setText("PC" + EditLine->text().upper());
			if(LookingForBarcode())
                EditLine->clear();
			return;
		}

        // +dms ==== 2011-05-17 ===== передача чека на другую кассу
        if (CompareKey(Key,SEND))//send check to other cashdesk
        {
            Log("PressKey>> SEND");
            if (IsCurrentGoodsEmpty&&(Cheque->GetSize()==0))
            {// откроем переданый чек

                try
                {
                    FILE * sendfile = fopen(sendstatefn,"r");
                    if (sendfile)
                    {
                        fclose(sendfile);
                        if (!ShowQuestion("Открыть переданный с другой кассы чек?",false))
                            return;

                        if (rename(sendstatefn, statefn))
                        {
                            ShowMessage("Ошибка открытия чека!\nСообщите администратору!");
                            return;
                        }

                        Cheque->Clear();
                        EditLine->clear();
                        RestoreSet();
                        UpdateList();
                        painter.end();
                        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,SENDCHECK_GET,Cheque->GetSum(),CashReg->CurrentSaleman->Id,-1);
                        ShowMessage("Чек восстановлен!");
                    }
                }
                catch(cash_error &er)
                {
                    CashReg->LoggingError(er);
                }

            }
            else
            {// передадим чек на другую кассу
                if (!ShowQuestion("Передать чек на другую кассу?",false))
                    return;

                bool ok;
                QString addr = QInputDialog::getText("APM-KACCA", W2U(" Введите адрес кассы "), QLineEdit::Normal, tr("apm-kacca1"), &ok, this );
                if ( ok && !addr.isEmpty() )
                {  // пытаемся отправить

                    Log(" ===> SEND CHECK ....");

                    QFile curfile;
                    curfile.setName("/Kacca/tmp/state.dat");
                    if (!curfile.exists())
                    {
                        Log(" ERROR (file not exists)");
                        ShowMessage("Файл чека не найден!");
                        return;
                    }

                    string host = U2W(addr);

                    Log((char*)("    send check to host "+host).c_str());

                    char cmd[256];
                    sprintf( cmd, "./sendcheck %s", host.c_str());
//;;Log(cmd);
                    system(cmd);

//                QFtp ftp;
//                ftp.connectToHost(addr);
//                ftp.login();
//                ftp.cd("/topsecret/csv");
//                ftp.put(("price-list.csv", &curfile);
//                ftp.close();

                    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,SENDCHECK_POST,Cheque->GetSum(),CashReg->CurrentSaleman->Id,-1,NULL,NULL,NULL,(char*)("Чек передан с "+Int2Str(CashReg->GetCashRegisterNum())+" на "+host).c_str());

                    string message = "MESSAGE: Передан чек с кассы №"+Int2Str(CashReg->GetCashRegisterNum());

                    CashReg->Socket->SendMessage(host.c_str(), message.c_str());

                    Log(" SUCCESS");
                }
            }

            return;
        }

        // +dms ==== 2010-03-29 ===== система ввода "свободной суммы"
		if (CompareKey(Key,FREESUMMA) && EditLine->ctrl)  // ввод "свободной суммы"
		{
            Log("PressKey>> FREESUMMA");
			if (!ShowQuestion("Добавить товар?",false))
                return;

            int GuardRes=NotCorrect;
			if ((!CashReg->CurrentSaleman->FreeSum)&&(!CheckGuardCode(GuardRes)))
						return;

			EditLine->clear();
			if (!EditLineCheck(""))
				return;

			bFreeSum=true;
			FreeSum = new FreeSumForm(this);

			QString GoodsName,GoodsPrice,GoodsCount,GoodBarcode;
			bool NoDiscount=false;
			bool GType = false;

			for(;;)
			{
				FreeSum->NameEdit->setText(GoodsName);
				FreeSum->PriceEdit->setText(GoodsPrice);
				FreeSum->CountEdit->setText(GoodsCount);
				FreeSum->NoDiscount->setChecked(NoDiscount);
				FreeSum->GType->setChecked(GType);


				if (FreeSum->exec()==QDialog::Accepted)
				{
					GoodsName	= FreeSum->NameEdit->text();
					GoodBarcode = FreeSum->BarcodeEdit->text();
					GoodsCount	= FreeSum->CountEdit->text();
					GoodsPrice	= FreeSum->PriceEdit->text();
					NoDiscount  = FreeSum->NoDiscount->isChecked();
					GType       = FreeSum->GType->isChecked();

					double val;
                    // проверка наименования
					if (GoodsName.isEmpty())
					{
						ShowMessage("Не задано наименование!");
						continue;
					}

                    // проверка количества
                    val = 0;
					if (!GoodsCount.isEmpty())
					{
						val = GoodsCount.toDouble();
					}

                    if (val<=0)
                    {
                        ShowMessage("Неправильное количество товара!");
                        continue;
                    }

                    // проверка цены
                    val = 0;
					if (!GoodsPrice.isEmpty())
					{
						val = GoodsPrice.toDouble();
					}

                    if (val<=0)
                    {
                        ShowMessage("Неправильная цена товара!");
                        continue;
                    }


                    if (!FreeSum->valid)
                    {
                        ShowMessage("Неправильный штрих-код: "+U2W(FreeSum->InfoLabel->text()));
                        continue;
                    }


                    bFreeSum=false;
                    delete FreeSum;
                    FreeSum=NULL;

                    // Добавляем специальную позицию товара
                    try
                    {
                        char tmpstr[1024];
                        sprintf(tmpstr, " =FreeSum= BC=%s, N=%s, C=%s, P=%s, Wt=%c, NoD=%c",
                                U2W(GoodBarcode).c_str(), U2W(GoodsName).c_str(),
                                U2W(GoodsCount).c_str(), U2W(GoodsPrice).c_str(),
                                GType?'t':'f', NoDiscount?'t':'f');
                        Log(tmpstr);

                        CurrentGoods=&CurGoods;
                        CurrentGoods->ItemCode		  = FreeSumCode;
                        CurrentGoods->ItemBarcode	  = U2W(GoodBarcode);
                        CurrentGoods->ItemName		  = U2W(GoodsName);
                        CurrentGoods->ItemDBPos		  = 0;

                        if (!GType) // тип товара
                        {
                            CurrentGoods->ItemPrecision = 1; // штучный
                        }
                        else
                        {
                            CurrentGoods->ItemPrecision = 0.001; // весовой товар
                        }

                        CurrentGoods->ItemCount       = RoundToNearest( GoodsCount.toDouble(), CurrentGoods->ItemPrecision );
                        CurrentGoods->ItemFullPrice	  = CurrentGoods->ItemPrice = RoundToNearest( GoodsPrice.toDouble(), 0.01 );
                        CurrentGoods->ItemCalcPrice	  = CurrentGoods->ItemFullPrice;
                        CurrentGoods->ItemExcessPrice = CurrentGoods->ItemFullPrice;
                        CurrentGoods->ItemFixedPrice  = CurrentGoods->ItemFullPrice;

                        CurrentGoods->NDS             = 0;

                        // расчет суммы
                        CurrentGoods->CalcSum();

                        if(!NoDiscount)
                        {
                            CurrentGoods->ItemDiscountsDesc = DefaultDiscountType;
                        }
                        else
                        {
                            string ZDiscountType;
                            for(int i=0;i<MAXDISCOUNTLEN;i++) ZDiscountType+="Z";
                            CurrentGoods->ItemDiscountsDesc = ZDiscountType;
                        }

                        CurrentGoods->ItemDepartment	= CashReg->ConfigTable->GetLongField("DEPARTMENT");
                        CurrentGoods->ItemNumber        = Cheque->GetSize()+1;

                        CurrentGoods->InputType   = IN_FREE;

                        CurrentGoods->ItemFlag    = SL;
                        CurrentGoods->GuardCode   = -1;
                        CurrentGoods->PaymentType = CashPayment;
                        time(&(CurrentGoods->tTime));

                        CurrentGoods->KKMNo     = 0;
                        CurrentGoods->KKMSmena  = 0;
                        CurrentGoods->KKMTime   = 0;

                        if(!Cheque->GetCount())
                            CashReg->m_vem.StartDoc(1, CashReg->GetLastCheckNum()+1);

                        // добавляем в чек
                        AppendRecord2Check();

                        CashReg->m_vem.AddGoods(CurrentGoods->ItemNumber,CurrentGoods,Cheque->GetCalcSum(),CurGoods.InputType);

                        CashReg->LogAddGoods(CurrentGoods);

                        if (PriceChecker==NULL)
                            IsCurrentGoodsEmpty=false;

                    }
                    catch(cash_error& er)
                    {
                        CashReg->LoggingError(er);
                    }

                    //ShowGoodsInfo();

				}
				break;
			}

            if (FreeSum!=NULL)
            {
                bFreeSum=false;
                delete FreeSum;
                FreeSum=NULL;
            }

			return;
		} // if (CompareKey(Key,FREESUMMA))  // ввод "свободной суммы"

        // +dms ==== 2010-05-17 ===== скринсейвер
		if (CompareKey(Key,SCREENSAVER) && EditLine->ctrl)  // скринсейвер
		{
		    Log("PressKey>> SCREENSAVER");
		    ShowScreenSaver = true;
            StartScreenSaver();
		} // if (CompareKey(Key,SCREENSAVER))  // svreensaver

	}
	catch(int er)
	{
		ErrorHandler(er);
	}
	catch(cash_error& er)
	{
		CashReg->DbfMutex->unlock();
		CashReg->LoggingError(er);
	}
}

void WorkPlace::ProcessBarcode(void)
{
    ;;Log("[FUNC] ProcessBarcode [START]");
    if (ScreenSaver!=NULL)
        ScreenSaver->close();

    RestartTimerSS(true);

	InitWorkPlace();
	try
	{
	    // + dms =====\ Определяем где использовать считываемый штрих-код
		QString bcode = EditLine->text();
//bcode = считываемый штрих код сканером
		// + dms ========= Акция Сбербанк - при считываниии определенных кодов  чек добавляется спец. товар ===========
        // if (AddActionGoods(U2W(bcode))) return;
        // - dms ======================== /

		if (PriceChecker!=NULL) //Проверка на proceChecker разобраться что это такое
		{
		    bcode = PriceChecker->SearchEdit->text();	
			
		    if(bcode.left(3)=="993")
		    {
		        AppendRecord2Check();
		        //;;Log("[FUNC] ProcessBarcode - PriceChecker [END]");
		        return;
		    }
		}
		
//Условие с priceChecker пропускается, считывал код проданной сметаны
		
        if (FreeSum!=NULL)
        {
            bcode = FreeSum->BarcodeEdit->text();
            FreeSum->valid = false;
        }
		// + dms =====/

		if (PriceChecker!=NULL)
			PriceChecker->Clear();

		for(int i = 0; i < bcode.length(); i++){ //Добавление всех товаров с маркировками, начало создания списка
			if(!bcode[i].isNumber()){
				if (LookingForBarcode())
                    EditLine->clear();           
			//;;Log("[FUNC] ProcessBarcode - LookingForBarcode-1 [END]");
			return; //Заканчиваем список маркированных товаров
			}
		}
		
		CashReg->DbfMutex->lock();
		if (CashReg->BarInfoTable->Locate("CODENAME","EAN13"))
			bcode=W2U(CashReg->BarInfoTable->GetStringField("CODEID"))+bcode;
		CashReg->DbfMutex->unlock();

        if (PriceChecker!=NULL) PriceChecker->SearchEdit->setText(bcode);
        else if (FreeSum!=NULL) FreeSum->BarcodeEdit->setText(bcode);
             else EditLine->setText(bcode);
		//PriceChecker==NULL? EditLine->setText(bcode) : PriceChecker->SearchEdit->setText(bcode);

		if (LookingForBarcode())
                    EditLine->clear();

        //;;Log("[FUNC] ProcessBarcode - LookingForBarcode-2 [END]");
        return;
    }

	catch(int er)
	{
		ErrorHandler(er);
	}
	catch(cash_error& er)
	{
		CashReg->DbfMutex->unlock();
		CashReg->LoggingError(er);
	}
	//;;Log("[FUNC] ProcessBarcode [END]");
}

bool WorkPlace::EditLineCheck(string msg)//check edit line status
{
	if (PaymentKeyPress>0)
	{
		ShowMessage("Чек не закрыт! Нажмите клавишу РАСЧЕТ!");
		return false;
	}

	if ((EditLine->text().isEmpty())&&(!msg.empty()))
	{
		ShowMessage(msg);
		return false;
	}

	return true;
}

void WorkPlace::SetPaymentType(FiscalCheque *CurCheque,int payment_type)
{
	CurCheque->SetPaymentType(payment_type);
	/*
	for (unsigned int i=0;i<CurCheque->GetSize();i++)
	{
		GoodsInfo tmpGoods=(*CurCheque)[i];
		tmpGoods.PaymentType=payment_type;
		CurCheque->SetAt(i,tmpGoods);
	}
	*/
}


/*
int WorkPlace::StoreRec2Check()
{

}
*/

void WorkPlace::UpdateList()
{
;;Log("[Func] UpdateList [START]");
Log("Kollichestvo=" + Double2Str(GoodsInfo().ItemCount));
	ChequeList->clear();
	int iNum=0;
	
	for(int i = 0; i < Cheque->GetCount(); i++){
		if((*Cheque)[i].ItemFlag == SL){
			QListViewItem *lastItem=ChequeList->firstChild();//update check's list

			if (lastItem!=NULL){
				for(;;){
					if (lastItem->itemBelow()!=NULL)
						lastItem=lastItem->itemBelow();
					else
						break;
				}
			}

//			QListViewItem *item=new QListViewItem(ChequeList,lastItem);//add goods to the cheque list

            GoodsInfo g = (*Cheque)[i];
            QColor color = Qt::black;

            if ((*Cheque)[i].ItemCode==FreeSumCode)
                color = Qt::darkMagenta;
            else
                if (CashReg->GoodInAction(&g))
                    color = Qt::darkBlue;

            if ((*Cheque)[i].DoubleWeightGoods)
                color = Qt::darkRed;
            ColorListViewItem *item=new ColorListViewItem(ChequeList, lastItem, color);//add goods to the cheque list
			item->setText(ListNo,Int2Str(++iNum).c_str());
            // + dms === 2010-03-30 =====\ спец. код не выводим
            item->setText(ListCode, (*Cheque)[i].ItemCode==FreeSumCode? "" : Int2Str((*Cheque)[i].ItemCode).c_str());
            // - dms ===/
			item->setText(ListName,W2U((*Cheque)[i].ItemName));
			item->setText(ListPrice,GetRounded((*Cheque)[i].ItemPrice,0.01).c_str());
			item->setText(ListCount,GetRounded((*Cheque)[i].ItemCount,(*Cheque)[i].ItemPrecision).c_str());
			item->setText(ListSum,GetRounded((*Cheque)[i].ItemSum,0.01).c_str());
			item->setText(ListDscnt,GetRounded((*Cheque)[i].ItemCalcSum-(*Cheque)[i].ItemSum,0.01).c_str());
			ChequeList->setSelected(item,true);
		}
	}

    // Пересчет скидок есть внутри процедуры ShowGoodsInfo
	//ProcessDiscount(Cheque);

    // + dms
    ShowGoodsInfo();
    // - dms
//;;Log("[Func] UpdateList [End]");
}

//static char * statefn = "tmp/state.dat";
//static char * sendstatefn = "tmp/sendstate.dat";

int WorkPlace::SaveSet()
{
	int checklen = Cheque->GetCount();
	if(!checklen)
		return 0;

	FILE * f = fopen( statefn, "w");
	if(!f)
		return 1;
	fprintf(f, "Customer=%lld\n", CurrentCustomer);
	fprintf(f, "ByPhone=%d\n", CustomerByPhone?1:0);
	fprintf(f, "AddonCustomer=%lld\n", CurrentAddonCustomer);
	fprintf(f, "tsum=%.2f\n", Cheque->tsum);
	fprintf(f, "trunc=%.2f\n", Cheque->trunc);
	//fprintf(f, "DiscountSumm=%.2f\n", Cheque->DiscountSumm);
	fprintf(f, "DiscountSumm=%.2f\n", 0);
	fprintf(f, "CertificateSumm=%.2f\n", Cheque->CertificateSumm);
	fprintf(f, "BankBonusSumm=%.2f\n", Cheque->BankBonusSumm);
	fprintf(f, "count=%d\n", checklen);
	for(int i = 0; i < checklen; i++){
		fputs((*Cheque)[i].ItemName.c_str(), f);
		fputs("\n",f);
		fputs((*Cheque)[i].ItemBarcode.c_str(), f);
		fputs("\n",f);
		fputs((*Cheque)[i].ItemSecondBarcode.c_str(), f);
		fputs("\n",f);
		fputs((*Cheque)[i].ItemDiscountsDesc.c_str(), f);
		fputs("\n",f);
		fprintf(f, "ItemCode=%d\n", (*Cheque)[i].ItemCode);
		fprintf(f, "ItemNumber=%d\n", (*Cheque)[i].ItemNumber);
		fprintf(f, "ItemDBPos=%d\n", (*Cheque)[i].ItemDBPos);
		fprintf(f, "ItemDepartment=%d\n", (*Cheque)[i].ItemDepartment);
		fprintf(f, "ItemFlag=%d\n", (*Cheque)[i].ItemFlag);
		fprintf(f, "PaymentType=%d\n", (*Cheque)[i].PaymentType);
		fprintf(f, "GuardCode=%d\n", (*Cheque)[i].GuardCode);
		fprintf(f, "CustomerCode=%lld\n", (*Cheque)[i].CustomerCode[0]);
		fprintf(f, "AddonCustomerCode=%lld\n", (*Cheque)[i].CustomerCode[1]);
		fprintf(f, "InputType=%d\n", (*Cheque)[i].InputType);
		fprintf(f, "DiscFlag=%d\n", (*Cheque)[i].DiscFlag);
		fprintf(f, "ItemPrice=%.3f\n", (*Cheque)[i].ItemPrice);
		fprintf(f, "ItemFullPrice=%.3f\n", (*Cheque)[i].ItemFullPrice);
		fprintf(f, "ItemMinimumPrice=%.3f\n", (*Cheque)[i].ItemMinimumPrice);
		fprintf(f, "ItemCalcPrice=%.3f\n", (*Cheque)[i].ItemCalcPrice);
		fprintf(f, "ItemFixedPrice=%.3f\n", (*Cheque)[i].ItemFixedPrice);
		fprintf(f, "ItemSum=%.3f\n", (*Cheque)[i].ItemSum);
        fprintf(f, "ItemSumToPay=%.3f\n", (*Cheque)[i].ItemSumToPay);
		fprintf(f, "ItemFullSum=%.3f\n", (*Cheque)[i].ItemFullSum);
		fprintf(f, "ItemFullDiscSum=%.3f\n", (*Cheque)[i].ItemFullDiscSum);
        fprintf(f, "ItemBankDiscSum=%.3f\n", (*Cheque)[i].ItemBankDiscSum);
		fprintf(f, "ItemCalcSum=%.3f\n", (*Cheque)[i].ItemCalcSum);
		fprintf(f, "ItemDiscount=%.3f\n", (*Cheque)[i].ItemDiscount);
		fprintf(f, "ItemDSum=%.3f\n", (*Cheque)[i].ItemDSum);
		fprintf(f, "ItemPrecision=%.3f\n", (*Cheque)[i].ItemPrecision);
		fprintf(f, "ItemCount=%.3f\n", (*Cheque)[i].ItemCount);
		fprintf(f, "tTime=%d\n", (*Cheque)[i].tTime);
		fprintf(f, "NDS=%.2f\n", (*Cheque)[i].NDS);
		fputs((*Cheque)[i].Lock.c_str(), f);
		fputs("\n",f);
		fprintf(f, "LockCode=%d\n", (*Cheque)[i].LockCode);
		fprintf(f, "Avans=%d\n", (*Cheque)[i].Avans);
		fprintf(f, "ItemMask=%d\n", (*Cheque)[i].ItemMask);
		fprintf(f, "ItemTextile=%d\n", (*Cheque)[i].ItemTextile);
		fprintf(f, "ItemMilk=%d\n", (*Cheque)[i].ItemMilk);
		fprintf(f, "ItemCigar=%d\n", (*Cheque)[i].ItemCigar);
        fprintf(f, "ItemAlco=%d\n", (*Cheque)[i].ItemAlco);

        int AlcoCnt = (*Cheque)[i].Actsiz.size();
        fprintf(f, "AlcoCnt=%d\n", AlcoCnt);

        for (int j=0;j<AlcoCnt;j++)
        {
            fputs((*Cheque)[i].Actsiz[j].c_str(), f);
            fputs("\n",f);
        }


		//fprintf(f, "Section=%d\n", (*Cheque)[i].Section);
	}
	fclose(f);
	return 0;
}

inline void fgets(char* buf, FILE * f)
{
	char * nl;
	fgets(buf, 1024, f);
	nl = strrchr(buf, 0x0d);//'\n');
	if(nl) *nl = '\0';
	nl = strrchr(buf, 0x0a);//'\n');
	if(nl) *nl = '\0';
}

int WorkPlace::RestoreSet(bool UseDisc, bool NoActsiz)
{
	FILE * f = fopen( statefn, "r");
	if(!f)
		return 1;
	fscanf(f, "Customer=%lld\n", &CurrentCustomer);
	int t=0;
	fscanf(f, "ByPhone=%d\n", &t);
	CustomerByPhone = (t==1);
	fscanf(f, "AddonCustomer=%lld\n", &CurrentAddonCustomer);
	fscanf(f, "tsum=%lf\n", &(Cheque->tsum));
	fscanf(f, "trunc=%lf\n", &(Cheque->trunc));
	fscanf(f, "DiscountSumm=%lf\n", &(Cheque->DiscountSumm));
	Cheque->DiscountSumm=0;
	Cheque->DiscountSummNotAction=0;
	fscanf(f, "CertificateSumm=%lf\n", &(Cheque->CertificateSumm));
	fscanf(f, "BankBonusSumm=%lf\n", &SumRestoreBankBonus);
	int checklen;
	fscanf(f, "count=%d\n", &checklen);

	int NoAlcoCnt = 0;

	for(int i = 0; i < checklen; i++){
		GoodsInfo g;
		char buf[1024];

		fgets(buf, f);
		g.ItemName = buf;

		fgets(buf, f);
		g.ItemBarcode = buf;

		fgets(buf, f);
		g.ItemSecondBarcode = buf;

		fgets(buf, f);
		g.ItemDiscountsDesc = buf;

		fscanf(f, "ItemCode=%d\n", &(g.ItemCode));
		fscanf(f, "ItemNumber=%d\n", &(g.ItemNumber));
		fscanf(f, "ItemDBPos=%d\n", &(g.ItemDBPos));
		fscanf(f, "ItemDepartment=%d\n", &(g.ItemDepartment));
		fscanf(f, "ItemFlag=%d\n", &(g.ItemFlag));
		fscanf(f, "PaymentType=%d\n", &(g.PaymentType));
		fscanf(f, "GuardCode=%d\n", &(g.GuardCode));
		fscanf(f, "CustomerCode=%lld\n", &(g.CustomerCode[0]));
		fscanf(f, "AddonCustomerCode=%lld\n", &(g.CustomerCode[1]));
		fscanf(f, "InputType=%d\n", &(g.InputType));
		fscanf(f, "DiscFlag=%d\n", &(g.DiscFlag));
		fscanf(f, "ItemPrice=%lf\n", &(g.ItemPrice));
		fscanf(f, "ItemFullPrice=%lf\n", &(g.ItemFullPrice));
		fscanf(f, "ItemMinimumPrice=%lf\n", &(g.ItemMinimumPrice));
		fscanf(f, "ItemCalcPrice=%lf\n", &(g.ItemCalcPrice));
		fscanf(f, "ItemFixedPrice=%lf\n", &(g.ItemFixedPrice));
		fscanf(f, "ItemSum=%lf\n", &(g.ItemSum));
		fscanf(f, "ItemSumToPay=%lf\n", &(g.ItemSumToPay));
		fscanf(f, "ItemFullSum=%lf\n", &(g.ItemFullSum));
		fscanf(f, "ItemFullDiscSum=%lf\n", &(g.ItemFullDiscSum));
		fscanf(f, "ItemBankDiscSum=%lf\n", &(g.ItemBankDiscSum));
		fscanf(f, "ItemCalcSum=%lf\n", &(g.ItemCalcSum));
		fscanf(f, "ItemDiscount=%lf\n", &(g.ItemDiscount));
		fscanf(f, "ItemDSum=%lf\n", &(g.ItemDSum));
		fscanf(f, "ItemPrecision=%lf\n", &(g.ItemPrecision));
		fscanf(f, "ItemCount=%lf\n", &(g.ItemCount));
		fscanf(f, "tTime=%d\n", &(g.tTime));
		time(&(g.tTime));
		fscanf(f, "NDS=%lf\n", &(g.NDS));
		fgets(buf, f);
		g.Lock = buf;
		fscanf(f, "LockCode=%d\n", &(g.LockCode));
		//fscanf(f, "Section=%d\n", &(g.Section));
		fscanf(f, "Avans=%d\n", &(g.Avans));
		fscanf(f, "ItemMask=%d\n", &(g.ItemMask));

short ItemTextile=0;
		fscanf(f, "ItemTextile=%d\n", &ItemTextile);
g.ItemTextile = ItemTextile;
short ItemMilk=0;
		fscanf(f, "ItemMilk=%d\n", &ItemMilk);
g.ItemMilk = ItemMilk;

		fscanf(f, "ItemCigar=%d\n", &(g.ItemCigar));
		fscanf(f, "ItemAlco=%d\n", &(g.ItemAlco));
        int AlcoCnt;
        fscanf(f, "AlcoCnt=%d\n", &AlcoCnt);
        int NoActsizCntCur=0;

        string tmpStr="";
        for (int j=0;j<AlcoCnt;j++)
        {
            fgets(buf, f);
            tmpStr=buf;
            if (!NoActsiz){
                g.Actsiz.insert(g.Actsiz.end(), tmpStr);
            }
            else
            {
                NoAlcoCnt++;
                NoActsizCntCur++;
            }
        }

		if (!UseDisc) g.ItemDiscount=0;
		if (!NoActsizCntCur) Cheque->Insert(g);
	}
    if (NoAlcoCnt)
        ShowMessage("Алкогольные товары НЕ восстановлены!\n\nАлкоголь считайте заново!");

    if (!UseDisc)
    {
        CurrentAddonCustomer=0;
    }
	long long number = CurrentCustomer;

	CashReg->DbfMutex->lock();
	DiscountType = (CashReg->CustomersTable->Locate("ID",0)) ? CashReg->CustomersTable->GetField("DISCOUNTS") : DefaultDiscountType;
	CashReg->DbfMutex->unlock();
    string DefDiscount = DiscountType;
	string discount;
	if (SearchDiscount(FindAuto,&discount,&number)==1)
	{
		CurrentCustomer=0;
		NeedCert=0;
		DiscountType = DefDiscount;
	}
	return 0;
}

int WorkPlace::CalcCountPrice()
{
	if(PriceChecker!=NULL)
		return 0;

	return CashReg->CalcCountPrice(CurrentGoods);
}

int WorkPlace::CheckUncorrectEnd()
{
    Log("[FUNC] CheckUncorrectEnd [START]");
	FILE *uce;
	b_uncorrectend = true;
	uce = fopen("./tmp/uncorrectend", "r");
	if(!uce) {
		b_uncorrectend = false;
		Log("[FUNC] CheckUncorrectEnd - NO UNCORRECTED [END]");
		return 0;
	}

	string date;
	char cdate[18];
	fscanf(uce, "%s", cdate);
	fclose(uce);
	date = cdate;
	int GuardRes = NotCorrect;
	RestoreSet();

	UpdateList();
	b_uncorrectend = false;
	remove("./tmp/uncorrectend");
	if (ShowQuestion("Касса была выключена с открытым "+date+" чеком!!! Аннулировать чек?",false))
	{
		int GuardRes=NotCorrect;   //check guard id
		if ((!CashReg->CurrentSaleman->Annul)&&(!CheckGuardCode(GuardRes)))
			return 1;
		CashReg->ProcessRegError(FiscalReg->Cancel());//annul cheque
		IsChequeInProgress=false;
		CashReg->AnnulCheck(Cheque,GuardRes);
		IsChequeInProgress=false;
		InitWorkPlace();
		AutoRestoreKeepedCheque();
	}
	else
	{

	    if (SumRestoreBankBonus)
	    {

	        //ShowMessage("Б О Н У С Ы: "+Double2Str(SumRestoreBankBonus));

            OnlyCashlessMode = true;

            CashReg->AddPayment(11,"UNCORRECT",&SumRestoreBankBonus);

            CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,RESTOREBONUS,SumRestoreBankBonus,CashReg->CurrentSaleman->Id,-1);

            ShowGoodsInfo();

            EditLine->ctrl=true;
            ProcessKey(4100);

	    }

    }
/*	else
	do{
	}while(!CheckGuardCode(GuardRes));
*/
    Log("[FUNC] CheckUncorrectEnd [END]");
	return 1;
}

int WorkPlace::RestoreKeepedCheque()
{
	try
	{
		keepstate = fopen("./tmp/keepstate.dat","r");
		if (keepstate)
		{
			int storechecknum;
			char buf[256];
			while(!feof(keepstate))
			{
				fscanf(keepstate,"%s",buf);
				sscanf(buf,"KCHNUM=%d", &storechecknum);
			}
			fclose(keepstate);
			storenum = fopen("./tmp/storenum.dat","w");
			fprintf(storenum,"%d",CashReg->GetLastCheckNum()+1);
			fclose(storenum);
			if ((storechecknum-CashReg->GetLastCheckNum())>100 || (CashReg->GetLastCheckNum()-storechecknum)>100 || storechecknum>1000000000 || CashReg->GetLastCheckNum()>1000000000){
				char tmpstr[128];//="Ошибка нумерации чеков. Текущий чек: "+CashReg->GetLastCheckNum()+". Отложенный чек: "+storechecknum+".\n Сообщите администратору!!!"
				//sprintf(tmpstr,"Ошибка нумерации чеков. Текущий чек: %d. Отложенный чек: %d.\n Сообщите администратору",CashReg->GetLastCheckNum(),storechecknum);
				sprintf(tmpstr,"Ошибка нумерации чеков. Текущий чек: %d. Отложенный чек: %d.\n Сообщите администратору!%d!%d!",CashReg->GetLastCheckNum(),storechecknum,CashReg->GetLastCheckNum()-storechecknum,storechecknum-CashReg->GetLastCheckNum());
				ShowMessage((string)tmpstr);
			}
			CashReg->SetLastCheckNum(storechecknum-1);

			if (rename("tmp/keepstate.dat","tmp/state.dat")){
				ShowMessage("Ошибка восстановления чека!\nСообщите администратору!");
				return -1;
			}
			Cheque->Clear();
			EditLine->clear();
			RestoreSet();
			IsCurrentGoodsEmpty=true;
			UpdateList();
			KeepCheckInfo->setProperty( "text","");
			painter.end();
			ShowMessage("Отложенный чек восстановлен!");
			return 1;
		}
		else return 0;
	}
	catch(cash_error &er)
	{
		CashReg->DbfMutex->unlock();
		CashReg->LoggingError(er);
	}
}

int WorkPlace::AutoRestoreKeepedCheque()
{
	if(keepstate=fopen("tmp/keepstate.dat","r"))
	{//
		fclose(keepstate);
		ShowMessage("Восстановление отложенного чека!");
		InitWorkPlace();
		RestoreKeepedCheque();
		return 1;
	}
	return 0;
}

void WorkPlace::CheckCheque(void)
{
    Log("[FUNC] CheckCheque [START]");
//	CheckUncorrectEnd();
	if(!CheckUncorrectEnd())
		AutoRestoreKeepedCheque();
	CashReg->KeepCheckInfo->setText("");
	//return 1;
	Log("[FUNC] CheckCheque [END]");
}

void WorkPlace::InitWPSlot()
{
	InitWorkPlace();
}

// #include <stdlib.h>
#include<malloc.h>
int WorkPlace::StartPriceChecker(int key)
{
	//
	char* kodstr=NULL;
	kodstr = (char*)malloc(20*sizeof(char));
	if (!kodstr){
		ShowMessage("Недостаточно памяти!");
		return 0;
	}
	pthread_t thread1;
	string strost;
	int pid1 = (int)getpid();
	int result;
	//
	PriceChecker->Clear();
//	PriceChecker->NameLine->setText(W2U(CurrentGoods->ItemName)+"*");
//	PriceChecker->CodeLine->setText(Int2Str(CurrentGoods->ItemCode).c_str());
//	PriceChecker->QuantLine->setText(GetRounded(CurrentGoods->ItemCount,CurrentGoods->ItemPrecision).c_str());
//	PriceChecker->SummLine->setText(GetRounded(CurrentGoods->ItemSum,0.01).c_str());
	FiscalCheque FoundPrices;
	GoodsInfo GoodsPrices;
	FoundPrices.Clear();
	DiscInfo.clear();
	DiscountInfo tmpDscInf;
	string upcbarcode="";
	try
	{
		thisApp->setOverrideCursor(waitCursor);
		InfoMessage->setText(W2U("Поиск товара"));

		if((key==4148||key==41480||key==12345))
		{
		    if(!PriceChecker->SearchEdit->text().isEmpty())
		    {
                QString EnterText = PriceChecker->SearchEdit->text();
                if(EnterText.left(3)=="993")
                {// Это бонусная Карта
                    if(key!=12345) return 0;

                    string CardNumberStr = U2W(EnterText).substr(0,12);
                    long long CardNumberL = atoll(CardNumberStr.c_str());
                    long long res=NotCorrect;
                    CashReg->FindCustomer(&res, CardNumberL);
                    if (res == NotCorrect) return 0;


                    CashReg->DbfMutex->lock();

                    int isBonusCard=0;
                    string curDiscountType="";
                    string strCardType="";

                    if(CashReg->CustomersTable->GetLLongField("ID")==CardNumberL)
                    {
                        curDiscountType=CashReg->CustomersTable->GetField("DISCOUNTS");
                        isBonusCard=CashReg->CustomersTable->GetLongField("NEEDCERT");
                    }
                    else
                    {
                        curDiscountType=CashReg->CustRangeTable->GetField("DISCOUNTS");
                        isBonusCard=CashReg->CustRangeTable->GetLongField("BONUSCARD");

                    }

                    if (CashReg->CustRangeTable->GetFirstRecord())
                    {// ищем скидку в таблице диапазонов
                        do{
                            if(CashReg->CustRangeTable->GetLLongField("IDFIRST") <= CardNumberL &&
                            CashReg->CustRangeTable->GetLLongField("IDLAST") >= CardNumberL)
                            {
                                int intCardType=CashReg->CustRangeTable->GetLongField("CUSTTYPE");
                                switch(intCardType)
                                {
                                    case 0:strCardType="Партнеры";break;
                                    case 1:strCardType="VIP";break;
                                    case 5:strCardType="Super-VIP";break;
                                    case 2:
                                    case 4:strCardType="5%";break;
                                    case 3:strCardType="2%";break;
                                }
                                break;
                            }
                        }
                        while(CashReg->CustRangeTable->GetNextRecord());
                    }


                    string isBonusCardStr = isBonusCard?"Бонусная":"Дисконтная";

                    // скидки
                    string tmpDiscountsDesc="";
                    for(int i=0; i<MAXDISCOUNTLEN; i++) tmpDiscountsDesc+="-";
                    for(int c=0;c<curDiscountType.length();c++) tmpDiscountsDesc[c]='-';

                    CashReg->DiscountsTable->GetFirstRecord();
                    do{
                        int type=tmpDscInf.type=CashReg->DiscountsTable->GetLongField("DISCTYPE");
                        if(type != 0 && type <= curDiscountType.length())
                        {
                            tmpDscInf.description=CashReg->DiscountsTable->GetField("DESCRIPT");
                            tmpDscInf.percent=CashReg->DiscountsTable->GetDoubleField("PERCENT");
                            tmpDscInf.fullpercent=CashReg->DiscountsTable->GetField("FPERCENT");
                            tmpDscInf.mode=CashReg->DiscountsTable->GetLongField("MODE");
                            DiscInfo.insert(DiscInfo.end(),tmpDscInf);
                        }
                    }while(CashReg->DiscountsTable->GetNextRecord());

                    CashReg->DbfMutex->unlock();


                    if (!isBonusCard)
                    {
                        CardInfoStruct CardInfo;
                        if (CashReg->FindCardInRangeTable(&CardInfo, CardNumberL))
                        {
                            if (CardInfo.BonusCard==1) isBonusCard = 1;
                        }
                    }
                    double BonusBalans=0;

                    PriceChecker->NameLine->setText(W2U(isBonusCardStr));
                    PriceChecker->SummLine->setText(GetRounded(BonusBalans,0.01).c_str());

                    GoodsPrices.ItemName=isBonusCardStr;
                    GoodsPrices.ItemBarcode= "Тип карты";
                    GoodsPrices.ItemSecondBarcode= isBonusCardStr;
                    GoodsPrices.ItemDiscountsDesc=curDiscountType;
                    FoundPrices.Insert(GoodsPrices);

                    GoodsPrices.ItemName=isBonusCardStr;
                    GoodsPrices.ItemBarcode= "Вид карты";
                    GoodsPrices.ItemSecondBarcode= strCardType;
                    GoodsPrices.ItemDiscountsDesc=curDiscountType;
                    FoundPrices.Insert(GoodsPrices);

                    string DTypeStr = "Стандартные";
                    if ((curDiscountType.length()>6) && (curDiscountType[6]=='Z'))
                        DTypeStr = "Настроенные";

                    GoodsPrices.ItemBarcode= "Тип скидки";
                    GoodsPrices.ItemSecondBarcode= DTypeStr;
                    FoundPrices.Insert(GoodsPrices);

                    if (isBonusCard)
                    {
                        // Информация по карте
                        ;;Log("+ GetCardInfo +");
                        BNS_RET* retInf = CashReg->BNS->exec("inf", CardNumberStr, "");
                        ;;Log("- GetCardInfo -");
                        if (!retInf->ReturnCode)
                        {
                            BonusBalans = retInf->ActiveBonuses;

                            GoodsPrices.ItemName=isBonusCardStr;
                            GoodsPrices.ItemBarcode= "Соединение";
                            GoodsPrices.ItemSecondBarcode= "Online";
                            GoodsPrices.ItemDiscountsDesc=curDiscountType;
                            FoundPrices.Insert(GoodsPrices);

                            GoodsPrices.ItemName=isBonusCardStr;
                            GoodsPrices.ItemBarcode= "Активные бонусы";
                            GoodsPrices.ItemSecondBarcode= GetRounded(retInf->ActiveBonuses,0.01);
                            GoodsPrices.ItemDiscountsDesc=curDiscountType;
                            FoundPrices.Insert(GoodsPrices);

                            //! Убрать удержанные бонусы
                            //GoodsPrices.ItemBarcode= "Удержанные бонусы";
                            //GoodsPrices.ItemSecondBarcode= GetRounded(retInf->HoldedBonuses,0.01);
                            //FoundPrices.Insert(GoodsPrices);

                            GoodsPrices.ItemBarcode= "Статус";
                            string CurSt = retInf->status;

                            if (CurSt=="1") CurSt="Активирована";
                            else if (CurSt=="2") CurSt="Подозрительная";
                            else if (CurSt=="3") CurSt="Проверена";
                            else if (CurSt=="4") CurSt="Не активирована";
                            else if (CurSt=="255") CurSt="Заблокирована";

                            GoodsPrices.ItemSecondBarcode= CurSt;
                            FoundPrices.Insert(GoodsPrices);

                            PriceChecker->SummLine->setText(GetRounded(BonusBalans,0.01).c_str());

                        }
                        else
                        {
                            GoodsPrices.ItemBarcode= "Соединение";
                            GoodsPrices.ItemSecondBarcode= "Off-line";
                            FoundPrices.Insert(GoodsPrices);

                            PriceChecker->SummLine->setText("Offline");

                        }

                    }

                    InfoMessage->clear();
                    thisApp->restoreOverrideCursor();
                    if (FoundPrices.GetCount()==0){
                        PriceChecker->Clear();
                        EditLine->clear();
                        if (key==47)
                            ShowMessage("Карта не найдена.");
                    }
                    else{
                        PriceChecker->ShowBonusList(&FoundPrices);////
                        FoundPrices.Clear();
                    }

                    return 0;

                }
           }
        }

        // ==== Информация по бонусам ====


		if (key==4148)
			ProcessBarcode();
		if (key!=47)
		{
			upcbarcode ="0";
			upcbarcode+= PriceChecker->SearchEdit->text().ascii();
		}
		PriceChecker->NameLine->setText(W2U(CurrentGoods->ItemName)+"*");
		PriceChecker->CodeLine->setText(Int2Str(CurrentGoods->ItemCode).c_str());
	//	PriceChecker->QuantLine->setText(GetRounded(CurrentGoods->ItemCount,CurrentGoods->ItemPrecision).c_str());
	//	PriceChecker->SummLine->setText(GetRounded(CurrentGoods->ItemSum,0.01).c_str());
		//char *kod; sprintf(kod,"%i",CurrentGoods->ItemCode);
		//sprintf(kodstr,"%i",CurrentGoods->ItemCode);//kod=Int2Str(CurrentGoods->ItemCode);
		//char buffer[16];int value=CurrentGoods->ItemCode;
		//int i=itoa(value,kod,10);
		//CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,
		//					20/*PRICECHECK*/,CurrentGoods->ItemPrice,
		//					CashReg->CurrentSaleman->Id,-1,CashReg->GoodsTable->GetStringField("BARCODE").c_str(),
		//					kodstr);
							//Int2Str(CurrentGoods->ItemCode).c_str());
		CashReg->DbfMutex->lock();
		//bool first=false;
		if (CashReg->GoodsTable->GetFirstRecord())
			do{
				if( ( (key==4148||key==41480) && !PriceChecker->SearchEdit->text().isEmpty() && (U2W(PriceChecker->SearchEdit->text())==CashReg->GoodsTable->GetStringField("BARCODE") || upcbarcode==CashReg->GoodsTable->GetStringField("BARCODE")) && wbarcode==0)
				||( (key==4148||key==41480) && !PriceChecker->SearchEdit->text().isEmpty() && wbarcode!=0 && wbarcode==CashReg->GoodsTable->GetLongField("CODE") )//
				||( key==47	&& !PriceChecker->SearchEdit->text().isEmpty() && PriceChecker->SearchEdit->text().toInt()==CashReg->GoodsTable->GetLongField("CODE")  ) )
				{
					GoodsPrices.ItemName=CashReg->GoodsTable->GetStringField("NAME");
					GoodsPrices.ItemCode=CashReg->GoodsTable->GetLongField("CODE");
					GoodsPrices.ItemBarcode=CashReg->GoodsTable->GetStringField("BARCODE");
					GoodsPrices.ItemDiscountsDesc=CashReg->GoodsTable->GetStringField("SKIDKA");
					GoodsPrices.ItemCount=CashReg->GoodsTable->GetDoubleField("MULT");
					GoodsPrices.ItemPrice=CashReg->GoodsTable->GetDoubleField("PRICE");
					GoodsPrices.ItemFixedPrice=CashReg->GoodsTable->GetDoubleField("FIXEDPRICE");
					/*if (first){
						sprintf(kodstr,"%i",GoodsPrices->ItemCode);
						CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,
							20,GoodsPrices->ItemPrice,
							CashReg->CurrentSaleman->Id,-1,GoodsPrices.ItemBarcode.c_str(),kodstr);
					}*/
					if (FoundPrices.GetCount()==0)
					{
						sprintf(kodstr,"%i",GoodsPrices.ItemCode);
						CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,
							20/*PRICECHECK*/,GoodsPrices.ItemPrice,
							CashReg->CurrentSaleman->Id,-1,GoodsPrices.ItemBarcode.c_str(),kodstr);
						if(wbarcode!=0){
							PriceChecker->QuantLine->setText(GetRounded(CurrentGoods->ItemCount,CurrentGoods->ItemPrecision).c_str());
							PriceChecker->SummLine->setText(GetRounded(CurrentGoods->ItemSum,0.01).c_str());
						}else{
							PriceChecker->QuantLine->setText(GetRounded(GoodsPrices.ItemCount,CurrentGoods->ItemPrecision).c_str());
							PriceChecker->SummLine->setText(GetRounded(GoodsPrices.ItemPrice,0.01).c_str());
						}
						CashReg->DiscountsTable->GetFirstRecord();
						// + dms
						string tmpDiscountsDesc="";
						for(int i=0; i<MAXDISCOUNTLEN; i++) tmpDiscountsDesc+="-";
						// - dms
						for(int c=0;c<GoodsPrices.ItemDiscountsDesc.length();c++)
							tmpDiscountsDesc[c]='-';
						//memset( tmpDiscountsDesc, '-', GoodsPrices.ItemDiscountsDesc.size());



						do{
							int type=tmpDscInf.type=CashReg->DiscountsTable->GetLongField("DISCTYPE");

							int allgoodssign=CashReg->DiscountsTable->GetLongField("ALLGOODS");
							if(type != 0 && type <= GoodsPrices.ItemDiscountsDesc.length()){
								tmpDscInf.description=CashReg->DiscountsTable->GetField("DESCRIPT");
								switch (GoodsPrices.ItemDiscountsDesc[type-1])
								{
								case '.':allgoodssign==1 ? tmpDiscountsDesc[type-1]='.':tmpDiscountsDesc[type-1]='X';break;
								case 'X':allgoodssign==1 ? tmpDiscountsDesc[type-1]='X':tmpDiscountsDesc[type-1]='Z';break;
								case 'Z':allgoodssign==1 ? tmpDiscountsDesc[type-1]='Z':tmpDiscountsDesc[type-1]='Z';break;
								default :tmpDiscountsDesc[type-1]='-';
								}
							DiscInfo.insert(DiscInfo.end(),tmpDscInf);
							}
						}while(CashReg->DiscountsTable->GetNextRecord());
						GoodsPrices.ItemDiscountsDesc=tmpDiscountsDesc;
					}

					FoundPrices.Insert(GoodsPrices);
					int c=0;
					/*for(int c=0;;c++)*/
					while(1){
						char fname[64];
						sprintf(fname, "Q%d", c);
						if(CashReg->GoodsTable->GetFieldNo(fname) == -1 || CashReg->GoodsTable->GetDoubleField(fname) < .001)
							break;
						GoodsPrices.ItemCount = CashReg->GoodsTable->GetDoubleField(fname);
						sprintf(fname, "P%d", c);
						if(CashReg->GoodsTable->GetFieldNo(fname) == -1)
							break;
						GoodsPrices.ItemPrice = CashReg->GoodsTable->GetDoubleField(fname);
						GoodsPrices.ItemBarcode="";//CashReg->GoodsTable->GetStringField("BARCODE");
						GoodsPrices.ItemFixedPrice=0.0;//CashReg->GoodsTable->GetDoubleField("FIXEDPRICE");
						FoundPrices.Insert(GoodsPrices);
						c++;
						/*if (CashReg->GoodsTable->GetDoubleField("Q0")>0.001 || CashReg->GoodsTable->GetDoubleField("P0")>0.001){
							GoodsPrices.ItemCount=CashReg->GoodsTable->GetDoubleField("Q0");
							GoodsPrices.ItemPrice=CashReg->GoodsTable->GetDoubleField("P0");
							FoundPrices.Insert(GoodsPrices);
						}*/
					}
				}
			}
			while(CashReg->GoodsTable->GetNextRecord());

		CashReg->DbfMutex->unlock();

		InfoMessage->clear();
		thisApp->restoreOverrideCursor();
		if (FoundPrices.GetCount()==0){
			PriceChecker->Clear();
			EditLine->clear();
			if (key==47)
				ShowMessage("Товар с указанным кодом не найден.");
		}
		else{
			PriceChecker->ShowPriceList(&FoundPrices);////
			FoundPrices.Clear();
		}
			//PriceChecker->showEvent();
			//SQLThread->start();
			//delete SQLOst;
			//SQLOst = new SQLThread(CashReg);
			//SQLServer->InitInstance();
	//	if(!CashReg->SQLServer->running())
	//		CashReg->SQLServer->start();
			//тут отдельным потоком расчет остатков
			//select sum(kol) from mggetosttov('+NomenDMod.NomenQKodNom.AsString+',''today'', ''today'')
			//bool ok;
			//unsigned long lCode=PriceChecker->CodeLine->text().toULong(&ok, 10);
			//if (ok){
				//ShowMessage(Double2Str(CashReg->SQLServer->GetOstTov(lCode)));
				//result = pthread_create(&thread1, NULL, Empty/*PriceChecker->GetOstTov(lCode)*/, &pid1);
				//if (result != 0)
					//ShowMessage("thread error");
				//else
					//ShowMessage(strost);
				//PriceChecker->RestLine->setText(GetRounded(CashReg->SQLServer->GetOstTov(lCode),0.001).c_str());

			//}else
				//PriceChecker->RestLine->setText("ERRCODE");
	}
	catch(cash_error& er)
	{
		CashReg->DbfMutex->unlock();
		CashReg->LoggingError(er);
		FoundPrices.Clear();
	}
}

int WorkPlace::SearchDiscount(int SearchDiscountMode,string *discount,long long *number)
{
#ifdef ZAPRETBONUS
	bool isDiscount,isCertificate,isMoney,isFlyer;
	isDiscount=false;
	isCertificate=false;
	isMoney=false;
	isFlyer=false;
	long long GuardRes=NotCorrect;
    long long CurSert = 0LL;
    long long OldCustomer = CurrentCustomer;
    long long OldAddonCustomer = CurrentAddonCustomer;

    CustomerByPhone=false;

	if (SearchDiscountMode == FindAuto)		isDiscount	= true;
	if (SearchDiscountMode == FindFromScaner)	isDiscount	= true;
	if (SearchDiscountMode == FindDiscnt)		isDiscount	= true;
	if (SearchDiscountMode == FindCertificate)	isCertificate	= true;
	if (SearchDiscountMode == FindMoney)		isMoney		= true;

	{
		char tmpstr[1024];
		sprintf(tmpstr,"SearchDiscount number = %lld",*number);
		Log(tmpstr);
	}

	if (SearchDiscountMode != FindAuto)
	{
		if (!EditLineCheck(""))
			return -1;

		if (!CashReg->CurrentSaleman->Discounts)
		{
			ShowMessage("Отсутствуют права на предоставление скидки!");
			return -1;
		}

		if (CurrentCertificate && isCertificate)
		{
			ShowMessage("Сертификат уже выбран!");
			return -1;
		}

		if(SearchDiscountMode != FindFromScaner)
		{
			GuardRes=CheckBarcode(Customer);//check customer's id
		}
		else
		{
			long long res=NotCorrect;
			CashReg->FindCustomer(&res,*number);
			GuardRes=res;
		}

        isFlyer = CashReg->IsFlyer(GuardRes);

        // Акция выдачи карты "Добрые соседи" при предъявлении флаера
        if ((isFlyer) && (InActionDobryeSosedy(CurrentCustomer, GuardRes)))
        {
             return ShowMessageAndLogInfo(Card2Str(*number)+"\n Сначала считайте карту \"Добрые соседи\" \n Затем считайте флаер!");
        }
        //==============

		if (!isFlyer && CurrentCustomer && isDiscount)
		{

			//ShowMessage("Пользователь скидки уже выбран!");
            //return -1;

            if (!ShowQuestion("Заменить текущую дисконтную карту?",true))
            {
                return -1;
            }
		}

		if ((GuardRes==NotCorrect) && isDiscount)
		{
			return ShowMessageAndLogInfo(Card2Str(*number)+" Неправильный пароль покупателя");
		}

		if ((GuardRes==NotCorrect) && isCertificate)
		{
    		return ShowMessageAndLogInfo(Card2Str(*number)+" Неправильный номер сертификата");
		}

		if ((GuardRes==NotCorrect) && isMoney)
		{
    		return ShowMessageAndLogInfo(Card2Str(*number)+" Неправильный номер сертификата");
		}

		if (GuardRes==NoPasswd)
			return -1;

		if (isDiscount)
            if (isFlyer) CurrentAddonCustomer = GuardRes;
            else CurrentCustomer=GuardRes;
		if (isCertificate)	CurrentCertificate=GuardRes;
        if (isMoney)        CurSert = GuardRes;


	}
	else
	{
	    long long res=NotCorrect;
        CashReg->FindCustomer(&res,*number);
		GuardRes = *number;

		isFlyer = CashReg->IsFlyer(GuardRes);
	}

	bool TimeLimit;
	bool FromRange=false;
    string TextInfo = "";

	CashReg->DbfMutex->lock();

	if (CashReg->CustomersTable->GetLLongField("ID")==GuardRes)
	{
	    TextInfo = CashReg->CustomersTable->GetField("INFO");
	    TextInfo = trim(TextInfo);
    }

	if (CashReg->CustomersTable->GetLLongField("ID")==GuardRes)
		TimeLimit=xbDate().JulianDays()<xbDate(CashReg->CustomersTable->GetField("ISSUE").c_str()).JulianDays();
	else
		TimeLimit=xbDate().JulianDays()<xbDate(CashReg->CustRangeTable->GetField("ISSUE").c_str()).JulianDays();
	CashReg->DbfMutex->unlock();

    if (!TextInfo.empty())
    {
        TextInfo = "Информация по карте N "+
        Card2Str(CashReg->CustomersTable->GetLLongField("ID"))
        +":\n\n"+TextInfo;

        ShowMessage(TextInfo);
    }

    if (
    (GuardRes>=992000498000LL && GuardRes<=992000499199LL) //	Ветеран
    ||
    (GuardRes>=992000470752LL && GuardRes<=992000485751LL) //	Ветеран
    ||
    (GuardRes>=992000425752LL && GuardRes<=992000455751LL) //	Ветеран
    ||
    (GuardRes>=992000621200LL && GuardRes<=992000721199LL) //	Ветеран
    ||
    (GuardRes>=992000399752LL && GuardRes<=992000419751LL) //	Ветеран
    ||
    (GuardRes>=922000425752LL && GuardRes<=922000455751LL) //	Ветеран
    ||
    (GuardRes>=992001468819LL && GuardRes<=992001574168LL) //	Ветеран Айкай
    ||
    (GuardRes>=992001051800LL && GuardRes<=992001151188LL) //	Ветеран Айкай
    ||
    (GuardRes>=993000840000LL && GuardRes<=993000844999LL) //	Ветеран Гастроном
    ||
    (GuardRes>=992000000001LL && GuardRes<=992000010000LL) //	Неизвестный диапазон 2-5%
    ||
    (GuardRes>=993000500100LL && GuardRes<=993000502999LL) //	Океан детства
    ||
    (GuardRes>=993000500000LL && GuardRes<=993000500099LL) //	Океан детства VIP
    ||
    (GuardRes>=993000900001LL && GuardRes<=993000901000LL) //	Океан мелочей
    )
    {
        //TextInfo = "ЗАМЕНИТЬ КАРТУ # "+ LLong2Str(GuardRes) +"\n\nВыдать взамен карту Любимого клиента Гастроном";
        TextInfo = "ЗАМЕНИТЬ КАРТУ # "+ Card2Str(GuardRes) +"\n\nВыдать взамен карту Любимого клиента Гастроном";

        ShowMessage(TextInfo);
    }

    if (GuardRes>=444000012001LL && GuardRes<=444000037200LL) {
        TextInfo = "ЗАМЕНИТЬ КАРТУ\n\nВыдать карту Гастроном Динамо";
        ShowMessage(TextInfo);
    }

	if (TimeLimit && isDiscount)
	{
		CurrentCustomer = OldCustomer;
		CurrentAddonCustomer = OldAddonCustomer;
   		return ShowMessageAndLogInfo(Card2Str(*number)+" Срок действия карты еще не наступил!");
	}

	if (TimeLimit && isCertificate)
	{
   		return ShowMessageAndLogInfo(Card2Str(*number)+" Срок действия сертификата еще не наступил!");
	}

	if (TimeLimit && isMoney)
	{
   		return ShowMessageAndLogInfo(Card2Str(*number)+" Срок действия сертификата еще не наступил!");
	}

	CashReg->DbfMutex->lock();
	if (CashReg->CustomersTable->GetLLongField("ID")==GuardRes)
		TimeLimit=xbDate().JulianDays()>xbDate(CashReg->CustomersTable->GetField("EXPIRE").c_str()).JulianDays();
	else
	{
		TimeLimit=xbDate().JulianDays()>xbDate(CashReg->CustRangeTable->GetField("EXPIRE").c_str()).JulianDays();
		FromRange=true;
	}
	CashReg->DbfMutex->unlock();


	CashReg->DbfMutex->lock();
	string CardAttr;
	if(isDiscount)    CardAttr="Карта N ";

	if(isCertificate) CardAttr="Сертификат N ";

	//CardAttr+=LLong2Str(FromRange?CurrentCustomer:CashReg->CustomersTable->GetLLongField("ID"));
    //if(isFlyer) CardAttr = "Карта-флаер N "+LLong2Str(CurrentAddonCustomer);
    //if(isMoney) CardAttr = "Сертификат N "+LLong2Str(FromRange?CurSert:CashReg->CustomersTable->GetLLongField("ID"));

	CardAttr+=Card2Str(FromRange?CurrentCustomer:CashReg->CustomersTable->GetLLongField("ID"));

    if(isFlyer) CardAttr = "Карта-флаер N "+Card2Str(CurrentAddonCustomer);
    if(isMoney) CardAttr = "Сертификат N "+Card2Str(FromRange?CurSert:CashReg->CustomersTable->GetLLongField("ID"));

    CashReg->DbfMutex->unlock();


//!+ Замена карт Фармакон и пр. на карты Гастроном
    if (CurrentCustomer < 993000000000LL)
    {
        CardInfoStruct CardInfoExp;
        if (CashReg->FindCardInRangeTable(&CardInfoExp, CurrentCustomer))
        {
            if(CardInfoExp.isExpire) {
                CurrentCustomer = OldCustomer;
                CurrentAddonCustomer = OldAddonCustomer;
                return ShowMessageAndLogInfo(CardAttr+"\n\nКАРТА ЗАБЛОКИРОВАНА!\n\nНЕОБХОДИМО ЗАМЕНИТЬ КАРТУ НА КАРТУ ГАСТРОНОМ 2%!");
            }
        }
    }
//!- Замена карт Фармакон и пр. на карты Гастроном

	if (TimeLimit && isDiscount)
	{
		//ShowMessage(CardAttr+"\nКарта заблокирована или срок действия истек!");
		CurrentCustomer = OldCustomer;
		CurrentAddonCustomer = OldAddonCustomer;
		return ShowMessageAndLogInfo(CardAttr+"\nКарта заблокирована или срок действия истек!");
	}

	if (TimeLimit && isCertificate)
	{
		return ShowMessageAndLogInfo(CardAttr+"\nСертификат заблокирован!");
	}
	if (TimeLimit && isMoney)
	{
		return ShowMessageAndLogInfo(CardAttr+"\nСертификат заблокирован!");
	}

	bool goNext;
	goNext = false;

	goNext = (SearchDiscountMode == FindAuto);
	if(!goNext)
	{
	    if(goNext = ShowQuestion(CardAttr,true))
	    {

	        //////
            long long res=NotCorrect;
            CashReg->FindCustomer(&res,GuardRes);
            /////

            *number = GuardRes;
	    }
	    else
	    {   if (isFlyer) CurrentAddonCustomer = OldAddonCustomer;

            if (OldCustomer)
            {
                CurrentCustomer = OldCustomer;
                *number = OldCustomer;
                return -1;
            }
	    }
	}

	if (goNext)//check discount's and customer's params
	{
		CashReg->DbfMutex->lock();

		NeedCert=0;

		int recno;
		if(!FromRange)
		{
			DiscountType_T=CashReg->CustomersTable->GetField("DISCOUNTS");
			NeedCert=CashReg->CustomersTable->GetLongField("NEEDCERT");
		}
		else
		{
			DiscountType_T=CashReg->CustRangeTable->GetField("DISCOUNTS");
		}

		CashReg->DbfMutex->unlock();

		*discount = DiscountType_T;

        /*
        if (DiscountType_T[1]=='.')
                ShowMessage(CardAttr+"\n\n    Внимание!!!\
                \n    Для участия в Новогодней акции нужно заполнить \
                \nанкетные данные.");
        */

        if (CashReg->FindCustomerInfo(CurrentCustomer))
        {
            CashReg->DbfMutex->lock();
            double disc_summ = CashReg->CustomersValueTable->GetDoubleField("DISCSUMMA");
            double total_summ = CashReg->CustomersValueTable->GetDoubleField("SUMMA");
            CashReg->DbfMutex->unlock();

            CashReg->ShowLevelDiscountMessage(disc_summ, total_summ);
		}


        if (!isMoney) // скидки меняем только если не оплата
        {

          if(!isFlyer)
          {
            DiscountType="";
            CashReg->LoadDiscountTypeFromDB(0LL,&DiscountType);

            for (int ii = 0; ii < MAXDISCOUNTLEN; ii++)
            {
                if (DiscountType.length()<(ii+1))
                    DiscountType+='X';
//!!!!!!!!!!!
//                if (DiscountType_T[ii] == '.')
//                    DiscountType[ii] = '.';//
//!!!!!!!!!!!

                string sk = "0123456789Z";

                if (DiscountType_T[ii] == '.')
                    DiscountType[ii] = '.';
                else
                  {
					if( sk.find(DiscountType_T[ii]) != string::npos)
                    DiscountType[ii] = DiscountType_T[ii];
                  }

            }
          }

          AddAddonDiscount(CurrentAddonCustomer);

        }

		ProcessDiscount(Cheque);
		double dsum = Cheque->GetDiscountSum();
		CashReg->m_vem.Discount(Cheque->GetCalcSum(),3,GuardRes, dsum);

		char str[1024];
		sprintf(str, "DiscountType = %s; DSum = %f", DiscountType.c_str(), dsum);
		Log(str);

        // ===BONUS===

        CurrentActiveBonus=0;
        StatusBonusCard ="";
        CurrentBonusBalans=0;

        if (!NeedCert)
        {
            CardInfoStruct CardInfo;
            if (CashReg->FindCardInRangeTable(&CardInfo, CurrentCustomer))
            {
                if (CardInfo.BonusCard==1) NeedCert = 1;
            }
        }

/*
        if (NeedCert)
        {
            string card = LLong2Str(CurrentCustomer);
            string strCheck = Int2Str(CashReg->GetLastCheckNum()+1);

            // Информация по карте
            ;;Log("+ GetCardInfo +");
            BNS_RET* retInf = CashReg->BNS->exec("inf", card, strCheck);
            ;;Log("- GetCardInfo -");
            if (!retInf->ReturnCode)
            {
                CurrentBonusBalans = retInf->ActiveBonuses;
            }

        }
*/
        // ===BONUS===

		return 0;
	}
	return 1;
#endif
} //SearchDiscount


// проверка на диапазон подарочных сертификатов
int WorkPlace::IsCertificate(long long CardNum)
{
//        if
//        (
//            (CardNum>=993000810000LL) && (CardNum<=993000810299LL)
//            ||
//            (CardNum>=993000811000LL) && (CardNum<=993000811149LL)
//            ||
//            (CardNum>=993000812000LL) && (CardNum<=993000812099LL)
//            ||
//            (CardNum>=993000813000LL) && (CardNum<=993000813049LL)
//        )
//        {
//        // это сертификат
//        return 1;
//        }

   //
//   return 0;

	long long GuardRes=NotCorrect;
    string DiscountStr;

/*
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
//*/

	try
	{
		CashReg->DbfMutex->lock();

		if (CashReg->CustomersTable->Locate("ID",CardNum))
		{
            GuardRes=CardNum;
            DiscountStr = CashReg->CustomersTable->GetField("DISCOUNTS");


#ifdef CERTIFICATES
                    Log("-------------------------------------------------");
                    Log("Скидка=" + DiscountStr);
                    Log("Это сертификат");
                    Log("---------------------------------------------------");
#endif

        }
        else
        {

#ifdef CERTIFICATES
                    Log("****************************************************");
                    Log("Скидка=" + DiscountStr);
                    Log("Это сертификат");
                    Log("****************************************************");
#endif

            if (CashReg->CustRangeTable->GetFirstRecord())
            {// ищем скидку в таблице диапазонов
                do{
                    if(CashReg->CustRangeTable->GetLLongField("IDFIRST") <= CardNum &&
                    CashReg->CustRangeTable->GetLLongField("IDLAST") >= CardNum)
                    {
                        GuardRes=CardNum;
                        DiscountStr = CashReg->CustRangeTable->GetField("DISCOUNTS");
                        break;
                    }
                }
                while(CashReg->CustRangeTable->GetNextRecord());
            }
        }
		CashReg->DbfMutex->unlock();

	}
	catch(cash_error& er)
	{
		CashReg->DbfMutex->unlock();
		CashReg->LoggingError(er);
	}

	//CashReg->FindCustomer(&GuardRes, CardNum);

    if (GuardRes==NotCorrect)
    {
        //ShowMessage("Неправильный номер сертификата.");
        return 0;
    }

    if (GuardRes==NoPasswd)
        return -1;

    int recno;

    bool IsRealCert = false;

	try
	{
		CashReg->DbfMutex->lock();

		// проверяем на сертификат
		CashReg->DiscountsTable->GetFirstRecord();

		do
		{
			int type=CashReg->DiscountsTable->GetLongField("DISCTYPE");
			int mode=CashReg->DiscountsTable->GetLongField("MODE");

			if( (mode==3)
             && (DiscountStr.size()>=type)
             && (DiscountStr[type-1]=='.') )
                    // он, родимый
                    IsRealCert = true;

		}while(!IsRealCert && CashReg->DiscountsTable->GetNextRecord());

        CashReg->DbfMutex->unlock();

	}
	catch(cash_error& er)
	{
		CashReg->DbfMutex->unlock();
	}

    return IsRealCert?1:0;

} //IsCertificate


int WorkPlace::IsCertificate(int CardNum)
{
    return IsCertificate(993000000000LL + (long long) CardNum);
}


int WorkPlace::SearchCertificate(int SearchDiscountMode,string *discount,long long *number, double *summa)
{
	bool isCertificate,isMoney;
	isCertificate=false;
	isMoney=false;
	long long GuardRes=NotCorrect;
    long long CurSert = 0LL;

	if (SearchDiscountMode == FindAuto)		    isCertificate = true;
	if (SearchDiscountMode == FindFromScaner)	isCertificate = true;
	if (SearchDiscountMode == FindCertificate)	isCertificate = true;
	if (SearchDiscountMode == FindMoney)		isMoney		= true;


	if (SearchDiscountMode != FindAuto)
	{

		if (!CashReg->CurrentSaleman->Discounts)
		{
			ShowMessage("Отсутствуют права на предоставление скидки!");
			return -1;
		}

		if(SearchDiscountMode != FindFromScaner)
		{
			GuardRes=CheckBarcode(Certificate, "Введите штрих-код сертификата");//check customer's id
		}
		else
		{
			long long res=NotCorrect;
			CashReg->FindCustomer(&res,*number);
			GuardRes=res;
		}

		if (GuardRes==NotCorrect)
		{
            return ShowMessageAndLogInfo(LLong2Str(*number)+" Неправильный номер сертификата");
		}

		if (GuardRes==NoPasswd)
			return -1;

		if (isCertificate)	CurrentCertificate=GuardRes;
        if (isMoney)        CurSert = GuardRes;

	}
	else
	{
	    long long res=NotCorrect;
        CashReg->FindCustomer(&res,*number);
		GuardRes = *number;
	}

	bool TimeLimit;
	bool FromRange=false;

	CashReg->DbfMutex->lock();
	if (CashReg->CustomersTable->GetLLongField("ID")==GuardRes)
		TimeLimit=xbDate().JulianDays()<xbDate(CashReg->CustomersTable->GetField("ISSUE").c_str()).JulianDays();
	else
		TimeLimit=xbDate().JulianDays()<xbDate(CashReg->CustRangeTable->GetField("ISSUE").c_str()).JulianDays();
	CashReg->DbfMutex->unlock();


	if ((TimeLimit)
	&& (!NeedActionVTB))
	{
        return ShowMessageAndLogInfo(LLong2Str(*number)+" Срок действия сертификата еще не наступил!");
	}


	CashReg->DbfMutex->lock();
	if (CashReg->CustomersTable->GetLLongField("ID")==GuardRes)
		TimeLimit=xbDate().JulianDays()>xbDate(CashReg->CustomersTable->GetField("EXPIRE").c_str()).JulianDays();
	else
	{
		TimeLimit=xbDate().JulianDays()>xbDate(CashReg->CustRangeTable->GetField("EXPIRE").c_str()).JulianDays();
		FromRange=true;
	}
	CashReg->DbfMutex->unlock();

	if (TimeLimit)
	{
        return ShowMessageAndLogInfo(LLong2Str(*number)+" Сертификат заблокирован!");
	}

	CashReg->DbfMutex->lock();
	string CardAttr;

	CardAttr="Сертификат N ";
	CardAttr+=LLong2Str(FromRange?GuardRes:CashReg->CustomersTable->GetLLongField("ID"));
	CashReg->DbfMutex->unlock();

	{
		char tmpstr[1024];
		sprintf(tmpstr,"Search sertificate number = %lld",GuardRes);
		Log(tmpstr);
	}


//	goNext = (SearchDiscountMode == FindAuto);

//	if (goNext)//check discount's and customer's params
	{
		CashReg->DbfMutex->lock();
		int recno;
		string DiscountStr;

		if(!FromRange)
		{
			DiscountStr=CashReg->CustomersTable->GetField("DISCOUNTS");
		}
		else
		{
			DiscountStr=CashReg->CustRangeTable->GetField("DISCOUNTS");
		}

		CashReg->DbfMutex->unlock();

        *discount = DiscountStr;

        try
        {
            CashReg->DbfMutex->lock();

            //ищем сумму сертификата
            CashReg->DiscountsTable->GetFirstRecord();
            time_t cur_time=time(NULL);
            tm *l_time=localtime(&cur_time);
            int Seconds=l_time->tm_hour*3600+l_time->tm_min*60+l_time->tm_sec;
            int Days=xbDate().JulianDays();

            double Charge=0,WholeCharge=0;

            bool IsRealCert = false;

            do
            {
                int type=CashReg->DiscountsTable->GetLongField("DISCTYPE");
                int mode=CashReg->DiscountsTable->GetLongField("MODE");

                if(mode != 3) continue;


                if(
                    (DiscountStr.size()<type)
                    ||
                    (DiscountStr[type-1]!='.')
                )
                    continue;


                double tmpCharge;

                if (mode==3)
                {
                    tmpCharge=CashReg->DiscountsTable->GetDoubleField("CHARGE");//recalculate goods price
                    IsRealCert = true;
                }

                if (tmpCharge>Charge) Charge=tmpCharge;

            }while(CashReg->DiscountsTable->GetNextRecord());

            CashReg->DbfMutex->unlock();

            //допускаются только сертификаты
            if (!IsRealCert)
            {
                *number = 0;
                summa = 0;

                return ShowMessageAndLogInfo(LLong2Str(GuardRes)+" не является подарочным сертификатом!");
            }

            *summa=Charge;

            string CardAttr;
            CardAttr = "Сертификат N ";
            CardAttr+=LLong2Str(GuardRes);

            if(ShowQuestion(CardAttr,true))
                *number = GuardRes;
            else return 1;

        }
        catch(cash_error& er)
        {
            CashReg->DbfMutex->unlock();
            *number = 0;
            summa = 0;
            //LoggingError(er);
            return -1;
        }

        char str[1024];
        sprintf(str, "Discount = %s; Summa = %f", DiscountType.c_str(), *summa);
        Log(str);

        return 0;
	}

	return 1;

} // SearchCertificate

void WorkPlace::UpdateBankStatus(void)
{
	QFont tmpFont=BankInfo->font();
	tmpFont.setStrikeOut(!CashReg->StatusBankConnection);
	BankInfo->setFont(tmpFont);
	BankInfo->repaint();
}

void WorkPlace::TimerBankAction(void)
{
    if (CashReg->TryBankConnect) CashReg->TestBankConnection(0, 1);
}

//!bool WorkPlace::GoodsLock(FiscalCheque* CurCheque, bool ShowMsg = true)
bool WorkPlace::GoodsLock(FiscalCheque* CurCheque, bool ShowMsg)
{
    if (!CurCheque) CurCheque = Cheque;
	if (!CurCheque) return false;

    try
    {
        for (int i=0;i<CurCheque->GetCount();i++)
        {
            if ((*CurCheque)[i].ItemFlag == SL)
            {
              string str = (*CurCheque)[i].Lock;
              if (CashReg->GoodsLock(str, GL_TimeLock) == GL_TimeLock)
              {
                  if (ShowMsg) ShowMessage("Продажа товара с кодом "+Int2Str((*CurCheque)[i].ItemCode)+" запрещена!");
                  return true;
              }
            }
        }
    }
	catch(cash_error& er)
	{
		CashReg->LoggingError(er);
	}

	return false;
}


void WorkPlace::StartScreenSaver(void)
{
    if (ScreenSaver!=NULL)
        return;

    if (!ShowScreenSaver)
        return;

    if (Cheque->GetCount()!=0)
        return;

    if (CurrentCustomer!=0)
        return;

    if (!EditLine->text().isEmpty())
        return;

    ScreenSaver = new ScreenSaverForm(this);

    ScreenSaver->path = W2U(CashReg->DBPicturePath)+"/";
    ScreenSaver->filename = "pic.jpg";
    ScreenSaver->WSize = CashReg->size();

    ScreenSaver->exec();

    delete ScreenSaver;
    ScreenSaver=NULL;

    return;
}


void WorkPlace::RestartTimerSS(bool start)
{
    if (TimerScreenSaver->isActive())
         TimerScreenSaver->stop();

    if (start && ShowScreenSaver)
    {
        TimerScreenSaver->start(WAITTIMESS);
    }

}


void WorkPlace::RunMessager(string MessageStr)
{

    const char *msgfile = "./tmp/history.msg";

    if (Messager!=NULL)
    {
        Messager->FillHistory(msgfile);
        return;
    }
    else
    {
        if (MessageStr!="")
        {
             int ret = QMessageBox::information
                    ( NULL, "APM KACCA", W2U(MessageStr)
                    ,W2U("Закрыть"), W2U("Ответить"),QString::null , 0, -1);
             if(ret!=1) return ;
        }
    }

    //if (Messager==NULL)
    Messager = new MsgForm(this, CashReg->LastMessageItem);

    for(;;)
    {
        Messager->EditLine->setText(CashReg->LastMessageText);
        Messager->NumberBox->setCurrentItem(CashReg->LastMessageItem);
        Messager->SaveHistory->setChecked(CashReg->SaveMessageHistory);
        Messager->FillHistory(msgfile);

        if (Messager->exec()==QDialog::Accepted)
        {
            if (!Messager->EditLine->text().isEmpty())
            {
                string header = "Касса №"+Int2Str(CashReg->GetCashRegisterNum())+":  "+CashReg->CurrentSaleman->Name+"\n";

                string addr ="apm-kacca", body,message = header+U2W(Messager->EditLine->text());

                //addr+=Int2Str((Messager->NumberBox->currentItem()+1));

                addr=U2W(Messager->NumberBox->currentText());

                body="MESSAGE:"+message;
                if ((!addr.empty())&&(!body.empty()))
                    CashReg->Socket->SendMessage(addr.c_str(),body.c_str());

                LogMessage("<<", message);

                Messager->EditLine->clear();

                CashReg->LastMessageText = Messager->EditLine->text();
                CashReg->LastMessageItem = Messager->NumberBox->currentItem();
                CashReg->SaveMessageHistory = Messager->SaveHistory->isChecked();

            }
        }
        else
        {
            break;
        }

    }
    CashReg->LastMessageText = Messager->EditLine->text();
    CashReg->LastMessageItem = Messager->NumberBox->currentItem();
    CashReg->SaveMessageHistory = Messager->SaveHistory->isChecked();

    if (!CashReg->SaveMessageHistory)
    {
        FILE *f = fopen(msgfile, "w");
        if(f)
        {
            fprintf(f,"\n");
            fclose(f);
        }
    }

    delete Messager;
    Messager = NULL;
}



int WorkPlace::AddAddonDiscount(long long number)
{
    if (!number) return 1;

	{
		char tmpstr[1024];
		sprintf(tmpstr," -> SearchAddonDiscount number = %lld", number);
		Log(tmpstr);
	}

    long long GuardRes=NotCorrect;
	long long res=NotCorrect;
	CashReg->FindCustomer(&res,number);
	GuardRes=number;

    if (GuardRes==NotCorrect)
    {
        //ShowMessage("Неправильный пароль покупателя");
        return -1;
    }

	if (GuardRes==NoPasswd)
		return -1;


	CashReg->DbfMutex->lock();
	if (CashReg->CustomersTable->GetLLongField("ID")==GuardRes)
	{
		DiscountType_T=CashReg->CustomersTable->GetField("DISCOUNTS");
	}
	else
	{
        DiscountType_T =CashReg->CustRangeTable->GetField("DISCOUNTS");
    }
	CashReg->DbfMutex->unlock();


		char str[1024];
		sprintf(str, "AddonDiscount = %s", DiscountType_T.c_str());
		Log(str);

    for (int ii = 0; ii < MAXDISCOUNTLEN; ii++)
    {
        if (DiscountType.length()<(ii+1))
            DiscountType+='X';

        string sk = "0123456789";

        bool isValue   = (sk.find(DiscountType[ii])!=string::npos);
        bool isValue_T = (sk.find(DiscountType_T[ii])!=string::npos);

        bool isDot     = (DiscountType[ii] == '.');
        bool isDot_T   = (DiscountType_T[ii] == '.');

        bool isZ     = (DiscountType[ii] == 'Z');
        bool isZ_T   = (DiscountType_T[ii] == 'Z');

        bool isX     = (DiscountType[ii] == 'X');
        bool isX_T   = (DiscountType_T[ii] == 'X');

        bool isR     = (DiscountType[ii] == 'R');
        bool isR_T   = (DiscountType_T[ii] == 'R');

//Log("$= 1 =$");
        if (isR_T)
        {
            DiscountType[ii] = 'Z';
            //Log((char*)Int2Str(ii).c_str());
           // Log("$= 1=1=1 =$");
        }
        else if (!isZ && isDot_T && !isValue)
        {
            DiscountType[ii] = '.';
           // Log((char*)Int2Str(ii).c_str());
          //  Log("$= 2 =$");
        }
        else
          {
            if (
                !isZ
                &&
                isValue_T
                &&
                (!isValue || (DiscountType[ii] < DiscountType_T[ii]))
               )
            {
                DiscountType[ii] = DiscountType_T[ii];
               // Log((char*)Int2Str(ii).c_str());
               // Log("$= 3 =$");
            }
          }
//Log("$= 4 =$");
    }

    return 0;

} //AddAddonDiscount





int WorkPlace::ProcessBankDiscounts(const char *card)
{

    string disc = "";
    long idcust =0;

    FiscalCheque::PayBank = true;

    int Result = CashReg->ProcessBankDiscounts(card, &disc, &idcust);

    if (!Result)
    {
        if(DiscountType.size() == 0)
        {
            CurrentCustomer = idcust;
            DiscountType    = disc;
        }
        else
        {
            for(int i = 0; i < DiscountType.size(); i++)
            {
                if((DiscountType[i] == '.')||(DiscountType[i] == 'Z'))   continue;
                if(disc[i] == '.')  DiscountType[i] = '.';
            }
        }

       // QMessageBox::information( NULL, "APM KACCA3", CashReg->CurrentWorkPlace->DiscountType.c_str(), "Ok");

        double a_sum = Cheque->GetSum(); // сумма чека до пересчета

        //ProcessDiscount();
        SetBankDSum();
        ShowGoodsInfo(); // пересчет скидок есть внутри процедуры

        double b_sum = Cheque->GetSum(); // сумма чека после пересчета

        string LogStr= card;
        LogStr = ">>> Bank discounts #"+LogStr.substr(0,8)+"# ="+Double2Str(a_sum-b_sum)+"\n"+disc;
        Log((char*)LogStr.c_str());
    }
    else
    {
      ShowGoodsInfo(); // пересчет скидок есть внутри процедуры
    }

    FiscalCheque::PayBank = false;

    return Result;
}


// проверка на повторный ввод весовых товаров
bool WorkPlace::CheckWeightGoods(FiscalCheque* CurCheque)
{
    if (!CurCheque) CurCheque = Cheque;
	if (!CurCheque) return false;

    bool ShowAllert=false;

    try
    {
        for (int i=0;i<CurCheque->GetCount();i++)
        {
            int code = (*CurCheque)[i].ItemCode;
            double count = (*CurCheque)[i].ItemCount;

            if
            (
            ((*CurCheque)[i].ItemFlag == SL)
            &&
            ((*CurCheque)[i].ItemPrecision != 1) // весовой товар
            &&
            ((*CurCheque)[i].ItemCount*10 - int((*CurCheque)[i].ItemCount*10) > .0001)
//            &&
//            (!(*CurCheque)[i].DoubleWeightGoods) // не отмечен
            )
            {

                for (int j=i+1;j<CurCheque->GetCount();j++)
                {
                    if
                    (
                    ((*CurCheque)[j].ItemFlag == SL)
                    &&
                    ((*CurCheque)[j].ItemPrecision != 1)
                    &&
                    ((*CurCheque)[j].ItemCode == code)
                    &&
                    ((*CurCheque)[j].ItemCount == count)
                    )
                    {
                        GoodsInfo g=(*CurCheque)[j];
                        g.DoubleWeightGoods = true;
                        CurCheque->SetAt(j,g);

                        ShowAllert = true;
                    }// if
                }//for j
            }// if
        } //for i
    }
	catch(cash_error& er)
	{
		CashReg->LoggingError(er);
	}

      if (ShowAllert)
      {
          UpdateList();

          GoodsWeightChecked=ShowQuestion("Внимание!\nВ чеке присутствуют весовые товары с одинаковым количеством!\n\nВсе равно отбить чек?",false);;
          return !GoodsWeightChecked;
      }

	return false;
}

bool WorkPlace::EnterBarcode(int Tag, string msg, string errmsg)
{

	int Res=CheckBarcode(Tag, msg);

	if ((Res==NotCorrect)
	|| (Res==NoPasswd))
	{
		if (errmsg!="") ShowMessage(errmsg);
		return false;
	}

	return true;

}

int WorkPlace::GetCertNominal(string barcode)
{
  int res =0;
  if (barcode== "100") res=100;
  else if (barcode== "250") res=250;
  else if (barcode== "500") res=500;
  else if (barcode== "1000")res=1000;
  else if (barcode== "1500")res=1500;
  else if (barcode== "2000")res=2000;
  else if (barcode== "3000")res=3000;
  else if (barcode== "5000")res=5000;

  return res;
}

int WorkPlace::ShowMessageAndLogInfo(string msg)
{
    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,CARDINFO,0,CashReg->CurrentSaleman->Id,-1,NULL,NULL,NULL, (char*)msg.c_str());
    ShowMessage(msg);
    return -1;
}


bool WorkPlace::IsHandDiscounts()
{
 bool res = true;
 for(short i=5;i<=7;i++) if (DiscountType[i]!='Z') res=false;
 return res;
}





int WorkPlace::ProcessBankBonusDiscount(string card, string sha1, bool uselimit)
{

//long WorkPlace::ProcessBankBonusPay(const char *card, string st, double bns, double a_bns, string info_text)

    if (Cheque->GetBankBonusSum()>0)
    {// делаем реверс
        if (!ShowQuestion("Внимание! Бонусы уже применены! \nОтменить текущую скидку по бонусам и задать новое значение скидки?",true))
        {
            return -1;
        }

        ProcessBankBonusDiscountReverse(card, sha1);
    }

//;;Log("+ ProcessBankBonusDiscount +");
    string disc = "";
    long idcust =0;

    long Result = 0;

    double chSumma = Cheque->GetSum()-1;

    if (chSumma<=0)
    {
        Log("БОНУСЫ:  Слишком маленькая сумма чека! (Должен быть > 1руб.)");
        return -1;
    }

// + dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========
// Минимальная сумма списания бонусов "Спасибо" не должна быть меньше 300 баллов
// Соответственно сумма чека должна быть больше 300 рублей
    if (chSumma<BANKBONUS_MINIMUM_LIMIT)
    {
        Log("БОНУСЫ:  Слишком маленькая сумма чека (менее 301 руб.)! ");
        return -1;
    }
// - dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========

;;Log("+ GetCardInfo +");

    //LProc* LPCX = new LProc();

    //LPCX ->Init();
    PCX_RET* ret = CashReg->WPCX->exec("inf", card, sha1, Cheque);
;;Log("- GetCardInfo -");



//        CashReg->LoggingAction(,ERRORBONUSRETDISC,sum*.01,,0, card, NULL, NULL, NULL, SberbankCard);// *

    CashReg->AddBankBonusOperation(ret);
//;;Log("- Step 1 -");
    if (ret->ReturnCode>=0)
    {
    //==================================================================================

	//setlocale( LC_ALL, "ru_RU->koi8-r");



//;;Log("- Step 2 -");


            double all_bns = Str2Double(ret->BNS_S) + Str2Double(ret->BNS_DELAY_S);
            double active_bns = Str2Double(ret->AB_S);


// + dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========
//Окно списания бонусов показываем только если кол-во бонусов >= 300
            if (active_bns<BANKBONUS_MINIMUM_LIMIT)
            {
                Log("БОНУСЫ:  Слишком мало бонусов (менее 300)! ");
                return -1;
            }

// Было до 01.03.2015
            //+ dms === 2014-05-08 === Ограничение на показ окна бонусов при оплате по безналу
            // окно показываем только если кол-во бонусов > 20
//            if ((uselimit) && (active_bns<=BANKBONUS_MINIMUM_LIMIT))
//            {
//                Log("БОНУСЫ:  Слишком мало бонусов (<20)! ");
//                return -1;
//            }
            //- dms === 2014-05-08 === /

// - dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========

;;Log("Выведен на экран диалог списания бонусов");

            BankBonus = new BankBonusForm(this);

			QString BonusCard = CashReg->WPCX->GetPwdCardNumber(card);
//;;Log("- Step 3 -");
			QString BonusEvalCount, BonusSumma, BonusStatus, BonusPromoText;

            QString BonusAllCount = W2U(Double2Str(RoundToNearest(all_bns, 0.001)));
            BonusEvalCount = W2U(Double2Str(RoundToNearest(active_bns , 0.001)));

            string st=trim(ret->CS_S);
            QRegExp rn("\\n");
            QRegExp sn("\\s");
            BonusStatus = W2U(st).replace(rn, "");
            BonusStatus = W2U(BonusStatus).replace(sn, "");

            string status = U2W(BonusStatus);
            BonusPromoText = W2U(ret->info);
//;;Log("- Step 4 -");
			for(;;)
			{
				BankBonus->CardEdit->setText(BonusCard);
				BankBonus->EvalCountEdit->setText(GetRounded(active_bns,0.01).c_str());
				BankBonus->AllCountEdit->setText(GetRounded(all_bns,0.01).c_str());
				BankBonus->StatusEdit->setText(BonusStatus);
                //BankBonus->InfoLabel->setText(BonusPromoText);

			if (BankBonus->exec()==QDialog::Accepted)
				{
                   if(status!="A")
                    {
                        ShowMessage("Карта неактивна, списание баллов невозможно !");
                        continue;
                    }

					double val;

					BonusSumma	= BankBonus->SummaEdit->text();

                    // проверка количества
                    val = 0;
					if (!BonusSumma.isEmpty())
					{
						val = BonusSumma.toDouble();
					}

                    if (val>0)
                    {

                       if(val>active_bns)
                        {
                            ShowMessage("Введенная сумма превышает сумму активных баллов!");
                            continue;
                        }

                       if(val>chSumma)
                        {
                            ShowMessage("Cумма баллов должна быть меньше суммы чека!");
                            continue;
                        }

                       if(val<.01)
                        {
                            ShowMessage("Слишком маленькая сумма баллов!");
                            continue;
                        }

// + dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========
// с 01.03.2015 действует новая бонусная программа:  сумма использования бонусов >= 300 бонусов
                       if(val<300)
                        {
                            ShowMessage("Сумма бонусов должна быть не меньше 300!");
                            continue;
                        }
// - dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========

//                        Log("3");
//                        Log((char*)Double2Str(val).c_str());
/*
                        CashReg->AddPayment(10,BonusCard.latin1(),&val);
                        Cheque->CertificateSumm = val;
                        ShowGoodsInfo();
*/
        //;;Log("- Step 5 -");
                        PCX_RET* ret1 = CashReg->WPCX->exec("disc", card, sha1, Cheque, val);
        //;;Log("- Step 6 -");

                        CashReg->AddBankBonusOperation( ret1 ) ;
        //;;Log("- Step 7 -");
                        if (ret1->ReturnCode>=0)
                        {

                            FiscalReg->PrintPCheck("/Kacca/pcx/p", false);

                            //;;Log("+ ProcessBankBonusDiscount +");
                                string BonusCard = card;

                                double a_sum = Cheque->GetSum(); // сумма чека до пересчета

                                CashReg->AddPayment(11,BonusCard,&val);

                                ShowGoodsInfo();

                                double b_sum = Cheque->GetSum(); // сумма чека после пересчета

                                string LogStr= card;
                                LogStr = ">>> Bank BONUS discounts #"+/* LogStr.substr(0,8)+"# ="+*/Double2Str(a_sum-b_sum);
                                Log((char*)LogStr.c_str());

                                //;;Log("- ProcessBankBonusDiscount -");
                                return 0;

                                Result = Double2Int(val*100);

                                LogStr= card;
                                LogStr = ">>> Bank BONUS Summ Enter #"+LogStr.substr(0,8)+"# ="+Double2Str(val);
                                Log((char*)LogStr.c_str());
                        }
                        else
                        {
                            ShowMessage(ret1->info);
                        }
                    }
                    else
                    {
                        ;;Log(" => Выбрано 0 баллов (Отмена)");
                    }

				}
                else
                {
                    ;;Log(" => Выбрана ОТМЕНА");
                }

				break;
			}

            if (BankBonus!=NULL)
            {
                delete BankBonus;
                BankBonus=NULL;
            }

    }
    else
    {
        if (ret->info!="")
           ShowMessage(ret->info);
    }

    //;;Log("- ProcessBankBonusDisc -");
    return Result;

}


int WorkPlace::ProcessBankBonusPay(string card, string sha1, double sum)
{
   ;;Log("+ ProcessBankBonusPay +");
   PCX_RET* ret = CashReg->WPCX->exec("pay", card, sha1, Cheque, sum);

  ;;Log("    PrintPCheck(/Kacca/pcx/p) +");
   FiscalReg->PrintPCheck("/Kacca/pcx/p", false);

    CashReg->AddBankBonusOperation( ret );
   ;;Log("- ProcessBankBonusPay -");
}


int WorkPlace::ProcessBankBonusDiscountReverse(string card, string sha1, bool isAnnul)
{
    //;;Log("+ProcessBankBonusDiscountReverse+");

    double sum = Cheque->BankBonusSumm;

    PCX_RET* ret = CashReg->WPCX->exec("rev", card, sha1, Cheque, sum);

    if(isAnnul)
       ret->action = BB_REVERSE_ANNUL;

    CashReg->AddBankBonusOperation( ret ) ;

    FiscalReg->PrintPCheck("/Kacca/pcx/p", false);

    //if (WPCX->PCX_Result.ReturnCode>=0)
    {
        ;;Log("+ REVERSE OK +");
        string BonusCard = card;

        CashReg->DeletePayment(11,BonusCard,true);

        ShowGoodsInfo();
    }

    //;;Log("-ProcessBankBonusDiscountReverse-");
    return 0;
}

bool WorkPlace::InActionDobryeSosedy(long long pCust, long long pAddonCust)
{
  return (
     (CashReg->GetMagNum()=="    44")
        && ((pAddonCust>=993000650000LL) && (pAddonCust<=993000660001LL))
        && (!CashReg->IsDobryeSosedy(pCust)));
}


int WorkPlace::ProccessBankCardNumber(const char *card)
{

    string disc = "";
    long idcust =0;


    string ActionBarcode="27084397"; // "27081914";
    string ActionCode="775373";


    string CardStr = card;
    string Bin = CardStr.substr(0,6);

    if (
        (Bin =="427601") // 42760149
        ||
        (Bin =="427901") // 42790149
        ||
        (Bin =="546901") // 54690149
        ||
        (Bin =="548401") //54840149
        ||
        (Bin =="485463") //485463 – Visa CREDIT momentum
        ||
        (Bin =="531310") //531310 – MC CREDIT momentum
//////////////////////////////////////
//        ||
//      (Bin =="42768680")

        )
{
    EditLine->setText(ActionBarcode);

    ProcessBarcode();

    ShowGoodsInfo(); // пересчет скидок есть внутри процедуры

    string LogStr= card;
    LogStr = ">>> Add action good 5% Senks from Sberbank #"+LogStr.substr(0,8)+"# ="+ActionBarcode+"\n"+disc;
    Log((char*)LogStr.c_str());
}

    return 1;
}

bool WorkPlace::AddActionGoods(string card)
{

    bool InAction=false;

    string disc = "";
    long idcust =0;

    string ActionBarcode="";
    string ActionCode="";

// 779473 4% (27089477)
// 779474 6% (27089484)

    if (
    ( (card.length()==8) && (card.substr(0,2)=="12") )
    ||
    ( (card.length()==9) && (card.substr(0,3)=="F12") )
    ||
    ( (card.length()==10) && (card.substr(0,4)=="FF12") )
    )
    {
        ActionBarcode="27089477";
        ActionCode="779473";

        InAction=true;
    }

    if (
    ( (card.length()==8) && (card.substr(0,2)=="18") )
    ||
    ( (card.length()==9) && (card.substr(0,3)=="F18") )
    ||
    ( (card.length()==10) && (card.substr(0,4)=="FF18") )
    )
    {
        ActionBarcode="27089484";
        ActionCode="779474";

        InAction=true;
    }


    if (InAction)
    {
        bool test = true;
//;;ShowMessage("Тест!");
		//проверим, если ли уже спец код для начисления доп. баллов Спасибо
		for (int i=0;i<Cheque->GetCount();i++)
		{
			GoodsInfo g=(*Cheque)[i];
            if (
                (g.ItemFlag==SL)
                &&
                (
                (g.ItemCode==779473)
                ||
                (g.ItemCode==779474)
                )
               )
            {
                test=false;
                break;
            }
		}

        if (test)
        {
            EditLine->setText(ActionBarcode);

            ProcessBarcode();

            ShowGoodsInfo(); // пересчет скидок есть внутри процедуры

            string LogStr= "";
            LogStr = ">>> Add action good 4-6% Senks from Sberbank ( CODE128 ) #"+card+"# ="+ActionBarcode+"\n"+disc;
            Log((char*)LogStr.c_str());
        }
        else
        {
            ShowMessage("Купон на начисление доп. баллов \"Спасибо\" уже считан в чеке!");
        }
    }

    return InAction;
}


double WorkPlace::GetSumActionPG()
{
    double res=0;

    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
            string str= g.Lock;

           if (CashReg->GoodsLock(str, GL_ActionPG) == GL_ActionPG)
                res+=g.ItemFullSum;
        }
    }

    return res;
}


int WorkPlace::CheckKimberly()
{
    int res=0;
    int cnt=0;
    double actsum=0;

    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
            string str= g.Lock;

           if (CashReg->GoodsLock(str, GL_ActionKimberly) == GL_ActionKimberly)
                actsum+=g.ItemCount*g.ItemFullPrice;
        }
    }

    ;;Log("===Check Kimberly===");


    // условие выдачи подарка по акции
    if (actsum>=750){
        res=1;
        ;;Log((char*)("===Action Kimberly! ("+Double2Str(actsum)+")===").c_str());
    }


    // C 28/06/2016 подарок выдается за каждый товар (было - за каждые 2шт товара)
    //res = (int)(cnt/2);

    //res = cnt;

    return res;
}


int WorkPlace::CheckSugar(string* res_str, int check_code, double check_cnt)
{
    int LIMIT_cnt = 3;
    double LIMIT_cnt_ves = 3.5;

    *res_str = "";
    int res=0;
    int cnt=0;
    double cnt_ves=0;

    if (!filepath_exists("/Kacca/tmp/CheckSugar")) {
        return res;
    }

/*
    time_t ltime;
    time( &ltime );
    struct tm *now;
    now = localtime( &ltime );

    // Период акции - 12.07.19 - 14.07.19
    if (
           !(
            (now->tm_year+1900 == 2019)
            &&
            (now->tm_mon+1 == 7)
            &&
            ((now->tm_mday >= 12) && (now->tm_mday <= 14))
           )
       )
    {
        return res;
    }
*/

    switch (check_code)
    {
        case 103518: cnt += (int)check_cnt; break;
        case 991: cnt_ves += check_cnt; break;
        case 0: break;
        default: return res;
    }


    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
           if ((g.ItemCode==103518))
                cnt+=(int)g.ItemCount;

           if ((g.ItemCode==991))
                cnt_ves+=g.ItemCount;

        }
    }

    double tot_weight = cnt_ves + (double)cnt * 0.9;

    ;;Log("==Check CheckSugar== Cnt = " + Int2Str(cnt)
          + " Cnt_ves = " + Double2Str(cnt_ves)
          + " tot_weight = " + Double2Str(tot_weight));

    if (cnt > LIMIT_cnt)
    {
        *res_str += "\nЗапрещена продажа сахара (103518) более "
                + Int2Str(LIMIT_cnt) + " шт ( в чеке "
                + Int2Str(cnt) + " шт. )";
        res = 1;
        return res;
    }

    if (tot_weight > LIMIT_cnt_ves)
    {
        *res_str += "\nЗапрещена продажа весового сахара (991) более "
                + Double2Str(LIMIT_cnt_ves) + " кг "
                "( в чеке " + Double2Str(tot_weight)+" кг. )";
        res = 1;
        return res;
    }


    return res;
}


int WorkPlace::CheckLibero()
{
    int res=0;
    int cnt=0;

    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
            string str= g.Lock;

           if (CashReg->GoodsLock(str, GL_ActionLibero) == GL_ActionLibero)
                cnt+=(int)g.ItemCount;
        }
    }

    //;;Log("==Check Libero==");
    //;;Log((char*)(Int2Str(cnt).c_str()));


    // C 28/06/2016 подарок выдается за каждый товар (было - за каждые 2шт товара)
    //res = (int)(cnt/2);

    res = cnt;

    return res;
}


int WorkPlace::CheckRedhop()
{
    int res=0;
    int cnt=0;

    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
           if ((g.ItemCode==822391) || (g.ItemCode==822392) || (g.ItemCode==822393))
                cnt+=(int)g.ItemCount;
        }
    }

    ;;Log("==Check CheckRedhop==");
    ;;Log((char*)(Int2Str(cnt).c_str()));

    // подарок выдается за каждые 3шт товара
    res = (int)(cnt/3);


    return res;
}

int WorkPlace::CheckBeerCup()
{
    int res=0;
    int cnt=0;

    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
           if ((g.ItemCode==1000205) || (g.ItemCode==18981))
                cnt+=(int)g.ItemCount;
        }
    }

    ;;Log("==Check CheckBeerCup==");
    ;;Log((char*)(Int2Str(cnt).c_str()));

    // подарок выдается за каждые 3шт товара
    res = (int)(cnt/3);


    return res;
}


int WorkPlace::CheckMonster()
{
    int res=0;
    int cnt=0;

    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
           if ((g.ItemCode==816710) || (g.ItemCode==816709) || (g.ItemCode==819837))
                cnt+=(int)g.ItemCount;
        }
    }

    ;;Log("==Check CheckMonster==");
    ;;Log((char*)(Int2Str(cnt).c_str()));

    // подарок выдается за любые 2шт товара
    res = (int)(cnt/2);


    return res;
}




int WorkPlace::ProcessBonusDiscount(long long lcard, bool uselimit)
{

    if(!CashReg->BonusMode(lcard)) return -1;

    string card = LLong2Str(lcard);

    string disc = "";
    long idcust =0;

    long Result = 0;

    string strCheck = Int2Str(CashReg->GetLastCheckNum()+1);


    // Если транзакция активна - отменим ее
    ;;Log("+CHECK PAY ID+");
    ;;Log("bonus_pay_id = '"+bonus_pay_id+"'");
    ProcessBonusDiscountReverse();

// Актуальная сумма чека
    double chSumma = Cheque->GetSum()-1;

// Информация по карте
;;Log("+ GetCardInfo +");
    BNS_RET* retInf = CashReg->BNS->exec("inf&reg", card, strCheck);
;;Log("- GetCardInfo -");
    CashReg->AddBonusOperation( retInf ) ;


    // Задействуем операцию получения баланса как
    // проверку связи c минимальным таймаутом
    // На последующие операции таймаут ставится максимальный
    if (retInf->ReturnCode)
    {
         // если ошибка проверки связи, то дальше не идем
        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,ERRORBONUSPAY,0,CashReg->GetCashmanCode(),0, card.c_str(), NULL, NULL, NULL);//
        ;;Log("ERROR INIT BONUS SALE");
        bonus_pay_id="";

        StatusBonusCard="-1";

        return -3;
    }

    CurrentActiveBonus = retInf->ActiveBonuses;

    int ReturnCodeInf = retInf->ReturnCode;
    double all_bns = retInf->ActiveBonuses; // + ret->HoldedBonuses;
    double active_bns = retInf->ActiveBonuses;

    string status = retInf->status;

    StatusBonusCard = retInf->status;

    //! Новая транзакция продажи
    // В ней будет
    // 1) сумма начисленных бонусов и
    // 2) сумма оплаченных бонусов (если будет списание)

    // Инициализация покупки, присвоение нового ИД
    BNS_RET* ret = CashReg->BNS->exec("sale&init", card, strCheck, Cheque, Cheque->GetDiscountSum());

    ;;Log((char*)Int2Str(ret->errorCode).c_str());
    ;;Log((char*)ret->id.c_str());

    CashReg->AddBonusOperation( ret ) ;

    bonus_pay_id = ret->id;

    // Получаем сумму, которую можно списать в конкретном чеке по данным ПЦ
    // в ней могут не быть учтены акционные бонусы
    // Например, акционные бонусы, которые можно списать только от суммы чека >500р
    //тогда при сумме чека <500р maxSumCanPay вернет сумму без акционных бонусов
    double max_can_pay = ret->maxSumCanPay;

    if (ret->ReturnCode)
    {
        // если ошибка инициализации покупки, то дальше не идем
        ;;Log("ERROR INIT BONUS SALE");
        bonus_pay_id="";

        StatusBonusCard="-1";

        return -2;

    }

    if(isEmptyBonusID(bonus_pay_id))
    {
        // если ошибка инициализации покупки, то дальше не идем
        ;;Log("ERROR: EMPTY ID!");
        bonus_pay_id="";

        StatusBonusCard="-1";

        return -2;

    }


    // Карта добавлена по номеру телефона => только начисление
    if (CustomerByPhone)
    {
        Log("Карта добавлена по номеру телефона. Возможно только начисление бонусов");
        return -1;
    }


    Log("Статус бонусной карты: \""+status+"\"");

    // Карта добавлена по номеру телефона => только начисление
    if (status=="255")
    {
        Log("Статус - Заблокирована!");
        return -1;
    }

    if ((status!="1") && (status!="2") && (status!="3"))
    {
        Log("Статус - Только начисление!");
        return -1;
    }


    // Далее выводим диалог списания бонусов, если это необходимо
    if (chSumma<=0)
    {
        Log("БОНУСЫ:  Слишком маленькая сумма чека! (Должен быть > 1руб.)");
        return -1;
    }

// + dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========
// Минимальная сумма списания бонусов "Спасибо" не должна быть меньше 300 баллов
// Соответственно сумма чека должна быть больше 300 рублей
    if (chSumma<_BONUS_MINIMUM_LIMIT_)
    {
        char tmpstr[1024];
        sprintf(tmpstr, "БОНУСЫ:  Слишком маленькая сумма чека (менее %d руб.)! ",_BONUS_MINIMUM_LIMIT_);
        Log(tmpstr);

        return -1;
    }
// - dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========



//        CashReg->LoggingAction(,ERRORBONUSRETDISC,sum*.01,,0, card, NULL, NULL, NULL, SberbankCard);// *

    //CashReg->AddBankBonusOperation(ret);
//;;Log("- Step 1 -");
    if (!ReturnCodeInf)
    {
    //==================================================================================

	//setlocale( LC_ALL, "ru_RU->koi8-r");



//;;Log("- Step 2 -");


// + dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========
//Окно списания бонусов показываем только если кол-во бонусов >= 300
            //if (active_bns<_BONUS_MINIMUM_LIMIT_)
            if (max_can_pay<_BONUS_MINIMUM_LIMIT_)
            {
                char tmpstr[1024];
                sprintf(tmpstr, "БОНУСЫ:  Слишком мало бонусов (менее %d )! ",_BONUS_MINIMUM_LIMIT_);
                Log(tmpstr);

                return -1;
            }

// Было до 01.03.2015
            //+ dms === 2014-05-08 === Ограничение на показ окна бонусов при оплате по безналу
            // окно показываем только если кол-во бонусов > 20
//            if ((uselimit) && (active_bns<=BANKBONUS_MINIMUM_LIMIT))
//            {
//                Log("БОНУСЫ:  Слишком мало бонусов (<20)! ");
//                return -1;
//            }
            //- dms === 2014-05-08 === /



// - dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========


            double max_bns = GetMaxBonusToPay(Cheque);

            //double max_act_bns = min(max_bns, active_bns);
            double max_act_bns = min(max_bns, max_can_pay);

;;Log("Выведен на экран диалог списания бонусов");

            BankBonus = new BankBonusForm(this);

			QString BonusCard = card;
//;;Log("- Step 3 -");
			QString BonusEvalCount, BonusSumma, BonusStatus, BonusPromoText;

            QString BonusAllCount = W2U(Double2Str(RoundToNearest(all_bns, 0.001)));
            BonusEvalCount = W2U(Double2Str(RoundToNearest(active_bns , 0.001)));

            //string st=trim(ret->CS_S);
            string st=StatusBonusCard;
            QRegExp rn("\\n");
            QRegExp sn("\\s");
            BonusStatus = W2U(st).replace(rn, "");
            BonusStatus = W2U(BonusStatus).replace(sn, "");

            string status = U2W(BonusStatus);
            BonusPromoText = "";
//;;Log("- Step 4 -");
			for(;;)
			{
				BankBonus->CardEdit->setText(BonusCard);
				//BankBonus->EvalCountEdit->setText(GetRounded(active_bns,0.01).c_str());
				//BankBonus->AllCountEdit->setText(GetRounded(all_bns,0.01).c_str());
				BankBonus->EvalCountEdit->setText(GetRounded(max_act_bns,0.01).c_str());
				BankBonus->AllCountEdit->setText(GetRounded(active_bns,0.01).c_str());

				BankBonus->StatusEdit->setText(BonusStatus);
                //BankBonus->InfoLabel->setText(BonusPromoText);

;;Log("Сумма чека: "+Double2Str(chSumma));
;;Log("Всего активных бонусов на карте: "+Double2Str(active_bns) );
;;Log("Максимально возможная сумма списания бонусов: "+Double2Str(max_bns));
;;Log("Доступно для списания: "+Double2Str(max_act_bns) );


			if (BankBonus->exec()==QDialog::Accepted)
				{
                   if((status!="1") && (status!="3"))
                    {
                        ShowMessage("Карта неактивна, списание баллов невозможно !");
                        continue;
                    }

					double val;

					BonusSumma	= BankBonus->SummaEdit->text();

                    // проверка количества
                    val = 0;
					if (!BonusSumma.isEmpty())
					{
						val = BonusSumma.toDouble();
					}

;;Log((char*)("ВВЕДЕНО БОНУСОВ ДЛЯ СПИСАНИЯ: "+Double2Str(val)).c_str() );

                    if (val>0)
                    {

                       if(val>max_act_bns)
                        {
                            ShowMessage("Введенная сумма превышает допустимую сумму для списания баллов!");
                            continue;
                        }

                       if(val>active_bns)
                        {
                            ShowMessage("Введенная сумма превышает сумму активных баллов!");
                            continue;
                        }

                       if(val>chSumma)
                        {
                            ShowMessage("Cумма баллов должна быть меньше суммы чека!");
                            continue;
                        }

                       if(val<.01)
                        {
                            ShowMessage("Слишком маленькая сумма баллов!");
                            continue;
                        }

// + dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========
// с 01.03.2015 действует новая бонусная программа:  сумма использования бонусов >= 300 бонусов
/*
                       if(val<_BONUS_MINIMUM_LIMIT_)
                        {
                            ShowMessage("Сумма бонусов должна быть не меньше "+Double2Str(_BONUS_MINIMUM_LIMIT_)+"!");
                            continue;
                        }
*/
                       if(val<1)
                        {
                            ShowMessage("Сумма бонусов должна быть не меньше 1 !");
                            continue;
                        }

// - dms ===== 2015-03-01 ======= Минимальная сумма списания 300 бонусов ========

//                        Log("3");
//                        Log((char*)Double2Str(val).c_str());
/*
                        CashReg->AddPayment(10,BonusCard.latin1(),&val);
                        Cheque->CertificateSumm = val;
                        ShowGoodsInfo();
*/
                        //ShowMessage(Double2Str(val));
                        //return 0;

// + dms ====== 2021-03-15 ======= При списании более 1000 бонусов - запрашиваем последние 4 цифры номера телефона
// Информация по карте

/*
if (val>=1000)
{
*/
;;Log("+ GetCustomerInfo +");
    BNS_RET* retCustInf = CashReg->BNS->exec("custByCard", card, strCheck);
;;Log("- GetCustomerInfo -");

    if (retCustInf->ReturnCode)
    {
        Log(" -> Ошибка получения номера телефона "+Int2Str(retCustInf->ReturnCode));
//        ShowMessage("Не удалось получить номер телефона клиента. Повторите попытку");
//        continue;
    }

    string enterVal = "";
    string last4dig = "";
    string mobileNumber = retCustInf->mobileNumber;
    if (mobileNumber.length()>5)
    {
        last4dig = mobileNumber.substr(mobileNumber.length() - 5, 4);
    }
    else
    {
        Log(" -> Неправильный номер телефона клиента "+mobileNumber);
        CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,BN_ERRPHONE,val,CashReg->CurrentSaleman->Id,0, card.c_str(), NULL, NULL, mobileNumber.c_str());
//        ShowMessage("Неправильный номер телефона клиента. Обратитесь к администратору");
//        continue;
    }

   // CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,BN_GETPHONE,val,CashReg->CurrentSaleman->Id,0, card.c_str(), NULL, NULL, last4dig.c_str());

    Log(" -> Получен номер телефона ("+mobileNumber+") -> ("+last4dig+")");
/*
    SumForm *dlg = new SumForm(this);
    dlg->FormCaption->setText(W2U("Введите последние 4 цифры номера телефона"));

    dlg->LineEdit->setText(EditLine->text());

    ;;Log(" -> Диалог ввода телефона <- ");
    if (dlg->exec() == QDialog::Accepted)
    {
        enterVal = U2W(dlg->LineEdit->text());

        Log(" -> Введены цифры ("+enterVal+")");

        if (enterVal.length() != 4)
        {
            ShowMessage("Некорректное значение. Нужно указать последние 4 цифры номера телефона");
            continue;
        }

    } else {
        continue;
    }
*/
    CashReg->LoggingAction(CashReg->GetLastCheckNum()+1,BN_CHECKPHONE,val,CashReg->CurrentSaleman->Id,0, card.c_str(), (char*)(enterVal.c_str()), NULL, last4dig.c_str());
/*
    if(last4dig != enterVal)
    {
        ShowMessage("Номера телефонов не совпадают! Обратитесь к администратору");
        continue;
    }
}
*/
// - dms ====== 2021-03-15 ======= При списании более 1000 бонусов - запрашиваем последние 4 цифры номера телефона



        //;;Log("- Step 5 -");
                        string sha1="";
                        //PCX_RET* ret1 = CashReg->WPCX->exec("disc", card, sha1, Cheque, val);
                        BNS_RET* ret1 = CashReg->BNS->exec("sale&disc", card, strCheck, Cheque, 0, val, bonus_pay_id);
        //;;Log("- Step 6 -");

                        CashReg->AddBonusOperation( ret1 ) ;
        //;;Log("- Step 7 -");
                        if (!ret1->ReturnCode)
                        {

                            //!FiscalReg->PrintPCheck("/Kacca/pcx/p", false);

                            //;;Log("+ ProcessBankBonusDiscount +");
                                string BonusCard = card;

                                double a_sum = Cheque->GetSum(); // сумма чека до пересчета

                                CashReg->AddPayment(12,BonusCard,&val);

                                ShowGoodsInfo();

                                double b_sum = Cheque->GetSum(); // сумма чека после пересчета

                                string LogStr= card;
                                LogStr = ">>> BONUS discounts #"+/* LogStr.substr(0,8)+"# ="+*/Double2Str(a_sum-b_sum);
                                Log((char*)LogStr.c_str());

                                //;;Log("- ProcessBankBonusDiscount -");

                                //Result = Double2Int(val*100);

                                Result = Double2Int(val*100);

                                LogStr= card;
                                LogStr = ">>> BONUS Summ Enter #"+LogStr+"# ="+Double2Str(val);
                                Log((char*)LogStr.c_str());


                              // ShowMessage("Отбить чек?");


                        }
                        else
                        {
                            ShowMessage(ret1->errorDescription);
                        }
                    }
                    else
                    {
                        ;;Log(" => Выбрано 0 баллов (Отмена)");
                    }

				}
                else
                {
                    ;;Log(" => Выбрана ОТМЕНА");
                }

				break;
			}

            if (BankBonus!=NULL)
            {
                delete BankBonus;
                BankBonus=NULL;
            }

    }
    else
    {
        if (ret->errorDescription!="")
           ShowMessage(ret->errorDescription);
    }

    //;;Log("- ProcessBankBonusDisc -");
    return Result;

}


int WorkPlace::ProcessBonusDiscountReverse(bool isAnnul)
{
    //;;Log("+ProcessBankBonusDiscountReverse+");

    if(isEmptyBonusID(bonus_pay_id)) return -1;

    string strCheck = Int2Str(CashReg->GetLastCheckNum()+1);

    string card = LLong2Str(CurrentCustomer);

    double sum = Cheque->BonusSumm;

    BNS_RET* ret = CashReg->BNS->exec("sale&del", card, strCheck, Cheque, 0, 0, bonus_pay_id);

    if(isAnnul)
       ret->action = BB_REVERSE_ANNUL;

    CashReg->AddBonusOperation( ret ) ;

    //FiscalReg->PrintPCheck("/Kacca/pcx/p", false);

    //if (WPCX->PCX_Result.ReturnCode>=0)
    {
        ;;Log("+ REVERSE OK +");
        string BonusCard = card;

        CashReg->DeletePayment(12,BonusCard,true);

        ShowGoodsInfo();

        bonus_pay_id = "";

    }

    //;;Log("-ProcessBankBonusDiscountReverse-");
    return 0;
}



int WorkPlace::ProcessBonusFix(long long lcard)
{

;;Log("[FUNC] ProcessBonusFix [START]");

    if(!CashReg->BonusMode(lcard)) return -1;
    if(isEmptyBonusID(bonus_pay_id)) return -1;

    string card = LLong2Str(lcard);

    string disc = "";
    long idcust =0;

    long Result = 0;

    double chSumma = Cheque->GetSum()-1;

    string strCheck = Int2Str(CashReg->GetLastCheckNum()+1);


    QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
    QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
    sm.setFixedSize(250,100);
    msg.setFixedSize(250,100);
    msg.setAlignment( QLabel::AlignCenter );
    msg.setText(W2U("Обработка бонусов..."));
    sm.show();
    thisApp->processEvents();

    // Если транзакция активна - зафиксируем ее
    ;;Log((char*)("bonus_pay_id = '"+bonus_pay_id+"'").c_str());
    ;;Log("  sale&payment ---\\");
    BNS_RET* ret = CashReg->BNS->exec("sale&payment", card, strCheck, Cheque, Cheque->GetSum(), 0, bonus_pay_id);
    ;;Log("  sale&payment ---/");
    CashReg->AddBonusOperation( ret ) ;


    ;;Log("  sale&fix ----\\");
    ret = CashReg->BNS->exec("sale&fix", card, strCheck, Cheque, Cheque->GetSumBonusAdd(), Cheque->GetSumBonusDisc(), bonus_pay_id);
    ;;Log("  sale&fix ----/");
    CashReg->AddBonusOperation( ret ) ;

    ;;Log("  inf after fix ----\\");
    ret = CashReg->BNS->exec("inf", card, strCheck);
    ;;Log("  inf after fix ----/");
    CashReg->AddBonusOperation( ret ) ;

;;Log("[FUNC] ProcessBonusFix [END]");
}

double WorkPlace::GetMaxBonusToPay(FiscalCheque* CurCheque)
{

	if (!CurCheque)
		CurCheque = Cheque;
	if (!CurCheque) return 0;

    double max_bns = 0;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (((g.ItemFlag!=SL) && (g.ItemFlag!=RT)) || (g.ItemCode==TRUNC) || (g.ItemCode==EXCESS_PREPAY)) continue;

        string strLock = g.Lock;
        if (CashReg->GoodsLock(strLock, GL_NoBonus) == GL_NoBonus) continue;

        double cur_sum=0;

        if(g.ItemMinimumPrice>0)
        {

            double delta = g.ItemSum - g.ItemCount * g.ItemMinimumPrice;

            if(delta>0) cur_sum = delta;

        }
        else
        {
            cur_sum = g.ItemSum;
        }


        max_bns+= cur_sum;

    }

    return max_bns;

}


bool WorkPlace::Check100_11()
{
    bool res=false;

    // Проверка только для магазина №100
    int mag = Str2Int(trim(CashReg->GetMagNum()));
    if (mag!=100) return res;


    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;


    bool isGS = false;
    bool isDomGS = false;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
            string str= g.Lock;

           if (CashReg->GoodsLock(str, GL_GastronomOnly) == GL_GastronomOnly)
                isGS=true;
           else
                isDomGS=true;
        }
    }

   // ;;Log("==Check Check100_11==");

    if (
        (CashReg->GetCashRegisterNum()==11) || (CashReg->GetCashRegisterNum()==9)
        || (CashReg->GetCashRegisterNum()==2) || (CashReg->GetCashRegisterNum()==10)
    )
        res = isDomGS;
    else
        res = isGS;

    return res;
}


long long WorkPlace::ScanBonusCard()
{

	long long Res=-1;

	while ((Res==NotCorrect) || (Res==NoPasswd))
	{
        Res = CheckBarcode(Customer);
	}

	return Res;
}


void WorkPlace::ExecBonusAction(FiscalCheque* CurCheque)
{
//;;Log("[ExecBonusAction] START ");

    return; // отключить

    SummaBonusAction = 0;

	if (!CurCheque) return;

    // Акция с 04.09.17 - 10.09.17 - двойные начисления бонусов для покупок от 1000р.

    bool isBonusCard = CashReg->BonusMode(CurrentCustomer);
    if (!isBonusCard) return;

	time_t ltime = time(NULL);
	tm* now = localtime(&ltime);

    if (
    (now->tm_year + 1900 == 2017)
    &&
    (now->tm_mon + 1 == 9)
    &&
    ((now->tm_mday >= 4) && (now->tm_mday <= 10))
    )
    {

        if (CurCheque->GetSum()>=1000)
        {
//;;Log("[ExecBonusAction] IN ACTION");
            for (int i=0;i<CurCheque->GetCount();i++)
            {
                GoodsInfo crGoods = (*CurCheque)[i];

                if ((crGoods.ItemFlag==SL)||(crGoods.ItemFlag==RT))
                {
                  SummaBonusAction +=crGoods.ItemBonusAdd;
                  crGoods.ItemBonusAdd = crGoods.ItemBonusAdd*2;
                }

                CurCheque->SetAt(i,crGoods);

            }
        }
    }
}

bool WorkPlace::isEmptyBonusID(string str)
{
    return str.length()<=0;
}

void WorkPlace::WaitForLoadGoods()
{
	// + dms ==== 2019-09-03 ===== Ждем загрузки товаров =====
    if (CashReg->IsInload())
    {
        ;;Log("[FUNC] Wait for goods loading [START]");

		QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
		QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
		sm.setFixedSize(250,100);
		msg.setFixedSize(250,100);
		msg.setAlignment( QLabel::AlignCenter );
		msg.setText(W2U("Идет обработка..."));
		sm.show();
		thisApp->processEvents();

/*        while(CashReg->IsInload())
        {
            //sleep(1);
            string i = "";if (CashReg->IsInload()) i = "true"; else i = "false";
            Log("IsInload = "+i);
        }
*/
        ;;Log("[FUNC] Wait for goods loading [FINISH]");
    }

    // - dms ==== 2019-09-03 ===== Ждем загрузки товаров =====
}

bool WorkPlace::IsGoods18()
{
    bool res=false;

    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
            string str= g.Lock;

           if (CashReg->GoodsLock(str, GL_Goods18) == GL_Goods18)
           {
                res=true;
                break;
           }
        }
    }

    return res;
}


int WorkPlace::CheckFrigovere()
{
    int res=0;
    int cnt=0;
    double actsum=0;

    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
            string str= g.Lock;

           if (CashReg->GoodsLock(str, GL_ActionFrigovere) == GL_ActionFrigovere)
                actsum+=g.ItemCount*g.ItemFullPrice;
        }
    }

    ;;Log("===Check Frigovere===");


    // условие выдачи подарка по акции
    if (actsum>=1500){
        res=1;
        ;;Log((char*)("===Action Frigovere! ("+Double2Str(actsum)+")===").c_str());
    }


    // C 28/06/2016 подарок выдается за каждый товар (было - за каждые 2шт товара)
    //res = (int)(cnt/2);

    //res = cnt;

    return res;
}


int WorkPlace::IsMask(int GoodCode)
{
    int res = 0;
    //все данные маски ниже многоразовые ШК ставим 2400001323807
    //
    //865201 Маска защитная ANNALIZA прямая ,хлопок 97%, эластан 3%, резинка белая 10мм Россия
    //865202 Маска защитная ANNALIZA прямая, спанбонд,резинка 80мм черная Россия
    //865203 Маска защитная ANNALIZA фигурная, хлопок 68%, п/э 27%, вискоза 3%, эластан 2%, черный,резинка 10мм Р
    //866188 Маска лицевая гигиеническая 5шт Россия
    //865946 Маска лицевая гигиеническая Россия
    //863968 Маска многоразовая Россия

    if (
    (GoodCode == 865201)
    ||
    (GoodCode == 865202)
    ||
    (GoodCode == 865203)
    ||
    (GoodCode == 866188)
    ||
    (GoodCode == 865946)
    ||
    (GoodCode == 863968)
    ) {
        res = 1;
    }

    return res;

}


int WorkPlace::CheckCheeseAction()
{
    int res=0;
    int cnt=0;
    double actsum=0;

    FiscalCheque* CurCheque = Cheque;
	if (!CurCheque) return res;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemFlag==SL)
        {
           if (g.ItemCode == 826823) cnt++;
        }
    }

    ;;Log("===CheckCheeseAction===");


    // условие выдачи подарка по акции
    if (cnt>0){
        ;;Log("===Action CHEESE!====");
    }


    // C 28/06/2016 подарок выдается за каждый товар (было - за каждые 2шт товара)
    //res = (int)(cnt/2);

    res = cnt;

    return res;
}
