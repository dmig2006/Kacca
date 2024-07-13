/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <math.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qheader.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qmessagebox.h>

#include "Return.h"
#include "Utils.h"
#include "Cash.h"

enum {ChosenCount=0,ItemNum,ItemCode,ItemName,ItemPrice,ItemCount,ItemSum,ItemAlco,Actsiz,};//columns numbers


/*
 *  Constructs a ReturnForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
ReturnForm::ReturnForm( QWidget* parent ) : QDialog( parent, "ReturnForm", true )
{

    CashReg = (CashRegister*)parent;

	setName( "ReturnForm" );
	resize( 550, 360 );
	setProperty( "minimumSize", QSize( 590, 360 ) );
	setProperty( "maximumSize", QSize( 590, 360 ) );
	setProperty( "baseSize", QSize( 590, 360 ) );
	setCaption(W2U("APM KACCA"));

	ReturnList = new QListView( this, "ReturnList" );
	ReturnList->addColumn( W2U("Выбор"),50 );
	ReturnList->addColumn( W2U(""),0);
	ReturnList->addColumn( W2U("  Код"),55);
	ReturnList->addColumn( W2U("                                Название"),280 );
	ReturnList->addColumn( W2U("Цена"),42 );
	ReturnList->addColumn( W2U("Кол-во"),50 );
	ReturnList->addColumn( W2U("Сумма"),52 );
	ReturnList->addColumn( W2U("ЕГАИС"),20 );
	ReturnList->addColumn( W2U("Марка"),20 );
	ReturnList->header()->setClickEnabled(false);
	ReturnList->header()->setResizeEnabled(false);
	ReturnList->header()->setMovingEnabled(false);
	ReturnList->setGeometry( QRect( 0, 0, 590, 310 ) );
	ReturnList->setProperty( "selectionMode", (int)QListView::Single );
	ReturnList->setProperty( "allColumnsShowFocus", QVariant( true, 0 ) );
	ReturnList->setSorting(-1);

	PushButton1 = new QPushButton( this, "PushButton1" );
	PushButton1->setGeometry( QRect( 80, 320, 93, 26 ) );
	PushButton1->setProperty( "text", W2U("Возврат") );

	PushButton2 = new QPushButton( this, "PushButton2" );
	PushButton2->setGeometry( QRect( 214, 320, 93, 26 ) );
	PushButton2->setProperty( "text",  W2U("Отмена") );

	SelectAllBox = new QCheckBox( this, "SelectAllBox" );
	SelectAllBox->setGeometry( QRect( 340, 320, 150, 20 ) );
	SelectAllBox->setProperty( "text", W2U("Выделить весь чек") );

	// signals and slots connections
	connect( ReturnList, SIGNAL( returnPressed(QListViewItem*) ), this, SLOT( ReturnListKeyDown(QListViewItem*) ) );
	connect( PushButton1, SIGNAL( clicked() ), this, SLOT( ReturnClicked() ) );
	connect( PushButton2, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( SelectAllBox, SIGNAL( clicked() ), this, SLOT( SelectAllBoxClick() ) );

	ReturnCheque = new FiscalCheque;
}

/*
 *  Destroys the object and frees any allocated resources
 */
ReturnForm::~ReturnForm()
{
	// no need to delete child widgets, Qt does it all for us
	delete ReturnCheque;
}

#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qlabel.h>
#include <qvalidator.h>
#include <string>

#include "Sum.h"
#include "Utils.h"

