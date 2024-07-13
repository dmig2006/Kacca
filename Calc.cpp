/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qfont.h>
#include <qcolor.h>

#include "Calc.h"
#include "Utils.h"

/* 
 *  Constructs a CalcForm which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
CalcForm::CalcForm( QWidget* parent) : QDialog( parent, "CalcForm", true )//init calculator
{
	setName( "CalcForm" );
	resize( 124, 178 );
	setCaption(W2U("APM KACCA"));

	CalcLine = new QLabel( this, "CalcLine" );
	CalcLine->setGeometry( QRect( 0, 0, 120, 22 ) );
	CalcLine->setProperty( "alignment", int( QLabel::AlignRight ) );
	QFont CalcFont(font());
	CalcFont.setPointSize(12);
	CalcFont.setBold(true);
	CalcLine->setFont(CalcFont);
	CalcLine->setBackgroundColor(QColor(255,255,255));
	CalcLine->setProperty( "frameShape", (int)QLabel::Panel );
	CalcLine->setProperty( "frameShadow", (int)QLabel::Sunken );

	BN7 = new QPushButton( this, "BN7" );
	BN7->setGeometry( QRect( 0, 30, 30, 26 ) );
	BN7->setProperty( "text", tr( "7" ) );
	BN7->setFont(CalcFont);

	BN8 = new QPushButton( this, "BN8" );
	BN8->setGeometry( QRect( 30, 30, 30, 26 ) );
	BN8->setProperty( "text", tr( "8" ) );
	BN8->setFont(CalcFont);

	BN9 = new QPushButton( this, "BN9" );
	BN9->setGeometry( QRect( 60, 30, 30, 26 ) );
	BN9->setProperty( "text", tr( "9" ) );
	BN9->setFont(CalcFont);

	BDiv = new QPushButton( this, "BDiv" );
	BDiv->setGeometry( QRect( 90, 30, 30, 26 ) );
	BDiv->setProperty( "text", tr( "/" ) );
	BDiv->setFont(CalcFont);

	BN4 = new QPushButton( this, "BN4" );
	BN4->setGeometry( QRect( 0, 60, 30, 26 ) );
	BN4->setProperty( "text", tr( "4" ) );
	BN4->setFont(CalcFont);

	BN5 = new QPushButton( this, "BN5" );
	BN5->setGeometry( QRect( 30, 60, 30, 26 ) );
	BN5->setProperty( "text", tr( "5" ) );
	BN5->setFont(CalcFont);

	BN6 = new QPushButton( this, "BN6" );
	BN6->setGeometry( QRect( 60, 60, 30, 26 ) );
	BN6->setProperty( "text", tr( "6" ) );
	BN6->setFont(CalcFont);

	BMul = new QPushButton( this, "BMul" );
	BMul->setGeometry( QRect( 90, 60, 30, 26 ) );
	BMul->setProperty( "text", tr( "X" ) );
	BMul->setFont(CalcFont);

	BN1 = new QPushButton( this, "B1" );
	BN1->setGeometry( QRect( 0, 90, 30, 26 ) );
	BN1->setProperty( "text", tr( "1" ) );
	BN1->setFont(CalcFont);

	BN2 = new QPushButton( this, "BN2" );
	BN2->setGeometry( QRect( 30, 90, 30, 26 ) );
	BN2->setProperty( "text", tr( "2" ) );
	BN2->setFont(CalcFont);

	BN3 = new QPushButton( this, "BN3" );
	BN3->setGeometry( QRect( 60, 90, 30, 26 ) );
	BN3->setProperty( "text", tr( "3" ) );
	BN3->setFont(CalcFont);

	BMinus = new QPushButton( this, "BMinus" );
	BMinus->setGeometry( QRect( 90, 90, 30, 26 ) );
	BMinus->setProperty( "text", tr( "---" ) );
	BMinus->setFont(CalcFont);

	BN0 = new QPushButton( this, "BN0" );
	BN0->setGeometry( QRect( 0, 120, 30, 26 ) );
	BN0->setProperty( "text", tr( "0" ) );
	BN0->setFont(CalcFont);

	BComma = new QPushButton( this, "BComma" );
	BComma->setGeometry( QRect( 30, 120, 30, 26 ) );
	BComma->setProperty( "text", tr( "," ) );
	BComma->setFont(CalcFont);

	BPM = new QPushButton( this, "BPM" );
	BPM->setGeometry( QRect( 60, 120, 30, 26 ) );
	BPM->setProperty( "text", tr( QString::fromUtf8( "+/-" ) ) );



	BPM->setFont(CalcFont);

	BPlus = new QPushButton( this, "BPlus" );
	BPlus->setGeometry( QRect( 90, 120, 30, 26 ) );
	BPlus->setProperty( "text", tr( "+" ) );
	BPlus->setFont(CalcFont);

	BBC = new QPushButton( this, "BBC" );
	BBC->setGeometry( QRect( 0, 150, 30, 26 ) );
	BBC->setProperty( "text", tr( "C" ) );
	BBC->setFont(CalcFont);

	BBS = new QPushButton( this, "BBS" );
	BBS->setGeometry( QRect( 30, 150, 30, 26 ) );
	BBS->setProperty( "text", tr( "<--" ) );
	BBS->setFont(CalcFont);



	BEq = new QPushButton( this, "BEq" );
	BEq->setGeometry( QRect( 60, 150, 60, 26 ) );
	BEq->setProperty( "text", tr( "=" ) );
	BEq->setFocus();
	BEq->setFont(CalcFont);

	// signals and slots connections
	connect( BN7, SIGNAL( clicked() ), this, SLOT( BN7Click() ) );
	connect( BN8, SIGNAL( clicked() ), this, SLOT( BN8Click() ) );
	connect( BN9, SIGNAL( clicked() ), this, SLOT( BN9Click() ) );
	connect( BDiv, SIGNAL( clicked() ), this, SLOT( BDivClick() ) );
	connect( BN4, SIGNAL( clicked() ), this, SLOT( BN4Click() ) );
	connect( BN5, SIGNAL( clicked() ), this, SLOT( BN5Click() ) );
	connect( BN6, SIGNAL( clicked() ), this, SLOT( BN6Click() ) );
	connect( BMul, SIGNAL( clicked() ), this, SLOT( BMulClick() ) );
	connect( BN1, SIGNAL( clicked() ), this, SLOT( BN1Click() ) );
	connect( BN2, SIGNAL( clicked() ), this, SLOT( BN2Click() ) );
	connect( BN3, SIGNAL( clicked() ), this, SLOT( BN3Click() ) );
	connect( BMinus, SIGNAL( clicked() ), this, SLOT( BMinusClick() ) );
	connect( BN0, SIGNAL( clicked() ), this, SLOT( BN0Click() ) );
	connect( BComma, SIGNAL( clicked() ), this, SLOT( BCommaClick() ) );
	connect( BPM, SIGNAL( clicked() ), this, SLOT( BPMClick() ) );
	connect( BPlus, SIGNAL( clicked() ), this, SLOT( BPlusClick() ) );
	connect( BBC, SIGNAL( clicked() ), this, SLOT( BCClick() ) );
	connect( BBS, SIGNAL( clicked() ), this, SLOT( BBSClick() ) );
	connect( BEq, SIGNAL( clicked() ), this, SLOT( BEqClick() ) );

	BN7->setAccel('7');//set accelerators
	BN8->setAccel('8');
	BN9->setAccel('9');
	BDiv->setAccel('/');
	BN4->setAccel('4');
	BN5->setAccel('5');
	BN6->setAccel('6');
	BMul->setAccel('*');
	BN1->setAccel('1');
	BN2->setAccel('2');
	BN3->setAccel('3');
	BMinus->setAccel('-');
	BN0->setAccel('0');
	BComma->setAccel('.');
	BPM->setAccel(Key_F9);
	BPlus->setAccel('+');
	BBC->setAccel(Key_Space);
	BBS->setAccel(Key_BackSpace);
	BEq->setAccel(Key_Enter);

	StoredValue=Value=0;
	ChosenAction='\0';
	FirstPress=true;
	CalcLine->setText("0");
}

/*  
 *  Destroys the object and frees any allocated resources
 */
