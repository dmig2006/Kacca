/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qcheckbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qvalidator.h>
#include <qapplication.h>
#include <stdio.h>

#include <string>
#include <vector>

using namespace std;

#include "Search.h"

#include "Utils.h"
#include "PriceSel.h"
#include "ExtControls.h"


/*
 *  Constructs a SearchForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SearchForm::SearchForm( QWidget* parent) : QDialog( parent, "SearchForm", true )
{
	setName( "SearchForm" );
	resize( 450, 320 );
	setProperty( "minimumSize", QSize( 450, 320 ) );
	setProperty( "maximumSize", QSize( 450, 320 ) );
	setProperty( "baseSize", QSize( 450, 320 ) );
	setCaption(W2U("APM KACCA"));

	QFont Form_font(font());
	Form_font.setPointSize(12);
	setFont(Form_font);

	CodeLabel = new QLabel( this, "CodeLabel" );
	CodeLabel->setGeometry( QRect( 10, 20, 100, 30 ) );
	CodeLabel->setText(W2U("Код"));

	CodeEdit = new QLineEdit( this, "CodeEdit" );
	CodeEdit->setGeometry( QRect( 120, 20, 310, 27 ) );

	int_Valid = new QIntValidator(CodeEdit);
	int_Valid->setRange(1,1000000);
	CodeEdit->setValidator(int_Valid);

	BarcodeLabel = new QLabel( this, "BarcodeLabel" );
	BarcodeLabel->setGeometry( QRect( 10, 70, 90, 30 ) );
	BarcodeLabel->setText(W2U("Штрих-код"));

	BarcodeEdit = new QLineEdit( this, "BarcodeEdit" );
	BarcodeEdit->setGeometry( QRect( 120, 70, 310, 27 ) );

	NameLabel = new QLabel( this, "NameLabel" );
	NameLabel->setGeometry( QRect( 10, 120, 80, 30 ) );
	NameLabel->setText(W2U("Название"));

	NameEdit = new QNameLineEdit( this, "NameLine" );
	NameEdit->setGeometry( QRect( 120, 120, 310, 30 ) );

	LookInside = new QCheckBox( this, "LookInside" );
	LookInside->setGeometry( QRect( 160, 160, 170, 20 ) );
	QFont LookInside_font( font() );
	LookInside_font.setPointSize( 10 );
	LookInside->setFont( LookInside_font );
	LookInside->setText(W2U("Поиск внутри названия"));

	PriceLabel = new QLabel( this, "PriceLabel" );
	PriceLabel->setGeometry( QRect( 10, 190, 50, 30 ) );
	PriceLabel->setText(W2U("Цена"));

	PriceEdit = new QLineEdit( this, "PriceEdit" );
	PriceEdit->setGeometry( QRect( 120, 190, 310, 30 ) );

	double_Valid = new QDoubleValidator(PriceEdit);
	double_Valid->setRange(1,1000000000,2);
	PriceEdit->setValidator(double_Valid);

	SearchButton = new QPushButton( this, "SearchButton" );
	SearchButton->setGeometry( QRect( 160, 250, 150, 40 ) );
	SearchButton->setText(W2U("Искать"));

	// signals and slots connections
	connect( SearchButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( NameEdit, SIGNAL( returnPressed() ), this, SLOT( accept() ) );

	NameEdit->setFocus();
}

/*
 *  Destroys the object and frees any allocated resources
 */
SearchForm::~SearchForm()
{
    // no need to delete child widgets, Qt does it all for us

	delete int_Valid;
	delete double_Valid;
}

//***********************************************************************
//enum{ChequeList,PriceList,GroupList,GoodsList};//column's numbers