void ReturnForm::ReturnListKeyDown(QListViewItem* item)//if return key was pressed we increment
{			//number of the current item or set it null if we reached the quantity of this goods

#ifdef ALCO_VES
    Log("Возврат");
#endif

	// проверка на штучность
	if((!strchr((const char *)item->text(ItemCount), ',')) && (item->text(ItemCount).toInt()>1))
	{//(fabs((*ReturnCheque)[atoi(item->text(ItemNum).latin1())-1].ItemPrecision-1)<epsilon){
		SumForm dlg(this);//show simple dialog and get check's number
		dlg.setCaption("APM KACCA");
		dlg.FormCaption->setText(W2U("Количество товара"));
#ifdef ALCO_VES
    Log("Возврат штуки");
#endif
		QIntValidator valid(dlg.LineEdit);
		valid.setRange(0,item->text(ItemCount).toInt());

		dlg.LineEdit->setValidator(&valid);
		dlg.LineEdit->setMaxLength(9);

		PushButton1->setDefault(false);

		if(dlg.exec() != QDialog::Accepted)
			return;
		if(dlg.LineEdit->text().toDouble() > Str2Double((const char *)item->text(ItemCount)))
			return;
		ReturnList->currentItem()->setText(ChosenCount,dlg.LineEdit->text());
	}else{


	    PushButton1->setDefault(false);

		if(ReturnList->currentItem()->text(ChosenCount).isEmpty())
		{



            // + EGAIS
            vector<string> ActsizList;
            string ActsizBarcode = "";
//            if (CashReg->EgaisSettings.ModePrintQRCode>-1)
            {

                int ActsizType = ReturnList->currentItem()->text(ItemAlco).toInt();

#ifdef ALCO_VES
                Log("ActsizType=" + Int2Str(ActsizType));
#endif

                if (ActsizType)
                { // Возврат алкогольной продукции

///Просит считать маркировку иначе не заходим в цикл

                    UpdateRetCheck(); // обновим информацию в возвратном чеке

                    int errAct = -1;

                    switch (ActsizType) {
                        case AT_ALCO: {
                            errAct = CashReg->CheckActsizBarcode(&ActsizBarcode, U2W(item->text(ItemCode))+" "+ U2W(item->text(ItemName)), ResultReturnCheck, ActsizList);
                            break;
                        }
                        case AT_CIGAR: {
                            errAct = CashReg->CheckActsizBarcodeDataMatrix(&ActsizBarcode, U2W(item->text(ItemCode))+" "+ U2W(item->text(ItemName)), ResultReturnCheck, ActsizList, AT_CIGAR);
                            break;
                        }
                        case AT_TEXTILE: {
                            errAct = CashReg->CheckActsizBarcodeDataMatrix(&ActsizBarcode, U2W(item->text(ItemCode))+" "+ U2W(item->text(ItemName)), ResultReturnCheck, ActsizList, AT_TEXTILE);
                            break;
                        }
                        case AT_MILK: {
                            errAct = CashReg->CheckActsizBarcodeDataMatrix(&ActsizBarcode, U2W(item->text(ItemCode))+" "+ U2W(item->text(ItemName)), ResultReturnCheck, ActsizList, AT_MILK);
                            break;
                        }
#ifdef ALCO_VES
                        case AT_PIVO_RAZLIV:{
                            errAct = CashReg->CheckActsizBarcodeDataMatrix(&ActsizBarcode, U2W(item->text(ItemCode))+" "+ U2W(item->text(ItemName)), ResultReturnCheck, ActsizList, AT_PIVO_RAZLIV);
                            break;
                        }
#endif
                    }

                    if (errAct) return;

                    ReturnList->currentItem()->setText(Actsiz,W2U(ActsizBarcode));
                }

            }
            // - EGAIS

			ReturnList->currentItem()->setText(ChosenCount,item->text(ItemCount));
		}
        else
            ReturnList->currentItem()->setText(ChosenCount,QString(""));
	}

}


void ReturnForm::UpdateRetCheck()
{
    ResultReturnCheck->Clear();

	QListViewItem *item=ReturnList->firstChild();

	for (unsigned int i=0;i<ReturnCheque->GetCount();i++)
	{
	    double value = Str2Double((const char *)item->text(ChosenCount));//.toDouble();//if we have single goods

        // Чек на возврат
        if (fabs(value)>=epsilon)//we add it with quantity
        {
            GoodsInfo tmpInfo=(*ReturnCheque)[i];//that equal to full quantity

            if (tmpInfo.ItemCount!=value)
            {
                tmpInfo.ItemSum = RoundToNearest(tmpInfo.ItemSum/tmpInfo.ItemCount * value,0.01);
                tmpInfo.ItemCalcSum = tmpInfo.ItemSum;
                tmpInfo.ItemFullSum = tmpInfo.ItemSum;

                tmpInfo.ItemCount=value;
                //tmpInfo.CalcSum();
            }

            // + dms
            tmpInfo.ItemFullDiscSum = tmpInfo.ItemSum;
            // - dms

            tmpInfo.ItemActsizBarcode = (const char *)item->text(Actsiz);

            tmpInfo.Actsiz.clear();
            tmpInfo.Actsiz.insert(tmpInfo.Actsiz.end(), tmpInfo.ItemActsizBarcode );


            ResultReturnCheck->Insert(tmpInfo);
        }

		item=item->nextSibling();
	}

}

void ReturnForm::SelectAllBoxClick()//set maximum quantity for each goods
{
	QListViewItem *item=ReturnList->firstChild();

	if (SelectAllBox->isChecked())
	{
		do
		{
            int ActsizType = item->text(ItemAlco).toInt();
            if (!ActsizType)
		    //if (item->text(ItemAlco).isEmpty())
		    {
                item->setText(ChosenCount,item->text(ItemCount));
		    }
            item=item->nextSibling();
		}
		while (item!=NULL);
	}
	else
	{
		do
		{
			item->setText(ChosenCount,"");
			item=item->nextSibling();
		}
		while (item!=NULL);
	}
}