CalcForm::~CalcForm()
{
	// no need to delete child widgets, Qt does it all for us
}

void CalcForm::BN0Click()
{
	AddChar('0');
}

void CalcForm::BN1Click()
{
	AddChar('1');
}

void CalcForm::BN2Click()
{
	AddChar('2');
}

void CalcForm::BN3Click()
{
	AddChar('3');
}

void CalcForm::BN4Click()
{
	AddChar('4');
}

void CalcForm::BN5Click()
{
	AddChar('5');
}

void CalcForm::BN6Click()
{
	AddChar('6');
}

void CalcForm::BN7Click()
{
	AddChar('7');
}

void CalcForm::BN8Click()
{
	AddChar('8');
}

void CalcForm::BN9Click()
{
	AddChar('9');
}

void CalcForm::BBSClick()//erase last digit in the result's line (and show zero if neccerary)
{
	if (Value!=0)
		if (CalcLine->text().length()==1)
		{
			Value=0;
			CalcLine->setText("0");
			FirstPress=true;
		}
		else
		{
			CalcLine->setText(CalcLine->text().left(CalcLine->text().length()-1));
			Value=CalcLine->text().toDouble();
			FirstPress=false;
		}
}

void CalcForm::BCClick()//erase result's line
{
	Value=0;
	CalcLine->setText("0");
	FirstPress=true;
}

