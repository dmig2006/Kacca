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

#include "PriceSel.h"
#include "Utils.h"
#include "ExtControls.h"

/*
 *  Constructs a PriceSelForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PriceSelForm::PriceSelForm( QWidget* parent ) : QDialog( parent, "PriceSelForm", true )
{
	int parentwidth,parentheight;
	if(parent)
	{
		parentwidth=parent->width();
		parentheight=parent->height();
	}
	else
	{
		parentwidth  = 800;
		parentheight = 600;
	}

	setName( "PriceSelForm" );
	resize(parentwidth-40, parentheight-60 );
	setProperty( "minimumSize", QSize(parentwidth-40, parentheight-60 ) );
	setProperty( "maximumSize", QSize(parentwidth-40, parentheight-60 ) );
	setProperty( "baseSize",    QSize(parentwidth-40, parentheight-60 ) );
	setCaption(W2U("APM KACCA"));

	QFont form_font( font() );
	form_font.setPointSize( 8 );
	setFont( form_font );

	PriceSelList = new SelQListView( this, "PriceSelList",PriceList );
	PriceSelList->addColumn( W2U( "" ),0);
	PriceSelList->header()->setClickEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->header()->setResizeEnabled( false, PriceSelList->header()->count() - 1 );

	PriceSelList->addColumn( W2U( "" ),0);
	PriceSelList->header()->setClickEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->header()->setResizeEnabled( false, PriceSelList->header()->count() - 1 );

	PriceSelList->addColumn( W2U( "ÊÎÄ" ),40 );
	PriceSelList->header()->setClickEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->header()->setResizeEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->addColumn( W2U( "ØÒÐÈÕ-ÊÎÄ" ),100);
	PriceSelList->header()->setClickEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->header()->setResizeEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->addColumn( W2U( "ÍÀÈÌÅÍÎÂÀÍÈÅ" ),parentwidth-180-PriceSelList->verticalScrollBar()->width());
	PriceSelList->header()->setClickEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->header()->setResizeEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->addColumn( W2U( "ÊÎË-ÂÎ" ),40 );
	PriceSelList->header()->setClickEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->header()->setResizeEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->addColumn( W2U( "ÖÅÍÀ" ),40 );
	PriceSelList->header()->setClickEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->header()->setResizeEnabled( false, PriceSelList->header()->count() - 1 );
	PriceSelList->setGeometry( QRect( 0, 0, parentwidth-40,parentheight-60 ) );
	PriceSelList->setProperty( "selectionMode", (int)QListView::Single );
	PriceSelList->setProperty( "allColumnsShowFocus", QVariant( true, 0 ) );
	PriceSelList->setSorting(3);
	PriceSelList->setHScrollBarMode(QListView::AlwaysOff);
	PriceSelList->setVScrollBarMode(QListView::AlwaysOn);
	PriceSelList->setFocus();

	// signals and slots connections
	connect( PriceSelList, SIGNAL( doubleClicked(QListViewItem*) ), this, SLOT( ReturnPress(QListViewItem*) ) );
	connect( PriceSelList, SIGNAL( returnPressed(QListViewItem*) ), this, SLOT( ReturnPress(QListViewItem*) ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
PriceSelForm::~PriceSelForm()
{
	// no need to delete child widgets, Qt does it all for us
}

void PriceSelForm::ReturnPress(QListViewItem* item)//if return key was pressed or double click
{										//took place we store	number of the selected item and close dialog
    if (item->text(SelLock)=="X")
        ShowMessage(PriceSelList->focusWidget(), "Íåäîñòàòî÷íî ïðàâ");
    else if (item->text(SelLock)=="T")
        ShowMessage(PriceSelList->focusWidget(), "Ïðîäàæà òîâàðà çàïðåùåíà")        ;
    else
    {
        *SelRow=item->text(SelNo).toInt()-1;
        accept();
    }
}

void PriceSelForm::ShowGoodsList(FiscalCheque *goods)//show goods according to given check
{
	if (goods->GetCount()==0)
		return;

	QListViewItem *item;

	for (unsigned int i=goods->GetCount()-1;(i>=0)&&(i<goods->GetCount());i--)
	{
		//item=new QListViewItem(PriceSelList);

		QColor color = Qt::black;
		string charlock = " ";
		switch ((*goods)[i].LockCode)
		{
            case GL_TimeLock: color=Qt::red; charlock="T"; break;
            case GL_FindByCode:
            case GL_FindByName:
            case GL_FindByPrice: color=Qt::gray; charlock="X"; break;
		}

		item=new ColorListViewItem(PriceSelList, color);

		item->setText(SelNo,Int2Str(i+1).c_str());
		item->setText(SelLock,charlock.c_str());
		item->setText(SelCode,Int2Str((*goods)[i].ItemCode).c_str());
		item->setText(SelBarcode,(*goods)[i].ItemBarcode.c_str());
		item->setText(SelName,W2U((*goods)[i].ItemName));
// + dms ÏËÒÕÇÌÑÅÍ ÄÏ 3 ÚÎÁËÏ× (ÂÙÌÏ 2: 0.01)
		item->setText(SelMult,GetRounded((*goods)[i].ItemMult,0.001).c_str());
// - dms
		item->setText(SelPrice,GetRounded((*goods)[i].ItemPrice,0.01).c_str());
	}
	PriceSelList->setSelected(PriceSelList->firstChild(),true);
}



