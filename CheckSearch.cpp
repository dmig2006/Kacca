/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "CheckSearch.h"
#include "Utils.h"

/*
 *  Constructs a CheckSearchForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
CheckSearchForm::CheckSearchForm( QWidget* parent) : QDialog( parent, "CheckSearchForm", true  )
{
	setName( "CheckSearchForm" );
	resize( 275, 180 );
	setProperty( "minimumSize", QSize( 275, 180 ) );
	setProperty( "maximumSize", QSize( 275, 180 ) );
	setProperty( "baseSize", QSize( 275, 180 ) );
	setCaption(W2U("APM KACCA"));

	QFont form_font( font() );
	form_font.setPointSize( 10 );
	setFont( form_font );

	Label1 = new QLabel( this, "Label1" );
	Label1->setGeometry( QRect( 5, 20, 80, 20 ) );
	Label1->setProperty( "text", W2U("Номер кассы") );

	Label2 = new QLabel( this, "Label2" );
	Label2->setGeometry( QRect( 5, 60, 80, 20 ) );
	Label2->setProperty( "text", W2U("Номер чека") );

	Label3 = new QLabel( this, "Label3" );
	Label3->setGeometry( QRect( 5, 100, 85, 20 ) );
	Label3->setProperty( "text", W2U("Дата продажи") );

	TextLabel1 = new QLabel( this, "TextLabel1" );
	TextLabel1->setGeometry( QRect( 130, 100, 16, 20 ) );
	QFont TextLabel1_font(  TextLabel1->font() );
	TextLabel1_font.setFamily( "Arial" );
	TextLabel1_font.setPointSize( 12 );
	TextLabel1_font.setBold( TRUE );
	TextLabel1->setFont( TextLabel1_font );
	TextLabel1->setProperty( "text", tr( " /" ) );

	TextLabel1_2 = new QLabel( this, "TextLabel1_2" );
	TextLabel1_2->setGeometry( QRect( 180, 100, 16, 20 ) );
	QFont TextLabel1_2_font(  TextLabel1_2->font() );
	TextLabel1_2_font.setFamily( "Arial" );
	TextLabel1_2_font.setPointSize( 12 );
	TextLabel1_2_font.setBold( TRUE );
	TextLabel1_2->setFont( TextLabel1_2_font );
	TextLabel1_2->setProperty( "text", tr( " /" ) );

	CashDeskEdit = new QLineEdit( this, "CashDeskEdit" );
	CashDeskEdit->setGeometry( QRect( 100, 20, 160, 22 ) );
	CashDeskEdit->setMaxLength(6);

	ChequeEdit = new QLineEdit( this, "ChequeEdit" );
	ChequeEdit->setGeometry( QRect( 100, 60, 160, 22 ) );
	ChequeEdit->setMaxLength(20);

	DateDayEdit = new QLineEdit( this, "DateDayEdit" );
	DateDayEdit->setGeometry( QRect( 100, 100, 28, 22 ) );
	DateDayEdit->setProperty( "maxLength", 2 );

	DateMonthEdit = new QLineEdit( this, "DateMonthEdit" );
	DateMonthEdit->setGeometry( QRect( 148, 100, 28, 22 ) );
	DateMonthEdit->setProperty( "maxLength", 2 );

	DateYearEdit = new QLineEdit( this, "DateYearEdit" );
	DateYearEdit->setGeometry( QRect( 200, 100, 50, 22 ) );
	DateYearEdit->setProperty( "maxLength", 4 );

	OKButton = new QPushButton( this, "OKButton" );
	OKButton->setGeometry( QRect( 20, 140, 104, 28 ) );
	OKButton->setProperty( "text", W2U("OK") );

	CancelButton = new QPushButton( this, "CancelButton" );
	CancelButton->setGeometry( QRect( 140, 140, 104, 28 ) );
	CancelButton->setProperty( "text", W2U("Отмена") );

	// signals and slots connections
//	connect( CashDeskEdit, SIGNAL( returnPressed() ), this, SLOT( accept() ) );
	connect( CashDeskEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( ChequeEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( DateDayEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( DateMonthEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( DateYearEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( OKButton, SIGNAL( clicked() ), this, SLOT( OKClick() ) );
	connect( CancelButton, SIGNAL( clicked() ), this, SLOT (reject() ) );

	CashDeskEdit->setFocus();
}

/*
 *  Destroys the object and frees any allocated resources
 */
CheckSearchForm::~CheckSearchForm()
{
    // no need to delete child widgets, Qt does it all for us
}

void CheckSearchForm::OKClick()//if some field is empty we set focus on it
{
	if (CashDeskEdit->text().isEmpty())
	{
		CashDeskEdit->setFocus();
		return;
	}

	QString s = CashDeskEdit->text();
	if(!(s[0] >= '0' && s[0] <= '9')){
		accept();
		return;
	}

	if (ChequeEdit->text().isEmpty())
	{
		ChequeEdit->setFocus();
		return;
	}

	if (DateDayEdit->text().isEmpty())
	{
		DateDayEdit->setFocus();
		return;
	}

	if (DateMonthEdit->text().isEmpty())
	{
		DateMonthEdit->setFocus();
		return;
	}

	if (DateYearEdit->text().isEmpty())
	{
		DateYearEdit->setFocus();
		return;
	}

	accept();
}