void CalcForm::BCommaClick()//enter comma symbol (if it wasn't pressed before)
{
	if (CalcLine->text().find('.')==-1)
		AddChar('.');
}

void CalcForm::BEqClick()//evaluate result
{
	if (ChosenAction!='\0')
		Evaluate();
}

void CalcForm::BMinusClick()//press minus button
{
	if (ChosenAction!='\0')
		Evaluate();
	ChosenAction='-';
	FirstPress=true;
}

void CalcForm::BMulClick()//press multiplications' button 
{
	if (ChosenAction!='\0')
		Evaluate();
	ChosenAction='*';
	FirstPress=true;
}

void CalcForm::BPMClick()//press plus/minus button
{
	if (CalcLine->text().at(0)=='-')
		Value=-Value;
	else
		if ((CalcLine->text().length()<12)&&(CalcLine->text()!="0"))
			Value=-Value;

	CalcLine->setText(D2S(Value));
}

void CalcForm::BPlusClick()//press plus button
{
	if (ChosenAction!='\0')
		Evaluate();
	ChosenAction='+';
	FirstPress=true;
}


void CalcForm::BDivClick()//press divide button
{
	if (ChosenAction!='\0')
		Evaluate();
	ChosenAction='/';
	FirstPress=true;
}

QString CalcForm::D2S(double num)//convert double number to string and cut off zeroes and comma
{
	string res=Double2Str(num);

	if (res.find_first_of('.')!=res.npos)
		while (res[res.size()-1]=='0')
			res.erase(res.end()-1);

	if (res[res.size()-1]=='.')
		res.erase(res.end()-1);

	return res.c_str();
}

void CalcForm::Evaluate(void)//evaluate result according to chosed action
{
	switch (ChosenAction)
	{
		case '/':
			if (Value!=0)

			{
				Value=StoredValue/Value;
				StoredValue=0;
				CalcLine->setText(D2S(Value));
			}
			else
				::ShowMessage(this,"Ошибка: деление на 0");
			break;
		case '*':
			Value=Value*StoredValue;
			StoredValue=0;
			CalcLine->setText(D2S(Value));
			break;
		case '+':
			Value=Value+StoredValue;
			StoredValue=0;
			CalcLine->setText(D2S(Value));
			break;
		case '-':
			Value=StoredValue-Value;
			StoredValue=0;
			CalcLine->setText(D2S(Value));
			break;
	}
	ChosenAction='\0';
	FirstPress=true;
}

void CalcForm::AddChar(char ch)//add char to result's line
{
	if (FirstPress)
	{
		StoredValue=Value;
		Value=0;
		CalcLine->clear();
		FirstPress=false;
	}

	if (CalcLine->text().length()<12)
		CalcLine->setText(CalcLine->text()+ch);

	if (CalcLine->text()==".")
		CalcLine->setText("0.");

	Value=CalcLine->text().toDouble();
}

