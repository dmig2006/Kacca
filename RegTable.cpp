/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "RegTable.h"

#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <stdio.h>

#include "Utils.h"
#include "Cash.h"
#include "Fiscal.h"

/* 
 *  Constructs a RegTableForm which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
RegTableForm::RegTableForm( CashRegister* parent) : QDialog( parent, "RegTableForm", true)
{
	resize( 300, 130 );
	setProperty( "minimumSize", QSize( 300, 130 ) );
	setProperty( "maximumSize", QSize( 300, 130 ) );
	setProperty( "baseSize", QSize( 300, 130 ) );
	setCaption(W2U("APM KACCA KACCA"));

	FieldLabel = new QLabel( this, "FieldLabel" );
	FieldLabel->setGeometry( QRect( 210, 10, 50, 20 ) );
	FieldLabel->setText( W2U("Поле") );

	RowLabel = new QLabel( this, "RowLabel" );
	RowLabel->setGeometry( QRect( 120, 10, 40, 20 ) );
	RowLabel->setText( W2U("Ряд") );

	TableLabel = new QLabel( this, "TableLabel" );
	TableLabel->setGeometry( QRect( 10, 10, 67, 20 ) );
	TableLabel->setText( W2U("Таблица") );

	ValueLabel = new QLabel( this, "ValueLabel" );
	ValueLabel->setGeometry( QRect( 10, 40, 80, 20 ) );
	ValueLabel->setText( W2U("Значение") );

	SetButton = new QPushButton( this, "SetButton" );
	SetButton->setGeometry( QRect( 10, 95, 80, 28 ) );
	SetButton->setText( W2U( "Установить" ) );

	GetButton = new QPushButton( this, "GetButton" );
	GetButton->setGeometry( QRect( 110, 95, 80, 28 ) );
	GetButton->setText( W2U( "Получить" ) );

	CancelButton = new QPushButton( this, "CancelButton" );
	CancelButton->setGeometry( QRect( 210, 95, 80, 28 ) );
	//CancelButton->setText( W2U("Выход") );
	CancelButton->setText(W2U("Exit"));

	TableEdit = new QLineEdit( this, "TableEdit" );
	TableEdit->setGeometry( QRect( 80, 10, 30, 22 ) );

	RowEdit = new QLineEdit( this, "RowEdit" );
	RowEdit->setGeometry( QRect( 170, 10, 30, 22 ) );

	FieldEdit = new QLineEdit( this, "FieldEdit" );
	FieldEdit->setGeometry( QRect( 255, 10, 30, 22 ) );

	ValueEdit = new QLineEdit( this, "ValueEdit" );
	ValueEdit->setGeometry( QRect( 90, 40, 190, 22 ) );

	asStringBox = new QCheckBox( this, "asStringBox" );
	asStringBox->setGeometry( QRect( 10, 70, 250, 20 ) );
	asStringBox->setProperty( "text", W2U("Использовать значение как текст") );

	TableEdit->setFocus();

	CashReg=parent;

	// signals and slots connections
	connect( SetButton, SIGNAL( clicked() ), this, SLOT( SetValue() ) );
	connect( GetButton, SIGNAL( clicked() ), this, SLOT( GetValue() ) );
	connect( CancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
RegTableForm::~RegTableForm()
{
    // no need to delete child widgets, Qt does it all for us
}

void RegTableForm::SetValue()
{
	char strval[128];memset(strval,'\0',128);
	int len;

	if (asStringBox->isChecked())
	{
		strcpy(strval,U2W(ValueEdit->text()).substr(0,/*CashReg->*/FiscalReg->LineLength).c_str());
		len=/*CashReg->*/FiscalReg->LineLength;
	}
	else
	{
		int val;
		bool conv;
		val=ValueEdit->text().toInt(&conv);
		if (!conv)
		{
			CashReg->ShowMessage("Неправильное значение!");
			return;
		}

		strval[0]=val%0xFF;
		len=1;
	}

	CashReg->ShowErrorCode(/*CashReg->*/FiscalReg->SetTable(TableEdit->text().toInt(),
		RowEdit->text().toInt(),FieldEdit->text().toInt(),strval,
		len,asStringBox->isChecked()));
}

void RegTableForm::GetValue()
{
	char strval[128];memset(strval,'\0',128);

	int er=FiscalReg->GetTable(TableEdit->text().toInt(),RowEdit->text().toInt(),
		FieldEdit->text().toInt(),strval,FiscalReg->LineLength,asStringBox->isChecked());
	if (er!=0)
	{
		CashReg->ShowErrorCode(er);
		return;
	}

	if (asStringBox->isChecked())
		ValueEdit->setText(W2U(strval));
	else
	{
		char buf[16];
		sprintf(buf,"%d",strval[0]);
		ValueEdit->setText(buf);
	}
}


