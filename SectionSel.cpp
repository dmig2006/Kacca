/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qheader.h>
#include <qlistview.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "SectionSel.h"
#include "ExtControls.h"
#include "Utils.h"

/*
 *  Constructs a SectionSelForm which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SectionSelForm::SectionSelForm( QWidget* parent) : QDialog( parent, "SectionSelForm", true )
{
	setName( "SectionSelForm" );
	resize(parent->width()-40, parent->height()-60);
	setProperty( "minimumSize", QSize(parent->width()-40, parent->height()-60) );
	setProperty( "maximumSize", QSize(parent->width()-40, parent->height()-60) );
	setProperty( "baseSize", QSize(parent->width()-40, parent->height()-60) );
	setCaption(W2U("APM KACCA"));

	QFont form_font( font() );
	form_font.setPointSize( 8 );
	setFont( form_font );

	SectionSelList = new SelQListView( this, "SectionSelList", GroupList);
	SectionSelList->addColumn( W2U( "" ),0 );
	SectionSelList->header()->setClickEnabled( false, SectionSelList->header()->count() - 1 );
	SectionSelList->header()->setResizeEnabled( false, SectionSelList->header()->count() - 1 );
	SectionSelList->addColumn( W2U("ÊÎÄ"),64 );
	SectionSelList->header()->setClickEnabled( FALSE, SectionSelList->header()->count() - 1 );
	SectionSelList->header()->setResizeEnabled( FALSE, SectionSelList->header()->count() - 1 );
	SectionSelList->addColumn( W2U( "ÍÀÈÌÅÍÎÂÀÍÈÅ" ),parent->width()-SectionSelList->verticalScrollBar()->width());
	SectionSelList->header()->setClickEnabled( FALSE, SectionSelList->header()->count() - 1 );
	SectionSelList->header()->setResizeEnabled( FALSE, SectionSelList->header()->count() - 1 );
	SectionSelList->setGeometry( QRect( 0, 0,parent->width()-40, parent->height()-60) );
	SectionSelList->setProperty( "selectionMode", (int)QListView::Single );
	SectionSelList->setProperty( "allColumnsShowFocus", QVariant( true, 0 ) );
	SectionSelList->setSorting(-1);
	SectionSelList->setHScrollBarMode(QListView::AlwaysOff);
	SectionSelList->setVScrollBarMode(QListView::AlwaysOn);

	SectionSelList->setFocus();

	// signals and slots connections
	connect( SectionSelList, SIGNAL( doubleClicked(QListViewItem*) ), this, SLOT( ReturnPress(QListViewItem*) ) );
	connect( SectionSelList, SIGNAL( returnPressed(QListViewItem*) ), this, SLOT( ReturnPress(QListViewItem*) ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
SectionSelForm::~SectionSelForm()
{
    // no need to delete child widgets, Qt does it all for us
}

void SectionSelForm::ReturnPress(QListViewItem* item)
{
	*SelRow=item->text(GrNum).toInt()-1;
	accept();
}

void SectionSelForm::ShowSectionList(vector<int> GrCodes,vector<string> S_Names)
{
	if ((GrCodes.size()!=S_Names.size())||(GrCodes.empty()))
		return;

	QListViewItem *item;
	for (unsigned int i=GrCodes.size()-1;(i>=0)&&(i<GrCodes.size());i--)
	{
		item= new QListViewItem(SectionSelList);
		item->setText(GrNum,Int2Str(i+1).c_str());
		item->setText(GrCode,Int2Str(GrCodes[i]).c_str());
		item->setText(GrName,W2U(S_Names[i]));
	}
	SectionSelList->setSelected(SectionSelList->firstChild(),true);
}

