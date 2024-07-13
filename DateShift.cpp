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

#include "DateShift.h"
#include "Utils.h"

/*
 *  Constructs a DateShiftRangeForm which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DateShiftRangeForm::DateShiftRangeForm( QWidget* parent) : QDialog( parent, "APM KACCA", true )
{
	setName( "DateShiftRangeForm" );
	resize( 308, 186 ); 
	setProperty( "minimumSize", QSize( 308, 186 ) );
	setProperty( "maximumSize", QSize( 308, 186 ) );
	setProperty( "baseSize", QSize( 308, 186 ) );
	setCaption(W2U("APM KACCA"));

	FirstShiftEdit = new QLineEdit( this, "FirstShiftEdit" );
	FirstShiftEdit->setGeometry( QRect( 60, 10, 50, 22 ) );
	FirstShiftEdit->setReadOnly(true);

	LastShiftEdit = new QLineEdit( this, "LastShiftEdit" );
	LastShiftEdit->setGeometry( QRect( 60, 70, 50, 22 ) );
	LastShiftEdit->setReadOnly(true);

	LastShiftLabel = new QLabel( this, "LastShiftLabel" );
	LastShiftLabel->setGeometry( QRect( 20, 100, 130, 20 ) ); 
	LastShiftLabel->setProperty( "frameShadow", (int)QLabel::Plain );
	LastShiftLabel->setProperty( "text", W2U( "Номер посл. смены" ) );
	LastShiftLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

	FirstDateEdit = new QLineEdit( this, "FirstDateEdit" );
	FirstDateEdit->setGeometry( QRect( 170, 10, 93, 22 ) );
	FirstDateEdit->setReadOnly(true);

	LastDateEdit = new QLineEdit( this, "LastDateEdit" );
	LastDateEdit->setGeometry( QRect( 170, 70, 93, 22 ) );
	LastDateEdit->setReadOnly(true);

	LastDateLabel = new QLabel( this, "LastDateLabel" );
	LastDateLabel->setGeometry( QRect( 150, 100, 130, 20 ) );
	LastDateLabel->setProperty( "frameShadow", (int)QLabel::Plain );
	LastDateLabel->setProperty( "text", W2U( "Дата посл. смены") );
	LastDateLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

	FirstShiftLabel = new QLabel( this, "FirstShiftLabel" );
	FirstShiftLabel->setGeometry( QRect( 20, 40, 130, 20 ) );
	FirstShiftLabel->setProperty( "frameShadow", (int)QLabel::Plain );
	FirstShiftLabel->setProperty( "text", W2U( "Номер перв. смены") );
	FirstShiftLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

	FirstDateLabel = new QLabel( this, "FirstDateLabel" );
	FirstDateLabel->setGeometry( QRect( 150, 40, 130, 20 ) );
	FirstDateLabel->setProperty( "frameShadow", (int)QLabel::Plain );
	FirstDateLabel->setProperty( "text", W2U( "Дата перв. смены") );
	FirstDateLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

	OKButton = new QPushButton( this, "OKButton" );
	OKButton->setGeometry( QRect( 100, 140, 93, 26 ) );
	OKButton->setProperty( "text", tr( "OK" ) );

	OKButton->setFocus();

	// signals and slots connections
	connect( OKButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DateShiftRangeForm::~DateShiftRangeForm()
{
    // no need to delete child widgets, Qt does it all for us
}

