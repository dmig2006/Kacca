#include "QrCode.h"

using namespace std;

QrCode::QrCode(QWidget* parent): QDialog(parent, "QrCode", true)
{
	Log("building QrCode_dlg");

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
	
    qrSiteApiLineEdit = new QLineEdit(this);
    qrSiteApiLineEdit -> setGeometry(40,40,113,24);

    qrStatusLabel = new QLabel(this);
    qrStatusLabel -> setGeometry(50,80,101,21);

    qrResultTextEdit = new QTextEdit(this);
    qrResultTextEdit -> setGeometry(210,30,311,131);

    qrTestApiButton = new QPushButton(this, "qrTestApiButton");
    qrTestApiButton -> setGeometry(10,110,80,24);
    qrTestApiButton->setText(W2U("ТестАпи"));

    qrExitButton = new QPushButton(this, "qrExitButton");
    qrExitButton -> setGeometry(110,110,80,24);
    qrExitButton ->setText(W2U("Выход"));

    connect( qrExitButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
}

QrCode::~QrCode()
{
    Log("deleting QrCode");
	delete qrSiteApiLineEdit;
    delete qrStatusLabel;
    delete qrResultTextEdit;
    delete qrTestApiButton;
    delete qrExitButton;
}
