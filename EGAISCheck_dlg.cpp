#include <qlayout.h>
#include <qframe.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtextedit.h>
#include <qstring.h>
#include <qfile.h>
#include <string>
#include <xbase/xbase.h>

using namespace std;

#include "EGAISCheck_dlg.h"
//#include "ui_EGAISCheck_dlg.h"

#include "DbfTable.h"
#include "Utils.h"
#include "Cash.h"
#include "Threads.h"

//bool EGAISCheck_dlg::UTM_is_owner(const QString* barcode) {

//#define EC_res_file "/Kacca/tmp/EC_r"

//    char curl_cmd[2048];
//    sprintf(curl_cmd, "curl http://%s/api/mark/check?code=%s -o " EC_res_file,
//            CashReg->EgaisSettings.IP.c_str(),
//            barcode->ascii());

//    LogEC("curl_cmd="+(string)curl_cmd);

//    int retcode = system(curl_cmd);
//    LogEC("retcode="+Int2Str(retcode));

//    QFile file(EC_res_file);
//    if(!file.open(IO_ReadOnly)) {
//        LogEC("error open file " EC_res_file" = " + file.errorString());
//    }
//    QTextStream in(&file);
//    bool is_owner = false;
//    while(!in.atEnd()) {
//        QString line = in.readLine();
//        if (line.contains("\"owner\":true")) {
//            LogEC("UTM answer = "+line);
//            is_owner = true;
//            break;
//        }
//    }
//    file.close();

//    LogEC("UTM_is_owner("
//          + (string)barcode->ascii()
//          + ") = "
//          + Int2Str(is_owner)
//          + "\n"
//          );
//    return is_owner;
//}


EGAISCheck_dlg::EGAISCheck_dlg(QWidget* parent)
    : QDialog(parent, "EGAISCheck_dlg", true)
{
    #define GOOD_BARCODE_msg "Товар МОЖНО продать через эту кассу"
    #define BAD_BARCODE_msg "Товар НЕЛЬЗЯ продать через эту кассу"

    Log("building EGAISCheck_dlg");

    CashReg = (CashRegister*)parent;

    setName("EGAISCheck_dlg");

    setMinimumSize( QSize( 190, 120 ) );
    setMaximumSize( QSize( 1280, 1024 ) );
    setBaseSize( QSize( 640, 480 ) );
    setFixedSize(CashReg->size());


    setCaption( W2U(" ") );

    QFont form_font( font() );
    form_font.setPointSize( 10 );
    setFont( form_font );

    vl = new QVBoxLayout(this, 0, 0, "vl");

    ec_Frame = new QFrame(this, "ec_Frame");

	exit_Button = new QPushButton(this, "exit_Button");
    exit_Button->setText(W2U("Выход"));
//    exit_Button->setText(W2U("exit"));
    vl_ec = new QVBoxLayout(ec_Frame, 0, 0, "vl");

    ec_TextEdit = new QTextEdit(ec_Frame, "ec_TextEdit");
    ec_TextEdit->setHScrollBarMode(QScrollView::AlwaysOff);
    ec_TextEdit->setTabChangesFocus(true);
//    ec_TextEdit->setText("1883000331232511200013QVPGYL7ZUOTBLH3TUBBT6ETQI"
//                         "QYA4BTHQUZVKSPFSSOMCL26C2OVUCT32GPCPAJJ7EBEQNEG"
//                         "ZRQCZO5PLK6ZACUHG2HMLJJM2PGMX3EERI65IHRCLGW7L6X"
//                         "3LMZJEQOY");
    ec_TextEdit->setWordWrap(QTextEdit::WidgetWidth);
    ec_TextEdit->setWrapPolicy(QTextEdit::Anywhere);

    res_Label = new QLabel(ec_Frame, "res_Label");
    res_Label->setText(W2U("Просканируйте штрихкод"));
    res_Label->setAlignment(QLabel::AlignCenter);

    vl_ec->addWidget(ec_TextEdit);
    vl_ec->addStretch(1);
    vl_ec->addWidget(res_Label);
    vl_ec->addStretch(1);

    vl->addWidget(ec_Frame);
    vl->addWidget(exit_Button);

    connect( exit_Button, SIGNAL( clicked() ), this, SLOT( accept() ) );
//    connect( CancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
//    connect( ec_TextEdit, SIGNAL( returnPressed() ), this, SLOT( accept() ) );

    ec_TextEdit->setFocus();
    ec_TextEdit->setText(W2U("Просканируйте штрихкод"));

    ec_scanner = new ComPortScan(CashReg
                                 ,CashReg->ConfigTable->GetLongField("SCANPORT")
                                 ,CashReg->ConfigTable->GetLongField("SCANBAUD")
                                 );
    Log("EGAISCheck_dlg built");
}

void EGAISCheck_dlg::emitBarcode(QString barcode) {
    ec_TextEdit->setText(barcode);
    if ( UTM_is_owner(&barcode) ) {
        res_Label->setPaletteBackgroundColor(Qt::green);
        res_Label->setText(W2U(GOOD_BARCODE_msg));
//        ec_TextEdit->setStyleSheet("QTextEdit {background-color:green}");
        Log(""+barcode+" "+GOOD_BARCODE_msg);
    }else{
        res_Label->setPaletteBackgroundColor(Qt::red);
        res_Label->setText(W2U(BAD_BARCODE_msg));
//        ec_TextEdit->setStyleSheet("QTextEdit {background-color:red}");
        Log(""+barcode+" "+BAD_BARCODE_msg);
    }
}

EGAISCheck_dlg::~EGAISCheck_dlg()
{
    delete ec_scanner;
    Log("deleting EGAISCheck_dlg");
}
