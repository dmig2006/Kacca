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
#include <qlineedit.h>
#include <string>

using namespace std;

#include "Passwd.h"
#include "Utils.h"

/*
 *  Constructs a PasswdForm which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *

 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PasswdForm::PasswdForm( QWidget* parent ) : QDialog( parent, "PasswdForm", true )
{
	setName( "PasswdForm" );
	resize( 270, 125 );
	setMinimumSize( QSize( 270, 125 ) );
	setMaximumSize( QSize( 270, 125 ) );
	setBaseSize( QSize( 270, 125 ) );

	QFont form_font( font() );
	form_font.setPointSize( 10 );
	setFont( form_font );

	setCaption(  W2U("APM KACCA")  );

	Label1 = new QLabel( this, "Label1" );
	Label1->setGeometry( QRect( 5, 10, 40, 20 ) );
	Label1->setText(W2U( "Имя" ) );

	Label2 = new QLabel( this, "Label2" );
	Label2->setGeometry( QRect( 5, 50, 50, 20 ) );
	Label2->setText( W2U( "Пароль" ) );

	OK = new QPushButton( this, "OK" );
	OK->setGeometry( QRect( 80, 90, 104, 28 ) );
	OK->setText( W2U( "OK" ) );

	UserName = new QLineEdit( this, "UserName" );
	UserName->setGeometry( QRect( 55, 10, 200, 20 ) );

	PasswdLine = new QLineEdit( this, "PasswdLine" );
	PasswdLine->setGeometry( QRect( 55, 50, 200, 20 ) );
	PasswdLine->setEchoMode(QLineEdit::Password);

	// signals and slots connections
	connect( OK, SIGNAL( clicked() ), this, SLOT( okclick() ) );
	connect( UserName, SIGNAL( returnPressed() ), this, SLOT( okclick() ) );
	connect( PasswdLine, SIGNAL( returnPressed() ), this, SLOT( okclick() ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
PasswdForm::~PasswdForm()
{
    // no need to delete child widgets, Qt does it all for us
}

void PasswdForm::okclick()//if some edit line is empty we set focus on it
{
	if (UserName->text().isEmpty())
	{
		UserName->setFocus();
		return;
	}
	if (PasswdLine->text().isEmpty())
	{
		PasswdLine->setFocus();
		return;
	}
	
	accept();
}

