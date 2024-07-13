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

#include "ShiftRange.h"
#include "Utils.h"

/*
 *  Constructs a ShiftRangeForm which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
ShiftRangeForm::ShiftRangeForm( QWidget* parent) : QDialog( parent, "APM KACCA", true )
{
	resize( 213, 132 ); 
	setMinimumSize( QSize( 213, 132 ) );
	setMaximumSize( QSize( 213, 132 ) );
	setBaseSize( QSize( 213, 132 ) );
	setCaption( W2U("APM KACCA") );

	FirstShiftLabel = new QLabel( this, "FirstShiftLabel" );
	FirstShiftLabel->setGeometry( QRect( 10, 10, 120, 20 ) ); 
	FirstShiftLabel->setProperty( "text", W2U("Первая смена") );

	LastShiftLabel = new QLabel( this, "LastShiftLabel" );
	LastShiftLabel->setGeometry( QRect( 10, 50, 120, 20 ) );
	LastShiftLabel->setProperty( "text", W2U( "Последняя смена" ) );

	OKButton = new QPushButton( this, "OKButton" );
	OKButton->setGeometry( QRect( 60, 90, 93, 26 ) );
	OKButton->setProperty( "text", tr( "OK" ) );

	LastShiftEdit = new QLineEdit( this, "LastShiftEdit" );
	LastShiftEdit->setGeometry( QRect( 140, 50, 60, 22 ) );

	FirstShiftEdit = new QLineEdit( this, "FirstShiftEdit" );
	FirstShiftEdit->setGeometry( QRect( 140, 10, 60, 22 ) );
	FirstShiftEdit->setFocus();

	// signals and slots connections
	connect( OKButton, SIGNAL( clicked() ), this, SLOT( OKClick() ) );
	connect( FirstShiftEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( LastShiftEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );

	// tab order
	setTabOrder( FirstShiftEdit, LastShiftEdit );
	setTabOrder( LastShiftEdit, OKButton );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
ShiftRangeForm::~ShiftRangeForm()
{
    // no need to delete child widgets, Qt does it all for us
}

void ShiftRangeForm::OKClick(void)
{
	if (FirstShiftEdit->text().isEmpty())
	{
		FirstShiftEdit->setFocus();
		return;
	}

	if (LastShiftEdit->text().isEmpty())
	{
		LastShiftEdit->setFocus();
		return;
	}

	accept();
}