PriceCheck::PriceCheck( QWidget* parent) : QDialog( parent, "PriceCheck", true )
{
//	GoodsInfo CurrentGoods;
//	FiscalCheque FoundGoods;
//	GoodsInfo tmpGoods;
//	FoundGoods.Clear();
//	string Barcode;
	//WorkPlace* WPlace;

	WPlace=(WorkPlace*)parent;


	WPlace->wbarcode=0;

	setName( "PriceCheck" );
	resize( 620, 380 );
	setProperty( "minimumSize", QSize( 620, 380 ) );
	setProperty( "maximumSize", QSize( 620, 380 ) );
	setProperty( "baseSize", QSize( 620, 380 ) );
	setProperty( "caption", W2U( "                                              nouck" ) );

	QFont Form_font(font());
	Form_font.setPointSize(10);
	//Form_font.setBold(true);
	setFont(Form_font);

	QFont Line_font(font());
	Line_font.setPointSize(8);

	QFont Label_font(font());
	Label_font.setPointSize(20);
	Label_font.setBold(true);

	QColor LabelColor;
	if (WPlace->BW)
		LabelColor=black;
	else
		LabelColor=darkRed;

	SearchLabel = new QColorLabel( this, "SearchLabel", LabelColor );
	SearchLabel->setGeometry( QRect( 180, 2, 420, 16 ) );
	SearchLabel->setFont(Line_font);
	SearchLabel->setText(W2U("ПОИСК                                                       ПО КОДУ - /.  ПО ШТРИХ-КОДУ - F5."));

	vector<int> ScanCodesFlow,PrefixSeq,PostfixSeq,Codes;

	Codes.insert(Codes.end(),4148);
	Codes.insert(Codes.end(),47);

	SearchEdit = new ExtQLineEdit( this, "SearchEdit", Codes, PrefixSeq, PostfixSeq );
	SearchEdit->setGeometry( QRect( 232, 1, 135, 16 ) );
	SearchEdit->setProperty( "frameShape", (int)QLabel::Panel );
	SearchEdit->setProperty( "frameShadow", (int)QLabel::Sunken );


	CodeLabel = new QLabel( this, "CodeLabel" );
	CodeLabel->setGeometry( QRect( 0, 41, 20, 10 ) );//( 0, 41, 20, 10 )
	CodeLabel->setFont(Line_font);
	CodeLabel->setText(W2U("Код"));

	CodeLine = new QLabel( this, "CodeLine" );
	CodeLine->setGeometry( QRect( 36, 37, 80, 18 ) );//( 20, 37, 95, 18 )
	CodeLine->setProperty( "frameShape", (int)QLabel::Panel );
	CodeLine->setProperty( "frameShadow", (int)QLabel::Sunken );
/////////////////////
	NameLabel = new QLabel( this, "NameLabel" );
	NameLabel->setGeometry( QRect( 0, 22, 26, 10 ) );//( 0,22 ,26 ,10  )
	NameLabel->setFont(Line_font);
	NameLabel->setText(W2U("Наим"));

	NameLine = new QLabel( this, "NameLine" );
	NameLine->setGeometry( QRect( 28, 18, 598, 18 ) );//( 28,18 ,498 ,18  )
	NameLine->setProperty( "frameShape", (int)QLabel::Panel );
	NameLine->setProperty( "frameShadow", (int)QLabel::Sunken );

	QuantLabel = new QLabel( this, "QuantLabel" );
	QuantLabel->setGeometry( QRect( 139, 41, 40, 10 ) );//( 119, 41, 40, 10 )
	QuantLabel->setFont(Line_font);
	QuantLabel->setText(W2U("Кол-во"));

	SummLabel = new QLabel( this , "SummLabel" );
	SummLabel->setGeometry( QRect( 0, 60, 40, 10 ) );//( 0, 60, 40, 10 )
	SummLabel->setFont(Line_font);
	SummLabel->setText(W2U("Сумма"));


	QuantLine = new QLabel( this, "QuantLine" );
	QuantLine->setGeometry( QRect( 187, 37, 68, 18 ) );//( 157, 37, 77, 18 )
	QuantLine->setProperty( "frameShape", (int)QLabel::Panel );
	QuantLine->setProperty( "frameShadow", (int)QLabel::Sunken );

	SummLine = new QLabel( this , "SummLine" );
	SummLine->setGeometry( QRect( 36, 56, 80, 18 ) );//( 38, 56, 80, 18 )
	SummLine->setProperty( "frameShape", (int)QLabel::Panel );
	SummLine->setProperty( "frameShadow", (int)QLabel::Sunken );

	RestLabel = new QLabel( this, "RestLabel" );
	RestLabel->setGeometry( QRect( 141, 60, 45, 10 ) );//( 119, 60, 45, 10 )
	RestLabel->setFont(Line_font);
	RestLabel->setText(W2U("Остаток"));

	RestLine = new QLabel( this, "RestLine" );
	RestLine->setGeometry( QRect( 187, 56, 68, 18 ) );//( 165, 56, 68, 18 )
	RestLine->setProperty( "frameShape", (int)QLabel::Panel );
	RestLine->setProperty( "frameShadow", (int)QLabel::Sunken );

	GoodsList=NULL;
	DiscList=NULL;


	TextLabel1 = new QColorLabel( this, "TextLabel1", LabelColor );
	TextLabel1->setFixedSize(200, 20);
	TextLabel1->move(20, 350);
//		setGeometry( QRect( 0, 380, 220, 40 ) );
	TextLabel1->setFont( Label_font );
	TextLabel1->setProperty( "text", W2U( "!!! НЕ ЧЕК !!!" ) );
	TextLabel1->setProperty( "alignment", int( QLabel::AlignCenter ) );

	SearchEdit->setFocus();

	connect( SearchEdit, SIGNAL(keyPressed(int)), this, SLOT(ProcessKey(int)));
}

