/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qlabel.h>
#include <string>

using namespace std;

#include "Sum.h"
#include "Utils.h"

/*
 *  Constructs a SumForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SumForm::SumForm( QWidget* parent ) : QDialog( parent, "SumForm", true )
{
	setName( "SumForm" );
	resize( 190, 120 );
	setMinimumSize( QSize( 190, 120 ) );
	setMaximumSize( QSize( 190, 120 ) );
	setBaseSize( QSize( 190, 120 ) );
	setCaption( W2U("APM KACCA") );

	QFont form_font( font() );
	form_font.setPointSize( 10 );
	setFont( form_font );

	FormCaption = new QLabel(this, "FormCaption");
	FormCaption->setGeometry(QRect(5,5,170,32));

	QFont FormCaption_font( font() );
	FormCaption_font.setPointSize( 8 );
	FormCaption->setFont( FormCaption_font );
	FormCaption->setAlignment(int( QLabel::AlignCenter ));

	LineEdit = new QLineEdit( this, "LineEdit" );
	LineEdit->setGeometry( QRect( 10, 40, 160, 22 ) );
	LineEdit->setFocus();

	OKButton = new QPushButton( this, "OKButton" );
	OKButton->setGeometry( QRect( 20, 80, 60, 28 ) );
	OKButton->setText( W2U( "OK" ) );

	CancelButton = new QPushButton( this, "PushButton" );
	CancelButton->setGeometry( QRect( 100, 80, 60, 28 ) );
	CancelButton->setText( W2U("Отмена") );

	// signals and slots connections
	connect( OKButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( CancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( LineEdit, SIGNAL( returnPressed() ), this, SLOT( accept() ) );
//	connect( LineEdit, SIGNAL( focusOutEvent( QFocusEvent * ) ), this, SLOT( accept() ) );

	LineEdit->setFocus();
}

/*
 *  Destroys the object and frees any allocated resources
 */
SumForm::~SumForm()
{
    // no need to delete child widgets, Qt does it all for us
}

