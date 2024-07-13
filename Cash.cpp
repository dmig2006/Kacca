/***************************************************************************
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *
#ifdef LOGPACKET
    ShowMessage("Отбить пакет");
#endif
#ifdef LOGNDS
        SalesRecord tmpSales=Sales[0];
        Log("============NDS========Close=======");
        Log("Nds=" + Double2Str(tmpSales.NDS));
        Log("===================================");
        FiscalReg
#endif
*
 ***************************************************************************/
#include <qframe.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qstatusbar.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qpopupmenu.h>
#include <qfile.h>
#include <qdir.h>
#include <qvalidator.h>
#include <qcombobox.h>
#include <qapplication.h>
#include <qfont.h>
#include <qtimer.h>
#include <qprogressbar.h>
#include <qinputdialog.h>
#include <qstringlist.h>
//#include <qimage.h>
#include <qmovie.h>

#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <algorithm>
#include <time.h>

#include <xbase/xbase.h>

#include "Cash.h"
#include "Passwd.h"
#include "Sum.h"
#include "Fiscal.h"
#include "CheckSearch.h"
#include "Return.h"
#include "Login.h"
#include "PortSelect.h"
#include "Utils.h"
#include "WorkPlace.h"
#include "DbfTable.h"
#include "Search.h"
#include "FreeSum.h"
#include "Threads.h"
#include "DateShift.h"
#include "ShiftRange.h"
#include "DateRange.h"
#include "RegTable.h"
#include "Guard.h"
#include "Ping.h"
#include "ExtControls.h"
#include "Payment.h"
#include "Certificate.h"
#include "PriceSel.h"
#include "Actsiz.h"
#include "ChangeForm.h"
#include "EGAISCheck_dlg.h"
extern "C"
{
#include <uuid/uuid.h>
}

#ifndef _WS_WIN_
#include <sys/vfs.h>
#endif
static bool inload = false;
static bool GoodsInLoadFlag = false;
static int ActiveItem = 10;
FILE *ff;

QApplication *thisApp;//this is just a pointer to our application
AbstractFiscalReg *FiscalReg; //abstract fiscal registrator

const char *BankIsInaccessible="Bank server is inaccessible";

//names of dbf-tables
extern const char
        *GOODS,*GOODSPRICE,*NONSENT,*STAFF,*SETTINGS,*DISCDESC,*BANKDISC,*CUSTOMERS,*CUSTRANGE,*NEWGOODS,
        *KEYBOARD,*GROUPS,*BARCONF,*SALES,*CURCHECK,*ACTIONS,*ACTREPORT,*BANKACTIONS,*BANKACTREPORT,*CUSTSALES,*RETURN,
        *FLAG,*CRPAYMENT,*PAYMENT,*PAYMENTTOSEND,*CUSTVAL, *BANKBONUSTOSEND, *BANKBONUS, *ACTSIZFILETOSEND, *ACTSIZFILE,
        *BONUSFILE, *BONUSFILETOSEND,
//Connection messages
        *ServerError,*XBaseError,*BankError,

        *ErrorLog,*DateTemplate,
//indices information
        *IndexFile[],*IndexField[],*IndexFileOld[];
extern int IndexNo;

//Tables description

extern xbSchema ConfigTableRecord[],StaffTableRecord[],SalesTableRecord[],
                GoodsTableRecord[],GoodsPriceTableRecord[],BarInfoTableRecord[],KeyboardTableRecord[],
                GroupsTableRecord[],ActionsTableRecord[],CustomersTableRecord[],CustRangeTableRecord[],
                CustomersSalesRecord[],CustomersValueRecord[],DiscountsRecord[],ReturnRecord[],
                MsgLevelRecord[],BankDiscountRecord[],PaymentTableRecord[], BankBonusTableRecord[],
                ActsizTableRecord[], BonusTableRecord[], BankActionsTableRecord[];

DbfTable * CashRegister::ConfigTable = NULL;

extern int revision;

/*
 *  Constructs a CashRegister which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
CashRegister::CashRegister( QWidget* parent)  : QWidget( parent, "CashRegister", true )
{
    int BaseSkip;
    Log("=== KASSA START ===");
    frmPasswd = NULL;

#ifdef BUTTON_OLD_GOODS
    flagRestoreGoods=false;
#endif

    //============================
    // При загрузке кассового ПО выполняется cоздание и инициализация объекта PCX
    // Объект должен жить до завершения работы кассового ПО
    // Создания и инициализации объекта на каждом запросе не должно быть


    Log(" INIT PCX... ");
    try {

        WPCX = new LProc();
        //! Бонусы СБЕРБАНК отключены!
        //!WPCX->Init();
        Log("  ..SUCCESS ");
    } catch(exception& ex) {
        // обработка ошибки создания объекта PCX
        string err= ex.what();
        err = "Ошибка инициализации PCX: "+err;

        ShowMessage(err);

        Log("  ..ERROR: ");
        Log((char*)ex.what());
    }

    LogMutex = new QMutex(true);
    StatusMutex = new QMutex(true);
    SQLMutex = new QMutex(true);

    ConfigTable = new DbfTable;//create internal dbf tables

    CheckMillenium = false;
    LastMessageText="";
    LastMessageItem=0;
    SaveMessageHistory=false;
//	scanLine = NULL;
    {
       QFile *workFlag = new QFile(".work");
       workFlag->remove();//удалим флаг (если было аварийное завершение)
       delete workFlag;
    }

    try
    {

        if (!QFile(SETTINGS).exists())
        {
            ConfigTable->CreateDatabase(SETTINGS,ConfigTableRecord);
            ConfigTable->BlankRecord();
            ConfigTable->AppendRecord();

            ConfigTable->PutField("MAIN_PATH","/Kacca");
            ConfigTable->PutField("GOODS_PATH","/Kacca/import");
            ConfigTable->PutField("SERVER","");
            ConfigTable->PutLongField("IB_WAIT",30);
            ConfigTable->PutLongField("MAXSPACE",500);// 0.5Mb space for arch
            ConfigTable->PutField("PIC_PATH","/Kacca/pic");
            ConfigTable->PutField("ARCH_PATH","/Kacca/arch");

            ConfigTable->PutRecord();

            MainPathChange();
            ImportPathChange();
            ArchPathChange();
        }
        else
        {
            ConfigTable->OpenDatabase(SETTINGS,ConfigTableRecord);
            ConfigTable->GetFirstRecord();
        };

        DBMainPath=ConfigTable->GetStringField("MAIN_PATH");
        DBImportPath=ConfigTable->GetStringField("GOODS_PATH");
        Cashless = ConfigTable->GetLogicalField("CASHLESS");
        CheckMsgLevel = ConfigTable->GetLogicalField("MSGLEVEL");

        FRKFont5 = ConfigTable->GetLogicalField("FRK_FONT");
        OFDMODE = ConfigTable->GetLogicalField("OFD");
        SBPayPass = ConfigTable->GetLogicalField("SBPAYPASS");

        DBPicturePath=ConfigTable->GetStringField("PIC_PATH");
        if (DBPicturePath=="") DBPicturePath=DBMainPath+"/pic";

        DBArchPath=ConfigTable->GetStringField("ARCH_PATH");
        if (Empty(DBArchPath)) DBArchPath=DBMainPath+"/arch";

        InitDirs();

        //create or open internal dbf tables
        SalesTable->SetUseDeleted(true);
        AttachDbfTable(SalesTable,DBSalesPath+QDir::separator()+SALES,SalesTableRecord);
        AttachDbfTable(NonSentTable,DBSalesPath+QDir::separator()+NONSENT,SalesTableRecord);
        AttachDbfTable(CurrentCheckTable,DBTmpPath+QDir::separator()+CURCHECK,SalesTableRecord);
        AttachDbfTable(StaffTable,DBGoodsPath+QDir::separator()+STAFF,StaffTableRecord);
        GoodsTable->SetUseDeleted(true);

        AttachDbfTable(GoodsTable,DBGoodsPath+QDir::separator()+GOODS,GoodsTableRecord, true);

        AttachDbfTable(GoodsPriceTable,DBGoodsPath+QDir::separator()+GOODSPRICE,GoodsPriceTableRecord, true);
        AttachDbfTable(BarInfoTable,DBGoodsPath+QDir::separator()+BARCONF,BarInfoTableRecord);
        AttachDbfTable(KeyboardTable,DBGoodsPath+QDir::separator()+KEYBOARD,KeyboardTableRecord,true);
        AttachDbfTable(GroupsTable,DBGoodsPath+QDir::separator()+GROUPS,GroupsTableRecord);
        AttachDbfTable(ActionsTable,DBTmpPath+QDir::separator()+ACTIONS,ActionsTableRecord);
        AttachDbfTable(ActreportTable,DBTmpPath+QDir::separator()+ACTREPORT,ActionsTableRecord);

// Bank check
        AttachDbfTable(BankActionsTable,DBTmpPath+QDir::separator()+BANKACTIONS,BankActionsTableRecord);
        AttachDbfTable(BankActreportTable,DBTmpPath+QDir::separator()+BANKACTREPORT,BankActionsTableRecord);

        CustomersTable->SetUseDeleted(true);
        AttachDbfTable(CustomersTable,DBGoodsPath+QDir::separator()+CUSTOMERS,CustomersTableRecord, true);
        AttachDbfTable(CustRangeTable,DBGoodsPath+QDir::separator()+CUSTRANGE,CustRangeTableRecord);
        AttachDbfTable(CustomersSalesTable,DBSalesPath+QDir::separator()+CUSTSALES,CustomersSalesRecord);
        AttachDbfTable(CustomersValueTable,DBGoodsPath+QDir::separator()+CUSTVAL,CustomersValueRecord,true);
        AttachDbfTable(DiscountsTable,DBGoodsPath+QDir::separator()+DISCDESC,DiscountsRecord,true);
        AttachDbfTable(BankDiscountTable,DBGoodsPath+QDir::separator()+BANKDISC,BankDiscountRecord,true);
        AttachDbfTable(MsgLevelTable,DBGoodsPath+"/msglevel.dbf",MsgLevelRecord);
#ifdef RET_CHECK
        AttachDbfTable(ReturnTable,DBSalesPath+QDir::separator()+RETURN,ReturnRecord);
#endif
        AttachDbfTable(CrPaymentTable,DBTmpPath+QDir::separator()+CRPAYMENT,PaymentTableRecord);
        AttachDbfTable( PaymentTable,DBSalesPath+QDir::separator()+PAYMENT,PaymentTableRecord);
        AttachDbfTable( PaymentToSendTable,DBSalesPath+QDir::separator()+PAYMENTTOSEND,PaymentTableRecord);

        AttachDbfTable(BankBonusTable,DBSalesPath+QDir::separator()+BANKBONUS,BankBonusTableRecord);
        AttachDbfTable(BankBonusToSendTable,DBSalesPath+QDir::separator()+BANKBONUSTOSEND,BankBonusTableRecord);

        AttachDbfTable(BonusTable,DBSalesPath+QDir::separator()+BONUSFILE,BonusTableRecord);
        AttachDbfTable(BonusToSendTable,DBSalesPath+QDir::separator()+BONUSFILETOSEND,BonusTableRecord);

        AttachDbfTable(ActsizTable,DBSalesPath+QDir::separator()+ACTSIZFILE,ActsizTableRecord);
        AttachDbfTable(ActsizToSendTable,DBSalesPath+QDir::separator()+ACTSIZFILETOSEND,ActsizTableRecord);

    }
    catch(cash_error& ex)
    {
        Log("... Error open table on start!!!");
        LoggingError(ex);
    }

    string capt = "";

    setCaption( W2U(capt) );

    ResolutionType=ConfigTable->GetLongField("RESOLUTION");


    //============================
    // При загрузке кассового ПО выполняется cоздание и инициализация объекта WBNS
    // Объект должен жить до завершения работы кассового ПО
    // Создания и инициализации объекта на каждом запросе не должно быть
    Log(" INIT BNS... ");
    try {

        BNS = new TBonus();

        BNS->Init();

        // Переопределяем номер терминала по номеру кассы
        // Шаблон Магазин(3 знака)_Касса(2 знака), например 044_01
        string smag = "000"+Int2Str(Str2Int(GetMagNum()));
        string skas = "00"+Int2Str(GetCashRegisterNum());
        string term = smag.substr(smag.length()-3,3)+"_"+skas.substr(skas.length()-2,2);
        BNS->Terminal = term;
        Log(" Terminal = "+BNS->Terminal);

        Log("  ..SUCCESS ");
    } catch(exception& ex) {
        // обработка ошибки создания объекта BNS
        string err= ex.what();
        err = "Ошибка инициализации BNS: "+err;

        ShowMessage(err);

        Log("  ..ERROR: ");
        Log((char*)ex.what());
    }


    switch (ResolutionType)
    {
        case res640x400Color:case res640x400BW:BaseSkip=0;break;
        case res640x480Color:case res640x480BW:BaseSkip=8;break;
        default: BaseSkip=8;break;
    }

    QFont ButtonFont(font());//GUI elements initialization
    ButtonFont.setPointSize(12);

    QFont StatusFont(font());//GUI elements initialization
    StatusFont.setPointSize(8);

    CashButton = new QPushButton( this, "CashButton" );
    CashButton->setFixedSize(200,40);
    CashButton->setText( W2U( "Касса" ) );
    CashButton->setFlat( false );
    CashButton->setFont(ButtonFont);

    FRButton = new QPushButton( this, "FRButton" );
    FRButton->setFixedSize(140,40);
    FRButton->setText( W2U( "Фискальный регистратор" ) );
    FRButton->setFlat( false );
    FRButton->setFont(ButtonFont);

    RefButton = new QPushButton( this, "RefButton" );
    RefButton->setFixedSize(140,40);
    RefButton->setText( W2U( "Справочники" ) );
    RefButton->setFlat( false );
    RefButton->setFont(ButtonFont);

    AdminButton = new QPushButton( this, "AdminButton" );
    AdminButton->setFixedSize(140,40);
    AdminButton->setText( W2U("Настройка" ) );
    AdminButton->setFlat( false );
    AdminButton->setFont(ButtonFont);

    TaxOfficerButton = new QPushButton( this, "TaxOfficerButton" );
    TaxOfficerButton->setFixedSize(110,40);
    TaxOfficerButton->setText( W2U( "Налоговый инспектор" ) );
    TaxOfficerButton->setFlat( false );
    TaxOfficerButton->setFont(ButtonFont);

    EGAISCheckButton = new QPushButton( this, "EGAISCheckButton" );
    EGAISCheckButton->setFixedSize(110,40);
    EGAISCheckButton->setText( W2U( "ЕГАИC" ) );
    EGAISCheckButton->setFlat( false );
    EGAISCheckButton->setFont(ButtonFont);

#ifdef BUTTON_OLD_GOODS
    RestoreOldGoodsButton = new QPushButton(this, "RestoreOldGoods");
    RestoreOldGoodsButton->setFixedSize(50,40);
    RestoreOldGoodsButton->setText( W2U( "Восстановление товаров" ) );
    RestoreOldGoodsButton->setFlat( false );
    RestoreOldGoodsButton->setFont(ButtonFont);
#endif

    ExitButton = new QPushButton( this, "ExitButton" );
    ExitButton->setFixedSize(45,40);
    ExitButton->setText( W2U( "Выход" ) );
    ExitButton->setFlat( false );
    ExitButton->setFont(ButtonFont);

    Frame = new QFrame( this, "Frame" );
    Frame->setFrameShape( QFrame::WinPanel );
    Frame->setFrameShadow( QFrame::Sunken );


//*
// + dms ===== New Year 2011 =====\

    LabelImage = new QLabel(this, "LabelImage");
    LabelImage->move(2, 245);

    LabelImage->setFixedSize(800, 320);
    LabelImage->setProperty( "alignment", int( QLabel::AlignCenter ) );
    //LabelImage->setBackgroundColor(Qt::black);
    LabelImage->setMargin(5);


//    QPixmap pic;
//
//    QImage img("./NewYear.gif");
//    if (! pic.convertFromImage( img ))
//    {
//        QImage img_logo("./logo.jpg");
//        pic.convertFromImage( img_logo );
//    }

    QMovie *movie = new QMovie("./logo1.gif");
    LabelImage->setMovie(*movie);

    //LabelImage->setPixmap(pic);

    //LabelImage->setScaledContents(usescale);


// - dms =====/
//*/

    int BottomLineHeight=12;

    InfoBar = new QProgressBar(this);
    InfoBar->setFixedSize(135,BottomLineHeight);
    InfoBar->setFont(StatusFont);

    InfoMessage = new QLabel(W2U(""),this);
    InfoMessage->setFixedSize(265,BottomLineHeight);
    InfoMessage->setFrameShape( QLabel::Panel );
    InfoMessage->setFrameShadow( QLabel::Sunken );
    InfoMessage->setFont(StatusFont);

    ServerInfo = new QLabel(W2U("БАЗА"),this);
    ServerInfo->setFixedSize(50,BottomLineHeight);
    ServerInfo->setFrameShape( QLabel::Panel );
    ServerInfo->setFrameShadow( QLabel::Sunken );
    ServerInfo->setFont(StatusFont);

    RegisterInfo = new QLabel(W2U("ФР"),this);
    RegisterInfo->setFixedSize(30,BottomLineHeight);
    RegisterInfo->setFrameShape( QLabel::Panel );
    RegisterInfo->setFrameShadow( QLabel::Sunken );
    RegisterInfo->setFont(StatusFont);

    KeepCheckInfo = new QLabel(W2U(""),this);
    KeepCheckInfo->setFixedSize(30,BottomLineHeight);
    KeepCheckInfo->setFrameShape( QLabel::Panel );
    KeepCheckInfo->setFrameShadow( QLabel::Sunken );
    KeepCheckInfo->setFont(StatusFont);

    BankInfo = new QLabel(W2U("Банк"),this);
    BankInfo->setFixedSize(40,BottomLineHeight);
    BankInfo->setFrameShape( QLabel::Panel );
    BankInfo->setFrameShadow( QLabel::Sunken );
    BankInfo->setFont(StatusFont);

    Bar1 = new QLabel(W2U(""),this);
    Bar1->setFixedSize(70,BottomLineHeight);
    Bar1->setFrameShape( QLabel::Panel );
    Bar1->setFrameShadow( QLabel::Sunken );
    Bar1->setFont(StatusFont);

    Bar2 = new QLabel(W2U(""),this);
    Bar2->setFixedSize(30,BottomLineHeight);
    Bar2->setFrameShape( QLabel::Panel );
    Bar2->setFrameShadow( QLabel::Sunken );
    Bar2->setFont(StatusFont);

    Bar3 = new QLabel(W2U(""),this);
    Bar3->setFixedSize(30,BottomLineHeight);
    Bar3->setFrameShape( QLabel::Panel );
    Bar3->setFrameShadow( QLabel::Sunken );
    Bar3->setFont(StatusFont);


    LastPressed=CashButton;

    QColor TimerColor;
    TimerColor=darkRed;


    drawWholeWidget();

    CurrentWorkPlace=NULL;

    thisApp=qApp;//just the internal link to the application

    Log("GUI was inited");

    move(0,0);
    show();

    thisApp->processEvents();

    //initialize fiscal registrator
    Log("initialize fiscal registrator");
    FiscalRegType=ConfigTable->GetLongField("REGTYPE");

;;Log("FiscalRegType="+Int2Str(FiscalRegType));

    switch (FiscalRegType)
    {
        case EmptyRegisterType:
            FiscalReg=new EmptyFiscalReg;
            break;
        case ElvesMiniType:
        case ShtrichVer2Type:
            FiscalRegType = ShtrichVer2EntireType;
        case ElvesMiniEntireType:
        case ShtrichVer2EntireType:
            FiscalReg=new ElvisRegVer2;
            break;
        case ShtrichCityType:
            setLineWidthDefault(32);
            FiscalReg=new ElvisRegVer3;
            break;
        case ShtrichVer3Type:
            FiscalRegType = ShtrichVer3EntireType;
        case ShtrichVer3EntireType:
            FiscalReg=new ElvisRegVer3;
            break;
        case ShtrichVer3PrinterType:
            FiscalRegType = ShtrichVer3EntirePrinterType;
        case ShtrichVer3EntirePrinterType:
            FiscalReg=new ElvisRegVer3Printer;
            break;
    }

    InfoMessage->setText(W2U("Подключение фискального регистратора..."));
    thisApp->processEvents();

    ShowErrorCode(FiscalReg->InitReg(ConfigTable->GetLongField("REGPORT"),
        ConfigTable->GetLongField("REGBAUD"),FiscalRegType,
        ConfigTable->GetStringField("REGPASSWD").c_str()));

    InfoMessage->setText(W2U("Синхронизация времени ФР/Система"));
    thisApp->processEvents();

    // синхронизация времени
    FiscalReg->SyncDateTime();

    //FiscalReg->DBArchPath = DBArchPath;

    //+ dms  ==== установим номера телефонов в чеке
    string tel = "";
    bool needset=false;

    int kas = GetCashRegisterNum();
    int mag = ConfigTable->GetLongField("DEPARTMENT");
    switch (mag)
    {
        case 44:tel = "          (3412)37-95-36";break;
        case 5:tel =  "          (3412)918-428";break;
        case 77:tel = "          (3412)918-298";break;
        default: needset=false;break;
    }
;;Log((char*)Int2Str(mag).c_str());
    if (needset)
    {

//	    char strval[128];memset(strval,'\0',128);
//	    int len=FiscalReg->LineLength;
//
//        tel = "            ООО Встреча";
//        strcpy(strval,tel.substr(0,FiscalReg->LineLength).c_str());
//        ShowErrorCode(FiscalReg->SetTable(4,14,1,strval,len,true));
//
//        memset(strval,'\0',128);
//        tel = "          ИНН  1841001935";
//        strcpy(strval,tel.substr(0,FiscalReg->LineLength).c_str());
//        ShowErrorCode(FiscalReg->SetTable(4,15,1,strval,len,true));
//
//        memset(strval,'\0',128);
//        tel = "       ул. 40 лет Победы, 110";
//        strcpy(strval,tel.substr(0,FiscalReg->LineLength).c_str());
//        ShowErrorCode(FiscalReg->SetTable(4,16,1,strval,len,true));
//
//        string FieldName = "CHECKNAME";
//        string name = "Продажа "+Int2Str(mag)+Int2Str(kas);
//
//
//			DbfMutex->lock();
//			ConfigTable->PutField(FieldName.c_str(), name.c_str());
//			ConfigTable->PutRecord();
//			DbfMutex->unlock();
//
//            // + dms
//            FiscalReg->CheckName = GetCheckName();
//            // - dms

        char strval[128];memset(strval,'\0',128);
        int len=FiscalReg->LineLength;


        if (!ConfigTable->GetLogicalField("OFD"))
        {
            strcpy(strval,tel.substr(0,FiscalReg->LineLength).c_str());
            ShowErrorCode(FiscalReg->SetTable(4,15,1,strval,len,true));

            memset(strval,'\0',128);
            tel = " ";
            strcpy(strval,tel.substr(0,FiscalReg->LineLength).c_str());
            ShowErrorCode(FiscalReg->SetTable(4,16,1,strval,len,true));
        }

        memset(strval,'\0',128);
        tel = "     ООО Домашний Гастроном";
        strcpy(strval,tel.substr(0,FiscalReg->LineLength).c_str());
        ShowErrorCode(FiscalReg->SetTable(4,14,1,strval,len,true));


    }

    // Установка БЕЗНАЛ
    {
        char strval[128];memset(strval,'\0',128);
        int len=FiscalReg->LineLength;

        tel = "БЕЗНАЛ";
        strcpy(strval,tel.substr(0,FiscalReg->LineLength).c_str());
        ShowErrorCode(FiscalReg->SetTable(5,4,1,strval,len,true));

        // Установка БЕЗНАЛИЧНЫМИ
        char v=1;
        FiscalReg->SetTable(17,1,39,&(v=1),1,false);

    }

    char v_c=1;
    ShowErrorCode(FiscalReg->SetTable(17,1,25,&(v_c=1),1,false));

    //- dms
    char v=1;
    ShowErrorCode(FiscalReg->SetTable(1,1,42,&(v=1),1,false));

//

//string MasActsiz[4];
//MasActsiz[0] = "00000046214409p!n\"Hr\"AB3A2snt";
//MasActsiz[1]  = "04606203098385>*knt!/AC68gPGT";
//MasActsiz[2] =  "04606203098903<5;hZNyAB_o65mJ";
//MasActsiz[3] =  "00000046217141je+ySKSABoAb54O";
//
//
//
//FILE * log = fopen( "/Kacca/tmp/log1", "a");// "r");
//if (log) {
//    for (int i=0;i<4; i++)
//    {
//
//        string Actsiz = MasActsiz[i];
//
//    string GTINStrAsString = Actsiz.substr(0,14);
//    string SerialAsString = Actsiz.substr(14,7);
//    string MRCAsString = Actsiz.substr(21,4);
//
//    const char* GTINStr = (const char*)GTINStrAsString.c_str();
//    const char* Serial = (const char*)SerialAsString.c_str();
//    const char* MRC = (const char*)MRCAsString.c_str();
//
//        unsigned long long GTINCode = atoll(GTINStr);
//
//    fprintf(log,"\n%s\tCIGAR. ACTSIZ %s", GetCurDateTime(), Actsiz.c_str());
//    //fprintf(log,"\n%s\tGTIN = %lld (%0*hhX) Serial = %s", GetCurDateTime(), GTINCode, 12, GTINCode, Serial);
//    fprintf(log,"\n%s\tGTIN = %s (%lld) Serial = %s  MRC = %s", GetCurDateTime(), GTINStr, GTINCode, Serial, MRC);
//    //fprintf(log,"\n%s\tGTIN = %s (%lld)", GetCurDateTime(), GTINStr, GTINCode);
//
//
//    }
//
//fclose(log);
//}

//


    InfoMessage->setText(W2U(""));
    thisApp->processEvents();

    //m_vem.Init("ШТРИХ", "ФР", KKMNo, GetCashRegisterNum);
    m_vem.Init("ШТРИХ", "ФР", GetCashRegisterNum());

    ScaleType=ConfigTable->GetLongField("SCALETYPE");
    DisplayType=ConfigTable->GetLongField("DISPLTYPE");
    CardReaderType=ConfigTable->GetLongField("CARDTYPE");
    pCR = CardReaderType & VFUP ? new(CRSBupos) : new(CardReader);

    FiscalReg->CheckName = ConfigTable->GetStringField("CHECKNAME");
    TryBankConnect = ConfigTable->GetLongField("TESTBANK");

    GetNewGoods();

    FILE *f = fopen((DBGoodsPath+QDir::separator()+"customer.ndx").c_str(), "r");
    if(!f){
        CustomersTable->CreateIndex((DBGoodsPath+QDir::separator()+"customer.ndx").c_str(),"ID");
    }else{
        fclose(f);
    }

    try
    {
        CustomersTable->CloseIndices();
        CustomersTable->OpenIndex(DBGoodsPath+QDir::separator()+"customer.ndx","ID");
        GoodsTable->CloseIndices();//open indices of the goods table

        for (int i=0;i<IndexNo;i++)
        {
            GoodsTable->OpenIndex(DBGoodsPath+QDir::separator()+IndexFile[i],IndexField[i]);
        }
    }
    catch(cash_error& ex)
    {
        LoggingError(ex);
    }

    SQLServer	= new SQLThread(this);
    SQLGetCheckInfo	= new SQLThread(this);
    SQLGetCheckInfo->usetimeout=false;// выкл. тайм-аут

    Socket		= new SocketThread(this);

    StatusBankConnection = true;

    if(TryBankConnect) Log("Test Bank Connection");
    TestBankConnection(0, 1); // 1 сек
    CurrentCheckNumber = 0;

    InfoMessage->setText(W2U("Подключение к серверу БД..."));
    thisApp->processEvents();

    SQLServer->InitInstance();
    //SQLServer->LoadRefers();

    //SQLGetCheckInfo->InitInstance();

    if(!SQLServer->running())
        SQLServer->start();
    Socket->start();



    InfoMessage->setText(W2U(""));
    thisApp->processEvents();

    CurrentSaleman = new Saleman;
    CurrentSaleman->Id=0;

    // signals and slots connections
    Log("signals and slots connections");

    connect( CashButton, SIGNAL( clicked() ), this, SLOT( CashButtonClick() ) );
    connect( FRButton, SIGNAL( clicked() ), this, SLOT( FRButtonClick() ) );
    connect( RefButton, SIGNAL( clicked() ), this, SLOT( RefButtonClick() ) );
    connect( AdminButton, SIGNAL( clicked() ), this, SLOT( AdminButtonClick() ) );
    connect( TaxOfficerButton, SIGNAL( clicked() ), this, SLOT( TaxOfficerButtonClick() ) );
    connect( EGAISCheckButton, SIGNAL( clicked() ), this, SLOT( EGAISCheckButtonClick() ) );

#ifdef BUTTON_OLD_GOODS
    connect( RestoreOldGoodsButton, SIGNAL( clicked() ), this, SLOT( RestoreCheckButtonClick() ) );
#endif

    connect( ExitButton, SIGNAL( clicked() ), this, SLOT( ExitButtonClick() ) );
    connect( ExitButton, SIGNAL( pressed() ), this, SLOT( ExitButtonClick() ) );

    Timer = new QTimer(this);//start up timer
    connect( Timer, SIGNAL(timeout()), this, SLOT(timerDone()) );
    Timer->start(1000);
    timerDone();

    //if one had something wrong we have to annul last check
    ProcessCurCheck();




    //ЕГАИС
    EgaisSettings.IP="";
    EgaisSettings.ModePrintQRCode=0;
    EgaisSettings.INN="";
    EgaisSettings.KPP="";
    EgaisSettings.Adress="";
    EgaisSettings.OrgName="";

    f = fopen("/Kacca/egais/egais.cfg","r");
    if(f){
        string str = "";

        // IP УТМ ЕГАИС
        char sMsg[255];
        int begpos, lastpos;

        fgets(sMsg, 255, f);

        str = sMsg;
        begpos = str.find("IP=");
        lastpos = str.find("\n");

        if (begpos!=string::npos)
            EgaisSettings.IP = str.substr(begpos+3, lastpos-1 - begpos-3);

        // ИНН
        fgets(sMsg, 255, f);

        str = sMsg;
        begpos = str.find("INN=");
        lastpos = str.find("\n");

        if (begpos!=string::npos)
            EgaisSettings.INN = str.substr(begpos+4, lastpos-1 - begpos-4);

        fgets(sMsg, 255, f);
        str = sMsg;
        begpos = str.find("KPP=");
        lastpos = str.find("\n");

        if (begpos!=string::npos)
            EgaisSettings.KPP = str.substr(begpos+4, lastpos-1 - begpos-4);

        fgets(sMsg, 255, f);
        str = sMsg;
        begpos = str.find("Adress=");
        lastpos = str.find("\n");

        if (begpos!=string::npos)
            EgaisSettings.Adress = str.substr(begpos+7, lastpos-1 - begpos-7);

        fgets(sMsg, 255, f);
        str = sMsg;
        begpos = str.find("Name=");
        lastpos = str.find("\n");

        if (begpos!=string::npos)
            EgaisSettings.OrgName = str.substr(begpos+5, lastpos-1 - begpos-5);

        int flgQRCode;
        fscanf(f, "ModePrintQRCode=%d", &flgQRCode);
        EgaisSettings.ModePrintQRCode = flgQRCode;

        FiscalReg->GetSmenaNumber(&KKMNo,&KKMSmena,&KKMTime);
        EgaisSettings.KKMNO = LLong2Str(KKMNo);

        if (EgaisSettings.ModePrintQRCode>-1)
        {
            if(EgaisSettings.IP=="")
            {
                ShowMessage("Ошибка! Не задан IP-адрес модуля ЕГАИС!");
            }
        }
    }
    else
    {
        time_t ltime;
        time( &ltime );
        struct tm *now;
        now = localtime( &ltime );

        // C 1 июля 2016г.
        if (
            (now->tm_year+1900 >= 2016)
            &&
            (now->tm_mon+1 >= 7)
           )
        {
            EgaisSettings.ModePrintQRCode=0;
        }
        else
        {
            EgaisSettings.ModePrintQRCode=-1;
        }
    }
    //ЕГАИС

    string stEgais = (EgaisSettings.ModePrintQRCode>-1)?"ЕГАИС":"ОШИБКА";
    Bar1->setProperty( "text",W2U(stEgais));

    Log("CashRegister(...) end");


}

/*
 *  Destroys the object and frees any allocated resources
 */
CashRegister::~CashRegister()
{
    delete ConfigTable;

    // no need to delete child widgets, Qt does it all for us
    delete Socket;
    delete SQLServer;//i hope it is obvious
    delete SQLGetCheckInfo;//i hope it is obvious

    delete StatusMutex;
    delete LogMutex;
    delete SQLMutex;

    switch (FiscalRegType) {
    case EmptyRegisterType:
        delete (EmptyFiscalReg*)FiscalReg;
        break;
    case ElvesMiniType:
    case ShtrichVer2Type:
    case ElvesMiniEntireType:
    case ShtrichVer2EntireType:
        delete (ElvisRegVer2*)FiscalReg;
        break;
    case ShtrichVer3Type:
    case ShtrichVer3EntireType:
    case ShtrichCityType:
        delete (ElvisRegVer3*)FiscalReg;
        break;
    case ShtrichVer3PrinterType:
    case ShtrichVer3EntirePrinterType:
        delete (ElvisRegVer3Printer*)FiscalReg;
        break;
    }

    delete CurrentSaleman;
    delete Timer;
    delete pCR;

    //WPCX->~LProc();
    delete WPCX;
    delete BNS;

}

void CashRegister::drawWholeWidget(void)
{
    int BaseSkip,xRes,yRes;

    switch (ResolutionType)
    {
        case res640x400Color:
        case res640x400BW:
            xRes = 640;
            yRes = 400;
            BaseSkip=0;
            break;
        case res640x480Color:
        case res640x480BW:
            xRes = 640;
            yRes = 480;
            BaseSkip=8;
            break;
        case res800x600Color:
        case res800x600BW:
            xRes = 800;
            yRes = 600;
            BaseSkip=8;
            break;
        case res1024x768Color:
        case res1024x768BW:
            xRes = 1024;
            yRes = 768;
            BaseSkip=8;
            break;
        default: BaseSkip=8;break;
    }

//	setFixedSize(xRes-frameGeometry().width()+width(), yRes + BaseSkip*10 -
//		frameGeometry().height()+height());
    setFixedSize(xRes-frameGeometry().width() +width()
            ,yRes-frameGeometry().height()+height());

/*
    CashButton->move(0,0);
    AdminButton->move(CashButton->width(),0);
    TaxOfficerButton->move(CashButton->width()+AdminButton->width(),0);
    ExitButton->move(CashButton->width()+AdminButton->width()+TaxOfficerButton->width(),0);
*/
    CashButton	->move(BaseSkip/2,0);

    FRButton	->move(BaseSkip/2,CashButton->height());

    RefButton	->move(BaseSkip/2,CashButton->height()+FRButton->height());

    AdminButton	->move(BaseSkip/2,CashButton->height()+FRButton->height()+RefButton->height());

    TaxOfficerButton->move(BaseSkip/2,CashButton->height()+FRButton->height()+RefButton->height()
                           +AdminButton->height());

    EGAISCheckButton->move(BaseSkip/2,
                           CashButton->height()
                           +FRButton->height()
                           +RefButton->height()
                           +AdminButton->height()
                           +TaxOfficerButton->height()
                           );
#ifdef BUTTON_OLD_GOODS
    RestoreOldGoodsButton->move(BaseSkip/2,
                           CashButton->height()
                           +FRButton->height()
                           +RefButton->height()
                           +AdminButton->height()
                           +TaxOfficerButton->height()
                           +EGAISCheckButton->height()
                           );
#endif

    ExitButton	->move(BaseSkip/2,CashButton->height()+FRButton->height()+RefButton->height()
                       +AdminButton->height()
                       +TaxOfficerButton->height()
                       +EGAISCheckButton->height()
                       +RestoreOldGoodsButton->height()
                       );

    QRect *MainRect = new QRect(geometry().topLeft(),geometry().bottomRight());

    CashButton	->setFixedSize(MainRect->width()-BaseSkip,CashButton->height());
    FRButton	->setFixedSize(MainRect->width()-BaseSkip,FRButton->height());
    RefButton	->setFixedSize(MainRect->width()-BaseSkip,RefButton->height());
    AdminButton	->setFixedSize(MainRect->width()-BaseSkip,AdminButton->height());
    TaxOfficerButton->setFixedSize(MainRect->width()-BaseSkip,TaxOfficerButton->height());
    EGAISCheckButton->setFixedSize(MainRect->width()-BaseSkip,EGAISCheckButton->height());
#ifdef BUTTON_OLD_GOODS
    RestoreOldGoodsButton->setFixedSize(MainRect->width()-BaseSkip,EGAISCheckButton->height());
#endif
    ExitButton	->setFixedSize(MainRect->width()-BaseSkip,ExitButton->height());


    Frame->setGeometry(
                QRect(
                    0,
                    ExitButton->geometry().bottom(),
                    MainRect->width(),
                    MainRect->height()
                    -CashButton->geometry().height()
                    -FRButton->geometry().height()
                    -RefButton->geometry().height()
                    -AdminButton->geometry().height()
                    -TaxOfficerButton->geometry().height()
                    -EGAISCheckButton->geometry().height()
#ifdef BUTTON_OLD_GOODS
                    -RestoreOldGoodsButton->geometry().height()
#endif
                    -ExitButton->geometry().height()
                    -InfoBar->geometry().height()
                    )
            );

    LabelImage->move(
                Frame->geometry().left(),
                Frame->geometry().top()
                );

    LabelImage->setFixedSize(
                Frame->width(),
                Frame->height()
                );

    InfoBar->setGeometry(QRect(0,MainRect->height()-InfoBar->geometry().height(),
        InfoBar->geometry().width(),InfoBar->geometry().height()));
    InfoMessage->setGeometry(QRect(InfoBar->width(),
        MainRect->height()-InfoMessage->geometry().height(),
        InfoMessage->geometry().width(),InfoMessage->geometry().height()));

    KeepCheckInfo->setGeometry(QRect(InfoBar->geometry().width()+InfoMessage->width(),
        MainRect->height()-KeepCheckInfo->geometry().height(),
        KeepCheckInfo->geometry().width(),KeepCheckInfo->geometry().height()));

    Bar1->setGeometry(QRect(InfoBar->geometry().width()
                            +InfoMessage->width()+KeepCheckInfo->width(),
                            MainRect->height()-Bar1->geometry().height(),
                            Bar1->geometry().width(),Bar1->geometry().height()));

    Bar2->setGeometry(QRect(InfoBar->geometry().width()+InfoMessage->width()
                            +KeepCheckInfo->width()+Bar1->width(),
                            MainRect->height()-Bar2->geometry().height(),
                            Bar2->geometry().width(),Bar2->geometry().height()));

    Bar3->setGeometry(QRect(InfoBar->geometry().width()+InfoMessage->width()
                            +KeepCheckInfo->width()+Bar1->width()+Bar2->width(),
                            MainRect->height()-Bar3->geometry().height(),
                            Bar3->geometry().width(),Bar3->geometry().height()));

    BankInfo->setGeometry(QRect(InfoBar->geometry().width()+InfoMessage->width()
                                +KeepCheckInfo->width()+Bar1->width()+Bar2->width()+Bar3->width(),
                                MainRect->height()-BankInfo->geometry().height(),
                                BankInfo->geometry().width(),BankInfo->geometry().height()));

    ServerInfo->setGeometry(QRect(InfoBar->geometry().width()+InfoMessage->width()
                                  +KeepCheckInfo->width()+Bar1->width()+Bar2->width()
                                  +Bar3->width()+BankInfo->width(),
                                  MainRect->height()-ServerInfo->geometry().height(),
                                  ServerInfo->geometry().width(),ServerInfo->geometry().height()));

    RegisterInfo->setGeometry(QRect(InfoBar->geometry().width()+InfoMessage->width()
                                    +KeepCheckInfo->width()
                                    +ServerInfo->geometry().width()+Bar1->width()+Bar2->width()
                                    +Bar3->width()+BankInfo->width(),
                                    MainRect->height()-RegisterInfo->geometry().height(),
                                    RegisterInfo->geometry().width(),
                                    RegisterInfo->geometry().height()));

    delete MainRect;
}

void CashRegister::paintEvent ( QPaintEvent * ev)//repaint all widgets
{
    drawWholeWidget();

    QWidget::paintEvent(ev);

    LastPressed->setFocus();
}

void CashRegister::ShowMessage(string str)//the main aim is to set up the parent of the dialog
{
    ::ShowMessage(thisApp->focusWidget(),str);
}

bool CashRegister::ShowQuestion(string str,bool IsYesDefault)
{
    return ::ShowQuestion(thisApp->focusWidget(),str,IsYesDefault);
}

void CashRegister::FRButtonClick()
{
    LastPressed=FRButton;

    if (PasswordRequest())
    {//if password is valid show menu

        setFRCashmenName("СИСТ. АДМИНИСТРАТОР");

        do
            ShowFRMenu();
        while(!ShowQuestion("Выйти из меню фискального регистратора?",false));
        m_vem.Unreg();
    }
}

void CashRegister::RefButtonClick()
{
    LastPressed=RefButton;

    if (PasswordRequest()){//if password is valid show menu
        do
            ShowRefMenu();
        while(!ShowQuestion("Выйти из меню \"Справочники?\"",false));
        m_vem.Unreg();
    }
}

void CashRegister::AdminButtonClick()
{
    LastPressed=AdminButton;

    if (PasswordRequest()){//if password is valid show menu
        do
            ShowAdminMenu();
        while(!ShowQuestion("Выйти из меню \"Настройки\"?",false));
        m_vem.Unreg();
    }
}

void CashRegister::setFRCashmenName(string fio)
{

    char strval[128];
    memset(strval,'\0',128);
    //int len=FiscalReg->LineLength;
    int len=21; // согласно документации
    strcpy(strval,fio.substr(0,len).c_str());
    ShowErrorCode(FiscalReg->SetTable(2,30,2,strval,len,true));

}

void CashRegister::CashButtonClick()
{

    LastPressed=CashButton;

    if (PasswordRequest())//if password is valid open cashman's workplace
    {

        setFRCashmenName(CurrentSaleman->Name);

        FiscalReg->GetSmenaNumber(&KKMNo,&KKMSmena,&KKMTime);

        QFile *workFlag = new QFile(".work");//касса в рабочем состоянии (флаг)
        workFlag->open(IO_WriteOnly);
        workFlag->close();

        LoggingAction(0,START,0,CurrentSaleman->Id,-1);
//        m_vem.StartKassa();
;;Log("CurrentWorkPlace = new WorkPlace(this);");
        CurrentWorkPlace = new WorkPlace(this);
;;Log("CurrentWorkPlace->exec();");
        CurrentWorkPlace->exec();
;;Log("delete CurrentWorkPlace;");
        delete CurrentWorkPlace;
        CurrentWorkPlace=NULL;

        LoggingAction(0,FINISH,0,CurrentSaleman->Id,-1);
//        m_vem.FinishKassa();

        workFlag->remove();//clear running flag
        delete workFlag;
        setActiveWindow();
        //m_vem.Unreg();
    }

    if(!SQLServer->running())
        SQLServer->start();

    Log("Post CashRegister::CashButtonClick()");
}

void CashRegister::ExitButtonClick()
{
    LastPressed=ExitButton;

    if (ShowQuestion("Выйти из программы?",false))//it's clear
        close();
}

void CashRegister::EGAISCheckButtonClick()
{
#ifndef QRCODE
    Log("CashRegister::EGAISCheckButtonClick() start QrCode");
    LastPressed=EGAISCheckButton;
    QrCode_dlg = new QrCode(this);
    QrCode_dlg->exec();
    delete QrCode_dlg;
    QrCode_dlg = NULL;
    Log("CashRegister::EGAISCheckButtonClick() finish QrCode");
    return;
#else if ARCH_FILE_SCRIPT

    Log("CashRegister::EGAISCheckButtonClick() start");
    LastPressed=EGAISCheckButton;

    //    if (!PasswordRequest()){//if password is valid show menu
    //    }

    ECD = new EGAISCheck_dlg(this);

//    setFixedSize(CashReg->size());
//    ECD->resize(640, 480);

    ECD->exec();
    delete ECD;
    ECD = NULL;

    Log("CashRegister::EGAISCheckButtonClick() finish");
    return;
//#else
/*
string str = QDateTime::currentDateTime().toString("dd.MM.yyyy_hh:mm");

flagRestoreGoods = true;

//    system("tar -czvf arch_test.tar.gz /Kacca/tmp");
    while (flagRestoreGoods)
    {
       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_act_dbf.tar.gz " + DBTmpPath+QDir :: separator() + "actreport.dbf").c_str()) ) flagRestoreGoods=false;
       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_fr_log.tar.gz " + DBTmpPath+QDir :: separator() + "fr.log").c_str()) ) flagRestoreGoods=false;
       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_control_log.tar.gz " + DBTmpPath+QDir :: separator() + "control.log").c_str()) ) flagRestoreGoods=false;
       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_bankact_dbf.tar.gz " + DBTmpPath+QDir :: separator() + "bankactreport.dbf").c_str()) ) flagRestoreGoods=false;
       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_log_log.tar.gz " + DBTmpPath+QDir :: separator() + "log").c_str()) ) flagRestoreGoods=false;

       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_bonus_dbf.tar.gz " + DBSalesPath+QDir::separator()+BONUSFILE + "log").c_str()) ) flagRestoreGoods=false;
       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_bonus_log.tar.gz " + "bonus/bonus.log").c_str()) ) flagRestoreGoods=false;

       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_sales_dbf.tar.gz " + DBSalesPath+QDir::separator() + "sale.dbf").c_str()) ) flagRestoreGoods=false;
       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_payment_dbf.tar.gz " + DBSalesPath+QDir::separator() + "payment.dbf").c_str()) ) flagRestoreGoods=false;

       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_egais_log.tar.gz " + "egais/egais.log").c_str()) ) flagRestoreGoods=false;
       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_pcx_log.tar.gz " + "pcx/.pcx.log").c_str()) ) flagRestoreGoods=false;
       if(! system(("tar -czvf " + DBArchPath+QDir::separator() + str + "_sb_log.tar.gz " + "upos/sb.log").c_str()) ) flagRestoreGoods=false;

    }
*/
#endif
}

#ifdef BUTTON_OLD_GOODS
void CashRegister::RestoreCheckButtonClick()
{
    Log("************************");
    Log("Restore goods");
    Log("************************");


    flagRestoreGoods = true;

    string FileNameGoods  = DBGoodsPath+QDir::separator()+"goods.dbf",
           FileErrorGoods = DBGoodsPath+QDir::separator()+"Error_goods.dbf",
           IndexBarCode = DBGoodsPath+QDir::separator() + "barcode.ndx",
           IndexCode = DBGoodsPath+QDir::separator() + "code.ndx",
           IndexGrCode = DBGoodsPath+QDir::separator() + "grcode.ndx",
           IndexPrice = DBGoodsPath+QDir::separator() + "price.ndx",
           FileOldGoods = DBGoodsPath+QDir::separator() + "old_goods.dbf",
           IndexOldBarCode = DBGoodsPath+QDir::separator() + "old_barcode.ndx",
           IndexOldCode = DBGoodsPath+QDir::separator() + "old_code.ndx",
           IndexOldGrCode = DBGoodsPath+QDir::separator() + "old_grcode.ndx",
           IndexOldPrice = DBGoodsPath+QDir::separator() + "old_price.ndx";


    while (flagRestoreGoods)
    {

        InfoMessage->setText(W2U("Загрузка товаров"));

        if(!system(("cp -rf " + FileNameGoods + " " + FileErrorGoods).c_str()) ) flagRestoreGoods=false;

        if(!system (("rm " + FileNameGoods).c_str()) ) flagRestoreGoods=false;
        if(!system (("rm " + IndexBarCode).c_str()) ) flagRestoreGoods=false;
        if(!system (("rm " + IndexCode).c_str()) ) flagRestoreGoods=false;
        if(!system (("rm " + IndexGrCode).c_str()) ) flagRestoreGoods=false;
        if(!system (("rm " + IndexPrice).c_str()) ) flagRestoreGoods=false;

        if(!system(("cp -rf " + FileOldGoods + " " + FileNameGoods).c_str()) ) flagRestoreGoods=false;
        if(!system(("cp -rf " + IndexOldBarCode + " " + IndexBarCode).c_str()) ) flagRestoreGoods=false;
        if(!system(("cp -rf " + IndexOldCode + " " + IndexCode).c_str()) ) flagRestoreGoods=false;
        if(!system(("cp -rf " + IndexOldGrCode + " " + IndexGrCode).c_str()) ) flagRestoreGoods=false;
        if(!system(("cp -rf " + IndexOldPrice + " " + IndexPrice).c_str()) ) flagRestoreGoods=false;

        Log("Копирование завершено");

    }
     ShowMessage("Данные восстановлены");
}
#endif

void CashRegister::TaxOfficerButtonClick()
{
    LastPressed=TaxOfficerButton;

    if(!PasswordRequest()){//if password is valid show menu
        return;
    }

    if(!CurrentSaleman->ZReport){
        ShowMessage("Доступ запрещен!");
        m_vem.Unreg();
        return;
    }

    ShowMessage("Меню налогового инспектора!");

    SumForm *dlg = new SumForm(this);
    dlg->FormCaption->setText(W2U("Пароль налогового инспектора"));
    dlg->LineEdit->setEchoMode(QLineEdit::Password);
    dlg->CancelButton->setFocus();

    if ((dlg->exec()==QDialog::Accepted)&&(!dlg->LineEdit->text().isEmpty()))
    {
        TaxOfficerPassword=dlg->LineEdit->text().latin1();
        do
        {
            QFont MenuFont(font());
            MenuFont.setPointSize(8);

            QPopupMenu *TaxOfficerMenu = new QPopupMenu(this);
            TaxOfficerMenu->setFont(MenuFont);

            TaxOfficerMenu->insertItem(W2U("Диапазон дат и смен"),this,SLOT(DateShiftRangeClick()));

            TaxOfficerMenu->insertSeparator();

            TaxOfficerMenu->insertItem(W2U("Кр. отчет по сменам"),this,SLOT(ShortShiftReportClick()));
            TaxOfficerMenu->insertItem(W2U("Полн. отчет по сменам"),this,SLOT(LongShiftReportClick()));
            TaxOfficerMenu->insertItem(W2U("Кр. отчет по датам"),this,SLOT(ShortDateReportClick()));
            TaxOfficerMenu->insertItem(W2U("Полн. отчет по датам"),this,SLOT(LongDateReportClick()));

            QRect tmpRect=TaxOfficerButton->geometry();
            TaxOfficerMenu->exec(mapToGlobal(QPoint(tmpRect.left(),tmpRect.bottom())));
            delete TaxOfficerMenu;
        }
        while(!ShowQuestion("Выйти из меню налогового инспектора?",false));
    }

    m_vem.Unreg();

    delete dlg;
    return;
}

//void CashRegister::AttachDbfTable(DbfTable* Table,string DbfName,xbSchema* DbfType,bool RecreateErrorFile=false)
void CashRegister::AttachDbfTable(DbfTable* Table,string DbfName,xbSchema* DbfType,bool RecreateErrorFile)
{

    if (access(DbfName.c_str(),0)!=0)//create or open dbf table
    {
        char s[1024];
        sprintf(s,"Try to create table %s ...",DbfName.c_str());
        Log(s);

        //Table->CreateDatabase(DbfName.c_str(),DbfType);
        Table->CreateDatabase(DbfName,DbfType);

        Log("... SUCCESS create file");
    }
    else
    {

        // + dms ======
        char s[1024];
        sprintf(s,"Try to open table %s ...",DbfName.c_str());
        Log(s);

        try
        {
            Table->OpenDatabase(DbfName,DbfType);
        }
        catch(cash_error& ex)
        {
            LoggingError(ex);

            if (RecreateErrorFile)
            {
               // если "битый" dbf-файл, то удалим его ...
                QFile *BadFile = new QFile(DbfName.c_str());
                BadFile->remove();
                delete BadFile;

                // ... и создадим новый
                Table->CreateDatabase(DbfName,DbfType);
                Log("... Error! Delete and create empty table");
            }
            else
            {
                Log("... Error!");
            }

        }
        // - dms ======

    }

}

bool CashRegister::PathChange(string Capt,string FieldName)
{
    SumForm *dlg = new SumForm(this);//show the simple dialog, read the line of the text
    dlg->FormCaption->setText(W2U(Capt));			//and write it into table of settings
    DbfMutex->lock();
    dlg->LineEdit->setText(ConfigTable->GetStringField(FieldName.c_str()).c_str());
    DbfMutex->unlock();
    dlg->LineEdit->selectAll();

    int res=dlg->exec();
    string path=dlg->LineEdit->text().latin1();
    delete dlg;

    if (res==QDialog::Accepted)
    {
        try
        {
            DbfMutex->lock();
            ConfigTable->PutField(FieldName.c_str(),path.c_str());
            ConfigTable->PutRecord();
            DbfMutex->unlock();
            return true;
        }
        catch(cash_error& ex)
        {
            DbfMutex->unlock();
            LoggingError(ex);
        }
    }

    return false;//if path was changed we return true
}

bool CashRegister::NumberChange(string Capt,string FieldName)
{
    SumForm *dlg = new SumForm(this);//show the simple dialog, read the number
    dlg->FormCaption->setText(W2U(Capt));				//and write it into table of settings
    QIntValidator *valid = new QIntValidator(dlg->LineEdit);
    valid->setRange(0,3200000);
    dlg->LineEdit->setValidator(valid);
    DbfMutex->lock();
    ConfigTable->GetFirstRecord();
    dlg->LineEdit->setText(Int2Str(ConfigTable->GetLongField(FieldName.c_str())).c_str());
    DbfMutex->unlock();
    dlg->LineEdit->selectAll();

    int res=dlg->exec();
    int depart=dlg->LineEdit->text().toInt();
    delete valid;
    delete dlg;

    if (res==QDialog::Accepted)
    {
        try
        {
            DbfMutex->lock();
            ConfigTable->GetFirstRecord();
            ConfigTable->PutLongField(FieldName.c_str(),depart);
            ConfigTable->PutRecord();
            DbfMutex->unlock();
            return true;
        }
        catch(cash_error& er)
        {
            DbfMutex->unlock();

            LoggingError(er);
        }
    }

    return false;
}

void CashRegister::MainPathChange(void)//change and create the main path of the program
{
    if (PathChange("Основной путь","MAIN_PATH"))
    {



        DbfMutex->lock();
        DBMainPath=ConfigTable->GetStringField("MAIN_PATH");
        DbfMutex->unlock();

        InitDirs();
    }
}

void CashRegister::ImportPathChange(void)//change and create the path of import
{
    if (PathChange("Новые товары","GOODS_PATH"))
    {
        DbfMutex->lock();
        DBImportPath=ConfigTable->GetStringField("GOODS_PATH");

        DbfMutex->unlock();

        InitDirs();
    }
}

void CashRegister::ArchPathChange(void)//change and create the path of import
{
    if (PathChange("Путь к архиву","ARCH_PATH"))
    {
        DbfMutex->lock();
        DBArchPath=ConfigTable->GetStringField("ARCH_PATH");

        DbfMutex->unlock();

        InitDirs();
    }
}


bool CashRegister::PasswordRequest(void)//check login/password and set parameters of the saleman
{
    DbfMutex->lock();
    int RecordsCount=StaffTable->RecordCount();
    DbfMutex->unlock();

    if (RecordsCount==0)
    {
        //if staff table is empty then we give all rights
        CurrentSaleman->Name="";
        CurrentSaleman->Code=0;
        CurrentSaleman->Id=0;
        CurrentSaleman->Login="";

        CurrentSaleman->Return=true;
        CurrentSaleman->Storno=true;
        CurrentSaleman->Annul=true;
        CurrentSaleman->CashIncasso=true;
        CurrentSaleman->CashIncome=true;
        CurrentSaleman->CurrentSum=true;
        CurrentSaleman->PrintReport=true;

        CurrentSaleman->ZReport=true;
        CurrentSaleman->XReport=true;
        CurrentSaleman->LoadGoods=true;
        CurrentSaleman->LoadStaff=true;
        CurrentSaleman->LoadKeys=true;
        CurrentSaleman->LoadBarcodes=true;
        CurrentSaleman->LoadGroups=true;
        CurrentSaleman->Weight/*SendSales*/=true;
        CurrentSaleman->FRMenu=true;
        CurrentSaleman->RefMenu=true;
        CurrentSaleman->AdminMenu=true;
        CurrentSaleman->Discounts=true;

        CurrentSaleman->Cashless=true;
        CurrentSaleman->BankBonusReturn=false;

        CurrentSaleman->CurrentGoods=true;
        CurrentSaleman->Quantity=true;

        return true;
    }

    frmPasswd = new PasswdForm(this);

    DbfMutex->lock();
    ComPortScan* Scanner = new ComPortScan(this
                    ,ConfigTable->GetLongField("SCANPORT")
                    ,ConfigTable->GetLongField("SCANBAUD")
                    );//init scanner's driver

    frmPasswd->UserName->setText(ConfigTable->GetStringField("LAST_MAN").c_str());
    DbfMutex->unlock();

    if (frmPasswd->UserName->text().isEmpty())
        frmPasswd->UserName->setFocus();
    else
        frmPasswd->PasswdLine->setFocus();

    int res=frmPasswd->exec();

    QString login=frmPasswd->UserName->text();
    string password=frmPasswd->PasswdLine->text().latin1();


    delete frmPasswd;
    frmPasswd=NULL;

    delete Scanner;
    Scanner=NULL;

    if (LastPressed==TaxOfficerButton)
    {
        if (password=="3y6(6" && ShowQuestion("Сбербанк. Сверка",true))
        {
            //ShowQuestion("Сверка СБ",true);
            //CardClose(true);
#ifndef _WS_WIN_
            pCR->CloseDay();
            pCR->GetTermNum();
#endif
            LastPressed=CashButton;
            return false;
        }
        if (password=="M06u^" && ShowQuestion("Тест считывателя пл.карт",true))
        {
            //ShowQuestion("Тест считывателя пл.карт",true);
            CardReaderTestClick();
            LastPressed=CashButton;
            return false;
        }
    }

    try
    {
        //find the person and set up his properties
        if (res==QDialog::Accepted)
        {
            DbfMutex->lock();

            if (!StaffTable->Locate("MAN_LOGIN",login.latin1()))
            {
                DbfMutex->unlock();
                ShowMessage("Имя не найдено!");
                return false;
            }

        if (password==StaffTable->GetStringField("SECURITY"))
            {
                ConfigTable->PutField("LAST_MAN",login.latin1());
                ConfigTable->PutRecord();

                CurrentSaleman->Id=StaffTable->GetLongField("ID");
                CurrentSaleman->Name=StaffTable->GetStringField("NAME");
                CurrentSaleman->Code=StaffTable->GetLongField("ROLE_TYPE");
                CurrentSaleman->Login=StaffTable->GetStringField("MAN_LOGIN");
                string SalemanRights=StaffTable->GetStringField("RIGHTS");

                DbfMutex->unlock();

                if (SalemanRights[0]=='.')
                        CurrentSaleman->Return=true;
                else
                    CurrentSaleman->Return=false;

                if (SalemanRights[1]=='.')
                    CurrentSaleman->Storno=true;
                else
                    CurrentSaleman->Storno=false;

                if (SalemanRights[2]=='.')
                    CurrentSaleman->Annul=true;
                else
                    CurrentSaleman->Annul=false;

                if (SalemanRights[3]=='.')
                    CurrentSaleman->CashIncasso=true;
                else
                    CurrentSaleman->CashIncasso=false;

                if (SalemanRights[4]=='.')
                    CurrentSaleman->CashIncome=true;
                else
                    CurrentSaleman->CashIncome=false;

                if (SalemanRights[5]=='.')
                    CurrentSaleman->CurrentSum=true;
                else
                    CurrentSaleman->CurrentSum=false;

                if (SalemanRights[6]=='.')
                    CurrentSaleman->PrintReport=true;
                else
                    CurrentSaleman->PrintReport=false;

                if (SalemanRights[7]=='.')
                    CurrentSaleman->ZReport=true;
                else
                    CurrentSaleman->ZReport=false;

                if (SalemanRights[8]=='.')
                    CurrentSaleman->XReport=true;
                else
                    CurrentSaleman->XReport=false;

                if (SalemanRights[9]=='.'){
                    CurrentSaleman->LoadGoods=
                    CurrentSaleman->LoadStaff=
                    CurrentSaleman->LoadKeys=
                    CurrentSaleman->LoadBarcodes=
                    CurrentSaleman->LoadGroups=true;
                }
                else{
                    CurrentSaleman->LoadGoods=
                    CurrentSaleman->LoadStaff=
                    CurrentSaleman->LoadKeys=
                    CurrentSaleman->LoadBarcodes=
                    CurrentSaleman->LoadGroups=false;
                }

                if (SalemanRights[14]=='.')
                    CurrentSaleman->Weight/*SendSales*/=true;
                else
                    CurrentSaleman->Weight/*SendSales*/=false;

                if (SalemanRights[15]=='.')
                {
                    CurrentSaleman->FRMenu=true;
                    CurrentSaleman->RefMenu=true;
                    CurrentSaleman->AdminMenu=true;
                }
                else
                {
                    CurrentSaleman->FRMenu=false;
                    CurrentSaleman->RefMenu=false;
                    CurrentSaleman->AdminMenu=false;
                }

                if (SalemanRights[16]=='.')
                    CurrentSaleman->Discounts=true;
                else
                    CurrentSaleman->Discounts=false;

                if (SalemanRights[17]=='.')
                    CurrentSaleman->Cashless=true; //право безналичного расчета
                else
                    CurrentSaleman->Cashless=false;

                if (SalemanRights[18]=='.')
                    CurrentSaleman->CurrentGoods=true;
                else
                    CurrentSaleman->CurrentGoods=false;

                if (SalemanRights[19]=='.')
                    CurrentSaleman->Quantity=true;
                else
                    CurrentSaleman->Quantity=false;

                if (SalemanRights[20]=='.')
                    CurrentSaleman->FreeSum=true;
                else
                    CurrentSaleman->FreeSum=false;

                if (SalemanRights[21]=='.')
                    CurrentSaleman->BankBonusReturn=true;
                else
                    CurrentSaleman->BankBonusReturn=false;

                m_vem.SetConnect(ConfigTable->GetStringField("VEMSRV").c_str(), ConfigTable->GetLongField("VEMPORT"));
                m_vem.InitMan(CurrentSaleman->Id, CurrentSaleman->Name.c_str(), CurrentSaleman->ZReport ? 2 : 1);
                m_vem.Reg();

                return true;
            }
            else
            {
                DbfMutex->unlock();
                ShowMessage("Пароль неверен!");
                return false;
            }
        }
        else
            return false;
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
        return false;
    }

    return false;
}

void CashRegister::ShowErrorCode(int er)//general fiscal error's handler
{
    QFont tmpFont=RegisterInfo->font();

    if (er!=0){
        tmpFont.setStrikeOut(true);
        cash_error error(register_error,er,"");
        LoggingError(error);
    }else
        tmpFont.setStrikeOut(false);

    RegisterInfo->setFont(tmpFont);
    RegisterInfo->repaint();
    if (CurrentWorkPlace!=NULL)
    {
        CurrentWorkPlace->RegisterInfo->setFont(tmpFont);
        CurrentWorkPlace->RegisterInfo->repaint();
    }
}

string CashRegister::GetCurTime(void)//get current time as a string
{
    time_t cur_time=time(NULL);
    tm *l_time=localtime(&cur_time);
    return QTime(l_time->tm_hour,l_time->tm_min,l_time->tm_sec).toString().latin1();
}

string CashRegister::GetCurDate(void)//get current date as a string
{
    return xbDate().FormatDate(DateTemplate).c_str();
}

void CashRegister::LoggingError(cash_error& er)//error handler
{
    vector<string> records;
    string msg,tmpStr;


    LogMutex->lock();

    switch (er.type())
    {
        case sql_error:
        case ping_error://if we are disconnected do nothing
            msg="SQL: ";msg+=er.what();
            SetSQLStatus(false);
            ShowMessage(ServerError);
            break;
        case register_error://show error message of the fiscal register
            msg="Fiscal register: "+FiscalReg->GetErrorStr(er.code(),1);
            ShowMessage("Фискальный регистратор: "+FiscalReg->GetErrorStr(er.code(),0));
            break;
        case xbase_error:
            if (er.showmsg) ShowMessage(XBaseError);//show DBF error
            msg="DBF: ";msg+=er.what();
            break;
        case bank_error://if bank is not accessable
            msg="Bank: ";msg+=er.what();
            StatusBankConnection = false;
            UpdateBankStatus();
            //ShowMessage(BankError);
            break;
        default:
            msg="Unknown: ";msg+=er.what();
            break;
    }

    QFile *ErrLogs = new QFile((DBTmpPath+QDir::separator()+ErrorLog).c_str());//create/open log file
    if (!ErrLogs->exists())
    {
        ErrLogs->open(IO_WriteOnly);
        ErrLogs->close();
    }

    if (!ErrLogs->open(IO_ReadOnly))
    {
        LogMutex->unlock();
        delete ErrLogs;
        return;
    }

    records.clear();

    int res;
    do
    {
        char buffer[256];memset(buffer,0,256);
        res=ErrLogs->readLine(buffer,256);
        records.insert(records.end(),buffer);
    }
    while(res!=-1);
    ErrLogs->close();

    while (records.size()>=1000)//if the log file contains more than 1000 lines we have to cut it
        records.erase(records.begin());

    records.insert(records.end(),GetCurDate()+' '+GetCurTime()+' '+msg+"\r\n");

    ErrLogs->remove();

    if (!ErrLogs->open(IO_WriteOnly))
    {
        LogMutex->unlock();
        delete ErrLogs;
        return;
    }

    for (unsigned int i=0;i<records.size();i++)//write new log-file
        if (ErrLogs->writeBlock(records[i].c_str(),records[i].size())==-1)
            break;

    ErrLogs->close();
    LogMutex->unlock();
    delete ErrLogs;
}

void CashRegister::TryToCancelCheck(void)
{
    for(;;)

    {
        int er=FiscalReg->Cancel();
        if (er==0)
            break;

        if (FiscalReg->IsPaperOut(er))//here we try to process the "paper out problem"
            ShowMessage("Вставьте бумагу и нажмите OK!");
        else
        {
            string Msg="Фискальный регистратор: ";
            Msg+=FiscalReg->GetErrorStr(er,0);
            Msg+="\nВыключите и включите фискальный регистратор.\n";

            Msg+="\nПопытаться аннулировать чек снова?";
            if (!ShowQuestion(Msg,true))
                break;
        }
    }
}

void CashRegister::LoggingAction(int checknum,int type,double sum,int saleman,int guard, const char * card, char* acode, char * bterm, const char * info, int bank_id)
{
    try//store critical action's information
    {
        DbfMutex->lock();
        ActionsTable->BlankRecord();
        ActionsTable->PutLongField("CHECKNUM",checknum);
        ActionsTable->PutField("ACT_DATE",xbDate().GetDate().c_str());
        ActionsTable->PutField("ACT_TIME",GetCurTime().c_str());

        ActionsTable->PutLongField("ACT_TYPE",type);
        ActionsTable->PutDoubleField("ACT_SUM",sum);
        ActionsTable->PutLongField("SALEMAN_ID",saleman);
        ActionsTable->PutLongField("GUARD_ID",guard);
        if(acode) ActionsTable->PutField("ACODE", acode);
        if(bterm) ActionsTable->PutField("BTERM", bterm);
        if(card) ActionsTable->PutField("CARD", card);
        if(info) ActionsTable->PutField("INFO", info);
        ActionsTable->PutLongField("BANK_ID",bank_id);
        ActionsTable->AppendRecord();

        ActreportTable->BlankRecord();
        ActreportTable->PutLongField("CHECKNUM",checknum);
        ActreportTable->PutField("ACT_DATE",xbDate().GetDate().c_str());
        ActreportTable->PutField("ACT_TIME",GetCurTime().c_str());

        ActreportTable->PutLongField("ACT_TYPE",type);
        ActreportTable->PutDoubleField("ACT_SUM",sum);
        ActreportTable->PutLongField("SALEMAN_ID",saleman);
        ActreportTable->PutLongField("GUARD_ID",guard);
        if(acode) ActreportTable->PutField("ACODE", acode);
        if(bterm) ActreportTable->PutField("BTERM", bterm);
        if(card) ActreportTable->PutField("CARD", card);
        if(info) ActreportTable->PutField("INFO", info);
        ActreportTable->PutLongField("BANK_ID",bank_id);
        ActreportTable->AppendRecord();

        DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
    if(!SQLServer->running())
        SQLServer->start();
}


void CashRegister::AddBankAction(int checknum,int type,int saleman, int bank_id, char * bterm, const char * id_report, int line, const char * info)
{
    try//store critical action's information
    {
        DbfMutex->lock();
        BankActionsTable->BlankRecord();
        BankActionsTable->PutLongField("CHECKNUM",checknum);
        BankActionsTable->PutField("DATESALE",xbDate().GetDate().c_str());
        BankActionsTable->PutField("TIMESALE",GetCurTime().c_str());

        BankActionsTable->PutLongField("ACTION",type);
        BankActionsTable->PutLongField("BANK",bank_id);
        BankActionsTable->PutField("IDREPORT",id_report);
        BankActionsTable->PutLongField("LINE",line);
        if(bterm) BankActionsTable->PutField("TERMINAL", bterm);
        if(info) BankActionsTable->PutField("TEXT", info);
        BankActionsTable->PutLongField("SALEMAN_ID",saleman);
        BankActionsTable->AppendRecord();

        BankActreportTable->BlankRecord();
        BankActreportTable->PutLongField("CHECKNUM",checknum);
        BankActreportTable->PutField("DATESALE",xbDate().GetDate().c_str());
        BankActreportTable->PutField("TIMESALE",GetCurTime().c_str());

        BankActreportTable->PutLongField("ACTION",type);
        BankActreportTable->PutLongField("BANK",bank_id);
        BankActreportTable->PutField("IDREPORT",id_report);
        BankActreportTable->PutLongField("LINE",line);
        if(bterm) BankActreportTable->PutField("TERMINAL", bterm);
        if(info) BankActreportTable->PutField("TEXT", info);
        BankActreportTable->PutLongField("SALEMAN_ID",saleman);
        BankActreportTable->AppendRecord();

        DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
    if(!SQLServer->running())
        SQLServer->start();
}

int CashRegister::AddBankBonusOperationToTable(DbfTable *tbl, PCX_RET* PCX_Return, int guard)
{
    try//store critical action's information
    {
        DbfMutex->lock();
        tbl->BlankRecord();

        tbl->PutLongField("ACTION",PCX_Return->action);

        tbl->PutLongField("CASHDESK",GetCashRegisterNum());
        tbl->PutLongField("CHECKNUM",GetLastCheckNum()+1);

        tbl->PutField("DATE",xbDate().GetDate().c_str());
        tbl->PutField("TIME",GetCurTime().c_str());

        tbl->PutField("CARD", PCX_Return->card.c_str());
        tbl->PutField("PAN4", PCX_Return->pwd_card.c_str());

        tbl->PutField("CARD_STAT", PCX_Return->CS_S.c_str());

        tbl->PutField("SHA1", PCX_Return->sha1.c_str());
        tbl->PutField("GUID", PCX_Return->guid.c_str());
        tbl->PutField("TRAN_ID", PCX_Return->id_tran.c_str());
        tbl->PutField("TRAN_DT", PCX_Return->dt_tran.c_str());

        tbl->PutDoubleField("SUMMA", PCX_Return->result_summa);
        tbl->PutDoubleField("ACTIVE_BNS", PCX_Return->active_bonus);
        tbl->PutDoubleField("CHANGE_BNS", PCX_Return->change_bonus);
        tbl->PutDoubleField("TRAN_SUMMA", PCX_Return->total_summa);

        tbl->PutLongField("CASHMAN",CurrentSaleman->Id);
        tbl->PutLongField("GUARD",guard);

        tbl->PutLLongField("CUSTOMER",PCX_Return->Customer);

        tbl->PutLongField("MODE", PCX_Return->mode);
        tbl->PutLongField("RETCODE", PCX_Return->ReturnCode);

        tbl->PutField("INFO", PCX_Return->info.c_str());

        tbl->PutLongField("RETCHECK",PCX_Return->RetCheque);
        tbl->PutField("RETTRAN_ID", PCX_Return->rettran_id.c_str());

        tbl->PutField("TERMINAL", PCX_Return->terminal.c_str());
        tbl->PutField("LIBVER", PCX_Return->LibVersion.c_str());

        tbl->AppendRecord();

        DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
        return -1;
    }

    return 0;
}

//void CashRegister::AddBankBonusOperation(PCX_RET* PCX_Return, int checknum, int saleman, int guard, char * bterm)
void CashRegister::AddBankBonusOperation(PCX_RET* PCX_Return, int guard)
{
    Log("+AddBankBonusOperation+");
    AddBankBonusOperationToTable(BankBonusTable, PCX_Return, guard);

    // к отправке
    AddBankBonusOperationToTable(BankBonusToSendTable, PCX_Return, guard);

    //if(!SQLServer->running()) SQLServer->start();
    Log("-AddBankBonusOperation-");
}


void CashRegister::LoggingAcsiz(DbfTable *tbl, int type, int trycnt, double amount, double sum,
                            int code, string actsiz, string barcode, string name,
                            int resCode, string result)
{

    tbl->BlankRecord();

    tbl->PutLongField("CASHDESK",GetCashRegisterNum());
    tbl->PutLongField("CHECKNUM",GetLastCheckNum()+1);

    tbl->PutField("DATE",xbDate().GetDate().c_str());
    tbl->PutField("TIME",GetCurTime().c_str());

    tbl->PutLongField("OPERATION",type);
    tbl->PutDoubleField("QUANT",amount);
    tbl->PutDoubleField("PRICE",sum);
    tbl->PutLongField("SALEMAN_ID",CurrentSaleman->Id);

    tbl->PutLongField("TRY", trycnt);

    tbl->PutLongField("CODE", code);
    tbl->PutField("ACTSIZ", actsiz.c_str());
    tbl->PutField("BARCODE", barcode.c_str());
    tbl->PutField("NAME", name.c_str());

    tbl->PutLongField("RESCODE", resCode);
    tbl->PutField("RESULT", result.c_str());

    tbl->AppendRecord();

}

int CashRegister::AddActsizOperationToTable(DbfTable *tbl, FiscalCheque * CurCheque, int type, int trycnt, int rescode, string result)
{
    try
    {

        DbfMutex->lock();

       for(unsigned int i=0;i<CurCheque->GetCount();i++)
        {
            if (((*CurCheque)[i].ItemFlag==SL)||((*CurCheque)[i].ItemFlag==RT))
            {
                int AlcoCnt = (*CurCheque)[i].Actsiz.size();

                if ((*CurCheque)[i].ItemAlco && AlcoCnt)
                {

                    double amount = 1;
                    double price = (*CurCheque)[i].ItemFullPrice;
                    if (type==1 || type==2) {
                        price=-price; //Возврат
                        amount = -amount;

                    }

                    for (int j=0;j<AlcoCnt;j++)
                    {

                        LoggingAcsiz(tbl, type, trycnt, amount, price,
                        (*CurCheque)[i].ItemCode, (*CurCheque)[i].Actsiz[j], (*CurCheque)[i].ItemBarcode,
                        (*CurCheque)[i].ItemName, rescode, result);

                    }

                }

            }
        }

        DbfMutex->unlock();

    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
        return -1;
    }

    return 0;
}

void CashRegister::AddActsizOperation(FiscalCheque * CurCheque, int type, int trycnt, int rescode, string result)
{
    Log("+AddActsizOperation+");

    AddActsizOperationToTable(ActsizTable, CurCheque, type, trycnt,  rescode,  result);

    // к отправке
    AddActsizOperationToTable(ActsizToSendTable, CurCheque, type, trycnt,rescode, result);

    //if(!SQLServer->running()) SQLServer->start();
    Log("-AddActsizOperation-");
}

int CashRegister::GetLastCheckNum(void)//get last check's number
{
    DbfMutex->lock();
    int res = ConfigTable->GetLongField("LAST_CHECK");
    DbfMutex->unlock();
    return res;
}

void CashRegister::SetLastCheckNum(int Num)//set last check's number
{
    try
    {
        DbfMutex->lock();
        ConfigTable->GetFirstRecord();
        ConfigTable->PutLongField("LAST_CHECK",Num);
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
}

int CashRegister::GetCashRegisterNum(void)//get the current cash register's number
{
    DbfMutex->lock();
    int res = ConfigTable->GetLongField("CASH_DESK");
    DbfMutex->unlock();
    return res;
}

void CashRegister::SetCashRegisterNum(int Num)//set the current cash register's number
{
    try
    {
        DbfMutex->lock();
        ConfigTable->GetFirstRecord();
        ConfigTable->PutLongField("CASH_DESK",Num);
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
}

string CashRegister::GetMagNum(void)//get the current store's number
{
    DbfMutex->lock();
    ConfigTable->GetFirstRecord();
    string res = ConfigTable->GetStringField("DEPARTMENT");

    DbfMutex->unlock();
    return res;
}

void CashRegister::SetMagNum(string Num)//set the current store's number
{
    try
    {
        DbfMutex->lock();
        ConfigTable->GetFirstRecord();
        ConfigTable->PutField("DEPARTMENT",Num.c_str());
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
}


string CashRegister::GetCashman(void)//get cashman's name
{
    return CurrentSaleman->Login;
}

int CashRegister::GetCashmanCode(void)//get cashman's id
{
    return CurrentSaleman->Code;
}

void CashRegister::ShowFRMenu(void)//меню работы с фискальным регистратором
{
    QFont MenuFont(font());
    MenuFont.setPointSize(12);

    QPopupMenu *FRMenu = new QPopupMenu(this);
    FRMenu->setFont(MenuFont);

    FRMenu->insertItem(W2U("QR-code"),this,SLOT(CashQRcodeClick()));

    //if current person hasn't some rights then one has to hide the corresponding menu item
    if (CurrentSaleman->CashIncasso)
        FRMenu->insertItem(W2U("Внесение денег в кассу"),this,SLOT(CashIncomeClick()));

    if (CurrentSaleman->CashIncome)
        FRMenu->insertItem(W2U("Изъятие денег из кассы"),this,SLOT(CashOutcomeClick()));

    if (CurrentSaleman->CurrentSum)
        FRMenu->insertItem(W2U("Наличность в кассе"),this,SLOT(MoneyInBoxClick()));

    QPopupMenu *ReportMenu=NULL;

    if (CurrentSaleman->PrintReport)
    {
        ReportMenu = new QPopupMenu(this);
        ReportMenu->setFont(MenuFont);
        if (CurrentSaleman->XReport)
            ReportMenu->insertItem(W2U("Отчет по кассе (X отчет)"),this,SLOT(XReportClick()));
        if (CurrentSaleman->ZReport){
            if(CardReaderType & CCCR)
                ReportMenu->insertItem(W2U("Отчет по ПС CityCard"),this,SLOT(CCReportClick()));
            if(CardReaderType & EGCR)
                ReportMenu->insertItem(W2U("Отчет по ПС ""Газпромбанк"""),this,SLOT(EGReportClick()));

            if(CardReaderType & VTBCR)
                ReportMenu->insertItem(W2U("Отчет по ПС ""ВТБ24"""),this,SLOT(VTBReportClick()));

            ReportMenu->insertItem(W2U("Суточный отчет (Z отчет)"),this,SLOT(ZReportClick()));
            if(Cashless)
            {
                ReportMenu->insertItem(W2U("Закрытие дня по карточкам"),this,SLOT(SetCardClose()));

                ReportMenu->insertItem(W2U("Печать сверки итогов"),this,SLOT(PrintCardCloseClick()));

            }
        }
        ReportMenu->insertSeparator();
        ReportMenu->insertItem(W2U("Установка времени фискального регистратора"),this,SLOT(SetCurrentTimeClick()));
        ReportMenu->insertItem(W2U("Установка даты фискального регистратора"),this,SLOT(SetCurrentDateClick()));
        ReportMenu->insertSeparator();
        if((Cashless)&&(CurrentSaleman->BankBonusReturn))
        {
            ReportMenu->insertItem(W2U("Установить вывод подробного Z-отчета"),this,SLOT(SetFullZReportClick()));
        }

        ReportMenu->insertSeparator();
        ReportMenu->insertItem(W2U("Открыть смену"),this,SLOT(OpenFRSmenaClick()));
        ReportMenu->insertItem(W2U("Закрыть чек"),this,SLOT(CloseAdminCheck()));
        //ReportMenu->insertItem(W2U("Продолжить печать"),this,SLOT(ReprintClick()));
        //ReportMenu->insertItem(W2U("Аннулировать чек"),this,SLOT(CancelCheckClick()));

#ifndef FR_CANCEL
        //Работает если существует файл

                QFile *f1 = new QFile("/Kacca/test.txt");
                if (f1->exists())
                    {
                        //ReportMenu->insertItem(W2U("Открыть чек"),this,SLOT(OpenCheck()));
                        //ReportMenu->insertItem(W2U("Закрыть чек"),this,SLOT(CloseAdminCheck()));
                    }

                //ReportMenu->insertItem(W2U("Закрыть чек"),this,SLOT(CloseCheck()));

                //ReportMenu->insertItem(W2U("Отмена документа"), this, SLOT(CancelDocument()));
               // ReportMenu->insertItem(W2U("Продолжить печать"),this,SLOT(ReprintClick()));
                //ReportMenu->insertItem(W2U("Аннулировать чек"),this,SLOT(CancelCheckClick()));
#endif

        FRMenu->insertItem(W2U("Печать отчета"),ReportMenu);
    }

    QPopupMenu *ReturnMenu=NULL;

    if (CurrentSaleman->Return)
    {
        ReturnMenu = new QPopupMenu(this);
        ReturnMenu->setFont(MenuFont);
        ReturnMenu->insertItem(W2U("Возврат по номеру чека"),this,SLOT(LocalReturnClick()));

//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========
/*
        if((Cashless)&&(CurrentSaleman->BankBonusReturn))
        {
            ReturnMenu->insertItem(W2U("Возврат бонусов \"Спасибо\" от Сбербанка"),this,SLOT(BankBonusReturnClick()));
        }
*/
//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========

        FRMenu->insertItem(W2U("Возврат"),ReturnMenu);
    }

    QRect tmpRect=FRButton->geometry();
    if (FRMenu->count()>0)
        FRMenu->exec(mapToGlobal(QPoint(tmpRect.left(),tmpRect.bottom())));

    CardClose(false);
    //show();

    delete FRMenu;

    if (CurrentSaleman->PrintReport)
        delete ReportMenu;
    if (CurrentSaleman->Return)
        delete ReturnMenu;
}

void CashRegister::ShowRefMenu(void)//меню "Справочники"
{
    QFont MenuFont(font());
    MenuFont.setPointSize(12);

    QPopupMenu *RefMenu = new QPopupMenu(this);
    RefMenu->setFont(MenuFont);


    if (CurrentSaleman->LoadGoods)
        RefMenu->insertItem(W2U("Загрузка товаров"),this,SLOT(LoadGoodsClick()));

    if (CurrentSaleman->LoadStaff)
    {
        RefMenu->insertItem(W2U("Загрузка персонала"),this,SLOT(LoadStaffClick()));
        RefMenu->insertItem(W2U("Дисконтные карты"),this,SLOT(LoadDiscountClick()));
    }

    if (CurrentSaleman->LoadKeys)
        RefMenu->insertItem(W2U("Загрузка клавиатуры"),this,SLOT(LoadKeysClick()));

    if (CurrentSaleman->LoadBarcodes)
        RefMenu->insertItem(W2U("Загрузка штрих-кодов"),this,SLOT(LoadBarcodesClick()));

    if (CurrentSaleman->LoadGroups)
        RefMenu->insertItem(W2U("Загрузка групп товаров"),this,SLOT(LoadGroupsClick()));


    QRect tmpRect=RefButton->geometry();
    if (RefMenu->count()>0)
        RefMenu->exec(mapToGlobal(QPoint(tmpRect.left(),tmpRect.bottom())));

    CardClose(false);
    //show();

    delete RefMenu;
}

void CashRegister::ShowAdminMenu(void)//show the administrator's menu
{
    QFont MenuFont(font());
    MenuFont.setPointSize(12);

    QPopupMenu *SetupMenu,*ResolutionMenu,*PathsMenu,*DBMenu,*ScanMenu,*RegMenu,
        *ScaleMenu,*DisplayMenu,*CardReaderMenu,*ActionMenu;
    SetupMenu=ResolutionMenu=PathsMenu=DBMenu=ScanMenu=RegMenu=
        ScaleMenu=DisplayMenu=CardReaderMenu=ActionMenu=NULL;

    SetupMenu = new QPopupMenu(this);
    SetupMenu->setFont(MenuFont);
    SetupMenu->insertItem(W2U("Номер магазина"),this,SLOT(ShopNumClick()));
    SetupMenu->insertItem(W2U("Номер кассы"),this,SLOT(CashDeskNumClick()));
    SetupMenu->insertItem(W2U("Номер чека"),this,SLOT(LastCheckClick()));
    SetupMenu->insertItem(W2U("Макс. кол-во"),this,SLOT(MaxCountClick()));
    SetupMenu->insertItem(W2U("Макс. сумма"),this,SLOT(MaxSumClick()));
    SetupMenu->insertItem(W2U("Макс.размер архива"),this,SLOT(MaxSpaceClick()));

    ResolutionMenu = new QPopupMenu(this);
    ResolutionMenu->setFont(MenuFont);
    ResolutionMenu->insertItem(W2U("640x400 ч/б"),this,SLOT(Res640x400BWClick()),0,res640x400BW);
    ResolutionMenu->insertItem(W2U("640x400 цв."),this,SLOT(Res640x400ColorClick()),0,res640x400Color);

    ResolutionMenu->insertItem(W2U("640x480 ч/б"),this,SLOT(Res640x480BWClick()),0,res640x480BW);
    ResolutionMenu->insertItem(W2U("640x480 цв."),this,SLOT(Res640x480ColorClick()),0,res640x480Color);

    ResolutionMenu->insertItem(W2U("800x600 ч/б"),this,SLOT(Res800x600BWClick()),0,res800x600BW);
    ResolutionMenu->insertItem(W2U("800x600 цв."),this,SLOT(Res800x600ColorClick()),0,res800x600Color);

    ResolutionMenu->insertItem(W2U("1024x768 ч/б"),this,SLOT(Res1024x768BWClick()),0,res1024x768BW);
    ResolutionMenu->insertItem(W2U("1024x768 цв."),this,SLOT(Res1024x768ColorClick()),0,res1024x768Color);

    ResolutionMenu->setItemChecked(res640x400BW,false);
    ResolutionMenu->setItemChecked(res640x400Color,false);

    ResolutionMenu->setItemChecked(res640x480BW,false);
    ResolutionMenu->setItemChecked(res640x480Color,false);

    ResolutionMenu->setItemChecked(res800x600BW,false);
    ResolutionMenu->setItemChecked(res800x600Color,false);

    ResolutionMenu->setItemChecked(res1024x768BW,false);
    ResolutionMenu->setItemChecked(res1024x768Color,false);

    ResolutionMenu->setItemChecked(ResolutionType,true);

    SetupMenu->insertItem(W2U("Разрешение экрана"),ResolutionMenu);

    PathsMenu = new QPopupMenu(this);
    PathsMenu->setFont(MenuFont);
    PathsMenu->insertItem(W2U("Основной путь"),this,SLOT(MainPathClick()));
    PathsMenu->insertItem(W2U("Путь к новым товарам"),this,SLOT(ImportPathClick()));
    PathsMenu->insertItem(W2U("Путь к архиву"),this,SLOT(ArchPathClick()));
    PathsMenu->insertItem(W2U("Каталог ScreenSaver`а"),this,SLOT(PicPathClick()));

    SetupMenu->insertItem(W2U("Пути"),PathsMenu);

    ScanMenu = new QPopupMenu(this);
    ScanMenu->setFont(MenuFont);
    ScanMenu->insertItem(W2U("Порт"),this,SLOT(ScanPortClick()));
    ScanMenu->insertItem(W2U("Тест"),this,SLOT(TestPortClick()));

    SetupMenu->insertItem(W2U("Сканер"),ScanMenu);

    RegMenu = new QPopupMenu(this);

    RegMenu->setFont(MenuFont);
    RegMenu->insertItem(W2U("Нет регистратора"),this,SLOT(EmptyRegClick()),0,EmptyRegisterType);
    RegMenu->insertItem(W2U("ЭЛВЕС-МИНИ-ФР-Ф полн."),this,SLOT(ElvesMiniEntireClick()),0,ElvesMiniEntireType);
    RegMenu->insertItem(W2U("ШТРИХ-ФР-Ф v.2 полн."),this,SLOT(ShtrichVer2EntireClick()),0,ShtrichVer2EntireType);
    RegMenu->insertItem(W2U("ШТРИХ-ФР-Ф v.3 полн."),this,SLOT(ShtrichVer3EntireClick()),0,ShtrichVer3EntireType);
// Регистратор без регистратора
    RegMenu->insertItem(W2U("ШТРИХ-ФР-Ф v.3 принтер"),this,SLOT(ShtrichVer3EntirePrinterClick()),0,ShtrichVer3EntirePrinterType);
    RegMenu->insertItem(W2U("ШТРИХ-СИТИ"),this,SLOT(ShtrichCityClick()),0,ShtrichCityType);
//
    RegMenu->setItemChecked(EmptyRegisterType,false);
    RegMenu->setItemChecked(ElvesMiniType,false);
    RegMenu->setItemChecked(ShtrichVer2Type,false);
    RegMenu->setItemChecked(ShtrichVer3Type,false);
    RegMenu->setItemChecked(ElvesMiniEntireType,false);
    RegMenu->setItemChecked(ShtrichVer2EntireType,false);
    RegMenu->setItemChecked(ShtrichVer3EntireType,false);
// + dms
    RegMenu->setItemChecked(ShtrichVer3EntirePrinterType,false);
    RegMenu->setItemChecked(ShtrichCityType,false);
// - dms
    RegMenu->setItemChecked(FiscalRegType,true);
    RegMenu->insertSeparator();
    RegMenu->insertItem(W2U("Порт"),this,SLOT(RegPortClick()));
    RegMenu->insertItem(W2U("Пароль фискального регистратора"),this,SLOT(RegPasswdClick()));
    RegMenu->insertSeparator();
    RegMenu->insertItem(W2U("Таблицы"),this,SLOT(RegTablesClick()));
    RegMenu->insertSeparator();
    RegMenu->insertItem(W2U("Номер фискального регистратора"),this,SLOT(RegNumberClick()));
    RegMenu->insertItem(W2U("Тест фискального регистратора"),this,SLOT(RegTestClick()));
    RegMenu->insertSeparator();
    RegMenu->insertItem(W2U("Наименование документа в чеке"),this,SLOT(CheckNameClick()));
    RegMenu->insertItem(W2U("Номер смены"),this,SLOT(LastSmenaClick()));
    RegMenu->insertItem(W2U("Печать 5-м шрифтом (ШТРИХ-М ФР-К)"),this,SLOT(FRK5Click()),0, FRKFontType);
    RegMenu->setItemChecked(FRKFontType,FRKFont5);
    RegMenu->insertSeparator();
    RegMenu->insertItem(W2U("Режим ОФД"),this,SLOT(OFDClick()),0, OFDModeType);
    RegMenu->setItemChecked(OFDModeType,OFDMODE);


    SetupMenu->insertItem(W2U("Фискальный регистратор"),RegMenu);

    ScaleMenu = new QPopupMenu(this);
    ScaleMenu->setFont(MenuFont);
    ScaleMenu->insertItem(W2U("Нет весов"),this,SLOT(EmptyScaleClick()),0,EmptyScaleType);
    ScaleMenu->insertItem(W2U("Масса К"),this,SLOT(MassaKScaleClick()),0,MassaKScaleType);
    ScaleMenu->insertItem(W2U("ВП-15Т.2"),this,SLOT(MassaKPClick()),0,MassaKPType);
    ScaleMenu->setItemChecked(EmptyScaleType,false);
    ScaleMenu->setItemChecked(MassaKScaleType,false);
    ScaleMenu->setItemChecked(MassaKPType,false);
    ScaleMenu->setItemChecked(ScaleType,true);
    ScaleMenu->insertSeparator();
    ScaleMenu->insertItem(W2U("Порт"),this,SLOT(ScalePortClick()));
    ScaleMenu->insertItem(W2U("Время отклика"),this,SLOT(ScaleWaitClick()));

    SetupMenu->insertItem(W2U("Весы"),ScaleMenu);

    DisplayMenu = new QPopupMenu(this);
    DisplayMenu->setFont(MenuFont);
    DisplayMenu->insertItem(W2U("Нет дисплея"),this,SLOT(EmptyDisplayClick()),0,EmptyDisplayType);
    DisplayMenu->insertItem(W2U("Datecs"),this,SLOT(DatecsDisplayClick()),0,DatecsDisplayType);
    DisplayMenu->insertItem(W2U("VSCom"),this,SLOT(VSComDisplayClick()),0,VSComDisplayType);
    DisplayMenu->setItemChecked(EmptyDisplayType,false);
    DisplayMenu->setItemChecked(DatecsDisplayType,false);
    DisplayMenu->setItemChecked(VSComDisplayType,false);
    DisplayMenu->setItemChecked(DisplayType,true);
    DisplayMenu->insertSeparator();
    DisplayMenu->insertItem(W2U("Порт"),this,SLOT(DisplayPortClick()));

    SetupMenu->insertItem(W2U("Дисплей"),DisplayMenu);

    CardReaderMenu = new QPopupMenu(this);
    CardReaderMenu->setFont(MenuFont);
    CardReaderMenu->insertItem(W2U("Безналичный расчет"),this,SLOT(CashlessClick()),0,CashlessType);
    CardReaderMenu->setItemChecked(CashlessType,Cashless);
    CardReaderMenu->insertSeparator();
    CardReaderMenu->insertItem(W2U("Нет считывателя"),this,SLOT(EmptyCardReaderClick()),0,EmptyCardReaderType);
    CardReaderMenu->insertItem(W2U("VeriFone"),this,SLOT(VeriFoneCardReaderClick()),0,VeriFoneType);
    CardReaderMenu->insertItem(W2U("CityCard"),this,SLOT(CityCardReaderClick()),0,CityCardType);
    CardReaderMenu->insertItem(W2U("VeriFone UPOS"),this,SLOT(VeriFoneUPOSCardReaderClick()),0,VeriFoneUPOSType);
    CardReaderMenu->insertItem(W2U("Ingenico (i3070)"),this,SLOT(EGateCardReaderClick()),0,EGateType);
    CardReaderMenu->insertItem(W2U("Ingenico VTB24"),this,SLOT(VTBCardReaderClick()),0,VTB24Type);
    CardReaderMenu->setItemChecked(EmptyCardReaderType,!CardReaderType);
    CardReaderMenu->setItemChecked(VeriFoneType,CardReaderType & VFCR);
    CardReaderMenu->setItemChecked(CityCardType,CardReaderType & CCCR);
    CardReaderMenu->setItemChecked(VeriFoneUPOSType,CardReaderType & VFUP);
    CardReaderMenu->setItemChecked(EGateType,CardReaderType & EGCR);

    //;;Log("()()()()()()");
    //;;Log((char*)Int2Str(CardReaderType).c_str());

    CardReaderMenu->setItemChecked(VTB24Type,CardReaderType & VTBCR);
    CardReaderMenu->insertSeparator();
    CardReaderMenu->insertItem(W2U("Порт"),this,SLOT(CardReaderPortClick()));
    CardReaderMenu->insertSeparator();
    CardReaderMenu->insertItem(W2U(" Тест считывателя"),this,SLOT(CardReaderTestClick()));
// + dms
    CardReaderMenu->insertSeparator();
    CardReaderMenu->insertItem(W2U("Проверять связь"),this,SLOT(TestBankConnectionClick()),0,SetCheckBankConnect);
    //CardReaderMenu->setItemChecked(SetCheckBankConnect,CheckBankConnect);
    CardReaderMenu->setItemChecked(SetCheckBankConnect,TryBankConnect);
// - dms

    CardReaderMenu->insertItem(W2U("PayPass (SberBank)"),this,SLOT(SetSbPayPassClick()),0,SetSbPayPassType);
    CardReaderMenu->setItemChecked(SetSbPayPassType,SBPayPass);

    SetupMenu->insertItem(W2U("Пл. карты"),CardReaderMenu);

    DBMenu = new QPopupMenu(this);
    DBMenu->setFont(MenuFont);
    DBMenu->insertItem(W2U("Сервер"),this,SLOT(DatabasePathClick()));
    DBMenu->insertItem(W2U("Имя и пароль"),this,SLOT(LoginCheckClick()));
    DBMenu->insertItem(W2U("Период"),this,SLOT(WaitPeriodClick()));
    DBMenu->insertItem(W2U("Роль"),this,SLOT(SetRoleClick()));

    SetupMenu->insertItem(W2U("База данных"),DBMenu);

    ActionMenu = new QPopupMenu(this);
    ActionMenu->setFont(MenuFont);
//	ActionMenu->insertItem(W2U("Миллениум"),this,SLOT(MilleniumClick()),0,MilleniumType);
//	ActionMenu->setItemChecked(MilleniumType,CheckMillenium);
//	ActionMenu->insertSeparator();
    ActionMenu->insertItem(W2U("Порог сообщения"),this,SLOT(MsgLevelClick()),0,MsgLevelType);
    ActionMenu->setItemChecked(MsgLevelType,CheckMsgLevel);

    SetupMenu->insertItem(W2U("Акции"),ActionMenu);
//! +dms
    QPopupMenu * tMenu = new QPopupMenu(this);
    tMenu->setFont(MenuFont);
    tMenu->insertItem(W2U("Сервер"),this,SLOT(vemSrvClick()));
    tMenu->insertItem(W2U("Порт"),this,SLOT(vemPortClick()));

    SetupMenu->insertItem(W2U("Видеонаблюдение"),tMenu);
//! -dms

    SetupMenu->insertItem(W2U("Тест"),this,SLOT(MenuTestClick()));

    SetupMenu->setActiveItem(3);

    //SetupMenu->activateItemAt(ActiveItem);
    //SetupMenu->activateItemAt(7);

    QRect tmpRect=AdminButton->geometry();
    if (SetupMenu->count()>0)
        SetupMenu->exec(mapToGlobal(QPoint(tmpRect.left(),tmpRect.bottom())));

    CardClose(false);
    //show();

    delete SetupMenu;

    delete DisplayMenu;
    delete ScaleMenu;
    delete RegMenu;
    delete ScanMenu;
    delete DBMenu;
    delete PathsMenu;
    delete ResolutionMenu;
    delete CardReaderMenu;

}

void CashRegister::CashQRcodeClick(void)
{
    int er;

    er=FiscalReg->PrintString("Вариант печати QR-кода №3");
    ///er=FiscalReg->PrintQRCode("Тест/Test", 3);
    //er=FiscalReg->PrintString(" ");
    //er=FiscalReg->PrintQRCode("http://check.egais.ru?id=88a7a3ed-39ae-45de-a3cc-644639f36f4e&dt=0910141104&cn=Magazin2014", 3);
    //er=FiscalReg->PrintQRCode("http://check.egais.ru?id=11111111111111111111111111111111111111111111111111111111111111&amp;Mag7", 3);

    //er=FiscalReg->PrintString(" ");
    er=FiscalReg->PrintQRCode("http://check.egais.ru?id=a7a9c6dd-13ad-4b09-ad01-c37a90323fca&amp;dt=1702171127&amp;cn=02000038931", 3);

//    er=FiscalReg->PrintString("");
//    er=FiscalReg->PrintQRCode("http://ru.wikipedia.org/wiki/QR-%EA%EE%E4", 3);
//    er=FiscalReg->PrintString("");
//    er=FiscalReg->PrintQRCode("http://wikipedia.ru", 3);
//    er=FiscalReg->PrintString(" ");
//    er=FiscalReg->PrintQRCode("http://check.egais.ru?id=111111111111111111111111111111111111111&cn=Magazin2017#oprst$", 3);

    er=FiscalReg->PrintString("Вариант печати QR-кода №2");
    er=FiscalReg->PrintQRCode("http://check.egais.ru?id=88a7a3ed-39ae-45de-a3cc-644639f36f4e&dt=0910141104&cn=Magazin2014", 2);

    er=FiscalReg->PrintString("Вариант печати QR-кода №1");
    er=FiscalReg->PrintQRCode("http://check.egais.ru?id=88a7a3ed-39ae-45de-a3cc-644639f36f4e&dt=0910141104&cn=Magazin2014", 1);

    //er=FiscalReg->PrintQRCode("http://ru.wikipedia.org/wiki/QR-%EA%EE%E4", 1);

    ShowErrorCode(er);
}

void CashRegister::CashIncomeClick(void)//process cash income
{
    double Value;
    SumForm *dlg = new SumForm(this);
    dlg->FormCaption->setText(W2U("Внесение денег"));

    QDoubleValidator *valid = new QDoubleValidator(dlg->LineEdit);
    valid->setRange(0.01,1e12,2);
    dlg->LineEdit->setValidator(valid);

    if (dlg->exec()==QDialog::Accepted)
    {
        Value=dlg->LineEdit->text().toDouble();
        m_vem.InCash(0,1,Value);
        int er=FiscalReg->CashIncome(Value);
        ShowErrorCode(er);
        if (er==0)
            LoggingAction(0,CASHIN,Value,CurrentSaleman->Id,-1);
    }

    delete valid;
    delete dlg;
}

void CashRegister::CashOutcomeClick(void)//process cash outcome
{
    double Value;
    SumForm *dlg = new SumForm(this);
    dlg->FormCaption->setText(W2U("Изъятие денег"));
    QDoubleValidator *valid = new QDoubleValidator(dlg->LineEdit);
    valid->setRange(0.01,1e12,2);
    dlg->LineEdit->setValidator(valid);

    if (dlg->exec()==QDialog::Accepted)
    {
        Value=dlg->LineEdit->text().toDouble();
        m_vem.OutCash(0,1,Value);
        int er=FiscalReg->CashOutcome(Value);
        ShowErrorCode(er);
        if (er==0)
            LoggingAction(0,CASHOUT,Value,CurrentSaleman->Id,-1);
    }

    delete valid;
    delete dlg;
}

void CashRegister::MoneyInBoxClick(void)//check moneybox of the fiscal register
{
    int er;
    double Sum;
    er=FiscalReg->GetCurrentSumm(Sum);
    ShowErrorCode(er);

    if (er==0)

    {
        SumForm *dlg = new SumForm(this);
        dlg->FormCaption->setText(W2U("Денежный ящик"));
        dlg->LineEdit->setText(GetRounded(Sum,0.01).c_str());
        dlg->LineEdit->setReadOnly(true);

        dlg->exec();
        LoggingAction(0,CASHBOX,Sum,CurrentSaleman->Id,-1);
        delete dlg;
    }
}

void CashRegister::XReportClick(void)//print X report
{
    Log("PressButton>> X-Report");
    m_vem.Check();
    // + dms
    if ((FiscalRegType==ShtrichVer3EntirePrinterType) || (FiscalRegType==ShtrichVer3PrinterType))
        SQLServer->GetZReportFromDB(&(FiscalReg->report));
    // - dms
    int er=FiscalReg->XReport();
    ShowErrorCode(er);
    if (er==0)
        LoggingAction(0,XREPORT,0,CurrentSaleman->Id,-1);
}


void CashRegister::ZReportClick(void)//print Z report
{
    Log("PressButton>> Z-Report");

    // принудительная установка подробного отчета по секциям
    ShowErrorCode(FiscalReg->SetFullZReport());

    double ZSumm;
    FiscalReg->GetSmenaNumber(&KKMNo,&KKMSmena,&KKMTime);

    bool RegAsPrinter = (FiscalRegType==ShtrichVer3EntirePrinterType) || (FiscalRegType==ShtrichVer3PrinterType);

    // + dms
    int smena = 0;
    if (RegAsPrinter)
    {


        int lKeyPressed = QMessageBox::information( NULL, "APM KACCA", W2U("Выберите действие:\n - Закрыть текущую смену \n - Распечатать отчет за прошлую смену")
            ,W2U("Закрыть текущую смену"), W2U("Отчет за произвольную смену"), NULL , 0, -1);


        if (lKeyPressed==1)
        {
            bool ok;
            smena = QInputDialog::getInteger("APM-KACCA", W2U("Введите номер смены:"), KKMSmena, 0, 100000, 1, &ok, this );

            if (!ok) return;
        }
        else
            if (lKeyPressed==-1) return;


        FiscalReg->report.KKMSmena = smena>0?smena:KKMSmena;

        int err = SQLGetCheckInfo->GetZReportFromDB(&(FiscalReg->report), smena);
        if (err)
        {
            ShowMessage("Ошибка выполнения операции. Нет доступа к серверу.");
            return;
        }
    }
    // - dms

    FiscalReg->SetDBArchPath(DBArchPath);

    int er=FiscalReg->ZReport(ZSumm); // ZSumm?

    ShowErrorCode(er);

    if (smena) return; // если это информационный отчет за указанную смену, то больше ничего делать не нужно

#ifndef _WS_WIN_
    {
        pCR->CC_CloseDay(GetCashRegisterNum(), true);
        time_t ltime;
        time( &ltime );
        struct tm *now;
        now = localtime( &ltime );
        char str[256];
        sprintf( str, "cc%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
        MoveFileSys("tmp/cc.log", DBArchPath+"/"+str);
        //rename("tmp/cc.log", (DBArchPath+"/"+str).c_str());

        sprintf( str, "log%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
        MoveFileSys("tmp/log", DBArchPath+"/"+str);

        //rename("tmp/log",(DBArchPath+"/"+str).c_str());

        sprintf( str, "sb%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
        MoveFileSys("upos/sb.log", DBArchPath+"/"+str);
        //rename("upos/sb.log",(DBArchPath+"/"+str).c_str());

        sprintf( str, "pcx%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
        MoveFileSys("pcx/.pcx.log", DBArchPath+"/"+str);
        //rename("pcx/.pcx.log",(DBArchPath+"/"+str).c_str());

        // vtb
        sprintf( str, "vtb%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
        MoveFileSys("vtb/vtb.log", DBArchPath+"/"+str);

        // egais
        sprintf( str, "egais%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
        MoveFileSys("egais/egais.log", DBArchPath+"/"+str);

        // bonus
//        if (now->tm_mday==1)
        {
           sprintf( str, "bonus%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
           MoveFileSys("bonus/bonus.log", DBArchPath+"/"+str);

           //string bonuspath = "bonus";
           //MoveFileSys("bonus/bonus.log", bonuspath+"/"+str);
        }

        // удалим вспомогательные файлы ЕГАИС
        system("rm -f /Kacca/egais/*.xml");

    }
#endif

    m_vem.Check();

    if (er==0)
    {

        LoggingAction(KKMSmena,ZREPORT,ZSumm,CurrentSaleman->Id,KKMNo);

        // + dms
        //if (RegAsPrinter)
            SetKKMSmena(KKMSmena+1);
        // - dms


        try
        {
            DbfMutex->lock();
            SalesTable->CloseDatabase();
            DbfMutex->unlock();

            string ArchFileName=GetCurDate()+' '+GetCurTime();//close shift, backup and
                                                                         //clear current sales
            for (unsigned int i=0;i<ArchFileName.size();i++)
                if ((ArchFileName[i]==' ')||(ArchFileName[i]==':')||(ArchFileName[i]=='.'))
                    ArchFileName[i]='_';

            ArchFileName=DBArchPath+QDir::separator()+ArchFileName+".dbf";

            ;;Log((char*)ArchFileName.c_str());

            QFile ArchFile(ArchFileName.c_str()),SalesFile((DBSalesPath+QDir::separator()+SALES).c_str());

            if ((!ArchFile.open(IO_WriteOnly))||(!SalesFile.open(IO_ReadOnly)))
            {
                    ShowMessage("Ошибка при архивировании файла продаж!");
                    return;
            }


            for(;;)
            {
                char buffer[512];
                memset(buffer,0,512);
                unsigned int len=SalesFile.readBlock(buffer,512);
                if (len==0)
                    break;
                else
                    ArchFile.writeBlock(buffer,len);
            }

            ArchFile.close();
            SalesFile.close();

            DbfMutex->lock();
            SalesTable->OpenDatabase((DBSalesPath+QDir::separator()+SALES).c_str());
            SalesTable->Zap();

            int maxSpace=ConfigTable->GetLongField("MAXSPACE");
            int ArchSize=maxSpace*0x100000;//max size of archives
            if (ArchSize==0){
                ConfigTable->PutLongField("MAXSPACE",500);
                maxSpace = 500;
            }
            DbfMutex->unlock();



            //create payment arh
            DbfMutex->lock();
            PaymentTable->CloseDatabase();
            DbfMutex->unlock();

            ArchFileName="payment"+GetCurDate()+' '+GetCurTime();
            for (unsigned int i=0;i<ArchFileName.size();i++)
                if ((ArchFileName[i]==' ')||(ArchFileName[i]==':')||(ArchFileName[i]=='.'))
                    ArchFileName[i]='_';
            ArchFileName=DBArchPath+QDir::separator()+ArchFileName+".dbf";

            MoveFileSys(DBSalesPath+QDir::separator()+PAYMENT, ArchFileName);
            //rename((DBSalesPath+QDir::separator()+PAYMENT).c_str(),ArchFileName.c_str());

            DbfMutex->lock();
            AttachDbfTable(  PaymentTable,DBSalesPath+QDir::separator()+PAYMENT,PaymentTableRecord);
            DbfMutex->unlock();


            //bankbonus archive
            // раз в год 1го февраля
            time_t cur_time=time(NULL);
            tm *l_time=localtime(&cur_time);

            if ((l_time->tm_mday==1) && (l_time->tm_mon==1))
            {
                //bankbonus archive
                DbfMutex->lock();
                BankBonusTable->CloseDatabase();
                DbfMutex->unlock();

                ArchFileName="bankbonus"+GetCurDate()+' '+GetCurTime();
                for (unsigned int i=0;i<ArchFileName.size();i++)
                    if ((ArchFileName[i]==' ')||(ArchFileName[i]==':')||(ArchFileName[i]=='.'))
                        ArchFileName[i]='_';
                ArchFileName=DBArchPath+QDir::separator()+ArchFileName+".dbf";

                MoveFileSys(DBSalesPath+QDir::separator()+BANKBONUS, ArchFileName);
                //rename((DBSalesPath+QDir::separator()+BANKBONUS).c_str(),ArchFileName.c_str());

                DbfMutex->lock();
                AttachDbfTable(BankBonusTable,DBSalesPath+QDir::separator()+BANKBONUS,BankBonusTableRecord);
                DbfMutex->unlock();
            }

            //bonus archive
            DbfMutex->lock();
            BonusTable->CloseDatabase();
            DbfMutex->unlock();

            ArchFileName="bonus"+GetCurDate()+' '+GetCurTime();
            for (unsigned int i=0;i<ArchFileName.size();i++)
                if ((ArchFileName[i]==' ')||(ArchFileName[i]==':')||(ArchFileName[i]=='.'))
                    ArchFileName[i]='_';
            ArchFileName=DBArchPath+QDir::separator()+ArchFileName+".dbf";

            MoveFileSys(DBSalesPath+QDir::separator()+BONUSFILE, ArchFileName);

            DbfMutex->lock();
            AttachDbfTable(BonusTable,DBSalesPath+QDir::separator()+BONUSFILE,BonusTableRecord);
            DbfMutex->unlock();

            //create act arh
            ArchFileName="act"+GetCurDate()+' '+GetCurTime();
            for (unsigned int i=0;i<ArchFileName.size();i++)
                if ((ArchFileName[i]==' ')||(ArchFileName[i]==':')||(ArchFileName[i]=='.'))
                    ArchFileName[i]='_';
            ArchFileName=DBArchPath+QDir::separator()+ArchFileName+".dbf";

            MoveFileSys(DBTmpPath+QDir::separator()+ACTREPORT, ArchFileName);
            //rename((DBTmpPath+QDir::separator()+ACTREPORT).c_str(),ArchFileName.c_str());


            //create bank act arh
            ArchFileName="bankact"+GetCurDate()+' '+GetCurTime();
            for (unsigned int i=0;i<ArchFileName.size();i++)
                if ((ArchFileName[i]==' ')||(ArchFileName[i]==':')||(ArchFileName[i]=='.'))
                    ArchFileName[i]='_';
            ArchFileName=DBArchPath+QDir::separator()+ArchFileName+".dbf";

            MoveFileSys(DBTmpPath+QDir::separator()+BANKACTREPORT, ArchFileName);


            if (ArchSize>0) //directory in MB //
            {
                QDir ArchDir(DBArchPath.c_str());//get sizes of all files in this directory
                QStringList FileNames = ArchDir.entryList(QDir::DefaultFilter,QDir::Time);
                FileNames.remove(".");
                FileNames.remove("..");
                QString ArchPath=(DBArchPath+QDir::separator()).c_str();

                QValueList<int> FileSizes;
                unsigned int i;
                for (i=0;i<FileNames.count();i++)
                    FileSizes.append(QFile(ArchPath+FileNames[i]).size());

                int DirSize;

                for(;;)
                {
                    DirSize=0;//remove the oldest file while directory's size is more
                    for (i=0;i<FileNames.count();i++)//than specified limit
                        DirSize+=FileSizes[i];

                    if ((DirSize<=ArchSize)||(FileNames.count()==0))
                        break;

                    if (QFile(ArchPath+FileNames.last()).remove())
                    {
                        FileNames.remove(FileNames.fromLast());
                        FileSizes.remove(FileSizes.fromLast());
                    }
                }
            }
            if (ArchSize<0){
                QDate cdate = QDate::currentDate();
                int curMonth = cdate.year()*12+cdate.month();
                int interval = maxSpace * -1;
                int modMonth;
                QDir ArchDir(DBArchPath.c_str());//get sizes of all files in this directory
                QStringList FileName = ArchDir.entryList(QDir::DefaultFilter,QDir::Time|QDir::Reversed);
                FileName.remove(".");
                FileName.remove("..");
                QString ArchPath=(DBArchPath+QDir::separator()).c_str();
                unsigned int i;
                for (i=0;interval < (curMonth - modMonth);i++){
                    QFileInfo fInf(ArchPath+FileName[i]);
                    QDateTime lastMod = fInf.lastModified();
                    modMonth = lastMod.date().year() * 12 + lastMod.date().month();
                    if ((curMonth - modMonth) > interval)
                        QFile::remove(ArchPath+FileName[i]);
                }
            }
//#endif
        }
        catch(cash_error& er)
        {
            LoggingError(er);
            return;
        }
    }
}

void CashRegister::OpenFRSmenaClick(void)//open kkm smena
{

    //int er1=FiscalReg->Reprint();
    //ShowErrorCode(er1);

    int er=FiscalReg->OpenKKMSmena();
    ShowErrorCode(er);


}

void CashRegister::ReprintClick(void)//open kkm smena
{
;;Log("[Proc] ReprintClick");
    int er=FiscalReg->Reprint();
    ShowErrorCode(er);

}

void CashRegister::CancelCheckClick(void)//open kkm smena
{
    ;;TryToCancelCheck();

}


void CashRegister::BankBonusReturnClick(void)//simple return of the check
{
    Log("PressButton>> BankBonusReturn");

    SumForm *dlg = new SumForm(this);//show simple dialog and get check's number
    dlg->FormCaption->setText(W2U("Номер чека"));

    int res=dlg->exec();

    QString s = dlg->LineEdit->text();
    string LogStr = " = ВОЗВРАТ БОНУСОВ СБЕРБАНКА ЧЕК # "+U2W(s)+ " =";

    Log((char*)LogStr.c_str());

    delete dlg;

    int depart;

    int retcheck = s.toInt();

    double sum=0;

    if (res==QDialog::Accepted)
    {
        try
        {


        //возврат баллов
             string card, sha1;
            string tran_id = "";

            DbfMutex->lock();

            //int retcheck = CurCheque->Cheque;
            double allbonus_summ=0;
            double total_summa=0;

            BankBonusTable->GetFirstRecord();//get goods information on the selected check
                                        //from the dbf-table of the current sales
            do
            {
                if ((BankBonusTable->GetLongField("CHECKNUM")!=retcheck)||(
                    BankBonusTable->GetLongField("ACTION")!=BB_DISCOUNT))
                {
                    continue;
                }

                tran_id = trim(BankBonusTable->GetField("TRAN_ID"));
                card = trim(BankBonusTable->GetField("CARD"));
                sha1 = trim(BankBonusTable->GetField("SHA1"));
                sum = -BankBonusTable->GetDoubleField("CHANGE_BNS");

                allbonus_summ = -BankBonusTable->GetDoubleField("CHANGE_BNS");
                total_summa = -BankBonusTable->GetDoubleField("TRAN_SUMMA");
            }
            while(BankBonusTable->GetNextRecord());

                DbfMutex->unlock();

            if (tran_id!="")
            {
                //;;Log("+ExecBonusReturnDisc+");
               // pCR->ExecBonusReturnDisc(card, sha1);
                //;;Log("+retdisc+");


                SumForm *dlg = new SumForm(this);//show simple dialog and get check's number
                dlg->FormCaption->setText(W2U("Сумма возврата"));
                dlg->LineEdit->setText(W2U(Double2Str(sum)));

                int res=dlg->exec();

                QString s = dlg->LineEdit->text();

                delete dlg;

                sum = s.toDouble();
                if (res==QDialog::Accepted)
                {

                            string LogStr= card;
                            LogStr = ">>> Bank BONUS Summ Return #"+LogStr+"# ="+Double2Str(sum);
                            Log((char*)LogStr.c_str());

                            PCX_RET* ret = WPCX->exec("retdisc", card, sha1, NULL, sum, tran_id);

                            ret->RetCheque = retcheck;
                            ret->total_summa = total_summa;

                            ret->action = BB_RETURN_DISC_GL; // разделим ручной возврат от автоматического

                            AddBankBonusOperation( ret ) ;

                            if (ret->ReturnCode>=0)
                            {

                                FiscalReg->PrintPCheck("/Kacca/pcx/p", false);
                                Log("  ...  SUCCESS");
                            }
                            else
                            {
                                ShowMessage(ret->info);
                                Log("   ...  ERROR");
                            }
                }
                //;;Log("-retdisc-");
            }
            else
            {
                string LogStr = "Нет данных по бонусам сбербанка по чеку №" +Int2Str(retcheck);
                ShowMessage(LogStr);
                Log((char*)(LogStr).c_str());
            }
        }

        catch(cash_error& ex)
        {
            LoggingError(ex);
        }
    }



}


#include "ExtControls.h"
#include "Utils.h"

void CashRegister::LocalReturnClick(void)//simple return of the check
{
    Log("PressButton>> ReturnCheck");

    FiscalReg->GetSmenaNumber(&KKMNo,&KKMSmena,&KKMTime);

    SumForm *dlg = new SumForm(this);//show simple dialog and get check's number

    dlg->FormCaption->setText(W2U("Номер чека"));

    int res=dlg->exec();

    QString s = dlg->LineEdit->text();
    string LogStr = " = ВОЗВРАТ ЧЕКА # "+U2W(s)+ " =";

    Log((char*)LogStr.c_str());

    delete dlg;

    int depart;

        depart = s.toInt();

    if (res==QDialog::Accepted)
    {
        try
        {
            DbfMutex->lock();
#ifdef RET_CHECK
            // Проверка чек на возвращенность (локально)
            if(ReturnTable->Locate("CHECKNUM",depart)){
                DbfMutex->unlock();
                ShowMessage("Этот чек уже возвращен.");
                Log(" >> Этот чек уже возвращен.");
                return;
            }
#endif
            //DbfMutex->unlock();
            FiscalCheque ReturnCheck;

            double CertSumma=0;

            //DbfMutex->lock();
            int RecordsCount=SalesTable->RecordCount();
            DbfMutex->unlock();

            bool LocalFound=true;

            if (RecordsCount==0)
            {
                //ShowMessage("Пустая база текущих продаж!");
                Log(" >> Пустая база текущих продаж!");
                //return;

                LocalFound=false;
            }

            DbfMutex->lock();

            if(!SalesTable->Locate("CHECKNUM",depart)){
                DbfMutex->unlock();
                //ShowMessage("Нет данных по указанному чеку.");
                Log(" >> Нет данных по указанному чеку.");
                //return;

                LocalFound = false;
            }


            if (!LocalFound)
            {
                DbfMutex->unlock();

                GlobalReturn(depart);
                return;
            }


            ReturnCheck.CashDesk = GetCashRegisterNum();
            ReturnCheck.Cheque = SalesTable->GetLongField("CHECKNUM");
            ReturnCheck.Date = SalesTable->GetField("DATE");

            SalesTable->GetFirstRecord();//get goods information on the selected check
                                        //from the dbf-table of the current sales
            do
            {
                if ((SalesTable->GetLongField("CHECKNUM")!=depart)||(
                    SalesTable->GetLongField("OPERATION")!=SL)){
                    continue;
                }
                int code = SalesTable->GetLongField("CODE");
                if((code == TRUNC) || (code == EXCESS_PREPAY))
                {
                    //ReturnCheck.tsum = SalesTable->GetDoubleField("QUANT");
                    continue;
                }

                GoodsInfo tmpGoods;
                tmpGoods.ItemCode = code;
                tmpGoods.ItemBarcode = SalesTable->GetStringField("BARCODE");
                tmpGoods.ItemSecondBarcode = SalesTable->GetStringField("SBARCODE");
                tmpGoods.ItemCount=SalesTable->GetDoubleField("QUANT");
                tmpGoods.ItemPrice=SalesTable->GetDoubleField("PRICE");
                tmpGoods.ItemFullPrice=SalesTable->GetDoubleField("FULLPRICE");
                tmpGoods.ItemMinimumPrice=SalesTable->GetDoubleField("MINPRICE");
                tmpGoods.ItemCalcPrice=SalesTable->GetDoubleField("CALCPRICE");
                tmpGoods.ItemSum=SalesTable->GetDoubleField("SUMMA");
                tmpGoods.ItemSumToPay=SalesTable->GetDoubleField("SUMMAPAY");
                tmpGoods.ItemFullSum=SalesTable->GetDoubleField("FULLSUMMA");
                tmpGoods.ItemFullDiscSum=SalesTable->GetDoubleField("FULLDSUMMA");
                tmpGoods.ItemBankBonusDisc=SalesTable->GetDoubleField("BANKBONUS");

                tmpGoods.ItemBonusAdd=SalesTable->GetDoubleField("BONUSADD");
                tmpGoods.ItemBonusDisc=SalesTable->GetDoubleField("BONUSDISC");

                tmpGoods.ItemBankDiscSum=SalesTable->GetDoubleField("BANKDSUMMA");
                tmpGoods.ItemCalcSum=SalesTable->GetDoubleField("CALCSUMMA");
                tmpGoods.ItemFlag=SL;
                tmpGoods.GuardCode=SalesTable->GetLongField("CASHMAN");;
                tmpGoods.KKMNo=KKMNo;
                tmpGoods.KKMSmena=KKMSmena;
                tmpGoods.KKMTime=KKMSmena;

                tmpGoods.PaymentType=SalesTable->GetLongField("PAYTYPE");

                tmpGoods.Avans=SalesTable->GetLongField("AVANS");
                tmpGoods.ItemCigar=SalesTable->GetLongField("CIGAR");
                tmpGoods.ItemAlco=SalesTable->GetLongField("ALCO");


                tmpGoods.ItemActsizType=SalesTable->GetLongField("ACTSIZTYPE");

                // TODO переделать, тип хранится в ItemActsizType, поэтому
                // ItemTextile, ItemAlco, ItemCigar можно убрать и переделать на ItemActsizType
                // пока оставлено для совместимости
                tmpGoods.ItemTextile=(tmpGoods.ItemActsizType == AT_TEXTILE);

                tmpGoods.ItemMilk=(tmpGoods.ItemActsizType == AT_MILK);
#ifdef ALCO_VES
                Log("tmpGoodsItemPivo=AT_PIVO_RAZLIV" + Int2Str(tmpGoods.ItemMilk));


                tmpGoods.ItemPivoRazliv = (tmpGoods.ItemActsizType == AT_PIVO_RAZLIV);
                Log("tmpGoodsItemPivo=AT_PIVO_RAZLIV" + Int2Str(tmpGoods.ItemPivoRazliv));
#endif

                //tmpGoods.ItemActsizBarcode=SalesTable->GetStringField("ACTSIZ");

                // + dms === информация о возвращаемом чеке ===
                tmpGoods.RetCheck=depart;
                // - dms

                // + dms === секция (отдел) ФР ===
                tmpGoods.Section=SalesTable->GetLongField("SECTION");

                string LockStr ="";

                int Pos = 0;
                if (tmpGoods.Section==1) Pos=LOCK_POS_ORG_INFO;
                else  if (tmpGoods.Section==2) Pos=LOCK_POS_ORG2_INFO;
                else  if (tmpGoods.Section==3) Pos=LOCK_POS_ORG3_INFO;
                else  if (tmpGoods.Section==4) Pos=LOCK_POS_ORG4_INFO;
                else  if (tmpGoods.Section==5) Pos=LOCK_POS_ORG5_INFO;
                else  if (tmpGoods.Section==6) Pos=LOCK_POS_ORG6_INFO;

                if (Pos>0)
                {
                    while(LockStr.length()<Pos-1) LockStr+="X";
                    LockStr+=".";
                    tmpGoods.Lock = LockStr;
                }

                // - dms

                if (GoodsTable->Locate("CODE",tmpGoods.ItemCode))
                {
                    if(GoodsTable->GetFieldNo("NDS") != -1){
                        tmpGoods.NDS = GoodsTable->GetDoubleField("NDS");
                    }
                    tmpGoods.ItemName=GoodsTable->GetStringField("NAME");
                    if (GoodsTable->GetLogicalField("GTYPE"))
                        tmpGoods.ItemPrecision=1;
                    else
                        tmpGoods.ItemPrecision=0.001;
                }
                else
                {
                    if (code == FreeSumCode)
                    {
                        tmpGoods.ItemName=SalesTable->GetStringField("NAME");
                        tmpGoods.ItemPrecision=0.001;
                    }
                    else
                    {
                        // ругаться
                        tmpGoods.ItemName="";
                        tmpGoods.ItemPrecision=1;
                    }
                }
                tmpGoods.DiscFlag=false;
                tmpGoods.CustomerCode[0]=SalesTable->GetLLongField("CUSTOMER");
                if (tmpGoods.CustomerCode[0]!=0)
                    tmpGoods.DiscFlag=true;
                else
                    tmpGoods.DiscFlag=false;

//				if( tmpGoods.PaymentType = SalesTable->GetLongField("PAYTYPE") ){
//					ShowMessage("Возврат безнала осуществляется через кассу магазина!");
//					Log("Возврат безнала осуществляется через кассу магазина!");
//!					DbfMutex->unlock();
//!					return;
//				}

                CertSumma += tmpGoods.ItemFullDiscSum - tmpGoods.ItemSum;

                ReturnCheck.Insert(tmpGoods);
            }
            while(SalesTable->GetNextRecord());

            DbfMutex->unlock();

            if (fabs(CertSumma)>epsilon)
            {

                if (!ShowQuestion("Продажа с использованием сертификатов! Продолжить?", true))
                    return;
            }


            if (ReturnCheck.GetCount()==0)
            {
                ShowMessage("Нечего возвращать!");
                Log("Нечего возвращать!");
                return;
            }

            ProcessReturn(&ReturnCheck);//show return dialog
        }
        catch(cash_error& ex)
        {
            DbfMutex->unlock();
            LoggingError(ex);
        }
    }
}

void CashRegister::GlobalReturnClick(void)//return of the check
{
    GlobalReturn();
}


void CashRegister::GlobalReturn(int num)//return of the check
{
    CheckSearchForm *dlg = new CheckSearchForm(this);//show the dialog where
                                        //cashdesk's number, check's number and sale's date
/*
    QRect r = dlg->CashDeskEdit->frameGeometry();
    delete dlg->CashDeskEdit;
    vector<BarcodeInfo*> tmpBarcodeTable;
    vector<int> Codes,PrefixSeq,PostfixSeq;//scanner's prefix and postfix,scancode's flow,weight's prefices of the barcode
    try
    {
        DbfMutex->lock();
        if (BarInfoTable->GetFirstRecord()){//read id/name of barcode's systems
            do
            {
                BarcodeInfo *tmpInfo=new BarcodeInfo;
                tmpInfo->CodeID=BarInfoTable->GetStringField("CODEID");
                tmpInfo->CodeName=BarInfoTable->GetStringField("CODENAME");

                tmpBarcodeTable.insert(tmpBarcodeTable.end(),tmpInfo);
            }
            while(BarInfoTable->GetNextRecord());
        }

        DbfMutex->unlock();
    }
    catch(cash_error &er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }

    char a;
    unsigned int i;

    string PreFX="PREFIX_",PostFX="POSTFIX_";
    for (a='1';a<='4';a++)
        for (i=0;i<tmpBarcodeTable.size();i++)
        {
            if ((tmpBarcodeTable[i]->CodeName==PreFX+a)&&(!tmpBarcodeTable[i]->CodeID.empty()))
                PrefixSeq.insert(PrefixSeq.begin(),QString(tmpBarcodeTable[i]->CodeID.c_str()).toInt(NULL,16));
            if ((tmpBarcodeTable[i]->CodeName==PostFX+a)&&(!tmpBarcodeTable[i]->CodeID.empty()))
                PostfixSeq.insert(PostfixSeq.begin(),QString(tmpBarcodeTable[i]->CodeID.c_str()).toInt(NULL,16));
        }

    dlg->CashDeskEdit = (scanLine = new ExtQLineEdit(dlg, "", Codes, PrefixSeq, PostfixSeq));
    dlg->CashDeskEdit->setGeometry(r);

    scanLine->shift = true;
//	connect( dlg->CashDeskEdit, SIGNAL( returnPressed() ), dlg, SLOT( OKClick() ) );
//	dlg->CashDeskEdit->setFocus();
//*/
    QIntValidator *CashDeskValid = new QIntValidator(dlg->CashDeskEdit);//is entered
    CashDeskValid->setRange(1,1000000);
    dlg->CashDeskEdit->setValidator(CashDeskValid);

    QIntValidator *ChequeValid = new QIntValidator(dlg->ChequeEdit);
    ChequeValid->setRange(1,100000000);
    dlg->ChequeEdit->setValidator(ChequeValid);

    QIntValidator *DayValid = new QIntValidator(dlg->DateDayEdit);
    DayValid->setRange(1,31);
    dlg->DateMonthEdit->setValidator(DayValid);

    QIntValidator *MonthValid = new QIntValidator(dlg->DateMonthEdit);
    MonthValid->setRange(1,12);
    dlg->DateMonthEdit->setValidator(MonthValid);

    QIntValidator *YearValid = new QIntValidator(dlg->DateYearEdit);
    YearValid->setRange(1,10000);
    dlg->DateYearEdit->setValidator(YearValid);


    dlg->CashDeskEdit->setText(Int2Str( GetCashRegisterNum() ));
    dlg->ChequeEdit->setText(Int2Str( num ));


    if (dlg->exec()==QDialog::Accepted)
    {
        FiscalCheque ReturnCheck;

//		QString s = dlg->CashDeskEdit->text();
//		if(s[0] >= '0' && s[0] <= '9'){
#			if QT_VERSION < 0x030000
            ReturnCheck.Date = dlg->DateYearEdit->text();
#			endif
#			if QT_VERSION >= 0x030000
            ReturnCheck.Date = dlg->DateYearEdit->text().ascii();
#			endif
            if(dlg->DateMonthEdit->text().length()<2){
                ReturnCheck.Date += '0';
            }
            ReturnCheck.Date += dlg->DateMonthEdit->text().ascii();
            if(dlg->DateDayEdit->text().length()<2){
                ReturnCheck.Date += '0';
            }
            ReturnCheck.Date += dlg->DateDayEdit->text().ascii();
            SQLServer->GetReturnCheque( ReturnCheck.CashDesk = dlg->CashDeskEdit->text().toInt(),//get goods information
                ReturnCheck.Cheque = dlg->ChequeEdit->text().toInt(),						//from the SQL database
                (dlg->DateDayEdit->text()+'.'+dlg->DateMonthEdit->text()+'.'+
                    dlg->DateYearEdit->text()).latin1(),&ReturnCheck);
/*		}
        else
        {
            int code = s.mid(1, s.length() - 2).toInt();// % (1 << ((sizeof(int) - 1) * 8));
            SQLServer->GetReturnCheque( ReturnCheck.CashDesk = (code >> ((sizeof(int) - 1) * 8)),//get goods information
                ReturnCheck.Cheque = (code % (1 << ((sizeof(int) - 1) * 8))),						//from the SQL database
                    0,&ReturnCheck);
        }
*/
        if (ReturnCheck.GetCount()>0)
            ProcessReturn(&ReturnCheck); //show return dialog
    }

//	scanLine = NULL;

//	delete CashDeskValid;
    delete ChequeValid;
    delete DayValid;
    delete MonthValid;
    delete YearValid;

    delete dlg;
}

int CashRegister::ProcessReturn(FiscalCheque *CurCheque)//show return dialog,select goods and
{														//and print return's check
    unsigned int i,k;
    double tsum = FiscalCheque::tsum;
    double Summ = tsum, TotalSum;
    double SumBankBonus=0;
    double SumBankBonusTotal=0;

    double SumBonusAdd=0;
    double SumBonusTotalAdd=0;
    double SumBonusDisc=0;
    double SumBonusTotalDisc=0;

    double SumCertTotal=0;
    double SumCertNew=0;
    int error_code;
    int PaymentType=CashPayment;

    string bonus_ret_id="";
    string strCheck = Int2Str(GetLastCheckNum()+1);
    string card = "";

//Log((char*)("tsum= "+Double2Str(tsum)).c_str());

    if (CurCheque->GetCount()>0)
        PaymentType=(*CurCheque)[0].PaymentType;

    ReturnForm *dlg = new ReturnForm(this);
    dlg->ShowCheque(CurCheque);

    FiscalCheque AnotherCheque,CurrentCheque, ReturnCheck;
    dlg->NewCheque=&AnotherCheque;


    dlg->ResultReturnCheck=&ReturnCheck;

    // Включим сканер
    DbfMutex->lock();
    ComPortScan* Scanner = new ComPortScan(this
                    ,ConfigTable->GetLongField("SCANPORT")
                    ,ConfigTable->GetLongField("SCANBAUD")
                    );//init scanner's driver
    DbfMutex->unlock();

    if (dlg->exec()==QDialog::Accepted)
    {
        delete Scanner;
        Scanner=NULL;

        Log("if (dlg->exec()==QDialog::Accepted)");
        for (i=0;i<AnotherCheque.GetCount();i++)  //check that we have enough money
        {
            Summ+=AnotherCheque[i].ItemSum;
            SumBankBonus+=AnotherCheque[i].ItemBankBonusDisc;

            //+ dms === 2013-10-17 ======
            SumCertNew+=AnotherCheque[i].ItemFullDiscSum - AnotherCheque[i].ItemSum;

            SumBonusAdd+=AnotherCheque[i].ItemBonusAdd;
            SumBonusDisc+=AnotherCheque[i].ItemBonusDisc;

        }

        for (i=0;i<CurCheque->GetCount();i++)
        {
            Summ-=(*CurCheque)[i].ItemSum;
            SumBankBonus-=(*CurCheque)[i].ItemBankBonusDisc;
            //+ dms === 2013-10-17 ======
            SumBankBonusTotal +=(*CurCheque)[i].ItemBankBonusDisc;
            SumCertTotal+=(*CurCheque)[i].ItemFullDiscSum - (*CurCheque)[i].ItemSum;

            SumBonusAdd-=(*CurCheque)[i].ItemBonusAdd;
            SumBonusDisc-=(*CurCheque)[i].ItemBonusDisc;

            SumBonusTotalAdd +=(*CurCheque)[i].ItemBonusAdd;
            SumBonusTotalDisc +=(*CurCheque)[i].ItemBonusDisc;

        }




        if (PaymentType==CashPayment)
        {
            error_code=FiscalReg->GetCurrentSumm(TotalSum);

            if (error_code==0)
            {
                if (Summ+TotalSum<0)
                {
                    // + dms
                    if ((FiscalRegType!=ShtrichVer3EntirePrinterType) && (FiscalRegType!=ShtrichVer3PrinterType))
                    {
                        ShowMessage("Не хватает наличных денег в ККМ!");
                        return 1;
                    }
                    // - dms
                }
            }
            else
            {
                ShowErrorCode(error_code);
                return error_code;
            }
        }
#ifndef _WS_WIN_
        else if(PaymentType > CashPayment && (CardReaderType & VFCR)){
            //if(pCR->Return(Double2Int((-100) * Summ))){
                // ошибка безналичного расчета
            //	return 1;//?
            //}
        }
#endif

        //возврат баллов
        if (SumBankBonus<0)
        {
            string card, sha1;
            string tran_id = "";

            DbfMutex->lock();

            int retcheck = CurCheque->Cheque;
            double allbonus_summ=0;

            BankBonusTable->GetFirstRecord();//get goods information on the selected check
                                        //from the dbf-table of the current sales
            do
            {
                if ((BankBonusTable->GetLongField("CHECKNUM")!=retcheck)||(
                    BankBonusTable->GetLongField("ACTION")!=BB_DISCOUNT))
                {
                    continue;
                }

                tran_id = trim(BankBonusTable->GetField("TRAN_ID"));
                card = trim(BankBonusTable->GetField("CARD"));
                sha1 = trim(BankBonusTable->GetField("SHA1"));

                allbonus_summ = -BankBonusTable->GetDoubleField("CHANGE_BNS");
            }
            while(BankBonusTable->GetNextRecord());

                DbfMutex->unlock();

            if (tran_id!="")
            {
//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========
/*
                ;;Log("+ExecBonusReturnDisc+");
               // pCR->ExecBonusReturnDisc(card, sha1);
                ;;Log("+retdisc+");

                string LogStr= card;
                LogStr = ">>> Bank BONUS Summ Return #"+LogStr+"# ="+Double2Str(-SumBankBonus);
                Log((char*)LogStr.c_str());

                PCX_RET* ret = WPCX->exec("retdisc", card, sha1, NULL, -SumBankBonus, tran_id);

                ret->RetCheque = CurCheque->Cheque;
                ret->total_summa = CurCheque->GetBankBonusSum();

                AddBankBonusOperation( ret ) ;

                if (ret->ReturnCode>=0)
                {

                    FiscalReg->PrintPCheck("/Kacca/pcx/p", false);
                    Log("  ...  SUCCESS");
                }
                else
                {
                    ShowMessage(ret->info);
                    Log("   ...  ERROR");
                }

                ;;Log("-retdisc-");
*/
//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========
            }
            else
            {
                string LogStr = "Нет данных по бонусам сбербанка по чеку №" +Int2Str(CurCheque->Cheque);
                ShowMessage(LogStr);
                Log((char*)(LogStr).c_str());
            }


        } // возврат бонусов


        //возврат баллов
        if ((SumBonusAdd<0) || (SumBonusDisc<0))
        {
            string tran_id="";
            DbfMutex->lock();

            int retcheck = CurCheque->Cheque;
            double allbonus_summ=0;

            BonusTable->GetFirstRecord();//get goods information on the selected check
                                        //from the dbf-table of the current sales
            do
            {
                if ((BonusTable->GetLongField("CHECKNUM")!=retcheck)||(
                    BonusTable->GetLongField("ACTION")!=13))  // операция фиксации транзакции покупки
                {
                    continue;
                }

                tran_id = trim(BonusTable->GetField("TRAN_ID"));
                card = trim(BonusTable->GetField("CARD"));

            }
            while(BonusTable->GetNextRecord());

                DbfMutex->unlock();

            if (tran_id!="")
            {

                string LogStr= card;
                LogStr = ">>> BONUS Summ Return #"+LogStr+"# ="+Double2Str(-SumBankBonus);
                Log((char*)LogStr.c_str());

                //! Новая транзакция возврата
                // В ней будет
                // 1) сумма начисленных бонусов и
                // 2) сумма оплаченных бонусов (если будет списание)

                // Инициализация покупки, присвоение нового ИД
                BNS_RET* ret = BNS->exec("return&init", card, strCheck, &ReturnCheck, -SumBonusAdd, -SumBonusDisc, tran_id);

                ;;Log((char*)Int2Str(ret->errorCode).c_str());
                ;;Log((char*)ret->id.c_str());

                AddBonusOperation( ret ) ;

                bonus_ret_id = ret->id;

                if (ret->ReturnCode)
                {
                    // если ошибка инициализации покупки, то дальше не идем
                    ;;Log("ERROR INIT BONUS RETURN");
                    bonus_ret_id="";
                    //return -2;
                }

            }
            else
            {
                string LogStr = "Нет данных по бонусам по чеку №" +Int2Str(CurCheque->Cheque);
                ShowMessage(LogStr);
                Log((char*)(LogStr).c_str());
            }


        } // возврат бонусов

        try
        {
            CurrentCheque.Clear();



            if (AnotherCheque.GetCount()>0)
            {//print sale's check which we must give to customer


                if (fabs(SumCertTotal-SumCertNew)>epsilon)
                {
                    CalcCertificateDiscounts(&AnotherCheque, SumCertTotal-SumCertNew);
                }

                m_vem.StartDoc(1,GetLastCheckNum()+1);
                for (i=0;i<AnotherCheque.GetCount();i++)
                {
//					DbfMutex->lock();
//					WriteGoodsIntoTable(CurrentCheckTable,AnotherCheque[i],GetCurTime());
//					DbfMutex->unlock();
//
//					ProcessRegError(FiscalReg->Sale(AnotherCheque[i].ItemName.c_str(),
//						AnotherCheque[i].ItemFullPrice,AnotherCheque[i].ItemCount,
//						AnotherCheque[i].Section));

                    CurrentCheque.Insert(AnotherCheque[i]);
//					m_vem.AddGoods(-1, &AnotherCheque[i], AnotherCheque.GetCalcSum(), 4);
                }

//				FiscalReg->OpenDrawer();
//				AddSalesToReg(&AnotherCheque);
////				Round();

           if (fabs(SumCertNew)>epsilon)
                 FiscalCheque::CertificateSumm = SumCertTotal;

            if (fabs(SumBankBonus)>epsilon)
                FiscalCheque::BankBonusSumm = SumBankBonus;


//				ProcessRegError(
//					FiscalReg->Close(
//						 AnotherCheque.GetSum()
//						,AnotherCheque.GetDiscountSum()
//						,GetCashRegisterNum()
//						,GetLastCheckNum()+1
//						,PaymentType
//						,AnotherCheque.GetCustomerCode()
//						,CurrentSaleman->Name
//						,this->GetCheckHeader()
//						,this->GetCheckFutter()
//						,false
//					)
//				);
//				m_vem.Calc(AnotherCheque.GetCalcSum(),AnotherCheque.GetDiscountSum(), PaymentType ? 2 : 1);
//				m_vem.Sell(AnotherCheque.GetCalcSum(),AnotherCheque.GetDiscountSum(), PaymentType ? 2 : 1);
//				m_vem.Pay(AnotherCheque.GetCalcSum(),AnotherCheque.GetDiscountSum(), PaymentType ? 2 : 1,
//					AnotherCheque.GetSum(),0);
//
//				FixCheque(&AnotherCheque);
//				SavePaymentFromReg();
            }
        }
        catch(int er)
        {
//			DbfMutex->lock();
//			WriteChequeIntoTable(CurrentCheckTable,&CurrentCheque,GetCurTime());
//			DbfMutex->unlock();
//
//			ShowErrorCode(er);
//			TryToCancelCheck();
//			AnnulCheck(&AnotherCheque,-1);
//			FixCheque(&AnotherCheque);
//			SavePaymentFromReg();

            //return er;
        }
        catch(cash_error &er)
        {
            DbfMutex->unlock();
            LoggingError(er);
            //return er.code();
        }


        try
        {

            if (CurrentCheque.GetCount()>0)
            {
                //!FiscalReg->WaitNextHeader();

                string CopyStr="КОПИЯ ЧЕКА"; //print the copy of the previous check
                unsigned int CopyLen=CopyStr.length(); //for internal purposes
                if (CopyLen<FiscalReg->LineLength)
                    for (i=0;i<(FiscalReg->LineLength-CopyLen)/2;i++)
                        CopyStr=' '+CopyStr;
                ProcessRegError(FiscalReg->PrintString(CopyStr.c_str()));
// + dms
                //ProcessRegError(FiscalReg->CreateInfoStr(GetCashRegisterNum(),GetLastCheckNum(),PaymentType,this->GetCheckHeader().c_str(),this->GetCheckFutter().c_str()));
                ProcessRegError(FiscalReg->CreateInfoStr(GetCashRegisterNum(),-1,PaymentType,this->GetCheckHeader().c_str(),this->GetCheckFutter().c_str()));

                string CashmanName = "Кассир " + CurrentSaleman->Name;
                ProcessRegError(FiscalReg->PrintString(CashmanName.c_str(), false));
                ProcessRegError(FiscalReg->PrintString("---------------------------------------", false));
                double FullSum=0;

                double sum, SumSec[11];
                sum=0;
                for(unsigned int j=0;j<11;j++) SumSec[j]=0;

                double sum_cert=0;

                for (i=0;i<CurrentCheque.GetCount();i++)
                {

                    SalesRecord tmpSales;
                    string tCode = CurrentCheque[i].ItemCode == FreeSumCode ? "" :Int2Str(CurrentCheque[i].ItemCode) + ' ';
                    tmpSales.Name = tCode.c_str() + CurrentCheque[i].ItemName;
                    tmpSales.Price=CurrentCheque[i].ItemPrice;
                    tmpSales.Quantity=CurrentCheque[i].ItemCount;
                    tmpSales.Sum = CurrentCheque[i].ItemSum;

                    // полная сумма без скидок
                    tmpSales.FullSum = CurrentCheque[i].ItemFullDiscSum;
                    double FullPrice = 0;
                    if (CurrentCheque[i].ItemCount!=0)
                        FullPrice = RoundToNearest(CurrentCheque[i].ItemFullDiscSum/CurrentCheque[i].ItemCount,0.01);

                    tmpSales.FullPrice=FullPrice;

                    tmpSales.Type=CurrentCheque[i].ItemFlag;

                    tmpSales.Section = CurrentCheque[i].Section;

                    tmpSales.Manufacture = GetOrgInfo(tmpSales.Section);
Log("TestCurrentCheque Kol=" + Double2Str(tmpSales.Quantity));

                    if (FiscalReg->IsFullCheck())
#ifndef _PRINT_FULL_PRICE_
                    ProcessRegError(FiscalReg->GoodsStr(tmpSales.Name.c_str(),tmpSales.Price,tmpSales.Quantity,tmpSales.Sum));
#else
                    ProcessRegError(FiscalReg->GoodsStr(tmpSales.Name.c_str(),tmpSales.FullPrice,tmpSales.Quantity,tmpSales.FullSum));
#endif

                    else
                        ProcessRegError(FiscalReg->GoodsStr(tmpSales.Name.c_str(),tmpSales.FullPrice,tmpSales.Quantity,tmpSales.FullSum));

                    // Вывод информации по производителю
                    if (!Empty(tmpSales.Manufacture))
                    {
                        ProcessRegError(FiscalReg->PrintString(tmpSales.Manufacture.c_str()));
                    }

                    double cursum = RoundToNearest(tmpSales.Sum,.01);

                    switch (tmpSales.Section)
                    {
                    case 1:  // секция 1 - поставщик Полюс
                    case 2:  // секция 2 - поставщик Глушков
                    case 3:  // секция 3 - страховые полисы ООО СК «ВТБ Страхование»
                             SumSec[tmpSales.Section]+=cursum; break;

                    default:  SumSec[0]+=cursum; break;// секция 0 - по умолчанию
                    }

                    sum += RoundToNearest(tmpSales.Sum,.01);

                    FullSum+=CurrentCheque[i].ItemSum;


                    sum_cert+= CurrentCheque[i].ItemFullDiscSum - CurrentCheque[i].ItemSum;

/*
                    ProcessRegError(FiscalReg->PrintString(CurrentCheque[i].ItemName.c_str()));

                    {
                        CopyStr=GetRounded(CurrentCheque[i].ItemCount,0.001)+" X      ";
                        if (FiscalReg->IsFullCheck())
                            GoodsStr(tmpSales.Name.c_str(),tmpSales.Price,tmpSales.Quantity,tmpSales.Sum);
                            CopyStr+=GetRounded(CurrentCheque[i].ItemPrice,0.01);
                        else
                            CopyStr+=GetRounded(CurrentCheque[i].ItemFullPrice,0.01);
                        CopyLen=CopyStr.length();
                        if (CopyLen<FiscalReg->LineLength)
                            for (k=0;k<FiscalReg->LineLength-CopyLen;k++)
                                CopyStr=' '+CopyStr;
                        ProcessRegError(FiscalReg->PrintString(CopyStr.c_str()));
                    }

                    if (FiscalReg->IsFullCheck())
                        ProcessRegError(FiscalReg->CreatePriceStr("",CurrentCheque[i].ItemSum));
                    else
                        ProcessRegError(FiscalReg->CreatePriceStr("",CurrentCheque[i].ItemCalcSum));

                    FullSum+=CurrentCheque[i].ItemSum;
                }

    */
            }

            ProcessRegError(FiscalReg->PrintString("---------------------------------------", false));
            ProcessRegError(PrintDiscountInfo(&CurrentCheque));

            double DiscSum=CurrentCheque.GetDiscountSum();
            if (fabs(DiscSum)>epsilon)
            {
                if (fabs(SumCertTotal)>1)
                {
                    ProcessRegError(FiscalReg->CreatePriceStr("СЕРТИФИКАТ",SumCertTotal));
                    ProcessRegError(FiscalReg->CreatePriceStr("СКИДКА",(DiscSum-SumCertTotal)<0?0:DiscSum-SumCertTotal));
                }
                else
                    ProcessRegError(FiscalReg->CreatePriceStr("СКИДКА",DiscSum));

            }

          for (unsigned int j=0;j<10;j++)
            {
                if (SumSec[j])
                {
                    ProcessRegError(FiscalReg->CreatePriceStr(Int2Str(j+1).c_str(), SumSec[j]));
                }
            }

           //if (sum0) ProcessRegError(FiscalReg->CreatePriceStr("1",sum0));
           //if (sum1) ProcessRegError(FiscalReg->CreatePriceStr("2",sum1));
           //if (sum2) ProcessRegError(FiscalReg->CreatePriceStr("3",sum2));

           ProcessRegError(FiscalReg->CreatePriceStr("ИТОГ",FullSum));
           //!ProcessRegError(FiscalReg->PrintHeader());

                FiscalReg->PrintString("", false);
                FiscalReg->PrintString("", false);
                FiscalReg->PrintHeader();

                FiscalReg->CutCheck();

          }
        }
        catch(int er)
        {
            ShowErrorCode(er);
            return er;
        }



// Возврат чека


        try
        {
            m_vem.StartDoc(2,GetLastCheckNum()+1);
            CurrentCheque.Clear();
            FiscalCheque::tsum = tsum;

            for (i=0;i<ReturnCheck.GetCount();i++)  //print the check of 'return'
            {
                GoodsInfo tmpGoods=ReturnCheck[i];
                tmpGoods.ItemFlag=RT;

                tmpGoods.Actsiz.clear();
               for (int j=0;j<ReturnCheck[i].Actsiz.size();j++)
                {
                    tmpGoods.Actsiz.insert(tmpGoods.Actsiz.end(), ReturnCheck[i].Actsiz[j]);
                }


                DbfMutex->lock();
                WriteGoodsIntoTable(CurrentCheckTable,tmpGoods,GetCurTime());
                DbfMutex->unlock();

                m_vem.Return(-1, 1, &tmpGoods, ReturnCheck.GetCalcSum());
                ProcessRegError(
                    FiscalReg->Return(
                        tmpGoods.ItemName.c_str()
                        ,tmpGoods.ItemFullPrice
                        ,tmpGoods.ItemCount
                        ,tmpGoods.Section
                    )
                );

                CurrentCheque.Insert(tmpGoods);
            }

            if (CurrentCheque.GetCount()>0)
            {
                //Возврат ЕГАИС
                int StatusEGAIS=-1;
                StatusEGAIS = CheckEGAIS(&CurrentCheque, true);
                if (StatusEGAIS)
                {
                    ShowMessage("Ошибка проверки чека в ЕГАИС!");
                    return -1;
                }


                // Возврат по терминалу сбербанка
                if(PaymentType > CashPayment
                //&& ((CardReaderType & VFCR || CardReaderType & VFUP))
                )
                {

                string bankName = "---";

                switch (PaymentType)
                {
                case CL_SBERBANK:
                case CL_VISA:
                case CL_MASTERCARD:
                case CL_MAESTRO:
                    bankName = "СБЕРБАНК";
                    break;
                case CL_VTB24:
                case CL_VISA_VTB:
                case CL_MASTERCARD_VTB:
                case CL_MAESTRO_VTB:
                case CL_UNKNOWN_VTB:
                    bankName = "ВТБ24";
                    break;
                case CL_GAZPROMBANK:
                case CL_VISA_GZ:
                case CL_MASTERCARD_GZ:
                case CL_MAESTRO_GZ:
                case CL_UNKNOWN_GZ:
                    bankName = "Газпромбанк";
                    break;
                default:
                    bankName = "<ОШИБКА: терминал не определен!>";
                    break;
                }

        ;;Log((char*)("=ВОЗВРАТ БЕЗНАЛА ЧЕРЕЗ ТЕРМИНАЛ "+bankName+"=").c_str());


            QString SecondButton = QString::null;
            QString ThirdButton = QString::null;
            string text = "Вернуть по безналу через терминал "+bankName+ "?";


            int lKeyPressed = -1;
            lKeyPressed = QMessageBox::information
            ( NULL, "APM KACCA", W2U(text)
            ,W2U("На банк. карту через ТЕРМИНАЛ"), W2U("Наличными из гл. кассы"), W2U("Отмена")
            , 0, -1);

            if (lKeyPressed!=0 && lKeyPressed!=1) return -1; // отказ

                    //if (ShowQuestion("Вернуть по безналу через терминал "+bankName+ "?", true))
                    if (lKeyPressed==0)
                    {
;;Log("=2= beznal start");

                       string RRN = "000000000000";
                       if (bankName == "ВТБ24"){

                            SumForm *dlg = new SumForm(this);//show simple dialog and get RRN

                            dlg->FormCaption->setText(W2U("Номер ссылки RRN"));

                            if (dlg->exec())
                            {
                                QString s = dlg->LineEdit->text();

                                string LogStr = " = s(RRN) # "+U2W(s)+ " =";
                                Log((char*)LogStr.c_str());

                                if (!s.isEmpty())
                                {
                                    RRN = U2W(s);
                                    RRN = trim(RRN);
                                }
                                else
                                {
                                    RRN = "000000000000";
                                }

                                while (RRN.length()<12) RRN+="0";

                                LogStr = " = RRN # "+RRN+ " =";
                                Log((char*)LogStr.c_str());

                                delete dlg;

                            }
                       }

                       if(pCR->Return(Double2Int(CurrentCheque.GetSum() * (-100)), PaymentType, RRN))
                       {
;;Log("=3= beznal error");
                            // ошибка безналичного расчета

                            Log("   ....ERROR");

                            // Отмена последней операции проверки ЕГАИС =
                            // Продажа
                            CheckEGAIS(&CurrentCheque);

                            return -1;
                        }
;;Log("=4= beznal success end" );
                        Log("   ....SUCCESS");
                    }

                }

                FiscalReg->OpenDrawer();
                AddSalesToReg(&CurrentCheque);

                // + dms ============
                if (fabs(SumCertTotal)>epsilon)
                FiscalCheque::CertificateSumm = SumCertTotal;

                if (fabs(SumBankBonusTotal)>epsilon)
                   FiscalCheque::BankBonusSumm = SumBankBonusTotal;
                // - dms ============

                FiscalCheque::PrePayCertificateSumm = 0;

                ProcessRegError(
                    FiscalReg->Close(
                         fabs(CurrentCheque.GetSum())
                        ,fabs(CurrentCheque.GetSum())
                        ,CurrentCheque.GetDiscountSum()
                        ,GetCashRegisterNum()
                        ,GetLastCheckNum()+1
                        ,PaymentType
                        ,CurrentCheque.GetCustomerCode()
                        ,CurrentSaleman->Name
                        ,this->GetCheckHeader()
                        ,this->GetCheckFutter()
                        ,false
                        ,GetCheckEgaisHeader()
                        ,CurrentCheque.egais_url
                        ,CurrentCheque.egais_sign
                        ,EgaisSettings.ModePrintQRCode

                    )
                );

                FixCheque(&CurrentCheque);
                SavePaymentFromReg();




                if (bonus_ret_id!="")
                {
                    ;;Log("+ProcessBonusReturnFix+");

                    // Если транзакция активна - зафиксируем ее
                    ;;Log((char*)("bonus_ret_id = '"+bonus_ret_id+"'").c_str());
                    ;;Log("return&fix");
                    BNS_RET* ret = BNS->exec("return&fix", card, strCheck, &ReturnCheck, -SumBonusAdd, -SumBonusDisc, bonus_ret_id);
                    AddBonusOperation( ret ) ;

                    ;;Log("inf after fix");
                    ret = BNS->exec("inf", card, strCheck);
                    AddBonusOperation( ret ) ;

                    ;;Log("-ProcessBonusReturnFix-");

                }


            }
            else
            {
                ShowMessage("Нечего возвращать");
            }
        }
        catch(int er)
        {
            DbfMutex->lock();
            WriteChequeIntoTable(CurrentCheckTable,&CurrentCheque,GetCurTime());
            DbfMutex->unlock();

            ShowErrorCode(er);
            TryToCancelCheck();
            AnnulCheck(&CurrentCheque,-1);
            FixCheque(&CurrentCheque);
            SavePaymentFromReg();
            return er;
        }
        catch(cash_error &er)
        {
            DbfMutex->unlock();
            LoggingError(er);
            return er.code();
        }
#ifdef RET_CHECK
        // Заносим чек в локальную таблицу возвращенных
        DbfMutex->lock();
        ReturnTable->BlankRecord();
        ReturnTable->PutLongField("CHECKNUM",CurCheque->Cheque);
        ReturnTable->PutField("DATE",(CurCheque->Date).c_str());
        ReturnTable->PutLongField("CASHDESK",CurCheque->CashDesk);
        //ReturnTable->PutLongField("KKM",CurCheque->);
        //ReturnTable->PutLongField("CHECK",CurCheque->);
        ReturnTable->PutField("SENT","F");
        ReturnTable->AppendRecord();
        DbfMutex->unlock();
#endif
    }

  //  this->ClearPayment();

    if (Scanner!=NULL)
    {
        delete Scanner;
        Scanner=NULL;
    }


    return 0;
}


int CashRegister::ProcessReturn_Old(FiscalCheque *CurCheque)//show return dialog,select goods and
{														//and print return's check
    unsigned int i,k;
    double tsum = FiscalCheque::tsum;
    double Summ = tsum, TotalSum;
    double SumBankBonus=0;
    double SumBankBonusTotal=0;
    double SumCertTotal=0;
    double SumCertNew=0;
    int error_code;
    int PaymentType=CashPayment;


    if (CurCheque->GetCount()>0)
        PaymentType=(*CurCheque)[0].PaymentType;

    ReturnForm *dlg = new ReturnForm(this);
    dlg->ShowCheque(CurCheque);

    FiscalCheque AnotherCheque,CurrentCheque;
    dlg->NewCheque=&AnotherCheque;

    if (dlg->exec()==QDialog::Accepted)
    {

        Log("if (dlg->exec()==QDialog::Accepted)");
        for (i=0;i<AnotherCheque.GetCount();i++)  //check that we have enough money
        {
            Summ+=AnotherCheque[i].ItemSum;
            SumBankBonus+=AnotherCheque[i].ItemBankBonusDisc;

            //+ dms === 2013-10-17 ======
            SumCertNew+=AnotherCheque[i].ItemFullDiscSum - AnotherCheque[i].ItemSum;
        }

        for (i=0;i<CurCheque->GetCount();i++)
        {
            Summ-=(*CurCheque)[i].ItemSum;
            SumBankBonus-=(*CurCheque)[i].ItemBankBonusDisc;
            //+ dms === 2013-10-17 ======
            SumBankBonusTotal +=(*CurCheque)[i].ItemBankBonusDisc;
            SumCertTotal+=(*CurCheque)[i].ItemFullDiscSum - (*CurCheque)[i].ItemSum;
        }




        if (PaymentType==CashPayment)
        {
            error_code=FiscalReg->GetCurrentSumm(TotalSum);

            if (error_code==0)
            {
                if (Summ+TotalSum<0)
                {
                    // + dms
                    if ((FiscalRegType!=ShtrichVer3EntirePrinterType) && (FiscalRegType!=ShtrichVer3PrinterType))
                    {
                        ShowMessage("Не хватает наличных денег в ККМ!");
                        return 1;
                    }
                    // - dms
                }
            }
            else
            {
                ShowErrorCode(error_code);
                return error_code;
            }
        }
#ifndef _WS_WIN_
        else if(PaymentType > CashPayment && (CardReaderType & VFCR)){
//			if(pCR->Return(Double2Int((-100) * Summ))){
                // ошибка безналичного расчета
//				return 1;//?
//			}
        }
#endif

        //возврат баллов
        if (SumBankBonus<0)
        {
            string card, sha1;
            string tran_id = "";

            DbfMutex->lock();

            int retcheck = CurCheque->Cheque;
            double allbonus_summ=0;

            BankBonusTable->GetFirstRecord();//get goods information on the selected check
                                        //from the dbf-table of the current sales
            do
            {
                if ((BankBonusTable->GetLongField("CHECKNUM")!=retcheck)||(
                    BankBonusTable->GetLongField("ACTION")!=BB_DISCOUNT))
                {
                    continue;
                }

                tran_id = trim(BankBonusTable->GetField("TRAN_ID"));
                card = trim(BankBonusTable->GetField("CARD"));
                sha1 = trim(BankBonusTable->GetField("SHA1"));

                allbonus_summ = -BankBonusTable->GetDoubleField("CHANGE_BNS");
            }
            while(BankBonusTable->GetNextRecord());

                DbfMutex->unlock();

            if (tran_id!="")
            {
//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========
/*
                ;;Log("+ExecBonusReturnDisc+");
               // pCR->ExecBonusReturnDisc(card, sha1);
                ;;Log("+retdisc+");

                string LogStr= card;
                LogStr = ">>> Bank BONUS Summ Return #"+LogStr+"# ="+Double2Str(-SumBankBonus);
                Log((char*)LogStr.c_str());

                PCX_RET* ret = WPCX->exec("retdisc", card, sha1, NULL, -SumBankBonus, tran_id);

                ret->RetCheque = CurCheque->Cheque;
                ret->total_summa = CurCheque->GetBankBonusSum();

                AddBankBonusOperation( ret ) ;

                if (ret->ReturnCode>=0)
                {

                    FiscalReg->PrintPCheck("/Kacca/pcx/p", false);
                    Log("  ...  SUCCESS");
                }
                else
                {
                    ShowMessage(ret->info);
                    Log("   ...  ERROR");
                }

                ;;Log("-retdisc-");
*/
//===== 2015-11-20 ======= Завершаем программу Спасибо от 22.11.2015 ========
            }
            else
            {
                string LogStr = "Нет данных по бонусам сбербанка по чеку №" +Int2Str(CurCheque->Cheque);
                ShowMessage(LogStr);
                Log((char*)(LogStr).c_str());
            }


        } // возврат бонусов

        try
        {
            CurrentCheque.Clear();



            if (AnotherCheque.GetCount()>0)
            {//print sale's check which we must give to customer


                if (fabs(SumCertTotal-SumCertNew)>epsilon)
                {
                    CalcCertificateDiscounts(&AnotherCheque, SumCertTotal-SumCertNew);
                }

                m_vem.StartDoc(1,GetLastCheckNum()+1);
                for (i=0;i<AnotherCheque.GetCount();i++)
                {
                    DbfMutex->lock();
                    WriteGoodsIntoTable(CurrentCheckTable,AnotherCheque[i],GetCurTime());
                    DbfMutex->unlock();

                    ProcessRegError(FiscalReg->Sale(AnotherCheque[i].ItemName.c_str(),
                        AnotherCheque[i].ItemFullPrice,AnotherCheque[i].ItemCount,
                        AnotherCheque[i].Section));

                    CurrentCheque.Insert(AnotherCheque[i]);
                    m_vem.AddGoods(-1, &AnotherCheque[i], AnotherCheque.GetCalcSum(), 4);
                }

                FiscalReg->OpenDrawer();
                AddSalesToReg(&AnotherCheque);
//				Round();

           if (fabs(SumCertNew)>epsilon)
                 FiscalCheque::CertificateSumm = SumCertTotal;

            if (fabs(SumBankBonus)>epsilon)
                FiscalCheque::BankBonusSumm = SumBankBonus;

            FiscalCheque::PrePayCertificateSumm = 0;

                ProcessRegError(
                    FiscalReg->Close(
                         AnotherCheque.GetSum()
                        ,AnotherCheque.GetSum()
                        ,AnotherCheque.GetDiscountSum()
                        ,GetCashRegisterNum()
                        ,GetLastCheckNum()+1
                        ,PaymentType
                        ,AnotherCheque.GetCustomerCode()
                        ,CurrentSaleman->Name
                        ,this->GetCheckHeader()
                        ,this->GetCheckFutter()
                        ,false
                    )
                );
                m_vem.Calc(AnotherCheque.GetCalcSum(),AnotherCheque.GetDiscountSum(), PaymentType ? 2 : 1);
                m_vem.Sell(AnotherCheque.GetCalcSum(),AnotherCheque.GetDiscountSum(), PaymentType ? 2 : 1);
                m_vem.Pay(AnotherCheque.GetCalcSum(),AnotherCheque.GetDiscountSum(), PaymentType ? 2 : 1,
                    AnotherCheque.GetSum(),0);

                FixCheque(&AnotherCheque);
                SavePaymentFromReg();
            }
        }
        catch(int er)
        {
            DbfMutex->lock();
            WriteChequeIntoTable(CurrentCheckTable,&CurrentCheque,GetCurTime());
            DbfMutex->unlock();

            ShowErrorCode(er);
            TryToCancelCheck();
            AnnulCheck(&AnotherCheque,-1);
            FixCheque(&AnotherCheque);
            SavePaymentFromReg();

            return er;
        }
        catch(cash_error &er)
        {
            DbfMutex->unlock();
            LoggingError(er);
            return er.code();
        }

        try
        {

            if (CurrentCheque.GetCount()>0)
            {
                //!FiscalReg->WaitNextHeader();

                string CopyStr="КОПИЯ ЧЕКА"; //print the copy of the previous check
                unsigned int CopyLen=CopyStr.length(); //for internal purposes
                if (CopyLen<FiscalReg->LineLength)
                    for (i=0;i<(FiscalReg->LineLength-CopyLen)/2;i++)
                        CopyStr=' '+CopyStr;
                ProcessRegError(FiscalReg->PrintString(CopyStr.c_str()));
// + dms
                ProcessRegError(FiscalReg->CreateInfoStr(GetCashRegisterNum(),GetLastCheckNum(),PaymentType,this->GetCheckHeader().c_str(),this->GetCheckFutter().c_str()));

                string CashmanName = "Кассир " + CurrentSaleman->Name;
                ProcessRegError(FiscalReg->PrintString(CashmanName.c_str(), false));
                ProcessRegError(FiscalReg->PrintString("---------------------------------------", false));
                double FullSum=0;

                double sum, SumSec[11];
                sum=0;
                for(unsigned int j=0;j<11;j++) SumSec[j]=0;

                double sum_cert=0;

                for (i=0;i<CurrentCheque.GetCount();i++)
                {

                    SalesRecord tmpSales;
                    string tCode = CurrentCheque[i].ItemCode == FreeSumCode ? "" :Int2Str(CurrentCheque[i].ItemCode) + ' ';
                    tmpSales.Name = tCode.c_str() + CurrentCheque[i].ItemName;
                    tmpSales.Price=CurrentCheque[i].ItemPrice;
                    tmpSales.Quantity=CurrentCheque[i].ItemCount;
                    tmpSales.Sum = CurrentCheque[i].ItemSum;

                    // полная сумма без скидок
                    tmpSales.FullSum = CurrentCheque[i].ItemFullDiscSum;
                    double FullPrice = 0;
                    if (CurrentCheque[i].ItemCount!=0)
                        FullPrice = RoundToNearest(CurrentCheque[i].ItemFullDiscSum/CurrentCheque[i].ItemCount,0.01);

                    tmpSales.FullPrice=FullPrice;

                    tmpSales.Type=CurrentCheque[i].ItemFlag;

                    tmpSales.Section = CurrentCheque[i].Section;

                    tmpSales.Manufacture = GetOrgInfo(tmpSales.Section);


                    if (FiscalReg->IsFullCheck())
#ifndef _PRINT_FULL_PRICE_
                    ProcessRegError(FiscalReg->GoodsStr(tmpSales.Name.c_str(),tmpSales.Price,tmpSales.Quantity,tmpSales.Sum));
#else
                    ProcessRegError(FiscalReg->GoodsStr(tmpSales.Name.c_str(),tmpSales.FullPrice,tmpSales.Quantity,tmpSales.FullSum));
#endif

                    else
                        ProcessRegError(FiscalReg->GoodsStr(tmpSales.Name.c_str(),tmpSales.FullPrice,tmpSales.Quantity,tmpSales.FullSum));

                    // Вывод информации по производителю
                    if (!Empty(tmpSales.Manufacture))
                    {
                        ProcessRegError(FiscalReg->PrintString(tmpSales.Manufacture.c_str()));
                    }

                    double cursum = RoundToNearest(tmpSales.Sum,.01);

                    switch (tmpSales.Section)
                    {
                    case 1:  // секция 1 - поставщик Полюс
                    case 2:  // секция 2 - поставщик Глушков
                    case 3:  // секция 3 - страховые полисы ООО СК «ВТБ Страхование»
                             SumSec[tmpSales.Section]+=cursum; break;

                    default:  SumSec[0]+=cursum; break;// секция 0 - по умолчанию
                    }

                    sum += RoundToNearest(tmpSales.Sum,.01);

                    FullSum+=CurrentCheque[i].ItemSum;


                    sum_cert+= CurrentCheque[i].ItemFullDiscSum - CurrentCheque[i].ItemSum;

/*
                    ProcessRegError(FiscalReg->PrintString(CurrentCheque[i].ItemName.c_str()));

                    {
                        CopyStr=GetRounded(CurrentCheque[i].ItemCount,0.001)+" X      ";
                        if (FiscalReg->IsFullCheck())
                            GoodsStr(tmpSales.Name.c_str(),tmpSales.Price,tmpSales.Quantity,tmpSales.Sum);
                            CopyStr+=GetRounded(CurrentCheque[i].ItemPrice,0.01);
                        else
                            CopyStr+=GetRounded(CurrentCheque[i].ItemFullPrice,0.01);
                        CopyLen=CopyStr.length();
                        if (CopyLen<FiscalReg->LineLength)
                            for (k=0;k<FiscalReg->LineLength-CopyLen;k++)
                                CopyStr=' '+CopyStr;
                        ProcessRegError(FiscalReg->PrintString(CopyStr.c_str()));
                    }

                    if (FiscalReg->IsFullCheck())
                        ProcessRegError(FiscalReg->CreatePriceStr("",CurrentCheque[i].ItemSum));
                    else
                        ProcessRegError(FiscalReg->CreatePriceStr("",CurrentCheque[i].ItemCalcSum));

                    FullSum+=CurrentCheque[i].ItemSum;
                }

    */
            }

            ProcessRegError(FiscalReg->PrintString("---------------------------------------", false));
            ProcessRegError(PrintDiscountInfo(&CurrentCheque));

            double DiscSum=CurrentCheque.GetDiscountSum();
            if (fabs(DiscSum)>epsilon)
            {
                if (fabs(SumCertTotal)>1)
                {
                    ProcessRegError(FiscalReg->CreatePriceStr("СЕРТИФИКАТ",SumCertTotal));
                    ProcessRegError(FiscalReg->CreatePriceStr("СКИДКА",(DiscSum-SumCertTotal)<0?0:DiscSum-SumCertTotal));
                }
                else
                    ProcessRegError(FiscalReg->CreatePriceStr("СКИДКА",DiscSum));

            }

          for (unsigned int j=0;j<10;j++)
            {
                if (SumSec[j])
                {
                    ProcessRegError(FiscalReg->CreatePriceStr(Int2Str(j+1).c_str(), SumSec[j]));
                }
            }

           //if (sum0) ProcessRegError(FiscalReg->CreatePriceStr("1",sum0));
           //if (sum1) ProcessRegError(FiscalReg->CreatePriceStr("2",sum1));
           //if (sum2) ProcessRegError(FiscalReg->CreatePriceStr("3",sum2));

           ProcessRegError(FiscalReg->CreatePriceStr("ИТОГ",FullSum));
           //!ProcessRegError(FiscalReg->PrintHeader());

          }
        }
        catch(int er)
        {
            ShowErrorCode(er);
            return er;
        }



// Возврат оригинального чека полностью


        try
        {
            m_vem.StartDoc(2,GetLastCheckNum()+1);
            CurrentCheque.Clear();
            FiscalCheque::tsum = tsum;


            for (i=0;i<CurCheque->GetCount();i++)  //print the check of 'return'
            {
                GoodsInfo tmpGoods=(*CurCheque)[i];
                tmpGoods.ItemFlag=RT;

                DbfMutex->lock();
                WriteGoodsIntoTable(CurrentCheckTable,tmpGoods,GetCurTime());
                DbfMutex->unlock();

                m_vem.Return(-1, 1, &tmpGoods, CurCheque->GetCalcSum());
                ProcessRegError(
                    FiscalReg->Return(
                        tmpGoods.ItemName.c_str()
                        ,tmpGoods.ItemFullPrice
                        ,tmpGoods.ItemCount
                        ,tmpGoods.Section
                    )
                );

                CurrentCheque.Insert(tmpGoods);
            }
            FiscalReg->OpenDrawer();
            AddSalesToReg(&CurrentCheque);

            // + dms ============
            if (fabs(SumCertTotal)>epsilon)
            FiscalCheque::CertificateSumm = SumCertTotal;

            if (fabs(SumBankBonusTotal)>epsilon)
               FiscalCheque::BankBonusSumm = SumBankBonusTotal;
            // - dms ============

            FiscalCheque::PrePayCertificateSumm = 0;

            ProcessRegError(
                FiscalReg->Close(
                     fabs(CurrentCheque.GetSum())
                    ,fabs(CurrentCheque.GetSum())
                    ,CurrentCheque.GetDiscountSum()
                    ,GetCashRegisterNum()
                    ,GetLastCheckNum()+1
                    ,PaymentType
                    ,CurrentCheque.GetCustomerCode()
                    ,CurrentSaleman->Name
                    ,this->GetCheckHeader()
                    ,this->GetCheckFutter()
                    ,false
                )
            );

            FixCheque(&CurrentCheque);
            SavePaymentFromReg();
        }
        catch(int er)
        {
            DbfMutex->lock();
            WriteChequeIntoTable(CurrentCheckTable,&CurrentCheque,GetCurTime());
            DbfMutex->unlock();

            ShowErrorCode(er);
            TryToCancelCheck();
            AnnulCheck(&CurrentCheque,-1);
            FixCheque(&CurrentCheque);
            SavePaymentFromReg();
            return er;
        }
        catch(cash_error &er)
        {
            DbfMutex->unlock();
            LoggingError(er);
            return er.code();
        }
#ifdef RET_CHECK
        // Заносим чек в локальную таблицу возвращенных
        DbfMutex->lock();
        ReturnTable->BlankRecord();
        ReturnTable->PutLongField("CHECKNUM",CurCheque->Cheque);
        ReturnTable->PutField("DATE",(CurCheque->Date).c_str());
        ReturnTable->PutLongField("CASHDESK",CurCheque->CashDesk);
        //ReturnTable->PutLongField("KKM",CurCheque->);
        //ReturnTable->PutLongField("CHECK",CurCheque->);
        ReturnTable->PutField("SENT","F");
        ReturnTable->AppendRecord();
        DbfMutex->unlock();
#endif
    }

  //  this->ClearPayment();

    return 0;
}

void CashRegister::AnnulCheck(FiscalCheque *CurCheque,int GuardCode)//annul the given check
{
    ;;Log("AnnulCheck");
    m_vem.CancelCheck(0,0);


// + dms ==== 2014-08-20 ====
/*
;;Log("AnnulCheck = step 1");
    if ((WPCX!=NULL) && (CurCheque->BankBonusSumm>0))
    {// Cancel bank bonus discounts
        ;;Log(" Cancel bank bonus discounts (ANNUL)");
        if (CurrentWorkPlace!=NULL)
            CurrentWorkPlace->ProcessBankBonusDiscountReverse(WPCX->PCX_Result.card, WPCX->PCX_Result.sha1, true);
    }
*/



;;Log("AnnulCheck = step 2");

    // Cancel  bonus discounts
    ;;Log(" Cancel bank bonus discounts (ANNUL)");
    if (CurrentWorkPlace!=NULL)
       CurrentWorkPlace->ProcessBonusDiscountReverse(true);

;;Log("AnnulCheck = step 2");
    // Cancel egais
    ;;Log(" Cancel EGAIS (ANNUL)");
    // Отмена последней операции проверки ЕГАИС
    CheckEGAIS(CurCheque, true, true);

    if (CurrentWorkPlace!=NULL)
       CurrentWorkPlace->ProcessBonusDiscountReverse(true);


;;Log("AnnulCheck = step 4");
    FiscalCheque tmpCheque;
    for (unsigned int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo tmpGoods=(*CurCheque)[i];
        switch (tmpGoods.ItemFlag)
        {
            case SL:tmpGoods.ItemFlag=ASL;break;//change sale's attributes
            case RT:tmpGoods.ItemFlag=ART;break;
            case ST:tmpGoods.ItemFlag=AST;break;
            default:break;
        }

        if(!tmpGoods.GuardCode || tmpGoods.GuardCode == -1)
            tmpGoods.GuardCode=GuardCode;//set the code of the guard who confirmed annulment
        tmpCheque.Insert(tmpGoods);
    }

    CurCheque->Clear();
    ClearCurrentCheck();
    WriteChequeIntoTable(CurrentCheckTable,&tmpCheque,GetCurTime());

    FixCheque(&tmpCheque);
    SavePaymentFromReg();

}

void CashRegister::ClearCurrentCheck(void)
{
    try
    {
        DbfMutex->lock();
        CurrentCheckTable->Zap();
        DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
}

void CashRegister::DeletePrepayExcess(FiscalCheque* CurCheque)//
{
    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if (g.ItemCode==EXCESS_PREPAY) {
            g.ItemFlag=ST;
            CurCheque->SetAt(i,g);
        }
    }
}

void CashRegister::AddPrepayExcess(FiscalCheque* CurCheque)//
{

    ;;Log(" ===> Начало AddPrepayExcess");
    double Summ = CurCheque->GetSum();
    double PrePaySumm = FiscalCheque::PrePayCertificateSumm;

    if (PrePaySumm > 0)
    {
        if (PrePaySumm - Summ > kopeyka) {

            double RestSum = PrePaySumm - Summ;

            ;;Log(" Added Prepay Excess Summ = "+Double2Str(RestSum));

            string CurTime=GetCurTime(),CurDate=GetCurDate();

            GoodsInfo r;
            r.ItemCode = EXCESS_PREPAY;//превышение суммы сертификатов над суммой чека (остаток от сертификата)
            //r.ItemName = "//Превышение номинальной стоимости подарочной карты над продажной ценой товара";
            r.ItemName = "*Превышение номинальной стоимости подарочной карты над продажной ценой товара";
            r.ItemCount = 1;
            r.ItemPrice = RestSum;
            r.ItemFullPrice = RestSum;
            r.ItemMinimumPrice = RestSum;
            r.ItemSum = RestSum;
            r.ItemSumToPay = 0;
            r.ItemCalcSum = RestSum;
            r.ItemFullSum = RestSum;
            r.ItemFullDiscSum = RestSum;
            r.PaymentType = (*CurCheque)[0].PaymentType;
            r.ItemFlag = ((*CurCheque)[0].ItemFlag == RT ? RT : SL);
            for(int i=0;i<MAXCUSTOMERCOUNT;i++)
            {
                r.CustomerCode[i] = (*CurCheque)[0].CustomerCode[i];
            }
            r.KKMNo=KKMNo;
            r.KKMSmena=KKMSmena;

            time(&r.tTime);
            CurCheque->Insert(r);

        }
    }

    ;;Log(" ===> Конец AddPrepayExcess");
}

void CashRegister::FixCheque(FiscalCheque* CurCheque)//close the given check and try to send
{
    try
    {//stored information to SQL database

        FILE *storenum;
        m_vem.EndDoc();
        string CurTime=GetCurTime(),CurDate=GetCurDate();

        if(fabs(CurCheque->tsum) >= .005){
            GoodsInfo r;
            r.ItemCode = TRUNC;//округление
            r.ItemCount = CurCheque->tsum;
            r.ItemPrice = -1;
            r.ItemFullPrice = -1;
            r.ItemMinimumPrice = -1;
            r.ItemSum = -CurCheque->tsum;
            r.ItemSumToPay = 0;
            r.ItemCalcSum = -CurCheque->tsum;
            r.ItemFullSum = -CurCheque->tsum;
            r.ItemFullDiscSum = -CurCheque->tsum;
            r.PaymentType = (*CurCheque)[0].PaymentType;
            r.ItemFlag = ((*CurCheque)[0].ItemFlag == RT ? RT : SL);
            for(int i=0;i<MAXCUSTOMERCOUNT;i++)
            {
                r.CustomerCode[i] = (*CurCheque)[0].CustomerCode[i];
            }
            r.KKMNo=KKMNo;
            r.KKMSmena=KKMSmena;
            //Log("time(&r.tTime) !!!");
            time(&r.tTime);
            CurCheque->Insert(r);
        }

        LogFixCheque(CurCheque);

        WriteChequeIntoTable(NonSentTable,CurCheque,CurTime);

        WriteChequeIntoTable(SalesTable,CurCheque,CurTime);

        WriteCustomersSales(CurCheque);

        CurrentCheckNumber = 0;

        int tmpNum=GetLastCheckNum();

        storenum = fopen("./tmp/storenum.dat","r");
        if (storenum){
            int storechecknum;
            fscanf(storenum,"%d",&storechecknum);
            fclose(storenum);
            remove("./tmp/storenum.dat");
            if ((storechecknum-GetLastCheckNum())>100 || (GetLastCheckNum()-storechecknum)>100 || storechecknum>1000000000 || GetLastCheckNum()>1000000000)
            {
                char tmpstr[128];//="Ошибка нумерации чеков. Текущий чек: "+this->GetLastCheckNum()+". Отложенный чек: "+storechecknum+".\n Сообщите администратору!!!"
                sprintf(tmpstr,"Ошибка нумерации чеков. Текущий чек: %d. Отложенный чек: %d.\n Сообщите администратору!%d!%d!",this->GetLastCheckNum(),storechecknum,this->GetLastCheckNum()-storechecknum,storechecknum-this->GetLastCheckNum());
                ShowMessage((string)tmpstr);
            }

            if (GetLastCheckNum() < (storechecknum-1))
            {
                CurrentCheckNumber = GetLastCheckNum()+1;
                SetLastCheckNum(storechecknum-1);
            }
            else
                SetLastCheckNum(GetLastCheckNum()+1);
        }
        else
            SetLastCheckNum(GetLastCheckNum()+1);
        for (unsigned int i=0;i<CurCheque->GetCount();i++){
            if ((*CurCheque)[i].ItemFlag!=SL){
                LoggingAction(tmpNum+1,(*CurCheque)[i].ItemFlag,
                             (*CurCheque)[i].ItemSum,CurrentSaleman->Id,(*CurCheque)[i].GuardCode);
            }
        }

        DbfMutex->lock();
        int RecordsCount=CurrentCheckTable->RecordCount();
        DbfMutex->unlock();

        LoggingAction(tmpNum+1,COUNTER,RecordsCount,-1,-1);



        bool Flag_SL = false; // признак того, что в чеке есть продажи
        bool Action_PGSmile = false; // признак того, что в чеке есть товары, участвующие в акции P&G Смайл (выдача флаеров с прикрепленными чеками)

        string cur_lock="";

        for (unsigned int i=0;i<CurCheque->GetCount();i++)
        {
            if ((*CurCheque)[i].ItemFlag == SL)
            {
                Flag_SL = true;

                // Акция P&G Смайл выдача флаеров + чек (с накоплением чеков)
                cur_lock = (*CurCheque)[i].Lock;
                if (GoodsLock(cur_lock, GL_PGSmile) == GL_PGSmile) //14
                {
                    Action_PGSmile = true; //  Есть Акция
                }

            }
        }

//        //! Разовая Акция - выдача призовых билетов раз в час по пятницам
//   		if((CurCheque->GetCount() > 0) && Flag_SL )
//		{
//		    if (CurrentWorkPlace!=0)
//		    {
////              if (SQLGetCheckInfo->CheckAction(CurrentWorkPlace->CurrentCustomer, CurrentWorkPlace->Cheque->GetSum())){}
////! Акция закончилась
////!               SQLGetCheckInfo->CheckAction(CurrentWorkPlace->CurrentCustomer, CurrentWorkPlace->Cheque->GetSum());
//		    }
//		}


        if((CurCheque->GetCount() > 0) && Flag_SL && CheckMsgLevel )
        {

            int cnt = MsgLevelList.size();
            int curdate = xbDate().JulianDays();
            char buf[9]; buf[8]=0;
            string msg = "";
            string msg_max = "";
            string msg_barcode = "";
            double level=0;
            double level_barcode=0;
            double sum = CurCheque->GetSum();
            double sum_full = CurCheque->GetFullSum();
            for(int i = 0; i < cnt; i++)
            {
                if((MsgLevelList[i].level > 0)
                &&
                (
                    (sum >= MsgLevelList[i].level)
                    ||
                    (
                        (sum_full >= MsgLevelList[i].level) && (MsgLevelList[i].mode == 8)
                    )
                ))
                {
                    sprintf(buf,"%04d%02d%02d",MsgLevelList[i].startdate.tm_year+1900, MsgLevelList[i].startdate.tm_mon+1, MsgLevelList[i].startdate.tm_mday);
                    if(curdate < xbDate(buf).JulianDays()) continue;
                    sprintf(buf,"%04d%02d%02d",MsgLevelList[i].finishdate.tm_year+1900, MsgLevelList[i].finishdate.tm_mon+1, MsgLevelList[i].finishdate.tm_mday);
                    if(curdate > xbDate(buf).JulianDays()) continue;


                    if (MsgLevelList[i].mode == 9)
                    {

                        string strmsg = MsgLevelList[i].msg;
                        int res = (int)(sum/MsgLevelList[i].level);

                        strmsg += " ( "+Int2Str(res)+" шт. )";

                        if (CurrentWorkPlace!=0)
                        {
                            CurrentWorkPlace->MItemName->setText(W2U(strmsg));
                        }

                        ShowMessage(strmsg);
                    }


                    if (MsgLevelList[i].mode == 3)
                    {
                        ShowMessage(MsgLevelList[i].msg);
                    }

                    if (MsgLevelList[i].mode == 8)
                    {
                        ShowMessage(MsgLevelList[i].msg);
                    }

                    if ((MsgLevelList[i].mode == 14) && Action_PGSmile)
                    {
                        ShowMessage(MsgLevelList[i].msg);
                    }

                    if ((MsgLevelList[i].mode == 4)
                        && (level < MsgLevelList[i].level))
                    {
                        msg_max = MsgLevelList[i].msg;
                        level = MsgLevelList[i].level;
                    }

                    if ((MsgLevelList[i].mode == 5)
                        && (level_barcode < MsgLevelList[i].level))
                    {
                        msg_barcode = MsgLevelList[i].msg;
                        level_barcode = MsgLevelList[i].level;
                    }

                    if (MsgLevelList[i].mode == 33)
                    {
                        // + Фильтр по дисконтным картам
                        if(MsgLevelList[i].custtype)
                        {
                            if (CurrentWorkPlace==NULL) continue;

                            long long CurrCard = CurrentWorkPlace->CurrentCustomer;

                            if(!CurrCard) continue;

                            CardInfoStruct CardInfo;
                            if (FindCardInRangeTable(&CardInfo, CurrCard))
                            {
                                if ( MsgLevelList[i].custtype == CardInfo.CardType )
                                {
                                    msg_max=MsgLevelList[i].msg;// инфо в чеке
                                    break;
                                }
                                else
                                    continue;
                            };
                        }
                        // - Фильтр по дисконтным картам
                    }
                }
            }

            if (msg_max!="") ShowMessage(msg_max);

            if (msg_barcode!="")
            {
                if (CurrentWorkPlace!=0)
                {
                    while (!CurrentWorkPlace->EnterBarcode(Flayer, msg_barcode)) ;

                }
            }
        }

        // Установим статус пречека в "обработан" для исключения повторного использования
        if (CurCheque->NumPrecheck>0)
            SQLServer->SetPreCheckStatus(CurCheque->NumPrecheck, 3);

        ClearCurrentCheck();
        CurCheque->Clear();

        if(CheckMillenium)
        {
            SQLServer->DoAfterFixing();
            if(!CheckMillenium && CurrentWorkPlace->ActionInfo){
                CurrentWorkPlace->ActionInfo->setText(" ");
            }
        }

    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
}

int CashRegister::GetActsizType(GoodsInfo goods)
{
    int res = 0;
    if (goods.Actsiz.size()>0) {
        if (goods.ItemAlco) res = AT_ALCO;
        else if (goods.ItemCigar) res = AT_CIGAR;
            else if (goods.ItemMask) res = AT_MASK;
                else if (goods.ItemTextile) res = AT_TEXTILE;
                    else if (goods.ItemMilk) res = AT_MILK;
#ifdef ALCO_VES
                        else if(goods.ItemPivoRazliv) {res = AT_PIVO_RAZLIV;Log("res=AT_PIVO_RAZLIV");}
#endif
    }
    return res;
}

void CashRegister::WriteGoodsIntoTable(DbfTable* Table,GoodsInfo goods,string CurTime,bool append)
{
    Table->BlankRecord();
    Table->PutLongField("CHECKNUM",GetLastCheckNum()+1);
    Table->PutLongField("CODE",goods.ItemCode);
    Table->PutDoubleField("QUANT",goods.ItemCount);
    Table->PutDoubleField("PRICE",goods.ItemPrice);
    Table->PutDoubleField("FULLPRICE",goods.ItemFullPrice);
    Table->PutDoubleField("MINPRICE",goods.ItemMinimumPrice);
    Table->PutDoubleField("CALCPRICE",goods.ItemCalcPrice);
    Table->PutDoubleField("SUMMA",goods.ItemSum);
    Table->PutDoubleField("SUMMAPAY",goods.ItemSumToPay);
    Table->PutDoubleField("FULLSUMMA",goods.ItemFullSum);
    Table->PutDoubleField("FULLDSUMMA",goods.ItemFullDiscSum);
    Table->PutDoubleField("BANKBONUS",goods.ItemBankBonusDisc);

    Table->PutDoubleField("BONUSADD",goods.ItemBonusAdd);
    Table->PutDoubleField("BONUSDISC",goods.ItemBonusDisc);

    Table->PutDoubleField("BANKDSUMMA",goods.ItemBankDiscSum);
    Table->PutDoubleField("CALCSUMMA",goods.ItemCalcSum);
    Table->PutDoubleField("DISCOUNT",goods.ItemDiscount);
    Table->PutLongField("OPERATION",goods.ItemFlag);
    Table->PutLongField("PAYTYPE",goods.PaymentType);
    Table->PutLongField("CASHMAN",CurrentSaleman->Id);
    Table->PutLLongField("CUSTOMER",goods.CustomerCode[0]);
    Table->PutLLongField("ADDONCUST",goods.CustomerCode[1]);
    Table->PutLongField("INPUTKIND",goods.InputType);
    Table->PutLongField("RETCHECK",goods.RetCheck);
    Table->PutLongField("SECTION",GetSection(goods.Lock));
    Table->PutLongField("GUARD",goods.GuardCode);
//	if(goods.ItemCode == FreeSumCode)
    Table->PutField("NAME",goods.ItemName.c_str());

    Table->PutField("BARCODE",goods.ItemBarcode.c_str());
    Table->PutField("SBARCODE",goods.ItemSecondBarcode.c_str());

    Table->PutLongField("AVANS",goods.Avans);
    Table->PutLongField("CIGAR",goods.ItemCigar);
    Table->PutLongField("ALCO",goods.ItemAlco);

    Table->PutLongField("ACTSIZTYPE",GetActsizType(goods));

    Table->PutLongField("BONUSFLAG", BonusMode(goods.CustomerCode[0])?1:0);

#ifdef LOGNDS
    if (GetSection(goods.Lock) > 0 ) Table->PutDoubleField("NDS", 0);
    else Table->PutDoubleField("NDS", goods.NDS);
#endif

    if (goods.Actsiz.size()>0)
        Table->PutField("ACTSIZ",goods.Actsiz[goods.Actsiz.size()-1].c_str());
        //Table->PutField("ACTSIZ",goods.ItemActsizBarcode.c_str());


    if(KKMNo>0)
        Table->PutLongField("KKMNO",KKMNo);
    if(KKMSmena>0)
        Table->PutLongField("KKMSMENA",KKMSmena);
    if(KKMTime>0)
        Table->PutLongField("KKMTIME",KKMTime);

    if(goods.tTime)
    {
        tm* t = localtime(&goods.tTime);
        char date[16];
        char time[16];
        sprintf(date,"%04d%02d%02d",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
        sprintf(time,"%02d:%02d:%02d",t->tm_hour, t->tm_min, t->tm_sec);
        Table->PutField("DATE",date);//xbDate().GetDate().c_str());
        Table->PutField("TIME",time);//CurTime.c_str());
    }
    else
    {
        Table->PutField("DATE",xbDate().GetDate().c_str());
        Table->PutField("TIME",CurTime.c_str());
    }
    if(append)
        Table->AppendRecord();
    else
        Table->PutRecord();
}

void CashRegister::WriteChequeIntoTable(FiscalCheque* CurCheque,string CurTime)
{
    try
    {
        DbfMutex->lock();
        WriteChequeIntoTable(CurrentCheckTable,CurCheque,CurTime);
        DbfMutex->unlock();
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }
}

void CashRegister::WriteChequeIntoTable(DbfTable* Table,FiscalCheque* CurCheque,string CurTime)
{//store the given check to the dbf-table
    unsigned int i;
    try
    {
        for (i=0;i<CurCheque->GetCount();i++)
        {
            DbfMutex->lock();
            WriteGoodsIntoTable(Table,(*CurCheque)[i],CurTime);
            DbfMutex->unlock();
        }
    }
    catch(cash_error& ex)
    {
        GoodsInfo goods = (*CurCheque)[i];
        char s[4096];
        sprintf(s, "\nCHECKNUM = %d\n"
        "CODE = %d\n"
        "QUANT = %.3f\n"
        "PRICE = %.2f\n"
        "FULLPRICE = %.2f\n"
        "MINPRICE = %.2f\n"
        "CALCPRICE = %.2f\n"
        "SUMMA = %.2f\n"
        "FULLSUMMA = %.2f\n"
        "FULLDSUMMA = %.2f\n"
        "BANKDSUMMA = %.2f\n"
        "CALCSUMMA = %.2f\n"
        "DISCOUNT = %.2f\n"
        "OPERATION = %d\n"
        "PAYTYPE = %d\n"
        "CASHMAN = %d\n"
        "CUSTOMER = %lld\n"
        "INPUTKIND = %d\n"
        "KKMNO = %d\n"
        "KKMSMENA = %d\n"
        "KKMTIME = %d\n",
        GetLastCheckNum()+1,
        goods.ItemCode,
        goods.ItemCount,
        goods.ItemPrice,
        goods.ItemFullPrice,
        goods.ItemMinimumPrice,
        goods.ItemCalcPrice,
        goods.ItemSum,
        goods.ItemFullSum,
        goods.ItemFullDiscSum,
        goods.ItemBankDiscSum,
        goods.ItemCalcSum,
        goods.ItemDiscount,
        goods.ItemFlag,
        goods.PaymentType,
        CurrentSaleman->Id,
        goods.CustomerCode[0],
        goods.InputType,
        KKMNo,
        KKMSmena,
        KKMTime
        );
        Log(s);
        DbfMutex->unlock();
        LoggingError(ex);
    }
}

void CashRegister::WriteCustomersSales(FiscalCheque *CurCheque)
{
    //write discount's information into dbf-table
    double Sum=0,CalcSum=0;
    long long CustomerCode=0;
    long long AddonCustomer=0;
    for (unsigned int i=0;i<CurCheque->GetCount();i++)
    {
        if ((*CurCheque)[i].DiscFlag)
        {
            if (((*CurCheque)[i].ItemFlag==SL) && ((*CurCheque)[i].ItemCode!=TRUNC) && ((*CurCheque)[i].ItemCode!=EXCESS_PREPAY))
            {
                Sum+=(*CurCheque)[i].ItemSum;
                CalcSum+=(*CurCheque)[i].ItemCalcSum;
            }
            if (((*CurCheque)[i].ItemFlag==RT) && ((*CurCheque)[i].ItemCode!=TRUNC) && ((*CurCheque)[i].ItemCode!=EXCESS_PREPAY))
            {
                Sum-=(*CurCheque)[i].ItemSum;
                CalcSum-=(*CurCheque)[i].ItemCalcSum;
            }
            if(!CustomerCode)
                CustomerCode=(*CurCheque)[i].CustomerCode[0];
            if(!AddonCustomer)
                AddonCustomer=(*CurCheque)[i].CustomerCode[1];
        }
    }
    try
    {
        if ((CalcSum!=0)&&(CustomerCode!=0))
        {
            DbfMutex->lock();
            CustomersSalesTable->BlankRecord();
            CustomersSalesTable->PutLLongField("IDCust",CustomerCode);

            CustomersSalesTable->PutField("DATESALE",xbDate().GetDate().c_str());
            CustomersSalesTable->PutField("TIMESALE",GetCurTime().c_str());
            CustomersSalesTable->PutDoubleField("DISCSUMMA",Sum);
            CustomersSalesTable->PutDoubleField("SUMMA",CalcSum);

            CustomersSalesTable->PutLongField("CASHDESK",GetCashRegisterNum());
            CustomersSalesTable->PutLongField("CHECKNUM",GetLastCheckNum()+1);
            CustomersSalesTable->AppendRecord();
            DbfMutex->unlock();
        }

        if ((CalcSum!=0)&&(AddonCustomer!=0))
        {
            DbfMutex->lock();
            CustomersSalesTable->BlankRecord();
            CustomersSalesTable->PutLLongField("IDCust",AddonCustomer);

            CustomersSalesTable->PutField("DATESALE",xbDate().GetDate().c_str());
            CustomersSalesTable->PutField("TIMESALE",GetCurTime().c_str());
            CustomersSalesTable->PutDoubleField("DISCSUMMA",Sum);
            CustomersSalesTable->PutDoubleField("SUMMA",CalcSum);

            CustomersSalesTable->PutLongField("CASHDESK",GetCashRegisterNum());
            CustomersSalesTable->PutLongField("CHECKNUM",GetLastCheckNum()+1);
            CustomersSalesTable->AppendRecord();
            DbfMutex->unlock();
        }

    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
}

bool CashRegister::IsInload(void)
{
    return GoodsInLoadFlag;
}

void CashRegister::GetNewGoods(void)//load new goods table and create indices
{

//;;Log("[FUNC] GetNewGoods [START]");

    if (inload) {
//        ;;Log("[FUNC] GetNewGoods - INPROGRESS [END]");
        return; // кто первым встал - того и тапки
    }

    inload = true; // а вот, собственно, и сами тапки..

    GetNewCustomer(); // nick

    string NewFileName  = DBImportPath+QDir::separator()+NEWGOODS,
           FileName     = DBGoodsPath+QDir::separator()+GOODS,
           OldFileName  = DBGoodsPath+QDir::separator()+"old_"+GOODS;

    QFile
            *NewGoodsFile = new QFile((DBImportPath+QDir::separator()+NEWGOODS).c_str()),
            *GoodsFile = new QFile((DBGoodsPath+QDir::separator()+GOODS).c_str()),
            *FlagFile = new QFile((DBImportPath+QDir::separator()+FLAG).c_str()),
            *OldIndexFile = new QFile(),*NewIndexFile = new QFile();

#ifdef BUTTON_OLD_GOODS
    QFile FileOldGoods((DBGoodsPath+QDir::separator() + "old_goods.dbf").c_str());
    QFile FileGoods((DBGoodsPath+QDir::separator() + "goods.dbf").c_str());
#endif

//;;Log("[FUNC] GetNewGoods [LABEL 1]");

    if ((NewGoodsFile->exists())&&(FlagFile->exists()) )  //if new goods was found
    {
        ;;Log("[GetNewGoods] Загрузка товаров... [START]");

        GoodsInLoadFlag = true; // загрузка товаров началась

        //Log("");
        bool open_error  = false,
             index_error = false;

        if ( NewGoodsFile->size()<10000 )
        {
            Log("Неправильная база товаров!");
            NewGoodsFile->remove();
            FlagFile->remove();
        }
        else
        {
            thisApp->setOverrideCursor(waitCursor);
            try
            {
                InfoBar->setTotalSteps(IndexNo+2);
                InfoBar->setProgress(1);
                InfoMessage->setText(W2U("Загрузка товаров"));
                if (CurrentWorkPlace!=NULL)
                {
                    CurrentWorkPlace->InfoBar->setTotalSteps(IndexNo+2);

                    CurrentWorkPlace->InfoBar->setProgress(1);
                    CurrentWorkPlace->InfoMessage->setText(W2U("Загрузка товаров"));
                }

                thisApp->processEvents();

;;Log("  [GetNewGoods] Отключаем базу данных GOODS... ");
                DbfMutex->lock();
                GoodsTable->CloseIndices();
                GoodsTable->CloseDatabase();
                DbfMutex->unlock();

#ifdef BUTTON_OLD_GOODS

            Log("Размер файла");
            Log("goods.dbf=" + Int2Str(FileGoods.size()));
            Log("old_goods.dbf=" + Int2Str((FileOldGoods.size())-30720));
            Log("old_goods.dbf=" + Int2Str(FileOldGoods.size()));

            ///Если размер goods.dbf >= размера old_goods.dbf - 30кб , то заменяем old_goods.dbf, иначе нет

            if ( FileGoods.size() >= FileOldGoods.size()-30720 )

            {
#endif
                remove(OldFileName.c_str());
                rename(FileName.c_str(), OldFileName.c_str());

#ifdef BUTTON_OLD_GOODS
            }
            else
                remove(FileName.c_str());
#endif

                //try to move new goods to the standart place
                //if (MoveFile(NewGoodsFile,GoodsFile))
                if(!rename(NewFileName.c_str(), FileName.c_str()))
                {
;;Log("  [GetNewGoods] Перенесли файл товаров... ");
                    InfoBar->setProgress(2);
                    InfoMessage->setText(W2U("Загрузка списка товаров"));
                    if (CurrentWorkPlace!=NULL)
                    {
                        CurrentWorkPlace->InfoBar->setProgress(2);
                        CurrentWorkPlace->InfoMessage->setText(W2U("Загрузка списка товаров"));
                    }
                    thisApp->processEvents();

                    DbfMutex->lock();

                    GoodsTable->OpenDatabase(FileName);
;;Log("  [GetNewGoods] Подключили новую базу данных GOODS... ");
                    try
                    {
;;Log("  [GetNewGoods] Начало создания индексов... [START]");
                        for(int i=0;i<IndexNo;i++)//indexing .dbf with goods info
                        {
                            OldIndexFile->setName(
                                (DBImportPath+QDir::separator()+IndexFile[i]).c_str());
                            NewIndexFile->setName(
                                (DBGoodsPath+QDir::separator()+IndexFile[i]).c_str());

#ifdef BUTTON_OLD_GOODS
        system(("cp " + (DBGoodsPath+QDir::separator()+IndexFile[i]) + " " + (DBGoodsPath+QDir::separator()+"old_" + IndexFile[i])).c_str());
#endif

                            if ((OldIndexFile->exists())&&(MoveFile(OldIndexFile,NewIndexFile)))
                                GoodsTable->OpenIndex(
                                  DBGoodsPath+QDir::separator()+IndexFile[i],IndexField[i]);
                            else
                                GoodsTable->CreateIndex(
                                  (DBGoodsPath+QDir::separator()+IndexFile[i]).c_str(),IndexField[i]);

                            InfoBar->setProgress(i+3);
                            InfoMessage->setText(W2U("Загрузка индекса ")+IndexField[i]);
                            if (CurrentWorkPlace!=NULL)
                            {
                                CurrentWorkPlace->InfoBar->setProgress(i+3);
                                CurrentWorkPlace->InfoMessage->setText(W2U("Загрузка индекса ")+IndexField[i]);
                            }
                            thisApp->processEvents();
                        }
;;Log("  [GetNewGoods] Окончание создания индексов... [START]");
                    }
                    catch(cash_error& ex)
                    {
                        index_error = true;
                        ex.showmsg = false;
                        LoggingError(ex);
                        Log("ERROR: Ошибка загрузки файла GOODS.DBF /индекс/");
                    }

                    DbfMutex->unlock();

                    InfoBar->setProgress(0);
                    InfoMessage->setText(W2U(""));
                    if (CurrentWorkPlace!=NULL)
                    {
                        CurrentWorkPlace->InfoBar->setProgress(0);
                        CurrentWorkPlace->InfoMessage->setText(W2U(""));
                    }
//					FlagFile->remove();

                }

                else
                {
                    open_error = true;
                    ShowMessage("Не могу загрузить список товаров!");
                }

            }
            catch(cash_error& ex)
            {
                DbfMutex->unlock();
                open_error = true;
                ex.showmsg = false;

                LoggingError(ex);

                ;; Log("ERROR: Ошибка загрузки файла GOODS.DBF");
            }

            InfoBar->reset();
            InfoMessage->clear();
            if (CurrentWorkPlace!=NULL)
            {
                CurrentWorkPlace->InfoBar->reset();
                CurrentWorkPlace->InfoMessage->setText(W2U(""));
                //CurrentWorkPlace->InfoMessage->clear();
            }
            thisApp->restoreOverrideCursor();
        }

        // если загрузка завершилась с ошибкой - вернем прежнюю базу товаров
        if (open_error || index_error )
        {
;;Log("[GetNewGoods] Загрузка товаров... [ERROR]");
            try
            {
                DbfMutex->lock();
                if (index_error)
                {
                    GoodsTable->CloseIndices();
                    GoodsTable->CloseDatabase();
                }

                rename(OldFileName.c_str(), FileName.c_str());
                GoodsTable->OpenDatabase(FileName);

                // попытаемся создать индексы
                try
                {
                   for (int i=0;i<IndexNo;i++)
                   {
                      GoodsTable->OpenIndex(DBGoodsPath+QDir::separator()+IndexFile[i],IndexField[i]);
                   }
                }
                catch(cash_error& ex)
                {
                    Log("%2");

                    ex.showmsg = false;
                    LoggingError(ex);

                    Log("%3");
                    Log("Файл GOODS.DBF: Ошибка создания индекса!");
                }

                Log("%1");

                DbfMutex->unlock();

                Log("%4");
                Log("Файл GOODS.DBF восстановлен");
            }
            catch(cash_error& ex)
            {
               DbfMutex->unlock();

               LoggingError(ex);

               Log("ERROR: Ошибка восстановления файла GOODS.DBF!");
            }

        }

        GoodsInLoadFlag = false; // загрузка товаров завершилась

;;Log("[GetNewGoods] Загрузка товаров... [END]");
    }else{
        //;;Log("[FUNC] GetNewGoods - NO FILES");
    }

    thisApp->processEvents();
    FlagFile->remove();

    inload = false;

//	QFileInfo fileInf((DBGoodsPath+QDir::separator()+GOODS).c_str());
//	QDateTime lastMod = fileInf.lastModified();
//	char strdate1[17];strdate1[16]=0;char strload[70]="Товары:";
//	sprintf(strdate1,"%2d.%2d.%4d-%2d:%2d",lastMod.date().day(),lastMod.date().month(),lastMod.date().year(),
//		lastMod.time().hour(),lastMod.time().minute());
//	for(int c=0;c<17;c++) if (strdate1[c]==' ') strdate1[c]='0';
//	strcat(strload,strdate1);
//	QFileInfo fileInf2((DBGoodsPath+QDir::separator()+CUSTOMERS).c_str());
//	QDateTime lastMod2 = fileInf2.lastModified();
//	char strdate2[17];strdate2[16]=0; strcat(strload,"  Скидки:");
//	sprintf(strdate2,"%2d.%2d.%4d-%2d:%2d",lastMod2.date().day(),lastMod2.date().month(),lastMod2.date().year(),
//		lastMod2.time().hour(),lastMod2.time().minute());
//	for(int c=0;c<17;c++) if (strdate2[c]==' ') strdate2[c]='0';
//	strcat(strload,strdate2);

//	if (CurrentWorkPlace!=NULL)
//		CurrentWorkPlace->InfoMessage->setText(W2U(strload));


    delete NewGoodsFile;
    delete GoodsFile;
    delete FlagFile;
    delete OldIndexFile;
    delete NewIndexFile;

#ifdef BUTTON_OLD_GOODS
    FileGoods.close();
    FileOldGoods.close();
#endif

//    ;;Log("[FUNC] GetNewGoods [END]");
}

void CashRegister::LoadGoodsClick(void)//load new goods
{
    GetNewGoods();
}

void CashRegister::LoadStaffClick(void)//load staff's information
{
    int TmpPeriod=SQLServer->GetWaitPeriod(); //if connection was lost try to establish it
    SQLServer->SetWaitPeriod(0);				//immediately
    thisApp->setOverrideCursor(waitCursor);
    SQLServer->GetStaffList();
    thisApp->restoreOverrideCursor();
    SQLServer->SetWaitPeriod(TmpPeriod);
}

void CashRegister::LoadDiscountClick(void)//load  discount's information
{
    int TmpPeriod=SQLServer->GetWaitPeriod();//if connection was lost try to establish it
    SQLServer->SetWaitPeriod(0);				//immediately
    thisApp->setOverrideCursor(waitCursor);
    SQLServer->GetDiscountInfo();
    SQLServer->GetCustomers();
    thisApp->restoreOverrideCursor();

    SQLServer->SetWaitPeriod(TmpPeriod);
}

void CashRegister::LoadKeysClick(void)//load keyboard's information
{
    int TmpPeriod=SQLServer->GetWaitPeriod();//if connection was lost try to establish it
    SQLServer->SetWaitPeriod(0);				//immediately
    thisApp->setOverrideCursor(waitCursor);
    SQLServer->GetKeysInfo();
    thisApp->restoreOverrideCursor();
    SQLServer->SetWaitPeriod(TmpPeriod);
}

void CashRegister::LoadBarcodesClick(void)//load barcode's information
{
    int TmpPeriod=SQLServer->GetWaitPeriod();//if connection was lost try to establish it
    SQLServer->SetWaitPeriod(0);				//immediately
    thisApp->setOverrideCursor(waitCursor);
    SQLServer->GetBarcodeInfo();
    thisApp->restoreOverrideCursor();
    SQLServer->SetWaitPeriod(TmpPeriod);
}

void CashRegister::LoadGroupsClick(void)//load information on goods groups
{
    int TmpPeriod=SQLServer->GetWaitPeriod();//if connection was lost try to establish it
    SQLServer->SetWaitPeriod(0);				//immediately
    thisApp->setOverrideCursor(waitCursor);
    SQLServer->GetGroupsInfo();
    thisApp->restoreOverrideCursor();
    SQLServer->SetWaitPeriod(TmpPeriod);
}

void CashRegister::LastSmenaClick(void)//change the store's number
{
    NumberChange("Номер смены","LAST_SMENA");
}

void CashRegister::CheckNameClick(void)//наименование документа в чеке (по умолчанию "Чек")
{
    CheckNameChange("Наименование документа в чеке","CHECKNAME");
    // + dms
    FiscalReg->CheckName = GetCheckName();
    // - dms
}

void CashRegister::ShopNumClick(void)//change the store's number
{
    NumberChange("Номер магазина","DEPARTMENT");
}

void CashRegister::CashDeskNumClick(void)//change the cashdesk's number
{
    NumberChange("Номер кассы","CASH_DESK");
}

void CashRegister::LastCheckClick(void)//change last check's number
{
    NumberChange("Последний чек","LAST_CHECK");
}

void CashRegister::MaxCountClick(void)//change maximum goods quantity in one sale
{
    NumberChange("Макс. количество","MAXAMOUNT");
}

void CashRegister::MaxSumClick(void)//change maximum goods sum in one sale
{
    NumberChange("Макс. сумма","MAXSUM");
}

void CashRegister::MaxSpaceClick(void)//change maximum free space provided for archieves
{
    NumberChange("Макс.размер архива(Мб)\nили срок хран.файлов(-1*мес)","MAXSPACE");
}

void CashRegister::MainPathClick(void)//change the main path of the program
{
    MainPathChange();
}

void CashRegister::ImportPathClick(void)//change the path to the new goods
{
    ImportPathChange();
}

void CashRegister::ArchPathClick(void)//change the path to the new goods
{
    ArchPathChange();
}

void CashRegister::PicPathClick(void)
{
    if (PathChange("Каталог картинок","PIC_PATH"))
    {
        DbfMutex->lock();
        DBPicturePath=ConfigTable->GetStringField("PIC_PATH");

        DbfMutex->unlock();
    }
}

void CashRegister::DatabasePathClick(void)//change path to SQL database
{
    PathChange("Путь к базе данных","SERVER");
    DbfMutex->lock();
    SQLServer->SetDBPath(ConfigTable->GetStringField("SERVER"));
    DbfMutex->unlock();
}

void CashRegister::LoginCheckClick(void)//set server's login/passwd and try to establish connection
{
    int TmpPeriod=SQLServer->GetWaitPeriod();
    SQLServer->SetWaitPeriod(0);
    LoginForm *dlg = new LoginForm(this);
    dlg->Label3->setText(dlg->Label3->text()+SQLServer->GetDBPath().c_str());
    dlg->LoginEdit->setText(SQLServer->GetDBLogin().c_str());

    if (SQLServer->GetDBLogin().empty())
        dlg->LoginEdit->setFocus();
    else
        dlg->PasswdEdit->setFocus();

    if (dlg->exec()==QDialog::Accepted)
    {
        try
        {
            DbfMutex->lock();
            ConfigTable->GetFirstRecord();
            ConfigTable->PutField("S_LOGIN",dlg->LoginEdit->text().latin1());
            ConfigTable->PutField("S_PASSWD",dlg->PasswdEdit->text().latin1());
            ConfigTable->PutRecord();
            DbfMutex->unlock();
        }
        catch(cash_error& er)
        {
            DbfMutex->unlock();
            LoggingError(er);
        }
        delete SQLServer;

        SQLServer = new SQLThread(this);
        SQLServer->InitInstance();
        if(!SQLServer->running())
            SQLServer->start();
    }
    SQLServer->SetWaitPeriod(TmpPeriod);
    delete dlg;
}

void CashRegister::WaitPeriodClick(void)//set timeout between attempts to connect to SQL database
{
    NumberChange("Период соединения","IB_WAIT");
    DbfMutex->lock();
    SQLServer->SetWaitPeriod(ConfigTable->GetLongField("IB_WAIT"));
    DbfMutex->unlock();
}

bool CashRegister::ComPortChange(string Capt,string PortName,string PortSpeed)//change parameters
{																			//of the serial port
    PortParamForm *dlg = new PortParamForm(this);
    dlg->FormCaption->setText(W2U(Capt));

    DbfMutex->lock();
    ConfigTable->GetFirstRecord();

    dlg->PortChoose->setCurrentItem(ConfigTable->GetLongField(PortName.c_str()));

    switch (ConfigTable->GetLongField(PortSpeed.c_str()))
    {
        case 1200:dlg->SpeedChoose->setCurrentItem(1);break;
        case 2400:dlg->SpeedChoose->setCurrentItem(2);break;
        case 4800:dlg->SpeedChoose->setCurrentItem(3);break;
        case 9600:dlg->SpeedChoose->setCurrentItem(4);break;
        case 19200:dlg->SpeedChoose->setCurrentItem(5);break;
        case 38400:dlg->SpeedChoose->setCurrentItem(6);break;
        case 57600:dlg->SpeedChoose->setCurrentItem(7);break;
        case 115200:dlg->SpeedChoose->setCurrentItem(8);break;
        default:dlg->SpeedChoose->setCurrentItem(0);break;
    }
    DbfMutex->unlock();

    int res=dlg->exec();

    int port=dlg->PortChoose->currentItem();
    int baud=dlg->SpeedChoose->currentItem();

    delete dlg;

    if (res==QDialog::Accepted)
    {
        try
        {
            DbfMutex->lock();
            ConfigTable->PutLongField(PortName.c_str(),port);
            switch (baud)
            {
                case 0:ConfigTable->PutLongField(PortSpeed.c_str(),0);break;
                case 1:ConfigTable->PutLongField(PortSpeed.c_str(),1200);break;
                case 2:ConfigTable->PutLongField(PortSpeed.c_str(),2400);break;
                case 3:ConfigTable->PutLongField(PortSpeed.c_str(),4800);break;
                case 4:ConfigTable->PutLongField(PortSpeed.c_str(),9600);break;
                case 5:ConfigTable->PutLongField(PortSpeed.c_str(),19200);break;
                case 6:ConfigTable->PutLongField(PortSpeed.c_str(),38400);break;
                case 7:ConfigTable->PutLongField(PortSpeed.c_str(),57600);break;
                case 8:ConfigTable->PutLongField(PortSpeed.c_str(),115200);break;
                default:break;
            }

            ConfigTable->PutRecord();
            DbfMutex->unlock();
            return true;
        }
        catch(cash_error& er)
        {
            DbfMutex->unlock();
            LoggingError(er);
        }
    }

    return false;
}

void CashRegister::ScanPortClick(void)//change parameters of the scanner's serial port
{
    ComPortChange("Сканер","SCANPORT","SCANBAUD");
}

void CashRegister::FiscalRegClick(int type)
{
/*	switch (FiscalRegType)
    {
        case EmptyRegisterType:delete (EmptyFiscalReg*)FiscalReg;break;
        case ElvesMiniType:case ShtrichVer2Type:case ElvesMiniEntireType:case ShtrichVer2EntireType:delete (ElvisRegVer2*)FiscalReg;break;
        case ShtrichVer3Type:case ShtrichVer3EntireType:case ShtrichVer3EntireType_T:delete (ElvisRegVer3*)FiscalReg;break;
        case ShtrichVer3EntireType_T:delete (ElvisRegVer3*)FiscalReg;break;
    }
*/
    delete FiscalReg;

    FiscalRegType=type;

    if (FiscalRegType == ShtrichCityType) {
        setLineWidthDefault(32);
    } else {
        setLineWidthDefault(36);
    }

    switch (FiscalRegType)
    {
        case EmptyRegisterType:FiscalReg=(AbstractFiscalReg*)new EmptyFiscalReg;break;
        case ElvesMiniType:case ShtrichVer2Type:case ElvesMiniEntireType:case ShtrichVer2EntireType:
            FiscalReg=(AbstractFiscalReg*)new ElvisRegVer2;break;
        case ShtrichVer3Type:case ShtrichVer3EntireType:case ShtrichCityType:
            FiscalReg=(AbstractFiscalReg*)new ElvisRegVer3;break;
        case ShtrichVer3PrinterType:case ShtrichVer3EntirePrinterType:
            FiscalReg=(AbstractFiscalReg*)new ElvisRegVer3Printer;break;
//
//		case ShtrichVer3EntireType_T:
//			FiscalReg=(AbstractFiscalReg*)new ElvisRegVer3_T;break;
    }

    try
    {
        DbfMutex->lock();
        ConfigTable->PutLongField("REGTYPE",type);
        ConfigTable->PutRecord();
        ShowErrorCode(FiscalReg->InitReg(ConfigTable->GetLongField("REGPORT"),
            ConfigTable->GetLongField("REGBAUD"),type,ConfigTable->GetStringField("REGPASSWD").c_str()));
        DbfMutex->unlock();
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }
    // + dms
    FiscalReg->CheckName = GetCheckName();
    // - dms
}

void CashRegister::EmptyRegClick(void)//set the current fiscal register as Empty
{
    FiscalRegClick(EmptyRegisterType);
}

void CashRegister::ElvesMiniClick(void)//set the current fiscal register as Elves-Mini-FR-F
{
    FiscalRegClick(ElvesMiniType);
}

void CashRegister::ShtrichVer2Click(void)//set the current fiscal register as Shtrich-FR-F ver. 2
{
    FiscalRegClick(ShtrichVer2Type);
}

void CashRegister::ShtrichVer3Click(void)//set the current fiscal register as Shtrich-FR-F ver. 3
{
    FiscalRegClick(ShtrichVer3Type);
}

void CashRegister::ElvesMiniEntireClick(void)//set the current fiscal register as Elves-Mini-FR-F
{
    FiscalRegClick(ElvesMiniEntireType);
}

void CashRegister::ShtrichVer2EntireClick(void)//set the current fiscal register as Shtrich-FR-F ver. 2
{
    FiscalRegClick(ShtrichVer2EntireType);
}

void CashRegister::ShtrichCityClick(void)//set the current fiscal register as Shtrich-FR-F ver. 2
{
    FiscalRegClick(ShtrichCityType);
}

void CashRegister::ShtrichVer3EntireClick(void)//set the current fiscal register as Shtrich-FR-F ver. 3
{
    FiscalRegClick(ShtrichVer3EntireType);
}

void CashRegister::ShtrichVer3EntirePrinterClick(void)//set the current fiscal register as Shtrich-FR-F ver. 3
{
    FiscalRegClick(ShtrichVer3EntirePrinterType);
}

void CashRegister::ShtrichVer3EntireClick_T(void)//set the current fiscal register as Shtrich-FR-F ver. 3
{
    FiscalRegClick(ShtrichVer3EntireType);
}

void CashRegister::RegPortClick(void)//change parameters of the fiscal register's serial port
{
    long oldbaud = ConfigTable->GetLongField("REGBAUD");
    if (ComPortChange("Фискальный регистратор","REGPORT","REGBAUD"))
    {

        DbfMutex->lock();
    //	int er=FiscalReg->InitReg(ConfigTable->GetLongField("REGPORT"),
        int er=FiscalReg->ChangeRegBaud(ConfigTable->GetLongField("REGPORT"), oldbaud,
                                    ConfigTable->GetLongField("REGBAUD"),FiscalRegType,
                                        ConfigTable->GetStringField("REGPASSWD").c_str());
        DbfMutex->unlock();
        ShowErrorCode(er);
    }
}

void CashRegister::RegPasswdClick(void)//change the fiscal register's password
{
    if (PathChange("Пароль регистратора","REGPASSWD"))
    {
        DbfMutex->lock();
        int er=FiscalReg->InitReg(ConfigTable->GetLongField("REGPORT"),
                                    ConfigTable->GetLongField("REGBAUD"),FiscalRegType,
                                        ConfigTable->GetStringField("REGPASSWD").c_str());
        DbfMutex->unlock();
        ShowErrorCode(er);
    }
}


void CashRegister::RegTablesClick(void)
{
    RegTableForm *dlg = new RegTableForm(this);

    QIntValidator *TableValid = new QIntValidator(dlg->TableEdit);//is entered
    TableValid->setRange(1,256);
    dlg->TableEdit->setValidator(TableValid);

    QIntValidator *RowValid = new QIntValidator(dlg->RowEdit);//is entered
    RowValid->setRange(1,256);
    dlg->RowEdit->setValidator(RowValid);

    QIntValidator *FieldValid = new QIntValidator(dlg->FieldEdit);//is entered
    FieldValid->setRange(1,256);
    dlg->FieldEdit->setValidator(FieldValid);

    dlg->exec();

    delete FieldValid;
    delete RowValid;
    delete TableValid;
    delete dlg;
}

void CashRegister::SetFullZReportClick(void)//установка вывода подробного Z-отчета в настройках ФР
{
    ShowErrorCode(FiscalReg->SetFullZReport());
}

void CashRegister::SetCurrentDateClick(void)//set current date in fiscal register
{											//it works only if shift is closed
    time_t cur_time=time(NULL);
    tm *l_time=localtime(&cur_time);
    ShowErrorCode(FiscalReg->SetDate(l_time->tm_mday,l_time->tm_mon+1,l_time->tm_year-100));
}

void CashRegister::SetCurrentTimeClick(void)//set current time in fiscal register
{											//it works only if shift is closed
    time_t cur_time=time(NULL);
    tm *l_time=localtime(&cur_time);
    ShowErrorCode(FiscalReg->SetTime(l_time->tm_hour,l_time->tm_min,l_time->tm_sec));
}

void CashRegister::EquipmentTypeChange(string field,unsigned int &var,unsigned int value)
{
    try
    {
        DbfMutex->lock();
        var=value;
        ConfigTable->PutLongField(field.c_str(),value);
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }
}

void CashRegister::EmptyScaleClick(void)
{
    EquipmentTypeChange("SCALETYPE",ScaleType,EmptyScaleType);
}


void CashRegister::MassaKScaleClick(void)
{
    EquipmentTypeChange("SCALETYPE",ScaleType,MassaKScaleType);
}

void CashRegister::MassaKPClick(void)
{
    EquipmentTypeChange("SCALETYPE",ScaleType,MassaKPType);
}

void CashRegister::ScalePortClick(void)//change parameters of the scale's serial port
{
    ComPortChange("Весы","SCALEPORT","SCALEBAUD");
}

void CashRegister::ScaleWaitClick(void)//change the timeout to wait for response from scales (in sec)
{
    NumberChange("Время отклика (в сек)","SCALE_WAIT");
}

void CashRegister::EmptyDisplayClick(void)
{
    EquipmentTypeChange("DISPLTYPE",DisplayType,EmptyDisplayType);
}

void CashRegister::DatecsDisplayClick(void)
{
    EquipmentTypeChange("DISPLTYPE",DisplayType,DatecsDisplayType);
}

void CashRegister::VSComDisplayClick(void)
{
    EquipmentTypeChange("DISPLTYPE",DisplayType,VSComDisplayType);
}

void CashRegister::EmptyCardReaderClick(void)
{
    EquipmentTypeChange("CARDTYPE",CardReaderType,EmptyCardReaderType);
}

void CashRegister::CityCardReaderClick(void)
{
    EquipmentTypeChange("CARDTYPE",CardReaderType,(CardReaderType ^ CCCR));
}

void CashRegister::EGateCardReaderClick(void)
{
    EquipmentTypeChange("CARDTYPE",CardReaderType,(CardReaderType ^ EGCR));
}

void CashRegister::VTBCardReaderClick(void)
{
    EquipmentTypeChange("CARDTYPE",CardReaderType,(CardReaderType ^ VTBCR));
}


void CashRegister::VeriFoneCardReaderClick(void)
{
    EquipmentTypeChange("CARDTYPE",CardReaderType,(CardReaderType ^ VFCR));
}

void CashRegister::VeriFoneUPOSCardReaderClick(void)
{
    EquipmentTypeChange("CARDTYPE",CardReaderType,(CardReaderType ^ VFUP));
    delete pCR;
    pCR = CardReaderType & VFUP ? new(CRSBupos) : new(CardReader);
}

void CashRegister::DisplayPortClick(void)//change parameters of the customer display's serial port
{
    ComPortChange("Дисплей покупателя","DISPLPORT","DISPLBAUD");
}

void CashRegister::CardReaderPortClick(void)

{
    ComPortChange("Считыватель пластиковых карт","CARDPORT","CARDBAUD");
}

void CashRegister::ProcessRegError(int er)//standart handler of fiscal register's errors
{
    if (er!=0)
        throw er;
    else
        ShowErrorCode(er);
}

void CashRegister::Res640x400ColorClick(void)//change screen resolution's parameter
{
    EquipmentTypeChange("RESOLUTION",ResolutionType,res640x400Color);
    repaint();
}

void CashRegister::Res640x480ColorClick(void)
{
    EquipmentTypeChange("RESOLUTION",ResolutionType,res640x480Color);
    repaint();
}

void CashRegister::Res800x600ColorClick(void)
{
    EquipmentTypeChange("RESOLUTION",ResolutionType,res800x600Color);
    repaint();
}

void CashRegister::Res1024x768ColorClick(void)
{
    EquipmentTypeChange("RESOLUTION",ResolutionType,res1024x768Color);
    repaint();
}

void CashRegister::Res640x400BWClick(void)
{
    EquipmentTypeChange("RESOLUTION",ResolutionType,res640x400BW);
    repaint();
}

void CashRegister::Res640x480BWClick(void)
{
    EquipmentTypeChange("RESOLUTION",ResolutionType,res640x480BW);
    repaint();
}

void CashRegister::Res800x600BWClick(void)
{
    EquipmentTypeChange("RESOLUTION",ResolutionType,res800x600BW);
    repaint();
}

void CashRegister::Res1024x768BWClick(void)
{
    EquipmentTypeChange("RESOLUTION",ResolutionType,res1024x768BW);
    repaint();
}

void CashRegister::timerDone(void)//show current time
{
//	setCaption("Вас обслуживает"+W2U(CurrentSaleman->Name)+" ("+(GetCurDate()+" "+GetCurTime()).c_str()+")");
    setCaption(W2U("APM KACCA rev."+Int2Str(revision)));
//	if (CurrentWorkPlace==NULL)
//		AboutLabel->setProperty( "text", W2U(GetCurDate()+"\n"+GetCurTime()));

}

int CashRegister::PrintDiscountInfo(FiscalCheque* CurCheque)//print discount's information
{
    double FullSum=CurCheque->GetCalcSum();
    if (fabs(FullSum)<epsilon)
        return 0;

    if (FiscalReg->LineLength>30)
        return FiscalReg->CreatePriceStr("СУММА БЕЗ СКИДКИ",FullSum);
  else
    return FiscalReg->CreatePriceStr("ВСЕГО",FullSum);
}

void CashRegister::ProcessCurCheck(void)//if program starts and finds not empty current check
{										//then it try to annul it
    try
    {
        string date;
        char cdate[18];
        FILE *uce;

        DbfMutex->lock();
        int RecordsCount=CurrentCheckTable->RecordCount();
        DbfMutex->unlock();

        if (RecordsCount>0)
        {

            uce = fopen("./tmp/uncorrectend","w");
            date = (CurrentCheckTable->GetField("DATE") + "/" +CurrentCheckTable->GetField("TIME"));
            date[0] = date[6]; date[1] = date[7];
            date[6] = date[2]; date[7] = date[3];
            date[3] = date[4]; date[4] = date[5];
            date[2] = date[5] = '.';
            fprintf(uce, date.c_str());
            fclose(uce);

        }
        TryToCancelCheck();
        uce = fopen("./tmp/uncorrectend","r");
        if (uce){
            fscanf(uce, "%s", cdate);
            fclose(uce);

            double summ=0.0;
            DbfMutex->lock();
            for (CurrentCheckTable->GetFirstRecord(),summ+=CurrentCheckTable->GetDoubleField("SUMMA");
                CurrentCheckTable->GetRecNo() < CurrentCheckTable->RecordCount();
                CurrentCheckTable->GetNextRecord(),	summ+=CurrentCheckTable->GetDoubleField("SUMMA"));
            DbfMutex->unlock();
            LoggingAction(CurrentCheckTable->GetLongField("CHECKNUM"),
                    UNCORRECTEND,
                    summ,
                    CurrentCheckTable->GetLongField("CASHMAN"),-1);
            date = cdate;
            ShowMessage("Касса была выключена с открытым "+date+" чеком!!!");
        }
        uce = fopen("./tmp/keepstate.dat","r");
        if (uce){
            int keepchecknum;
            char buf[256];
            while(!feof(uce))
            {
                fscanf(uce,"%s",buf);
                sscanf(buf,"KCHNUM=%d", &keepchecknum);
                sscanf(buf,"KCHTIME=%s", cdate);
            }
            date = cdate;
            fclose(uce);
            KeepCheckInfo->setText(W2U("O.Ч."));
            ShowMessage("Касса была выключена с отложенным "+date+" чеком #"+Int2Str(keepchecknum)+"!!!");
        }

    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
}

void CashRegister::DateShiftRangeClick(void)
{
    DateShiftRange range;
    int er=FiscalReg->GetDateShiftRange(TaxOfficerPassword.c_str(),range);
    if (er!=0)
        ShowErrorCode(er);
    else
    {
        DateShiftRangeForm *dlg = new DateShiftRangeForm(this);
        dlg->FirstShiftEdit->setText(QString().setNum(range.FirstShift));
        dlg->LastShiftEdit->setText(QString().setNum(range.LastShift));

        char buffer[64];

        sprintf(buffer,"%02d.%02d.%04d",range.FirstDate.tm_mday,range.FirstDate.tm_mon,range.FirstDate.tm_year+2000);
        dlg->FirstDateEdit->setText(buffer);

        sprintf(buffer,"%02d.%02d.%04d",range.LastDate.tm_mday,range.LastDate.tm_mon,range.LastDate.tm_year+2000);
        dlg->LastDateEdit->setText(buffer);

        dlg->exec();
        delete dlg;
    }
}

void CashRegister::ShiftRangeReport(bool type)
{
    DateShiftRange range;
    ShiftRangeForm *dlg = new ShiftRangeForm(this);

    QIntValidator *FirstShiftValid = new QIntValidator(dlg->FirstShiftEdit);
    FirstShiftValid->setRange(1,1000000);
    dlg->FirstShiftEdit->setValidator(FirstShiftValid);
    QIntValidator *LastShiftValid = new QIntValidator(dlg->LastShiftEdit);
    LastShiftValid->setRange(1,1000000);
    dlg->LastShiftEdit->setValidator(LastShiftValid);

    if (dlg->exec()==QDialog::Accepted)
    {
        range.FirstShift=dlg->FirstShiftEdit->text().toInt();

        range.LastShift=dlg->LastShiftEdit->text().toInt();
        ShowErrorCode(FiscalReg->ShiftRangeReport(TaxOfficerPassword.c_str(),range,type));
    }

    delete FirstShiftValid;
    delete LastShiftValid;
    delete dlg;
}

void CashRegister::DateRangeReport(bool type)
{
    DateShiftRange range;
    DateRangeForm *dlg = new DateRangeForm(this);

    QIntValidator *FirstDateDayValid = new QIntValidator(dlg->FirstDateDayEdit);
    FirstDateDayValid->setRange(1,31);
    dlg->FirstDateDayEdit->setValidator(FirstDateDayValid);

    QIntValidator *LastDateDayValid = new QIntValidator(dlg->LastDateDayEdit);

    LastDateDayValid->setRange(1,31);
    dlg->LastDateDayEdit->setValidator(LastDateDayValid);

    QIntValidator *FirstDateMonthValid = new QIntValidator(dlg->FirstDateMonthEdit);
    FirstDateMonthValid->setRange(1,12);
    dlg->FirstDateMonthEdit->setValidator(FirstDateMonthValid);

    QIntValidator *LastDateMonthValid = new QIntValidator(dlg->LastDateMonthEdit);
    LastDateMonthValid->setRange(1,12);

    dlg->LastDateMonthEdit->setValidator(LastDateMonthValid);

    QIntValidator *FirstDateYearValid = new QIntValidator(dlg->FirstDateYearEdit);
    FirstDateYearValid->setRange(2000,3000);
    dlg->FirstDateYearEdit->setValidator(FirstDateYearValid);

    QIntValidator *LastDateYearValid = new QIntValidator(dlg->LastDateYearEdit);
    LastDateYearValid->setRange(2000,3000);
    dlg->LastDateYearEdit->setValidator(LastDateYearValid);

    if (dlg->exec()==QDialog::Accepted)
    {
        range.FirstDate.tm_mday=dlg->FirstDateDayEdit->text().toInt();
        range.FirstDate.tm_mon=dlg->FirstDateMonthEdit->text().toInt();
        range.FirstDate.tm_year=dlg->FirstDateYearEdit->text().toInt()-2000;

        range.LastDate.tm_mday=dlg->LastDateDayEdit->text().toInt();
        range.LastDate.tm_mon=dlg->LastDateMonthEdit->text().toInt();
        range.LastDate.tm_year=dlg->LastDateYearEdit->text().toInt()-2000;

        ShowErrorCode(FiscalReg->DateRangeReport(TaxOfficerPassword.c_str(),range,type));
    }

    delete FirstDateDayValid;
    delete FirstDateMonthValid;
    delete FirstDateYearValid;
    delete LastDateDayValid;
    delete LastDateMonthValid;
    delete LastDateYearValid;
    delete dlg;
}

void CashRegister::ShortDateReportClick(void)

{
    DateRangeReport(false);
}

void CashRegister::LongDateReportClick(void)
{
    DateRangeReport(true);
}

void CashRegister::ShortShiftReportClick(void)
{
    ShiftRangeReport(false);
}

void CashRegister::LongShiftReportClick(void)
{
    ShiftRangeReport(true);
}

#include <qfocusdata.h>
//QWidget* QFocusData::focusWidget()

void CashRegister::customEvent(QCustomEvent *ev)
{
    //;;Log("[Func] customEvent() [Start]");
    switch (ev->type())
    {
        case SQLEventType:LoggingError(*(cash_error*)ev->data());break;
        case MessageEventType:
            {
                string MessageText = *(string*)ev->data();
                LogMessage(">>", MessageText);
                if (CurrentWorkPlace!=NULL)
                {
                    CurrentWorkPlace->RunMessager(MessageText);
                }
                else
                {
                     ShowMessage(MessageText);
                }
            }
            break;
        case UpEventType:SQLServer->SetDetachStatus(false);break;
        case DownEventType:SQLServer->SetDetachStatus(true);break;
        case BarcodeEventType://find goods by barcode
           Log("BarcodeEventType " + *(string*)ev->data()+" BARCODEEVENTTYPEEND") ;
//           ;;Log(*(string*)ev->data());
           if (ECD!=NULL) {
			   Log("ECD-EMITBARCODE [START]");
               ECD->emitBarcode(W2U(*(string*)ev->data()));
           } else if (ActsizCard!=NULL) {
			   Log("ActsizCard-EMITBARCODE [START]");
               ActsizCard->BarcodeEdit->emitBarcode(W2U(*(string*)ev->data()));
           } else if (CurrentWorkPlace!=NULL) {
               if (CurrentWorkPlace->BarcodeCard!=NULL) {
				   Log("CURRENWORKPLACE-EMITBARCODE [START]");
                   CurrentWorkPlace->BarcodeCard->EditLine->emitBarcode(W2U(*(string*)ev->data()));
               } else {
                   {
                       char tmpstr[1024];
                       sprintf(tmpstr," custom event \"%s\"\n",(*(string*)ev->data()).c_str());
                       Log(tmpstr);
                   }
                   if (CurrentWorkPlace->PriceChecker!=NULL/*bPriceCheck*/)
                   {
					   Log("PRICECHECKER-EMITBARCODE [START]");
                       CurrentWorkPlace->PriceChecker->SearchEdit->emitBarcode(W2U(*(string*)ev->data()));
                       CurrentWorkPlace->PriceChecker->SearchEdit->emitKey(41480);
                   } else if (CurrentWorkPlace->FreeSum!=NULL) {
					   Log("FreeSume-EMITBARCODE [START]");
                       CurrentWorkPlace->FreeSum->BarcodeEdit->emitBarcode(W2U(*(string*)ev->data()));
                       //CurrentWorkPlace->FreeSum->BarcodeEdit->emitKey(41480);
                   }
                   //else
                   {
					   Log("EDITLINE-EMITBARCODE [START]");
                       CurrentWorkPlace->EditLine->emitKey(0);
                       CurrentWorkPlace->EditLine->emitBarcode(W2U(*(string*)ev->data()));
                   }
               }
           } else if(frmPasswd != NULL) {
               string tmpstr,name,passwd;
               size_t found;

               tmpstr = *(string*)ev->data();
               found  = tmpstr.find_first_of("$");

               if (found==string::npos) return;

               name   = tmpstr.substr(0,found);
               passwd = tmpstr.substr(found+1);

               frmPasswd->UserName->setText(W2U(name));
               frmPasswd->PasswdLine->setText(W2U(passwd));
               frmPasswd->okclick();

               tmpstr = "";
               tmpstr += "Password from scaner name = \"";
               tmpstr += name;
               tmpstr += "\" password = \"";
               tmpstr += passwd;
               tmpstr += "\"";

               Log((char*) tmpstr.c_str());

           } else {
               // тест сканера
               string s = *(string*)ev->data();
               ShowMessage(s);
           }
        break;
    default: break;
    }
//Завершение добавление товара в список, на фр еще не отправляем
    //;;Log("[Func] customEvent() [End]");
}

void CashRegister::SetSQLStatus(bool status)
{
    StatusMutex->lock();

    QFont tmpFont=ServerInfo->font();
    tmpFont.setStrikeOut(!status);
    ServerInfo->setFont(tmpFont);
    ServerInfo->repaint();
    if (CurrentWorkPlace!=0)
    {
        CurrentWorkPlace->ServerInfo->setFont(tmpFont);
        CurrentWorkPlace->ServerInfo->repaint();
    }

    StatusMutex->unlock();
}



void CashRegister::InitDirs(void)
{
    DBGoodsPath=DBMainPath+QDir::separator()+"data";//create directories
    DBSalesPath=DBMainPath+QDir::separator()+"sales";
    DBTmpPath=DBMainPath+QDir::separator()+"tmp";
    QDir().mkdir(DBGoodsPath.c_str());         //create directories
    QDir().mkdir(DBTmpPath.c_str());
    QDir().mkdir(DBSalesPath.c_str());
    QDir().mkdir(DBImportPath.c_str());

    string DBArchPath_Def=DBMainPath+QDir::separator()+"arch";
    QDir().mkdir(DBArchPath_Def.c_str());

    QDir().mkdir(DBArchPath.c_str());

}

void CashRegister::AddSalesToReg(FiscalCheque* CurCheque)
{
    vector<SalesRecord> Sales;
    for(unsigned int i=0;i<CurCheque->GetCount();i++){
        if (((*CurCheque)[i].ItemFlag==SL)||((*CurCheque)[i].ItemFlag==RT))
        {

            string markGood = "";

            if ((*CurCheque)[i].ItemAlco ||	(*CurCheque)[i].ItemCigar)
            {
                markGood = "[M] ";
            }

            SalesRecord tmpSales;
            string tCode = (*CurCheque)[i].ItemCode == FreeSumCode ? "" :Int2Str((*CurCheque)[i].ItemCode) + ' ';
            tmpSales.Name = tCode.c_str() + markGood+ trim((*CurCheque)[i].ItemName);
            tmpSales.Price=(*CurCheque)[i].ItemPrice;
            tmpSales.Quantity=(*CurCheque)[i].ItemCount;
            tmpSales.Sum = (*CurCheque)[i].ItemSum;
            tmpSales.SumCorr = 0;
            // полная сумма без скидок
            //tmpSales.FullPrice=(*CurCheque)[i].ItemFullDiscPrice;
            tmpSales.FullSum = (*CurCheque)[i].ItemFullDiscSum;
            double FullPrice = 0;
            if ((*CurCheque)[i].ItemCount!=0)
                FullPrice = RoundToNearest((*CurCheque)[i].ItemFullDiscSum/(*CurCheque)[i].ItemCount,0.01);

            tmpSales.FullPrice=FullPrice;
            //
            tmpSales.Type=(*CurCheque)[i].ItemFlag;

            //;;Log((char*)("LOCK ("+tCode+")="+(*CurCheque)[i].Lock).c_str());

            tmpSales.Section = GetSection((*CurCheque)[i].Lock);
            tmpSales.Manufacture = GetOrgInfo(tmpSales.Section);

            int NDS = 0;
            if (tmpSales.Section==0)
            {
                // на НДС разбиваем только 0-ю секцию, остальные Без НДС
                switch (int((*CurCheque)[i].NDS))
                {
                    case 18:NDS=1;break;
                    case 20:NDS=1;break;
                    case 10:NDS=2;break;
                    default:NDS=0;break;
                }

            }
            tmpSales.NDS = NDS;

            if ((*CurCheque)[i].ItemCode == EXCESS_PREPAY)
            {
                tmpSales.PaymentType = 4; // Продажа
                tmpSales.PaymentItem = 15;  // Внереализационный доход
            }
            else
            {
                if ((*CurCheque)[i].Avans)
                {
                    tmpSales.PaymentType = 3; // Аванс
                    tmpSales.PaymentItem = 10;  // Платеж
                } else {
                    tmpSales.PaymentType = 4; // Полный расчет
                    tmpSales.PaymentItem = 1;  // Товар
                }
            }

            // Ciragette actsiz
            tmpSales.ActsizType = AT_NONE;
            tmpSales.Actsiz = "";

#ifdef ALCO_VES

            if ((*CurCheque)[i].ItemPivoRazliv)
            {
				  Log("CurCheck PivoRazliv");
				  
                if ((*CurCheque)[i].Actsiz.size()>0) {
                    tmpSales.ActsizType = AT_PIVO_RAZLIV;
                    tmpSales.Actsiz = (*CurCheque)[i].Actsiz[(*CurCheque)[i].Actsiz.size()-1];
                }
            }
            else
            {
#endif

            if ((*CurCheque)[i].ItemCigar)
            {
                if ((*CurCheque)[i].Actsiz.size()>0) {
                    tmpSales.ActsizType = AT_CIGAR;
                    tmpSales.Actsiz = (*CurCheque)[i].Actsiz[(*CurCheque)[i].Actsiz.size()-1];
                }
            }
            else
            {

                if ((*CurCheque)[i].ItemTextile)
                {
                    if ((*CurCheque)[i].Actsiz.size()>0) {
                        tmpSales.ActsizType = AT_TEXTILE;
                        tmpSales.Actsiz = (*CurCheque)[i].Actsiz[(*CurCheque)[i].Actsiz.size()-1];
                    }
                }               
                else
                {

                    if ((*CurCheque)[i].ItemMilk)
                    {
						
						 Log("CurCheck Milk");
						
                        if ((*CurCheque)[i].Actsiz.size()>0) {
                            tmpSales.ActsizType = AT_MILK;
                            tmpSales.Actsiz = (*CurCheque)[i].Actsiz[(*CurCheque)[i].Actsiz.size()-1];
                        }
                    }
                    else
                    {

                    //все данные маски ниже многоразовые ШК ставим 2400001323807
                    //
                    //865201 Маска защитная ANNALIZA прямая ,хлопок 97%, эластан 3%, резинка белая 10мм Россия
                    //865202 Маска защитная ANNALIZA прямая, спанбонд,резинка 80мм черная Россия
                    //865203 Маска защитная ANNALIZA фигурная, хлопок 68%, п/э 27%, вискоза 3%, эластан 2%, черный,резинка 10мм Р
                    //866188 Маска лицевая гигиеническая 5шт Россия
                    //865946 Маска лицевая гигиеническая Россия
                    //863968 Маска многоразовая Россия
                            if ((*CurCheque)[i].ItemMask) {
                                tmpSales.ActsizType = AT_MASK;
                                tmpSales.Actsiz = "2400001323807";
                            }
                    }

                }
            }
#ifdef ALCO_VES
            }
#endif

            Sales.insert(Sales.end(),tmpSales);
        }
    }

    //! === 54-ФЗ === !//
    // Расчет Дельты копеек для 54-ФЗ
    // Описание алгоритма: согласно закону 54-ФЗ "Онлайн-касса"
    // сумма по всем позициям может отличатся от итоговой суммы чека, но не более чем на 1 рубль
    // Дельта может быть строго положительная и в пределах 0 - 0.99
    // Алгоритм: Если итоговая дельта попадает в допустимые пределы, то оставляем все "как есть"
    // Если Дельта отрицательная, то корректируем суммы, увеличивая цену позиции на 0.01
    // На каждой иттерации:
    // 1. ищем позицию с минимальным расхождением (дельтой) и корректируем ее цену и сумму,
    //      данную позицию больше не корректируем
    // 2. проверяем что получилось:
    //      если итоговая дельта по чеку удовлетворяет условиям, то завершаем цикл
    //      иначе ищем следующую минимальную дельту и повторяем п1.

    double DeltaOnCheck = GetDeltaSales(Sales);

    LoggingAction(GetLastCheckNum()+1,DELTAONCHECKBEGIN, DeltaOnCheck, CurrentSaleman->Id, -1);

    if((DeltaOnCheck<0) || (DeltaOnCheck>=1))
    {
        //Нужна корректировка
        int SalesCount = Sales.size();
        for(unsigned int i=0;i<SalesCount;i++)
        {
            int minItem = -1;
            double minDelta = 1;

            double LocalCount, Summa, LocalPrice, LocalSum, Delta, DeltaNew;
            LocalCount=Summa=LocalPrice=LocalSum=Delta=DeltaNew=0;

            // Ищем минимальную дельту
            for(unsigned int j=0;j<SalesCount;j++)
            {

                SalesRecord tmpSales = Sales[j];
                if (tmpSales.SumCorr>0) continue; // уже откорректированный

                LocalCount = tmpSales.Quantity;
                Summa      = RoundToNearest((tmpSales.SumCorr==0)?tmpSales.Sum:tmpSales.SumCorr, .01);
Log("Summa=" + Double2Str(Summa));
                LocalPrice = RoundToNearest((LocalCount==0)?0:Summa/LocalCount, .01);
Log("LocalPrice=" + Double2Str(LocalPrice));
                LocalSum   = RoundToNearest(LocalPrice * LocalCount, .01);
Log("LocalSum=" + Double2Str(LocalSum));
                Delta      = RoundToNearest(LocalSum-tmpSales.Sum, .01);
Log("Delta=" + Double2Str(Delta));

                if ((Delta<0) && (fabs(Delta)>=.01))
                {
                    LocalPrice += 0.01;
                    LocalSum = RoundToNearest(LocalPrice * tmpSales.Quantity, .01);
                    DeltaNew = RoundToNearest(LocalSum-tmpSales.Sum, .01);

                    if (DeltaNew - Delta < minDelta)
                    {
                        minDelta = DeltaNew - Delta;
                        minItem  = j;
                    }


                }
            }

            if (minItem<0) break; // не нашли минимальную дельту,  алгоритм

            // если нашли минимальную дельту, то внесем корректировку и пересчитаем итог
            if ((minItem>=0) && (minItem<SalesCount))
            {
                SalesRecord tmpSales = Sales[minItem];
                LocalCount = tmpSales.Quantity;
                Summa      = RoundToNearest((tmpSales.SumCorr==0)?tmpSales.Sum:tmpSales.SumCorr, .01);
Log("Summa=" + Double2Str(Summa));
                LocalPrice = RoundToNearest((LocalCount==0)?0:Summa/LocalCount, .01);
Log("LocalPrice=" + Double2Str(LocalPrice));
                LocalSum   = RoundToNearest(LocalPrice * LocalCount, .01);
Log("LocalSum=" + Double2Str(LocalSum));
                Delta      = RoundToNearest(LocalSum-tmpSales.Sum, .01);
Log("Delta=" + Double2Str(Delta));

                if ((Delta<0) && (fabs(Delta)>=.01))
                {
                    // расчет скорректированной суммы = Количество * ( <первоначальная цена> + 0.01 )
                    LocalPrice += 0.01;
                    LocalSum = RoundToNearest(LocalPrice * tmpSales.Quantity, .01);

                    tmpSales.SumCorr = LocalSum; // скорректированная сумма, она пойдет на ФР

                    Sales[minItem] = tmpSales; // запишем изменения

                    // Проверим, что получилось
                    // если Итоговая дельта на весь чек стала положительной, то завершаем расчет
                    DeltaOnCheck = GetDeltaSales(Sales);
                    if(!((DeltaOnCheck<0) || (DeltaOnCheck>=1)))
                    {
                        break;
                    }

                }

            }



        }

    }

    LoggingAction(GetLastCheckNum()+1,DELTAONCHECKEND, DeltaOnCheck, CurrentSaleman->Id, -1);

    // Фиксируем итоговую структуру
    FiscalReg->InsertSales(Sales);
}

double CashRegister::GetDeltaSales(vector<SalesRecord> Sales)
{
	#ifndef BONUS_LOG_SKIDKA
		;;Log("GetDeltaSales Start");
	#endif
	    
	
    double DeltaOnCheck=0;
    double LocalCount, Summa, LocalPrice, LocalSum, Delta;
    LocalCount=Summa=LocalPrice=LocalSum=Delta=0;
    int SalesCount = Sales.size();
    for(unsigned int i=0;i<SalesCount;i++)
    {
        SalesRecord tmpSales = Sales[i];
        LocalCount = tmpSales.Quantity;
        Summa      = RoundToNearest((tmpSales.SumCorr==0)?tmpSales.Sum:tmpSales.SumCorr, .01);       
        LocalPrice = RoundToNearest((LocalCount==0)?0:Summa/LocalCount, .01);
        LocalSum   = RoundToNearest(LocalPrice * LocalCount, .01);
        Delta      = RoundToNearest(LocalSum-tmpSales.Sum, .01);

        if (fabs(Delta)>=.01)
        {
            DeltaOnCheck+=Delta;
        }

    }

    return DeltaOnCheck;

}



void CashRegister::AddPaymentToReg()
{
    vector<PaymentInfo> Payment;

    LoadPayment(&Payment);

    FiscalReg->InsertPayment(Payment);
}

void CashRegister::SavePaymentFromReg()
{
    vector<PaymentInfo> *Payment=NULL;

    FiscalReg->GetPayment(&Payment);
    if (Payment)
    {
        long long CustomerCode=0;
        if (CurrentWorkPlace)
            CustomerCode = CurrentWorkPlace->CurrentCustomer;

        SavePayment(*Payment,CurrentCheckNumber?CurrentCheckNumber:GetLastCheckNum(),KKMNo,KKMSmena,KKMTime,CurrentSaleman->Id, CustomerCode);
        Payment->clear();
        CurrentCheckNumber = 0;
    }
}

void CashRegister::CashlessClick()
{
    Cashless = !Cashless;
    try
    {
        DbfMutex->lock();
        ConfigTable->PutField("CASHLESS", Cashless ?"T":"F");
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }
}


void CashRegister::FRK5Click()
{
    FRKFont5 = !FRKFont5;
    try
    {
        DbfMutex->lock();
        ConfigTable->PutField("FRK_FONT", FRKFont5 ?"T":"F");
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }
}

void CashRegister::OFDClick()
{
    OFDMODE = !OFDMODE;
    try
    {
        DbfMutex->lock();
        ConfigTable->PutField("OFD", OFDMODE ?"T":"F");
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }
}

void CashRegister::SetSbPayPassClick()
{
    SBPayPass = !SBPayPass;
    try
    {
        DbfMutex->lock();
        ConfigTable->PutField("SBPAYPASS", SBPayPass ?"T":"F");
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }
}

void CashRegister::CloseAdminCheck()
{
    //ExitButton->setBackgroundColor(yellow);
    //ShowMessage("Чек закрыт Админ");
    Log("============================");
    Log("Был закрыт чек");
    Log("============================");
    int er=FiscalReg->closeAdminCheck();
    ShowErrorCode(er);
}


// Nick
void CashRegister::MilleniumClick()
{
    CheckMillenium = !CheckMillenium;
}

int CashRegister::ImportIntoWork(DbfTable *tbl,string filename,string errortext)
//перенос из папки импорта в рабочую папку
//
{
    FILE * f;
    if((f = fopen((DBImportPath+QDir::separator()+filename).c_str(), "r")))
    {
        fclose(f);
        DbfMutex->lock();
        tbl->CloseDatabase();
        DbfMutex->unlock();

        remove((DBGoodsPath+QDir::separator()+"tt"+filename).c_str());
        rename((DBGoodsPath+QDir::separator()+filename).c_str(),
            (DBGoodsPath+QDir::separator()+"tt"+filename).c_str());
        if(rename((DBImportPath+QDir::separator()+filename).c_str(),
            (DBGoodsPath+QDir::separator()+filename).c_str()))
        {
            rename((DBGoodsPath+QDir::separator()+"tt"+filename).c_str(),
                (DBGoodsPath+QDir::separator()+filename).c_str());
            ShowMessage(errortext);
        }

        bool open_error = false;

        DbfMutex->lock();
        try
        {
            tbl->OpenDatabase(DBGoodsPath+QDir::separator()+filename);
        }
        catch(cash_error& er)
        {
            char tmpstr[1024];
            sprintf(tmpstr,"ERROR: Ошибка загрузки файла %s",filename.c_str());
            Log(tmpstr);

            open_error = true;
            er.showmsg = false;
            LoggingError(er);
        }

        if (open_error)
        {
           // накосячил - верни все как было!
            rename((DBGoodsPath+QDir::separator()+"tt"+filename).c_str(),
                (DBGoodsPath+QDir::separator()+filename).c_str());

            tbl->OpenDatabase(DBGoodsPath+QDir::separator()+filename);

            char tmpstr[1024];
            sprintf(tmpstr,"Файл %s восстановлен",filename.c_str());
            Log(tmpstr);

        }

        DbfMutex->unlock();

        remove((DBGoodsPath+QDir::separator()+"tt"+filename).c_str());
    }

}

void CashRegister::GetNewCustomer()
{
    //;;Log("[FUNC] GetNewCustomer [START]");

    FILE * f;

    // проверяем флаг
    if(!(f = fopen((DBImportPath+QDir::separator()+FLAG).c_str(), "r")))
    {
        return;
    }
    fclose(f);


    ImportIntoWork(DiscountsTable,DISCDESC,"Не могу перенести схему скидок!");
    ImportIntoWork(CustRangeTable,CUSTRANGE,"Не могу перенести диапазон карточек!");
    ImportIntoWork(CustomersValueTable,CUSTVAL,"Не могу перенести накопления по дисконтным картам!");
    ImportIntoWork(GoodsPriceTable,GOODSPRICE,"Не могу перенести цены товаров!");

    bool copy_error, open_error, index_error;
    copy_error = open_error = index_error = false;

    string FileName    = DBGoodsPath+QDir::separator()+CUSTOMERS,
           OldFileName = DBGoodsPath+QDir::separator()+"old_"+CUSTOMERS,
           NewFileName = DBImportPath+QDir::separator()+CUSTOMERS,
           IndexOldFileName = DBGoodsPath+QDir::separator()+"ttcustomer.ndx",
           IndexFileName    = DBGoodsPath+QDir::separator()+"customer.ndx",
           IndexNewFileName = DBImportPath+QDir::separator()+"customer.ndx";

    if((f = fopen(NewFileName.c_str(), "r")))
    {
        fclose(f);
        try
        {

            DbfMutex->lock();
            CustomersTable->CloseIndices();
            CustomersTable->CloseDatabase();//store customer's list into local table
            DbfMutex->unlock();

            remove(OldFileName.c_str());
            rename(FileName.c_str(), OldFileName.c_str());

            if(rename(NewFileName.c_str(), FileName.c_str()))
            {
                rename(OldFileName.c_str(),	FileName.c_str());
                ShowMessage("Не могу перенести список дисконтных карт!");
                copy_error=true;
            }


            DbfMutex->lock();

            CustomersTable->OpenDatabase(DBGoodsPath+QDir::separator()+CUSTOMERS);

            try
            {
                if(!(f = fopen(IndexNewFileName.c_str(), "r")) || (copy_error))
                {
                    CustomersTable->CreateIndex(IndexFileName.c_str(),"ID");
                }
                else
                {
                    fclose(f);
                    remove(IndexOldFileName.c_str());
                    rename(IndexFileName.c_str(), IndexOldFileName.c_str());
                    if(rename(IndexNewFileName.c_str(), IndexFileName.c_str()))
                    {
                        CustomersTable->CreateIndex(IndexFileName.c_str(),"ID");
                    }
                    remove(IndexOldFileName.c_str());
                }

                CustomersTable->CloseIndices();
                CustomersTable->OpenIndex(IndexFileName,"ID");

                DbfMutex->unlock();
            }
            catch(cash_error& ex) // ошибка открытия индекса
            {
               DbfMutex->unlock();
               ex.showmsg = false;
               LoggingError(ex);

               index_error = true;
               Log("ERROR: Ошибка загрузки файла CUSTOMER.DBF /индекс/");
            }

        }
        catch(cash_error& ex) // ошибка открытия файла
        {
            DbfMutex->unlock();
            ex.showmsg = false;
            LoggingError(ex);

            open_error = true;
           Log("ERROR: Ошибка загрузки файла CUSTOMER.DBF");
        }

        if (open_error || index_error)
        {
            try
            {

                DbfMutex->lock();
                if (index_error)
                {
                    CustomersTable->CloseIndices();
                    CustomersTable->CloseDatabase();
                }

                // вернем файл на родину
                rename(OldFileName.c_str(), FileName.c_str());

                CustomersTable->OpenDatabase(FileName);

                //CustomersTable->CreateIndex((DBGoodsPath+QDir::separator()+"customer.ndx").c_str(),"ID");
                //CustomersTable->OpenIndex(DBGoodsPath+QDir::separator()+"customer.ndx","ID");

                DbfMutex->unlock();

                Log("Файл CUSTOMER.DBF восстановлен");

            }
            catch(cash_error& ex) // ошибка восстановления файла
            {
                DbfMutex->unlock();

                LoggingError(ex);

                Log("ERROR: Ошибка восстановления файла CUSTOMER.DBF");
            }

        }

        remove(OldFileName.c_str());

    }

    //Log("- GetNewCustomer # 3");

}

void CashRegister::MsgLevelClick()
{
    CheckMsgLevel = !CheckMsgLevel;
    try
    {
        DbfMutex->lock();
        ConfigTable->PutField("MSGLEVEL", CheckMsgLevel ?"T":"F");
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }
}

void CashRegister::TestPortClick()
{
    ComPortScan s(this,ConfigTable->GetLongField("SCANPORT"),ConfigTable->GetLongField("SCANBAUD"));
    ShowMessage("ScanerTest");
}

void CashRegister::RegNumberClick()
{
    char str[64];
    sprintf(str, "%d", FiscalReg->SerialNumber());
    ShowMessage(str);
}

void CashRegister::RegTestClick()
{
    FiscalReg->Test();
}

void CashRegister::MenuTestClick()
{
    //FiscalReg->PrintBarCode();

    FiscalReg->GoodsStr("Колбаски куриные 1кг Компания ООО ЮГО-ВОСТОК",999,0.876,875.124);

    FiscalReg->PrintString("TEST",false);
    FiscalReg->PrintBarCode(1234567890);
    FiscalReg->PrintString("\n",false);
    FiscalReg->PrintString("\n",false);
    FiscalReg->PrintString("\n",false);
    FiscalReg->PrintString("\n",false);
    FiscalReg->CutCheck();
    /*
#ifndef _WS_WIN_
    struct statfs sfs;
    statfs(".", &sfs);
    char s[128];
    sprintf(s, "f_bsize=%d, f_bfree=%d, f_blocks=%d", sfs.f_bsize, sfs.f_bfree, sfs.f_blocks);
    Log(s);
#endif
    */

}

#include <qstatusbar.h>
#include <qsemimodal.h>
#include <qstatusbar.h>
#include <qtextview.h>
#include <qbutton.h>
#include "Display.h"

void CashRegister::CardClose(bool set_flag)
{
    static bool flag = false;
    if(set_flag){
        flag = true;
        return;
    }
    if(!flag)
        return;
#ifndef _WS_WIN_
    pCR->CloseDay();
    pCR->GetTermNum();
#endif
    flag = false;
}

void CashRegister::SetCardClose()
{
    Log("PressButton>> CardClose");
    CardClose(true);
}

void CashRegister::PrintCardClose()
{
    SumForm *dlg = new SumForm(this); //show simple dialog
    dlg->FormCaption->setText(W2U("Кол-во строк"));
    dlg->LineEdit->setText("10");
    int res=dlg->exec();
    if (res<=0) return;
    QString s = dlg->LineEdit->text();
    delete dlg;
    int kolstr = s.toInt();

    if (kolstr<=0) return;

    const char *pfile = "./tmp/p";

    remove(pfile);

    if (CreatePrintFile(pfile, kolstr))
           FiscalReg->PrintPCheck(pfile, false);

}

void CashRegister::PrintCardCloseClick()
{
    Log("PressButton>> PrintCardCloseClick");
    PrintCardClose();
}


void CashRegister::vemSrvClick()
{
    QString txt = ConfigTable->GetStringField("VEMSRV").c_str();
    bool ok = FALSE;
#	if QT_VERSION < 0x030000
    txt = QInputDialog::getText( "APM KACCA", W2U("Сервер видеонаблюдения."), txt, &ok);
#	endif
#	if QT_VERSION >= 0x030000
    txt = QInputDialog::getText( "APM KACCA", W2U("Сервер видеонаблюдения."),QLineEdit::Normal,txt, &ok);
#	endif
    if(ok){
        ConfigTable->PutField("VEMSRV", txt.ascii());
        m_vem.SetConnect(txt.ascii(), ConfigTable->GetLongField("VEMPORT"));
    }
}

void CashRegister::vemPortClick()
{
    int num = ConfigTable->GetLongField("VEMPORT");
    bool ok = FALSE;
    num = QInputDialog::getInteger( "APM KACCA", W2U("Порт сервера видеонаблюдения."), num, 0, 65535, 1, &ok);
    if(ok){
        ConfigTable->PutLongField("VEMPORT", num);
        m_vem.SetConnect(ConfigTable->GetField("VEMSRV").c_str(), num);
    }
}

void CashRegister::SetRoleClick()
{
    QString txt = ConfigTable->GetStringField("S_ROLE").c_str();
    bool ok = FALSE;
#	if QT_VERSION < 0x030000
    txt = QInputDialog::getText( "APM KACCA", "ROLE", txt, &ok);
#	endif
#	if QT_VERSION >= 0x030000
    txt = QInputDialog::getText( "APM KACCA", "ROLE",QLineEdit::Normal, txt, &ok);
#	endif

    if(ok){
        ConfigTable->PutField("S_ROLE", txt.ascii());
    }
}

void CashRegister::CCReportClick()
{
#ifndef _WS_WIN_
    Log("PressButton>> CC Close Day");
    if (pCR->CC_CloseDay(GetCashRegisterNum()))
        ShowMessage("С момента последнего отчета\nопераций по ПС CityCard не было!");
#endif
}

void CashRegister::EGReportClick()
{
#ifndef _WS_WIN_
    Log("PressButton>> EG Close Day");
    time_t ltime;
    time( &ltime );
    struct tm *now;
    now = localtime( &ltime );
    char str[256];

    sprintf( str, "emvtrans%d_%d_%d_%d_%d_%d.dat", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
    rename("egate/emvtrans.dat.0", (DBArchPath+"/"+str).c_str());

    sprintf( str, "emvgate%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
    rename("egate/.egate.log", (DBArchPath+"/"+str).c_str());

    if (pCR->EG_CloseDay(GetCashRegisterNum()))
        ShowMessage("С момента последнего отчета\nопераций по ПС ""ГазпромБанк"" не было!");
#endif
}

void CashRegister::VTBReportClick()
{
#ifndef _WS_WIN_
    Log("PressButton>> VTB Close Day");
    time_t ltime;
    time( &ltime );
    struct tm *now;
    now = localtime( &ltime );
    char str[256];

    sprintf( str, "vtb%d_%d_%d_%d_%d_%d.log", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec );
    //!rename("egate/.egate.log", (DBArchPath+"/"+str).c_str());

    if (pCR->VTB_CloseDay(GetCashRegisterNum()))
        ShowMessage("С момента последнего отчета\nопераций по ПС ""ВТБ24"" не было!");
#endif
}


// расчитывает общую сумму выданных сертификатов и
// возвращает список сертификатов
string GetListOfCertificates(IssueCertStruct cards[], double* summa )
{
    string FirstStr, SecondStr, tmptext = "";
    *summa =0;
    for(int i=0; i<IssueCertCount; i++ )
      {
          if(
            (cards[i].Certificate != 0)
            //&& (cards[i].Summa != 0)
            )
          {
             FirstStr  = "  "+LLong2Str(cards[i].Certificate);
             SecondStr = " ="+GetRounded(cards[i].Summa, 0.01);

            if (cards[i].Summa != 0)
                tmptext+= "\n" + GetFormatString(FirstStr, SecondStr, FiscalReg->LineLength );
            else
                tmptext+= "\n" + FirstStr;

            *summa += cards[i].Summa;
          }
      }

    if (tmptext=="") tmptext = "\n   <пусто>";

   return "---------Выдан сертификат:----------" + tmptext;
}


string CashRegister::GetCheckHeader(void)
{
    Log("=GetCheckHeader=");

    long long crCustomer=0;
    long long CurNumber =0;

    string CheckHeader="";
    string IssueCertificateText="";
    string ChangeCardText = "";
    string MillionText = "";
    string ActionVTBText = "";
    string ActionPGText = "";
    string TempText="";
    string TextHeader="";
    string ActionText="";
    char   key[40];
    key[0]=0;

    char   mil_key[40];
    mil_key[0]=0;

    char   actionVTB_key[40];
    actionVTB_key[0]=0;

//	IssueCertStruct cards[20];
//	for(int i=0;i<IssueCertCount;i++)
//    {
//     cards[i].Certificate=0;
//     cards[i].Summa=0;
//    }

    if(CurrentWorkPlace)
    {
        crCustomer=CurrentWorkPlace->CurrentCustomer;

        CurrentWorkPlace->NeedChangeCard2VIP=false;
        CurrentWorkPlace->NeedChangeCard2SuperVIP=false;

        //bool crNeedActionVTB=CurrentWorkPlace->NeedActionVTB;

        //;;if (crNeedActionVTB) Log("=crNeedActionVTB=");

        bool crNeedActionVTB = (crCustomer>=993000000000LL); // включим всегда для акции Подарок -Икра 7 лет Гатроном


        //int crNeedCert=CurrentWorkPlace->NeedCert;
        bool crNeedCert=(crCustomer>=993000000000LL);

        double crSumma = CurrentWorkPlace->Cheque->GetSum();

        //данные по дисконтной карте
        CheckHeader+=GetCustomerInfo(crCustomer);

        /*
        // Уникальный номер чека для акции
        string UINcheck = "";
        UINcheck+=RightSymbol("000"+Int2Str(Str2Int(trim(GetMagNum()))), 3);
        UINcheck+=RightSymbol("00"+Int2Str(GetCashRegisterNum()), 2);
        UINcheck+=RightSymbol("0000000000"+Int2Str(GetLastCheckNum()+1), 10);

        CheckHeader+="\n";
        CheckHeader+=GetCenterString("Уникальный номер чека");
        CheckHeader+="\n";
        CheckHeader+=GetCenterString(UINcheck);
        CheckHeader+="\n";
        */


        if(BonusMode(crCustomer)){
            CheckHeader+="\n";

            if (getLineWidthDefault() == 36) {
                CheckHeader+="============== БОНУСЫ ==============";
            } else {
                CheckHeader+="============ БОНУСЫ ============";
            }
            //BNS_RET* ret = BNS->exec("inf", LLong2Str(crCustomer), Int2Str(GetLastCheckNum()+1));

            string StatusStr = GetStatusStr(CurrentWorkPlace->StatusBonusCard);

            //!CheckHeader +="\n"+GetFormatString("Карта:", LLong2Str(crCustomer));
            CheckHeader +="\n"+GetFormatString("Статус:", StatusStr);

            if (CurrentWorkPlace->StatusBonusCard=="4")
            {
                if (getLineWidthDefault() == 36) {
                    CheckHeader +="\n*Активируйте карту на gastronom18.ru";
                    CheckHeader +="\n или отправив SMS с номером карты на";
                    CheckHeader +="\n номер +7(912) 011-10-90";
                    CheckHeader +="\n подробности на сайте gastronom18.ru";
                } else {
                    CheckHeader +="\n*Активируйте карту на сайте ";
                    CheckHeader +="\n gastronom18.ru или отправив ";
                    CheckHeader +="\n SMS с номером карты на номер";
                    CheckHeader +="\n +7(912) 011-10-90";
                    CheckHeader +="\n подробности на gastronom18.ru";
                }
            }
            if (CurrentWorkPlace->CurrentActiveBonus){
                CheckHeader +="\n"+GetFormatString("Баланс:", GetRounded(CurrentWorkPlace->CurrentActiveBonus,0.01));
            }else{
                //CheckHeader+="---";
            }

            CheckHeader +="\n"+GetFormatString("Начислено:", GetRounded(CurrentWorkPlace->Cheque->GetBonusAddSum(),0.01));

            if (CurrentWorkPlace->SummaBonusAction){
                CheckHeader +="\n"+GetFormatString("в т.ч. по акции:", GetRounded(CurrentWorkPlace->SummaBonusAction,0.01));
            }

            CheckHeader +="\n"+GetFormatString("Списано:", GetRounded(CurrentWorkPlace->Cheque->GetBonusDiscSum(),0.01));

            CheckHeader+="\n";
            for (int i=0;i<getLineWidthDefault();i++) CheckHeader+="=";
            CheckHeader+="\n";

        }

//;;Log((char*)CheckHeader.c_str());

        double Summa, ActionSumma;

        // Отключаем проверку для всех подряд.
        // Проверка на выдачу сертификата происходит только на те карты, накопленная сумма которых
        // подходит к точке выдачи сертификата, либо для чеков на большУю сумму (over 10000)
//!      if ((crNeedCert) || (crSumma>=10000))

// + dms ====== Акция Выдача сертификатов =======
// Проверка для всех чеков
      if ((crNeedCert) || (crNeedActionVTB) || (crSumma>=10000))
// + dms ====== Акция Выдача сертификатов =======
      {

        double fullsum = CurrentWorkPlace->Cheque->GetSum() + CurrentWorkPlace->Cheque->GetDiscountSum();

        Log("Проверка выдачи сертификата...");
        if (SQLGetCheckInfo->IssueCertificate(key,&IssueCertificateText,&TextHeader,&Summa,crCustomer,CurrentWorkPlace->Cheque->GetSum(), &ChangeCardText, &MillionText, mil_key, &ActionVTBText, actionVTB_key, &ActionSumma, fullsum, &ActionText))
        {

#ifdef CERTIFICATES
            Log("Certificate=" + IssueCertificateText);
#endif


// С 01 июля 2020 г. отменяется выдача сертификатов за накопления. Вместо сертификатов теперь будут начисляться бонусы на карту
            CheckHeader+="\n";
            CheckHeader+=TextHeader;
            CheckHeader+="\n";

            LoggingAction(GetLastCheckNum()+1,ISSUECERTIFICATECOMMIT,Summa,CurrentSaleman->Id, -1);

            int lKeyPressed = -1;
            while (lKeyPressed !=0)
            {
                lKeyPressed = QMessageBox::information
                ( NULL, "APM KACCA", W2U(TextHeader)
                ,W2U("ОК"),W2U("Х") ,""
                , 1, -1);
            }

/*  С 01 июля 2020 г. отменяется выдача сертификатов за накопления. Вместо сертификатов теперь будут начисляться бонусы на карту
            // Выдача сертификата
            int res = ProccessIssueCertificate(key,&CheckHeader, IssueCertificateText, TextHeader,crCustomer,Summa);

            // Если выдача отменена, то карту не меняем
            if (res==1)
            {
               ChangeCardText = "";
            }

*/
        }

      }


        if (MillionText != "")
        {

            Summa=0;
            ProccessIssueCertificate(mil_key,&CheckHeader, MillionText, TextHeader,crCustomer,Summa, true);

        }


/*
        if ((crNeedActionVTB)&&(ActionVTBText != ""))
        {

            ;;Log((char*)("Необходимо выдать сертификат ВТБ24 ="+Double2Str(ActionSumma)).c_str());
            ;;Log((char*)ActionVTBText.c_str());

            ProccessIssueCertificate(actionVTB_key,&CheckHeader, ActionVTBText, TextHeader,crCustomer,ActionSumma, false, true);

        }
*/

        if ((crNeedActionVTB)&&(ActionVTBText != ""))
        {

            string text = "    ПРИГЛАСИТЬ ЗАМ.ДИРЕКТОРА";
            text +=       "\n         ДЛЯ ВЫДАЧИ ПОДАРКА";
            text +="\n"+ActionVTBText;

            ;;Log((char*)("Необходимо выдать ПОДАРОК - БАНКУ ИКРЫ "));
            ;;Log((char*)text.c_str());


            int GuardRes=NotCorrect;


            // заблокируем сканер
            bool SaveScannerSuspend = CurrentWorkPlace->Scanner->suspend;
            CurrentWorkPlace->Scanner->suspend = true;

            QString SecondButton = QString::null;
            QString ThirdButton = QString::null;

            int lKeyPressed = -1;
            while ((lKeyPressed !=0) && (lKeyPressed !=1))
            {
                lKeyPressed = QMessageBox::information
                ( NULL, "APM KACCA", W2U(text)
                ,W2U("Сканировать подарок"), SecondButton, ThirdButton
                , 0, -1);
            }



            //bool oldscannersuspend=CurrentWorkPlace->Scanner->suspend;
            CurrentWorkPlace->Scanner->suspend = false;
            long long Barcode = CurrentWorkPlace->ScanBarcode();
            CurrentWorkPlace->Scanner->suspend = SaveScannerSuspend;

            ;;Log((char*)("ВЫДАН ПОДАРОК: "+LLong2Str(Barcode)).c_str());

            // Заголовок чека
            text = TextHeader;
            text += "\n"+ActionVTBText;
            text += "\n--------------------------------------";
            text += "\nПодпись зам. директора:";
            text += "\n\n____________________________________";
            text += "\n\n\n\n";
            text += "printheader\n";
            text += "cutcheck\n";


            CheckHeader = text + "\n" + CheckHeader;


        }

        // Акция P&G  5 августа - 9 сентября 2015

        double crSummaPG=0;
        bool crNeedActionPG = false;

        time_t ltime;
        time( &ltime );
        struct tm *now;
        now = localtime( &ltime );

        // Период акции - до 13 марта 2016г.
        // Продлен до 10 апреля 2016г.
        // Продлен до 3 мая 2016г.
        if (
            (now->tm_year+1900 == 2016)
            &&
            (
                (now->tm_mon+1 < 5)
                ||
                ((now->tm_mon+1 == 5) && (now->tm_mday <= 3))
            )
           )
        {

            crSummaPG = CurrentWorkPlace->GetSumActionPG();

            // Условия акции: сумма товаров P&G должна быть от 400 рублей
            crNeedActionPG = (crSummaPG >= 450);

            if (crNeedActionPG)
            {
                //акуия действует только в маг. 77,17,130
                try{
                    int mag = Str2Int(trim(GetMagNum()));
                    if (
                        (mag==77)
                        || (mag==17)
                        || (mag==130)
                    )
                    {
                        Log("<!> Mag P&G Action <!>");
                    }else
                    {// для прочих магазинов
                        Log("<!> Mag NOT P&G Action <!>");
                        //! ОТКЛЮЧИТЬ
                         crNeedActionPG = false;
                    }
                } catch(exception& ex) {
                    crNeedActionPG = false;
                }

            }
        }

        if (crNeedActionPG)
        {
            ActionSumma = 100; // Сертификат номиналом 100 рублей

            char tmpstr[256];

            sprintf( tmpstr, "%02d.%02d.%4d %02d:%02d:%02d",
            now->tm_mday,
            now->tm_mon + 1,
            now->tm_year + 1900,
            now->tm_hour,
            now->tm_min,
            now->tm_sec );

            string strdate = tmpstr;

            ActionPGText = strdate+"\n";
            ActionPGText += GetFormatString("Касса #"+Int2Str(GetCashRegisterNum()),"Чек #"+Int2Str(GetLastCheckNum()+1));
            ActionPGText +="\nКассир " + CurrentSaleman->Name;
            ActionPGText +="\n"+GetFormatString("Выдача сертификата P&G ", GetRounded(ActionSumma,0.01));
            //ActionVTBStr +="\nКарта N " + LLong2Str(crCustomer);
            ActionPGText +="\n" + GetFormatString("Сумма товаров P&G ", " ="+GetRounded(crSummaPG,0.01));

            ;;Log((char*)("Необходимо выдать сертификат P&G ="+Double2Str(ActionSumma)).c_str());
            ;;Log((char*)ActionPGText.c_str());

            char actionPG_key[40];

            // акция
            uuid_t act_id;

            uuid_generate(act_id);
            uuid_unparse(act_id,actionPG_key);

            ProccessIssueCertificate(actionPG_key,&CheckHeader, ActionPGText, TextHeader,crCustomer,ActionSumma, false, false, true);

        }


        // Замена карты
        CurrentWorkPlace->NeedChangeCard2VIP = false;
        CurrentWorkPlace->NeedChangeCard2SuperVIP = false;
        if (ChangeCardText != "")
        {

            ShowMessage(ChangeCardText);

            if (ChangeCardText.find("Super-VIP") != string::npos){
                CurrentWorkPlace->NeedChangeCard2SuperVIP = true;
            }
            else
            {
                CurrentWorkPlace->NeedChangeCard2VIP = true;
            }
            //ChangeBonusCard2Vip(crCustomer);

        }

        // Текст акции
        if (ActionText != "")
        {

            ShowMessage(ActionText);

        }


   }
    //SQLGetCheckInfo->GetCheckHeader(&CheckHeader,crCustomer);


//! отладить после.. перенести в отдельную функцию GetVoteInfo
//
//    // информация на корешке голосования
//	string info="";
//	int sd;
//	int curdate=xbDate().JulianDays();
//	int count = MsgLevelList.size();
//	char buf[9]; buf[8]=0;
//	//ShowMessage(NULL,Int2Str(curdate));
//	for(int i = 0; i < count; i++)
//		if(MsgLevelList[i].level == -2.0)
//		{
//			sprintf(buf,"%04d",MsgLevelList[i].startdate.tm_year+1900);
//			sprintf(buf+4,"%02d",MsgLevelList[i].startdate.tm_mon+1);
//			sprintf(buf+6,"%02d",MsgLevelList[i].startdate.tm_mday);
//			if(curdate < xbDate(buf).JulianDays())
//				continue;
//			sprintf(buf,"%04d",MsgLevelList[i].finishdate.tm_year+1900);
//			sprintf(buf+4,"%02d",MsgLevelList[i].finishdate.tm_mon+1);
//			sprintf(buf+6,"%02d",MsgLevelList[i].finishdate.tm_mday);
//			if(curdate > xbDate(buf).JulianDays())
//				continue;
//			info = info +"\n"+ MsgLevelList[i].msg;// инфо в чеке
//			//break;
//		}
//
//
//		if (!info.empty())
//		{
//            string ast = "";
//            for (j=0;j<50;j++) ast+="*";
//
//            CheckHeader = info+"\n"+CheckHeader;
//        }



    Log("end CheckHeader");

    return CheckHeader;
}

int CashRegister::ProccessIssueCertificate(char* key,string * CheckHeader, string IssueCertificateText, string TextHeader, long long crCustomer, double Summa, bool isMillion, bool isActionVTB, bool isActionPG)
{

    int res=-1;

    long long CurNumber =0;
    string TempText="";

    IssueCertStruct cards[20];
    for(int i=0;i<IssueCertCount;i++)
    {
     cards[i].Certificate=0;
     cards[i].Summa=0;
    }


////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

            int GuardRes=NotCorrect;

            string text;

            bool toexit = false;
            bool allright = false;
            double WholeSumma, CurSumma;
            WholeSumma = CurSumma = 0;

            // заблокируем сканер
            bool SaveScannerSuspend = CurrentWorkPlace->Scanner->suspend;
            CurrentWorkPlace->Scanner->suspend = true;

            QString SecondButton = QString::null;
            QString ThirdButton = QString::null;

            text = IssueCertificateText;

            WholeSumma =0;

            text +="\n" +GetListOfCertificates(cards, &WholeSumma);

            while (!toexit)
            {

                int lKeyPressed = -1;
                while ((lKeyPressed !=0) && (lKeyPressed !=1))
                {
                    lKeyPressed = QMessageBox::information
                    ( NULL, "APM KACCA", W2U(text)
                    ,W2U(isMillion?"Сканировать сертификат \"ТОРТ НА ЗАКАЗ\"":"Сканировать сертификат"), SecondButton, ThirdButton
                    , 0, -1);
                }

                // Нажали кнопку отмена
                if (lKeyPressed==1)
                {
                  if (ShowQuestion("Отменить выдачу сертификата?",false))
                  {
                      //ChangeCardText = "";
                      res=1;
                      toexit = true;
                  }
                  continue;

                }

                SecondButton  = W2U("Отмена");
                string discount;
                bool oldscannersuspend;

                CurSumma = 0;
                CurNumber=0;

                oldscannersuspend=CurrentWorkPlace->Scanner->suspend;
                CurrentWorkPlace->Scanner->suspend = false;
                CurrentWorkPlace->SearchCertificate(FindCertificate,&discount,&CurNumber,&CurSumma);
                CurrentWorkPlace->Scanner->suspend = oldscannersuspend;

                if (CurNumber>0)
                {
                    // Проверим сертификат на активность
                    switch(SQLGetCheckInfo->CheckCertificate(CurNumber, 0, false))
                    {
                        case 0:
                        case 1:
                            if (WholeSumma+CurSumma <= Summa)
                              {for(int i=0; i<IssueCertCount; i++ )
                                {if (cards[i].Certificate == CurNumber)
                                       break;
                                 else
                                   if (cards[i].Certificate == 0 )
                                    {
                                       cards[i].Certificate = CurNumber;
                                       cards[i].Summa = CurSumma;
                                       break;
                                    }
                                }
                              }
                            else
                                ShowMessage("Превышение суммы подарочной карты");

                            break;

                        //case 1:
                            // dms ====2014-01-01 ==== сертификат автоматически разблокируется при отбитии чека
                            //т.ч. эту проверку пропускаем
                            //ShowMessage("Сертификат заблокирован");
                          //  break;

                        case 2:
                            ShowMessage("Неправильный номер сертификата");
                            break;

                        case 3:
                            ShowMessage("Нет данных по сертификату");
                            break;

                        case -1: // ошибка связи с базой
                            ShowMessage("Невозможно проверить сертификат, сеть недоступна");
                            break;
                    }
                }

                text = IssueCertificateText;
                WholeSumma =0;

                text +="\n" +GetListOfCertificates(cards, &WholeSumma);

                //for(int i=0;i<IssueCertCount;i++) if (0 != cards[i].Certificate) WholeSumma+=cards[i].Summa;

                if  (
                      (WholeSumma == Summa)
                      &&
                      (cards[0].Certificate != 0) // Для тортов на заказ - проверям, считан ли хотя бы один сертификат (сумму при этом не проверяем)
                    )
                {
                    allright = true;
                }

                toexit = allright || lKeyPressed == 1;

            } // набираем сумму сертификата

            bool oldscannersuspend;

            string finishtext = "\n" +GetListOfCertificates(cards, &WholeSumma);

            text = IssueCertificateText + finishtext;
            text +="\n";
            text +="\n";
            text +="Нажмите кнопку ОK и введите код СПП";

            bool SPPCommit = false;
            bool Cancel = false;
            int cnt = 3; // 3 попытки
            if (allright)
            {
                while(!SPPCommit && cnt)
                {
                    while (QMessageBox::information
                        ( NULL,"APM KACCA",W2U(text)
                        ,"Ok", QString::null, QString::null
                        ,0,-1)!=0);

                    oldscannersuspend=CurrentWorkPlace->Scanner->suspend;
                    CurrentWorkPlace->Scanner->suspend = false;
                    SPPCommit = CurrentWorkPlace->CheckGuardCode(GuardRes);
                    CurrentWorkPlace->Scanner->suspend = oldscannersuspend;

                    cnt--;
                }
            }

            IssueCertificateText = TextHeader + "\n" + IssueCertificateText;

            // разблокируем сканер
            CurrentWorkPlace->Scanner->suspend = false;


            int StatusOK, StatusCancel;

            if (isActionPG)
            {// Если это акция P&G 2015
               StatusOK     = 30; // Выдан
               StatusCancel = 32; // Отменен
            }
            else
                if (isActionVTB)
                {// Если это акция ВТБ24
                    StatusOK     = 20; // Выдан
                    StatusCancel = 22; // Отменен
                }
                else
                    if (isMillion)
                    {// Если это ТОРТ НА ЗАКАЗ
                        StatusOK     = 10; // Выдан
                        StatusCancel = 12; // Отменен
                    }
                    else
                    {// Для обычных подарочных карт
                        StatusOK     = 0; // Выдан
                        StatusCancel = 2; // Отменен
                    }

                // Выдача сертификатов - подарочных карт
                if (SPPCommit)
                {

                    if (SQLGetCheckInfo->IssueCertificateCommit(key,&TempText, cards, GetCashRegisterNum(),CurrentSaleman->Id,GuardRes, crCustomer, Summa,StatusOK))
                    //if (SQLGetCheckInfo->IssueCertificateCommit(key,&TempText, GetCashRegisterNum(),CurrentSaleman->Id,GuardRes, crCustomer, Summa))
                    {
                        text=IssueCertificateText+finishtext+TempText;
                        *CheckHeader=text+*CheckHeader;

                        LoggingAction(GetLastCheckNum()+1,ISSUECERTIFICATECOMMIT,Summa,CurrentSaleman->Id,GuardRes);

                        res=0;
    //;;Log((char *) CheckHeader.c_str());
                    }
                }
                else
                {
                    if(SQLGetCheckInfo->IssueCertificateRollback(key,&TempText,0,GetCashRegisterNum(),CurrentSaleman->Id,GuardRes,StatusCancel))
                    {
                        IssueCertificateText+=TempText;
                        *CheckHeader=IssueCertificateText+*CheckHeader;

                        LoggingAction(GetLastCheckNum()+1,ISSUECERTIFICATEROLLBACK,Summa,CurrentSaleman->Id,GuardRes);

                        res=1;
                    }
                }

//			}

    return res;
}


string CashRegister::GetCheckFutter(double sum)
{
    string info="";
    int sd;
    int curdate=xbDate().JulianDays();
    int count = MsgLevelList.size();
    //double sum = CurCheque->GetSum();
    char buf[9]; buf[8]=0;
    double level = -1;
    //ShowMessage(NULL,Int2Str(curdate));
    for(int i = 0; i < count; i++)
        if((MsgLevelList[i].mode == 1) &&
           (MsgLevelList[i].level <= sum) &&
           (level <= MsgLevelList[i].level))
        {
            sprintf(buf,"%04d",MsgLevelList[i].startdate.tm_year+1900);
            sprintf(buf+4,"%02d",MsgLevelList[i].startdate.tm_mon+1);
            sprintf(buf+6,"%02d",MsgLevelList[i].startdate.tm_mday);
            if(curdate < xbDate(buf).JulianDays())
                continue;
            sprintf(buf,"%04d",MsgLevelList[i].finishdate.tm_year+1900);
            sprintf(buf+4,"%02d",MsgLevelList[i].finishdate.tm_mon+1);
            sprintf(buf+6,"%02d",MsgLevelList[i].finishdate.tm_mday);
            if(curdate > xbDate(buf).JulianDays())
                continue;

            // + Фильтр по дисконтным картам
            if(MsgLevelList[i].custtype)
            {
                if (CurrentWorkPlace==NULL) continue;

                long long CurrCard = CurrentWorkPlace->CurrentCustomer;

                if(!CurrCard) continue;

                CardInfoStruct CardInfo;
                if (FindCardInRangeTable(&CardInfo, CurrCard))
                {
                    if ( MsgLevelList[i].custtype == CardInfo.CardType )
                    {
                        info=MsgLevelList[i].msg;// инфо в чеке
                        break;
                    }
                    else
                        continue;
                };
            }
            // - Фильтр по дисконтным картам

            //info+="\n"+MsgLevelList[i].msg;// инфо в чеке
            info=MsgLevelList[i].msg;// инфо в чеке
            level = MsgLevelList[i].level;
            //break;
        }


    return info;
}

int CashRegister::CardReaderTestClick(void)
{
    tCR = new(CardReader);
    //tCR->CC_Test(0.01,0);
    tCR->CC_Test(0,0);
    delete tCR;
    return 0;
}

int CashRegister::DeleteOldFiles(QString ArchPath, int period)//period in months
{
    int deleted = 0,year,month;
/*
    QDir ArchDir(ArchPath.c_str());
    QStringList files = ArchDir.entryList(QDir::Files,QDir::Unsorted), sTmp;
    QFileInfo fInf;
    QDateTime lastMod;
    QDateTime cdate = QDateTime::currentDateTime();
    bool ok;
    sTmp = cdate.toString("yyyy");
    year = sTmp.toInt(&ok);
    sTmp = cdate.toString("MM");
    month = sTmp.toInt(&ok);
    int icdate = (year-1900)*12+month;

    for(int i=0;i<files.count();i++){
        fInf = QFileInfo(ArchPath + QDir::separator()+files[i]);
        lastMod = fInf.lastModified();
        //fileMod = lastMod.toString("yyyy")+" "+lastMod.toString("MM");
        int fmm = (lastMod.toString("yyyy").toInt(&ok)-1900)*12+lastMod.toString("MM").toInt(&ok);
        if ((icdate - fmm) > period){
            QFile::remove(ArchPath + QDir::separator()+files[i]);
            deleted++;
        }
    }*/
    return deleted;
}

void CashRegister::TestBankConnectionClick()
{
    bool Res = NumberChange("Кол-во попыток связи с банком","TESTBANK");
    GetTryBankConnect();

    if (Res && TryBankConnect) TestBankConnection(0x11);
}


//!void CashRegister::TryConnectToBank(unsigned short timeout = 0)
void CashRegister::TryConnectToBank(unsigned short timeout)
{
    // связь с банком

    bool NewStatus = StatusBankConnection;

    if ((short)TryBankConnect<0) TryBankConnect = -TryBankConnect;

    // заданное кол-во попыток
    for(int i=1;i<=TryBankConnect;i++)
    {
        // основной IP  /telnet 212.120.183.9 666/
        //if (Socket->TestBank("212.120.183.9", 666, timeout)==0)
        //if (Socket->TestBank("94.51.87.70", 666, timeout)==0)
        if (Socket->TestBank("194.54.14.89", 666, timeout)==0)
        {
            NewStatus = true;
            break;
        }
        else
            NewStatus = false;
    }

    if (!NewStatus)
    {
        // заданное кол-во попыток
        for(int i=1;i<=TryBankConnect;i++)
        {
            // резервный IP /telnet 194.186.207.162 670/
            if (Socket->TestBank("194.54.14.162", 670, timeout)==0)
            {
                NewStatus = true;
                break;
            }
            else
                NewStatus = false;
        }
    }

    if (NewStatus != StatusBankConnection) // если смена статуса
    {
        StatusBankConnection = NewStatus;
        if (!StatusBankConnection)
            throw cash_error(bank_error,0,BankIsInaccessible);
    }

}

// Флаг ShowMsg:
//0-й бит - показывать сообщение об ошибке,
//1-й бит - показывать собщение при успешном установлении связи
// timeout: если =0, то без таймаута
//void CashRegister::TestBankConnection(unsigned short ShowMsg, unsigned short timeout =0)
void CashRegister::TestBankConnection(unsigned short ShowMsg, unsigned short timeout)
{
    //Log("[FUNC] TestBankConnection [START]");
    // проверка связи с банком
    try
    {
        TryConnectToBank(timeout);
    }
    catch(cash_error& er)
    {
        Log("[FUNC] TestBankConnection [ERROR]");
        LoggingError(er);
    }

    if (StatusBankConnection && (ShowMsg & 0x10))  ShowMessage("Связь с банком успешно установлена");
    if (!StatusBankConnection && (ShowMsg & 0x01)) ShowMessage("Ошибка связи с банком!");

    UpdateBankStatus();
    if (CurrentWorkPlace!=0) CurrentWorkPlace->UpdateBankStatus();

    Log("[FUNC] TestBankConnection [END]");

}

void CashRegister::UpdateBankStatus(void)
{
    StatusMutex->lock();

    QFont tmpFont=BankInfo->font();
    tmpFont.setStrikeOut(!StatusBankConnection);
    BankInfo->setFont(tmpFont);
    BankInfo->repaint();
    if (CurrentWorkPlace!=0)
    {
        CurrentWorkPlace->BankInfo->setFont(tmpFont);
        CurrentWorkPlace->BankInfo->repaint();
    }

    StatusMutex->unlock();
}

//Номер ККМ смены (для ФР в режиме печати)
int CashRegister::GetKKMSmena(void)//get the current store's number
{
    DbfMutex->lock();
    ConfigTable->GetFirstRecord();
    int res = ConfigTable->GetLongField("LAST_SMENA");

    DbfMutex->unlock();
    return res;
}

void CashRegister::SetKKMSmena(int Num)//set the current store's number
{
    try
    {
        DbfMutex->lock();
        ConfigTable->GetFirstRecord();
        ConfigTable->PutLongField("LAST_SMENA",Num);
        ConfigTable->PutRecord();
        DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
    }
}

// устанавливает наименование документа в чеке (по умолчанию "Чек")
bool CashRegister::CheckNameChange(string Capt,string FieldName)
{
    SumForm *dlg = new SumForm(this);//show the simple dialog, read the line of the text
    dlg->FormCaption->setText(W2U(Capt));
    DbfMutex->lock();
    dlg->LineEdit->setText(W2U(ConfigTable->GetStringField(FieldName.c_str()).c_str()));
    DbfMutex->unlock();
    dlg->LineEdit->selectAll();

    int res=dlg->exec();

    char name[128];memset(name,'\0',128);
    strcpy(name,U2W(dlg->LineEdit->text()).c_str());

    delete dlg;

    if (res==QDialog::Accepted)
    {
        try
        {
            DbfMutex->lock();
            ConfigTable->PutField(FieldName.c_str(), name);
            ConfigTable->PutRecord();
            DbfMutex->unlock();
            return true;
        }
        catch(cash_error& ex)
        {
            DbfMutex->unlock();
            LoggingError(ex);
        }
    }

    return false;//if path was changed we return true
}

//// возвращает наименование документа в чеке (по умолчанию "Чек")
string CashRegister::GetCheckName(void)
{
    DbfMutex->lock();
    string res = ConfigTable->GetStringField("CHECKNAME");
    DbfMutex->unlock();

    return res;
}

void CashRegister::GetTryBankConnect(void)
{
    DbfMutex->lock();
    TryBankConnect = ConfigTable->GetLongField("TESTBANK");
    DbfMutex->unlock();
}

 void CashRegister::LogFixCheque(FiscalCheque* CurCheque)
 {
    try
    {
       char tmpstr[1024];
       sprintf(tmpstr, "=== Final check === # %d : Sum=%.2f, SumToPay=%.2f, Disc=%.2f, Cert=%.2f, Bonus=%.2f, BBonus=%.2f, PType=%d, PrePay=%.2f",
            GetLastCheckNum()+1,
            CurCheque->GetSum(),
            CurCheque->GetSumToPay(),
            CurCheque->GetDiscountSum()-CurCheque->CertificateSumm-CurCheque->BankBonusSumm-CurCheque->BonusSumm,
            CurCheque->CertificateSumm,
            CurCheque->BonusSumm,
            CurCheque->BankBonusSumm,
            CurCheque->PaymentType,
            CurCheque->PrePayCertificateSumm);
        Log(tmpstr);

    }
    catch(cash_error& ex)
    {
        LoggingError(ex);
    }

 }


void CashRegister::LogAddGoods(GoodsInfo * g)
{
    char tmpstr[1024];
#ifdef LOGNDS
    if (GetSection(g->Lock) > 0) sprintf(tmpstr, " + %d (%d), %d, %s (%s), P=%.2f, C=%.3f, S=%.2f, %s, NDS=%.0f",
                                         g->ItemNumber, g->InputType,  g->ItemCode, g->ItemBarcode.c_str(), g->ItemSecondBarcode.c_str(), g->ItemPrice, g->ItemCount, g->ItemSum, g->ItemName.c_str(), 0);
    else sprintf(tmpstr, " + %d (%d), %d, %s (%s), P=%.2f, C=%.3f, S=%.2f, %s, NDS=%.0f",
                 g->ItemNumber, g->InputType,  g->ItemCode, g->ItemBarcode.c_str(), g->ItemSecondBarcode.c_str(), g->ItemPrice, g->ItemCount, g->ItemSum, g->ItemName.c_str(), g->NDS);

#else
    sprintf(tmpstr, " + %d (%d), %d, %s (%s), P=%.2f, C=%.3f, S=%.2f, %s, NDS=%.0f",
            g->ItemNumber, g->InputType,  g->ItemCode, g->ItemBarcode.c_str(), g->ItemSecondBarcode.c_str(), g->ItemPrice, g->ItemCount, g->ItemSum, g->ItemName.c_str(), g->NDS);
#endif
        Log(tmpstr);
}

void CashRegister::LogDelGoods(GoodsInfo * g)
{



    char tmpstr[1024];
    //sprintf(tmpstr, "= ADD = %d, %d, %s,\"%s\",C=%.2f, CNT=%.3f, S=%.2f, D=%.2f",
    sprintf(tmpstr, " - %d, %d, %s (%s), P=%.2f, C=%.3f, S=%.2f, %s, NDS=%.0f",
            g->ItemNumber, g->ItemCode, g->ItemBarcode.c_str(), g->ItemSecondBarcode.c_str(),
            //g->ItemName.c_str(),
            g->ItemPrice, g->ItemCount, g->ItemSum, g->ItemName.c_str(), g->NDS);
    Log(tmpstr);
}

void CashRegister::LogEditGoods(GoodsInfo * g)
{


    char tmpstr[1024];
    sprintf(tmpstr, " * %d, %d, %s (%s), P=%.2f, C=%.3f, S=%.2f, %s, NDS=%.0f",
            g->ItemNumber, g->ItemCode, g->ItemBarcode.c_str(), g->ItemSecondBarcode.c_str(), g->ItemPrice, g->ItemCount, g->ItemSum, g->ItemName.c_str(), g->NDS);
    Log(tmpstr);
}


void CashRegister::ShowLevelDiscountMessage(double disc_summ, double total_summ)
{
            string msg_disc, msg_total;
            double level_disc, level_total;
            level_disc=level_total=0;
            msg_disc=msg_total="";
            int cnt = MsgLevelList.size();
            int curdate = xbDate().JulianDays();
            char buf[9]; buf[8]=0;

            for(int i = 0; i < cnt; i++)
            {
                if ((MsgLevelList[i].mode != 6) && (MsgLevelList[i].mode != 7)) continue;

                sprintf(buf,"%04d%02d%02d",MsgLevelList[i].startdate.tm_year+1900, MsgLevelList[i].startdate.tm_mon+1, MsgLevelList[i].startdate.tm_mday);
                if(curdate < xbDate(buf).JulianDays()) continue;
                sprintf(buf,"%04d%02d%02d",MsgLevelList[i].finishdate.tm_year+1900, MsgLevelList[i].finishdate.tm_mon+1, MsgLevelList[i].finishdate.tm_mday);
                if(curdate > xbDate(buf).JulianDays()) continue;

                if ((MsgLevelList[i].mode == 6) // сообщения по превышению суммы накопленной скидки
                    && (disc_summ  >= MsgLevelList[i].level)
                    && (level_disc < MsgLevelList[i].level))
                {
                    msg_disc = MsgLevelList[i].msg;
                    level_disc = MsgLevelList[i].level;
                }

                if ((MsgLevelList[i].mode == 7) // сообщения по превышению накопленной суммы
                    && (total_summ  >= MsgLevelList[i].level)
                    && (level_total < MsgLevelList[i].level))
                {
                    msg_total = MsgLevelList[i].msg;
                    level_total = MsgLevelList[i].level;
                }

            }

            if (msg_disc!="")  ShowMessage(msg_disc);

            if (msg_total!="") ShowMessage(msg_total);
}


int CashRegister::LookingForGoodsDB
(
    int Tag
    ,string strEditLine
    ,GoodsInfo *CurGoods
    ,bool priceCheckerIsNull
    ,QString GoodsCode
    ,QString GoodsBarcode
    ,QString GoodsName
    ,QString GoodsPrice
    ,bool LookInsideStatus
)
//in accordance with filter looking for barcode
{
    FiscalCheque FoundGoods;
    GoodsInfo tmpGoods;
    double Price=0;
    string Barcode;
    string Name;
    int Code=0;
    int Group=0;
    string KeyName;
    QString qstrEditLine(strEditLine.c_str());
    int lock_code = GL_NoLock;

    try
    {
        bool err, loc;

        FoundGoods.Clear();

        switch (Tag)
        {
        case PriceFilter:

            Price=GetDoubleNum(strEditLine, &err);//if we search by price then we have to check it

            if ((Price<0.1) || (!err))
            {
                return 1;
            }

            loc=SQLServer->GetGoodsFromDB(&FoundGoods, Code, Barcode, Name, Price);

            if (!loc)//find the first goods with the given price
            {
                return 2;
            }

            KeyName="PRICE";
            CurGoods->InputType = IN_DBPRICE;
            lock_code = GL_FindByPrice;

        break;

/*		case GroupFilter:
            Group=qstrEditLine.toInt(&err);//if we search by group's code then we have to check it
            if ((Group<=0)||(!err))
            {
                return 3;
            }

            DbfMutex->lock();

            loc=GoodsTable->Locate("GRCODE",Group);
            DbfMutex->unlock();

            if (!loc)//find the first goods with the given group
            {
                return 4;
            }

            KeyName="GRCODE";
            CurGoods->InputType = IN_GR;
        break;
*/
        case BarcodeFilter:
            Barcode=strEditLine;
#ifdef PIVO_BOTTLE
    ;;Log("Barcode" + Barcode);
#endif
            loc=SQLServer->GetGoodsFromDB(&FoundGoods, Code, Barcode, Name, Price);

            if (!loc)//find the first goods with the given barcode
            {
                return 5;
            }

            KeyName="BARCODE";
            CurGoods->InputType = IN_DBSCAN;
        break;

        case CodeFilter:
            Code=qstrEditLine.toInt(&err);//if we search by group's code then we have to check it
            if ((Code<=0)||(!err))
            {
                return 6;
            }

            loc=SQLServer->GetGoodsFromDB(&FoundGoods, Code, Barcode, Name, Price);

            if (!loc)//find the first goods with the given group
            {
                return 7;
            }

            KeyName="CODE";
            CurGoods->InputType = IN_DBCODE;
            lock_code = GL_FindByCode;
        break;
/*		case PropertiesFilter:
            CurGoods->InputType = IN_NAME;
            if (GoodsCode.toInt()>0) lock_code = GL_FindByCode;
            else if (!GoodsName.isEmpty()) lock_code = GL_FindByName;
               else if (GoodsPrice.toDouble()>0) lock_code = GL_FindByPrice;

        break;
*/
        }

        Log((char*)(Int2Str(FoundGoods.GetCount()).c_str()));

//printf("FoundGoods.GetSize()=%d\n",FoundGoods.GetSize());

        if (FoundGoods.GetSize()==0)
        {
            return 8;
        }


        int ResPos=-1; // Номер товара в найденном списке, который нужно добавить в продажу

        if (FoundGoods.GetSize()==1)   //if we have found unique goods
        {

            string StrLock = FoundGoods[0].Lock;
            lock_code = 0;

            CurGoods->ItemCount = 1;



            if (GoodsLock(StrLock, GL_TimeLock)==GL_TimeLock) return GL_TimeLock;

            if (lock_code!=GL_NoLock) // надо проверить на запрет
            {

                 CurGoods->LockCode = GoodsLock(StrLock, lock_code);
                if (CurGoods->LockCode!=GL_NoLock) return CurGoods->LockCode;
            }

            ResPos = 0;
        }
        else
        {
            for (int i=0;i<FoundGoods.GetCount();i++)
            {
                GoodsInfo tmpGoods=FoundGoods[i];
                tmpGoods.LockCode = GoodsLock(tmpGoods.Lock, GL_TimeLock);

                if (tmpGoods.LockCode != GL_TimeLock)
                    tmpGoods.LockCode = GoodsLock(tmpGoods.Lock, lock_code);

                FoundGoods.SetAt(i,tmpGoods);
            }
        }


        if (FoundGoods.GetSize()>1 && priceCheckerIsNull)//select goods from the list
        {

            int SelPos;
            PriceSelForm *dlg = new PriceSelForm(NULL);
            dlg->SelRow=&SelPos;
            dlg->ShowGoodsList(&FoundGoods);

            int res=dlg->exec();
            delete dlg;
            if (res==QDialog::Accepted)
            {
                CurGoods->ItemCount = 1;//CashReg->GoodsTable->GetLogicalField("TYPE") ? 1 : 0;

                ResPos = SelPos;
            }
            else
                return 11;
        }



        if (ResPos>=0)
        {
          // Add goods

            if (FoundGoods[ResPos].ItemPrice<epsilon)
            {
                return 1; //неправильная цена товара
            }

            CurGoods->ItemBarcode       = FoundGoods[ResPos].ItemBarcode;

            CurGoods->ItemCode		    = FoundGoods[ResPos].ItemCode;
            CurGoods->ItemName		    = FoundGoods[ResPos].ItemName;
            CurGoods->ItemDBPos		    = -1;
            CurGoods->ItemPrice         = FoundGoods[ResPos].ItemPrice;
            CurGoods->ItemFullPrice	    = CurGoods->ItemPrice;
            CurGoods->ItemCalcPrice 	= CurGoods->ItemFullPrice;
            CurGoods->ItemExcessPrice	= CurGoods->ItemFullPrice;
            CurGoods->ItemFixedPrice	= FoundGoods[ResPos].ItemFixedPrice;
            CurGoods->ItemMinimumPrice  = FoundGoods[ResPos].ItemMinimumPrice;
#ifdef LOGNDS
           CurGoods->NDS               = FoundGoods[ResPos].NDS;
#endif
            CurGoods->ItemPrecision     = FoundGoods[ResPos].ItemPrecision;



            CurGoods->ItemCount     = FoundGoods[ResPos].ItemPrecision;

            if	(FoundGoods[ResPos].ItemMult > .0001)
            {
                CurGoods->ItemCount = FoundGoods[ResPos].ItemMult;
                //CalcCountPrice(CurGoods);
            }
            else
            {
                CurGoods->ItemCount=1;
            }
            CurGoods->CalcSum();

            CurGoods->ItemDiscountsDesc = FoundGoods[ResPos].ItemDiscountsDesc;
            CurGoods->Lock              = FoundGoods[ResPos].Lock;

          return 0;
        }

    }
    catch(cash_error& er)
    {
        LoggingError(er);
    }

    return 12;
}


double CashRegister::GetDoubleNum(string strEditLine,bool* err)//auxililary function
{
    QString GoodsPrice(strEditLine.c_str());
    for (unsigned int i=0;i<GoodsPrice.length();i++)
        if (GoodsPrice.at(i)==',')
            GoodsPrice=GoodsPrice.replace(i,1,".");

    return GoodsPrice.toDouble(err);
}

string CashRegister::GetOrgInfo(int Section)
{
    string res = "";
    //!int Section = GetSection(StrLock);
    if (Section>0)
    {
            int cnt = MsgLevelList.size();
            int curdate = xbDate().JulianDays();
            char buf[9]; buf[8]=0;

            for(int i = 0; i < cnt; i++)
            {

                if(
                    ((Section==1) && (MsgLevelList[i].mode == LOCK_POS_ORG_INFO))
                    ||
                    ((Section==2) && (MsgLevelList[i].mode == LOCK_POS_ORG2_INFO))
                    ||
                    ((Section==3) && (MsgLevelList[i].mode == LOCK_POS_ORG3_INFO))
                    ||
                    ((Section==4) && (MsgLevelList[i].mode == LOCK_POS_ORG4_INFO))
                    ||
                    ((Section==5) && (MsgLevelList[i].mode == LOCK_POS_ORG5_INFO))
                    ||
                    ((Section==6) && (MsgLevelList[i].mode == LOCK_POS_ORG6_INFO))
                   )
                {
                    //! проверка по дате не нужна
                    //! принадлежность товара к секции определяется строкой запретов
//                    sprintf(buf,"%04d%02d%02d",MsgLevelList[i].startdate.tm_year+1900, MsgLevelList[i].startdate.tm_mon+1, MsgLevelList[i].startdate.tm_mday);
//                    if(curdate < xbDate(buf).JulianDays()) continue;
//                    sprintf(buf,"%04d%02d%02d",MsgLevelList[i].finishdate.tm_year+1900, MsgLevelList[i].finishdate.tm_mon+1, MsgLevelList[i].finishdate.tm_mday);
//                    if(curdate > xbDate(buf).JulianDays()) continue;

                    res = MsgLevelList[i].msg;

//				    ;;Log((char*)res.c_str());

                    break;
                }
            }
    }
    return res;
}

int CashRegister::GetSection(string StrLock)
{
    int res = 0;
    if ((StrLock.size()>=LOCK_POS_ORG_INFO) && (StrLock[LOCK_POS_ORG_INFO-1]=='.'))
    {
         res = 1;
    }
    else
        if ((StrLock.size()>=LOCK_POS_ORG2_INFO) && (StrLock[LOCK_POS_ORG2_INFO-1]=='.'))
        {
             res = 2;
        }
        else
            if ((StrLock.size()>=LOCK_POS_ORG3_INFO) && (StrLock[LOCK_POS_ORG3_INFO-1]=='.'))
            {
                 res = 3;
            }
            else
                if ((StrLock.size()>=LOCK_POS_ORG4_INFO) && (StrLock[LOCK_POS_ORG4_INFO-1]=='.'))
                {
                     res = 4;
                }

                else
                    if ((StrLock.size()>=LOCK_POS_ORG5_INFO) && (StrLock[LOCK_POS_ORG5_INFO-1]=='.'))
                    {
                         res = 5;
                    }
                    else
                        if ((StrLock.size()>=LOCK_POS_ORG6_INFO) && (StrLock[LOCK_POS_ORG6_INFO-1]=='.'))
                        {
                             res = 6;
                        }

    return res;
}


string CashRegister::GetMessageInfo(int num)
{
    string res = "";

    if (num>0)
    {
            int cnt = MsgLevelList.size();
            int curdate = xbDate().JulianDays();
            char buf[9]; buf[8]=0;

            for(int i = 0; i < cnt; i++)
            {

                if(MsgLevelList[i].mode == num)
                {
                    sprintf(buf,"%04d%02d%02d",MsgLevelList[i].startdate.tm_year+1900, MsgLevelList[i].startdate.tm_mon+1, MsgLevelList[i].startdate.tm_mday);
                    if(curdate < xbDate(buf).JulianDays()) continue;
                    sprintf(buf,"%04d%02d%02d",MsgLevelList[i].finishdate.tm_year+1900, MsgLevelList[i].finishdate.tm_mon+1, MsgLevelList[i].finishdate.tm_mday);
                    if(curdate > xbDate(buf).JulianDays()) continue;

                    res = MsgLevelList[i].msg;

                    ;;Log((char*)res.c_str());

                    break;
                }
            }
    }
    return res;
}

void CashRegister::CalcCertificateDiscounts(FiscalCheque* CurCheque, double sum)
{
// + dms ====================/
        //распределяем суммовую скидку и скидку по сертификатам по всему чеку
        //double BaseDiscountSum  = CurCheque->CertificateSumm;
        double BaseDiscountSum  = sum;
        double BaseCheckSum     = CurCheque->GetSum();
        double AllocDiscountSum = 0;
        double AllocCheckSum    = 0;
        double CurDiscount      = 0;

        for (int i=0;i<CurCheque->GetCount();i++)
        {
            GoodsInfo crGoods = (*CurCheque)[i];

            if (((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT)) || (crGoods.ItemCode==TRUNC) || (crGoods.ItemCode==EXCESS_PREPAY) ) continue;

            if (BaseCheckSum - AllocCheckSum > 0)
                CurDiscount = RoundToNearest((BaseDiscountSum - AllocDiscountSum)/(BaseCheckSum - AllocCheckSum)*crGoods.ItemSum, 0.01);
            else
                CurDiscount = 0;

            //+ dms ===== сумма без учета скидки по сертификату
            //crGoods.ItemFullDiscSum += CurDiscount;
            //- dms =====/

            AllocCheckSum += crGoods.ItemSum;

            if (crGoods.ItemSum	> CurDiscount)
                crGoods.ItemSum	-= CurDiscount;

            else
            {
                CurDiscount = crGoods.ItemSum;
                crGoods.ItemSum	= 0;
            }

            AllocDiscountSum += CurDiscount;

            CurCheque->SetAt(i,crGoods);
        }
// - dms ====================/

}

string CashRegister::GetRoundedWithDot(double num,double prec)//as above but function return formated string
{
    string str = GetRounded(num,prec);
    for (int i=0;i<str.length();i++) //replace dot symbol by comma
        if (str[i]==',')
            str[i]='.';

    return str;
}

// ret - возврат (сумму подаем со знаком минус)
// cancel - отмена последней операции, суть - тот же возврат, но номер чека должен быть уникальный
int CashRegister::CheckEGAIS(FiscalCheque* CurCheque, bool ret, bool cancel)
{

    string strTip = "ПРОДАЖА";
    if (cancel) strTip = "ВОЗВРАТ";
    ;;Log("[Func] CheckEGAIS "+strTip+" [Begin]");

    int res = -1;

    int type = 0;
    if (ret) type = 1;
    if (cancel) type = 2;

    int k = 1;

    do {

        (*CurCheque).egais_url = "";
        (*CurCheque).egais_sign = "";

        string CheckNumStr = "";
        if (cancel)
            CheckNumStr = ("1" + Int2Str(k) + Int2Str(GetLastCheckNum()+1));
        else
            CheckNumStr = Int2Str(GetLastCheckNum()+1);

        string ip_server = EgaisSettings.IP;
        string bottle= "";
        string tmpFileName= "/Kacca/egais/tmp_alco.xml";
        string resFileName= "/Kacca/egais/alco_"+CheckNumStr+".xml";

        string egaisFileName = "/Kacca/egais/alco_"+CheckNumStr+"_egais.xml";

        //;;Log("[CheckEGAIS] Step 1");

        int cnt=0;
        for(unsigned int i=0;i<CurCheque->GetCount();i++)
        {
            if (((*CurCheque)[i].ItemFlag==SL)||((*CurCheque)[i].ItemFlag==RT))
            {
                int AlcoCnt = (*CurCheque)[i].Actsiz.size();

                if ((*CurCheque)[i].ItemAlco && AlcoCnt)
                {
                    double price = (*CurCheque)[i].ItemFullPrice;
                    if (ret) price=-price; //Возврат

                    for (int j=0;j<AlcoCnt;j++)
                    {
                        bottle += bottle!=""?"\n":"";
                        bottle +="<Bottle barcode=\\\""+(*CurCheque)[i].Actsiz[j]+"\\\"";
                        bottle +=" ean=\\\""+(*CurCheque)[i].ItemBarcode+"\\\"";
                        bottle +=" price=\\\""+GetRoundedWithDot(price,0.01)+"\\\"";
                        bottle +=" volume=\\\"\\\" ";
                        bottle +=" />";
                    }

                    //;;ShowMessage((*CurCheque)[i].ItemName);
                    cnt++;
                }

            }
        }

        //;;Log("[CheckEGAIS] Step 2");

        if (!cnt){
            ;;Log("[CheckEGAIS] Нет алкоголя. Выход.");
             return 0; // нет алкогольной продукции, проверка не нужна
        }


        if(ip_server=="")
        {
            ShowMessage("Ошибка! Не задан IP-адрес модуля ЕГАИС!");
            return -2;
        }


        string logstr_start, logstr_end;

        // Проверка в ЕГАИС
        char s[255];
        time_t tt = time(NULL);
        tm* t = localtime(&tt);
        //sprintf(s, "%02d%02d%02d%02d%02d%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        sprintf(s, "%02d%02d%02d%02d%02d", t->tm_mday, t->tm_mon + 1, (t->tm_year + 1900)-2000, t->tm_hour, t->tm_min);
        string dt = s;

        // Временная метка
        char dtstr_start[256];
        sprintf(dtstr_start, "%02d.%02d.%4d %02d:%02d:%02d> ",t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        logstr_start = dtstr_start;

    //;;Log("[CheckEGAIS] Step 3");

    //    string cmd = "sed ";
    //    cmd+=" -e 's/&INN&/"+EgaisSettings.INN+"/g'";
    //    cmd+=" -e 's/&KASSA&/"+EgaisSettings.KKMNO+"/g'";
    //    cmd+=" -e 's/&KASSA&/"+EgaisSettings.KKMNO+"/g'";
    //    cmd+=" -e 's/&KASSA&/"+EgaisSettings.KKMNO+"/g'";
    //
    //    cmd+=" -e 's/&KASSA&/"+EgaisSettings.KKMNO+"/g'";
    //    cmd+=" -e 's/&CHECK&/"+CheckNumStr+"/g'";
    //    cmd+=" -e 's/&SMENA&/"+Int2Str(GetKKMSmena())+"/g'";
    //    cmd+=" -e 's/&DATATIME&/"+dt+"/g'";
    //    cmd+=" -e 's/&BOTTLE&/"+bottle+"/g'";
    //    cmd+=" "+tmpFileName+" > "+resFileName;


        string cmd = "";
        cmd+= "echo \"<?xml version=\\\"1.0\\\" encoding=\\\"UTF-8\\\"?>\" > "+resFileName+";";
        cmd+= "echo \"<Cheque\" >> "+resFileName+";";
        cmd+= "echo \"inn=\\\""+EgaisSettings.INN+"\\\"\" >> "+resFileName+";";
        cmd+= "echo \"kpp=\\\""+EgaisSettings.KPP+"\\\"\" >> "+resFileName+";";
        cmd+= "echo \"datetime=\\\""+dt+"\\\"\" >> "+resFileName+";";
        cmd+= "echo \"address=\\\""+EgaisSettings.Adress+"\\\"\" >> "+resFileName+";";
        cmd+= "echo \"name=\\\""+EgaisSettings.OrgName+"\\\"\" >> "+resFileName+";";
        cmd+= "echo \"kassa=\\\""+EgaisSettings.KKMNO+"\\\"\" >> "+resFileName+";";
        cmd+= "echo \"shift=\\\""+Int2Str(GetKKMSmena())+"\\\"\" >> "+resFileName+";";
        cmd+= "echo \"number=\\\""+CheckNumStr+"\\\"\" >> "+resFileName+";";
        cmd+= "echo \">\" >> "+resFileName+";";
        cmd+= "echo \""+bottle+"\" >> "+resFileName+";";
        cmd+= "echo \"</Cheque>\" >> "+resFileName+";";

        cmd+=" curl -F 'xml_file=@"+resFileName+"' http://"+ip_server+"/xml > "+egaisFileName;

        //cmd+="; echo \"\n</Cheque>\" >> "+resFileName;

    //;;Log("[CheckEGAIS] Step 4");

        //char cmd[256] = "sed -i 's/&BOTTLE&/"+bottle+"/g' /Kacca/egais/tmp_alco.xml >> /Kacca/egais/alco_"+Int2Str(GetLastCheckNum()+1)+".xml").c_str();
        char cmdcpy[256];
        int retsys;
        //qApp->setOverrideCursor(QApplication::waitCursor);
        FILE* f;

        QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
        QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
        sm.setFixedSize(250,100);
        msg.setText(W2U("Проверка ЕГАИС... "+Int2Str(k)));
        msg.setFixedSize(250,100);
        msg.setAlignment( QLabel::AlignCenter );
        sm.show();
        thisApp->processEvents();

    //;;Log("[CheckEGAIS] Step 5");

    /*
        argv[0] = cmd;//"emvgate";
        for(int i = 1; argv[i]; i++){
            strcat(cmd, " ");
            strcat(cmd, argv[i]);
        }
        chdir("/Kacca/egate");

        //FILE *lf;
        const char* tmpbuf;

        char logfile[]="./.egate.log";
        strcpy(cmdcpy,cmd);
        f = fopen(logfile, "a");
        tmpbuf=CashReg->GetCurDate().c_str();
        fwrite(tmpbuf,10,1,f);
        fprintf(f, " ");
        tmpbuf=CashReg->GetCurTime().c_str();
        fwrite(tmpbuf,8,1,f);
        fprintf(f, "> =KKM= start =\n");
    //	fprintf(f,"%s -- ",cmdcpy+(strchr(cmdcpy,' ')-cmdcpy));
        fclose(f);

    */
        //;;Log((char*)cmd.c_str());
        Log("curl -F 'xml_file=@"+resFileName+"' http://"+ip_server+"/xml > "+egaisFileName);
        //;;printf("%s", cmd.c_str());

    //;;Log("[CheckEGAIS] Перед запуском команды CMD");

        //! start !//
        retsys=system((char*)cmd.c_str());

    //;;Log("[CheckEGAIS] После запуска команды CMD");

        // Лог

        tt = time(NULL);
        t = localtime(&tt);
        char dtstr_end[256];
        sprintf(dtstr_end, "%02d.%02d.%4d %02d:%02d:%02d> ",t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);

        logstr_end = dtstr_end;


        string logEgais = "/Kacca/egais/egais.log";

        cmd = "";
        cmd += "echo \""+logstr_start+" ===Start #"+CheckNumStr+" ===\" >> "+logEgais+";";
        //cmd += "echo \""+logstr_start+" \" >> egais.log;";
        cmd += "cat "+resFileName+" >> "+logEgais+";";
        //cmd += "echo \""+logstr_start+" \" >> egais.log;";
        cmd += "echo \""+logstr_end+" === Result #"+CheckNumStr+" ===\" >> "+logEgais+";";
        //cmd += "echo \""+logstr_end+" \" >> egais.log;";
        cmd += "cat "+egaisFileName+" >> "+logEgais+";";
        cmd += "echo \""+logstr_end+" \" >> "+logEgais+";";
        cmd += "echo \""+logstr_end+" === End #"+CheckNumStr+" ===\" >> "+logEgais+";";
        cmd += "echo \"  \" >> "+logEgais+";";
        cmd += "echo \"  \" >> "+logEgais+";";

    //;;Log((char*)cmd.c_str());


    //;;Log("[CheckEGAIS] before log");

        //! start !//
        retsys = system((char*)cmd.c_str());

    //;;Log("[CheckEGAIS] after log");
    /*
        f = fopen(logfile, "a");
        tmpbuf=CashReg->GetCurDate().c_str();
        fwrite(tmpbuf,10,1,f);
        fprintf(f, " ");
        tmpbuf=CashReg->GetCurTime().c_str();
        fwrite(tmpbuf,8,1,f);
        fprintf(f, "> =KKM= finish (%d) =\n",retsys);
        fprintf(f, "\n",retsys);
        fclose(f);
    */

        chdir("/Kacca");
    //	LoggingUpos();
        //FILE* f;

        //!
        //egaisFileName ="/Kacca/egais/alco_egais.xml";

    //;;Log("[CheckEGAIS] Step 6");

        char * fn = (char*)egaisFileName.c_str();
        f = fopen(fn,"r");
        if(!f) return -1;
        //int e;
        //fscanf(f,"%d ",&e);
        char sMsg[2048];
        fgets(sMsg, 2048, f);

        if(sMsg){
        //	QMessageBox::information( NULL, "APM KACCA", W2U(sMsg), "Ok");
        }

        string str = sMsg;

        string sign, url;

        sign="";
        url="";


    //;;Log("[CheckEGAIS] Step 7");

        int begpos = str.find("<url>");
        int lastpos = str.find("</url>");

        if ((begpos!=string::npos) && (lastpos!=string::npos))
            url = str.substr(begpos+5, lastpos-1 - begpos-5);

        begpos = str.find("<sign>");
        lastpos = str.find("</sign>");

        if ((begpos!=string::npos) && (lastpos!=string::npos))
            sign = str.substr(begpos+6, lastpos-1 - begpos-6);

        if (!(url.empty()) && !(sign.empty())){
            res=0;
            (*CurCheque).egais_url = url;
            (*CurCheque).egais_sign = sign;

            Log((char*)("egais_url="+url).c_str());
            Log((char*)("egais_sign="+sign).c_str());

        }
        else{

            res=1;
        }

        //!AddActsizOperation(CurCheque, type, k, res, str);

        k++;

    } while (cancel && (k<=3) && (res!=0));

    //QMessageBox::information( NULL, "APM KACCA", W2U(url), "Ok");
    //QMessageBox::information( NULL, "APM KACCA", W2U(sign), "Ok");

    ;;Log("[Func] CheckEGAIS [End]");

    return res;
}


string CashRegister::GetCheckEgaisHeader()
{
    string str="";

    // Проверка в ЕГАИС
    char s[255];
    time_t tt = time(NULL);
    tm* t = localtime(&tt);
    //sprintf(s, "%02d%02d%02d%02d%02d%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
    sprintf(s, "%02d.%02d.%02d %02d:%02d", t->tm_mday, t->tm_mon + 1, (t->tm_year + 1900)-2000, t->tm_hour, t->tm_min);
    string dt = s;

    str+=GetFormatString("ИНН:"+EgaisSettings.INN, "КПП:"+EgaisSettings.KPP, 34);
    str+="\n"+GetFormatString("КАССА:"+EgaisSettings.KKMNO, "СМЕНА:"+Int2Str(GetKKMSmena()), 34);
    str+="\n"+GetFormatString("ЧЕК:"+Int2Str(GetLastCheckNum()+1), "ДАТА:"+dt, 34);

    //str+="\nРежим="+Int2Str(EgaisSettings.ModePrintQRCode);

    str+=" ";

    return str;
}


int CashRegister::CheckCigarBarcode(string Actsiz, FiscalCheque* CurCheque)
{
    int err=0;

    //FiscalCheque* CurCheque = Cheque;
    if (!CurCheque) return -1;

    for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo g=(*CurCheque)[i];

        if ((g.ItemFlag==SL) || (g.ItemFlag==RT))
        {
            for (int j=0;j<g.Actsiz.size();j++)
               if(Actsiz==g.Actsiz[j]){
                    ShowMessage("ПОВТОР! Данный штрихкод уже зарегистрирован в чеке!");
                    err=1;
                    break;
               }
        }
    }

    return err;
}

int CashRegister::CheckActsizBarcodeDataMatrix(
        string *ActsizBarcode,
        string msg,
        FiscalCheque* CurCheque,
        vector<string> ActsizList,
        int ActsizType
)
{

    int err=0;

//    bool oldscannersuspend=Scanner->suspend;
//    Scanner->suspend = false;

    string res="";
    string Actsiz="";
    ActsizCard=new ActsizForm(this, msg);
    ActsizCard->TitleLabel->setProperty( "text", W2U("ПРОДАЖА МАРКИРОВАННОЙ ПРОДУКЦИИ"));
    ActsizCard->TLabel->setProperty( "text", W2U("Считайте маркировку DataMatrix"));



    int resdlg;

    try
    {
		bool isExit=false;
        do{
            if (ActsizCard->exec()==QDialog::Accepted)
            {

                //Barcode=U2W(BarcodeCard->EditLine->text().upper());
                Actsiz=U2W(ActsizCard->BarcodeEdit->text());
#ifdef PIVO_BOTTLE  ///Провекра символа разделителя в шаблоне соответствующей группы товаров				
//;;Log("Actsiz=" + Actsiz + " Vivod symbol=" + Actsiz[25]); //проверка марки пива
//;;Log("Actsiz=" + Actsiz + " Vivod symbol=" + Actsiz[24]); Продажа молочной продукции
//;;Log("Actsiz=" + Actsiz + " Vivod symbol=" + Actsiz[26]); Продажа беларусских товаров
;;Log("Actsiz=" + Actsiz + " Vivod symbol=" + Actsiz[31]); //Продажа товаров по воде
//;;Log("Actsiz=" + Actsiz + " Vivod symbol=" + Actsiz.substr(Actsiz.length()-4, 4)); //Продажа разливного пива
//;;Log("Actsiz=" + Actsiz + " Vivod symbol=" + Actsiz[25]); //Продажа разливного пива
#endif
                if(!Actsiz.empty())
                {

                    int ResMatch=-1;
                    ;;Log("Actsiz.length()="+Int2Str(Actsiz.length()));
					

                    switch (ActsizType) {
                        case AT_MILK : {
                            if (Actsiz.length()>=20) 
							{

                                Log("Молоко или вода датаматрикс");

								if (
                                ((Actsiz.length()>=30) && (Actsiz.length()<=55) || (Actsiz.length()==83))
                                )
                                 ResMatch=1;
                                else Log("Actsiz Length >=40 or <= 30  ResMatch =" + Int2Str(ResMatch)); 

                                char spec = 0x1D;
                                Log("Actsiz Length="+Int2Str(Actsiz.length())+" Actsiz=" + Actsiz); 
#ifdef MILK_AND_WATER
								if((Actsiz[24]==spec) || (Actsiz.length()==30))
                                {
									;;Log("Зашли в молочную продукцию");
									
									;;Log("TestActsiz Milk=" + Int2Str(ActsizType));
									
                                    ///Проверяем на символ разделителя, при его отсутсвии добавляем
                                    if (Actsiz[24]!=spec)
                                    {
                                        ;;Log("Молочная продукция - Origin Actsiz DataMatrix (30)="+Actsiz);
                                        Actsiz = Actsiz.substr(0, 24) + spec + Actsiz.substr(24, 6);
                                    }
                                    else ;;Log("Молочная продукция - Origin Actsiz DataMatrix (31)="+Actsiz);
                                    
                                }	
#else
								if (Actsiz.length()==30) 
								{

                                    // Добавляем спецсимвол
                                    ;;Log("Origin Actsiz DataMatrix (30)="+Actsiz);
                                    //char spec = 0x1D;
                                    Actsiz = Actsiz.substr(0, 24) + spec + Actsiz.substr(24, 6);
                                }	
#endif
								
#ifdef PIVO_BOTTLE
                                else if((Actsiz[25]==spec) || (Actsiz.length()==31))
                                {
									;;Log("Зашли в бутылочное пиво");
									
									;;Log("TestActsiz PivoBottle=" + Int2Str(ActsizType));

                                    ///Проверяем на символ разделителя
                                    if (Actsiz==MARKINGACTSIZ)
                                    {
                                        ;;Log("MARKINGACTSIZ=111111111111111111111111111111");
                                        Actsiz = "";
										ResMatch = 2;
                                    }
                                    else
                                    {
                                        // Добавляем спецсимвол                                      
                                        //char spec = 0x1D;
                                        if (Actsiz[25]!=spec)
                                        {
                                            ;;Log("Пиво в бутылке или банке - Origin Actsiz DataMatrix (31)="+Actsiz);
                                            Actsiz = Actsiz.substr(0, 25) + spec + Actsiz.substr(25, 6);
                                        }
                                        else ;;Log("Пиво в бутылке или банке - Origin Actsiz DataMatrix (32)="+Actsiz);
                                    }
                                }
#endif

#ifdef MILK_AND_WATER
								else if((Actsiz[26]==spec) || (Actsiz.length()==32))
                                {
									;;Log("Зашли в молочную продукцию");
									;;Log("TestActsiz Type Belarus=" + Int2Str(ActsizType));
                                    ///Проверяем на символ разделителя, при его отсутсвии добавляем
                                    if (Actsiz[26]!=spec)
                                    {
                                        ;;Log("Молочная продукция - Origin Actsiz DataMatrix (30)="+Actsiz);
                                        Actsiz = Actsiz.substr(0, 26) + spec + Actsiz.substr(26, 6);
                                    }
                                    else ;;Log("Молочная продукция - Origin Actsiz DataMatrix (31)="+Actsiz);
                                    
                                }	
#else
								else if (Actsiz.length()==32) 
								{

                                    // Добавляем спецсимвол
                                    ;;Log("Origin Actsiz DataMatrix (32)="+Actsiz);
                                    //char spec = 0x1D;
                                    Actsiz = Actsiz.substr(0, 26) + spec + Actsiz.substr(26, 6);
                                }
#endif

#ifdef MILK_AND_WATER
								 else if((Actsiz[31]==spec) || (Actsiz.length()==37))
								 {
									;;Log("Зашли в продукцию воды");
									;;Log("TestActsiz Water=" + Int2Str(ActsizType));
                                    ///Проверяем на символ разделителя, при его отсутсвии добавляем
                                    if (Actsiz[31]!=spec)
                                    {
                                        ;;Log("Продукция воды - Origin Actsiz DataMatrix (31)="+Actsiz);
                                        Actsiz = Actsiz.substr(0, 31) + spec + Actsiz.substr(31, 6);
                                    }
                                    else ;;Log("Продукция воды - Origin Actsiz DataMatrix (31)="+Actsiz);
                                }
#else
								 else if (Actsiz.length()==37) 
								 {

                                    if (Actsiz[31]!=spec) 
									{
                                        // Добавляем спецсимвол
                                        ;;Log("Origin Actsiz DataMatrix (38)="+Actsiz);
                                        //char spec = 0x1D;
                                        Actsiz = Actsiz.substr(0, 31) + spec + Actsiz.substr(31, 6);
                                    }
                                }
#endif
								
#ifdef BIODOBAVKA
                                else if(Actsiz.length()==83)
                                {
									;;Log("TestActsiz Biodobavka=" + Int2Str(ActsizType));

                                        // Добавляем спецсимвол
                                        ;;Log("Origin Actsiz DataMatrix (31)="+Actsiz);
                                        //char spec = 0x1D;
                                    Actsiz = Actsiz.substr(0, 31) + spec
                                            + Actsiz.substr(31, 6) + spec + Actsiz.substr(37, 46);
                                }
#endif

                                else

                                {
                                    //ResMatch = -1;
                                    ;;Log("ERROR Actsiz DataMatrix length ("
                                          + Int2Str(Actsiz.length()) + ")");

                                }
                            }
                            break;
                        }
                        case AT_TEXTILE : {
                            if (Actsiz.length()>=20) {

                                Log("Датаматрикс текстиль");

                                // Шаблон акциза (взят из ответа ЕГАИС)
                                //QRegExp rx("\\d\\d[a-zA-Z0-9]{21}\\d[0-1]\\d[0-3]\\d{10}[a-zA-Z0-9]{31}");

                                //ResMatch = rx.match(W2U(Actsiz)); // проверка на шаблон

                                if (
                                (Actsiz.length()==20)
                                ||
                                ((Actsiz.length()>=83) && (Actsiz.length()<=87))
                                )
                                 ResMatch=1;
                            }
                            break;
                        }
                        case AT_CIGAR : {
                            if (Actsiz.length()==29) {
                                Log("Датаматрикс сигареты");
                                ResMatch=1;
                            }
                            break;
                        }

#ifdef ALCO_VES
 ///Пробую сделать проверку по типу марки не по длине
                        case AT_PIVO_RAZLIV:{
							;;Log("TestActsiz=" + Int2Str(ActsizType));
						
						if (Actsiz.substr(Actsiz.length()-4, 4) == ADDMARKINGACTSIZ)
						{
							;;Log("Датаматрикс пиво на разлив");
							char spec = 0x1D;
							
							;;Log("Aсtsiz =" + Actsiz);
							Actsiz = Actsiz.substr(0,Actsiz.length()-4);
							;;Log("Aсtsiz posle obrabotkee=" + Actsiz);
							if(Actsiz[25] != spec)
							{
								Actsiz = Actsiz.substr(0, 25) + spec + Actsiz.substr(25, 8);
								;;Log("PivoRazliv-Origin Actsiz DataMatrix (34)=" + Actsiz);
								ResMatch=2;
							}
							else
							{
								;;Log("Origin Actsiz DataMatrix (35)=" + Actsiz);
                                ResMatch=2;
							}
							
							
						}
 /*                         
 
							if ((Actsiz.length()==36) || (Actsiz.length()==46))
                            {
                                Log("Датаматрикс пиво на разлив");
                                Log("Aсtsiz obrabotka=" + Actsiz);
                                //char spec = 0x1D;
                                Actsiz = Actsiz.substr(0, 25) + Actsiz.substr(25, 8);
                                //SQLServer->SendSalesPivoRazliv(Actsiz);
                                //Actsiz = Actsiz.substr(0,32);
                                ;;Log("Origin Actsiz DataMatrix (33)="+Actsiz);
                                ResMatch=2;
                            }
                            else if ((Actsiz.length()==35) || (Actsiz.length()==45))
                            {
                                Log("Датаматрикс пиво на разлив Возврат");
                                Log("ACtsiz obrabotka=" + Actsiz);
                                char spec = 0x1D;
                                Actsiz = Actsiz.substr(0, 25) + spec + Actsiz.substr(25, 8);
                                //SQLServer->SendSalesPivoRazliv(Actsiz);
                                //Actsiz = Actsiz.substr(0,32);
                                ;;Log("Origin Actsiz DataMatrix (35)="+Actsiz);
                                ResMatch=2;
                            }

*/
                        }
#endif
                    }

                    //;;Log((char*)("Actsiz DataMatrix="+Actsiz).c_str());
                    //;;Log("Actsiz DataMatrix="+Actsiz + " Actsiz Length="+Int2Str(Actsiz.length()));
                    //;;Log((char*)("ResMatch="+Int2Str(ResMatch)).c_str());

                    if (ResMatch >= 0)
                    {
                        // Считали акциз

                        // Проверка на повтор
                        bool isOk=true;

                        // Проверка на повтор в текущем наборе
                        if (ActsizList.size()>0)
                        {
                            for (int j=0;j<ActsizList.size();j++)
                               if(Actsiz==ActsizList[j]){
                                    isOk=false;
                                    break;
                               }
                        }

                        // Проверка на повтор в текущем чеке
                        if (isOk)
                        {
                            //FiscalCheque* CurCheque = Cheque;
                            if (!CurCheque) return -1;

                            for (int i=0;i<CurCheque->GetCount();i++)
                            {
                                GoodsInfo g=(*CurCheque)[i];

                                if ((g.ItemFlag==SL) || (g.ItemFlag==RT))
                                {
                                    for (int j=0;j<g.Actsiz.size();j++)
                                       if(Actsiz==g.Actsiz[j]){
                                            isOk=false;
                                            break;
                                       }
                                }

                                if (!isOk) break;
                            }
                        }

#ifdef ALCO_VES
                        ;;Log((char*)("Resmath + test="+Int2Str(ResMatch)).c_str());
						;;Log("Actsiz = " + Actsiz);
                        if (ResMatch==2) isOk=true;
#endif

                        if(isOk)
                        {
                            err=0;
                            res = Actsiz;
                            isExit=true;
                        }
                        else
                        {
                            // Это не акциз, продолжаем считывание
                            err = ERRORACTSIZ;
                            isExit=false;
                            ActsizCard->StatusLabel->setText(W2U("ПОВТОР! Данный штрихкод уже зарегистрирован в чеке!"));
                        }
                    }
                    else
                    {

                        if ( (ActsizType==AT_MILK)&&(Actsiz==SERVICE_EAN_MILK) ) {
                            err=0;
                            res = Actsiz;
                            isExit=true;
                        } else {
                        // Это не акциз, продолжаем считывание
                            err = ERRORACTSIZ;
                            isExit=false;
                            ActsizCard->StatusLabel->setText(W2U("ОШИБКА! Неправильный штрихкод акцизной марки!"));
                        }
                    }
                }
                else
                {
                    ActsizCard->StatusLabel->setText(W2U("ОШИБКА! Акцизная марка не считана!"));
                }
            }
            else
            {
                // Нажата кнопка отмены
                err = ERRORACTSIZ;
                isExit=true;
            }
        }
        while (!isExit);
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }

    //Scanner->suspend = oldscannersuspend;


    if (ActsizCard!=NULL)
    {
        delete ActsizCard;
        ActsizCard=NULL;
    }

//;;char log_str[100];
//;;sprintf(log_str, " Акциз #%s ", res.c_str());
//;;Log(log_str);

    *ActsizBarcode = res;

    return err;
}


int CashRegister::CheckActsizBarcode(string *ActsizBarcode, string msg, FiscalCheque* CurCheque, vector<string> ActsizList)
{
    int err=0;

//    bool oldscannersuspend=Scanner->suspend;
//    Scanner->suspend = false;

    string res="";
    string Actsiz="";
    ActsizCard=new ActsizForm(this, msg);

    int resdlg;

    try
    {
        bool isExit=false;
        do{
            if (ActsizCard->exec()==QDialog::Accepted)
            {
                //Barcode=U2W(BarcodeCard->EditLine->text().upper());
                Actsiz=U2W(ActsizCard->BarcodeEdit->text());
                if(!Actsiz.empty()){

                    int ResMatch=0;
                    ;;Log("Actsiz.length()="+Int2Str(Actsiz.length()));
                    if (Actsiz.length()>=150) {
                        // Новый формат акциза с 01.07.2018г
                        Log("NEW ACTSIZ SINCE 01.06.2018");
                        Actsiz = Actsiz.substr(0,150);
                    }
                    else
                    {
                        Log("OLD ACTSIZ");
                        // Старый формат акциза до 01.07.2018г
                        if (Actsiz.substr(0,8)=="F3002400")
                        {
                            // если впереди акциза есть префикс F3002400, то ставим его в конец
                            // чтобы успешно прошла проверка на шаблон
                            Actsiz = Actsiz.substr(8,Actsiz.length()-8) + Actsiz.substr(0,8);
                        }


                        // Шаблон акциза (взят из ответа ЕГАИС)
                        QRegExp rx("\\d\\d[a-zA-Z0-9]{21}\\d[0-1]\\d[0-3]\\d{10}[a-zA-Z0-9]{31}");

                        ResMatch = rx.match(W2U(Actsiz)); // проверка на шаблон

                        if (Actsiz.length()>70) ResMatch=-1;
                    }

                    ;;Log((char*)("Actsiz="+Actsiz).c_str());
                    ;;Log((char*)("ResMatch="+Int2Str(ResMatch)).c_str());

                    if (ResMatch >= 0)
                    {
                        // Считали акциз

                        // Проверка на повтор
                        bool isOk=true;

                        // Проверка на повтор в текущем наборе
                        if (ActsizList.size()>0)
                        {
                            for (int j=0;j<ActsizList.size();j++)
                               if(Actsiz==ActsizList[j]){
                                    isOk=false;
                                    break;
                               }
                        }

                        // Проверка на повтор в текущем чеке
                        if (isOk)
                        {
                            //FiscalCheque* CurCheque = Cheque;
                            if (!CurCheque) return -1;

                            for (int i=0;i<CurCheque->GetCount();i++)
                            {
                                GoodsInfo g=(*CurCheque)[i];
								Log("CurCheck = =");

                                if ((g.ItemFlag==SL) || (g.ItemFlag==RT))
                                {
									
                                    for (int j=0;j<g.Actsiz.size();j++){
                                       Log("ActsizReturn=" + g.Actsiz[j]);
									   if(Actsiz==g.Actsiz[j]){
										   isOk=false;
                                            break;
                                       }
									}
                                }

                                if (!isOk) break;
                            }
                        }

                        if (isOk)
                        {
                            err=0;
                            res = Actsiz;
                            isExit=true;
                        }
                        else
                        {
                            // Это не акциз, продолжаем считывание
                            err = ERRORACTSIZ;
                            isExit=false;
                            ActsizCard->StatusLabel->setText(W2U("ПОВТОР! Данный штрихкод уже зарегистрирован в чеке!"));
                        }
                    }
                    else
                    {
                        // Это не акциз, продолжаем считывание
                        err = ERRORACTSIZ;
                        isExit=false;
                        ActsizCard->StatusLabel->setText(W2U("ОШИБКА! Неправильный штрихкод акцизной марки!"));
                    }
                }
                else
                {
                    ActsizCard->StatusLabel->setText(W2U("ОШИБКА! Акцизная марка не считана!"));
                }
            }
            else
            {
                // Нажата кнопка отмены
                err = ERRORACTSIZ;
                isExit=true;
            }
        }
        while (!isExit);
    }
    catch(cash_error& er)
    {
        DbfMutex->unlock();
        LoggingError(er);
    }

    //Scanner->suspend = oldscannersuspend;


    if (ActsizCard!=NULL)
    {
        delete ActsizCard;
        ActsizCard=NULL;
    }

//;;char log_str[100];
//;;sprintf(log_str, " Акциз #%s ", res.c_str());
//;;Log(log_str);
    if (filepath_exists("/Kacca/tmp/UTM_is_owner")) {
        QString qres = QString::fromAscii(res.data(), res.size());
        if (!UTM_is_owner(&qres))
        {
            ShowMessage("Товар с данной акцизной маркой нельзя продать через эту кассу");
            CurrentWorkPlace->EditLine->clear();
            *ActsizBarcode = "";
            return ERRORACTSIZ;
        }
    }
    *ActsizBarcode = res;

    return err;
}

void CashRegister::GetBonusInfo(string card)//close the given check and try to send
{

//    //BNS->exec("inf", card, CurCheque, CurCheque->GetSum());
//
//
//    ;;Log((char*)("BNS->GetCardInfo(card = "+card+")").c_str());
//
//    BNS_RET* ret = BNS->exec("inf&reg", card);
//
//    ;;Log((char*)Int2Str(ret->errorCode).c_str());l


}

void CashRegister::AddBonus(long long Lcard, FiscalCheque* CurCheque)//close the given check and try to send
{

//    if(Lcard>0LL){
//
//
//        string card = LLong2Str(Lcard);
//
//        //BNS->exec("inf", card, CurCheque, CurCheque->GetSum());
//        string strCheck = Int2Str(GetLastCheckNum()+1);
//
//        //;;Log((char*)("BNS->GetCardInfo(card = "+card+")").c_str());
//
//
//        BNS_RET* ret = BNS->exec("inf", card, strCheck);
//
//        double d_sum = 0;
//
//        if (ret->ActiveBonuses>1) d_sum = ret->ActiveBonuses*5/100;
//
//        //;;Log((char*)Int2Str(ret->errorCode).c_str());
//
//    //    ret = BNS->Payment(card, CurCheque);
//
//        ret = BNS->exec("sale&init", card, strCheck, CurCheque, CurCheque->GetDiscountSum(), d_sum);
//
//        ;;Log((char*)Int2Str(ret->errorCode).c_str());
//        ;;Log((char*)ret->id.c_str());
//    }

}


bool CashRegister::BonusMode(long long card)
{
    bool res = false ;

    if (BNS!=NULL){

        if (BNS->UseBNS)
        {

            int isBonus=0;
            if(CurrentWorkPlace)
            {
                // Временно используем незадействованный флаг
                if (card>=993000000000LL)
                    isBonus=CurrentWorkPlace->NeedCert;

                if (isBonus)
                {
                    res=true;
                }
            }
        }
    }
    return res;
}


void CashRegister::AddBonusOperation(BNS_RET* BNS_Return, int guard)
{
    //Log("+AddBonusOperation+");
    AddBonusOperationToTable(BonusTable, BNS_Return, guard);

    // к отправке
    AddBonusOperationToTable(BonusToSendTable, BNS_Return, guard);

    //if(!SQLServer->running()) SQLServer->start();
    //Log("-AddBonusOperation-");
}


int CashRegister::AddBonusOperationToTable(DbfTable *tbl, BNS_RET* BNS_Return, int guard)
{
    try//store critical action's information
    {
        DbfMutex->lock();
        tbl->BlankRecord();

//		tbl->PutLongField("ACTION",BNS_Return->action);

        tbl->PutLongField("CASHDESK",GetCashRegisterNum());
        tbl->PutLongField("CHECKNUM",GetLastCheckNum()+1);

        tbl->PutField("DATE",xbDate().GetDate().c_str());
        tbl->PutField("TIME",GetCurTime().c_str());

        tbl->PutField("CARD", BNS_Return->card.c_str());
        tbl->PutField("OPER", BNS_Return->operation.c_str());
//        tbl->PutField("PAN4", BNS_Return->pwd_card.c_str());

        tbl->PutField("CARD_STAT", BNS_Return->CS_S.c_str());

//        tbl->PutField("SHA1", BNS_Return->sha1.c_str());
//        tbl->PutField("GUID", BNS_Return->guid.c_str());
        tbl->PutField("TRAN_ID", BNS_Return->id.c_str());
//        tbl->PutField("TRAN_DT", BNS_Return->dt_tran.c_str());

        tbl->PutDoubleField("SUMMA", BNS_Return->result_summa);
        tbl->PutDoubleField("ACTIVE_BNS", BNS_Return->bonus_active);
        tbl->PutDoubleField("HOLD_BNS", BNS_Return->bonus_hold);
        tbl->PutDoubleField("SUMMA_ADD", BNS_Return->summa_add);
        tbl->PutDoubleField("SUMMA_REM", BNS_Return->summa_remove);
        tbl->PutDoubleField("SUMMA_CHG", BNS_Return->summa_change);

        tbl->PutLongField("CASHMAN",CurrentSaleman->Id);
        tbl->PutLongField("GUARD",guard);

        tbl->PutLLongField("CUSTOMER",BNS_Return->Customer);

        tbl->PutLongField("ACTION", BNS_Return->action);

        tbl->PutLongField("MODE", BNS_Return->mode);
        tbl->PutLongField("RETCODE", BNS_Return->ReturnCode);


        string desc = BNS_Return->info;
        if((desc.length()>4) && (desc.substr(0,4)=="null")) desc="null";

        tbl->PutField("INFO", desc.c_str());

        tbl->PutLongField("RETCHECK",BNS_Return->RetCheque);
        tbl->PutField("RETTRAN_ID", BNS_Return->rettran_id.c_str());

        tbl->PutField("TERMINAL", BNS_Return->terminal.c_str());

        tbl->AppendRecord();

DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
        return -1;
    }

    return 0;
}

bool CashRegister::CheckSourceCard(long long CurNumber, CardInfoStruct *CardInfo, bool usemsg)
{
   bool needChange = true;

   if(CurNumber==0LL)
    {
         return false;
    }

/*
   if(CurNumber<993000000000LL)
     {
         if (usemsg) ShowMessage("Неправильный номер карты. Считайте дисконтную карту Гастроном!");
         needChange=false;
     }
*/

    if(needChange)
    {
        if (FindCardInRangeTable(CardInfo, CurNumber))
        {
            if (CardInfo->BonusCard==1)
            {
                if (usemsg) ShowMessage("Карта является бонусной. Считайте дисконтную карту!");
                needChange=false;
            }

            // Условия для карт партнеров
            if(CurNumber<993000000000LL)
            {
                 if (!CardInfo->ParType) // условный тип карты партнеров в признак
                 {
                     //  Карта партнеров
                     Log("Карта партнеров не попадающая под замену!");
                     //if (usemsg) ShowMessage("Неправильный номер карты. Считайте дисконтную карту Гастроном!");
                     needChange=false;
                 }
                 else
                 {
                     // Подменяем тип карты источника для выбора правильной карты замены
                     CardInfo->CardType = CardInfo->ParType;
                 }
            }
        }
        else
        {
           if (usemsg) ShowMessage("Карта не найдена в дипазонах карт!");
           needChange=false;
        };
    }

    if(needChange)
    {
        if (FindCardInCustomerTable(CardInfo, CurNumber))
        {
            if (CardInfo->BonusCard==1)
            {
                if (usemsg) ShowMessage("Карта является бонусной. Считайте дисконтную карту!");
                needChange=false;
            }

        };
    }

    if(needChange)
    {

        string card = LLong2Str(CurNumber);

        ;;Log("+ GetCardInfo +");
        BNS_RET* retInf = BNS->exec("inf", card);
        ;;Log("- GetCardInfo -");

        if ((retInf->errorCode==0) && (retInf->status=="4") && (retInf->ActiveBonuses==0))
        {
          // можно менять
        }
        else
            if ((retInf->errorCode==0) || (retInf->errorCode==-22))
            {
                if (usemsg) ShowMessage("Карта уже заменена");
                needChange=false;
            }
            else
            {
                // Код 25 - карты с таким номеров не существует
                if (retInf->errorCode!=25)
                {
                    if (usemsg) ShowMessage("Ошибка связи с ПЦ. Неудачная попытка проверки карты");
                    needChange=false;
                }
            }

    }

    return needChange;
}



void CashRegister::ChangeBonusCard(long long SourceCard)
{

    FILE * fe = fopen("/Kacca/bonus/change.flg", "r");
    if(!fe) return;
    fclose(fe);

    int lKeyPressed=-1;

    string SourceCardText="------------";
    string DestCardText="------------";

    CardInfoStruct SourceCardInfo;

    if (SourceCard>0LL)
        if (CheckSourceCard(SourceCard, &SourceCardInfo, false))  SourceCardText = LLong2Str(SourceCard);
        else return;
    else
        if (SourceCard<0LL) SourceCard=0LL;
        else return;

    long long DestCard, CurNumber;

    DestCard=CurNumber=0LL;

    int res=-1;
    double Summa=0;

    int GuardRes=NotCorrect;

    string text;


    bool finish = false;
    bool toexit = false;
    bool allright = false;
    double WholeSumma, CurSumma;
    WholeSumma = CurSumma = 0;

    // заблокируем сканер
    bool SaveScannerSuspend = CurrentWorkPlace->Scanner->suspend;
    CurrentWorkPlace->Scanner->suspend = true;


    QString FirstButton  = QString::null;
    QString SecondButton  = QString::null;
    QString ThirdButton = QString::null;

    int mode_change = 0;
    if ((SourceCard>0LL) && (SourceCard<993000000000LL)) mode_change = 2;

    ChangeWin = new ChangeForm(this, mode_change);

    while (!toexit)
    {

        finish=false;

        ChangeWin->CurS->setText(W2U(""));
        ChangeWin->CurD->setText(W2U(""));

        QFont tmp_font( font() );

        ChangeWin->SourceCardLabel->setFont( tmp_font );
        ChangeWin->DestCardLabel->setFont( tmp_font );

        tmp_font.setBold(true);

        if (SourceCard==0LL)
        {

            ChangeWin->CurS->setText(W2U("-->"));

            FirstButton = W2U("Считать дисконтную карту");

            ChangeWin->SourceCardLabel->setFont( tmp_font );

        }
        else
        {
            if (DestCard==0LL)
            {
                FirstButton = W2U("Считать БОНУСНУЮ карту");

                ChangeWin->CurD->setText(W2U("-->"));

                ChangeWin->DestCardLabel->setFont( tmp_font );

            }
            else
            {

                ChangeWin->InfoLabel->setText(W2U("Нажмите кнопку ЗАМЕНИТЬ для выдачи карты"));

                FirstButton = W2U("Заменить");
                finish=true; // завершаем
            }
        }


        SecondButton  = W2U("Отмена");

        ChangeWin->AddButton->setText(FirstButton);
        ChangeWin->CancelButton->setText(SecondButton);

        ChangeWin->SourceCard->setText(W2U(SourceCardText));
        ChangeWin->DestCard->setText(W2U(DestCardText));

        int lKeyPressed = -1;
        if (ChangeWin->exec()==QDialog::Accepted)
        {
            lKeyPressed = 0;
        }
        else
        {
            lKeyPressed = 1;
        }

        // Нажали кнопку отмена
        if (lKeyPressed==1)
        {
            if (ShowQuestion("Отменить выдачу бонусной карты?",true))
            {
                //ChangeCardText = "";
                res=1;
                toexit = true;
            }
            continue;

        }


        if (!finish)
        {

            bool oldscannersuspend;

            CurSumma = 0;
            CurNumber=0;

            oldscannersuspend=CurrentWorkPlace->Scanner->suspend;
            CurrentWorkPlace->Scanner->suspend = false;
            CurNumber = CurrentWorkPlace->ScanBonusCard();
            CurrentWorkPlace->Scanner->suspend = oldscannersuspend;

            if (CurNumber>0)
            {

                string card = LLong2Str(CurNumber);

                if (SourceCard==0LL)
                {
                    bool needChange = CheckSourceCard(CurNumber, &SourceCardInfo, true);

                    if (needChange)
                    {
                        // Нет ошибок
                        SourceCard = CurNumber;
                        //SourceCardText = LLong2Str(SourceCard);
                        SourceCardText = Card2Str(SourceCard);
                    }
                }
                else
                {
                    bool err = false;
                    if(SourceCard==CurNumber)
                    {
                        ShowMessage("Повтор карты");
                        err=true;
                    }

                    if(CurNumber<993000000000LL)
                    {
                        ShowMessage("Неправильный номер карты. Считайте бонусную карту Гастроном!");
                        err=true;
                    }

                    CardInfoStruct CardInfo;
                    if (FindCardInRangeTable(&CardInfo, CurNumber))
                    {
                        if (CardInfo.BonusCard==1)
                        {
                            if (
                            ((SourceCardInfo.CardType==1) && (CardInfo.CardType!=1))
                            ||
                            ((SourceCardInfo.CardType==2) && (CardInfo.CardType!=2) && (CardInfo.CardType!=3))
                            ||
                            ((SourceCardInfo.CardType==4) && (CardInfo.CardType!=2) && (CardInfo.CardType!=3))
                            ||
                            ((SourceCardInfo.CardType==3) && (CardInfo.CardType!=3))
                            )
                            {
                                ShowMessage("Карты должны быть одинакового типа ("+Int2Str(SourceCardInfo.CardType)+" <-> "+Int2Str(CardInfo.CardType)+")");
                                err=true;
                            }

                        }else
                        {
                            ShowMessage("Карта не является бонусной. Считайте бонусную карту Гастроном!");
                            err=true;
                        }

                    };


                    if(!err)
                    {

                        ;;Log("+ GetCardInfo +");
                        BNS_RET* retInf = BNS->exec("inf", card);
                        ;;Log("- GetCardInfo -");

                        if (retInf->errorCode==0)
                        {
                            ShowMessage("Карта уже зарегистрирована в системе");
                            err=true;
                        }
                        else
                        {
                            // Код 25 - карты с таким номеров не существует
                            if (retInf->errorCode!=25)
                            {
                                ShowMessage("Ошибка связи с ПЦ. Неудачная попытка проверки карты");
                                err=true;
                            }
                        }

                    }

                    if (!err)
                    {
                        // Нет ошибок
                        DestCard = CurNumber;
                        //DestCardText = LLong2Str(DestCard);
                        DestCardText = Card2Str(DestCard);
                    }
                }

            }
        }
        else
        {
        // Процесс выдачи карты

            // Записать информацию в базу
            AddCardChangeOperationToTable(BonusTable, SourceCard, DestCard, 99, "TRY CARD CHANGE");

            AddCardChangeOperationToTable(BonusToSendTable, SourceCard, DestCard, 99, "TRY CARD CHANGE");

            ;;Log("+ GetCard +");
            string card = LLong2Str(SourceCard);
            BNS_RET* retInf = BNS->exec("reg", card);
            ;;Log("- GetCardInfo -");

            if (retInf->errorCode==0)
            {
                ;;Log("+ BonusCardChange +");
                BNS_RET* retInf = BNS->exec("change", LLong2Str(SourceCard), LLong2Str(DestCard));
                ;;Log("- BonusCardChange -");
                if (retInf->errorCode==0)
                {

                    // Записать информацию в базу
                    AddCardChangeOperationToTable(BonusTable, SourceCard, DestCard, 100, "CARD CHANGE");

                    AddCardChangeOperationToTable(BonusToSendTable, SourceCard, DestCard, 100, "CARD CHANGE");

                    ShowMessage("Карта успешно зарегистрирована");
                }
                else
                {
                    ShowMessage("Ошибка замены карты! Карту не выдавать!");
                }

            }
            else
            {
                ShowMessage("Ошибка замены карты! Карту не выдавать!");
            }

        }

        if  ( finish )
        {
            allright = true;
        }

        toexit = allright || lKeyPressed == 1;

    } // считываем Карту партнеров и Бонусную карту

    // разблокируем сканер
    CurrentWorkPlace->Scanner->suspend = false;

}


int CashRegister::AddCardChangeOperationToTable(DbfTable *tbl, long long SourceCard, long long DestCard, long ActionStatus, char* ActionText)
{
    try//store critical action's information
    {
        DbfMutex->lock();
        tbl->BlankRecord();

//		tbl->PutLongField("ACTION",BNS_Return->action);

        tbl->PutLongField("CASHDESK",GetCashRegisterNum());
        tbl->PutLongField("CHECKNUM",GetLastCheckNum()+1);

        tbl->PutField("DATE",xbDate().GetDate().c_str());
        tbl->PutField("TIME",GetCurTime().c_str());

        tbl->PutField("OPER", ActionText);

        tbl->PutLongField("CASHMAN",CurrentSaleman->Id);

        tbl->PutLongField("ACTION", ActionStatus);

        tbl->PutLLongField("OLDCARD",SourceCard);
        tbl->PutLLongField("NEWCARD",DestCard);

        tbl->AppendRecord();

        DbfMutex->unlock();
    }
    catch(cash_error& ex)
    {
        DbfMutex->unlock();
        LoggingError(ex);
        return -1;
    }

    return 0;
}




int CashRegister::GetBonusCardByPhone(string phone, long long *number)
{
    int res=1;

    ;;Log("+ GetCardByPhone +");
    BNS_RET* retInf = BNS->exec("cardByPhone", phone);
    ;;Log("- GetCardByPhone -");

    if (retInf->errorCode==0)
    {
        *number = retInf->Customer;

        ;;Log("Основная карта по номеру телефона: "+LLong2Str(*number));

        res = 0;
    }

    return res;
}

string CashRegister::GetStatusStr(string str)
{
    string StatusStr="";
    if (CurrentWorkPlace->StatusBonusCard=="1") StatusStr = "Активирована";
    else if (CurrentWorkPlace->StatusBonusCard=="2") StatusStr = "Только начисления";
    else if (CurrentWorkPlace->StatusBonusCard=="3") StatusStr = "Активирована";
    else if (CurrentWorkPlace->StatusBonusCard=="4") StatusStr = "Не активирована";
    else if (CurrentWorkPlace->StatusBonusCard=="255") StatusStr = "Заблокирована";
    else if (CurrentWorkPlace->StatusBonusCard=="-1") StatusStr = "Offline";
    else StatusStr = "Offline";
    return StatusStr;
}




void CashRegister::ChangeBonusCard2Vip(long long SourceCard, int destCardType)
{

    int lKeyPressed=-1;

    string SourceCardText="------------";
    string DestCardText="------------";

    CardInfoStruct SourceCardInfo;

    if (SourceCard>0LL)
        //SourceCardText = LLong2Str(SourceCard);
        SourceCardText = Card2Str(SourceCard);
    else
        //if (SourceCard<0LL) SourceCard=0LL; else
        return;

    string sourceCardTypeText = "";
    string destCardTypeText = "";

    int changeFormMode = 1;

    if (destCardType == 0) destCardType = CD_VIP;

    switch (destCardType)
    {
        case CD_SUPER_VIP :
        {
            sourceCardTypeText = "VIP";
            destCardTypeText = "Super-VIP";
            changeFormMode = 3;
            break;
        }
        case CD_VIP :
        {
            sourceCardTypeText = "5%";
            destCardTypeText = "VIP";
            changeFormMode = 1;
            break;
        }
    }


    long long DestCard, CurNumber;

    DestCard=CurNumber=0LL;

    int res=-1;
    double Summa=0;

    int GuardRes=NotCorrect;

    string text;


    bool finish = false;
    bool toexit = false;
    bool allright = false;
    double WholeSumma, CurSumma;
    WholeSumma = CurSumma = 0;

    // заблокируем сканер
    bool SaveScannerSuspend = CurrentWorkPlace->Scanner->suspend;
    CurrentWorkPlace->Scanner->suspend = true;


    QString FirstButton  = QString::null;
    QString SecondButton  = QString::null;
    QString ThirdButton = QString::null;


    ChangeWin = new ChangeForm(this, changeFormMode);

//    ChangeWin->FormLabel->setProperty( "text", W2U( "ЗАМЕНА 5% КАРТЫ НА VIP" ) );

    while (!toexit)
    {

        finish=false;

        ChangeWin->CurS->setText(W2U(""));
        ChangeWin->CurD->setText(W2U(""));


        QFont tmp_font( font() );

        ChangeWin->SourceCardLabel->setFont( tmp_font );
        ChangeWin->DestCardLabel->setFont( tmp_font );

        tmp_font.setBold(true);

        if (SourceCard==0LL)
        {

            ChangeWin->CurS->setText(W2U("-->"));

            FirstButton = W2U("Считать карту "+sourceCardTypeText);

            ChangeWin->SourceCardLabel->setFont( tmp_font );

        }
        else
        {
            if (DestCard==0LL)
            {
                FirstButton = W2U("Считать "+destCardTypeText+" карту");

                ChangeWin->CurD->setText(W2U("-->"));

                ChangeWin->DestCardLabel->setFont( tmp_font );

            }
            else
            {

                ChangeWin->InfoLabel->setText(W2U("Нажмите кнопку ЗАМЕНИТЬ для выдачи карты"));

                FirstButton = W2U("Заменить");
                finish=true; // завершаем
            }
        }


        SecondButton  = W2U("Отмена");

        ChangeWin->AddButton->setText(FirstButton);
        ChangeWin->CancelButton->setText(SecondButton);

        ChangeWin->SourceCard->setText(W2U(SourceCardText));
        ChangeWin->DestCard->setText(W2U(DestCardText));

        int lKeyPressed = -1;
        if (ChangeWin->exec()==QDialog::Accepted)
        {
            lKeyPressed = 0;
        }
        else
        {
            lKeyPressed = 1;
        }

        // Нажали кнопку отмена
        if (lKeyPressed==1)
        {
            if (ShowQuestion("Отменить выдачу "+destCardTypeText+" карты?",true))
            {
                //ChangeCardText = "";
                res=1;
                toexit = true;
            }
            continue;

        }


        if (!finish)
        {

            bool oldscannersuspend;

            CurSumma = 0;
            CurNumber=0;

            oldscannersuspend=CurrentWorkPlace->Scanner->suspend;
            CurrentWorkPlace->Scanner->suspend = false;
            CurNumber = CurrentWorkPlace->ScanBonusCard();
            CurrentWorkPlace->Scanner->suspend = oldscannersuspend;

            if (CurNumber>0)
            {

                string card = LLong2Str(CurNumber);

                if (SourceCard==0LL)
                {
                    bool needChange = true; //CheckSourceCard(CurNumber, &SourceCardInfo, true);

                    if (needChange)
                    {
                        // Нет ошибок
                        SourceCard = CurNumber;
                        SourceCardText = LLong2Str(SourceCard);
                    }
                }
                else
                {
                    bool err = false;
                    if(SourceCard==CurNumber)
                    {
                        ShowMessage("Повтор карты");
                        err=true;
                    }

                    if(CurNumber<993000000000LL)
                    {
                        ShowMessage("Неправильный номер карты. Считайте бонусную "+destCardTypeText+" карту Гастроном!!");
                        err=true;
                    }

                    CardInfoStruct CardInfo;
                    if (FindCardInRangeTable(&CardInfo, CurNumber))
                    {
                        if (CardInfo.BonusCard==1)
                        {
                            if (CardInfo.CardType!=destCardType)
                            {
                                ShowMessage("Карта не является "+destCardTypeText+". Считайте бонусную "+destCardTypeText+" карту Гастроном!");
                                err=true;
                            }

                        }else
                        {
                            ShowMessage("Карта не является бонусной. Считайте бонусную "+destCardTypeText+" карту Гастроном!");
                            err=true;
                        }

                    };


                    if(!err)
                    {

                        ;;Log("+ GetCardInfo +");
                        BNS_RET* retInf = BNS->exec("inf", card);
                        ;;Log("- GetCardInfo -");

                        if (retInf->errorCode==0)
                        {

                            if (destCardType == CD_SUPER_VIP)
                            {
                                // Супер-VIP карты могут быть прописаны в системе
                                if ((retInf->status != "4") || (retInf->ActiveBonuses != 0) || (retInf->HoldedBonuses != 0))
                                {
                                    ShowMessage("Карта уже зарегистрирована в системе");
                                    err=true;
                                }
                            }
                            else
                            {
                                ShowMessage("Карта уже зарегистрирована в системе");
                                err=true;
                            }
                        }
                        else
                        {
                            // Код 25 - карты с таким номеров не существует
                            if (retInf->errorCode!=25)
                            {
                                ShowMessage("Ошибка связи с ПЦ. Неудачная попытка проверки карты");
                                err=true;
                            }
                        }

                    }

                    if (!err)
                    {
                        // Нет ошибок
                        DestCard = CurNumber;
                        //DestCardText = LLong2Str(DestCard);
                        DestCardText = Card2Str(DestCard);
                    }
                }

            }
        }
        else
        {
        // Процесс выдачи карты

            // Записать информацию в базу
            AddCardChangeOperationToTable(BonusTable, SourceCard, DestCard, 199, (char*)("TRY CARD CHANGE TO "+destCardTypeText).c_str());

            AddCardChangeOperationToTable(BonusToSendTable, SourceCard, DestCard, 199, (char*)("TRY CARD CHANGE TO "+destCardTypeText).c_str());

//            ;;Log("+ GetCard +");
//            string card = LLong2Str(SourceCard);
//            BNS_RET* retInf = BNS->exec("reg", card);
//            ;;Log("- GetCardInfo -");
//
//            if (retInf->errorCode==0)
//            {
                ;;Log("+ BonusCardChange +");
                BNS_RET* retInf = BNS->exec("change", LLong2Str(SourceCard), LLong2Str(DestCard));
                ;;Log("- BonusCardChange -");
                if (retInf->errorCode==0)
                {

                    // Записать информацию в базу
                    AddCardChangeOperationToTable(BonusTable, SourceCard, DestCard, 200, (char*)("CARD CHANGE TO "+destCardTypeText).c_str());

                    AddCardChangeOperationToTable(BonusToSendTable, SourceCard, DestCard, 200, (char*)("CARD CHANGE TO "+destCardTypeText).c_str());

                    ShowMessage("Карта успешно зарегистрирована");
                }
                else
                {
                    ShowMessage("Ошибка замены карты! Карту не выдавать!");
                }

//            }
//            else
//            {
//                ShowMessage("Ошибка замены карты! Карту не выдавать!");
//            }

        }

        if  ( finish )
        {
            allright = true;
        }

        toexit = allright || lKeyPressed == 1;

    } // считываем Карту партнеров и Бонусную карту

    // разблокируем сканер
    CurrentWorkPlace->Scanner->suspend = false;

}
