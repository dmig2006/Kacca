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
#include <qpushbutton.h>

#include "Actsiz.h"
#include "Utils.h"
#include "ExtControls.h"


ActsizForm::ActsizForm( QWidget* parent, string msg) : QDialog( parent, "ActsizForm", true )
{
    if (msg=="") msg = "Счмтайте акциз";
    msg = FormatString(msg, 41);
    short cntlines = 0;
    for ( int i=0;i<msg.length();i++)
        if (msg[i]=='\n') cntlines++;
    cntlines++;

    int w_width, w_heigth;
    w_width = 500;
    w_heigth = 6*30 + cntlines*11+20+20;
    if (w_heigth>500)     w_heigth=500;

	setName( "ActsizForm" );
	resize( w_width, 210 );
	setProperty( "minimumSize", QSize( w_width, w_heigth ) );
	setProperty( "maximumSize", QSize( w_width, w_heigth ) );
	setProperty( "baseSize", QSize( w_width, w_heigth+10 ) );
	setCaption(W2U("APM KACCA"));

    int X = 15;
    int Y = 10;

	TitleLabel = new QColorLabel(this, "FormLabel", darkBlue );
	TitleLabel->setFixedSize(w_width-30,30);
	TitleLabel->move(X, Y);
	QFont TitleLabel_font( font() );
	TitleLabel_font.setPointSize( 14 );
	TitleLabel->setFont( TitleLabel_font );
	TitleLabel->setProperty( "text", W2U("ПРОДАЖА АЛКОГОЛЬНОЙ ПРОДУКЦИИ"));
	TitleLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

    Y += 35;

	HelpLabel = new QLabel( this, "HelpLabel" );
	HelpLabel->setGeometry( QRect( X, Y, w_width-2*X, cntlines*11+30 ) );
	QFont HelpLabel_font( font() );
	//HelpLabel_font.setPointSize( 14 );
	HelpLabel->setFont( HelpLabel_font );

	HelpLabel->setProperty( "text", W2U(msg));
	HelpLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

    Y += cntlines*11+30;

	TLabel = new QColorLabel(this, "FormLabel", darkBlue );
	TLabel->setFixedSize(w_width-30,15);
	TLabel->move(X, Y);
	TLabel->setProperty( "text", W2U("Считайте акцизную марку"));
	TLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

    Y += 20;

	vector<int> ScanCodesFlow,PrefixSeq,PostfixSeq,Codes;

	Codes.insert(Codes.end(),4148);
	Codes.insert(Codes.end(),47);

	BarcodeEdit = new ExtQLineEdit( this, "BarcodeLine", Codes, PrefixSeq, PostfixSeq );
	HelpLabel_font.setPointSize( 10 );
	BarcodeEdit->setGeometry( QRect( X, Y+10, w_width-2*X, 30 ) );
	BarcodeEdit->setFont( HelpLabel_font );
	BarcodeEdit->setProperty( "readOnly", true);
	BarcodeEdit->setProperty( "echoMode", (int)QLineEdit::Password );
	BarcodeEdit->setFocus();

    Y +=45;

	StatusLabel = new QColorLabel(this, "FormLabel", red );
	StatusLabel->setFixedSize(w_width-2*X,20);
	StatusLabel->move(X, Y);
	QFont StatusLabel_font( font() );
	StatusLabel_font.setPointSize( 11 );
	//StatusLabel_font.setBold(true);
	StatusLabel->setFont( StatusLabel_font );


    Y +=28;
// Кнопки
	AddButton = new QPushButton( this, "OKButton" );
	AddButton->setGeometry( QRect( 40, Y, 200, 40 ) );
	AddButton->setText(W2U("ОК"));

	CancelButton = new QPushButton( this, "CancelButton" );
	CancelButton->setGeometry( QRect( 270, Y, 200, 40 ) );
	CancelButton->setText(W2U("Отмена"));

	// signals and slots connections
	connect( AddButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( CancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );

	// signals and slots connections
	connect( BarcodeEdit, SIGNAL( returnPressed() ), this, SLOT( accept() ) );
	connect( BarcodeEdit, SIGNAL( barcodeFound() ), this, SLOT( accept() ) );

	//EditLine->setFocus();
	setFocus();

}

string ActsizForm::FormatString(string info, int linelength)
{

    string result = "";
    char sym = 0;
    int len = info.length();
	if (len > 0)
	{
        string str, word;
		int i;
		for (i=0;i<len;i++)
		{
		    sym=info[i];
            if (sym=='^') sym='\n';

		    word+=sym;

            if ((str+word).length()>linelength)
            {
                result+=str+"\n";
                str="";
                Log((char*)result.c_str());
            }
            else if (sym=='\n') // принудительный перенос
		    {
                result+=str+word;
		        str="";
		        word="";
            }

		    if
		    (
		    (sym == ' ')
		    ||
		    (sym == '.')
		    ||
		    (sym == ',')
		    ||
		    (sym == '!')
		    ||
		    (sym == '?')
		    )
		    {
                str+=word;
                word="";
		    }

		}

        str+=word;

        if (str.length()>0)
        {
            result+=str;
        }
	}

	return result;
}

/*
 *  Destroys the object and frees any allocated resources
 */
ActsizForm::~ActsizForm()
{
	// no need to delete child widgets, Qt does it all for us
}


