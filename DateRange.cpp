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

#include "DateRange.h"
#include "Utils.h"

/*
 *  Constructs a DateRangeForm which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DateRangeForm::DateRangeForm( QWidget* parent) : QDialog( parent, "APM KACCA", true )
{
	resize( 275, 180 ); 
	setProperty( "minimumSize", QSize( 275, 180 ) );
	setProperty( "maximumSize", QSize( 275, 180 ) );
	setProperty( "baseSize", QSize( 275, 180 ) );
	setCaption(W2U("APM KACCA"));

	QFont TextLabel_font( font() );
	TextLabel_font.setPointSize( 12 );
	TextLabel_font.setBold( true );

	TextLabel1_2_2 = new QLabel( this, "TextLabel1_2_2" );
	TextLabel1_2_2->setGeometry( QRect( 205, 30, 16, 22 ) ); 
	TextLabel1_2_2->setFont( TextLabel_font ); 
	TextLabel1_2_2->setProperty( "text", tr( " /" ) );

	TextLabel1_3 = new QLabel( this, "TextLabel1_3" );
	TextLabel1_3->setGeometry( QRect( 160, 30, 16, 22 ) ); 
	TextLabel1_3->setFont( TextLabel_font ); 
	TextLabel1_3->setProperty( "text", tr( " /" ) );

	FirstDateLabel = new QLabel( this, "FirstDateLabel" );
	FirstDateLabel->setGeometry( QRect( 11, 30, 110, 22 ) ); 
	FirstDateLabel->setProperty( "text", W2U( "Первая дата" ) );

	TextLabel1_3_2 = new QLabel( this, "TextLabel1_3_2" );
	TextLabel1_3_2->setGeometry( QRect( 160, 80, 16, 22 ) ); 
	TextLabel1_3_2->setFont( TextLabel_font ); 
	TextLabel1_3_2->setProperty( "text", tr( " /" ) );

	TextLabel1_2_2_2 = new QLabel( this, "TextLabel1_2_2_2" );
	TextLabel1_2_2_2->setGeometry( QRect( 205, 80, 16, 22 ) ); 
	TextLabel1_2_2_2->setFont( TextLabel_font ); 
	TextLabel1_2_2_2->setProperty( "text", tr( " /" ) );

	LastDateLabel = new QLabel( this, "LastDateLabel" );
	LastDateLabel->setGeometry( QRect( 11, 80, 110, 22 ) ); 
	LastDateLabel->setProperty( "text", W2U( "Последняя дата" ) );

	FirstDateDayEdit = new QLineEdit( this, "FirstDateDayEdit" );
	FirstDateDayEdit->setGeometry( QRect( 130, 30, 28, 22 ) ); 
	FirstDateDayEdit->setProperty( "maxLength", 2 );

	FirstDateMonthEdit = new QLineEdit( this, "FirstDateMonthEdit" );
	FirstDateMonthEdit->setGeometry( QRect( 180, 30, 28, 22 ) ); 
	FirstDateMonthEdit->setProperty( "maxLength", 2 );

	FirstDateYearEdit = new QLineEdit( this, "FirstDateYearEdit" );
	FirstDateYearEdit->setGeometry( QRect( 219, 30, 45, 22 ) ); 
	FirstDateYearEdit->setProperty( "maxLength", 4 );

	LastDateDayEdit = new QLineEdit( this, "LastDateDayEdit" );
	LastDateDayEdit->setGeometry( QRect( 130, 80, 28, 22 ) ); 
	LastDateDayEdit->setProperty( "maxLength", 2 );

	LastDateMonthEdit = new QLineEdit( this, "LastDateMonthEdit" );
	LastDateMonthEdit->setGeometry( QRect( 180, 80, 28, 22 ) ); 
	LastDateMonthEdit->setProperty( "maxLength", 2 );

	LastDateYearEdit = new QLineEdit( this, "LastDateYearEdit" );
	LastDateYearEdit->setGeometry( QRect( 220, 80, 45, 22 ) ); 
	LastDateYearEdit->setProperty( "maxLength", 4 );

	OKButton = new QPushButton( this, "OKButton" );
	OKButton->setGeometry( QRect( 90, 130, 90, 26 ) ); 
	OKButton->setProperty( "text", tr( "OK" ) );

	FirstDateDayEdit->setFocus();

	// signals and slots connections
	connect( OKButton, SIGNAL( clicked() ), this, SLOT( OKClick() ) );
	connect( FirstDateDayEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( FirstDateMonthEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( FirstDateYearEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( LastDateDayEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( LastDateMonthEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( LastDateYearEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DateRangeForm::~DateRangeForm()
{
    // no need to delete child widgets, Qt does it all for us
}

void DateRangeForm::OKClick()
{
	if (FirstDateDayEdit->text().isEmpty())
	{
		FirstDateDayEdit->setFocus();
		return;
	}

	if (FirstDateMonthEdit->text().isEmpty())
	{
		FirstDateMonthEdit->setFocus();
		return;
	}

	if (FirstDateYearEdit->text().isEmpty())
	{
		FirstDateYearEdit->setFocus();
		return;
	}

	if (LastDateDayEdit->text().isEmpty())
	{
		LastDateDayEdit->setFocus();
		return;
	}

	if (LastDateMonthEdit->text().isEmpty())
	{
		LastDateMonthEdit->setFocus();
		return;
	}

	if (LastDateYearEdit->text().isEmpty())
	{
		LastDateYearEdit->setFocus();
		return;
	}

	accept();
}

