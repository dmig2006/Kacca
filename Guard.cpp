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

#include "Guard.h"
#include "Utils.h"
#include "ExtControls.h"

/*
 *  Constructs a GuardForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
GuardForm::GuardForm( QWidget* parent, vector<int> prefix, vector<int> postfix) : QDialog( parent, "GuardForm", true )
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

string GuardForm::FormatString(string info, int linelength)
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

GuardForm::GuardForm( QWidget* parent, vector<int> prefix, vector<int> postfix, string msg) : QDialog( parent, "GuardForm", true )
{
    if (msg=="") msg = "¬ведите штрих-код";
    msg = FormatString(msg, 41);
    short cntlines = 0;
    for ( int i=0;i<msg.length();i++)
        if (msg[i]=='\n') cntlines++;
    cntlines++;

    int w_width, w_heigth;
    w_width = 300;
    w_heigth = 30+30 + cntlines*11+20+10;
    if (w_heigth>300)     w_heigth=300;

	setName( "GuardForm" );
	resize( w_width, 210 );
	setProperty( "minimumSize", QSize( w_width, w_heigth ) );
	setProperty( "maximumSize", QSize( w_width, w_heigth ) );
	setProperty( "baseSize", QSize( w_width, w_heigth+10 ) );
	setCaption(W2U("APM KACCA"));

	HelpLabel = new QLabel( this, "HelpLabel" );
	HelpLabel->setGeometry( QRect( 10, 10, w_width-20, w_heigth-30-30 ) );
	QFont HelpLabel_font( font() );
	//HelpLabel_font.setPointSize( 14 );
	HelpLabel->setFont( HelpLabel_font );


	HelpLabel->setProperty( "text", W2U(msg));
	HelpLabel->setProperty( "alignment", int( QLabel::AlignCenter ) );

	vector<int> codes;
	EditLine = new ExtQLineEdit( this, "LineEdit", codes, prefix, postfix );
	//EditLine->setDisabled(true);
	EditLine->setGeometry( QRect( 10, w_heigth-30-10, w_width-20, 30 ) );
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
GuardForm::~GuardForm()
{
	// no need to delete child widgets, Qt does it all for us
}


