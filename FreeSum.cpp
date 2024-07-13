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

#include "FreeSum.h"

#include "Utils.h"
#include "PriceSel.h"
#include "ExtControls.h"


/*
 *  Constructs a FreeSumForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
FreeSumForm::FreeSumForm( QWidget* parent) : QDialog( parent, "FreeSumForm", true )
{
    valid = false;

	setName( "FreeSumForm" );
	resize( 500, 240 );
	setProperty( "minimumSize", QSize( 500, 240 ) );
	setProperty( "maximumSize", QSize( 500, 240 ) );
	setProperty( "baseSize", QSize( 500, 240 ) );
	setCaption(W2U("APM KACCA"));

	QFont Form_font(font());
	Form_font.setPointSize(12);
	setFont(Form_font);

// Заголовок

	FormLabel = new QColorLabel(this, "FormLabel", darkBlue );
	FormLabel->setFixedSize(480,30);
	FormLabel->move(10,3);

    QFont FormLabel_font( font() );
	FormLabel_font.setPointSize( 12 );
	FormLabel_font.setBold(true);
	FormLabel->setFont( FormLabel_font );

	FormLabel->setProperty( "text", W2U( "Ввод свободной суммы" ) );
	FormLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );


// Поле ввода "Наименование"
	BarcodeLabel = new QLabel( this, "BarcodeLabel" );
	BarcodeLabel->setGeometry( QRect( 10, 35, 100, 25 ) );
	BarcodeLabel->setText(W2U("Штрих-код:"));

	InfoLabel = new QColorLabel( this, "InfoLabel", red );
	InfoLabel->setGeometry( QRect( 5, 220, 300, 25 ) );

    QFont InfoLabel_font( font() );
	InfoLabel_font.setPointSize( 10 );
	InfoLabel->setFont( InfoLabel_font );

	InfoLabel->setText(W2U("Штрих-код не считан!"));


	vector<int> ScanCodesFlow,PrefixSeq,PostfixSeq,Codes;

	Codes.insert(Codes.end(),4148);
	Codes.insert(Codes.end(),47);

	BarcodeEdit = new ExtQLineEdit( this, "BarcodeLine", Codes, PrefixSeq, PostfixSeq );
	BarcodeEdit->setGeometry( QRect( 130, 35, 200, 25 ) );
	BarcodeEdit->setProperty( "frameShape", (int)QLabel::Panel );
	BarcodeEdit->setProperty( "frameShadow", (int)QLabel::Sunken );
	BarcodeEdit->setProperty( "text", tr( "" ) );
	BarcodeEdit->setProperty( "alignment", int( QLabel::AlignCenter ) );

	BarcodeEdit->setReadOnly(true);


// Поле ввода "Наименование"
	NameLabel = new QLabel( this, "NameLabel" );
	NameLabel->setGeometry( QRect( 10, 70, 120, 25 ) );
	NameLabel->setText(W2U("Наименование:"));

	NameEdit = new QNameLineEdit( this, "NameLine" );
	NameEdit->setGeometry( QRect( 130, 70, 360, 25 ) );

// Поле ввода "Количество"
	CountLabel = new QLabel( this, "CountLabel" );
	CountLabel->setGeometry( QRect( 10, 105,120, 25 ) );
	CountLabel->setText(W2U("Количество:"));

	CountEdit = new QLineEdit( this, "CountEdit" );
	CountEdit->setGeometry( QRect( 130, 105, 140, 25 ) );

	double_CountValid = new QDoubleValidator(CountEdit);
	double_CountValid->setRange(1,1000000000,3);
	CountEdit->setValidator(double_CountValid);

// Поле ввода "Цена"
	PriceLabel = new QLabel( this, "PriceLabel" );
	PriceLabel->setGeometry( QRect( 290,105, 56, 25 ) );
	PriceLabel->setText(W2U("Цена:"));

	PriceEdit = new QLineEdit( this, "PriceEdit" );
	PriceEdit->setGeometry( QRect( 350, 105, 140, 25 ) );

	double_PriceValid = new QDoubleValidator(PriceEdit);
	double_PriceValid->setRange(1,1000000000,2);

	PriceEdit->setValidator(double_PriceValid);

// Надпись "Сумма"
	SummaLabel = new QLabel( this, "SummaLabel" );

	SummaLabel->setFixedSize(150,30);
	SummaLabel->move(340,140);

	QFont summa_font( font() );
	summa_font.setPointSize( 14 );
	summa_font.setBold(true);
	SummaLabel->setFont( summa_font );

	SummaLabel->setProperty( "text", "0.00" );
	SummaLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

// флажок "Весовой товар"

	QFont font10( font() );
	font10.setPointSize( 10 );

	GType = new QCheckBox( this, "GType" );
	GType->setGeometry( QRect( 40, 135, 270, 20 ) );
	GType->setFont( font10 );
	GType->setText(W2U("Весовой товар"));

// флажок "Не предоставлять скидки"
	NoDiscount = new QCheckBox( this, "NoDiscount" );
	NoDiscount->setGeometry( QRect( 40, 160, 270, 20 ) );
	NoDiscount->setFont( font10 );
	NoDiscount->setText(W2U("Не предоставлять скидки"));


// Кнопки
	AddButton = new QPushButton( this, "AddButton" );
	AddButton->setGeometry( QRect( 150, 185, 200, 40 ) );
	AddButton->setText(W2U("Добавить"));

	// signals and slots connections
	connect( AddButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	//connect( NameEdit, SIGNAL( returnPressed() ), this, SLOT( accept() ) );

    connect( CountEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( onCountChange(const QString &) ) );
    connect( PriceEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( updateSumInfo(const QString &) ) );
    connect( BarcodeEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( onBarcodeChange(const QString &) ) );
    connect( NoDiscount, SIGNAL( stateChanged(int) ), this, SLOT( onCheckedNoDiscount() ) );

//    connect( BarcodeEdit, SIGNAL(barcodeFound()), this, SLOT(ProcessBarcode()));
//    connect( NameEdit, SIGNAL(barcodeFound()), this, SLOT(ProcessBarcode()));

	NameEdit->setFocus();
}

/*
 *  Destroys the object and frees any allocated resources
 */
