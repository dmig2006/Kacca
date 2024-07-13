/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qvalidator.h>
#include <qapplication.h>
#include <qpainter.h>
#include <qstyle.h>

#include <algorithm>
#include <stdio.h>

#include "ExtControls.h"
#include "Utils.h"
#include "WorkPlace.h"
#include "Cash.h"

extern unsigned short RecodeTable[RecodeTableSize][2];

//Extended line edit class

ExtQLineEdit::ExtQLineEdit(QWidget* parent,const char* name,vector<int> codes,vector<int> prefix,vector<int> postfix):
	QLineEdit(parent,name)//extended line edit
{
	valid = new QDoubleValidator(0,1e15,3,this);//only number's are valid
	setValidator(valid);
	shift = IsPrefixCode = ctrl = false;
	PrefixSeq=prefix;
	PostfixSeq=postfix;
	Codes=codes;
};

ExtQLineEdit::~ExtQLineEdit()
{
	delete valid;
};

void ExtQLineEdit::keyPressEvent ( QKeyEvent * e)
{
//	вычисление префиксов и постфиксов
//	static int buf[1024];
//	static int *ptr = buf;
//	*(ptr++) = e->key();

/*
	{
		char s[64];
		sprintf(s, "ExtQLineEdit::keyPressEvent      %d", e->key());
		Log(s);
	}
*/

	if ((e->key()==Key_Up)||(e->key()==Key_Down)||(e->key()==Key_PageUp)||(e->key()==Key_PageDown))
		emit keyPressed(e->key());

	ScanCodesFlow.insert(ScanCodesFlow.begin(),e->key());
	if (ScanCodesFlow.size()>100)
		ScanCodesFlow.resize(100);

	//by using prefix and postfix cut barcode and process it (if we have keyboard'scanner)

	if ((!PrefixSeq.empty())&&(!PostfixSeq.empty()))
	{
		vector<int>::iterator PrefixIterator,PostfixIterator;
		PrefixIterator=std::search(ScanCodesFlow.begin(),ScanCodesFlow.end(),PrefixSeq.begin(),PrefixSeq.end());
		PostfixIterator=std::search(ScanCodesFlow.begin(),ScanCodesFlow.end(),PostfixSeq.begin(),PostfixSeq.end());

		if ((PrefixIterator!=ScanCodesFlow.end())&&(!IsPrefixCode))
		{
			IsPrefixCode=true;
			clear();

			ScanCodesFlow.clear();
			return;
		}

		if ((IsPrefixCode)&&(PostfixIterator!=ScanCodesFlow.end()))
		{
			IsPrefixCode=false;

			QString Barcode;

			for (int i=0;i<KeysFromScanner.size()-PostfixSeq.size()+1;i++)
				Barcode+=KeysFromScanner[i];

			KeysFromScanner.clear();
			ScanCodesFlow.clear();

			emitBarcode(Barcode);
			return;
		}

		if (IsPrefixCode)
		{
			KeysFromScanner.insert(KeysFromScanner.end(),e->text()) ;
			return;
		}
	}

	if (!IsPrefixCode)
		if (std::find(Codes.begin(),Codes.end(),e->key())!=Codes.end()){
			shift = (ShiftButton & e->state());
			ctrl =  (ControlButton & e->state());
			emitKey(e->key());
		}else{
			emitKey(0);//e->key());//
			QLineEdit::keyPressEvent(e);
		}
}

void ExtQLineEdit::emitBarcode(QString barcode)
{
	;;Log("Start barcode");
	setText(barcode);
//	if(shift){// если это штрих с чека для возврата (установлен шифт)
//		returnPressed();
//	}else{
		//ShowMessage(NULL,"emitBarcode");
		emit barcodeFound();
//	}
}

void ExtQLineEdit::emitKey(int key)
{
//	ShowMessage(NULL,Int2Str(key));
	emit keyPressed(key);
}

//extended list view control
SelQListView::SelQListView(QWidget* parent,const char* name,int type):QListView( parent, name )
{
	ListViewType=type;
	Control_pressed=Alt_pressed=Shift_pressed=false;
	keytable_switcher=alt_shift_press;

	rus_keytable_choosed=true;
};

