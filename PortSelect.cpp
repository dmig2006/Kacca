/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qcombobox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "PortSelect.h"
#include "Utils.h"

/*
 *  Constructs a PortParamForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PortParamForm::PortParamForm( QWidget* parent ) : QDialog( parent, "PortParamForm", true )
{
	setName( "PortParamForm" );
    resize( 275, 160 );
    setProperty( "minimumSize", QSize( 275, 160 ) );
    setProperty( "maximumSize", QSize( 275, 160 ) );
    setProperty( "baseSize", QSize( 275, 160 ) );
	setCaption( W2U ("APM KACCA"));

	FormCaption = new QLabel(this, "FormCaption");
	FormCaption->setGeometry(QRect(10,5,250,22));

	QFont FormCaption_font( font() );
	FormCaption_font.setPointSize( 12 );
	FormCaption->setFont( FormCaption_font );
	FormCaption->setAlignment(int( QLabel::AlignCenter ));

    QFont form_font( font() );
    form_font.setPointSize( 10 );
    setFont( form_font );

    PortChoose = new QComboBox( FALSE, this, "PortChoose" );
    PortChoose->setGeometry( QRect( 100, 35, 150, 22 ) );

    PortChoose->insertItem("");
    PortChoose->insertItem("ttyS0");
    PortChoose->insertItem("ttyS1");
    PortChoose->insertItem("ttyS2");
    PortChoose->insertItem("ttyS3");
    PortChoose->insertItem("ttyS4");
    PortChoose->insertItem("ttyS5");
    PortChoose->insertItem("ttyS6");
    PortChoose->insertItem("ttyS7");
    PortChoose->insertItem("ttyS8");
    PortChoose->insertItem("ttyS9");
    PortChoose->insertItem("ttyS10");

    SpeedChoose = new QComboBox( FALSE, this, "ComboBox2" );
    SpeedChoose->setGeometry( QRect( 100, 75, 150, 22 ) );

    SpeedChoose->insertItem("");
    SpeedChoose->insertItem("1200");
    SpeedChoose->insertItem("2400");
    SpeedChoose->insertItem("4800");
    SpeedChoose->insertItem("9600");
    SpeedChoose->insertItem("19200");
    SpeedChoose->insertItem("38400");
    SpeedChoose->insertItem("57600");
    SpeedChoose->insertItem("115200");

    TextLabel1 = new QLabel( this, "TextLabel1" );
    TextLabel1->setGeometry( QRect( 10, 35, 80, 20 ) );
    TextLabel1->setProperty( "text", W2U( "Порт" ) );

    TextLabel2 = new QLabel( this, "TextLabel2" );
    TextLabel2->setGeometry( QRect( 10, 75, 80, 20 ) );
    TextLabel2->setProperty( "text", W2U( "Скорость" ) );

    PushButton1 = new QPushButton( this, "PushButton1" );
    PushButton1->setGeometry( QRect( 40, 115, 93, 26 ) );
    PushButton1->setProperty( "text", W2U("ОК") );

    PushButton2 = new QPushButton( this, "PushButton2" );
    PushButton2->setGeometry( QRect( 150, 115, 93, 26 ) );
    PushButton2->setProperty( "text", W2U( "Отмена" ) );

    // signals and slots connections
    connect( PushButton1, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( PushButton2, SIGNAL( clicked() ), this, SLOT( reject() ) );

    PortChoose->setFocus();
}

/*
 *  Destroys the object and frees any allocated resources
 */
PortParamForm::~PortParamForm()
{
	// no need to delete child widgets, Qt does it all for us
}