PriceCheck::~PriceCheck()
{
	//CashReg->SQLServer->exit();
}


void PriceCheck::keyPressEvent(QKeyEvent* e)//analogously
{
	if ((e->key()==Qt::Key_Escape))
	{   QCloseEvent ev = QCloseEvent();
	    closeEvent(&ev);
	}
	else
		QDialog::keyPressEvent(e);
}


void PriceCheck::closeEvent(QCloseEvent* e)//close saleman's workplace
{
		//if (SearchEdit->text().isEmpty())
		//	{ShowMessage(NULL,"not empty");}
		//else
	QDialog::closeEvent(e);
}

void PriceCheck::ProcessBarcode(void)
{
//	ShowMessage(NULL,"SCANNER EVENT");
}


void PriceCheck::ProcessKey(int key)
{
	//ShowMessage(NULL,Int2Str(key));
    if (key==12345)
    {
        WPlace->wbarcode=0;
        WPlace->StartPriceChecker(key);
        SearchEdit->clear();
        return;
    }


	if (key!=41480 && Clear()==1)
		SearchEdit->clear();
	if ((key!=47) && (key!=4148))
		return;
	if (SearchEdit->text().isEmpty()){
		switch(key)
		{
		case 47: ShowMessage(NULL,("Введите код товара"));break;
		case 4148: ShowMessage(NULL,("Введите штрих-код товара"));break;
		default: ShowMessage(NULL,("Введите параметры поиска"));
		}
		return;
	}
	WPlace->wbarcode=0;
	WPlace->StartPriceChecker(key);
}