FreeSumForm::~FreeSumForm()
{
    // no need to delete child widgets, Qt does it all for us

	delete double_CountValid;
	delete double_PriceValid;
}

void FreeSumForm::updateSumInfo(const QString &)
{
    double cnt, price, sum;
    QString strCount, strPrice;

    strCount = CountEdit->text();
    strPrice = PriceEdit->text();

    cnt = price = sum = 0;

    if(!strCount.isEmpty())
    {
        cnt = strCount.toDouble();
    }

    if(!strPrice.isEmpty())
    {
        price = strPrice.toDouble();
    }

    sum = cnt*price;

    SummaLabel->setText(W2U(GetRounded(sum,0.01)));

}

void FreeSumForm::onBarcodeChange(const QString & Str)
{
    if (BarcodeEdit->text().isEmpty())
        InfoLabel->setText(W2U("Штрих-код не считан!"));
}

void FreeSumForm::onCountChange(const QString & Str)
{
    QString strCount = CountEdit->text();

    double cnt = 0;
    if (!strCount.isEmpty())
    {
        cnt = strCount.toDouble();
    }

    GType->setChecked( (int)cnt!=cnt );

    updateSumInfo(Str);
}

void FreeSumForm::onCheckedNoDiscount()
{
    QString strName = NameEdit->text();

    if (NoDiscount->isChecked())
    {
        if (strName.left(1)!="*")
        {
            NameEdit->setText("*"+strName);
        }
    }
    else
    {
        if (strName.left(1)=="*")
        {
            NameEdit->setText(strName.remove(0,1));
        }
    }

}

void FreeSumForm::ProcessBarcode(void)
{
	//ShowMessage(NULL,"SCANNER EVENT");
}
