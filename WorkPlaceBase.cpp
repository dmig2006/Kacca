/****************************************************************************
** Form implementation generated from reading ui file 'WorkPlaceBase.ui'
**
** Created: Tue Aug 17 13:10:16 2010
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "WorkPlaceBase.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qprogressbar.h>
#include <qheader.h>
#include <qlistview.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>
#include <qpixmap.h>


/*
 *  Constructs a WorkPlaceBase as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
WorkPlaceBase::WorkPlaceBase( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "WorkPlaceBase" );
    WorkPlaceBaseLayout = new QVBoxLayout( this, 0, 0, "WorkPlaceBaseLayout"); 

    frTop = new QFrame( this, "frTop" );
    frTop->setMinimumSize( QSize( 640, 70 ) );
    frTop->setMaximumSize( QSize( 32767, 70 ) );
    frTop->setFrameShape( QFrame::NoFrame );
    frTop->setFrameShadow( QFrame::Plain );
    frTopLayout = new QHBoxLayout( frTop, 0, 0, "frTopLayout"); 

    frTopLeft = new QFrame( frTop, "frTopLeft" );
    frTopLeft->setFrameShape( QFrame::NoFrame );
    frTopLeft->setFrameShadow( QFrame::Raised );
    frTopLeftLayout = new QVBoxLayout( frTopLeft, 0, 0, "frTopLeftLayout"); 

    layout3 = new QHBoxLayout( 0, 0, 0, "layout3"); 

    layout1 = new QVBoxLayout( 0, 0, 0, "layout1"); 

    lbSaleman = new QLabel( frTopLeft, "lbSaleman" );
    lbSaleman->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)7, (QSizePolicy::SizeType)5, 0, 1, lbSaleman->sizePolicy().hasHeightForWidth() ) );
    lbSaleman->setAlignment( int( QLabel::AlignCenter ) );
    layout1->addWidget( lbSaleman );

    Saleman = new QLabel( frTopLeft, "Saleman" );
    Saleman->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 1, Saleman->sizePolicy().hasHeightForWidth() ) );
    Saleman->setAlignment( int( QLabel::AlignCenter ) );
    layout1->addWidget( Saleman );
    layout3->addLayout( layout1 );

    layout2 = new QVBoxLayout( 0, 0, 0, "layout2"); 

    lbTotalDiscount = new QLabel( frTopLeft, "lbTotalDiscount" );
    lbTotalDiscount->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)7, (QSizePolicy::SizeType)5, 0, 0, lbTotalDiscount->sizePolicy().hasHeightForWidth() ) );
    lbTotalDiscount->setAlignment( int( QLabel::AlignCenter ) );
    layout2->addWidget( lbTotalDiscount );

    eTotalDiscount = new QLabel( frTopLeft, "eTotalDiscount" );
    QFont eTotalDiscount_font(  eTotalDiscount->font() );
    eTotalDiscount_font.setPointSize( 18 );
    eTotalDiscount->setFont( eTotalDiscount_font ); 
    eTotalDiscount->setAlignment( int( QLabel::WordBreak | QLabel::AlignCenter ) );
    layout2->addWidget( eTotalDiscount );
    layout3->addLayout( layout2 );
    frTopLeftLayout->addLayout( layout3 );

    eEdit = new QLineEdit( frTopLeft, "eEdit" );
    eEdit->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 2, eEdit->sizePolicy().hasHeightForWidth() ) );
    frTopLeftLayout->addWidget( eEdit );
    frTopLayout->addWidget( frTopLeft );

    frTopRight = new QFrame( frTop, "frTopRight" );
    frTopRight->setFrameShape( QFrame::NoFrame );
    frTopRight->setFrameShadow( QFrame::Raised );
    frTopRightLayout = new QVBoxLayout( frTopRight, 0, 0, "frTopRightLayout"); 

    lbTotalSumm = new QLabel( frTopRight, "lbTotalSumm" );
    lbTotalSumm->setEnabled( TRUE );
    lbTotalSumm->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 1, lbTotalSumm->sizePolicy().hasHeightForWidth() ) );
    lbTotalSumm->setAlignment( int( QLabel::AlignCenter ) );
    frTopRightLayout->addWidget( lbTotalSumm );

    MTotalSum = new QLabel( frTopRight, "MTotalSum" );
    MTotalSum->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)7, (QSizePolicy::SizeType)5, 0, 5, MTotalSum->sizePolicy().hasHeightForWidth() ) );
    QFont MTotalSum_font(  MTotalSum->font() );
    MTotalSum_font.setPointSize( 32 );
    MTotalSum_font.setBold( TRUE );
    MTotalSum->setFont( MTotalSum_font ); 
    MTotalSum->setAlignment( int( QLabel::WordBreak | QLabel::AlignCenter ) );
    frTopRightLayout->addWidget( MTotalSum );
    frTopLayout->addWidget( frTopRight );
    WorkPlaceBaseLayout->addWidget( frTop );

    progressBar1 = new QProgressBar( this, "progressBar1" );
    progressBar1->setMinimumSize( QSize( 0, 5 ) );
    progressBar1->setMaximumSize( QSize( 32767, 5 ) );
    progressBar1->setBackgroundOrigin( QProgressBar::WindowOrigin );
    progressBar1->setFrameShadow( QProgressBar::Raised );
    progressBar1->setTotalSteps( 100 );
    progressBar1->setCenterIndicator( FALSE );
    progressBar1->setIndicatorFollowsStyle( FALSE );
    progressBar1->setPercentageVisible( FALSE );
    WorkPlaceBaseLayout->addWidget( progressBar1 );

    frGrid = new QFrame( this, "frGrid" );
    frGrid->setMinimumSize( QSize( 640, 70 ) );
    frGrid->setFrameShape( QFrame::NoFrame );
    frGrid->setFrameShadow( QFrame::Plain );
    frGridLayout = new QVBoxLayout( frGrid, 0, 0, "frGridLayout"); 

    ChequeList = new QListView( frGrid, "ChequeList" );
    ChequeList->addColumn( tr( "No" ) );
    ChequeList->header()->setClickEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->header()->setResizeEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->addColumn( trUtf8( "\xd0\x9a\xd0\x9e\xd0\x94" ) );
    ChequeList->header()->setClickEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->header()->setResizeEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->addColumn( trUtf8( "\xd0\x9d\xd0\x90\xd0\x98\xd0\x9c\xd0\x95\xd0\x9d\xd0\x9e\xd0\x92\xd0\x90\xd0\x9d\xd0"
    "\x98\xd0\x95" ) );
    ChequeList->header()->setClickEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->header()->setResizeEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->addColumn( trUtf8( "\xd0\xa6\xd0\x95\xd0\x9d\xd0\x90" ) );
    ChequeList->header()->setClickEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->header()->setResizeEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->addColumn( trUtf8( "\xd0\x9a\xd0\x9e\xd0\x9b\x2d\xd0\x92\xd0\x9e" ) );
    ChequeList->header()->setClickEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->header()->setResizeEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->addColumn( trUtf8( "\xd0\xa1\xd0\xa3\xd0\x9c\xd0\x9c\xd0\x90" ) );
    ChequeList->header()->setClickEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->header()->setResizeEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->addColumn( trUtf8( "\xd0\xa1\xd0\x9a\xd0\x98\xd0\x94\xd0\x9a\xd0\x90" ) );
    ChequeList->header()->setClickEnabled( FALSE, ChequeList->header()->count() - 1 );
    ChequeList->header()->setResizeEnabled( FALSE, ChequeList->header()->count() - 1 );
    frGridLayout->addWidget( ChequeList );
    WorkPlaceBaseLayout->addWidget( frGrid );

    frBottom = new QFrame( this, "frBottom" );
    frBottom->setMinimumSize( QSize( 640, 72 ) );
    frBottom->setMaximumSize( QSize( 32767, 10000 ) );
    frBottom->setFrameShape( QFrame::NoFrame );
    frBottom->setFrameShadow( QFrame::Plain );
    frBottomLayout = new QVBoxLayout( frBottom, 0, 0, "frBottomLayout"); 

    layout13 = new QGridLayout( 0, 1, 1, 0, 0, "layout13"); 

    lbName = new QLabel( frBottom, "lbName" );
    lbName->setMinimumSize( QSize( 0, 10 ) );
    lbName->setMaximumSize( QSize( 32767, 10 ) );

    layout13->addWidget( lbName, 0, 1 );

    MItemPrice = new QLabel( frBottom, "MItemPrice" );
    MItemPrice->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, MItemPrice->sizePolicy().hasHeightForWidth() ) );
    MItemPrice->setMinimumSize( QSize( 100, 25 ) );
    MItemPrice->setMaximumSize( QSize( 100, 25 ) );
    QFont MItemPrice_font(  MItemPrice->font() );
    MItemPrice_font.setPointSize( 18 );
    MItemPrice->setFont( MItemPrice_font ); 
    MItemPrice->setFrameShape( QLabel::Box );
    MItemPrice->setFrameShadow( QLabel::Raised );

    layout13->addWidget( MItemPrice, 1, 2 );

    MItemName = new QLabel( frBottom, "MItemName" );
    MItemName->setMinimumSize( QSize( 0, 25 ) );
    MItemName->setMaximumSize( QSize( 32767, 25 ) );
    QFont MItemName_font(  MItemName->font() );
    MItemName_font.setPointSize( 18 );
    MItemName->setFont( MItemName_font ); 
    MItemName->setFrameShape( QLabel::Box );
    MItemName->setFrameShadow( QLabel::Raised );

    layout13->addWidget( MItemName, 1, 1 );

    lbCode = new QLabel( frBottom, "lbCode" );
    lbCode->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, lbCode->sizePolicy().hasHeightForWidth() ) );
    lbCode->setMinimumSize( QSize( 100, 10 ) );
    lbCode->setMaximumSize( QSize( 100, 10 ) );

    layout13->addWidget( lbCode, 0, 0 );

    MItemCode = new QLabel( frBottom, "MItemCode" );
    MItemCode->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, MItemCode->sizePolicy().hasHeightForWidth() ) );
    MItemCode->setMinimumSize( QSize( 100, 25 ) );
    MItemCode->setMaximumSize( QSize( 100, 25 ) );
    QFont MItemCode_font(  MItemCode->font() );
    MItemCode_font.setPointSize( 18 );
    MItemCode->setFont( MItemCode_font ); 
    MItemCode->setFrameShape( QLabel::Box );
    MItemCode->setFrameShadow( QLabel::Raised );

    layout13->addWidget( MItemCode, 1, 0 );

    lbPrice = new QLabel( frBottom, "lbPrice" );
    lbPrice->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, lbPrice->sizePolicy().hasHeightForWidth() ) );
    lbPrice->setMinimumSize( QSize( 100, 10 ) );
    lbPrice->setMaximumSize( QSize( 100, 10 ) );

    layout13->addWidget( lbPrice, 0, 2 );
    frBottomLayout->addLayout( layout13 );

    layout16 = new QHBoxLayout( 0, 0, 0, "layout16"); 

    layout14 = new QGridLayout( 0, 1, 1, 0, 0, "layout14"); 

    lbCount = new QLabel( frBottom, "lbCount" );

    layout14->addWidget( lbCount, 0, 1 );

    MItemCount = new QLabel( frBottom, "MItemCount" );
    MItemCount->setMinimumSize( QSize( 0, 25 ) );
    MItemCount->setMaximumSize( QSize( 32767, 25 ) );
    QFont MItemCount_font(  MItemCount->font() );
    MItemCount_font.setPointSize( 18 );
    MItemCount->setFont( MItemCount_font ); 
    MItemCount->setFrameShape( QLabel::Box );
    MItemCount->setFrameShadow( QLabel::Raised );

    layout14->addWidget( MItemCount, 1, 1 );

    lbDiscount = new QLabel( frBottom, "lbDiscount" );

    layout14->addWidget( lbDiscount, 0, 2 );

    MItemDiscount = new QLabel( frBottom, "MItemDiscount" );
    MItemDiscount->setMinimumSize( QSize( 0, 25 ) );
    MItemDiscount->setMaximumSize( QSize( 32767, 25 ) );
    QFont MItemDiscount_font(  MItemDiscount->font() );
    MItemDiscount_font.setPointSize( 18 );
    MItemDiscount->setFont( MItemDiscount_font ); 
    MItemDiscount->setFrameShape( QLabel::Box );
    MItemDiscount->setFrameShadow( QLabel::Raised );

    layout14->addWidget( MItemDiscount, 1, 2 );

    lbSum = new QLabel( frBottom, "lbSum" );

    layout14->addWidget( lbSum, 0, 3 );

    lbNumber = new QLabel( frBottom, "lbNumber" );

    layout14->addWidget( lbNumber, 0, 0 );

    MItemNumber = new QLabel( frBottom, "MItemNumber" );
    MItemNumber->setMinimumSize( QSize( 0, 25 ) );
    MItemNumber->setMaximumSize( QSize( 32767, 25 ) );
    QFont MItemNumber_font(  MItemNumber->font() );
    MItemNumber_font.setPointSize( 18 );
    MItemNumber->setFont( MItemNumber_font ); 
    MItemNumber->setFrameShape( QLabel::Box );
    MItemNumber->setFrameShadow( QLabel::Raised );

    layout14->addWidget( MItemNumber, 1, 0 );

    MItemSum = new QLabel( frBottom, "MItemSum" );
    MItemSum->setMinimumSize( QSize( 0, 25 ) );
    MItemSum->setMaximumSize( QSize( 32767, 25 ) );
    QFont MItemSum_font(  MItemSum->font() );
    MItemSum_font.setPointSize( 18 );
    MItemSum->setFont( MItemSum_font ); 
    MItemSum->setFrameShape( QLabel::Box );
    MItemSum->setFrameShadow( QLabel::Raised );

    layout14->addWidget( MItemSum, 1, 3 );
    layout16->addLayout( layout14 );

    lbTime = new QLabel( frBottom, "lbTime" );
    lbTime->setMinimumSize( QSize( 130, 35 ) );
    lbTime->setMaximumSize( QSize( 130, 35 ) );
    QFont lbTime_font(  lbTime->font() );
    lbTime_font.setPointSize( 18 );
    lbTime->setFont( lbTime_font ); 
    lbTime->setAlignment( int( QLabel::AlignCenter ) );
    layout16->addWidget( lbTime );
    frBottomLayout->addLayout( layout16 );
    WorkPlaceBaseLayout->addWidget( frBottom );

    frState = new QFrame( this, "frState" );
    frState->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)0, 0, 0, frState->sizePolicy().hasHeightForWidth() ) );
    frState->setMinimumSize( QSize( 0, 12 ) );
    frState->setMaximumSize( QSize( 32767, 12 ) );
    frState->setFrameShape( QFrame::ToolBarPanel );
    frState->setFrameShadow( QFrame::Plain );
    frStateLayout = new QHBoxLayout( frState, 0, 0, "frStateLayout"); 
    spacer1 = new QSpacerItem( 101, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    frStateLayout->addItem( spacer1 );

    ServerInfo = new QLabel( frState, "ServerInfo" );
    ServerInfo->setFrameShape( QLabel::Panel );
    ServerInfo->setFrameShadow( QLabel::Sunken );
    frStateLayout->addWidget( ServerInfo );

    RegisterInfo = new QLabel( frState, "RegisterInfo" );
    RegisterInfo->setFrameShape( QLabel::Panel );
    RegisterInfo->setFrameShadow( QLabel::Sunken );
    frStateLayout->addWidget( RegisterInfo );
    WorkPlaceBaseLayout->addWidget( frState );
    languageChange();
    resize( QSize(640, 480).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
}

/*
 *  Destroys the object and frees any allocated resources
 */
