/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "Exclamation.h"
#include "ExtControls.h"
#include "Utils.h"

#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/*
 *  Constructs a ExclamationForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
ExclamationForm::ExclamationForm( QWidget* parent) : QDialog( parent, "APM KACCA", true )
{
	resize( 500, 380 );
	setMinimumSize( QSize( 500, 380 ) );
	setMaximumSize( QSize( 500, 380 ) );
	setBaseSize( QSize( 500, 380 ) );
	setCaption( tr( "APM KACCA" ) );

	YesButton = new QPushButton( this, "YesButton" );
	YesButton->setGeometry( QRect( 200, 340, 108, 28 ) );
	YesButton->setText( W2U( "Ok" ) );
	YesButton->setFocus();

//	NoButton = new QPushButton( this, "NoButton" );
//	NoButton->setGeometry( QRect( 290, 340, 108, 28 ) );
//	NoButton->setText( W2U( "Нет" ) );
//	NoButton->setFocus();

	AlertLabel = new QColorLabel( this, "AlertLabel", darkRed );
	AlertLabel->setGeometry( QRect( 0, 255, 500, 81 ) );
	QFont AlertLabel_font( font() );
	AlertLabel_font.setPointSize( 12 );
	AlertLabel->setFont( AlertLabel_font );
	AlertLabel->setText( tr( "" ) );
	AlertLabel->setAlignment( int( QLabel::AlignCenter ) );

	ExclamationLabel = new QColorLabel( this, "ExclamationLabel", darkRed );
	ExclamationLabel->setGeometry( QRect( 0, 1, 500, 250 ) );
	QFont ExclamationLabel_font( font() );
	ExclamationLabel_font.setPointSize( 36 );
	ExclamationLabel->setFont( ExclamationLabel_font );
	ExclamationLabel->setFrameShape( QLabel::Panel );
	ExclamationLabel->setFrameShadow( QLabel::Sunken );
	ExclamationLabel->setText( tr( "" ) );
	ExclamationLabel->setAlignment( int( QLabel::AlignCenter ) );

	// signals and slots connections
	connect( YesButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
ExclamationForm::~ExclamationForm()
{
    // no need to delete child widgets, Qt does it all for us
}


/*
ExclamationForm::ExclamationForm( QWidget* parent) : QDialog( parent, "APM KACCA", true )
{
	resize( 500, 380 );
	setMinimumSize( QSize( 500, 380 ) );
	setMaximumSize( QSize( 500, 380 ) );
	setBaseSize( QSize( 500, 380 ) );
	setCaption( tr( "APM KACCA" ) );

	YesButton = new QPushButton( this, "YesButton" );
	YesButton->setGeometry( QRect( 100, 340, 108, 28 ) );
	YesButton->setText( W2U( "Да" ) );

	NoButton = new QPushButton( this, "NoButton" );
	NoButton->setGeometry( QRect( 290, 340, 108, 28 ) );
	NoButton->setText( W2U( "Нет" ) );
	NoButton->setFocus();

	AlertLabel = new QColorLabel( this, "AlertLabel", darkRed );
	AlertLabel->setGeometry( QRect( 0, 300, 500, 31 ) );
	QFont AlertLabel_font( font() );
	AlertLabel_font.setPointSize( 16 );
	AlertLabel->setFont( AlertLabel_font );
	AlertLabel->setText( tr( "" ) );
	AlertLabel->setAlignment( int( QLabel::AlignCenter ) );

	ExclamationLabel = new QColorLabel( this, "ExclamationLabel", darkRed );
	ExclamationLabel->setGeometry( QRect( 0, 1, 500, 290 ) );
	QFont ExclamationLabel_font( font() );
	ExclamationLabel_font.setPointSize( 72 );
	ExclamationLabel->setFont( ExclamationLabel_font );
	ExclamationLabel->setFrameShape( QLabel::Panel );
	ExclamationLabel->setFrameShadow( QLabel::Sunken );
	ExclamationLabel->setText( tr( "" ) );
	ExclamationLabel->setAlignment( int( QLabel::AlignCenter ) );

	// signals and slots connections
	connect( YesButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
}
*/
