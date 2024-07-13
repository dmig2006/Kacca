/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "Message.h"

#include <qregexp.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qlistview.h>
#include <qheader.h>
#include <qheader.h>
#include <qcheckbox.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
//#include <qtextedit.h>

#include "ExtControls.h"
#include "Utils.h"



string readln(FILE* f)
{
    char buf[1024];

    fgets(buf, 1024, f);

    char * nl;
	nl = strrchr(buf, 0x0d);//'\n');
	if(nl) *nl = '\0';
	nl = strrchr(buf, 0x0a);//'\n');
	if(nl) *nl = '\0';

    string ret = buf;
    return ret;
}

/*
 *  Constructs a MsgForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
MsgForm::MsgForm( QWidget* parent, int CurItem)  : QDialog( parent, "APM KACCA", true)
{
	resize( 420, 274 );
	setCaption(W2U("APM KACCA"));

	//история сообщений
	ListHistory = new QListView( this, "ListHistory" );
	ListHistory->addColumn( W2U( "Cообщения" ),400 );
	ListHistory->header()->setClickEnabled( false, ListHistory->header()->count() - 1 );

	ListHistory->setGeometry( QRect( 2, 2, 416, 220 ));

    ListHistory->setSorting(-1);


	EditLine = new QNameLineEdit( this, "EditLine" );

	EditLine->setGeometry( QRect( 2, 224, 418, 24 ) );
	EditLine->setBackgroundColor(ListHistory->backgroundColor());


	NumberBox = new QComboBox( FALSE, this, "NumberBox" );


    FILE * f = fopen("./tmp/list.msg", "r");
    if(!f)
    {
        for(int i=1;i<=15;i++)
            NumberBox->insertItem( W2U("apm-kacca"+Int2Str(i)) );
    }
    else
    {
        QString str = "";
        while (!feof(f))
        {
            str = W2U(readln(f));
            NumberBox->insertItem( str );
        }

        fclose(f);
    }
	NumberBox->setGeometry( QRect( 2, 250, 150, 24 ) );

	NumberBox->setCurrentItem(CurItem);


	SaveHistory = new QCheckBox( this, "ClearHistory" );
	SaveHistory->setGeometry( QRect( 254, 250, 160, 24 ) );
	SaveHistory->setText(W2U("Сохранять историю"));


	connect( EditLine, SIGNAL( returnPressed() ), this, SLOT( accept() ) );

	EditLine->setFocus();
}


int MsgForm::FillHistory(const char* msgfile)
{
    int LineLenth=50;
    ListHistory->clear();
    try
    {
        FILE * f = fopen(msgfile, "r");
        if (!f) return 1;
        QString Text="";
        QListViewItem *LastItem = NULL;

        QColor color = Qt::black;
        while (!feof(f))
        {
            Text = W2U(readln(f));

            if (Text.left(2)==">>")
                color = Qt::darkMagenta;
            else
                if (Text.left(2)=="<<")
                    color = Qt::darkBlue;

            QString CurText = Text;

            int lastpos=0;

            QRegExp* fstr = new QRegExp("[\\s,.!?&]");
            while (!Text.isEmpty())
            {
                if (Text.length()>LineLenth)
                {
                    CurText = Text.left(LineLenth);

                    lastpos = CurText.findRev(QRegExp("[\\s,.!?&]"));

                    if (0<lastpos) CurText = CurText.left(lastpos);
                }
                else
                {
                    CurText = Text;
                }

                ColorListViewItem *item=new ColorListViewItem(ListHistory, LastItem, color);//add goods to the cheque list

                item->setText(0, CurText);

                LastItem = item;

                Text = Text.mid(CurText.length());

            }
            delete fstr;
        }

        ListHistory->ensureItemVisible(LastItem);
        ListHistory->setSelected(LastItem,true);
        ListHistory->setCurrentItem(LastItem);
        fclose(f);

    }
	catch(int &er)
	{

	}

}

/*
 *  Destroys the object and frees any allocated resources
 */
MsgForm::~MsgForm()
{
    // no need to delete child widgets, Qt does it all for us

//    LastMessageItem = NumberBox->currentItem();
//
//    if (!EditLine->text().isEmpty())
//    {
//        LastMessageText = EditLine->text();
//    }
}


