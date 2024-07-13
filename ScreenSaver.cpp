/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qheader.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qapplication.h>
#include <stdio.h>
#include <qimage.h>
#include <qsizepolicy.h>
#include <qpalette.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qfile.h>


#include <stdlib.h> // дл€ system

//#include <string>
#include <vector>

using namespace std;

#include "ScreenSaver.h"

#include "Utils.h"
#include "ExtControls.h"


/*
 *  Constructs a ScreenSaver which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
ScreenSaverForm::ScreenSaverForm( QWidget* parent) : QDialog( parent, "ScreenSaverForm", true ,
WStyle_Customize | WType_Popup | WStyle_NormalBorder)
{

	WPlace=(WorkPlace*)parent;

	WSize = WPlace->size();
    CurNumber = max(0, WPlace->PictureNumber);

    int w, h;
    w = WSize.width()>0 ? WSize.width() + 10 : 760;
    h = WSize.height()>0 ? WSize.height() + 26 : 570;

    move(1, 1);
    setName( "ScreenSaver" );
	setFixedSize( w, h );
	setProperty( "minimumSize", QSize( w, h) );
	setProperty( "maximumSize", QSize( w, h) );
	setProperty( "baseSize", QSize( w, h) );

    setBackgroundColor(Qt::black);

	setBaseSize(w,h);

    setWFlags(WStyle_Customize);

    logo = "/Kacca/logo.jpg";

    path = W2U(WPlace->CashReg->DBPicturePath)+"/";

    if (path.isEmpty())
        path = "./pic/";

    if (filename.isEmpty())
        filename = "pic2.jpg";

    char cmd[256];
	sprintf( cmd, "find %s -type f \\( -name \"*.jpg\" -o -name \"*.bmp\" -o -name \"*.png\" -o -name \"*.gif\" \\) > piclist", path.ascii());
	int res = system(cmd);


	LabelImage = new QLabel(this, "LabelImage");
	LabelImage->setFixedSize(size());
	LabelImage->setProperty( "alignment", int( QLabel::AlignCenter ) );
    LabelImage->setBackgroundColor(Qt::black);
    LabelImage->setMargin(5);

    Next();

	Timer = new QTimer(this);//start clock

	connect( Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	Timer->start(5*1000 /* 3*60*1000 */);  // в милисекундах

	Log("ScreenSaver");
}

ScreenSaverForm::~ScreenSaverForm()
{
    delete Timer;

    WPlace->PictureNumber = CurNumber;

    Log("~ScreenSaver");
}

void ScreenSaverForm::keyPressEvent(QKeyEvent* e)//analogously
{
    switch (e->key())
    {
        case Qt::Key_Home:	    CurNumber=0;
        case Qt::Key_PageDown:	Next();	break;
        case Qt::Key_PageUp:	Prev();	break;
        case Qt::Key_Escape:    WPlace->ShowScreenSaver = false;
        default: QCloseEvent ev; QDialog::closeEvent(&ev); break;
    }
}

void ScreenSaverForm::ProcessKey(int key)
{

}


int ScreenSaverForm::FindNextFile(void)
{

    filename = logo;
    string PrevFileName = U2W(logo);

	QFile *f = new QFile("piclist");
	if (!f->exists())
	{
        delete f;
		return -1;
	}


	if (!f->open(IO_ReadOnly))
	{
		delete f;
		return -1;
	}

    int res, num=0;

	do
	{
		char buffer[1024];
		memset(buffer,0,1024);
		res = f->readLine(buffer, 1024);
		if (res!=-1) PrevFileName = buffer;
		num++;
	}
	while((res!=-1)&&(CurNumber>=num));

    //if (!er)
    filename = W2U(PrevFileName).stripWhiteSpace();

    if (res==-1) CurNumber=0;

	f->close();

	delete f;

	return 0;
}

void ScreenSaverForm::Next(void)
{
    QPixmap pic;

    int er = FindNextFile();

//;;Log((char*)("& "+U2W(filename)).c_str());

    QImage img(filename);
    if (! pic.convertFromImage( img ))
    {
        QImage img_logo(logo);
        pic.convertFromImage( img_logo );
    }

    //img.smoothScale(w, h);

    int ih, iw, h, w;

    ih = img.size().height();
    iw = img.size().width();

    h = LabelImage->size().height();
    w = LabelImage->size().width();

    bool usescale = false;

    int dh = h - ih,
        dw = w - iw;

    if ( (dh<0) && (dw<0) ) // картинка не умещаетс€
    {
       usescale = true; //раcт€гиваем
    }

    LabelImage->setPixmap(pic);

    LabelImage->setScaledContents(usescale);

	CurNumber++;

}

void ScreenSaverForm::Prev(void)
{
    if (CurNumber>1) CurNumber-=2;
    Next();
}

void ScreenSaverForm::OnTimer(void)
{
    Next();
}