void ReturnForm::ShowCheque(FiscalCheque *CurCheque)//show given check
{
	if (CurCheque->GetCount()==0)
		return;

	ReturnCheque->Clear();
	QListViewItem *item;
	unsigned int i;
	for (i=CurCheque->GetCount()-1;(i>=0)&&(i<CurCheque->GetCount());i--)
	{
		item = new QListViewItem(ReturnList);
		item->setText(ChosenCount,"");
		item->setText(ItemNum,Int2Str(i+1).c_str());
		item->setText(ItemCode,Int2Str((*CurCheque)[i].ItemCode).c_str());
		item->setText(ItemName,W2U((*CurCheque)[i].ItemName));
		item->setText(ItemPrice,GetRounded((*CurCheque)[i].ItemPrice,0.01).c_str());

		double prec = (*CurCheque)[i].ItemPrecision;
		double cnt = (*CurCheque)[i].ItemCount;
		if ((prec==1) && (fabs( cnt-floor(cnt) )>1e-3)) prec = 0.001;

		item->setText(ItemCount,GetRounded((*CurCheque)[i].ItemCount, prec).c_str());
		item->setText(ItemSum,GetRounded((*CurCheque)[i].ItemSum,0.01).c_str());


        int ActsizType = AT_NONE;
        if ((*CurCheque)[i].ItemAlco) {
            ActsizType = AT_ALCO;
        } else {
            if (((*CurCheque)[i].ItemCigar) && ((*CurCheque)[i].ItemActsizType == AT_CIGAR)) {
                ActsizType = AT_CIGAR;
            } else {
                if ((*CurCheque)[i].ItemTextile) {
                    ActsizType = AT_TEXTILE;
                } else {
                    if ((*CurCheque)[i].ItemMilk) {
                        ActsizType = AT_MILK;
                    } else {
#ifdef ALCO_VES             
							if ((*CurCheque)[i].ItemPivoRazliv) {
                            ActsizType = AT_PIVO_RAZLIV;
                        }
#endif
                    }
                }

            }
        }

		item->setText(ItemAlco,W2U(QString(Int2Str(ActsizType))));
		item->setText(Actsiz,W2U(QString("")));
	}

	for (i=0;i<CurCheque->GetCount();i++)
		ReturnCheque->Insert((*CurCheque)[i]);

	ReturnList->setFocus();

	ReturnList->setSelected(ReturnList->firstChild(),true);
}

void ReturnForm::ReturnClicked(void)//if 'return' button was pressed we create the new check
{
	//which contains all non-returned goods
	bool GoodsWasNotSelected=true;
	QListViewItem *item=ReturnList->firstChild();

    ResultReturnCheck->Clear();

	for (unsigned int i=0;i<ReturnCheque->GetCount();i++)
	{
	    double value = Str2Double((const char *)item->text(ChosenCount));//.toDouble();//if we have single goods

        // Чек на возврат
        if (fabs(value)>=epsilon)//we add it with quantity
        {
            GoodsInfo tmpInfo=(*ReturnCheque)[i];//that equal to full quantity

            if (tmpInfo.ItemCount!=value)
            {
                tmpInfo.ItemSum = RoundToNearest(tmpInfo.ItemSum/tmpInfo.ItemCount * value,0.01);
                tmpInfo.ItemCalcSum = tmpInfo.ItemSum;
                tmpInfo.ItemFullSum = tmpInfo.ItemSum;

                tmpInfo.ItemSumToPay = tmpInfo.ItemSum;

                tmpInfo.ItemCount=value;
                //tmpInfo.CalcSum();
            }

            // + dms
            tmpInfo.ItemFullDiscSum = tmpInfo.ItemSum;
            // - dms

            tmpInfo.ItemActsizBarcode = (const char *)item->text(Actsiz);

            tmpInfo.Actsiz.clear();
            tmpInfo.Actsiz.insert(tmpInfo.Actsiz.end(), tmpInfo.ItemActsizBarcode );

            ResultReturnCheck->Insert(tmpInfo);
        }


		if (item->text(ChosenCount).isEmpty() || value < epsilon)
		{
			NewCheque->Insert((*ReturnCheque)[i]);//if current goods was not selected
			item=item->nextSibling();				//we add it at whole
			continue;
		}

		GoodsWasNotSelected=false;

		if (fabs((*ReturnCheque)[i].ItemPrecision-1)<epsilon)
		{
			if (fabs((*ReturnCheque)[i].ItemCount-value)>=epsilon)//we add it with quantity
			{
				GoodsInfo tmpInfo=(*ReturnCheque)[i];//that equal to full quantity minus
				tmpInfo.ItemCount-=value;				//selected quantity

				tmpInfo.CalcSum();

				// + dms
				tmpInfo.ItemFullDiscSum = tmpInfo.ItemSum;
				tmpInfo.ItemSumToPay = tmpInfo.ItemSum;
				// - dms

				NewCheque->Insert(tmpInfo);
			}
		}

		item=item->nextSibling();
	}

	if (GoodsWasNotSelected)
	{
		ShowMessage(this,"Не выбраны товары для возврата!");
		NewCheque->Clear();
		ResultReturnCheck->Clear();
		ReturnList->setFocus();
	}
	else
		accept();
}