void PriceCheck::ShowPriceList(FiscalCheque *price)//show goods according to given check
{
	//ShowMessage(NULL,WPlace->DiscInfo[0].description);


	if (price->GetCount()==0){
		ShowMessage(NULL,"Данные по товару не найдены");
		return;
	}
	if(GoodsList==NULL)
	{
		GoodsList = new QListView( this, "GoodsList");
		QFont GoodsList_font(  GoodsList->font() );
		GoodsList_font.setPointSize( 8 );
		GoodsList->setFont( GoodsList_font );
		GoodsList->addColumn( W2U( "Штрих-код" ),100 );
		GoodsList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->addColumn( W2U( "Кол-во" ),49 );
		GoodsList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->addColumn( W2U( "Цена" ),59);
		GoodsList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->addColumn( W2U( "Фикс.Цена" ),59);
		GoodsList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->setGeometry( QRect( 1, 75, 270, 257) );

		GoodsList->setProperty( "selectionMode", (int)QListView::NoSelection );
		GoodsList->setProperty( "allColumnsShowFocus", QVariant( true, 0 ) );
		GoodsList->setSorting(-1);

		GoodsList->setShowSortIndicator(false);
		GoodsList->setHScrollBarMode(QListView::AlwaysOff);

		if((*price).GetCount()==0){
			ShowMessage(NULL,"Товар не найден!!!");
			return;
		}
		if (!NameLine->text().isEmpty())
			NameLine->setText(W2U((*price)[0].ItemName));
		if ((*price)[0].ItemCode!=0)
			CodeLine->setText(Int2Str((*price)[0].ItemCode).c_str());

		for (unsigned int i=(*price).GetCount()-1;(i>=0)&&(i<(*price).GetCount());i--)
		{
			itm=new QListViewItem(GoodsList);
			itm->setSelectable(false);
			itm->setText(0,((*price)[i].ItemBarcode).c_str());
			itm->setText(1,GetRounded((*price)[i].ItemCount,0.001).c_str());
			itm->setText(2,GetRounded((*price)[i].ItemPrice,0.01).c_str());
			itm->setText(3,(*price)[i].ItemFixedPrice==0.0 ? "" : GetRounded((*price)[i].ItemFixedPrice,0.01).c_str());
		}
		GoodsList->show();
	}
//***********************************************
	if(DiscList==NULL){
		DiscList = new QListView( this, "DiscList");
		QFont DiscList_font(  DiscList->font() );
		DiscList_font.setPointSize( 8 );
		DiscList->setFont( DiscList_font );
		DiscList->addColumn( W2U( "   Скидка" ),265 );
		DiscList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		DiscList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		DiscList->addColumn( W2U( " Состояние" ),80 );
		DiscList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		DiscList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		DiscList->setGeometry( QRect( 275, 37, 370, 345) );
		DiscList->setProperty( "selectionMode", (int)QListView::NoSelection );
		DiscList->setProperty( "allColumnsShowFocus", QVariant( true, 0 ) );
		DiscList->setSorting(-1);

/*		string discs[]={"Ветеранская","Макс. цена","Специальная","Фиксир. цена","Рыбный день","МРСК","Новогодняя №1",
			"Новогодняя №2","Банковские карты","По сумме(Грошель)","Количественная","Зеленый дуб","Ученый кот","Золотая цепь","МРСК",
			"Паритет","АСПЭК","Оптима","Айкай","Фармакон"};
*/		string status;
		//ShowMessage(NULL,(*price)[0].ItemDiscountsDesc);
		for (int j=((*price)[0].ItemDiscountsDesc.length()); j>-1; j--){
			for (int c=0; c<WPlace->DiscInfo.size();c++){
				//if (j==DiscInfo[c].type)
				if(WPlace->DiscInfo[c].type==j){
					ditm=new QListViewItem(DiscList);
					ditm->setSelectable(false);
					//ditm->setText(0,W2U(discs[j]));
					ditm->setText(0,W2U(WPlace->DiscInfo[c].description));
					switch((*price)[0].ItemDiscountsDesc[j-1]){
						case '.':status="Всегда";break;
						case 'X':status="Разрешена";break;
						case 'Z':status="Запрещена";break;
						default:status="Не определена";
					}
					ditm->setText(1,W2U(status));
				}
			}
		}
		DiscList->show();
	}
}