WorkPlaceBase::~WorkPlaceBase()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void WorkPlaceBase::languageChange()
{
    setCaption( tr( "APM KACCA" ) );
    lbSaleman->setText( trUtf8( "\xd0\x92\xd0\xb0\xd1\x81\x20\xd0\xbe\xd0\xb1\xd1\x81\xd0\xbb\xd1\x83\xd0\xb6\xd0\xb8"
    "\xd0\xb2\xd0\xb0\xd0\xb5\xd1\x82" ) );
    Saleman->setText( tr( "textLabel4" ) );
    lbTotalDiscount->setText( trUtf8( "\xd0\xa1\xd0\xba\xd0\xb8\xd0\xb4\xd0\xba\xd0\xb0\x20\xd0\xbf\xd0\xbe\x20\xd1\x87\xd0"
    "\xb5\xd0\xba\xd1\x83" ) );
    eTotalDiscount->setText( tr( "<font color=\"#0000ff\">0.00</font>" ) );
    lbTotalSumm->setText( trUtf8( "\x43\xd1\x83\xd0\xbc\xd0\xbc\xd0\xb0\x20\xd1\x87\xd0\xb5\xd0\xba\xd0\xb0" ) );
    MTotalSum->setText( tr( "<font color=\"#0000ff\">0.00</font>" ) );
    ChequeList->header()->setLabel( 0, tr( "No" ) );
    ChequeList->header()->setLabel( 1, trUtf8( "\xd0\x9a\xd0\x9e\xd0\x94" ) );
    ChequeList->header()->setLabel( 2, trUtf8( "\xd0\x9d\xd0\x90\xd0\x98\xd0\x9c\xd0\x95\xd0\x9d\xd0\x9e\xd0\x92\xd0\x90\xd0\x9d\xd0"
    "\x98\xd0\x95" ) );
    ChequeList->header()->setLabel( 3, trUtf8( "\xd0\xa6\xd0\x95\xd0\x9d\xd0\x90" ) );
    ChequeList->header()->setLabel( 4, trUtf8( "\xd0\x9a\xd0\x9e\xd0\x9b\x2d\xd0\x92\xd0\x9e" ) );
    ChequeList->header()->setLabel( 5, trUtf8( "\xd0\xa1\xd0\xa3\xd0\x9c\xd0\x9c\xd0\x90" ) );
    ChequeList->header()->setLabel( 6, trUtf8( "\xd0\xa1\xd0\x9a\xd0\x98\xd0\x94\xd0\x9a\xd0\x90" ) );
    ChequeList->clear();
    QListViewItem * item = new QListViewItem( ChequeList, 0 );

    lbName->setText( trUtf8( "\xd0\x9d\xd0\xb0\xd0\xb8\xd0\xbc\xd0\xb5\xd0\xbd\xd0\xbe\xd0\xb2\xd0\xb0\xd0\xbd\xd0"
    "\xb8\xd0\xb5" ) );
    MItemPrice->setText( QString::null );
    MItemName->setText( QString::null );
    lbCode->setText( trUtf8( "\xd0\x9a\xd0\xbe\xd0\xb4" ) );
    MItemCode->setText( QString::null );
    lbPrice->setText( trUtf8( "\xd0\xa6\xd0\xb5\xd0\xbd\xd0\xb0" ) );
    lbCount->setText( trUtf8( "\xd0\x9a\xd0\xbe\xd0\xbb\xd0\xb8\xd1\x87\xd0\xb5\xd1\x81\xd1\x82\xd0\xb2\xd0\xbe" ) );
    MItemCount->setText( QString::null );
    lbDiscount->setText( trUtf8( "\xd0\xa1\xd0\xba\xd0\xb8\xd0\xb4\xd0\xba\xd0\xb0" ) );
    MItemDiscount->setText( QString::null );
    lbSum->setText( trUtf8( "\xd0\xa1\xd1\x83\xd0\xbc\xd0\xbc\xd0\xb0" ) );
    lbNumber->setText( tr( "No" ) );
    MItemNumber->setText( QString::null );
    MItemSum->setText( QString::null );
    lbTime->setText( tr( "00:00" ) );
    ServerInfo->setText( trUtf8( "\xd0\x91\xd0\x90\xd0\x97\xd0\x90" ) );
    RegisterInfo->setText( trUtf8( "\xd0\xa4\xd0\xa0" ) );
}

