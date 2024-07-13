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
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "Certificate.h"
#include "Utils.h"
#include "ExtControls.h"

/*
 *  Constructs a GuardForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
CertForm::CertForm( QWidget* parent, vector<int> prefix, vector<int> postfix ) : QDialog( parent, "CertForm", true )
{
	setName( "GuardForm" );
	resize( 240, 110 );
	setProperty( "minimumSize", QSize( 240, 100 ) );
	setProperty( "maximumSize", QSize( 240, 100 ) );
	setProperty( "baseSize", QSize( 240, 110 ) );
	setCaption(W2U("APM KACCA"));

	HelpLabel = new QLabel( this, "HelpLabel" );
	HelpLabel->setGeometry( QRect( 10, 10, 210, 20 ) );
	QFont HelpLabel_font( font() );
	HelpLabel_font.setPointSize( 14 );
	HelpLabel->setFont( HelpLabel_font );
	HelpLabel->setProperty( "text", W2U("¬ведите штрих-код"));
	HelpLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

	vector<int> codes;
	EditLine = new ExtQLineEdit( this, "LineEdit", codes, prefix, postfix );
	//EditLine->setDisabled(true);
	EditLine->setGeometry( QRect( 10, 40, 210, 30 ) );
	EditLine->setFont( HelpLabel_font );
	EditLine->setProperty( "echoMode", (int)QLineEdit::Password );

	// signals and slots connections
	connect( EditLine, SIGNAL( returnPressed() ), this, SLOT( accept() ) );
	connect( EditLine, SIGNAL( barcodeFound() ), this, SLOT( accept() ) );

	//EditLine->setFocus();
	setFocus();
}

/*
 *  Destroys the object and frees any allocated resources
 */
CertForm::~CertForm()
{
	// no need to delete child widgets, Qt does it all for us
}


