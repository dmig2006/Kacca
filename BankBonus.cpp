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

#include "BankBonus.h"

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
BankBonusForm::BankBonusForm( QWidget* parent) : QDialog( parent, "BankBonusForm", true )
{
    valid = false;

	setName( "BankBonusForm" );
	resize( 500, 240 );
	setProperty( "minimumSize", QSize( 500, 285 ) );
	setProperty( "maximumSize", QSize( 500, 285 ) );
	setProperty( "baseSize", QSize( 500, 290 ) );
	setCaption(W2U("APM KACCA"));

	QFont Form_font(font());
	Form_font.setPointSize(12);
	setFont(Form_font);

// Заголовок

	FormLabel = new QColorLabel(this, "FormLabel", darkBlue );
	FormLabel->setFixedSize(480,30);
	FormLabel->move(5,3);

    QFont FormLabel_font( font() );
	FormLabel_font.setPointSize( 12 );
	FormLabel_font.setBold(true);
	FormLabel->setFont( FormLabel_font );

	FormLabel->setProperty( "text", W2U( "БОНУСЫ" ) );
	FormLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Промо-текст
	InfoLabel = new QColorLabel( this, "InfoLabel", darkRed );
	InfoLabel->setGeometry( QRect( 5, 259, 490, 26 ) );


    QFont InfoLabel_font( font() );
	InfoLabel_font.setPointSize( 14 );
	InfoLabel_font.setBold(true);

	InfoLabel->setFont( InfoLabel_font );

	InfoLabel->setProperty("text",W2U("Желаете оплатить бонусами ? "));
	InfoLabel->setProperty( "frameShape", (int)QLabel::Panel );
	InfoLabel->setProperty( "frameShadow", (int)QLabel::Sunken );

	InfoLabel->setProperty( "alignment", int( QLabel::AlignLeft ) );


/*
// Edit current discount card number
	MDiscCard = new QColorLabel(DiscInfoBox, "MDiscCard", DiscColor );
	MDiscCard->setFixedSize(200, 22);//(175, 25);
	MDiscCard->move( CurPos, 3);
	QFont MDiscCard_font( font() );
	MDiscCard_font.setPointSize( 14 );
	//MDiscCard_font.setBold(true);
	MDiscCard->setFont( MDiscCard_font );
	MDiscCard->setProperty( "frameShape", (int)QLabel::Panel );
	MDiscCard->setProperty( "frameShadow", (int)QLabel::Sunken );
	MDiscCard->setProperty( "text", tr( "" ) );
	MDiscCard->setProperty( "alignment", int( QLabel::AlignCenter ) );
*/



// Поле ввода "Карта"
	CardLabel = new QLabel( this, "CardLabel" );
	CardLabel->setGeometry( QRect(  10, 40, 100, 25 ) );
	CardLabel->setText(W2U("Карта"));

	CardEdit = new QColorLabel( this, "CardEdit", black );
	CardEdit->setGeometry( QRect( 245, 40, 250, 25 ) );

	CardEdit->setProperty( "frameShape", (int)QLabel::Panel );
	CardEdit->setProperty( "frameShadow", (int)QLabel::Sunken );

	CardEdit->setProperty( "alignment", int( QLabel::AlignCenter ) );

	//QString qcard = card;
    //CardEdit->setText(qcard);

// Поле ввода "Статус"
	StatusLabel = new QLabel( this, "StatusLabel" );
	StatusLabel->setGeometry( QRect( 10, 70, 120, 25 ) );
	StatusLabel->setText(W2U("Состояние"));

	StatusEdit = new QLabel( this, "StatusLine" );
	StatusEdit->setGeometry( QRect( 245, 70, 250, 25 ) );

	StatusEdit->setProperty( "frameShape", (int)QLabel::Panel );
	StatusEdit->setProperty( "frameShadow", (int)QLabel::Sunken );

	StatusEdit->setProperty( "alignment", int( QLabel::AlignCenter ) );
	//StatusEdit->setProperty( "alignment", int( QLabel::AlignTop ) );


// Поле ввода "Количество"
	AllCountLabel = new QLabel( this, "AllCountLabel" );
	AllCountLabel->setGeometry( QRect( 10, 100,250, 25 ) );
	AllCountLabel->setText(W2U("Количество бонусов на карте"));

	AllCountEdit = new QLabel( this, "AllCountEdit" );
	AllCountEdit->setGeometry( QRect( 335, 100, 160, 25 ) );

	AllCountEdit->setProperty( "frameShape", (int)QLabel::Panel );
	AllCountEdit->setProperty( "frameShadow", (int)QLabel::Sunken );

	AllCountEdit->setProperty( "alignment", int( QLabel::AlignRight ) );

// Поле ввода "Доступно количество"
	EvalCountLabel = new QLabel( this, "EvalCountLabel" );
	EvalCountLabel->setGeometry( QRect( 10,130, 250, 25 ) );
	EvalCountLabel->setText(W2U("в т.ч. доступных для списания"));

	EvalCountEdit = new QLabel( this, "EvalCountEdit" );
	EvalCountEdit->setGeometry( QRect( 335, 130, 160, 25 ) );

	EvalCountEdit->setProperty( "alignment", int( QLabel::AlignRight ) );

	EvalCountEdit->setProperty( "frameShape", (int)QLabel::Panel );
	EvalCountEdit->setProperty( "frameShadow", (int)QLabel::Sunken );

	EvalCountEdit->setProperty( "alignment", int( QLabel::AlignRight ) );

// Надпись "Сумма списания"
	SummaLabel = new QLabel( this, "SummaLabel" );
	SummaLabel->setGeometry( QRect( 10, 170,300, 40 ) );
	SummaLabel->setText(W2U("Списать баллов в счет оплаты:"));

	QFont summa_font( font() );
	summa_font.setPointSize( 14 );
	summa_font.setBold(true);
	SummaLabel->setFont( summa_font );


	SummaEdit = new QLineEdit( this, "SummaEdit" );
	SummaEdit->setGeometry( QRect( 335, 170, 160, 40 ) );

	double_SummaValid = new QDoubleValidator(SummaEdit);
	double_SummaValid->setRange(1,1000,2);

	SummaEdit->setValidator(double_SummaValid);

    summa_font.setPointSize( 18 );
    SummaEdit->setFont(summa_font);
    SummaEdit->setAlignment(int( QLabel::AlignRight ) );


// Кнопки
	AddButton = new QPushButton( this, "AddButton" );
	AddButton->setGeometry( QRect( 80, 215, 170, 40 ) );
	AddButton->setText(W2U("Оплатить"));

	CancelButton = new QPushButton( this, "AddButton" );
	CancelButton->setGeometry( QRect( 270, 215, 170, 40 ) );
	CancelButton->setText(W2U("Отмена"));

	// signals and slots connections
	connect( AddButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( CancelButton, SIGNAL( clicked() ), this, SLOT(close() ) );
	//connect( NameEdit, SIGNAL( returnPressed() ), this, SLOT( accept() ) );
/*

    connect( AllCountEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( onCountChange(const QString &) ) );
    connect( EvalCountEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( updateSumInfo(const QString &) ) );
    connect( BarcodeEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( onBarcodeChange(const QString &) ) );
    connect( NoDiscount, SIGNAL( stateChanged(int) ), this, SLOT( onCheckedNoDiscount() ) );

//    connect( BarcodeEdit, SIGNAL(barcodeFound()), this, SLOT(ProcessBarcode()));
//    connect( NameEdit, SIGNAL(barcodeFound()), this, SLOT(ProcessBarcode()));
	*/

	SummaEdit->setFocus();

}

/*
 *  Destroys the object and frees any allocated resources
 */
BankBonusForm::~BankBonusForm()
{
    // no need to delete child widgets, Qt does it all for us

	delete double_SummaValid;

}

