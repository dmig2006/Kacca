/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qframe.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "Login.h"
#include "Utils.h"

/*
 *  Constructs a LoginBase which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
LoginForm::LoginForm( QWidget* parent ) : QDialog( parent, "LoginForm",true )
{
	setName( "LoginForm" );
	resize( 265, 180 );
	setProperty( "minimumSize", QSize( 265, 180 ) );
	setProperty( "maximumSize", QSize( 265, 180 ) );
	setProperty( "baseSize", QSize( 265, 180 ) );

	setCaption(W2U("APM KACCA") );

	QFont form_font( font() );
	form_font.setPointSize( 10 );
	setFont( form_font );

	TextLabel1 = new QLabel( this, "TextLabel1" );
	TextLabel1->setGeometry( QRect( 10, 60, 53, 20 ) );
	TextLabel1->setProperty( "text", W2U("Имя") );

	TextLabel2 = new QLabel( this, "TextLabel2" );
	TextLabel2->setGeometry( QRect( 10, 90, 53, 20 ) );
	TextLabel2->setProperty( "text", W2U("Пароль") );

	LoginEdit = new QLineEdit( this, "LoginEdit" );
	LoginEdit->setGeometry( QRect( 80, 60, 170, 22 ) );
	
	PasswdEdit = new QLineEdit( this, "PasswdEdit" );
	PasswdEdit->setGeometry( QRect( 80, 90, 170, 22 ) );
	PasswdEdit->setProperty( "echoMode", (int)QLineEdit::Password );

	PushButton1 = new QPushButton( this, "PushButton1" );
	PushButton1->setGeometry( QRect( 20, 130, 93, 26 ) );
	PushButton1->setProperty( "text", "OK" );

	PushButton2 = new QPushButton( this, "PushButton2" );
	PushButton2->setGeometry( QRect( 140, 130, 93, 26 ) );
	PushButton2->setProperty( "text", W2U("Отмена") );

	Frame3 = new QFrame( this, "Frame3" );
	Frame3->setGeometry( QRect( 0, 10, 265, 40 ) );
	Frame3->setProperty( "frameShape", (int)QFrame::StyledPanel );
	Frame3->setProperty( "frameShadow", (int)QFrame::Sunken );

	Label3 = new QLabel( Frame3, "Label3" );
	Label3->setGeometry( QRect( 10, 10, 240, 20 ) );
	Label3->setProperty( "text", W2U("База: ") );

	// signals and slots connections
	connect( PushButton2, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( PushButton1, SIGNAL( clicked() ), this, SLOT( OKClick() ) );
	connect( LoginEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
	connect( PasswdEdit, SIGNAL( returnPressed() ), this, SLOT( OKClick() ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
LoginForm::~LoginForm()
{
	// no need to delete child widgets, Qt does it all for us
}

void LoginForm::OKClick()//if some edit line is empty we set focus on it
{
	if (LoginEdit->text().isEmpty())
	{
		LoginEdit->setFocus();
		return;
	}
	if (PasswdEdit->text().isEmpty())
	{
		PasswdEdit->setFocus();
		return;
	}
	accept();
}