void SelQListView::keyPressEvent(QKeyEvent *e)
{
	if (ListViewType==Prices){//()
		focusNextPrevChild(true);
		return;
	}

	if ((ListViewType==PriceList)||(ListViewType==GroupList))
		if (e->key()==Key_BackSpace)//if backspace was pressed we clear current filter
		{
			FilterStr="";
			setSelected(firstChild(),true);
			QListView::keyPressEvent(e);
			return;
		}

	if ((ListViewType==PriceList)||(ListViewType==GroupList))
		if ((e->key()!=Key_Up)&&(e->key()!=Key_Down)&&(e->key()!=Key_PageUp)&&(e->key()!=Key_PageDown))
		{
			if (e->key()==Key_Control)	Control_pressed=true;

			if (e->key()==Key_Alt)	Alt_pressed=true;

			if (e->key()==Key_Shift)	Shift_pressed=true;
			if ((keytable_switcher==ctrl_press && Control_pressed==true) ||
				(keytable_switcher==alt_press && Alt_pressed==true) ||

				(keytable_switcher==shift_press && Shift_pressed==true) ||
				(keytable_switcher==alt_shift_press && Alt_pressed==true && Shift_pressed==true) ||
				(keytable_switcher==ctrl_shift_press && Control_pressed==true && Shift_pressed==true))
				rus_keytable_choosed=!rus_keytable_choosed;

			if (rus_keytable_choosed)
			{
				QString txt=e->text();

				for (unsigned int i=0; i<txt.length(); i++)//recode by using special table
				{
    				int pos;
					for (pos=0;pos<RecodeTableSize;pos++)
						if (RecodeTable[pos][0]==txt.at(i).unicode())
							break;
					if (pos<RecodeTableSize)
						txt.at(i)=RecodeTable[pos][1];
				}
				FilterStr+=txt;
			}

			QListViewItem *item=firstChild();//highlight first goods that satisfy filter's condition
			int col;

			if (ListViewType==PriceList)
				col=SelName;
			else
				col=GrName;

			while(item!=NULL)
			{
				if (item->text(col).left(FilterStr.length()).upper()==FilterStr.upper())
				{
					setSelected(item,true);
					break;
				}
				item=item->nextSibling();
			}
		}

	if ((ListViewType==ChequeList))
		if ((e->key()!=Key_Up)&&(e->key()!=Key_Down)&&(e->key()!=Key_PageUp)&&(e->key()!=Key_PageDown))
		{
			((WorkPlace*)parentWidget())->EditLine->setFocus();
			//((WorkPlace*)parentWidget())->CashReg->
			thisApp->sendEvent(
						((WorkPlace*)parentWidget())->EditLine,e);
		}

	QListView::keyPressEvent(e);
}

void SelQListView::keyReleaseEvent(QKeyEvent *e)
{
	if (e->key()==Key_Control)
		Control_pressed=false;
	if (e->key()==Key_Alt)
		Alt_pressed=false;
	if (e->key()==Key_Shift)
		Shift_pressed=false;
	QListView::keyReleaseEvent(e);
}

void QColorLabel::drawContents (QPainter *p)//draw color label (i took it from Qt's source code)
{
	QRect cr = contentsRect();

	int m=indent();
	if (m<0)
	{
		if (frameWidth()>0)
			m=p->fontMetrics().width('x')/2;
		else
			m=0;
	}
	if (m>0)
	{
		if (alignment()&AlignLeft)
			cr.setLeft(cr.left()+m);
		if (alignment()&AlignRight)
			cr.setRight(cr.right()-m);
		if (alignment()&AlignTop)
			cr.setTop(cr.top()+m);
		if (alignment()&AlignBottom)
			cr.setBottom(cr.bottom()-m);
	}
#	if QT_VERSION < 0x030000
	style().drawItem(p,cr.x(),cr.y(),cr.width(),cr.height(),
		alignment(),colorGroup(),isEnabled(),pixmap(),text(),-1,&CurColor);
#	endif
#	if QT_VERSION >= 0x030000
//	QRect tmpRect;
//	tmpRect.left=
//	tmpRect.top
//	tmpRect.
//	tmpRect.

	style().drawItem(p,cr,
		alignment(),colorGroup(),isEnabled(),pixmap(),text(),-1,&CurColor);
#	endif
}

QNameLineEdit::QNameLineEdit(QWidget* parent,const char *name):QLineEdit(parent,name)
{
  Control_pressed=Alt_pressed=Shift_pressed=false;
	keytable_switcher=alt_shift_press;
	rus_keytable_choosed=true;
};

void QNameLineEdit::keyPressEvent(QKeyEvent *e)
{
	if (e->key()==Key_Control)
		Control_pressed=true;
	if (e->key()==Key_Alt)
		Alt_pressed=true;
	if (e->key()==Key_Shift)
		Shift_pressed=true;
	if ((keytable_switcher==ctrl_press && Control_pressed==true) ||
		(keytable_switcher==alt_press && Alt_pressed==true) ||
		(keytable_switcher==shift_press && Shift_pressed==true) ||
		(keytable_switcher==alt_shift_press && Alt_pressed==true && Shift_pressed==true) ||
		(keytable_switcher==ctrl_shift_press && Control_pressed==true && Shift_pressed==true))
		rus_keytable_choosed=!rus_keytable_choosed;

	QString txt=e->text();
	if ((rus_keytable_choosed)&&(!txt.isEmpty()))
	{
		for (unsigned int i=0; i<txt.length(); i++)
		{
			int pos;
			for (pos=0;pos<RecodeTableSize;pos++)
				if (RecodeTable[pos][0]==txt.at(i).unicode())
					break;
			if (pos<RecodeTableSize)
			{
				txt.at(i)=RecodeTable[pos][1];
				insert(txt);
			}
			else
				QLineEdit::keyPressEvent(e);
		}
	}
	else
		QLineEdit::keyPressEvent(e);
}

void QNameLineEdit::keyReleaseEvent(QKeyEvent *e)
{
	if (e->key()==Key_Control)
		Control_pressed=false;
	if (e->key()==Key_Alt)
		Alt_pressed=false;
	if (e->key()==Key_Shift)
		Shift_pressed=false;
	QLineEdit::keyReleaseEvent(e);
}

void ColorListViewItem::paintCell(QPainter *p, const QColorGroup &cg,
			int column, int width, int align)
{
    QColorGroup cc = cg;
    if (TextColor.isValid())
        cc.setColor(QColorGroup::Text, TextColor);
    // рисуем
    QListViewItem::paintCell(p, cc, column, width, align);
}

