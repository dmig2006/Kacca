/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "PaymentType.h"
#include "Utils.h"

/*
 *  Constructs a PaymentTypeForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PaymentTypeForm::PaymentTypeForm( QWidget* parent )  : QDialog( parent, "PaymentTypeForm", true )
{
	QSize s( 161, 190 );
	//s.setWidth ( int w )
	//s.setHeight ( int h )
	setName( "PaymentTypeForm" );
	resize( s );
	setProperty( "minimumSize", s );
	setProperty( "maximumSize", s );
	setProperty( "baseSize", s );
	setCaption( W2U ("APM KACCA"));

//	BankBonusButton = new QPushButton( this, "BankBonusButton" );
//	BankBonusButton->setGeometry( QRect( 5, 10, 150, 30 ) );
//	BankBonusButton->setProperty( "text", W2U("Бонусы ") );

	CertificateButton = new QPushButton( this, "CertificateButton" );
	CertificateButton->setGeometry( QRect( 5, 20, 150, 30 ) );
	CertificateButton->setProperty( "text", W2U("Сертификаты") );

	CashlessButton = new QPushButton( this, "CashlessButton" );
	CashlessButton->setGeometry( QRect( 5, 60, 150, 30 ) );
	CashlessButton->setProperty( "text", W2U("Безналичный расчет") );

	CashButton = new QPushButton( this, "CashButton" );
	CashButton->setGeometry( QRect( 5, 100, 150, 30 ) );
	CashButton->setProperty( "text", W2U("Наличный расчет")  );

	CancelButton = new QPushButton( this, "CancelButton" );
	CancelButton->setGeometry( QRect( 5, 140, 150, 30 ) );
	CancelButton->setProperty( "text", W2U("Отмена") );

	CashlessButton/*CancelButton*/->setFocus();

	PaymentType=CashPayment;

	// signals and slots connections
	connect( CancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( CashButton, SIGNAL( clicked() ), this, SLOT( cashclicked() ) );
	connect( CashlessButton, SIGNAL( clicked() ), this, SLOT( cashlessclicked() ) );
	connect( CertificateButton, SIGNAL( clicked() ), this, SLOT( certificateclicked() ) );
	//connect( BankBonusButton, SIGNAL( clicked() ), this, SLOT( bankbonusclicked() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
PaymentTypeForm::~PaymentTypeForm()
{
    // no need to delete child widgets, Qt does it all for us
}

void PaymentTypeForm::cashclicked()//set type of payment
{
	PaymentType=CashPayment;
	accept();
}

void PaymentTypeForm::cashlessclicked()
{
	PaymentType=CashlessPayment;
	accept();
}

void PaymentTypeForm::certificateclicked()
{
	PaymentType=CertificatePayment;
	accept();
}

void PaymentTypeForm::bankbonusclicked()
{
	PaymentType=BankBonusPayment;
	accept();
}
