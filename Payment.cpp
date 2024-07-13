#include <math.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qheader.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qmessagebox.h>

#include "Payment.h"
#include "Utils.h"
#include "Cash.h"
#include "WorkPlace.h"

enum {ItemNumber=0,ItemSumm=2};


/*
 *  Constructs a ReturnForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PaymentForm::PaymentForm( QWidget* parent ) : QDialog( parent, "PaymentForm", true )
{
	WPlace  =(WorkPlace*) parent;
	CashReg = WPlace->CashReg;

	setName( "PaymentForm" );
	resize( 360, 300 );
	setProperty( "minimumSize", QSize( 360, 300 ) );
	setProperty( "maximumSize", QSize( 360, 300 ) );
	setProperty( "baseSize", QSize( 360, 300 ) );
	setCaption(W2U("APM KACCA"));

	PaymentList = new QListView( this, "PaymentList" );
	PaymentList->addColumn( W2U("СЕРТИФИКАТ"),360*2/3 );
	PaymentList->addColumn( "",0);
	PaymentList->addColumn( W2U("СУММА"),360/3);
	PaymentList->header()->setClickEnabled(false);
	PaymentList->header()->setResizeEnabled(false);
	PaymentList->header()->setMovingEnabled(false);
	PaymentList->setGeometry( QRect( 0, 0, 360, 250 ) );
	PaymentList->setProperty( "selectionMode", (int)QListView::Single );
	PaymentList->setProperty( "allColumnsShowFocus", QVariant( true, 0 ) );
	PaymentList->setHScrollBarMode(QScrollView::AlwaysOff);
	PaymentList->setSorting(-1);

	btnAdd = new QPushButton( this, "btnAdd" );
	btnAdd->setGeometry( QRect( 20, 260, 93, 30 ) );
	btnAdd->setProperty( "text", W2U("Добавить") );

	btnDelete = new QPushButton( this, "btnDelete" );
	btnDelete->setGeometry( QRect( 135, 260, 93, 30 ) );
	btnDelete->setProperty( "text", W2U("Удалить") );

	btnOk = new QPushButton( this, "btnOk" );
	btnOk->setGeometry( QRect( 255, 260, 93, 30 ) );
	btnOk->setProperty( "text", W2U("ОК") );

//	btnCancel = new QPushButton( this, "btnCancel" );
//	btnCancel->setGeometry( QRect( 80, 320, 93, 26 ) );
//	btnCancel->setProperty( "text", W2U("Отмена") );

	// signals and slots connections
	connect( btnAdd, SIGNAL( clicked() ), this, SLOT(btnAddClicked() ) );
	connect( btnDelete, SIGNAL( clicked() ), this, SLOT(btnDeleteClicked() ) );
	connect( btnOk, SIGNAL( clicked() ), this, SLOT(btnOkClicked() ) );

	ShowPayment();
}

/*
 *  Destroys the object and frees any allocated resources
 */
PaymentForm::~PaymentForm()
{
	// no need to delete child widgets, Qt does it all for us
}

#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qlabel.h>
#include <qvalidator.h>
#include <string>

#include "Sum.h"
#include "Utils.h"
#include <stdio.h>
#include <stdlib.h>

void PaymentForm::btnAddClicked(void)
{
    int TypeCert = CD_CERTIFICATE;

	string discount;
	long long number=0;
	string StrNumber = "";
	double Summa;
	bool oldscannersuspend;

	oldscannersuspend=WPlace->Scanner->suspend;
	WPlace->Scanner->suspend = false;
    WPlace->SearchCertificate(FindMoney,&discount,&number,&Summa);
	WPlace->Scanner->suspend = oldscannersuspend;

    StrNumber = LLong2Str(number);

    CardInfoStruct CardInfo;
    if (CashReg->FindCardInRangeTable(&CardInfo, number))
    {
        if ( CardInfo.CardType == CD_PREPAY_CERTIFICATE) TypeCert = CD_PREPAY_CERTIFICATE;
    }

    if (number)
    {
        switch(CashReg->CheckPayment(TypeCert,StrNumber,discount,&Summa,WPlace->Cheque->GetCalcSum()))
        {
            case 0: //Все нормально
                switch(CashReg->SQLGetCheckInfo->CheckCertificate(number, WPlace->CurrentCustomer, true))
                {
                    case 0:
                        CashReg->AddPayment(TypeCert,StrNumber,&Summa);
                        break;

                    case 1:
                        CashReg->ShowMessage("Сертификат заблокирован");
                        break;

                    case 2:
                        CashReg->ShowMessage("Неправильный номер сертификата");
                        break;

                    case 3:
                        CashReg->ShowMessage("Нет данных по сертификату");
                        break;

                    case 4:
                        CashReg->ShowMessage("Неправильная дисконтная карта\n\nСертификат привязан к другой дисконтной карте");
                        break;

                    case -1: // ошибка связи с базой
                        CashReg->ShowMessage("Невозможно проверить сертификат, сеть недоступна");
                        break;
                }

                break;
            case 1:
                CashReg->ShowMessage("Сертификат уже введен");
                break;
            case 2:
                CashReg->ShowMessage("Неправильный номер сертификата");
                break;

        }
    }

	ShowPayment();

	PaymentList->setFocus();
}

void PaymentForm::btnDeleteClicked(void)
{
	QListViewItem *item;
	item = PaymentList->selectedItem();
	if(item != NULL)
	{
		CashReg->DeletePayment(10,U2W(item->text(ItemNumber)));
	}
	ShowPayment();
	PaymentList->setFocus();
}

void PaymentForm::btnOkClicked(void)
{
	WPlace->ProcessDiscount();
	accept();
}

void PaymentForm::ShowPayment(void)
{
	Payment.clear();
	PaymentList->clear();

	CashReg->LoadPayment(&Payment);

	QListViewItem *item;
	unsigned int i;
	if (Payment.size()>0)
		for(i=0;i<Payment.size();i++)
		{
			item = new QListViewItem(PaymentList);
			item->setText(ItemNumber,Payment[i].Number.c_str());
			item->setText(ItemSumm  ,GetRounded(Payment[i].Summa, 0.01).c_str());
		}

	PaymentList->setFocus();

	PaymentList->setSelected(PaymentList->firstChild(),true);
}