void PriceCheck::ShowBonusList(FiscalCheque *price)//show goods according to given check
{
	//ShowMessage(NULL,WPlace->DiscInfo[0].description);

	if (price->GetCount()==0){
		ShowMessage(NULL,"Данные по товару не найдены");
		return;
	}
	if(GoodsList==NULL)
	{
		GoodsList = new QListView( this, "GoodsList");
		QFont GoodsList_font(  GoodsList->font() );
		GoodsList_font.setPointSize( 8 );
		GoodsList->setFont( GoodsList_font );
		GoodsList->addColumn( W2U( "Параметр" ),170 );
		GoodsList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->addColumn( W2U( "Значение" ),100 );
		GoodsList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		GoodsList->setGeometry( QRect( 1, 75, 270, 257) );

		GoodsList->setProperty( "selectionMode", (int)QListView::NoSelection );
		GoodsList->setProperty( "allColumnsShowFocus", QVariant( true, 0 ) );
		GoodsList->setSorting(-1);

		GoodsList->setShowSortIndicator(false);
		GoodsList->setHScrollBarMode(QListView::AlwaysOff);

		if((*price).GetCount()==0){
			ShowMessage(NULL,"Карта не найдена!!!");
			return;
		}
		if (!NameLine->text().isEmpty())
			NameLine->setText(W2U((*price)[0].ItemName));
		if ((*price)[0].ItemCode!=0)
			CodeLine->setText(Int2Str((*price)[0].ItemCode).c_str());

		for (unsigned int i=(*price).GetCount()-1;(i>=0)&&(i<(*price).GetCount());i--)
		{
			itm=new QListViewItem(GoodsList);
			itm->setSelectable(false);
			itm->setText(0,W2U( (*price)[i].ItemBarcode ));
			itm->setText(1,W2U( (*price)[i].ItemSecondBarcode ));
		}
		GoodsList->show();
	}
//***********************************************
	if(DiscList==NULL){
		DiscList = new QListView( this, "DiscList");
		QFont DiscList_font(  DiscList->font() );
		DiscList_font.setPointSize( 8 );
		DiscList->setFont( DiscList_font );
		DiscList->addColumn( W2U( "   Скидка" ),265 );
		DiscList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		DiscList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		DiscList->addColumn( W2U( " Состояние" ),80 );
		DiscList->header()->setClickEnabled( false, GoodsList->header()->count() - 1 );
		DiscList->header()->setResizeEnabled( false, GoodsList->header()->count() - 1 );
		DiscList->setGeometry( QRect( 275, 37, 370, 345) );
		DiscList->setProperty( "selectionMode", (int)QListView::NoSelection );
		DiscList->setProperty( "allColumnsShowFocus", QVariant( true, 0 ) );
		DiscList->setSorting(-1);

		string status;

		string curDiscount = (*price)[0].ItemDiscountsDesc;


		for (int j=curDiscount.length(); j>-1; j--)
		{

            char curD = curDiscount[j-1];

            if (curD == 'X') continue;

			for (int c=0; c<WPlace->DiscInfo.size();c++)
			{
				if(WPlace->DiscInfo[c].type==j)
				{
					ditm=new QListViewItem(DiscList);
					ditm->setSelectable(false);
					//ditm->setText(0,W2U(discs[j]));
					ditm->setText(0,W2U(WPlace->DiscInfo[c].description));
                    switch(curD)
                    {
                        case '.':status= GetDiscStr(curD, WPlace->DiscInfo[c]) ;break;
                        case 'X':status="-";break;
                        case 'Z':status="Запрещена";break;
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':status= GetDiscStr(curD, WPlace->DiscInfo[c]) ;break;
                        default:status=curD;
                    }
					ditm->setText(1,W2U(status));
				}
			}
		}

		DiscList->show();
	}
}

string PriceCheck::GetDiscStr(char curD, DiscountInfo tmpDscInf)
{
    string resStr = GetRounded(tmpDscInf.percent,1);
    if (tmpDscInf.mode == 7)
    {
       // Настроенные скидки
       string strDisc = tmpDscInf.fullpercent;
       string str = "";
       str.push_back(curD);
       str += ":";
       int begpos = strDisc.find(str);
       if (begpos!=string::npos)
       {

           string tmpstr = strDisc.substr(begpos+str.length(), string::npos);
           int lastpos = tmpstr.find(";");
           if (lastpos!=string::npos)
           {
                resStr = tmpstr.substr(0, lastpos);
           }
       }
    }
    return resStr;
}

int PriceCheck::Clear()
{
	int ok=0;
	if (!CodeLine->text().isEmpty() || !NameLine->text().isEmpty()){
		ok=1;
		CodeLine->clear();
		NameLine->clear();
		RestLine->clear();
	}
	QuantLine->clear();
	SummLine->clear();
	if(GoodsList!=NULL) {
		delete GoodsList;
		GoodsList=NULL;
		ok=1;
	}
	if(DiscList!=NULL) {
		delete DiscList;
		DiscList=NULL;
		ok=1;
	}
	return ok;
}
void PriceCheck::GetOstTov(unsigned long code)
{
	//RestLine->setText(GetRounded(CashReg->SQLServer->GetOstTov(Code),0.001).c_str());

}
