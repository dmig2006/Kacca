/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//#include <qcheckbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
//#include <qvalidator.h>
//#include <qapplication.h>
#include <stdio.h>

#include <string>
#include <vector>

using namespace std;

#include "ChangeForm.h"

#include "Utils.h"
#include "PriceSel.h"
#include "ExtControls.h"


/*
 *  Constructs a ChangeForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
ChangeForm::ChangeForm( QWidget* parent, int mode) : QDialog( parent, "ChangeForm", true )
{
	setName( "ChangeForm" );
	resize( 500, 300 );
	setProperty( "minimumSize", QSize( 500, 210 ) );
	setProperty( "maximumSize", QSize( 500, 210 ) );
	setProperty( "baseSize", QSize( 500, 230 ) );
	setCaption(W2U("APM KACCA"));

	QFont Form_font(font());
	Form_font.setPointSize(12);
	setFont(Form_font);

// Заголовок

    FormLabel = new QColorLabel(this, "FormLabel", (mode==1)?darkRed:((mode==2)?darkGreen:darkBlue) );
    FormLabel->setProperty( "text", W2U( "ЗАМЕНА БОНУСНОЙ КАРТЫ" ) );

	FormLabel->setFixedSize(480,30);
	FormLabel->move(5,3);

    QFont FormLabel_font( font() );
    FormLabel_font.setPointSize( 12 );
    FormLabel_font.setBold(true);

    FormLabel->setFont( FormLabel_font );
	FormLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

// Промо-текст
	InfoLabel = new QColorLabel( this, "InfoLabel", darkRed );
	InfoLabel->setGeometry( QRect( 5, 120, 490, 26 ) );

    QFont InfoLabel_font( font() );
	InfoLabel_font.setPointSize( 14 );
	InfoLabel_font.setBold(true);

	InfoLabel->setFont( InfoLabel_font );

	//InfoLabel->setProperty("text",W2U("Желаете оплатить бонусами ? "));
	InfoLabel->setProperty( "frameShape", (int)QLabel::Panel );
	InfoLabel->setProperty( "frameShadow", (int)QLabel::Sunken );

	InfoLabel->setProperty( "alignment", int( QLabel::AlignLeft ) );

// Поле ввода "Карта"
    QFont tmp_font( font() );
    tmp_font.setPointSize( 12 );
    tmp_font.setBold(true);

	CurS = new QLabel( this, "CurS" );
	CurS->setGeometry( QRect(  10, 40, 30, 25 ) );
    CurS->setText(W2U(""));
    CurS->setFont( tmp_font );

	SourceCardLabel = new QLabel( this, "SourceCardLabel" );
	SourceCardLabel->setGeometry( QRect(  40, 40, 200, 25 ) );
	SourceCardLabel->setText(W2U("Дисконтная карта"));

	SourceCard = new QColorLabel( this, "SourceCard", black );
	SourceCard->setGeometry( QRect( 245, 40, 250, 25 ) );

	SourceCard->setProperty( "frameShape", (int)QLabel::Panel );
	SourceCard->setProperty( "frameShadow", (int)QLabel::Sunken );

	SourceCard->setProperty( "alignment", int( QLabel::AlignCenter ) );


// Поле ввода "Карта"
	CurD = new QLabel( this, "CurD" );
	CurD->setGeometry( QRect(  10, 80, 30, 25 ) );
    CurD->setText(W2U(""));
    CurD->setFont( tmp_font );

	DestCardLabel = new QLabel( this, "DestCardLabel" );
	DestCardLabel->setGeometry( QRect(  40, 80, 200, 25 ) );
	DestCardLabel->setText(W2U("Бонусная карта "));

	DestCard = new QColorLabel( this, "DestCard", black );
	DestCard->setGeometry( QRect( 245, 80, 250, 25 ) );

	DestCard->setProperty( "frameShape", (int)QLabel::Panel );
	DestCard->setProperty( "frameShadow", (int)QLabel::Sunken );

	DestCard->setProperty( "alignment", int( QLabel::AlignCenter ) );



    switch (mode) {
        case 1: {
            // Change to VIP card
            FormLabel->setProperty( "text", W2U( "ЗАМЕНА 5% КАРТЫ НА VIP" ) );
            SourceCardLabel->setText(W2U("Текущая 5% карта"));
            DestCardLabel->setText(W2U("Новая VIP карта "));
            break;
        }

        case 2:
        {
            // Change partner card
            FormLabel->setProperty( "text", W2U( "ЗАМЕНА КАРТЫ ПАРТНЕРОВ" ) );
            SourceCardLabel->setText(W2U("Карта ПАРТНЕРОВ"));
            DestCardLabel->setText(W2U("Бонусная карта "));
            break;
        }

        case 3:
        {
            // Change to Super-VIP card
            FormLabel->setProperty( "text", W2U( "ЗАМЕНА VIP КАРТЫ НА SUPER-VIP" ) );
            SourceCardLabel->setText(W2U("Текущая VIP карта"));
            DestCardLabel->setText(W2U("Новая Super-VIP карта "));
            break;
        }

    }



// Кнопки
	AddButton = new QPushButton( this, "AddButton" );
	AddButton->setGeometry( QRect( 20, 160, 230, 40 ) );
	AddButton->setText(W2U("Считать номер карты"));

	CancelButton = new QPushButton( this, "AddButton" );
	CancelButton->setGeometry( QRect( 260, 160, 230, 40 ) );
	CancelButton->setText(W2U("Отмена"));

	// signals and slots connections
	connect( AddButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( CancelButton, SIGNAL( clicked() ), this, SLOT(close() ) );

	//connect( NameEdit, SIGNAL( returnPressed() ), this, SLOT( accept() ) );
//    connect( BarcodeEdit, SIGNAL(barcodeFound()), this, SLOT(ProcessBarcode()));
//    connect( NameEdit, SIGNAL(barcodeFound()), this, SLOT(ProcessBarcode()));


}

/*
 *  Destroys the object and frees any allocated resources
 */
ChangeForm::~ChangeForm()
{


}

