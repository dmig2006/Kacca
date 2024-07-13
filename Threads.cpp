/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <string.h>
#include <qprogressbar.h>
#include <qapplication.h>
#include <qcursor.h>
#include <xbase/xbase.h>
#include <algorithm>
#include <qdir.h>
#include <qlabel.h>

extern "C"
{
#include <uuid/uuid.h>
}

#include "WorkPlace.h"
#include "Threads.h"
#include "Search.h"
#include "Cash.h"
#include "Utils.h"
#include "Guard.h"
#include "DbfTable.h"
#include "Ping.h"
#include "Exclamation.h"

// nick
#include "ExtControls.h"
#include "Payment.h"

#include <stdlib.h>
#include <time.h>

#define  SQLDAVERSION 1

extern const char *CUSTOMERS,*CUSTRANGE,*STAFF,*BARCONF,*KEYBOARD,*GROUPS,*DISCDESC,*BANKDISC,*DateTemplate;
const char *HostIsInaccessible="Server is inaccessible";

extern xbSchema
	CustomersTableRecord[],CustRangeTableRecord[],StaffTableRecord[],BarInfoTableRecord[],
	KeyboardTableRecord[],GroupsTableRecord[],DiscountsRecord[], BankDiscountRecord[];

const unsigned int MaxTimeout=865;
char * Q_RETURN_CREATE = "create table RETCHECK (CHECKNUM integer,DATESALE DATE,KASSANUM INTEGER,OPER SMALLINT,KKM INTEGER, REGCHECK INTEGER)";

using namespace std;

extern int revision;

ComPortScan::ComPortScan(CashRegister *host,int port,int baud)
{
    srand(time(NULL));
    port_id = rand() % 1000 + 1;

    ;;Log("[ComPort-"+Int2Str(port_id)+"] INIT ");

	CashReg=host;
	if (port*baud!=0)
		hComPort=OpenPort(port,baud,0,100);//OpenPortScan(port,baud,0,100);
	else
		hComPort=0;

	stop_flag=false;//if we opened serial port we launch thread
	suspend = false;
    allsymbols = false;

	if (hComPort!=0)
		start();
}

ComPortScan::~ComPortScan()
{
//    ;;Log("[ComPort-"+Int2Str(port_id)+"] DELETE START");

	stop_flag=true;//stop thread

	while(running()){
		usleep(10000);
	}

	if (hComPort!=0)
		ClosePort(hComPort);
//	;;Log("[ComPort-"+Int2Str(port_id)+"] DELETE END ");
}

void ComPortScan::stop(void)
{
	stop_flag=true;//stop thread
}

//#define SCAN_LENGTH 100
#define SCAN_LENGTH 151

void ComPortScan::run(void)
{
    ;;Log("[ComPort-"+Int2Str(port_id)+"] RUN [START]");

	char buffer[SCAN_LENGTH];

/*
	while((!stop_flag)&&(hComPort!=0))
	{
		ClearPort(hComPort);
		memset(buffer,0,33);

		if(!ReadFromPort(hComPort,buffer,32))
			continue;

		postEvent(CashReg,new ComPortEvent(buffer));
		ClearPort(hComPort);
		Beep();
	}

	return;
//*/
//*
	char b,*pr;
	int count;

	while((!stop_flag)&&(hComPort!=0))
	{
		while(suspend)
			sleep(1);

		ClearPort(hComPort);
		//!memset(buffer,0,SCAN_LENGTH-1);
		memset(buffer,0,SCAN_LENGTH);

		pr=buffer;
		b=0x00;
		count=0;

		if (ReadFromPort(hComPort,&b,1)==0)//try to read something
			continue;

        if (stop_flag) continue;

		*pr=b;
		pr++;
//;;Log("[ComPort-"+Int2Str(port_id)+"] #2");
		do
		{
			if (ReadFromPort(hComPort,&b,1)==0)//read bytes from the serial port
				break;

// "All symbols" Added only to read TEXTILE DataMatrix barcode with invisible symbol 0x1D (groups delimiter)
// in order to send properly barcode to FR (ФФД 1.2)
			if ((isprint(b)!=0) || (allsymbols && b==0x1D))
			{
//			    char s[255];
//			    sprintf(s,"%0*hhX ", 2, b);
//			    ;;Log(s);

				*pr=b;
				pr++;
			}
			count++;
		}
		while(count<SCAN_LENGTH-1);//barcode has to be fewer than 32 symbols
;;Log("[ComPort-"+Int2Str(port_id)+"] #Считан штрихкод. Count ="+Int2Str(count));
        //buffer[SCAN_LENGTH-1]='/0';
		postEvent(CashReg,new ComPortEvent(buffer));
//;;Log("[ComPort-"+Int2Str(port_id)+"] #4");
		//ClosePort(hComPort);
		//while( (hComPort = OpenPort(m_port, m_baud, 0, 100)) == 0);

		Beep();
	}

//;;Log("[ComPort-"+Int2Str(port_id)+"] RUN [END]");
//*/
}

#ifndef OFF_FUNCTION
void ComPortScan::run_old(void)
{

    Log("RUN OLD TEST");

	char buffer[33];
/*
	while((!stop_flag)&&(hComPort!=0))
	{
		ClearPort(hComPort);
		memset(buffer,0,33);

		if(!ReadFromPort(hComPort,buffer,32))
			continue;

		postEvent(CashReg,new ComPortEvent(buffer));
		ClearPort(hComPort);
		Beep();
	}

	return;
//*/
//*
	char b,*pr;
	int count;

	while((!stop_flag)&&(hComPort!=0))
	{
		while(suspend)
			sleep(1);

		ClearPort(hComPort);
		memset(buffer,0,32);

		pr=buffer;
		b=0x00;
		count=0;

		if (ReadFromPort(hComPort,&b,1)==0)//try to read something
			continue;

		*pr=b;
		pr++;

		do
		{
			if (ReadFromPort(hComPort,&b,1)==0)//read bytes from the serial port
				break;

			if (isprint(b)!=0)
			{
				*pr=b;
				pr++;
			}
			count++;
		}
		while(count<32);//barcode has to be fewer than 32 symbols

		postEvent(CashReg,new ComPortEvent(buffer));

		//ClosePort(hComPort);
		//while( (hComPort = OpenPort(m_port, m_baud, 0, 100)) == 0);

		Beep();
	}
//*/
}
#endif

///Записываем в buf штрихкод считанный сканером
/// Выводим данную информацию в лог
/// Buffer Хранит данные считанные сканером
ComPortEvent::ComPortEvent(string buf):QCustomEvent(BarcodeEventType)
{   //Log("[FUNC] ComPortEvent [START]");
    ;;Log("Read Com Port=" + buf);
    //;;Log("[FUNC] after buffer [START]");
	buffer=buf;//store barcode string in the internal buffer
	//;;Log("#l1");
	setData(&buffer);
	//;;Log("[FUNC] ComPortEvent [END]");
}

SQLEvent::SQLEvent(cash_error& er):QCustomEvent(SQLEventType)
{
	error = new cash_error(er.type(),er.code(),er.what());
	setData(error);
}

SQLEvent::~SQLEvent()
{
	delete error;
}

SQLThread::SQLThread(CashRegister *p)
{
	CashReg=p;
	usetimeout=true;
	firststart=true;

#ifdef IBASE
//;;Log("ibtable=NULL (SQLThread(CashRegister *p))");
		ibstmt=ibtable=ibtran=sqlda=NULL;
		dpb=NULL;
#else
		SQLAllocEnv(&henv);
		hdbc=SQL_NULL_HDBC;
		hstmt=SQL_NULL_HSTMT;
#endif

 	try
	{
		CashReg->DbfMutex->lock();

		DBPath=CashReg->ConfigTable->GetStringField("SERVER");//load database settings
		DBLogin=CashReg->ConfigTable->GetStringField("S_LOGIN");
		DBPassword=CashReg->ConfigTable->GetStringField("S_PASSWD");

		ConWaitPeriod=CashReg->ConfigTable->GetLongField("IB_WAIT");

    Log("-------------------------");
    Log("DBPath=" + DBPath);
    Log("DBLogin=" + DBLogin);
    Log("DBPassword=" + DBPassword);
    Log("-------------------------");

		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& er)
	{
		ProcessError(er,false);
	}

	stop_flag=detach_status=false;
}

SQLThread::~SQLThread()
{
	stop_flag=true;

	while(running()){
		usleep(10000);
	}

	DetachFromDB();
}


void SQLThread::InitInstance(void)
{
    ;;Log(">> SQLThread:: InitInstance");
	ConAttempt=time(NULL);//store time of last connection

	CashReg->SQLMutex->lock();

	int tmpWaitPeriod=ConWaitPeriod;


	try
	{
#ifdef IBASE
//;;Log("ibtable=NULL (SQLThread::InitInstance(void))");
		ibstmt=ibtable=ibtran=sqlda=NULL;
		dpb=NULL;
#else
		SQLAllocEnv(&henv);
		hdbc=SQL_NULL_HDBC;
		hstmt=SQL_NULL_HSTMT;
#endif
		SetWaitPeriod(0);
		AttachToDB();
		SetWaitPeriod(tmpWaitPeriod);
	}
	catch(cash_error& er)
	{
		SetWaitPeriod(tmpWaitPeriod);
		ProcessError(er,false);
		return;
	}

    LoadRefers();
}

void SQLThread::LoadRefers(void)
{
    Log(">> Load refers... ");
	CashReg->SQLMutex->unlock();


	thisApp->setOverrideCursor(waitCursor);
	CashReg->InfoBar->setTotalSteps(7);//load settings
	CashReg->InfoMessage->setText(W2U("Чтение настроек"));
	thisApp->processEvents();

	CashReg->InfoBar->setProgress(1);
	CashReg->InfoMessage->setText(W2U("Загрузка персонала"));
	thisApp->processEvents();

	GetStaffList();

	CashReg->InfoBar->setProgress(2);
	CashReg->InfoMessage->setText(W2U("Загрузка настроек клавиатуры"));
	thisApp->processEvents();

	GetKeysInfo();

	CashReg->InfoBar->setProgress(3);
	CashReg->InfoMessage->setText(W2U("Загрузка настроек штрих-кодов"));
	thisApp->processEvents();

	GetBarcodeInfo();

	CashReg->InfoBar->setProgress(4);
	CashReg->InfoMessage->setText(W2U("Загрузка групп товаров"));
	thisApp->processEvents();

	GetGroupsInfo();

	CashReg->InfoBar->setProgress(5);
	CashReg->InfoMessage->setText(W2U("Загрузка схем скидок"));
	thisApp->processEvents();

	GetDiscountInfo();

	CashReg->InfoBar->setProgress(6);
	CashReg->InfoMessage->setText(W2U("Загрузка пользователей дисконтных карт"));
	thisApp->processEvents();

	GetBankDiscount();

	GetCustomers();

	GetMsgLevel();

	//GetDiscountInfo();// +
	//грузить диапазоны тут
	CashReg->InfoBar->setProgress(6);
	CashReg->InfoMessage->setText(W2U("Загрузка диапазонов дисконтных карт"));
	thisApp->processEvents();

	GetCustRange();

	CashReg->InfoBar->reset();
	CashReg->InfoMessage->clear();
	//string verdate = __DATE__;
	//CashReg->InfoMessage->setText(W2U("Вер.2.2 "+ verdate));
	thisApp->restoreOverrideCursor();
}


void SQLThread::AttachToDB(void)
{

#ifdef IBASE
	if (ibtable==NULL)
#else
	if (hdbc==SQL_NULL_HDBC)
#endif
	{
        ;;Log(" >> AttachToDB >>");
        ;;Log((char*)("   = time(NULL) = "+Int2Str(time(NULL))).c_str());
        ;;Log((char*)("   = ConAttempt = "+Int2Str(ConAttempt)).c_str());
        ;;Log((char*)("   = ConWaitPeriod = "+Int2Str(ConWaitPeriod)).c_str());
		if (time(NULL)-ConAttempt<ConWaitPeriod*60)//if timeout has gone we try to establish
		{
		    ;;Log("    er: timeout");

            if (usetimeout)
            {
               throw cash_error(sql_error,0,"");//connection again
            }
            else
            {
                ;;cash_error* _err = new cash_error(10,0," TIMEOUT");
                ;;CashReg->LoggingError(*_err);
                ;;delete _err;
            }
		}

		ConAttempt=time(NULL);

		CashReg->SetSQLStatus(true);

		ConnectionStatus=true;

		if (!PingServer())
		{
		    ;;Log(" er: ping 1");
			throw cash_error(ping_error,0,HostIsInaccessible);
		}
/*
		if (!PingServer()){
			if (time(NULL)-ConAttempt<ConWaitPeriod*60)//if timeout has gone we try to establish
				throw cash_error(sql_error,0,"");//connection again
			ConAttempt=time(NULL);
			throw cash_error(ping_error,0,HostIsInaccessible);
		}
//		ConAttempt=time(NULL);
		CashReg->SetSQLStatus(true);
		ConnectionStatus=true;
*/
#ifdef IBASE
		char ISC_FAR dpb_buffer[256];
		dpb_buffer[0]=isc_dpb_version1;
		dpb_length=1;
		char ISC_FAR *temp_dpb=dpb_buffer;
		string r = CashReg->ConfigTable->GetStringField("S_ROLE");
		if(r.empty()){
		isc_expand_dpb(&temp_dpb,&dpb_length,isc_dpb_user_name,DBLogin.c_str(),
			isc_dpb_password,DBPassword.c_str(),isc_dpb_lc_ctype,"WIN1251",NULL);
		}else{
		isc_expand_dpb(&temp_dpb,&dpb_length,isc_dpb_user_name,DBLogin.c_str(),
			isc_dpb_password,DBPassword.c_str(),isc_dpb_lc_ctype,"WIN1251",
			isc_dpb_sql_role_name, r.c_str(), NULL);
		}

		dpb=new char[dpb_length];
		memcpy(dpb,temp_dpb,dpb_length);
;;Log("  > SQLConnect 1");
		isc_attach_database(status,DBPath.length(),(char*)DBPath.c_str(),&ibtable,dpb_length,dpb);
;;Log("  > end < SQLConnect 1");
		if (!((status[0]==1)&&(status[1]==0)))
#else
		string server=DBPath;
		if (server.find(":")!=server.npos)
			server=server.substr(server.find(":")+1);
;;Log("  > SQLConnect 2");
		SQLAllocConnect(henv,&hdbc);
		if (SQLConnect(hdbc,(unsigned char*)server.c_str(),SQL_NTS,(unsigned char*)DBLogin.c_str(),
			SQL_NTS,(unsigned char*)DBPassword.c_str(),SQL_NTS)!=SQL_SUCCESS)
#endif
        {
            ;;Log(" er: SQLConnect");
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
        }
	}
	else
		if (!PingServer())
		{
		    ;;Log(" er: ping 2");
			throw cash_error(ping_error,0,HostIsInaccessible);
		}
}

string SQLThread::GetErrorDescription(void)
{
	string msg;//get description of the last SQL error

#ifdef IBASE
	ISC_STATUS *pvector;
	pvector=status;
	char ib_msg[512];
	while (isc_interprete(ib_msg,&pvector)>0)
	{
		msg+=ib_msg;
		if (ib_msg[strlen(ib_msg)-1]!=',')
			msg+=',';
	}
#else
	SQLTCHAR szSqlState[6];
	SQLINTEGER fNativeError;
	SQLTCHAR szErrorMsg[SQL_MAX_MESSAGE_LENGTH+1];
	SQLSMALLINT cbErrorMessage;
	if (SQLError(henv,hdbc,hstmt,szSqlState,&fNativeError,szErrorMsg,
		SQL_MAX_MESSAGE_LENGTH+1,&cbErrorMessage)==SQL_SUCCESS)
		msg=(char*)szErrorMsg;
#endif
	return msg;
}

void SQLThread::DetachFromDB(void)//detach from database
{
	if (PingServer())
	{
#ifdef IBASE
		if (ibstmt!=NULL){
			Log("isc_dsql_free_statement(status,&ibstmt,DSQL_close);");
			isc_dsql_free_statement(status,&ibstmt,DSQL_close);
		}
		if (ibtable!=NULL){
			Log("isc_detach_database(status,&ibtable);");
			isc_detach_database(status,&ibtable);
		}
		if (sqlda!=NULL){
			Log("delete sqlda;");
			delete sqlda;
		}
		if (dpb!=NULL){
			Log("delete dpb;");
			delete dpb;
		}
	}
	dpb=NULL;
	;;Log("ibtran =NULL (DetachFromDB(void))" );
	ibstmt=ibtable=ibtran=sqlda=NULL;
#else
		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);
		if (hdbc!=SQL_NULL_HDBC)
			SQLDisconnect(hdbc);
		SQLFreeConnect(hdbc);
	}
	hstmt=SQL_NULL_HSTMT;
	hdbc=SQL_NULL_HDBC;
#endif
}

void SQLThread::StartTran(void)//start transaction
{
	if (!PingServer())
		throw cash_error(ping_error,0,HostIsInaccessible);
#ifdef IBASE
	char isc_tpb[]={isc_tpb_version3,isc_tpb_write,isc_tpb_read_committed,isc_tpb_nowait,isc_tpb_rec_version,};

	ISC_TEB teb;
	teb.db_ptr=(long*)&ibtable;
	teb.tpb_len=5;
	teb.tpb_ptr=isc_tpb;//since the bug in the IB 5.x take place we use this function

//	Log("isc_start_multiple(status,&ibtran,1,&teb)");
	if (isc_start_multiple(status,&ibtran,1,&teb))
#else
	if (SQLSetConnectOption(hdbc,SQL_AUTOCOMMIT,SQL_AUTOCOMMIT_OFF)!=SQL_SUCCESS)
#endif
	{
		/*
		char s[512];
		sprintf(s, "%s", GetErrorDescription().c_str());
		Log("throw cash_error(sql_error,0,GetErrorDescription().c_str());", s);
		sprintf(s, "status[1] = %d", status[1]);
		Log(s);
		*/
		throw cash_error(sql_error,0,GetErrorDescription().c_str());
	}
//	Log("end StartTran(void)");
}

void SQLThread::CommitTran(void)//commit transaction
{
	if (!PingServer())
		throw cash_error(ping_error,0,HostIsInaccessible);
#ifdef IBASE
	if (isc_commit_transaction(status,&ibtran))
#else
	if (SQLTransact(henv,hdbc,SQL_COMMIT)!=SQL_SUCCESS)
#endif
		throw cash_error(sql_error,0,GetErrorDescription().c_str());
}

void SQLThread::GetStaffList(void)
{
	vector<StaffStruct> StaffList;

	try
	{
		CashReg->SQLMutex->lock();

		AttachToDB(); //copy staff's description to the local .dbf

		StartTran();

		CashReg->DbfMutex->lock();
		string sel_str="select id,name,man_login,security,role_type,rights from mgstaff \
			where isactive=1 and cash_desk=0 or cash_desk=";
		sel_str+=Int2Str(CashReg->ConfigTable->GetLongField("CASH_DESK"));
		CashReg->DbfMutex->unlock();


#ifdef IBASE
		long ID;
		SQL_VARCHAR(40) Name;
		SQL_VARCHAR(10) Login;
		SQL_VARCHAR(20) Passwd;
		long Code;
		SQL_VARCHAR(35) Rights;

		StaffList.clear();

		short id_flag=0,login_flag=0,name_flag=0,passwd_flag=0,code_flag=0,rights_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
 		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(6)];
		sqlda->sqln = 6;
		sqlda->sqld = 6;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,(char*)sel_str.c_str(),1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char ISC_FAR *)&ID;
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &id_flag;

		sqlda->sqlvar[1].sqldata = (char ISC_FAR *)&Name;
		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[1].sqlind  = &name_flag;

		sqlda->sqlvar[2].sqldata = (char ISC_FAR *)&Login;
		sqlda->sqlvar[2].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[2].sqlind  = &login_flag;

		sqlda->sqlvar[3].sqldata = (char ISC_FAR *)&Passwd;
		sqlda->sqlvar[3].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[3].sqlind  = &passwd_flag;

		sqlda->sqlvar[4].sqldata = (char ISC_FAR *)&Code;
		sqlda->sqlvar[4].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[4].sqlind  = &code_flag;

		sqlda->sqlvar[5].sqldata = (char ISC_FAR *)&Rights;
		sqlda->sqlvar[5].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[5].sqlind  = &rights_flag;

		if (isc_dsql_execute(status, &ibtran, &ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			StaffStruct tmpStaff;
			int i;

			tmpStaff.Name="";

			if (name_flag==0)
				for (i=0;i<Name.vary_length;i++)
					tmpStaff.Name+=Name.vary_string[i];

			tmpStaff.Login="";

			if (login_flag==0)
				for (i=0;i<Login.vary_length;i++)
					tmpStaff.Login+=Login.vary_string[i];

			tmpStaff.Passwd="";

			if (passwd_flag==0)
				for (i=0;i<Passwd.vary_length;i++)
					tmpStaff.Passwd+=Passwd.vary_string[i];

			if (code_flag==0)
				tmpStaff.Code=Code;
			else
				tmpStaff.Code=0;

			if (id_flag==0)
				tmpStaff.Id=ID;
			else
				tmpStaff.Id=0;

			tmpStaff.Rights="";
			if (rights_flag==0)
				for (i=0;i<Rights.vary_length;i++)
					tmpStaff.Rights+=Rights.vary_string[i];

			if (tmpStaff.Id > 0) StaffList.insert(StaffList.end(),tmpStaff);// "if()" -
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		long ID;
		char Name[41];
		char Login[11];
		char Passwd[21];
		long Code;
		char Rights[36];
		long id_flag,login_flag,name_flag,passwd_flag,code_flag,rights_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			StaffStruct tmpStaff;

			SQLGetData(hstmt,1,SQL_C_LONG,&ID,sizeof(long),&id_flag);
			SQLGetData(hstmt,2,SQL_C_CHAR,Name,41,&name_flag);
			SQLGetData(hstmt,3,SQL_C_CHAR,Login,11,&login_flag);
			SQLGetData(hstmt,4,SQL_C_CHAR,Passwd,41,&passwd_flag);
			SQLGetData(hstmt,5,SQL_C_LONG,&Code,sizeof(long),&code_flag);
			SQLGetData(hstmt,6,SQL_C_CHAR,Rights,36,&rights_flag);

			if (name_flag!=SQL_NULL_DATA)
				tmpStaff.Name=Name;
			else
				tmpStaff.Name="";
			if (login_flag!=SQL_NULL_DATA)
				tmpStaff.Login=Login;
			else
				tmpStaff.Login="";
			if (passwd_flag!=SQL_NULL_DATA)
				tmpStaff.Passwd=Passwd;
			else
				tmpStaff.Passwd="";
			if (code_flag!=SQL_NULL_DATA)
				tmpStaff.Code=Code;
			else
				tmpStaff.Code=0;
			if (id_flag!=SQL_NULL_DATA)
				tmpStaff.Id=ID;
			else
				tmpStaff.Id=0;
			if (rights_flag!=SQL_NULL_DATA)
				tmpStaff.Rights=Rights;
			else
				tmpStaff.Rights="";

			//StaffList.insert(StaffList.end(),tmpStaff);
			if (tmpStaff.Id > 0) StaffList.insert(StaffList.end(),tmpStaff);// "if()" -
		}


		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);
		hstmt=SQL_NULL_HSTMT;
#endif
		CashReg->SQLMutex->unlock();

		CashReg->DbfMutex->lock();

		CashReg->StaffTable->CloseDatabase();//write staff's information into local table
		CashReg->StaffTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+STAFF).c_str(),StaffTableRecord);

		for (unsigned int i=0;i<StaffList.size();i++)
		{
			CashReg->StaffTable->BlankRecord();

			CashReg->StaffTable->PutLongField("ID",StaffList[i].Id);
			CashReg->StaffTable->PutField("NAME",StaffList[i].Name.c_str());
			CashReg->StaffTable->PutField("MAN_LOGIN",StaffList[i].Login.c_str());
			CashReg->StaffTable->PutField("SECURITY",StaffList[i].Passwd.c_str());
			CashReg->StaffTable->PutLongField("ROLE_TYPE",StaffList[i].Code);
			CashReg->StaffTable->PutField("RIGHTS",StaffList[i].Rights.c_str());

			CashReg->StaffTable->AppendRecord();
		}

		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
}

void SQLThread::GetKeysInfo(void)
{
	vector<string> KeyName;
	vector<short> KeyCode;

	KeyName.clear();KeyCode.clear();

	try
	{
		CashReg->SQLMutex->lock();

		AttachToDB();

		StartTran(); //load keys parameters

		char *sel_str="select name,keycode from mgkeyboard";

#ifdef IBASE
		SQL_VARCHAR(40) Name;
		short Code;
		short name_flag=0,code_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(2)];
		sqlda->sqln = 2;
		sqlda->sqld = 2;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&Name;
		sqlda->sqlvar[0].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[0].sqlind  = &name_flag;

		sqlda->sqlvar[1].sqldata = (char ISC_FAR*)&Code;
		sqlda->sqlvar[1].sqltype = SQL_SHORT + 1;
		sqlda->sqlvar[1].sqlind  = &code_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			string TmpName="";

			if (name_flag==0)
				for (int i=0;i<Name.vary_length;i++)
					TmpName+=Name.vary_string[i];

			KeyName.insert(KeyName.end(),TmpName);

			if (code_flag==0)
				KeyCode.insert(KeyCode.end(),Code);
			else
				KeyCode.insert(KeyCode.end(),0);
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		char Name[41];
		short Code;
		SQLINTEGER name_flag,code_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str,SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			SQLGetData(hstmt,1,SQL_C_CHAR,Name,41,&name_flag);
			SQLGetData(hstmt,2,SQL_C_SHORT,&Code,sizeof(short),&code_flag);

			if (name_flag!=SQL_NULL_DATA)
				KeyName.insert(KeyName.end(),Name);
			else
				KeyName.insert(KeyName.end(),"");
			if (code_flag!=SQL_NULL_DATA)
				KeyCode.insert(KeyCode.end(),Code);
			else
				KeyCode.insert(KeyCode.end(),0);
		}

		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);
		hstmt=SQL_NULL_HSTMT;
#endif
		CashReg->SQLMutex->unlock();

		CashReg->DbfMutex->lock();

		CashReg->KeyboardTable->CloseDatabase();//store keyboard settings in the local table
		CashReg->KeyboardTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+KEYBOARD).c_str(),KeyboardTableRecord);

		for (unsigned int i=0;i<KeyName.size();i++)
		{
			CashReg->KeyboardTable->BlankRecord();
			CashReg->KeyboardTable->PutField("NAME",KeyName[i].c_str());
			CashReg->KeyboardTable->PutLongField("KEYCODE",KeyCode[i]);
			CashReg->KeyboardTable->AppendRecord();
		}

		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
}

void SQLThread::GetBarcodeInfo(void)//copy barcode information to the local table
{
	vector<BarcodeInfo> Barcodes;

	try
	{
		CashReg->SQLMutex->lock();

		AttachToDB();

		StartTran();

		char *sel_str="select codename,codeid from barcodeinfo order by len desc";

#ifdef IBASE
		SQL_VARCHAR(20) CodeName;
		SQL_VARCHAR(4) CodeID;
		short name_flag=0,id_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(2)];
		sqlda->sqln = 2;
		sqlda->sqld = 2;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;


		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char *)&CodeName;
		sqlda->sqlvar[0].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[0].sqlind  = &name_flag;

		sqlda->sqlvar[1].sqldata = (char *)&CodeID;
		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[1].sqlind  = &id_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			BarcodeInfo tmpInfo;
			int i;

			tmpInfo.CodeName=tmpInfo.CodeID="";

			if (name_flag==0)
				for (i=0;i<CodeName.vary_length;i++)
					if (CodeName.vary_string[i]!=' ')
						tmpInfo.CodeName+=CodeName.vary_string[i];
					else
						break;

			if (id_flag==0)
				for (i=0;i<CodeID.vary_length;i++)
					if (CodeID.vary_string[i]!=' ')
						tmpInfo.CodeID+=CodeID.vary_string[i];
				else
					break;

			Barcodes.insert(Barcodes.end(),tmpInfo);
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		char CodeName[21];
		char CodeID[5];
		SQLINTEGER name_flag,id_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str,SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			SQLGetData(hstmt,1,SQL_C_CHAR,CodeName,21,&name_flag);
			SQLGetData(hstmt,2,SQL_C_CHAR,CodeID,5,&id_flag);

			BarcodeInfo tmpInfo;
			if (name_flag!=SQL_NULL_DATA)
				tmpInfo.CodeName=CodeName;
			else
				tmpInfo.CodeName="";
			if (id_flag!=SQL_NULL_DATA)
				tmpInfo.CodeID=CodeID;
			else
				tmpInfo.CodeID="";
			Barcodes.insert(Barcodes.end(),tmpInfo);
		}

		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);
		hstmt=SQL_NULL_HSTMT;
#endif
		CashReg->SQLMutex->unlock();

		CashReg->DbfMutex->lock();
		CashReg->BarInfoTable->CloseDatabase();//store barcode's information into local table

		CashReg->BarInfoTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+BARCONF).c_str(),BarInfoTableRecord);
		for (unsigned int i=0;i<Barcodes.size();i++)
		{
			CashReg->BarInfoTable->BlankRecord();
			CashReg->BarInfoTable->PutField("CODENAME",Barcodes[i].CodeName.c_str());
			CashReg->BarInfoTable->PutField("CODEID",Barcodes[i].CodeID.c_str());
			CashReg->BarInfoTable->PutLongField("LEN",Barcodes[i].CodeID.length());
			CashReg->BarInfoTable->AppendRecord();
		}
		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
}

void SQLThread::GetGroupsInfo(void)//copy section's description into local table
{
	vector<string> S_Name;
	vector<int> GrCode;

	S_Name.clear();GrCode.clear();

	try
	{
		CashReg->SQLMutex->lock();

		AttachToDB();

		StartTran();

		char *sel_str="select groupcode,groupname from mggroups order by groupcode";

#ifdef IBASE
		SQL_VARCHAR(80) Name;
		long Code;
		short name_flag=0,code_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(2)];
		sqlda->sqln = 2;
		sqlda->sqld = 2;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&Code;
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &code_flag;

		sqlda->sqlvar[1].sqldata = (char ISC_FAR*)&Name;
		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[1].sqlind  = &name_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			string TmpName="";

			if (name_flag==0)
				for (int i=0;i<Name.vary_length;i++)
					TmpName+=Name.vary_string[i];

			S_Name.insert(S_Name.end(),TmpName);

			if (code_flag==0)
				GrCode.insert(GrCode.end(),Code);
			else
				GrCode.insert(GrCode.end(),0);
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		char Name[81];
		long Code;
		SQLINTEGER name_flag,code_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str,SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			SQLGetData(hstmt,1,SQL_C_LONG,&Code,sizeof(long),&code_flag);
			SQLGetData(hstmt,2,SQL_C_CHAR,Name,81,&name_flag);

			if (name_flag!=SQL_NULL_DATA)
				S_Name.insert(S_Name.end(),Name);
			else
				S_Name.insert(S_Name.end(),"");
			if (code_flag!=SQL_NULL_DATA)
				GrCode.insert(GrCode.end(),Code);
			else
				GrCode.insert(GrCode.end(),0);
		}

		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);
		hstmt=SQL_NULL_HSTMT;
#endif
		CashReg->SQLMutex->unlock();

		CashReg->DbfMutex->lock();
		CashReg->GroupsTable->CloseDatabase();//store section's information into local table
		CashReg->GroupsTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+GROUPS).c_str(),GroupsTableRecord);

		for (unsigned int i=0;i<S_Name.size();i++)
		{
			CashReg->GroupsTable->BlankRecord();
			CashReg->GroupsTable->PutField("S_NAME",S_Name[i].c_str());
			CashReg->GroupsTable->PutLongField("GRCODE",GrCode[i]);
			CashReg->GroupsTable->AppendRecord();
		}
		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
}


#ifndef NEW_CUSTOMER

void SQLThread::GetCustomers(void)//copy customer's list into local table
{
	vector<CustomerStruct> Customers;

//	DWORD time = GetTickCount();
	try
	{
		CashReg->SQLMutex->lock();

		AttachToDB();

		StartTran();

		char *sel_str="select id,discounts,issuedate,expdate from mgcustomer";

#ifdef IBASE
		long ID;
		SQL_VARCHAR(MAXDISCOUNTLEN) Discounts;
		ISC_QUAD IssueDate,ExpDate;
		short id_flag=0,discounts_flag=0,issue_flag=0,exp_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(4)];
		sqlda->sqln = 4;
		sqlda->sqld = 4;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char *)&ID;//^^
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &id_flag;

		sqlda->sqlvar[1].sqldata = (char *)&Discounts;
		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[1].sqlind  = &discounts_flag;

		sqlda->sqlvar[2].sqldata = (char *)&IssueDate;
		sqlda->sqlvar[2].sqltype = SQL_DATE+1;
		sqlda->sqlvar[2].sqlind = &issue_flag;

		sqlda->sqlvar[3].sqldata = (char *)&ExpDate;
		sqlda->sqlvar[3].sqltype = SQL_DATE+1;
		sqlda->sqlvar[3].sqlind = &exp_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			CustomerStruct tmpCustomer;
			int i;

			if (id_flag==0)
				tmpCustomer.Id=ID;
			else
				tmpCustomer.Id=0;

			tmpCustomer.Discounts="";
			if (discounts_flag==0)
				for (i=0;i<Discounts.vary_length;i++)
					tmpCustomer.Discounts+=Discounts.vary_string[i];

			if (issue_flag==0)
				isc_decode_date(&IssueDate,&(tmpCustomer.issue));
			else
				tmpCustomer.issue.tm_mday=tmpCustomer.issue.tm_mon=tmpCustomer.issue.tm_year=0;

			if (exp_flag==0)
				isc_decode_date(&ExpDate,&(tmpCustomer.expire));
			else
				tmpCustomer.expire.tm_mday=tmpCustomer.expire.tm_mon=tmpCustomer.expire.tm_year=0;

			Customers.insert(Customers.end(),tmpCustomer);
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());






		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		long ID;

		char Discounts[21];

		char IssueDate[3*sizeof(short)],ExpDate[3*sizeof(short)];
		SQLINTEGER id_flag,discounts_flag,issue_flag,exp_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str,SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			SQLGetData(hstmt,1,SQL_C_LONG,&ID,sizeof(long),&id_flag);
			SQLGetData(hstmt,2,SQL_C_CHAR,Discounts,21,&discounts_flag);
			SQLGetData(hstmt,3,SQL_C_DATE,IssueDate,3*sizeof(short),&issue_flag);
			SQLGetData(hstmt,4,SQL_C_DATE,ExpDate,3*sizeof(short),&exp_flag);

			CustomerStruct tmpCustomer;

			if (id_flag!=SQL_NULL_DATA)
				tmpCustomer.Id=ID;
			else
				tmpCustomer.Id=0;

			if (discounts_flag!=SQL_NULL_DATA)
				tmpCustomer.Discounts=Discounts;
			else
				tmpCustomer.Discounts="";

			if (issue_flag!=SQL_NULL_DATA)
			{
				tmpCustomer.issue.tm_year=((short*)IssueDate)[0]-1900;

				tmpCustomer.issue.tm_mon=((short*)IssueDate)[1]-1;
				tmpCustomer.issue.tm_mday=((short*)IssueDate)[2];
			}
			else
				tmpCustomer.issue.tm_mday=tmpCustomer.issue.tm_mon=tmpCustomer.issue.tm_year=0;
			if (exp_flag!=SQL_NULL_DATA)
			{
				tmpCustomer.expire.tm_year=((short*)ExpDate)[0]-1900;
				tmpCustomer.expire.tm_mon=((short*)ExpDate)[1]-1;
				tmpCustomer.expire.tm_mday=((short*)ExpDate)[2];
			}
			else
				tmpCustomer.expire.tm_mday=tmpCustomer.expire.tm_mon=tmpCustomer.expire.tm_year=0;

			Customers.insert(Customers.end(),tmpCustomer);
		}

		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		hstmt=SQL_NULL_HSTMT;
#endif
		CashReg->SQLMutex->unlock();

		CashReg->DbfMutex->lock();
		CashReg->CustomersTable->CloseIndices();
		CashReg->CustomersTable->CloseDatabase();//store customer's list into local table
		CashReg->CustomersTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+CUSTOMERS).c_str(),CustomersTableRecord);

		for (unsigned int i=0;i<Customers.size();i++)
   		{
			CashReg->CustomersTable->BlankRecord();
			CashReg->CustomersTable->PutLongField("ID",Customers[i].Id);
			CashReg->CustomersTable->PutField("DISCOUNTS",Customers[i].Discounts.c_str());
			char date_buf[9];date_buf[8]='\0';
			if (Customers[i].issue.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",Customers[i].issue.tm_year+1900);
				sprintf(date_buf+4,"%02d",Customers[i].issue.tm_mon+1);
				sprintf(date_buf+6,"%02d",Customers[i].issue.tm_mday);
				CashReg->CustomersTable->PutField("ISSUE",date_buf);
			}
			else
				CashReg->CustomersTable->PutField("ISSUE","19000101");

			if (Customers[i].expire.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",Customers[i].expire.tm_year+1900);
				sprintf(date_buf+4,"%02d",Customers[i].expire.tm_mon+1);
				sprintf(date_buf+6,"%02d",Customers[i].expire.tm_mday);
				CashReg->CustomersTable->PutField("EXPIRE",date_buf);
			}
			else
				CashReg->CustomersTable->PutField("EXPIRE","21000101");
			CashReg->CustomersTable->AppendRecord();
		}

		CashReg->CustomersTable->CreateIndex((CashReg->ConfigTable->GetStringField("MAIN_PATH")+
			QDir::separator()+"data"+QDir::separator()+"customer.ndx").c_str(),"ID");

		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
//	time = GetTickCount() - time;
}

#elif NEW_CUSTOMER == 0

// Nick
void SQLThread::GetCustomers(void)//copy customer's list into local table
{
//	vector<CustomerStruct> Customers;

//	DWORD time = GetTickCount();
	try
	{
		CashReg->DbfMutex->lock();
		CashReg->CustomersTable->CloseIndices();
		CashReg->CustomersTable->CloseDatabase();//store customer's list into local table
		CashReg->DbfMutex->unlock();

		CashReg->SQLMutex->lock();
		CashReg->CustomersTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+"t"+CUSTOMERS).c_str(),CustomersTableRecord);

//		CashReg->CustomersTable->CreateIndex((CashReg->ConfigTable->GetStringField("MAIN_PATH")+
//			QDir::separator()+"data"+QDir::separator()+"customer.ndx").c_str(),"ID");

		AttachToDB();

		StartTran();

		char *sel_str="select id,discounts,issuedate,expdate from mgcustomer";

#ifdef IBASE
		long ID;
		SQL_VARCHAR(MAXDISCOUNTLEN) Discounts;
		ISC_QUAD IssueDate,ExpDate;
		short id_flag=0,discounts_flag=0,issue_flag=0,exp_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(4)];
		sqlda->sqln = 4;
		sqlda->sqld = 4;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char *)&ID;
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &id_flag;

		sqlda->sqlvar[1].sqldata = (char *)&Discounts;
		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[1].sqlind  = &discounts_flag;

		sqlda->sqlvar[2].sqldata = (char *)&IssueDate;
		sqlda->sqlvar[2].sqltype = SQL_DATE+1;
		sqlda->sqlvar[2].sqlind = &issue_flag;

		sqlda->sqlvar[3].sqldata = (char *)&ExpDate;
		sqlda->sqlvar[3].sqltype = SQL_DATE+1;
		sqlda->sqlvar[3].sqlind = &exp_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			CustomerStruct tmpCustomer;
			int i;

			if (id_flag==0)
				tmpCustomer.Id=ID;
			else
				tmpCustomer.Id=0;

			tmpCustomer.Discounts="";
			if (discounts_flag==0)
				for (i=0;i<Discounts.vary_length;i++)
					tmpCustomer.Discounts+=Discounts.vary_string[i];

			if (issue_flag==0)
				isc_decode_date(&IssueDate,&(tmpCustomer.issue));
			else
				tmpCustomer.issue.tm_mday=tmpCustomer.issue.tm_mon=tmpCustomer.issue.tm_year=0;

			if (exp_flag==0)
				isc_decode_date(&ExpDate,&(tmpCustomer.expire));
			else
				tmpCustomer.expire.tm_mday=tmpCustomer.expire.tm_mon=tmpCustomer.expire.tm_year=0;

//			Customers.insert(Customers.end(),tmpCustomer);

			CashReg->DbfMutex->lock();
			CashReg->CustomersTable->BlankRecord();
			CashReg->CustomersTable->PutLongField("ID",tmpCustomer.Id);
			CashReg->CustomersTable->PutField("DISCOUNTS",tmpCustomer.Discounts.c_str());
			char date_buf[9];date_buf[8]='\0';
			if (tmpCustomer.issue.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",tmpCustomer.issue.tm_year+1900);
				sprintf(date_buf+4,"%02d",tmpCustomer.issue.tm_mon+1);
				sprintf(date_buf+6,"%02d",tmpCustomer.issue.tm_mday);
				CashReg->CustomersTable->PutField("ISSUE",date_buf);
			}
			else
				CashReg->CustomersTable->PutField("ISSUE","19000101");

			if (tmpCustomer.expire.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",tmpCustomer.expire.tm_year+1900);
				sprintf(date_buf+4,"%02d",tmpCustomer.expire.tm_mon+1);
				sprintf(date_buf+6,"%02d",tmpCustomer.expire.tm_mday);
				CashReg->CustomersTable->PutField("EXPIRE",date_buf);
			}
			else
				CashReg->CustomersTable->PutField("EXPIRE","21000101");
			CashReg->CustomersTable->AppendRecord();
			CashReg->DbfMutex->unlock();
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		long ID;

		char Discounts[21];

		char IssueDate[3*sizeof(short)],ExpDate[3*sizeof(short)];
		SQLINTEGER id_flag,discounts_flag,issue_flag,exp_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str,SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			SQLGetData(hstmt,1,SQL_C_LONG,&ID,sizeof(long),&id_flag);
			SQLGetData(hstmt,2,SQL_C_CHAR,Discounts,21,&discounts_flag);
			SQLGetData(hstmt,3,SQL_C_DATE,IssueDate,3*sizeof(short),&issue_flag);
			SQLGetData(hstmt,4,SQL_C_DATE,ExpDate,3*sizeof(short),&exp_flag);

			CustomerStruct tmpCustomer;

			if (id_flag!=SQL_NULL_DATA)
				tmpCustomer.Id=ID;
			else
				tmpCustomer.Id=0;

			if (discounts_flag!=SQL_NULL_DATA)
				tmpCustomer.Discounts=Discounts;
			else
				tmpCustomer.Discounts="";

			if (issue_flag!=SQL_NULL_DATA)
			{
				tmpCustomer.issue.tm_year=((short*)IssueDate)[0]-1900;

				tmpCustomer.issue.tm_mon=((short*)IssueDate)[1]-1;
				tmpCustomer.issue.tm_mday=((short*)IssueDate)[2];
			}
			else
				tmpCustomer.issue.tm_mday=tmpCustomer.issue.tm_mon=tmpCustomer.issue.tm_year=0;
			if (exp_flag!=SQL_NULL_DATA)
			{
				tmpCustomer.expire.tm_year=((short*)ExpDate)[0]-1900;
				tmpCustomer.expire.tm_mon=((short*)ExpDate)[1]-1;
				tmpCustomer.expire.tm_mday=((short*)ExpDate)[2];
			}
			else
				tmpCustomer.expire.tm_mday=tmpCustomer.expire.tm_mon=tmpCustomer.expire.tm_year=0;

//			Customers.insert(Customers.end(),tmpCustomer);

			CashReg->DbfMutex->lock();
			CashReg->CustomersTable->BlankRecord();
			CashReg->CustomersTable->PutLongField("ID",tmpCustomer.Id);
			CashReg->CustomersTable->PutField("DISCOUNTS",tmpCustomer.Discounts.c_str());
			char date_buf[9];date_buf[8]='\0';
			if (tmpCustomer.issue.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",tmpCustomer.issue.tm_year+1900);
				sprintf(date_buf+4,"%02d",tmpCustomer.issue.tm_mon+1);
				sprintf(date_buf+6,"%02d",tmpCustomer.issue.tm_mday);
				CashReg->CustomersTable->PutField("ISSUE",date_buf);
			}
			else
				CashReg->CustomersTable->PutField("ISSUE","19000101");

			if (tmpCustomer.expire.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",tmpCustomer.expire.tm_year+1900);
				sprintf(date_buf+4,"%02d",tmpCustomer.expire.tm_mon+1);
				sprintf(date_buf+6,"%02d",tmpCustomer.expire.tm_mday);
				CashReg->CustomersTable->PutField("EXPIRE",date_buf);
			}
			else
				CashReg->CustomersTable->PutField("EXPIRE","21000101");
			CashReg->CustomersTable->AppendRecord();
			CashReg->DbfMutex->unlock();
		}

		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		hstmt=SQL_NULL_HSTMT;
#endif
		CashReg->SQLMutex->unlock();

		CashReg->CustomersTable->CloseDatabase();//store customer's list into local table

		remove((CashReg->DBGoodsPath+QDir::separator()+"tt"+CUSTOMERS).c_str());
		rename((CashReg->DBGoodsPath+QDir::separator()+CUSTOMERS).c_str(),
			(CashReg->DBGoodsPath+QDir::separator()+"tt"+CUSTOMERS).c_str());
		if(rename((CashReg->DBGoodsPath+QDir::separator()+"t"+CUSTOMERS).c_str(),
			(CashReg->DBGoodsPath+QDir::separator()+CUSTOMERS).c_str()))
		{
			rename((CashReg->DBGoodsPath+QDir::separator()+"tt"+CUSTOMERS).c_str(),
				(CashReg->DBGoodsPath+QDir::separator()+CUSTOMERS).c_str());
			CashReg->ShowMessage("Не могу переименовать список дисконтных карт!");
		}
		remove((CashReg->DBGoodsPath+QDir::separator()+"tt"+CUSTOMERS).c_str());

		CashReg->DbfMutex->lock();
		CashReg->CustomersTable->OpenDatabase((CashReg->DBGoodsPath+QDir::separator()+CUSTOMERS).c_str());
		CashReg->CustomersTable->CreateIndex((CashReg->ConfigTable->GetStringField("MAIN_PATH")+
			QDir::separator()+"data"+QDir::separator()+"customer.ndx").c_str(),"ID");

		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
//	time = GetTickCount() - time;
}

#else

// Загрузка из импорт директории (реализована в CashRegister::GetCustomers())
void SQLThread::GetCustomers(void)//copy customer's list into local table
{
/*
//	DWORD time = GetTickCount();
	FILE * f;

	if(!(f = fopen((CashReg->DBImportPath+QDir::separator()+CUSTOMERS).c_str(), "r"))){
		//GetCustomersOld();
		return;
	}
	fclose(f);

	try
	{
		CashReg->DbfMutex->lock();
		CashReg->CustomersTable->CloseIndices();
		CashReg->CustomersTable->CloseDatabase();//store customer's list into local table
		CashReg->DbfMutex->unlock();

		remove((CashReg->DBGoodsPath+QDir::separator()+"tt"+CUSTOMERS).c_str());
		rename((CashReg->DBGoodsPath+QDir::separator()+CUSTOMERS).c_str(),
			(CashReg->DBGoodsPath+QDir::separator()+"tt"+CUSTOMERS).c_str());
		if(rename((CashReg->DBImportPath+QDir::separator()+CUSTOMERS).c_str(),
			(CashReg->DBGoodsPath+QDir::separator()+CUSTOMERS).c_str()))
		{
			rename((CashReg->DBGoodsPath+QDir::separator()+"tt"+CUSTOMERS).c_str(),
				(CashReg->DBGoodsPath+QDir::separator()+CUSTOMERS).c_str());
			CashReg->ShowMessage("Не могу переименовать список дисконтных карт!");
		}
		remove((CashReg->DBGoodsPath+QDir::separator()+"tt"+CUSTOMERS).c_str());

		CashReg->DbfMutex->lock();
		CashReg->CustomersTable->OpenDatabase((CashReg->DBGoodsPath+QDir::separator()+CUSTOMERS).c_str());

		if(!(f = fopen((CashReg->DBImportPath+QDir::separator()+"customer.ndx").c_str(), "r"))){
			CashReg->CustomersTable->CreateIndex((CashReg->ConfigTable->GetStringField("MAIN_PATH")+
				QDir::separator()+"data"+QDir::separator()+"customer.ndx").c_str(),"ID");
		}else{
			fclose(f);
			remove((CashReg->DBGoodsPath+QDir::separator()+"ttcustomer.ndx").c_str());
			rename((CashReg->DBGoodsPath+QDir::separator()+"customer.ndx").c_str(),
				(CashReg->DBGoodsPath+QDir::separator()+"ttcustomer.ndx").c_str());
			if(rename((CashReg->DBImportPath+QDir::separator()+"customer.ndx").c_str(),
				(CashReg->DBGoodsPath+QDir::separator()+"customer.ndx").c_str()))
			{
				CashReg->CustomersTable->CreateIndex((CashReg->ConfigTable->GetStringField("MAIN_PATH")+
					QDir::separator()+"data"+QDir::separator()+"customer.ndx").c_str(),"ID");
			}
			remove((CashReg->DBGoodsPath+QDir::separator()+"ttcustomer.ndx").c_str());
		}


		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
//	time = GetTickCount() - time;
*/
}

#endif

void SQLThread::GetDiscountInfo(void)//copy discount's description to the local database
{
	vector<DiscountInfo> Desc;
	try
	{
		CashReg->SQLMutex->lock();

		AttachToDB();

		StartTran();

		char *sel_str="select\
				 percent,disctype,description,charge,fullpercent,fullcharge,starttime\
				,finishtime,startdate,finishdate,dayofweek,allgoods,isonprice,mode\
				from discschema order by disctype";

		Log("GetDiscountInfo begin connect to server");
#ifdef IBASE
		long type;
		double percent,charge;
		SQL_VARCHAR(100) fullpercent,fullcharge/*,description*/;
		SQL_VARCHAR(512) description;//**
		ISC_QUAD starttime,finishtime,startdate,finishdate;
		long dayofweek,allgoods,byprice,mode;

        int i;

		short type_flag,description_flag,percent_flag,charge_flag,fullpercent_flag,fullcharge_flag,starttime_flag,
			finishtime_flag,startdate_flag,finishdate_flag,day_flag,allgoods_flag,byprice_flag,mode_flag;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(14)];
		sqlda->sqln = 14;
		sqlda->sqld = 14;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		//ShowMessage(NULL,Int2Str(sqlda->sqld));

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
		{
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
			//CommitTran();
			//CashReg->SQLMutex->unlock();
			//return;
		}

		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&percent;
		sqlda->sqlvar[0].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[0].sqlind  = &percent_flag;

		sqlda->sqlvar[1].sqldata = (char ISC_FAR*)&type;
		sqlda->sqlvar[1].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[1].sqlind  = &type_flag;

		sqlda->sqlvar[2].sqldata = (char ISC_FAR*)&description+2;
		sqlda->sqlvar[2].sqltype = SQL_TEXT+1;
		sqlda->sqlvar[2].sqlind  = &description_flag;

		sqlda->sqlvar[3].sqldata = (char ISC_FAR*)&charge;
		sqlda->sqlvar[3].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[3].sqlind  = &charge_flag;

		sqlda->sqlvar[4].sqldata = (char ISC_FAR*)&fullpercent;
		sqlda->sqlvar[4].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[4].sqlind  = &fullpercent_flag;

		sqlda->sqlvar[5].sqldata = (char ISC_FAR*)&fullcharge;
		sqlda->sqlvar[5].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[5].sqlind  = &fullcharge_flag;

		sqlda->sqlvar[6].sqldata = (char ISC_FAR*)&starttime;
		sqlda->sqlvar[6].sqltype = SQL_DATE + 1;
		sqlda->sqlvar[6].sqlind  = &starttime_flag;

		sqlda->sqlvar[7].sqldata = (char ISC_FAR*)&finishtime;
		sqlda->sqlvar[7].sqltype = SQL_DATE + 1;
		sqlda->sqlvar[7].sqlind  = &finishtime_flag;

		sqlda->sqlvar[8].sqldata = (char ISC_FAR*)&startdate;
		sqlda->sqlvar[8].sqltype = SQL_DATE+1;
		sqlda->sqlvar[8].sqlind = &startdate_flag;

		sqlda->sqlvar[9].sqldata = (char ISC_FAR*)&finishdate;
		sqlda->sqlvar[9].sqltype = SQL_DATE+1;
		sqlda->sqlvar[9].sqlind = &finishdate_flag;

		sqlda->sqlvar[10].sqldata = (char ISC_FAR*)&dayofweek;
		sqlda->sqlvar[10].sqltype = SQL_LONG+1;
		sqlda->sqlvar[10].sqlind = &day_flag;

		sqlda->sqlvar[11].sqldata = (char ISC_FAR*)&allgoods;
		sqlda->sqlvar[11].sqltype = SQL_LONG+1;
		sqlda->sqlvar[11].sqlind = &allgoods_flag;

		sqlda->sqlvar[12].sqldata = (char ISC_FAR*)&byprice;
		sqlda->sqlvar[12].sqltype = SQL_LONG+1;
		sqlda->sqlvar[12].sqlind = &byprice_flag;

		sqlda->sqlvar[13].sqldata = (char ISC_FAR*)&mode;
		sqlda->sqlvar[13].sqltype = SQL_LONG+1;
		sqlda->sqlvar[13].sqlind = &mode_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{

			DiscountInfo tmpInfo;

            tmpInfo.fullpercent="";

/*			if (type_flag==0)
				tmpInfo.type=type;
			else
				tmpInfo.type=0;
*/
			if (description_flag==0)
			{
				//tmpInfo.description=description.vary_string;
				char tmpstr[513];//**
				strncpy(tmpstr,description.vary_string,512);
				tmpstr[strlen(tmpstr)-1]=' ';
				int pos=strstr(tmpstr,"          ")-tmpstr;
				if(/*pos!=NULL && */pos <513 && pos>0)//**
					tmpstr[pos]=0;
				bool empty=true;
				bool comment=false;
				for(int y=strlen(tmpstr)-1; y>-1;y--){
					if (tmpstr[y]==' ' && empty)
						tmpstr[y]=0;
					else
						empty=false;
					if (tmpstr[y]==')' && strchr(tmpstr,'('))
						comment=true;
					if (tmpstr[y]=='(' && !comment)
						tmpstr[y]=0;
					if (tmpstr[y]=='(' && comment){
						comment=false;
						tmpstr[y]=' ';
					}
					if(comment)
						memcpy(tmpstr+y,tmpstr+y+1,strlen(tmpstr)-y+1);
				}
			//	ShowMessage(NULL,tmpstr);
				tmpInfo.description=tmpstr;
			}
			else
				tmpInfo.description="-";

			if (type_flag==0)
				tmpInfo.type=type;
			else
				tmpInfo.type=0;

			if (percent_flag==0)
				tmpInfo.percent=percent;
			else
				tmpInfo.percent=0;

			if (charge_flag==0)
				tmpInfo.charge=charge;
			else
				tmpInfo.charge=0;

			if (fullpercent_flag==0)
				for (i=0;i<fullpercent.vary_length;i++)
					tmpInfo.fullpercent+=fullpercent.vary_string[i];
			else
				tmpInfo.fullpercent="";

			if (fullcharge_flag==0)
				for (i=0;i<fullcharge.vary_length;i++)
					tmpInfo.fullcharge+=fullcharge.vary_string[i];
			else
				tmpInfo.fullcharge="";

			if (starttime_flag==0)

				isc_decode_date(&starttime,&(tmpInfo.starttime));
			else
				tmpInfo.starttime.tm_hour=tmpInfo.starttime.tm_min=tmpInfo.starttime.tm_sec=0;

			if (finishtime_flag==0)
				isc_decode_date(&finishtime,&(tmpInfo.finishtime));
			else
				tmpInfo.finishtime.tm_hour=tmpInfo.finishtime.tm_min=tmpInfo.finishtime.tm_sec=0;


			if (startdate_flag==0)
				isc_decode_date(&startdate,&(tmpInfo.startdate));
			else
				tmpInfo.startdate.tm_mday=tmpInfo.startdate.tm_mon=tmpInfo.startdate.tm_year=0;

			if (finishdate_flag==0)
				isc_decode_date(&finishdate,&(tmpInfo.finishdate));
			else
				tmpInfo.finishdate.tm_mday=tmpInfo.finishdate.tm_mon=tmpInfo.finishdate.tm_year=0;

			if (day_flag==0)
				tmpInfo.dayofweek=dayofweek;
			else
				tmpInfo.dayofweek=0;

			if (allgoods_flag==0)
				tmpInfo.allgoods=allgoods;
			else
				tmpInfo.allgoods=0;

			if (byprice_flag==0)
				tmpInfo.byprice=byprice;
			else
				tmpInfo.byprice=0;

			if (mode_flag==0)
				tmpInfo.mode=mode;
			else
				tmpInfo.mode=0;


			Desc.insert(Desc.end(),tmpInfo);

		}//end feth

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;
		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		long type;
		double percent,charge;
		char fullpercent[101],fullcharge[101],description[101];
		char starttime[3*sizeof(short)],finishtime[3*sizeof(short)],
				startdate[3*sizeof(short)],finishdate[3*sizeof(short)];
		long dayofweek,allgoods,byprice,mode;
		SQLINTEGER type_flag,percent_flag,charge_flag,fullpercent_flag,fullcharge_flag,
			starttime_flag,finishtime_flag,startdate_flag,finishdate_flag,day_flag,allgoods_flag,byprice_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str,SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			SQLGetData(hstmt,1,SQL_C_LONG,&type,sizeof(long),&type_flag);
			SQLGetData(hstmt,2,SQL_C_CHAR,&description,101,&description_flag);
			SQLGetData(hstmt,3,SQL_C_DOUBLE,&percent,sizeof(double),&percent_flag);
			SQLGetData(hstmt,4,SQL_C_DOUBLE,&charge,sizeof(double),&charge_flag);
			SQLGetData(hstmt,5,SQL_C_CHAR,&fullpercent,101,&fullpercent_flag);
			SQLGetData(hstmt,6,SQL_C_CHAR,&fullcharge,101,&fullcharge_flag);
			SQLGetData(hstmt,7,SQL_C_TIME,starttime,3*sizeof(short),&starttime_flag);
			SQLGetData(hstmt,8,SQL_C_TIME,finishtime,3*sizeof(short),&finishtime_flag);
			SQLGetData(hstmt,9,SQL_C_DATE,startdate,3*sizeof(short),&startdate_flag);
			SQLGetData(hstmt,10,SQL_C_DATE,finishdate,3*sizeof(short),&finishdate_flag);
			SQLGetData(hstmt,11,SQL_C_LONG,&dayofweek,sizeof(long),&day_flag);
			SQLGetData(hstmt,12,SQL_C_LONG,&allgoods,sizeof(long),&allgoods_flag);
			SQLGetData(hstmt,13,SQL_C_LONG,&byprice,sizeof(long),&byprice_flag);
			SQLGetData(hstmt,14,SQL_C_LONG,&mode,sizeof(long),&mode_flag);


			DiscountInfo tmpInfo;

			if (type_flag!=SQL_NULL_DATA)
				tmpInfo.type=type;
			else
				tmpInfo.type=0;

			if (percent_flag!=SQL_NULL_DATA)
				tmpInfo.percent=percent;
			else
				tmpInfo.percent=0;

			if (charge_flag!=SQL_NULL_DATA)
				tmpInfo.charge=charge;
			else
				tmpInfo.charge=0;

			if (fullpercent_flag!=SQL_NULL_DATA)
				tmpInfo.fullpercent=fullpercent;
			else
				tmpInfo.fullpercent="";

			if (description_flag!=SQL_NULL_DATA)
				tmpInfo.description=description;
			else
				tmpInfo.description="";

			if (fullcharge_flag!=SQL_NULL_DATA)
				tmpInfo.fullcharge=fullcharge;
			else
				tmpInfo.fullcharge="";

			if (starttime_flag!=SQL_NULL_DATA)
			{
				tmpInfo.starttime.tm_hour=((short*)starttime)[0];
				tmpInfo.starttime.tm_min=((short*)starttime)[1];
				tmpInfo.starttime.tm_sec=((short*)starttime)[2];;
			}
			else
				tmpInfo.starttime.tm_hour=tmpInfo.starttime.tm_min=tmpInfo.starttime.tm_sec=0;

			if (finishtime_flag!=SQL_NULL_DATA)
			{
				tmpInfo.finishtime.tm_hour=((short*)finishtime)[0];

				tmpInfo.finishtime.tm_min=((short*)finishtime)[1];
				tmpInfo.finishtime.tm_sec=((short*)finishtime)[2];;
			}
			else
				tmpInfo.finishtime.tm_hour=tmpInfo.finishtime.tm_min=tmpInfo.finishtime.tm_sec=0;

			if (startdate_flag!=SQL_NULL_DATA)
			{
				tmpInfo.startdate.tm_year=((short*)startdate)[0]-1900;
				tmpInfo.startdate.tm_mon=((short*)startdate)[1]-1;
				tmpInfo.startdate.tm_mday=((short*)startdate)[2];
			}
			else
				tmpInfo.startdate.tm_mday=tmpInfo.startdate.tm_mon=tmpInfo.startdate.tm_year=0;

			if (finishdate_flag!=SQL_NULL_DATA)
			{
				tmpInfo.finishdate.tm_year=((short*)finishdate)[0]-1900;
				tmpInfo.finishdate.tm_mon=((short*)finishdate)[1]-1;
				tmpInfo.finishdate.tm_mday=((short*)finishdate)[2];
			}
			else
				tmpInfo.finishdate.tm_mday=tmpInfo.finishdate.tm_mon=tmpInfo.finishdate.tm_year=0;

			if (day_flag!=SQL_NULL_DATA)
				tmpInfo.dayofweek=dayofweek;
			else
				tmpInfo.dayofweek=0;

			if (allgoods_flag!=SQL_NULL_DATA)
				tmpInfo.allgoods=allgoods;
			else
				tmpInfo.allgoods=0;

			if (byprice_flag!=SQL_NULL_DATA)
				tmpInfo.byprice=byprice;
			else
				tmpInfo.byprice=0;

			if (mode_flag!=SQL_NULL_DATA)
				tmpInfo.mode=mode;
			else
				tmpInfo.mode=0;

			Desc.insert(Desc.end(),tmpInfo);
		}

		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);
		hstmt=SQL_NULL_HSTMT;
#endif
		CashReg->SQLMutex->unlock();

		CashReg->DbfMutex->lock();

		CashReg->DiscountsTable->CloseDatabase();//store discount's description to the local table
		CashReg->DiscountsTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+DISCDESC).c_str(),DiscountsRecord);

		for (unsigned int i=0;i<Desc.size();i++)
		{

			CashReg->DiscountsTable->BlankRecord();
			CashReg->DiscountsTable->PutLongField("DISCTYPE",Desc[i].type);

			CashReg->DiscountsTable->PutField("DESCRIPT",Desc[i].description.c_str());

			CashReg->DiscountsTable->PutDoubleField("PERCENT",Desc[i].percent);
			CashReg->DiscountsTable->PutDoubleField("CHARGE",Desc[i].charge);
			CashReg->DiscountsTable->PutField("FPERCENT",Desc[i].fullpercent.c_str());

			CashReg->DiscountsTable->PutField("FCHARGE",Desc[i].fullcharge.c_str());

			char time_buf[9];time_buf[8]='\0';

			sprintf(time_buf,"%02d:",Desc[i].starttime.tm_hour);
			sprintf(time_buf+3,"%02d:",Desc[i].starttime.tm_min);
			sprintf(time_buf+6,"%02d",Desc[i].starttime.tm_sec);
			CashReg->DiscountsTable->PutField("STARTTIME",time_buf);

			sprintf(time_buf,"%02d:",Desc[i].finishtime.tm_hour);
			sprintf(time_buf+3,"%02d:",Desc[i].finishtime.tm_min);
			sprintf(time_buf+6,"%02d",Desc[i].finishtime.tm_sec);
			CashReg->DiscountsTable->PutField("FINISHTIME",time_buf);

			char date_buf[9];date_buf[8]='\0';

			if (Desc[i].startdate.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",Desc[i].startdate.tm_year+1900);
				sprintf(date_buf+4,"%02d",Desc[i].startdate.tm_mon+1);
				sprintf(date_buf+6,"%02d",Desc[i].startdate.tm_mday);
				CashReg->DiscountsTable->PutField("STARTDATE",date_buf);
				//ShowMessage(NULL,date_buf);
			}
			else
				CashReg->DiscountsTable->PutField("STARTDATE","19000101");

			if (Desc[i].finishdate.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",Desc[i].finishdate.tm_year+1900);
				sprintf(date_buf+4,"%02d",Desc[i].finishdate.tm_mon+1);
				sprintf(date_buf+6,"%02d",Desc[i].finishdate.tm_mday);
				CashReg->DiscountsTable->PutField("FINISHDATE",date_buf);
			}
			else
				CashReg->DiscountsTable->PutField("FINISHDATE","21000101");

			CashReg->DiscountsTable->PutLongField("DAYOFWEEK",Desc[i].dayofweek);
			CashReg->DiscountsTable->PutLongField("ALLGOODS",Desc[i].allgoods);
			CashReg->DiscountsTable->PutLongField("BYPRICE",Desc[i].byprice);
			CashReg->DiscountsTable->PutLongField("MODE",Desc[i].mode);

			CashReg->DiscountsTable->AppendRecord();
		}
		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
}

void SQLThread::GetReturnCheque(int cashdesk,int chequenum,string DateStr,FiscalCheque* ReturnCheck)
{//by using date and number of cashdesk and check load return check from server
	try
	{
		CashReg->SQLMutex->lock();

		AttachToDB();

		StartTran();

#ifdef RET_CHECK
		//запрос на наличие записи в таблице возвращенных чеков
		string q_check = "select checknum from RETCHECK where checknum="+Int2Str(chequenum)+
			" and kassanum="+Int2Str(cashdesk)+" and datesale='"+DateStr+" 00.00'";
#endif
		string sel_str="select kodnom,kol,cena,summa,paymenttype,efullcena,customer_id,alco,cigar,actsiztype,barcode,otdel,avans,summapay from mgsales where kassanum="
			+Int2Str(cashdesk)+" and checknum="+Int2Str(chequenum)+
			" and tip="+Int2Str(SL);
		if(!DateStr.empty()){
			sel_str += " and datesale='"+DateStr+" 00.00'";
		}
#ifdef IBASE
		long code,payment_type, alco, cigar, actsiztype, section, avans;
		long long customer_id;
		double amount,price,summa,summapay,fullprice;
		short code_flag=0,amount_flag=0,price_flag=0,summa_flag=0,summapay_flag=0,alco_flag=0,cigar_flag=0,actsiztype_flag=0,
			fullprice_flag=0,customerid_flag=0,payment_type_flag=0, barcode_flag=0, section_flag=0,
			avans_flag=0;
		long fetch_stat;

		SQL_VARCHAR(20) barcode;


		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

#ifdef RET_CHECK
//		/* проверка чека на возвращенность
		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(1)];
		sqlda->sqln = sqlda->sqld = 1;
		sqlda->version = SQLDAVERSION;
		ibstmt=NULL;

		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&code;
		sqlda->sqlvar[0].sqltype = SQL_LONG+1;
		sqlda->sqlvar[0].sqlind  = &code_flag;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt)){
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
		}

		if(!isc_dsql_prepare(status, &ibtran,&ibstmt,0,(char*)q_check.c_str(),1,sqlda)){
			if(isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL)){
				throw cash_error(sql_error,0,GetErrorDescription().c_str());
			}

			int r = isc_dsql_fetch(status,&ibstmt,1,sqlda);

			if(isc_dsql_free_statement(status,&ibstmt,DSQL_drop)){//DSQL_close)){//
				throw cash_error(sql_error,0,GetErrorDescription().c_str());
			}

			if(!r){
				CashReg->ShowMessage("Этот чек уже возвращен!");
				CommitTran();
				return;
			}
		}

		delete sqlda;
//		*/
#endif

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(7)];
		sqlda->sqln = sqlda->sqld = 14;


		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,(char*)sel_str.c_str(),1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&code;
		sqlda->sqlvar[0].sqltype = SQL_LONG+1;
		sqlda->sqlvar[0].sqlind  = &code_flag;

		sqlda->sqlvar[1].sqldata = (char ISC_FAR*)&amount;
		sqlda->sqlvar[1].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[1].sqlind  = &amount_flag;

		sqlda->sqlvar[2].sqldata = (char ISC_FAR*)&price;
		sqlda->sqlvar[2].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[2].sqlind  = &price_flag;

		sqlda->sqlvar[3].sqldata = (char ISC_FAR*)&summa;
		sqlda->sqlvar[3].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[3].sqlind  = &summa_flag;

		sqlda->sqlvar[4].sqldata = (char ISC_FAR*)&payment_type;
		sqlda->sqlvar[4].sqltype = SQL_LONG+1;
		sqlda->sqlvar[4].sqlind  = &payment_type_flag;

		sqlda->sqlvar[5].sqldata = (char ISC_FAR*)&fullprice;
		sqlda->sqlvar[5].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[5].sqlind  = &fullprice_flag;

		sqlda->sqlvar[6].sqldata = (char ISC_FAR*)&customer_id;
		sqlda->sqlvar[6].sqltype = SQL_INT64+1;
		sqlda->sqlvar[6].sqlind  = &customerid_flag;

		sqlda->sqlvar[7].sqldata = (char ISC_FAR*)&alco;
		sqlda->sqlvar[7].sqltype = SQL_LONG+1;
		sqlda->sqlvar[7].sqlind  = &alco_flag;

		sqlda->sqlvar[8].sqldata = (char ISC_FAR*)&cigar;
		sqlda->sqlvar[8].sqltype = SQL_LONG+1;
		sqlda->sqlvar[8].sqlind  = &cigar_flag;

		sqlda->sqlvar[9].sqldata = (char ISC_FAR*)&actsiztype;
		sqlda->sqlvar[9].sqltype = SQL_LONG+1;
		sqlda->sqlvar[9].sqlind  = &actsiztype_flag;

		sqlda->sqlvar[10].sqldata = (char ISC_FAR*)&barcode;
		sqlda->sqlvar[10].sqltype = SQL_VARYING +1;
		sqlda->sqlvar[10].sqlind  = &barcode_flag;

		sqlda->sqlvar[11].sqldata = (char ISC_FAR*)&section;
		sqlda->sqlvar[11].sqltype = SQL_LONG+1;
		sqlda->sqlvar[11].sqlind  = &section_flag;

		sqlda->sqlvar[12].sqldata = (char ISC_FAR*)&avans;
		sqlda->sqlvar[12].sqltype = SQL_LONG+1;
		sqlda->sqlvar[12].sqlind  = &avans_flag;

		sqlda->sqlvar[13].sqldata = (char ISC_FAR*)&summapay;
		sqlda->sqlvar[13].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[13].sqlind  = &summapay_flag;


		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			GoodsInfo tmpGoods;

			if (code_flag==0)
				tmpGoods.ItemCode=code;
			else
				tmpGoods.ItemCode=0;
			if (amount_flag==0)
				tmpGoods.ItemCount=amount;
			else
				tmpGoods.ItemCount=0;

			if(code_flag == 0 && code == TRUNC)
			{
			// округление
				FiscalCheque::tsum = tmpGoods.ItemCount;
				continue;
			}

			if (summa_flag==0)
				tmpGoods.ItemSum=summa;
			else
				tmpGoods.ItemSum=0;

			if (summapay_flag==0)
				tmpGoods.ItemSumToPay=summapay;
			else
				tmpGoods.ItemSumToPay=0;

			if (price_flag==0)
				tmpGoods.ItemPrice=price;
			else
				tmpGoods.ItemPrice=0;
			if (fullprice_flag==0)
				tmpGoods.ItemFullPrice=fullprice;
			else
				tmpGoods.ItemFullPrice=0;
			if (payment_type_flag==0)
				tmpGoods.PaymentType=payment_type;
			else
				tmpGoods.PaymentType=CashPayment;

			if (alco_flag==0)
				tmpGoods.ItemAlco=alco;
			else
				tmpGoods.ItemAlco=0;

			if (cigar_flag==0)
				tmpGoods.ItemCigar=cigar;
			else
				tmpGoods.ItemCigar=0;

			if (actsiztype_flag==0)
				tmpGoods.ItemActsizType=actsiztype;
			else
				tmpGoods.ItemActsizType=0;

			if (avans_flag==0)
				tmpGoods.Avans=avans;
			else
				tmpGoods.Avans=0;

			tmpGoods.ItemBarcode = "";
			if (barcode_flag==0)
                for (int i=0;i<barcode.vary_length;i++)
                    tmpGoods.ItemBarcode+=barcode.vary_string[i];

			tmpGoods.ItemCalcSum=RoundToNearest(tmpGoods.ItemCalcPrice*tmpGoods.ItemCount,0.01);
			tmpGoods.ItemFullSum=RoundToNearest(tmpGoods.ItemFullPrice*tmpGoods.ItemCount,0.01);
			tmpGoods.ItemFullDiscSum=tmpGoods.ItemSum;

			tmpGoods.ItemFlag=SL;
			tmpGoods.GuardCode=-1;

			CashReg->DbfMutex->lock();
			if (CashReg->GoodsTable->Locate("CODE",tmpGoods.ItemCode))
			{
				if(CashReg->GoodsTable->GetFieldNo("NDS") != -1)
				{
					tmpGoods.NDS = CashReg->GoodsTable->GetDoubleField("NDS");
				}
				tmpGoods.ItemName=CashReg->GoodsTable->GetField("NAME");
				if (CashReg->GoodsTable->GetLogicalField("GTYPE"))
					tmpGoods.ItemPrecision=1;
				else
					tmpGoods.ItemPrecision=0.001;
			}
			else
			{
				// ругаться!
				tmpGoods.ItemName="";
				tmpGoods.ItemPrecision=1;
			}
			CashReg->DbfMutex->unlock();

			tmpGoods.DiscFlag=false;
			if (customerid_flag==0)
				tmpGoods.CustomerCode[0]=customer_id;
			else
				tmpGoods.CustomerCode[0]=0;
			if (tmpGoods.CustomerCode[0]!=0)
				tmpGoods.DiscFlag=true;
			else
				tmpGoods.DiscFlag=false;

            // + dms ==== возврат по секциям
			if (section_flag==0)
				tmpGoods.Section=section;
			else
                tmpGoods.Section=0;

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
            // - dms ==== возврат по секциям

            tmpGoods.ItemTextile = (tmpGoods.ItemActsizType == AT_TEXTILE);
            tmpGoods.ItemMilk = (tmpGoods.ItemActsizType == AT_MILK);
			Log("tmpGoodsItemMilk = " + Int2Str(tmpGoods.ItemMilk));

#ifdef ALCO_VES
            tmpGoods.ItemPivoRazliv = (tmpGoods.ItemActsizType == AT_PIVO_RAZLIV);
			Log("tmpGoodsItemPivoRazliv=" + Int2Str(tmpGoods.ItemPivoRazliv) );
#endif

            tmpGoods.RetCheck = chequenum;

			ReturnCheck->Insert(tmpGoods);
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		long code,customer_id,payment_type;
		double amount,price,summa,fullprice;
		SQLINTEGER code_flag,amount_flag,price_flag,summa_flag,fullprice_flag,customerid_flag,
			payment_type_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			SQLGetData(hstmt,1,SQL_C_LONG,&code,sizeof(long),&code_flag);
			SQLGetData(hstmt,2,SQL_C_DOUBLE,&amount,sizeof(double),&amount_flag);
			SQLGetData(hstmt,3,SQL_C_DOUBLE,&price,sizeof(double),&price_flag);
			SQLGetData(hstmt,4,SQL_C_DOUBLE,&summa,sizeof(double),&summa_flag);
			SQLGetData(hstmt,5,SQL_C_LONG,&payment_type,sizeof(long),&payment_type_flag);
			SQLGetData(hstmt,6,SQL_C_DOUBLE,&fullprice,sizeof(double),&fullprice_flag);
			SQLGetData(hstmt,7,SQL_C_LONG,&customer_id,sizeof(long),&customerid_flag);

			GoodsInfo tmpGoods;

			if (code_flag!=SQL_NULL_DATA)
				tmpGoods.ItemCode=code;
			else
				tmpGoods.ItemCode=0;
			if (amount_flag!=SQL_NULL_DATA)
				tmpGoods.ItemCount=amount;
			else
				tmpGoods.ItemCount=0;
			if (price_flag!=SQL_NULL_DATA)
				tmpGoods.ItemPrice=price;
			else
				tmpGoods.ItemPrice=0;
			if (fullprice_flag!=SQL_NULL_DATA)
				tmpGoods.ItemFullPrice=fullprice;
			else
				tmpGoods.ItemFullPrice=0;
			if (summa_flag!=SQL_NULL_DATA)
				tmpGoods.ItemSum=summa;
			else
				tmpGoods.ItemSum=0;
			if (payment_type_flag!=SQL_NULL_DATA)
				tmpGoods.PaymentType=payment_type;
			else
				tmpGoods.PaymentType=CashPayment;
			tmpGoods.ItemFullSum=tmpGoods.ItemFullPrice*tmpGoods.ItemCount;
			tmpGoods.ItemCalcSum=tmpGoods.ItemCalcPrice*tmpGoods.ItemCount;
			tmpGoods.ItemFullDiscSum=tmpGoods.ItemSum;

			tmpGoods.ItemFlag=SL;
			tmpGoods.GuardCode=-1;

			CashReg->DbfMutex->lock();
			if (CashReg->GoodsTable->Locate("CODE",tmpGoods.ItemCode))
			{
				tmpGoods.ItemName=CashReg->GoodsTable->GetField("NAME");
				if (CashReg->GoodsTable->GetLogicalField("GTYPE"))
					tmpGoods.ItemPrecision=1;
				else
					tmpGoods.ItemPrecision=0.001;
			}
			CashReg->DbfMutex->unlock();

			tmpGoods.DiscFlag=false;
			if (customerid_flag!=SQL_NULL_DATA)
				tmpGoods.CustomerCode[0]=customer_id;
			else
				tmpGoods.CustomerCode[0]=0;
			if (tmpGoods.CustomerCode[0]!=0)
				tmpGoods.DiscFlag=true;
			else
				tmpGoods.DiscFlag=false;
			ReturnCheck->Insert(tmpGoods);
		}

		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);
		hstmt=SQL_NULL_HSTMT;
#endif
		CashReg->SQLMutex->unlock();

		if (ReturnCheck->GetCount()==0)
			CashReg->ShowMessage("Нечего возвращать!");
		else
			return;
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
}

void SQLThread::ProcessError(cash_error& er,bool CallFromThread)//general error's handler
{
	if ((er.type()==sql_error)||(er.type()==ping_error))//detach from database and show error message
	{
	    Log("<<ERROR>> sql_error");
		Log("DetachFromDB();");
		DetachFromDB();
		CashReg->SQLMutex->unlock();
		sleep(1);
	}


	if (er.type()==xbase_error)
	{
	    Log("<<ERROR>> xbase_error");
		CashReg->DbfMutex->unlock();
	}

	Log("if (ConnectionStatus)");
	if (ConnectionStatus)
	{
		Log("SetConnectionStatus(false);");
		SetConnectionStatus(false);

		if (CallFromThread)
		{
		    Log("CallFromThread");
			Log("postEvent(CashReg,new SQLEvent(er));");
			postEvent(CashReg,new SQLEvent(er));
			sleep(30);//stop the thread for half minute
		}
		else
			CashReg->LoggingError(er);
	}
	Log("endif (ConnectionStatus)");
}


bool SQLThread::PingServer(void)//is server accessible or not
{
//*
	if (GetDetachStatus())
		return false;

    return true;

	if (DBPath.find(":")!=DBPath.npos)
	{
		string host=DBPath.substr(0,DBPath.find(":"));
		for (int i=0;i<5;i++)
		{

			if ((firststart)&&(CashReg->Socket->TestConnect(host.c_str())))
			{
			   //;; Log("~ PingServer ~ true");

                firststart=false;
                if (CashReg->Socket->TestBank(host.c_str(), 3050, 1)==0)
                {
                    Log ("OK_Socket");
                    return true;
                }
                else
                {
                    Log ("Fail_Socket");
                    return false;
                }

			    return true;

			}
			else
			{
                Log("~ PingServer ~ false");
            }
		}

                return false;

	}
//*/
	return true;
}

bool SQLThread::GetConnectionStatus(void)
{
	return ConnectionStatus;
}

void SQLThread::SetConnectionStatus(bool status)
{
	CashReg->SQLMutex->lock();
	ConnectionStatus=status;
	CashReg->SQLMutex->unlock();
}

bool SQLThread::GetDetachStatus(void)
{
	return detach_status;
}

void SQLThread::SetDetachStatus(bool status)
{
	CashReg->SQLMutex->lock();
	detach_status=status;
	if (detach_status)
		ConWaitPeriod=MaxTimeout;
	else
	{
		CashReg->DbfMutex->lock();
		ConWaitPeriod=CashReg->ConfigTable->GetLongField("IB_WAIT");
  	ConAttempt=0;
		CashReg->DbfMutex->unlock();
	}
	CashReg->SQLMutex->unlock();
}

string SQLThread::GetDBPath(void)
{
	return DBPath;
}

void SQLThread::SetDBPath(string path)
{
	CashReg->SQLMutex->lock();
	DBPath=path;
	CashReg->SQLMutex->unlock();
}

string SQLThread::GetDBLogin(void)
{
	return DBLogin;
}

void SQLThread::SetDBLogin(string login)
{
	CashReg->SQLMutex->lock();
	DBLogin=login;
	CashReg->SQLMutex->unlock();
}

string SQLThread::GetDBPassword(void)
{
	return DBPassword;
}

void SQLThread::SetDBPassword(string password)
{
	CashReg->SQLMutex->lock();
	DBPassword=password;
	CashReg->SQLMutex->unlock();
}

time_t SQLThread::GetWaitPeriod(void)
{
	return ConWaitPeriod;
}

void SQLThread::SetWaitPeriod(time_t period)
{
	CashReg->SQLMutex->lock();
	ConWaitPeriod=period;
	CashReg->SQLMutex->unlock();
}

void SQLThread::SendSales(void)//lazy write of sales
{

	try
	{
			CashReg->DbfMutex->lock();
			if (!CashReg->NonSentTable->GetFirstRecord())
			{
				CashReg->NonSentTable->Zap();
				CashReg->DbfMutex->unlock();
				return;
			}

			do{
				xbLong SaveRecNo = CashReg->NonSentTable->GetRecNo();

				GoodsInfo tmpGoods;  //for each sale create record and send it to server
				string CurDate,CurTime,Saleman_Name;
				int SalemanID=CashReg->NonSentTable->GetLongField("CASHMAN");
				int CheckNum=CashReg->NonSentTable->GetLongField("CHECKNUM");
				if(!CheckNum)
				{
					CashReg->NonSentTable->DeleteRecord();
					continue;
				}

                tmpGoods.ItemCode=CashReg->NonSentTable->GetLongField("CODE"); //Получаем данные для записи в БД
                tmpGoods.ItemCount=CashReg->NonSentTable->GetDoubleField("QUANT");
				tmpGoods.ItemFlag=CashReg->NonSentTable->GetLongField("OPERATION");
				tmpGoods.PaymentType=CashReg->NonSentTable->GetLongField("PAYTYPE");
				tmpGoods.ItemPrice=CashReg->NonSentTable->GetDoubleField("PRICE");
				tmpGoods.ItemFullPrice=CashReg->NonSentTable->GetDoubleField("FULLPRICE");
				tmpGoods.ItemCalcPrice=CashReg->NonSentTable->GetDoubleField("CALCPRICE");
				tmpGoods.ItemSum=CashReg->NonSentTable->GetDoubleField("SUMMA");
                tmpGoods.ItemSumToPay=CashReg->NonSentTable->GetDoubleField("SUMMAPAY");
				tmpGoods.ItemFullSum=CashReg->NonSentTable->GetDoubleField("FULLSUMMA");
				tmpGoods.ItemFullDiscSum=CashReg->NonSentTable->GetDoubleField("FULLDSUMMA");
				tmpGoods.ItemBankDiscSum=CashReg->NonSentTable->GetDoubleField("BANKDSUMMA");

				tmpGoods.ItemBankBonusDisc=CashReg->NonSentTable->GetDoubleField("BANKBONUS");

				tmpGoods.ItemBonusAdd=CashReg->NonSentTable->GetDoubleField("BONUSADD");
				tmpGoods.ItemBonusDisc=CashReg->NonSentTable->GetDoubleField("BONUSDISC");
				tmpGoods.ItemBonusFlag = CashReg->NonSentTable->GetLongField("BONUSFLAG");

				tmpGoods.ItemCalcSum=CashReg->NonSentTable->GetDoubleField("CALCSUMMA");
				tmpGoods.CustomerCode[0]=CashReg->NonSentTable->GetLLongField("CUSTOMER");
				tmpGoods.CustomerCode[1]=CashReg->NonSentTable->GetLLongField("ADDONCUST");
				tmpGoods.InputType=CashReg->NonSentTable->GetLongField("INPUTKIND");

				tmpGoods.KKMNo=CashReg->NonSentTable->GetLongField("KKMNO");
				tmpGoods.KKMSmena=CashReg->NonSentTable->GetLongField("KKMSMENA");
				tmpGoods.KKMTime=CashReg->NonSentTable->GetLongField("KKMTime");

                tmpGoods.ItemName = CashReg->NonSentTable->GetStringField("NAME");
                tmpGoods.ItemBarcode = CashReg->NonSentTable->GetStringField("BARCODE");
                tmpGoods.ItemSecondBarcode = CashReg->NonSentTable->GetStringField("SBARCODE");
                tmpGoods.RetCheck = CashReg->NonSentTable->GetLongField("RETCHECK");
                tmpGoods.GuardCode = CashReg->NonSentTable->GetLongField("GUARD");

                tmpGoods.Avans = CashReg->NonSentTable->GetLongField("AVANS");
                tmpGoods.ItemCigar = CashReg->NonSentTable->GetLongField("CIGAR");
                tmpGoods.ItemAlco = CashReg->NonSentTable->GetLongField("ALCO");
                tmpGoods.ItemActsizType = CashReg->NonSentTable->GetLongField("ACTSIZTYPE");
                tmpGoods.ItemActsizBarcode = CashReg->NonSentTable->GetStringField("ACTSIZ");

                //if (tmpGoods.ItemCigar){
                    tmpGoods.ItemActsizBarcode = StrReplaceAll(tmpGoods.ItemActsizBarcode, "'", "`");
                      tmpGoods.ItemActsizBarcode = StrReplaceAll(tmpGoods.ItemActsizBarcode, ":", "`");
//                    tmpGoods.ItemActsizBarcode = StrReplaceAll(tmpGoods.ItemActsizBarcode, "\"", "*");
                //}

                tmpGoods.Section = CashReg->NonSentTable->GetLongField("SECTION");

                tmpGoods.ItemMinimumPrice=CashReg->NonSentTable->GetDoubleField("MINPRICE");

				CurTime=CashReg->NonSentTable->GetStringField("TIME");
				CurDate=xbDate().FormatDate(DateTemplate,CashReg->NonSentTable->GetField("DATE").c_str());
				CashReg->DbfMutex->unlock();
#ifdef LOGNDS

                tmpGoods.NDS = CashReg->NonSentTable->GetDoubleField("NDS");
#endif
                double count=0,summa=0,summaPay=0,ecount=0,esumma=0,fsumma=0, bsumma=0;
				switch (tmpGoods.ItemFlag) //according to goods flag set various parameters
				{
				case SL:   //if goods was saled then we set standart parameters
					count=tmpGoods.ItemCount;
					summa=tmpGoods.ItemSum;
					summaPay=tmpGoods.ItemSumToPay;
					fsumma=tmpGoods.ItemFullDiscSum;
					bsumma=tmpGoods.ItemBankDiscSum;
					ecount=esumma=0;
					break;
				case RT:   //if return
					count=-tmpGoods.ItemCount;
					summa=-tmpGoods.ItemSum;
					summaPay=-tmpGoods.ItemSumToPay;
					fsumma=-tmpGoods.ItemFullDiscSum;
					bsumma=-tmpGoods.ItemBankDiscSum;
					ecount=tmpGoods.ItemCount;
					esumma=tmpGoods.ItemSum;
					break;
				case ST:  //if storno
					count=summa=fsumma=bsumma=summaPay=0;
					ecount=tmpGoods.ItemCount;
					esumma=tmpGoods.ItemSum;
					break;
				case ASL: //if sale was annuled
					count=summa=fsumma=bsumma=summaPay=0;
					ecount=tmpGoods.ItemCount;
					esumma=tmpGoods.ItemSum;
					break;
				case ART: //if return was annuled
					count=summa=fsumma=bsumma=summaPay=0;
					ecount=tmpGoods.ItemCount;
					esumma=tmpGoods.ItemSum;
					break;
				case AST: //if storno was anuled
					count=summa=fsumma=bsumma=summaPay=0;
					ecount=tmpGoods.ItemCount;
					esumma=tmpGoods.ItemSum;
					break;
				}
				//create SQL-statement

#ifndef LOGNDS

                Log("====================================");
                Log("NDS=" + Double2Str(tmpGoods.NDS));
                        //tmpGoods.NDS));
                Log("Price=" + Double2Str(tmpGoods.ItemPrice));
                Log("NumCheck=" + Int2Str(CheckNum));
                Log("=====================================");
#endif
                ///Добавил НДС

				string Insert_str=
				 "INSERT INTO MGSALES (IDPODR,DATESALE,TIMESALE,DTSALE,KASSANUM,CHECKNUM,KODNOM,KOL,CENA,\
SUMMA,SUMMAPAY,TIP,PAYMENTTYPE,EKOL,ESUMMA,CASHMAN_ID,CUSTOMER_ID,ADDONCUSTOMER,INPUTKIND,EFULLCENA,\
ECALCCENA,KKMNO,KKMSMENA,KKMTIME,BARCODE,NAME,FULLSUMMA,BANKSUMMA,RETCHECK,VERSION,SBARCODE,\
GUARD_ID, BANKBONUS, BANK, OTDEL, MINPRICE, AVANS, ACTSIZTYPE, CIGAR, ALCO, ACTSIZ, BONUSADD, BONUSDISC, BONUSFLAG, NDS) VALUES(";
				Insert_str+=CashReg->GetMagNum()+",'";
				Insert_str+=CurDate+"','30.12.1899 ";
				Insert_str+=CurTime+"','";
				Insert_str+=CurDate+" "+CurTime+"',";
				Insert_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
				Insert_str+=Int2Str(CheckNum)+",";
				Insert_str+=Int2Str(tmpGoods.ItemCode)+",";
				Insert_str+=TruncZeros(Double2Str(count))+",";
                Insert_str+=TruncZeros(Double2Str(tmpGoods.ItemPrice))+","; //CENA
				Insert_str+=TruncZeros(Double2Str(summa))+",";
				Insert_str+=TruncZeros(Double2Str(summaPay))+",";
				Insert_str+=Int2Str(tmpGoods.ItemFlag)+',';
				Insert_str+=Int2Str(tmpGoods.PaymentType)+',';
				Insert_str+=TruncZeros(Double2Str(ecount))+",";
				Insert_str+=TruncZeros(Double2Str(esumma))+",";
				Insert_str+=Int2Str(SalemanID)+",";
				Insert_str+=LLong2Str(tmpGoods.CustomerCode[0])+",";
				Insert_str+=LLong2Str(tmpGoods.CustomerCode[1])+",";
				Insert_str+=Int2Str(tmpGoods.InputType)+",";
                Insert_str+=TruncZeros(Double2Str(tmpGoods.ItemFullPrice))+","; //EFULLCENA
				Insert_str+=TruncZeros(Double2Str(tmpGoods.ItemCalcPrice))+",";
				Insert_str+=Int2Str(tmpGoods.KKMNo)+",";
				Insert_str+=Int2Str(tmpGoods.KKMSmena)+",";
				Insert_str+=Int2Str(tmpGoods.KKMTime)+",";
                Insert_str+="'"+tmpGoods.ItemBarcode+"',"; //BARCODE
				//Insert_str+=tmpGoods.ItemCode==FreeSumCode?"'"+tmpGoods.ItemName+"',":"NULL,";

//;;Log("==start BD==");
                Log((char*)("      ---> "+tmpGoods.ItemName+" /C="+TruncZeros(Double2Str(count))+"; S="+TruncZeros(Double2Str(summa))+"/  Op="+Int2Str(tmpGoods.ItemFlag)).c_str());

                Insert_str+="'"+StrReplaceAll(tmpGoods.ItemName, "'", "`")+"',"; //NAME
				Insert_str+=TruncZeros(Double2Str(fsumma))+",";
				Insert_str+=TruncZeros(Double2Str(bsumma))+",";
				Insert_str+=Int2Str(tmpGoods.RetCheck)+",";
				Insert_str+=Int2Str(revision)+",";
                Insert_str+="'"+tmpGoods.ItemSecondBarcode+"',"; //SBARCODE
				Insert_str+=Int2Str(tmpGoods.GuardCode)+",";
				Insert_str+=TruncZeros(Double2Str(tmpGoods.ItemBankBonusDisc))+",";
				Insert_str+=Int2Str(tmpGoods.BankID)+",";
				Insert_str+=Int2Str(tmpGoods.Section)+",";
				Insert_str+=TruncZeros(Double2Str(tmpGoods.ItemMinimumPrice))+",";

                Insert_str+=Int2Str(tmpGoods.Avans)+",";
                Insert_str+=Int2Str(tmpGoods.ItemActsizType)+",";
                Insert_str+=Int2Str(tmpGoods.ItemCigar)+",";
				Insert_str+=Int2Str(tmpGoods.ItemAlco)+",";
                Insert_str+="'"+tmpGoods.ItemActsizBarcode+"',"; //ACTSIZ

				Insert_str+=TruncZeros(Double2Str(tmpGoods.ItemBonusAdd))+",";
				Insert_str+=TruncZeros(Double2Str(tmpGoods.ItemBonusDisc))+",";
                Insert_str+=Int2Str(tmpGoods.ItemBonusFlag)+","; //BONUSFLAG "," добавил из-за НДС
       ///Добавляем НДС
                Insert_str+=TruncZeros(Double2Str(tmpGoods.NDS));

				Insert_str+=")";

				CashReg->SQLMutex->lock();
//;;Log((char*)StrReplaceAll(tmpGoods.ItemName, "'", "`").c_str());

//;;Log((char*)Insert_str.c_str());
//;;Log("==finish==");

				AttachToDB();

				StartTran();
#ifdef IBASE
				if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_str.c_str(),1,NULL))
					throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);

				SQLAllocStmt(hdbc,&hstmt);
				if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
					throw cash_error(sql_error,0,GetErrorDescription().c_str());

				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);
				hstmt=SQL_NULL_HSTMT;
#endif
				CommitTran();

				CashReg->SQLMutex->unlock();

				CashReg->DbfMutex->lock();
				CashReg->NonSentTable->SetRecNo(SaveRecNo);
				CashReg->NonSentTable->DeleteRecord();
			}
			while(CashReg->NonSentTable->GetNextRecord());
			CashReg->NonSentTable->Zap();
			CashReg->DbfMutex->unlock();
			Log("");
			Log("");
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,true);
    }
}

#ifndef ALCO_VES
void SQLThread::SendSalesPivoRazliv(string str)
{
    Log("-------SendSalesPivoRazliv START---------");
    string Insert_str_Pivo="INSERT INTO PIVORAZLIV (CODE_DATAMATRIKS,BARCODE,MASSA,TYPE,MAX_MASSA) VALUES(";
    Insert_str_Pivo += str;
    Insert_str_Pivo += (")");
    Log("Строка добавление записи = " + Insert_str_Pivo);
    Log("-------SendSalesPivoRazliv FINISH---------");

}
#endif

void SQLThread::SendProtocols(void)//send all stored protocols to the server
{
	bool StartSending=false;
	try
	{
		CashReg->DbfMutex->lock();
		if (!CashReg->ActionsTable->GetFirstRecord())
		{
			CashReg->DbfMutex->unlock();
			return;
		}


			do
			{
			    string CurDate, CurTime;

			    //xbDate(CashReg->ActionsTable->GetField("ACT_DATE").c_str()).FormatDate(DateTemplate)

			    CurDate = xbDate().FormatDate(DateTemplate,CashReg->ActionsTable->GetField("ACT_DATE").c_str());
			    CurTime = CashReg->ActionsTable->GetStringField("ACT_TIME");

				xbLong SaveRecNo = CashReg->ActionsTable->GetRecNo();

				string Insert_str="INSERT INTO MGCASHDESKLOG (IDPODR,ACTION_DATE,ACTION_TIME,DTSALE,\
					CASH_DESK,CHECKNUM,ACTION_TYPE,ACTION_SUM,SALEMAN_ID,GUARD_ID,CARD,ACODE,BTERM,INFO,VERSION, BANK) VALUES(";
				Insert_str+=CashReg->GetMagNum()+",'";
//				Insert_str+=(xbDate(CashReg->ActionsTable->GetField("ACT_DATE").c_str()).
//					FormatDate(DateTemplate)+"','30.12.1899 ").c_str();
				Insert_str+=CurDate+"','30.12.1899 ";
				Insert_str+=CurTime+"','";
				Insert_str+=CurDate+" "+CurTime+"',";
				Insert_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
				Insert_str+=Int2Str(CashReg->ActionsTable->GetLongField("CHECKNUM"))+",";
				Insert_str+=Int2Str(CashReg->ActionsTable->GetLongField("ACT_TYPE"))+",";
				Insert_str+=Double2Str(CashReg->ActionsTable->GetDoubleField("ACT_SUM"))+",";
				Insert_str+=Int2Str(CashReg->ActionsTable->GetLongField("SALEMAN_ID"))+",";
				Insert_str+=Int2Str(CashReg->ActionsTable->GetLongField("GUARD_ID"))+",";
				Insert_str+= "'" + StrReplaceAll(CashReg->ActionsTable->GetStringField("CARD"), "'", "`") +"',";
				Insert_str+= "'" + CashReg->ActionsTable->GetStringField("ACODE")+"',";
				Insert_str+= "'" + CashReg->ActionsTable->GetStringField("BTERM")+"',";
				Insert_str+= "'" + StrReplaceAll(CashReg->ActionsTable->GetStringField("INFO"), "'", "`")+"',";
				Insert_str+=Int2Str(revision)+",";
				Insert_str+=Int2Str(CashReg->ActionsTable->GetLongField("BANK_ID"));
				Insert_str+=")";

				CashReg->DbfMutex->unlock();
				CashReg->SQLMutex->lock();

				AttachToDB();

				StartTran();
#ifdef IBASE
				if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_str.c_str(),1,NULL))
					throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);

				SQLAllocStmt(hdbc,&hstmt);
				if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
					throw cash_error(sql_error,0,GetErrorDescription().c_str());

				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);
				hstmt=SQL_NULL_HSTMT;
#endif
				CommitTran();

				CashReg->SQLMutex->unlock();

				CashReg->DbfMutex->lock();
				CashReg->ActionsTable->SetRecNo(SaveRecNo);
				CashReg->ActionsTable->DeleteRecord();
			}
			while(CashReg->ActionsTable->GetNextRecord());
			CashReg->ActionsTable->Zap();
			CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,true);
	}
}

void SQLThread::SendBankAction(void)//send all bank operation
{
	bool StartSending=false;
	try
	{
		CashReg->DbfMutex->lock();
		if (!CashReg->BankActionsTable->GetFirstRecord())
		{
			CashReg->DbfMutex->unlock();
			return;
		}

			do
			{
			    string CurDate, CurTime;

			    //xbDate(CashReg->ActionsTable->GetField("ACT_DATE").c_str()).FormatDate(DateTemplate)

			    CurDate = xbDate().FormatDate(DateTemplate,CashReg->BankActionsTable->GetField("DATESALE").c_str());
			    CurTime = CashReg->BankActionsTable->GetStringField("TIMESALE");

				xbLong SaveRecNo = CashReg->BankActionsTable->GetRecNo();

                string txt = CashReg->BankActionsTable->GetStringField("TEXT");
                txt = StrReplaceAll(txt, "'", "*");

				string Insert_str="INSERT INTO BANKREPORTS (IDPODR,DATESALE,TIMESALE,DTSALE,\
					KASSANUM,CHECKNUM,ACTION,CASHMAN_ID,BANK,TERMINAL,IDREPORT,LINE,TEXT) VALUES(";
				Insert_str+=CashReg->GetMagNum()+",'";
				Insert_str+=CurDate+"','30.12.1899 ";
				Insert_str+=CurTime+"','";
				Insert_str+=CurDate+" "+CurTime+"',";
				Insert_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
				Insert_str+=Int2Str(CashReg->BankActionsTable->GetLongField("CHECKNUM"))+",";
				Insert_str+=Int2Str(CashReg->BankActionsTable->GetLongField("ACTION"))+",";
				Insert_str+=Int2Str(CashReg->BankActionsTable->GetLongField("SALEMAN_ID"))+",";
				Insert_str+=Int2Str(CashReg->BankActionsTable->GetLongField("BANK"))+",";
                Insert_str+= "'" + CashReg->BankActionsTable->GetStringField("TERMINAL")+"',";
				Insert_str+= "'" + CashReg->BankActionsTable->GetStringField("IDREPORT")+"',";
				Insert_str+=Int2Str(CashReg->BankActionsTable->GetLongField("LINE"))+",";
				Insert_str+= "'" + txt +"'";
				Insert_str+=")";

				CashReg->DbfMutex->unlock();
				CashReg->SQLMutex->lock();

				AttachToDB();

				StartTran();
#ifdef IBASE
				if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_str.c_str(),1,NULL))
					throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);

				SQLAllocStmt(hdbc,&hstmt);
				if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
					throw cash_error(sql_error,0,GetErrorDescription().c_str());

				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);
				hstmt=SQL_NULL_HSTMT;
#endif
				CommitTran();

				CashReg->SQLMutex->unlock();

				CashReg->DbfMutex->lock();
				CashReg->BankActionsTable->SetRecNo(SaveRecNo);
				CashReg->BankActionsTable->DeleteRecord();
			}
			while(CashReg->BankActionsTable->GetNextRecord());
			CashReg->BankActionsTable->Zap();
			CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,true);
	}
}


void SQLThread::SendCustomSales(void)//send all stored sales of the customers with discount cards
{									//to the server
	bool StartSending=false;
	try
	{
		CashReg->DbfMutex->lock();
		if (!CashReg->CustomersSalesTable->GetFirstRecord())
		{
			CashReg->DbfMutex->unlock();
			return;
		}
			do{

			    string CurDate, CurTime;

			    CurDate = xbDate().FormatDate(DateTemplate,	CashReg->CustomersSalesTable->GetField("DATESALE").c_str());
			    CurTime = CashReg->CustomersSalesTable->GetField("TIMESALE");

				xbLong SaveRecNo = CashReg->CustomersSalesTable->GetRecNo();

				string Insert_str="INSERT INTO MGCUSTSALES (IDPODR,IDCUST,DATESALE,TIMESALE,DTSALE,CASHDESK,CHECKNUM,DISCSUMMA,SUMMA,VERSION) VALUES(";
				Insert_str+=CashReg->GetMagNum()+",";
				Insert_str+=LLong2Str(CashReg->CustomersSalesTable->GetLLongField("IDCUST"))+",'";
				Insert_str+=CurDate+"',";
				Insert_str+="'30.12.1899 "+CurTime+"','";
				Insert_str+=CurDate+" "+CurTime+"',";
				Insert_str+=Int2Str(CashReg->CustomersSalesTable->GetLongField("CASHDESK"))+",";
				Insert_str+=Int2Str(CashReg->CustomersSalesTable->GetLongField("CHECKNUM"))+",";
				Insert_str+=Double2Str(CashReg->CustomersSalesTable->GetDoubleField("DISCSUMMA"))+",";
				Insert_str+=Double2Str(CashReg->CustomersSalesTable->GetDoubleField("SUMMA"))+",";
				Insert_str+=Int2Str(revision);
				Insert_str+=")";

				CashReg->DbfMutex->unlock();

				CashReg->SQLMutex->lock();

				AttachToDB();

				StartTran();
#ifdef IBASE
				if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_str.c_str(),1,NULL))
					throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);

				SQLAllocStmt(hdbc,&hstmt);
				if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
					throw cash_error(sql_error,0,GetErrorDescription().c_str());

				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);
				hstmt=SQL_NULL_HSTMT;
#endif
				CommitTran();

				CashReg->SQLMutex->unlock();
				CashReg->DbfMutex->lock();
				CashReg->CustomersSalesTable->SetRecNo(SaveRecNo);
				CashReg->CustomersSalesTable->DeleteRecord();
			}
			while(CashReg->CustomersSalesTable->GetNextRecord());
			CashReg->CustomersSalesTable->Zap();
			CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,true);
	}
}

void SQLThread::run(void)
{

	//if (CashReg->CurrentWorkPlace!=NULL && CashReg->CurrentWorkPlace->PriceChecker!=NULL){
	//	Beep();
		//GetOstTov();
		//return;
	//}



	Log("START   ->{SQL} SQLThread::Run() ");
	stop_flag = true;
	try{
		do
		{
;;Log("   -> {SQL} Send START");
			//if (CashReg->CurrentWorkPlace!=NULL && CashReg->CurrentWorkPlace->PriceChecker!=NULL)
			//	GetOstTov();
;;Log("   -> {SQL} Send SendSales");
			SendSales();//try to send sales,protocols and customer's info
;;Log("   -> {SQL} Send SendCustomSales");
			SendCustomSales();
;;Log("   -> {SQL} Send SendProtocols");
			SendProtocols();
;;Log("   -> {SQL} Send SendBankAction");
			SendBankAction();
#			ifdef RET_CHECK
;;Log("   -> {SQL} Send SendReturn");
			SendReturn();
#			endif
;;Log("   -> {SQL} Send SendPayment");
			SendPayment();
;;Log("   -> {SQL} Send SendBankBonus");
			SendBankBonus();
;;Log("   -> {SQL} Send SendBonus");
			SendBonus();
;;Log("   -> {SQL} Send SendBonus");
			SendActsiz();
;;Log("   -> {SQL} Send END");
	  		CashReg->SQLMutex->lock();
	   		if (GetDetachStatus())
			{
			    ;;Log("   -> {SQL} GetDetachStatus error");
				cash_error er(ping_error,0,HostIsInaccessible);
				ProcessError(er,true);
			}
			else
			{
			    ;;Log("   -> {SQL} GetDetachStatus ok");
			    ;;Log("   -> {SQL} =1=");
				CashReg->SQLMutex->unlock();
				;;Log("   -> {SQL} =2=");
			}
		}
		while(!stop_flag);
	}
	catch(cash_error& ex)
	{
	    Log("   -> {SQL} ERR");
		ProcessError(ex,false);
		Log("   -> {SQL} SQL Error!");
	}
	Log("END   ->{SQL} SQLThread::Run() ");
}

void SQLThread::DoAfterFixing(void)
{
//	/*
	int RecordCount;
	do
	{
		CashReg->DbfMutex->lock();
		RecordCount=CashReg->ActionsTable->RecordCount();
		CashReg->DbfMutex->unlock();
	}
	while(RecordCount!=0);

	int customer=0;
	string sel_str="select customer from millenium where cash_desk="+
			Int2Str(CashReg->GetCashRegisterNum());

	try
	{

		CashReg->SQLMutex->lock();

#ifdef IBASE
		short customer_flag=0;
		long fetch_stat;

		AttachToDB();
		StartTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(1)];
		sqlda->sqln = 1;
		sqlda->sqld = 1;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,(char*)sel_str.c_str(),1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&customer;
		sqlda->sqlvar[0].sqltype = SQL_LONG+1;
		sqlda->sqlvar[0].sqlind  = &customer_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0);

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		SQLINTEGER customer_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			SQLGetData(hstmt,1,SQL_C_LONG,&customer,sizeof(long),&customer_flag);

			if (customer_flag==SQL_NULL_DATA)
				customer=0;
		}

		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		hstmt=SQL_NULL_HSTMT;
#endif
		CashReg->SQLMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
	if(customer<1000000)
		return;
	if(customer>1000001){
		CashReg->CheckMillenium = false;
		return;
	}
	CashReg->CheckMillenium = false;

	ExclamationForm *dlg = new ExclamationForm(CashReg->CurrentWorkPlace);
	if (customer==1000000){
		dlg->ExclamationLabel->setText(W2U("ПРИЗ!!!"));
	}
	if (customer==1000001){
		dlg->ExclamationLabel->setText(W2U("ПОКУПКА!"));
	}
	dlg->AlertLabel->setText(W2U("Покупатель N "+Int2Str(customer)+"! Продолжить?"));
	dlg->exec();
}




#ifdef RET_CHECK
///*
void SQLThread::SendReturn()
{
	bool StartSending=false;
	try
	{

		// удалить отправленные не сегодняшние
		string date = xbDate().GetDate().c_str();
		CashReg->DbfMutex->lock();
		if(!CashReg->ReturnTable->GetFirstRecord()){
			CashReg->DbfMutex->unlock();
			return;
		}
		while(CashReg->ReturnTable->GetLogicalField("SENT") && CashReg->ReturnTable->GetField("DATE") != date){
			CashReg->ReturnTable->DeleteRecord();
			if(!CashReg->ReturnTable->GetNextRecord()){
				CashReg->DbfMutex->unlock();
				return;
			}
		}
		if (!CashReg->ReturnTable->Locate("SENT",false))
		{
			CashReg->DbfMutex->unlock();
			return;
		}
		CashReg->DbfMutex->unlock();

		StartSending=true;

		CashReg->SQLMutex->lock();

		AttachToDB();

		StartTran();
#ifdef IBASE
		if(isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,"select checknum from RETCHECK where checknum=0",1,NULL)){
			if(isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,Q_RETURN_CREATE,1,NULL)){
			//if(isc_dsql_exec_immed2(status,&ibtable,&ibtran,0,Q_RETURN_CREATE,1,NULL,NULL)){
				string s = GetErrorDescription();
				throw cash_error(sql_error,0,GetErrorDescription().c_str());
			}
		}
#else
///*
		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);
		if(SQLExecDirect(hstmt,"select checknum from return where checknum=0",SQL_NTS)!=SQL_SUCCESS){
			if(SQLExecDirect(hstmt,Q_RETURN_CREATE,SQL_NTS)!=SQL_SUCCESS)
				throw cash_error(sql_error,0,GetErrorDescription().c_str());
		}

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);
		hstmt=SQL_NULL_HSTMT;
//*/
#endif
		CommitTran();
		CashReg->SQLMutex->unlock();

		for(;;)//insert each record into table
		{
			CashReg->DbfMutex->lock();

			string d = xbDate().FormatDate(DateTemplate,CashReg->ReturnTable->GetField("DATE").c_str());
			if(d.find(' ') != -1){
				CashReg->ReturnTable->DeleteRecord();
				if (!CashReg->ReturnTable->Locate("SENT",false))
				{
					CashReg->DbfMutex->unlock();
					break;
				}
				CashReg->DbfMutex->unlock();
				continue;
			}
			string Insert_str="INSERT INTO RETCHECK (CHECKNUM,DATESALE,KASSANUM,OPER,KKM,REGCHECK) VALUES(";
			Insert_str+=Int2Str(CashReg->ReturnTable->GetLongField("CHECKNUM"))+",";
			Insert_str+=("'"+d+" 00:00',").c_str();
			Insert_str+=Int2Str(CashReg->ReturnTable->GetLongField("CASHDESK"))+",";
			Insert_str+=Int2Str(CashReg->ReturnTable->GetLongField("OPERATION"))+",";
			Insert_str+=Double2Str(CashReg->ReturnTable->GetDoubleField("KKM"))+",";
			Insert_str+=Double2Str(CashReg->ReturnTable->GetDoubleField("CHECK"))+")";

			CashReg->DbfMutex->unlock();

			CashReg->SQLMutex->lock();

			AttachToDB();

			StartTran();
#ifdef IBASE
			if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_str.c_str(),1,NULL))
				throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
			if (hstmt!=SQL_NULL_HSTMT)
				SQLFreeStmt(hstmt,SQL_DROP);

			SQLAllocStmt(hdbc,&hstmt);
			if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
				throw cash_error(sql_error,0,GetErrorDescription().c_str());

			if (hstmt!=SQL_NULL_HSTMT)
				SQLFreeStmt(hstmt,SQL_DROP);
			hstmt=SQL_NULL_HSTMT;
#endif
			CommitTran();

			CashReg->SQLMutex->unlock();

			CashReg->DbfMutex->lock();
			CashReg->ReturnTable->PutField("SENT","T");
			if (!CashReg->ReturnTable->Locate("SENT",false))
			{
				CashReg->DbfMutex->unlock();
				break;
			}

			CashReg->DbfMutex->unlock();
		}
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,true);
	}

}
//*/
#endif

int SQLThread::GetMsgLevel()
{
	if(CashReg->MsgLevelTable->GetFirstRecord()){
		CashReg->MsgLevelList.clear();
		do{
			MsgLevel ml;
			ml.level = CashReg->MsgLevelTable->GetDoubleField("LEVEL");
			ml.msg = CashReg->MsgLevelTable->GetStringField("MSG"); // : '' -> +'\0'
/*			if(CashReg->MsgLevelTable->GetFieldNo("STARTDATE")!=-1)
				ml.startdate=CashReg->MsgLevelTable->GetField("STARTDATE");
			else{
				ml.startdate="19000101";
				ShowMessage(NULL,"NO STARTDATE");
			}
			if(CashReg->MsgLevelTable->GetFieldNo("FINISHDATE")!=-1)
				ml.startdate=CashReg->MsgLevelTable->GetField("FINISHDATE");
			else{
				ml.startdate="19000101");
				ShowMessage(NULL,"NO FINISHDATE");
			}

*/		}while(CashReg->MsgLevelTable->GetNextRecord());
	}
	CashReg->SQLMutex->lock();
	AttachToDB();
	StartTran();

#ifdef IBASE
	double level;
	short mode;
	int CustType;

	SQL_VARCHAR(200) msg;//  : 100<-200
	ISC_QUAD startdate,finishdate;

	short mode_flag=0,level_flag=0,msg_flag=0,custtype_flag=0;
	short startdate_flag=0,finishdate_flag=0;
	long fetch_stat;

	bool newtable=true;

	if (sqlda!=NULL)
 	{
		delete sqlda;
		sqlda=NULL;
	}

	sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(6)];
	sqlda->sqln = 6;
	sqlda->sqld = 6;
	sqlda->version = SQLDAVERSION;

	ibstmt=NULL;

	try
	{
	if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
		throw cash_error(sql_error,0,GetErrorDescription().c_str());
	if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,"select sumlevel, msg, mode, startdate, finishdate, custtype from mgmsglevel where isactive=1 order by sumlevel desc",1,sqlda)){
		string s = GetErrorDescription();
		CommitTran();//
		newtable=false;
		//CashReg->SQLMutex->unlock();//
		//return 0;//
			//(270208)
		AttachToDB();
		StartTran();
		if (sqlda!=NULL){
			delete sqlda;
			sqlda=NULL;
		}
		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(2)];
		sqlda->sqln = 2;
		sqlda->sqld = 2;
		sqlda->version = SQLDAVERSION;
		ibstmt=NULL;
		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,"select sumlevel, msg from mgmsglevel where isactive=1 order by sumlevel desc",1,sqlda)){
			string s = GetErrorDescription();
			CommitTran();
			CashReg->SQLMutex->unlock();
			return 0;
		}
	}


	sqlda->sqlvar[0].sqldata = (char ISC_FAR *)&level;
	sqlda->sqlvar[0].sqltype = SQL_DOUBLE + 1;
	sqlda->sqlvar[0].sqlind  = &level_flag;

	sqlda->sqlvar[1].sqldata = (char ISC_FAR *)&msg;
	sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
	sqlda->sqlvar[1].sqlind  = &msg_flag;

	if(newtable){

        sqlda->sqlvar[2].sqldata = (char ISC_FAR *)&mode;
        sqlda->sqlvar[2].sqltype = SQL_SHORT + 1;
        sqlda->sqlvar[2].sqlind  = &mode_flag;

		sqlda->sqlvar[3].sqldata = (char *)&startdate;
		sqlda->sqlvar[3].sqltype = SQL_DATE + 1;
		sqlda->sqlvar[3].sqlind  = &startdate_flag;

		sqlda->sqlvar[4].sqldata = (char *)&finishdate;
		sqlda->sqlvar[4].sqltype = SQL_DATE + 1;
		sqlda->sqlvar[4].sqlind  = &finishdate_flag;

		sqlda->sqlvar[5].sqldata = (char *)&CustType;
		sqlda->sqlvar[5].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[5].sqlind  = &custtype_flag;
	}

	if (isc_dsql_execute(status, &ibtran, &ibstmt, 1, NULL))
		throw cash_error(sql_error,0,GetErrorDescription().c_str());
	CashReg->MsgLevelList.clear();
	CashReg->MsgLevelTable->Zap();

	while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
	{
		MsgLevel ml;

		if (msg_flag==0)
			for (int i=0;i<msg.vary_length;i++)
					ml.msg += msg.vary_string[i];
		else
			ml.msg = "";


		if (level_flag==0)
			ml.level=level;
		else
			ml.level=0;//-1;

		if (mode_flag==0)
			ml.mode=mode;
		else
			ml.mode=-1;

		if (custtype_flag==0)
			ml.custtype=CustType;
		else
			ml.custtype=0;

		if (startdate_flag==0)
				isc_decode_date(&startdate,&(ml.startdate));
			else
				ml.startdate.tm_mday=ml.startdate.tm_mon=ml.startdate.tm_year=0;

			if (finishdate_flag==0)
				isc_decode_date(&finishdate,&(ml.finishdate));
			else
				ml.finishdate.tm_mday=ml.finishdate.tm_mon=ml.finishdate.tm_year=0;

		CashReg->MsgLevelTable->AppendRecord();
		CashReg->MsgLevelTable->PutDoubleField("LEVEL", ml.level);
		CashReg->MsgLevelTable->PutField("MSG",ml.msg.c_str()+'\0');
		CashReg->MsgLevelTable->PutLongField("MODE",ml.mode);
		char date_buf[9];date_buf[8]='\0';
		if (newtable && ml.startdate.tm_mday!=0){
			//char date_buf[9];date_buf[8]='\0';
			//if(ml.startdate.tm_mday!=0){
			sprintf(date_buf,"%04d",ml.startdate.tm_year+1900);
			sprintf(date_buf+4,"%02d",ml.startdate.tm_mon+1);
			sprintf(date_buf+6,"%02d",ml.startdate.tm_mday);
			//ShowMessage(NULL,date_buf);
		}
		else
			sprintf(date_buf,"%08s","19000102");date_buf[8]='\0';
		CashReg->MsgLevelTable->PutField("STARTDATE", date_buf);

		if(newtable && ml.finishdate.tm_mday!=0){
			sprintf(date_buf,"%04d",ml.finishdate.tm_year+1900);
			sprintf(date_buf+4,"%02d",ml.finishdate.tm_mon+1);
			sprintf(date_buf+6,"%02d",ml.finishdate.tm_mday);
		}
		else
			sprintf(date_buf,"%08s","21000102");date_buf[8]='\0';
		CashReg->MsgLevelTable->PutField("FINISHDATE", date_buf);

        CashReg->MsgLevelTable->PutLongField("CUSTTYPE",ml.custtype);
		CashReg->MsgLevelTable->PutRecord();

		CashReg->MsgLevelList.insert(CashReg->MsgLevelList.end(),ml);

	}

	if (fetch_stat!=100)
		throw cash_error(sql_error,0,GetErrorDescription().c_str());

	if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
		throw cash_error(sql_error,0,GetErrorDescription().c_str());

	ibstmt=NULL;

	if (sqlda!=NULL)
	{
		delete sqlda;
		sqlda=NULL;
	}
#endif
	CommitTran();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
	CashReg->SQLMutex->unlock();

	return 0;
}


bool SQLThread::SetPreCheckStatus(unsigned long code, int PreCheckStatus)
{
    try
	{
		//CashReg->SQLMutex->lock();
		;;Log((char*)("SetPreCheckStatus: code="+Int2Str(code)+", status="+Int2Str(PreCheckStatus)).c_str());

		char update_str[1024];
		sprintf(update_str,"update mgprecheck set status=%d where precode= %d", PreCheckStatus, code);

        CashReg->SQLMutex->lock();

        AttachToDB();

        StartTran();
#ifdef IBASE
        if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,update_str,1,NULL))
            throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
        if (hstmt!=SQL_NULL_HSTMT)
            SQLFreeStmt(hstmt,SQL_DROP);

        SQLAllocStmt(hdbc,&hstmt);
        if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
            throw cash_error(sql_error,0,GetErrorDescription().c_str());

        if (hstmt!=SQL_NULL_HSTMT)
            SQLFreeStmt(hstmt,SQL_DROP);
        hstmt=SQL_NULL_HSTMT;
#endif
        CommitTran();

        CashReg->SQLMutex->unlock();

	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
		return false;
	}

	return true;
}

bool SQLThread::GetPreCheck(unsigned long code, FiscalCheque *check, long long CurrentCustomer)
{
	bool ret = false;
	int CountAdd=0, CountAll=0;
	try
	{
		CashReg->SQLMutex->lock();

		AttachToDB();

		StartTran();

		string sel_str="select GCODE,GNAME,GPRICE,GVOL,IDCUST, STATUS from MGPRECHECK where STATUS in (0,1) and PRECODE="
			+Int2Str(code);
#ifdef IBASE
		long gcode;
		double gprice, gvol;
		SQL_VARCHAR(30) gname;
		short gcode_flag=0,gprice_flag=0,gname_flag=0,gvol_flag=0,idcust_flag=0,status_flag=0;
		long fetch_stat;

        long long idcust, ID_Customer;
        long st=0, PrecheckStatus=0;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(6)];
		sqlda->sqln = sqlda->sqld = 6;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,(char*)sel_str.c_str(),1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&gcode;
		sqlda->sqlvar[0].sqltype = SQL_LONG+1;
		sqlda->sqlvar[0].sqlind  = &gcode_flag;

		sqlda->sqlvar[1].sqldata = (char ISC_FAR*)&gname;
		sqlda->sqlvar[1].sqltype = SQL_VARYING+1;//
		sqlda->sqlvar[1].sqlind  = &gname_flag;

		sqlda->sqlvar[2].sqldata = (char ISC_FAR*)&gprice;
		sqlda->sqlvar[2].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[2].sqlind  = &gprice_flag;

		sqlda->sqlvar[3].sqldata = (char ISC_FAR*)&gvol;
		sqlda->sqlvar[3].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[3].sqlind  = &gvol_flag;

		sqlda->sqlvar[4].sqldata = (char ISC_FAR*)&idcust;
		sqlda->sqlvar[4].sqltype = SQL_INT64+1;
		sqlda->sqlvar[4].sqlind  = &idcust_flag;

		sqlda->sqlvar[5].sqldata = (char ISC_FAR*)&st;
		sqlda->sqlvar[5].sqltype = SQL_LONG+1;
		sqlda->sqlvar[5].sqlind  = &status_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

        bool Wrong_IDCust=false;

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{

            CountAll++;

			if (status_flag==0)
			{
				PrecheckStatus=st;
			}
			else
				PrecheckStatus=0;

			if (idcust_flag==0)
				ID_Customer=idcust;
			else
				ID_Customer=0;

			bool isOk=true;
			if ((ID_Customer!=0) && (ID_Customer!=CurrentCustomer))
			{
			    isOk=false;
			    Wrong_IDCust=true;
			}

		    if (isOk)
		    {

			ret = true;
			GoodsInfo tmpGoods;

			if (gcode_flag==0)
				tmpGoods.ItemCode=gcode;
			else
				tmpGoods.ItemCode=0;
			if (gvol_flag==0)
				tmpGoods.ItemCount=gvol;
			else
				tmpGoods.ItemCount=0;
			if (gprice_flag==0)
				tmpGoods.ItemFullPrice=tmpGoods.ItemPrice=gprice;
			else
				tmpGoods.ItemFullPrice=tmpGoods.ItemPrice=0;
			tmpGoods.ItemName = "";
			if (gname_flag==0){
				for(int i = 0; i < gname.vary_length; i++)
					tmpGoods.ItemName += gname.vary_string[i];
			}

			tmpGoods.ItemName = trim(tmpGoods.ItemName);

			tmpGoods.PaymentType=CashPayment;

			tmpGoods.ItemSum=tmpGoods.ItemFullSum=tmpGoods.ItemFullDiscSum=RoundToNearest(tmpGoods.ItemFullPrice*tmpGoods.ItemCount,0.01);

			tmpGoods.ItemFlag=SL;
			tmpGoods.GuardCode=-1;

            bool isFound=false;
			CashReg->DbfMutex->lock();
			if (CashReg->GoodsTable->Locate("CODE",tmpGoods.ItemCode))
			{
				tmpGoods.ItemName=CashReg->GoodsTable->GetField("NAME");
				if (CashReg->GoodsTable->GetLogicalField("GTYPE"))
					tmpGoods.ItemPrecision=1;
				else
					tmpGoods.ItemPrecision=0.001;

				tmpGoods.GuardCode=-1;

#ifdef LOGNDS
                tmpGoods.NDS = CashReg->GoodsTable->GetDoubleField("NDS");
                Log("===================================");
                Log("NDS=" + Double2Str(tmpGoods.NDS));
                Log("====================================");
#endif

                tmpGoods.ItemBarcode=CashReg->GoodsTable->GetStringField("BARCODE");
				tmpGoods.ItemDiscountsDesc=CashReg->GoodsTable->GetStringField("SKIDKA");
				tmpGoods.ItemDBPos=CashReg->GoodsTable->GetRecNo();
				tmpGoods.ItemDepartment=CashReg->ConfigTable->GetLongField("DEPARTMENT");
//				tmpGoods.ItemFullPrice=CurrentGoods->ItemPrice=CashReg->GoodsTable->GetDoubleField("PRICE");
				tmpGoods.ItemFixedPrice=CashReg->GoodsTable->GetDoubleField("FIXEDPRICE");

                if (tmpGoods.ItemFullPrice<0.001)
                {
                    tmpGoods.InputType = IN_PRECHECK0; // цена берется из базы, скидки предоставляются
                    tmpGoods.ItemFullPrice=tmpGoods.ItemPrice=CashReg->GoodsTable->GetDoubleField("PRICE");
                    tmpGoods.ItemSum=tmpGoods.ItemFullSum=tmpGoods.ItemFullDiscSum=RoundToNearest(tmpGoods.ItemFullPrice*tmpGoods.ItemCount,0.01);
                }
                else
                {
                    tmpGoods.InputType = IN_PRECHECK; // цена уже задана, скидок нет

                    string ZDiscountType="";
                    for(int i=0;i<MAXDISCOUNTLEN;i++) ZDiscountType+="Z";
                    tmpGoods.ItemDiscountsDesc = ZDiscountType;
                }

                tmpGoods.Lock = CashReg->GoodsTable->GetField("LOCK");

				tmpGoods.PaymentType=CashPayment;

				isFound=true;
			}
			else
			{
//				tmpGoods.ItemName="";
				tmpGoods.ItemPrecision=1;
			}
			CashReg->DbfMutex->unlock();

            // Товар не найден по коду
            if (!isFound)
            {
                string msg = "Не найден товар по коду "+Int2Str(tmpGoods.ItemCode);
                msg+= "! \n Товар не будет добавлен в чек!";
                CashReg->ShowMessage(msg);
                continue;
            }

            // + EGAIS
            short isAlco=0;
            string strLock = tmpGoods.Lock;
            Log((char*)strLock.c_str());
            if (CashReg->EgaisSettings.ModePrintQRCode>-1)
            {
                if (CashReg->GoodsLock(strLock, GL_AlcoEGAIS) == GL_AlcoEGAIS)
                { // Продажа алкогольной продукции
                  isAlco=1;
                }
            }
            // - EGAIS
            tmpGoods.ItemAlco = isAlco;

            tmpGoods.Actsiz.clear();

            string ActsizBarcode;
            int errAct;

            if(isAlco)
            {
                int KolAlco = tmpGoods.ItemCount;
                //tmpGoods.ItemCount = 1;

                //ActsizBarcode = tmpGoods.ItemActsizBarcode;
                for (int j=0;j<KolAlco;j++)
                {
                    //! тут добавить проверку на правильность формата акцизной марки
                    if (ActsizBarcode=="")
                    {
                      errAct=0;
                      while(errAct || (ActsizBarcode == ""))
                      {
                        errAct = CashReg->CheckActsizBarcode(&ActsizBarcode, "Позиция # "+Int2Str(j+1)+" из "+Int2Str(KolAlco)+" \n\n"+ Int2Str(tmpGoods.ItemCode)+" "+ tmpGoods.ItemName, check, tmpGoods.Actsiz);
                      }
                      tmpGoods.ItemActsizBarcode = ActsizBarcode;

                        //tmpGoods.Actsiz.clear();
                        //for (int j=0;j<ListActsizBarcodes.size();j++)
                        tmpGoods.Actsiz.insert(tmpGoods.Actsiz.end(), ActsizBarcode);
                    }

                    ActsizBarcode="";

                }
            }

            check->Insert(tmpGoods);

            CountAdd++;
		    }
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		// oracle
#endif
		CashReg->SQLMutex->unlock();

		if (CountAdd)
        {

            ;;Log((char*)("Статус пречека="+Int2Str(PrecheckStatus)).c_str());

            if (PrecheckStatus>0)
            {
                // Установим статус "в обработке"
                SetPreCheckStatus(code, 2);

                if (check->NumPrecheck<=0)
                    check->NumPrecheck = code;
            }
        }
		else
        {
            if (CountAll)
                CashReg->ShowMessage("Неправильная дисконтная карта.");
            else
                CashReg->ShowMessage("Нет данных по этому пречеку.");
        }
    }
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
	return ret;
}

void SQLThread::GetOstTov(/*unsigned long code*/)
{
	string xbdate=xbDate().GetDate().c_str();
	char ibdate[]={xbdate[6],xbdate[7],'.',xbdate[4],xbdate[5],'.',xbdate[0],xbdate[1],xbdate[2],xbdate[3]};
	double foundOstTov;
	string code;
	//CashReg->ShowMessage(CashReg->CurrentWorkPlace->PriceChecker->CodeLine->text());
	try
	{
		if(CashReg->CurrentWorkPlace->PriceChecker->CodeLine->text().isEmpty())
			return;

		code=CashReg->CurrentWorkPlace->PriceChecker->CodeLine->text().ascii();
		CashReg->SQLMutex->lock();
		AttachToDB();
		StartTran();
		string sel_str="select sum(kol) from mggetosttov("+code+",'"+ibdate+"','"+ibdate+"')";
#ifdef IBASE
		double ostTov;
		short ostTov_flag=0;
		long fetch_stat;
		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(1)];
		sqlda->sqln = sqlda->sqld = 1;
		sqlda->version = SQLDAVERSION;
		ibstmt=NULL;
		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,(char*)sel_str.c_str(),1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&ostTov;
		sqlda->sqlvar[0].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[0].sqlind  = &ostTov_flag;
		CashReg->ShowMessage("PreExecute");
		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
		CashReg->ShowMessage("PostExecute");
		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0){
			if (ostTov_flag==0)
					foundOstTov=ostTov;
			else
				foundOstTov=0;
		}
		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
		ibstmt=NULL;
		CommitTran();
		CashReg->ShowMessage("CommitTran");

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
		// oracle
#endif
		CashReg->SQLMutex->unlock();

		if (!foundOstTov)
			CashReg->ShowMessage("Нет данных по остаткам.");
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
	//return foundOstTov;
	CashReg->CurrentWorkPlace->PriceChecker->RestLine->setText(GetRounded(foundOstTov,0.001).c_str());
}

//-------------------------------------------------------------------------------------------------------------------------------
void SQLThread::GetBankDiscount()
{
	try
	{
		CashReg->SQLMutex->lock();
		AttachToDB();

		StartTran();

		char *sel_str="select bankid,bankidend,discounts,idcust from mgbankdiscount";

#ifdef IBASE
		//SQL_VARCHAR(7) BankId;
		long BankId, BankIdEnd;
		SQL_VARCHAR(MAXDISCOUNTLEN) Discounts;
		long idcoust;
		short id_flag=0,idend_flag=0,discounts_flag=0, idcust_flag = 0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(4)];
		sqlda->sqln = 4;
		sqlda->sqld = 4;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char *)&BankId;
		sqlda->sqlvar[0].sqltype = SQL_LONG  + 1;
		sqlda->sqlvar[0].sqlind  = &id_flag;

		sqlda->sqlvar[1].sqldata = (char *)&BankIdEnd;
		sqlda->sqlvar[1].sqltype = SQL_LONG  + 1;
		sqlda->sqlvar[1].sqlind  = &idend_flag;

		sqlda->sqlvar[2].sqldata = (char *)&Discounts;
		sqlda->sqlvar[2].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[2].sqlind  = &discounts_flag;

		sqlda->sqlvar[3].sqldata = (char *)&idcoust;
		sqlda->sqlvar[3].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[3].sqlind  = &idcust_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		CashReg->DbfMutex->lock();
		CashReg->BankDiscountTable->CloseDatabase();//store customer's list into local table
		CashReg->BankDiscountTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+"tbankdisc.dbf").c_str(),BankDiscountRecord);
		CashReg->DbfMutex->unlock();

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			int i;

			CashReg->DbfMutex->lock();
			CashReg->BankDiscountTable->BlankRecord();
			CashReg->BankDiscountTable->PutLongField("BANKID", id_flag==0 ? BankId : 0);
			CashReg->BankDiscountTable->PutLongField("BANKIDEND", idend_flag==0 ? BankIdEnd : 0);
			CashReg->BankDiscountTable->PutField("DISCOUNTS", discounts_flag==0 ? Discounts.vary_string : "");
			CashReg->BankDiscountTable->PutLongField("IDCUST", discounts_flag==0 ? idcoust : 0);
			CashReg->BankDiscountTable->AppendRecord();
			CashReg->DbfMutex->unlock();
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
/*		long ID;

		char Discounts[21];

		char IssueDate[3*sizeof(short)],ExpDate[3*sizeof(short)];
		SQLINTEGER id_flag,discounts_flag,issue_flag,exp_flag;

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		SQLAllocStmt(hdbc,&hstmt);

		if (SQLExecDirect(hstmt,(unsigned char*)sel_str,SQL_NTS)!=SQL_SUCCESS)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while (SQLFetch(hstmt)==SQL_SUCCESS)
		{
			SQLGetData(hstmt,1,SQL_C_LONG,&ID,sizeof(long),&id_flag);
			SQLGetData(hstmt,2,SQL_C_CHAR,Discounts,21,&discounts_flag);
			SQLGetData(hstmt,3,SQL_C_DATE,IssueDate,3*sizeof(short),&issue_flag);
			SQLGetData(hstmt,4,SQL_C_DATE,ExpDate,3*sizeof(short),&exp_flag);

			CustomerStruct tmpCustomer;

			if (id_flag!=SQL_NULL_DATA)
				tmpCustomer.Id=ID;
			else
				tmpCustomer.Id=0;

			if (discounts_flag!=SQL_NULL_DATA)
				tmpCustomer.Discounts=Discounts;
			else
				tmpCustomer.Discounts="";

			if (issue_flag!=SQL_NULL_DATA)
			{
				tmpCustomer.issue.tm_year=((short*)IssueDate)[0]-1900;

				tmpCustomer.issue.tm_mon=((short*)IssueDate)[1]-1;
				tmpCustomer.issue.tm_mday=((short*)IssueDate)[2];
			}
			else
				tmpCustomer.issue.tm_mday=tmpCustomer.issue.tm_mon=tmpCustomer.issue.tm_year=0;
			if (exp_flag!=SQL_NULL_DATA)
			{
				tmpCustomer.expire.tm_year=((short*)ExpDate)[0]-1900;
				tmpCustomer.expire.tm_mon=((short*)ExpDate)[1]-1;
				tmpCustomer.expire.tm_mday=((short*)ExpDate)[2];
			}
			else
				tmpCustomer.expire.tm_mday=tmpCustomer.expire.tm_mon=tmpCustomer.expire.tm_year=0;

//			Customers.insert(Customers.end(),tmpCustomer);

			CashReg->DbfMutex->lock();
			CashReg->CustomersTable->BlankRecord();
			CashReg->CustomersTable->PutLongField("ID",tmpCustomer.Id);
			CashReg->CustomersTable->PutField("DISCOUNTS",tmpCustomer.Discounts.c_str());
			char date_buf[9];date_buf[8]='\0';
			if (tmpCustomer.issue.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",tmpCustomer.issue.tm_year+1900);
				sprintf(date_buf+4,"%02d",tmpCustomer.issue.tm_mon+1);
				sprintf(date_buf+6,"%02d",tmpCustomer.issue.tm_mday);
				CashReg->CustomersTable->PutField("ISSUE",date_buf);
			}
			else
				CashReg->CustomersTable->PutField("ISSUE","19000101");

			if (tmpCustomer.expire.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",tmpCustomer.expire.tm_year+1900);
				sprintf(date_buf+4,"%02d",tmpCustomer.expire.tm_mon+1);
				sprintf(date_buf+6,"%02d",tmpCustomer.expire.tm_mday);
				CashReg->CustomersTable->PutField("EXPIRE",date_buf);
			}
			else
				CashReg->CustomersTable->PutField("EXPIRE","21000101");
			CashReg->CustomersTable->AppendRecord();
			CashReg->DbfMutex->unlock();
		}

		CommitTran();

		if (hstmt!=SQL_NULL_HSTMT)
			SQLFreeStmt(hstmt,SQL_DROP);

		hstmt=SQL_NULL_HSTMT;
*/
#endif
		CashReg->SQLMutex->unlock();
		CashReg->DbfMutex->lock();
		CashReg->BankDiscountTable->CloseDatabase();//store customer's list into local table
		CashReg->DbfMutex->unlock();
		remove((CashReg->DBGoodsPath+QDir::separator()+"bankdisc.dbf").c_str());
		rename((CashReg->DBGoodsPath+QDir::separator()+"tbankdisc.dbf").c_str(),
			(CashReg->DBGoodsPath+QDir::separator()+"bankdisc.dbf").c_str());
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
		CashReg->DbfMutex->lock();
		CashReg->BankDiscountTable->CloseDatabase();//store customer's list into local table
		CashReg->DbfMutex->unlock();
	}
	CashReg->BankDiscountTable->OpenDatabase((CashReg->DBGoodsPath+QDir::separator()+"bankdisc.dbf").c_str());
	remove((CashReg->DBGoodsPath+QDir::separator()+"tbankdisc.dbf").c_str());
}
//-------------------------------
void SQLThread::GetCustRange()
{
	vector<CustRangeStruct> CustRange;
	try
	{
		CashReg->SQLMutex->lock();
		AttachToDB();
		StartTran();
		char *sel_str="select idfirst,idlast,discounts,issue,expire,custtype,summa,bonuscard,partype from custrange";
#ifdef IBASE
		long long IDFirst,IDLast;
		int CustType;
		int ParType;
		int BonusCard;
		double summa;
		//SQL_VARCHAR(6) BankId;
		SQL_VARCHAR(MAXDISCOUNTLEN) Discounts;
		ISC_QUAD IssueDate,ExpDate;
		//long idcoust;
		short idfirst_flag=0,idlast_flag=0,discounts_flag=0,issue_flag=0,exp_flag=0,custtype_flag=0,partype_flag=0,summa_flag=0,bonuscard_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(8)];
		sqlda->sqln = 9;
		sqlda->sqld = 9;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//^^
		sqlda->sqlvar[0].sqldata = (char *)&IDFirst;
		sqlda->sqlvar[0].sqltype = SQL_INT64 + 1;
		sqlda->sqlvar[0].sqlind  = &idfirst_flag;

		sqlda->sqlvar[1].sqldata = (char *)&IDLast;
		sqlda->sqlvar[1].sqltype = SQL_INT64 + 1;
		sqlda->sqlvar[1].sqlind  = &idlast_flag;

		sqlda->sqlvar[2].sqldata = (char *)&Discounts;
		sqlda->sqlvar[2].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[2].sqlind  = &discounts_flag;

		sqlda->sqlvar[3].sqldata = (char *)&IssueDate;
		sqlda->sqlvar[3].sqltype = SQL_DATE + 1;
		sqlda->sqlvar[3].sqlind  = &issue_flag;

		sqlda->sqlvar[4].sqldata = (char *)&ExpDate;
		sqlda->sqlvar[4].sqltype = SQL_DATE + 1;
		sqlda->sqlvar[4].sqlind  = &exp_flag;

		sqlda->sqlvar[5].sqldata = (char *)&CustType;
		sqlda->sqlvar[5].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[5].sqlind  = &custtype_flag;

		sqlda->sqlvar[6].sqldata = (char ISC_FAR*)&summa;
		sqlda->sqlvar[6].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[6].sqlind  = &summa_flag;

		sqlda->sqlvar[7].sqldata = (char *)&BonusCard;
		sqlda->sqlvar[7].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[7].sqlind  = &bonuscard_flag;

		sqlda->sqlvar[8].sqldata = (char *)&ParType;
		sqlda->sqlvar[8].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[8].sqlind  = &partype_flag;


		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		//CashReg->DbfMutex->lock();
		//CashReg->CustRangeTable->CloseDatabase();//store customer's range list into local table
		//CashReg->CustRangeTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+"tcustrange.dbf").c_str(),CustRangeTableRecord);
		//CashReg->DbfMutex->unlock();

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			int i;
			CustRangeStruct tmpCustRange;

			if (idfirst_flag==0)
				tmpCustRange.IdFirst=IDFirst;
			else
				tmpCustRange.IdFirst=0;

			if (idlast_flag==0)
				tmpCustRange.IdLast=IDLast;
			else
				tmpCustRange.IdLast=0;

			tmpCustRange.Discounts="";
			if (discounts_flag==0)
				for (i=0;i<Discounts.vary_length;i++)
					tmpCustRange.Discounts+=Discounts.vary_string[i];

			if (issue_flag==0)
				isc_decode_date(&IssueDate,&(tmpCustRange.issue));
			else
				tmpCustRange.issue.tm_mday=tmpCustRange.issue.tm_mon=tmpCustRange.issue.tm_year=0;

			if (exp_flag==0)
				isc_decode_date(&ExpDate,&(tmpCustRange.expire));
			else
				tmpCustRange.expire.tm_mday=tmpCustRange.expire.tm_mon=tmpCustRange.expire.tm_year=0;

			if (custtype_flag==0)
				tmpCustRange.CustType=CustType;
			else
				tmpCustRange.CustType=0;
            //;;Log((char*)Double2Str(summa).c_str());
			if (summa_flag==0)
				tmpCustRange.Summa=summa;
			else
				tmpCustRange.Summa=0;

			if (bonuscard_flag==0)
				tmpCustRange.BonusCard=BonusCard;
			else
				tmpCustRange.BonusCard=0;

			if (partype_flag==0)
				tmpCustRange.ParType=ParType;
			else
				tmpCustRange.ParType=0;

			CustRange.insert(CustRange.end(),tmpCustRange);
		}
		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#else
#endif
		CashReg->SQLMutex->unlock();
		CashReg->DbfMutex->lock();
		CashReg->CustRangeTable->CloseDatabase();//store customer's list into local table
		//CashReg->ShowMessage("pre CreateDatabase");
		CashReg->CustRangeTable->CreateDatabase((CashReg->DBGoodsPath+QDir::separator()+CUSTRANGE).c_str(),CustRangeTableRecord);
		//CashReg->ShowMessage("post CreateDatabase");
		for (unsigned int i=0;i<CustRange.size();i++)
   		{
			CashReg->CustRangeTable->BlankRecord();
			CashReg->CustRangeTable->PutLLongField("IDFIRST",CustRange[i].IdFirst);
			CashReg->CustRangeTable->PutLLongField("IDLAST",CustRange[i].IdLast);
			CashReg->CustRangeTable->PutField("DISCOUNTS",CustRange[i].Discounts.c_str());
			char date_buf[9];date_buf[8]='\0';
			if (CustRange[i].issue.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",CustRange[i].issue.tm_year+1900);
				sprintf(date_buf+4,"%02d",CustRange[i].issue.tm_mon+1);
				sprintf(date_buf+6,"%02d",CustRange[i].issue.tm_mday);
				CashReg->CustRangeTable->PutField("ISSUE",date_buf);
			}
			else
				CashReg->CustRangeTable->PutField("ISSUE","19000101");

			if (CustRange[i].expire.tm_mday!=0)
			{
				sprintf(date_buf,"%04d",CustRange[i].expire.tm_year+1900);
				sprintf(date_buf+4,"%02d",CustRange[i].expire.tm_mon+1);
				sprintf(date_buf+6,"%02d",CustRange[i].expire.tm_mday);
				CashReg->CustRangeTable->PutField("EXPIRE",date_buf);
			}
			else
				CashReg->CustRangeTable->PutField("EXPIRE","21000101");
            CashReg->CustRangeTable->PutLongField("CUSTTYPE",CustRange[i].CustType);
            CashReg->CustRangeTable->PutDoubleField("SUMMA",CustRange[i].Summa);
            CashReg->CustRangeTable->PutLongField("BONUSCARD",CustRange[i].BonusCard);
            CashReg->CustRangeTable->PutLongField("PARTYPE",CustRange[i].ParType);
			CashReg->CustRangeTable->AppendRecord();
		}
		CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
}

void SQLThread::SendPayment(void)//lazy write of payment
{
    //;;Log(" ==== >> SendPayment");
	DbfTable * tbl = CashReg->PaymentToSendTable;

	try
	{
			CashReg->DbfMutex->lock();
			if (!tbl->GetFirstRecord())
			{
				tbl->Zap();
				CashReg->DbfMutex->unlock();
				return;
			}

			do{
				xbLong SaveRecNo = tbl->GetRecNo();
//;;Log((char*)Int2Str(SaveRecNo).c_str());
				PaymentInfo tmpPayment;  //for each payment create record and send it to server

				string CurDate,CurTime,Saleman_Name;
				int SalemanID = tbl->GetLongField("CASHMAN");
				int CheckNum  = tbl->GetLongField("CHECKNUM");
				if(!CheckNum)
				{
					tbl->DeleteRecord();
					continue;
				}

				tmpPayment.Type		= tbl->GetLongField("TYPE");
				tmpPayment.Number	= tbl->GetStringField("NUMBER");
				tmpPayment.Summa	= tbl->GetDoubleField("SUMMA");
//	{"CUSTOMER",XB_NUMERIC_FLD,10,0},//code of the customer with discount card
//	{"INPUTKIND",XB_NUMERIC_FLD,3,0},//code of the customer with discount card
				tmpPayment.KKMNo	= tbl->GetLongField("KKMNO");
				tmpPayment.KKMSmena	= tbl->GetLongField("KKMSMENA");
				tmpPayment.KKMTime	= tbl->GetLongField("KKMTIME");
				tmpPayment.Customer = tbl->GetLLongField("CUSTOMER");

				CurTime			= tbl->GetStringField("TIME");
				CurDate			= xbDate().FormatDate(DateTemplate,tbl->GetField("DATE").c_str());
				CashReg->DbfMutex->unlock();

				//create SQL-statement
				string Insert_str=
				 "INSERT INTO MGPAYMENT (IDPODR,DATEPAYMENT,TIMEPAYMENT,DTSALE,KASSANUM,CHECKNUM,\
							TYPEPAYMENT,NUMBER,SUMMA,CASHMAN_ID,CUSTOMER_ID,KKMNO,KKMSMENA,KKMTIME,VERSION) VALUES(";
				Insert_str+=CashReg->GetMagNum()+",'";
				Insert_str+=CurDate+"','30.12.1899 ";
				Insert_str+=CurTime+"','";
				Insert_str+=CurDate+" "+CurTime+"',";
				Insert_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
				Insert_str+=Int2Str(CheckNum)+",";

				Insert_str+=Int2Str(tmpPayment.Type)+",";
				Insert_str+="'"+tmpPayment.Number+"',";
				Insert_str+=TruncZeros(Double2Str(tmpPayment.Summa))+",";

				Insert_str+=Int2Str(SalemanID)+",";
				Insert_str+=Int2Str(0)+",";
				Insert_str+=Int2Str(tmpPayment.KKMNo)+",";
				Insert_str+=Int2Str(tmpPayment.KKMSmena)+",";
				Insert_str+=Int2Str(tmpPayment.KKMTime)+",";
				Insert_str+=Int2Str(revision);
				Insert_str+=")";

				CashReg->SQLMutex->lock();
//;Log(">>> Insert_str:");
//;;Log((char*)Insert_str.c_str());

				AttachToDB();

				StartTran();
#ifdef IBASE
				if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_str.c_str(),1,NULL))
					throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);

				SQLAllocStmt(hdbc,&hstmt);
				if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
					throw cash_error(sql_error,0,GetErrorDescription().c_str());

				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);
				hstmt=SQL_NULL_HSTMT;
#endif

//*

                if ((tmpPayment.Type==10) || (tmpPayment.Type==15))
                {
                    //;;Log("tmpPayment.Type==10");
                    // сертификаты запишем отдельно
                    string Insert_Cert_str=
                     "INSERT INTO MG_PAY_CERTIFICATE(IDPODR,DATESALE,TIMESALE,DTSALE,CASHDESK,CHECKNUM,\
                                CERTIFICATE,SUMMA,CASHMAN,IDCUST,STATUS,VERSION) VALUES(";
                    Insert_Cert_str+=CashReg->GetMagNum()+",'";
                    Insert_Cert_str+=CurDate+"','30.12.1899 ";
                    Insert_Cert_str+=CurTime+"','";
                    Insert_Cert_str+=CurDate+" "+CurTime+"',";
                    Insert_Cert_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
                    Insert_Cert_str+=Int2Str(CheckNum)+",";

                    Insert_Cert_str+=tmpPayment.Number+",";
                    Insert_Cert_str+=TruncZeros(Double2Str(tmpPayment.Summa))+",";

                    Insert_Cert_str+=Int2Str(SalemanID)+",";
                    Insert_Cert_str+=LLong2Str(tmpPayment.Customer)+",";
                    Insert_Cert_str+=Int2Str(tmpPayment.Type)+",";
                    Insert_Cert_str+=Int2Str(revision);
                    Insert_Cert_str+=")";

                  //  AttachToDB();

                    //StartTran();

#ifdef IBASE
                    // сертификаты
                    if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_Cert_str.c_str(),1,NULL))
                        throw cash_error(sql_error,0,GetErrorDescription().c_str());
 #else
                    if (hstmt!=SQL_NULL_HSTMT)
                        SQLFreeStmt(hstmt,SQL_DROP);

                    SQLAllocStmt(hdbc,&hstmt);
                    if (SQLExecDirect(hstmt,(unsigned char*)Insert_Cert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
                        throw cash_error(sql_error,0,GetErrorDescription().c_str());

                    if (hstmt!=SQL_NULL_HSTMT)
                        SQLFreeStmt(hstmt,SQL_DROP);
                    hstmt=SQL_NULL_HSTMT;
#endif
//;;Log(">>> Insert_Cert_str:");
//;;Log((char*)Insert_Cert_str.c_str());

                   // CommitTran();

                }
//*/

//*
                // Банковские бонусы "Спасибо" Сбербанк
                if (tmpPayment.Type==11)
                {
    //;;Log("tmpPayment.Type==11");
                    // сертификаты запишем отдельно
                    string Insert_Bank_str=
                     "INSERT INTO MG_PAY_BANKBONUS(IDPODR,DATESALE,TIMESALE,DTSALE,CASHDESK,CHECKNUM,\
                                CARD,SUMMA,CASHMAN,IDCUST,VERSION) VALUES(";
                    Insert_Bank_str+=CashReg->GetMagNum()+",'";
                    Insert_Bank_str+=CurDate+"','30.12.1899 ";
                    Insert_Bank_str+=CurTime+"','";
                    Insert_Bank_str+=CurDate+" "+CurTime+"',";
                    Insert_Bank_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
                    Insert_Bank_str+=Int2Str(CheckNum)+",";

                    Insert_Bank_str+=+"'"+tmpPayment.Number+"',";
                    Insert_Bank_str+=TruncZeros(Double2Str(tmpPayment.Summa))+",";

                    Insert_Bank_str+=Int2Str(SalemanID)+",";
                    Insert_Bank_str+=LLong2Str(tmpPayment.Customer)+",";
                    Insert_Bank_str+=Int2Str(revision);
                    Insert_Bank_str+=")";

//;;Log(">>> Insert_Bank_str:");
//;;Log((char*)Insert_Bank_str.c_str());
                  //  AttachToDB();

                    //StartTran();

#ifdef IBASE
                    //
                    if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_Bank_str.c_str(),1,NULL))
                        throw cash_error(sql_error,0,GetErrorDescription().c_str());
 #else
                    if (hstmt!=SQL_NULL_HSTMT)
                        SQLFreeStmt(hstmt,SQL_DROP);

                    SQLAllocStmt(hdbc,&hstmt);
                    if (SQLExecDirect(hstmt,(unsigned char*)Insert_Bank_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
                        throw cash_error(sql_error,0,GetErrorDescription().c_str());

                    if (hstmt!=SQL_NULL_HSTMT)
                        SQLFreeStmt(hstmt,SQL_DROP);
                    hstmt=SQL_NULL_HSTMT;
#endif

                   // CommitTran();

                }


                // Бонусы Гастроном
                if (tmpPayment.Type==12)
                {
                    string Insert_Bonus_str=
                     "INSERT INTO MG_PAY_BONUS(IDPODR,DATESALE,TIMESALE,DTSALE,CASHDESK,CHECKNUM,\
                                CARD,SUMMA,CASHMAN,IDCUST,VERSION) VALUES(";
                    Insert_Bonus_str+=CashReg->GetMagNum()+",'";
                    Insert_Bonus_str+=CurDate+"','30.12.1899 ";
                    Insert_Bonus_str+=CurTime+"','";
                    Insert_Bonus_str+=CurDate+" "+CurTime+"',";
                    Insert_Bonus_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
                    Insert_Bonus_str+=Int2Str(CheckNum)+",";

                    Insert_Bonus_str+=+"'"+tmpPayment.Number+"',";
                    Insert_Bonus_str+=TruncZeros(Double2Str(tmpPayment.Summa))+",";

                    Insert_Bonus_str+=Int2Str(SalemanID)+",";
                    Insert_Bonus_str+=LLong2Str(tmpPayment.Customer)+",";
                    Insert_Bonus_str+=Int2Str(revision);
                    Insert_Bonus_str+=")";

//;;Log(">>> Insert_Bank_str:");
//;;Log((char*)Insert_Bank_str.c_str());
                  //  AttachToDB();

                    //StartTran();

#ifdef IBASE
                    //
                    if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_Bonus_str.c_str(),1,NULL))
                        throw cash_error(sql_error,0,GetErrorDescription().c_str());
 #else
                    if (hstmt!=SQL_NULL_HSTMT)
                        SQLFreeStmt(hstmt,SQL_DROP);

                    SQLAllocStmt(hdbc,&hstmt);
                    if (SQLExecDirect(hstmt,(unsigned char*)Insert_Bonus_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
                        throw cash_error(sql_error,0,GetErrorDescription().c_str());

                    if (hstmt!=SQL_NULL_HSTMT)
                        SQLFreeStmt(hstmt,SQL_DROP);
                    hstmt=SQL_NULL_HSTMT;
#endif

                   // CommitTran();

                }


                CommitTran();


				CashReg->SQLMutex->unlock();

				CashReg->DbfMutex->lock();
				tbl->SetRecNo(SaveRecNo);
				tbl->DeleteRecord();
			}
			while(tbl->GetNextRecord());
			tbl->Zap();
			CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,true);
	}
}

void SQLThread::GetCheckHeader(string * ResultStr,int crCustomer)
{
	*ResultStr = "";

	try
	{
		//CashReg->SQLMutex->lock();
		Log("SQLThread::GetCheckHeader");
		AttachToDB();
		StartTran();
		char sel_str[1024];
		sprintf(sel_str,"select number,line from GetCheckHeader(%d) order by number",crCustomer);
#ifdef IBASE
		long number;
		SQL_VARCHAR(1024) line;
		short number_flag=0,line_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(5)];
		sqlda->sqln = 2;
		sqlda->sqld = 2;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//^^
		sqlda->sqlvar[0].sqldata = (char *)&number;
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &number_flag;

		sqlda->sqlvar[1].sqldata = (char *)&line;
		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[1].sqlind  = &line_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			int i;
			string tmpstr;

			tmpstr="";
			if (line_flag==0)
				for (i=0;i<line.vary_length;i++)
					tmpstr+=line.vary_string[i];
			*ResultStr+=tmpstr;
			*ResultStr+='\n';

		}
		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#endif
		//CashReg->SQLMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
}

bool SQLThread::IssueCertificate(char* key,string * ResultStr, string * HeaderStr,double * ptrSumma,long long crCustomer,double Summa, string * ChangeCardStr, string * MillionCardStr, char* mil_key, string * ActionVTBStr, char* actionVTB_key,double * ptrActionSumma, double fullsum, string * ActionText)
{
    //+ dms === Акция ВТБ 24 ===
	//if(crCustomer==0) return false;
	//+ dms === Акция ВТБ 24 ===

	int Issue=0;
	int Change=0;
	int Million=0;
	int Action=0;
	int ResAction=0;

	*ptrSumma=0;
	*ptrActionSumma=0;

	uuid_t id;

	uuid_generate(id);
	uuid_unparse(id,key);

    // сгенерируем уникальный код для тортов на заказ
	uuid_t mil_id;

	uuid_generate(mil_id);
	uuid_unparse(mil_id,mil_key);

// акция
	uuid_t act_id;

	uuid_generate(act_id);
	uuid_unparse(act_id,actionVTB_key);

	try
	{
	    //CashReg->SQLMutex->lock();

	    char logstr[100];
		sprintf(logstr,"IssueCertificate %lld",crCustomer);
		Log(logstr);
//;;Log("+ AttachToDB +");
		AttachToDB();
//;;Log("- AttachToDB -");
		StartTran();
		char sel_str[1024];
        //sprintf(sel_str,"select curdatetime,issue,summa,totalsum from IssueCertificate('%s',%lld,%8.2f,%s)",key,crCustomer,Summa,CashReg->GetMagNum().c_str());
        sprintf(sel_str,"select curdatetime,issue,summa,totalsum,change,million,action,actionsumma,resaction from IssueCertificate_vtb('%s',%lld,%8.2f,%s,%d,%d,'%s','%s',%8.2f)",key,crCustomer,Summa,CashReg->GetMagNum().c_str(),CashReg->GetCashRegisterNum(), CashReg->GetLastCheckNum()+1, mil_key, actionVTB_key, fullsum);

//;;Log(sel_str);

#ifdef IBASE
		long issue, change, million, action, resaction;
		double summa=0, totalsum=0, TotalSumma=0, actionsumma=0;
		ISC_QUAD curdatetime;
		tm ServDateTime;

		SQL_VARCHAR(1024) line;
		short datetime_flag=0,issue_flag=0,summa_flag=0,totalsum_flag=0, change_flag=0, million_flag=0, action_flag=0, resaction_flag=0, actionsumma_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(8)];
		sqlda->sqln = 9;
		sqlda->sqld = 9;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//^^

/*    CURDATETIME DATE,
    ISSUE INTEGER,
    SUMMA NUMERIC(15,2),
    TOTALSUM NUMERIC(15,2)*/

        sqlda->sqlvar[0].sqldata = (char *)&curdatetime;
		sqlda->sqlvar[0].sqltype = SQL_DATE + 1;
		sqlda->sqlvar[0].sqlind  = &datetime_flag;

		sqlda->sqlvar[1].sqldata = (char *)&issue;
		sqlda->sqlvar[1].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[1].sqlind  = &issue_flag;

		sqlda->sqlvar[2].sqldata = (char ISC_FAR*)&summa;
		sqlda->sqlvar[2].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[2].sqlind  = &summa_flag;

		sqlda->sqlvar[3].sqldata = (char ISC_FAR*)&totalsum;
		sqlda->sqlvar[3].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[3].sqlind  = &totalsum_flag;

		sqlda->sqlvar[4].sqldata = (char *)&change;
		sqlda->sqlvar[4].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[4].sqlind  = &change_flag;

		sqlda->sqlvar[5].sqldata = (char *)&million;
		sqlda->sqlvar[5].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[5].sqlind  = &million_flag;

		sqlda->sqlvar[6].sqldata = (char *)&action;
		sqlda->sqlvar[6].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[6].sqlind  = &action_flag;

		sqlda->sqlvar[7].sqldata = (char ISC_FAR*)&actionsumma;
		sqlda->sqlvar[7].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[7].sqlind  = &actionsumma_flag;

		sqlda->sqlvar[8].sqldata = (char *)&resaction;
		sqlda->sqlvar[8].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[8].sqlind  = &resaction_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
//			int i;
//			string tmpstr;
//
//			tmpstr="";
//			if (line_flag==0)
//				for (i=0;i<line.vary_length;i++)
//					tmpstr+=line.vary_string[i];
//			*ResultStr+=tmpstr;
//			*ResultStr+='\n';

			if (datetime_flag==0)
				isc_decode_date(&curdatetime,&(ServDateTime));

            if (issue_flag==0)
				Issue+=issue;

			if (summa_flag==0)
                *ptrSumma+=summa;

			if (totalsum_flag==0)
				TotalSumma+=totalsum;

            if (change_flag==0)
				Change+=change;

            if (million_flag==0)
				Million+=million;

            if (action_flag==0)
				Action+=action;

			if (actionsumma_flag==0)
                *ptrActionSumma+=actionsumma;

            if (resaction_flag==0)
				ResAction+=resaction;

		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

        char tmpstr[1024];
        sprintf(tmpstr,"%02d.%02d.%4d %02d:%02d:%02d",
        ServDateTime.tm_mday,
        ServDateTime.tm_mon + 1,
        ServDateTime.tm_year + 1900,
        ServDateTime.tm_hour,
        ServDateTime.tm_min,
        ServDateTime.tm_sec);

        string strdate, strkey;
        strdate = tmpstr;
        strkey  = key;


// С 01 июля 2020 г. отменяется выдача сертификатов за накопления. Вместо сертификатов теперь будут начисляться бонусы на карту
        *HeaderStr ="\nПоздравляем! Вы накопили "+GetRounded(TotalSumma,0.01);
        *HeaderStr +="\nВам будет начислено "+GetRounded(*ptrSumma,0.01)+" бонусов";
        *HeaderStr +="\nв подарок на карту Гастроном!";
        *ResultStr = *HeaderStr;

/* Отменяется
//        *HeaderStr = strdate+"\n"+strkey;
        *HeaderStr = strdate+"\n";
        *HeaderStr += GetFormatString("Касса #"+Int2Str(CashReg->GetCashRegisterNum()),"Чек #"+Int2Str(CashReg->GetLastCheckNum()+1));
        *HeaderStr +="\nКассир " + CashReg->CurrentSaleman->Name;
        *ResultStr = "";
        *ResultStr +=GetFormatString("Выдача сертификата ", GetRounded(*ptrSumma,0.01));
        //*ResultStr +="\nДисконтная карта № " + LLong2Str(crCustomer);
        *ResultStr +="\nДисконтная карта № " + Card2Str(crCustomer);
        *ResultStr +="\n" + GetFormatString("Накопленная сумма ", " ="+GetRounded(TotalSumma,0.01));

*/
//

        if (Change>0)
        {
            *ChangeCardStr = "ЗАМЕНА КАРТЫ!\n";
            if (Change>=5) {
                *ChangeCardStr +="Super-VIP";
            }
            *ChangeCardStr +="\nКарта № " + Card2Str(crCustomer);
            *ChangeCardStr +="\n" + GetFormatString("Накопленная сумма ", " ="+GetRounded(TotalSumma,0.01));
            *ChangeCardStr +="\n\nОкно замены карты выйдет автоматически после отбития чека.";
            *ChangeCardStr +="\nДождитесь окончания чека!";
        }

        if (ResAction>0)
        {
            *ActionText = "БЕСПЛАТНЫЙ КОФЕ В ПОДАРОК!\n";
        }

        if (Million>0)
        {
            *MillionCardStr = "";
            //*MillionCardStr = "Кассир " + CashReg->CurrentSaleman->Name + "\n";
            *MillionCardStr +="\nВыдача сертификата \"ТОРТ НА ЗАКАЗ\"!\n";
            //*MillionCardStr +="\nКарта N " + LLong2Str(crCustomer);
            *MillionCardStr +="\nКарта N " + Card2Str(crCustomer);
            *MillionCardStr +="\n" + GetFormatString("Накопленная сумма ", " ="+GetRounded(TotalSumma,0.01));
        }

        if (Action>0)
        {

            *ActionVTBStr = "============= ПОДАРОК ================";
            //*ActionVTBStr += "\n\n Дисконтная карта N " + LLong2Str(crCustomer);
            *ActionVTBStr += "\n\n Дисконтная карта N " + Card2Str(crCustomer);
            *ActionVTBStr += "\n\n"+GetFormatString(" Накопленная сумма ", " ="+GetRounded(*ptrActionSumma,0.01));
            *ActionVTBStr += "\n\n"+GetFormatString(" Количество подарков ", " ="+Int2Str(Action));

/*
            *ActionVTBStr = "";
            *ActionVTBStr +="\n"+GetFormatString("Выдача сертификата ВТБ24 ", GetRounded(*ptrActionSumma,0.01));
            *ActionVTBStr +="\nКарта N " + LLong2Str(crCustomer);
            *ActionVTBStr +="\n" + GetFormatString("Сумма чека ", " ="+GetRounded(Summa,0.01));
*/
        }

#endif
		//CashReg->SQLMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
		return false;
	}

	if (Issue>0) return true;

	return false;
}


bool SQLThread::IssueCertificateCommit(char* key,string * ResultStr,IssueCertStruct cards[], int crCashdesk,int crCashman,int crGuard,long long crCustomer, double IssueSumma, int StatusOK)
//bool SQLThread::IssueCertificateCommit(char* key,string * ResultStr, int crCashdesk,int crCashman,int crGuard,long long crCustomer, double IssueSumma)
{
    int cnt = 0;

	try
	{

     Log("SQLThread::IssueCertificateCommit");

//	 for (int k=0; k<100000; k++);

     *ResultStr ="";
	 for(int k=0; k<IssueCertCount; k++)
	 {
		//CashReg->SQLMutex->lock();

        // Сертификат торт на заказ может быь с 0-й суммой
		//!if (cards[k].Summa == 0) continue;
		if (cards[k].Certificate == 0) continue;

		long long CertificateNumber;
		CertificateNumber = cards[k].Certificate;

// log
        char str[1024];
        sprintf(str, "Issue card #%lld = %8.2f", cards[k].Certificate, cards[k].Summa);
        Log(str);


		AttachToDB();
		StartTran();
		char sel_str[1024];

       sprintf(sel_str,"select number,line from IssueCertificate_Commit('%s',%lld,%d,%d,%d,%s,%lld,%15.2f,%15.2f,%d,%d) order by number",key,crCustomer,crCashdesk,crCashman,crGuard,CashReg->GetMagNum().c_str(),cards[k].Certificate ,cards[k].Summa, IssueSumma, CashReg->GetLastCheckNum()+1, StatusOK);

//;;Log(sel_str);

#ifdef IBASE
		long number;
		SQL_VARCHAR(1024) line;
		short number_flag=0,line_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(5)];
		sqlda->sqln = 2;
		sqlda->sqld = 2;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//^^
		sqlda->sqlvar[0].sqldata = (char *)&number;
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &number_flag;

		sqlda->sqlvar[1].sqldata = (char *)&line;
		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[1].sqlind  = &line_flag;

//;;Log("\nbefore IssueCertificateCommit execute");
		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			int i;
			string tmpstr;

			tmpstr="";
			if (line_flag==0)
				for (i=0;i<line.vary_length;i++)
					tmpstr+=line.vary_string[i];
			//*ResultStr+=tmpstr;
			//*ResultStr+='\n';
		}
		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

        cnt+=1;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#endif
		//CashReg->SQLMutex->unlock();
     } //for

    if (cnt>0)
        {

        if ((StatusOK==10)||(StatusOK==20))
        {
            *ResultStr += "\nФИО получателя:";
            *ResultStr += "\n";
            *ResultStr += "\n____________________________________";
            *ResultStr += "\nТелефон:";
            *ResultStr += "\n";
            *ResultStr += "\n____________________________________";
        }
        else
        {
            *ResultStr += "\nСертификат получил:";
            *ResultStr += "\n";
            *ResultStr += "\n____________________________________";
        }
        *ResultStr += "\nСертификат выдал:";
        *ResultStr += "\n";
        *ResultStr += "\n____________________________________";
        *ResultStr += "\n";
        *ResultStr += "\n";
        *ResultStr += "\n";
        *ResultStr += "\n";
        *ResultStr += "printheader\n";
        *ResultStr += "cutcheck\n";
        }
    else
        {
        *ResultStr = "";
        }

	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
		return false;
	}

	return true;
}



bool SQLThread::IssueCertificateRollback(char* key,string * ResultStr,int crCertificate,int crCashdesk,int crCashman,int crGuard, int StatusCancel)
{
	try
	{
		//CashReg->SQLMutex->lock();
		Log("SQLThread::IssueCertificateRollback");
		AttachToDB();
		StartTran();
		char sel_str[1024];
		sprintf(sel_str,"select number,line from IssueCertificate_Rollback('%s',%d,%d,%d,%d,%s,%d) order by number",key,crCertificate,crCashdesk,crCashman,crGuard,CashReg->GetMagNum().c_str(), StatusCancel);
		//;;Log(sel_str);

#ifdef IBASE
		long number;
		SQL_VARCHAR(1024) line;
		short number_flag=0,line_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(5)];
		sqlda->sqln = 2;
		sqlda->sqld = 2;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//^^
		sqlda->sqlvar[0].sqldata = (char *)&number;
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &number_flag;

		sqlda->sqlvar[1].sqldata = (char *)&line;
		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
		sqlda->sqlvar[1].sqlind  = &line_flag;


		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			int i;
			string tmpstr;

			tmpstr="";
			if (line_flag==0)
				for (i=0;i<line.vary_length;i++)
					tmpstr+=line.vary_string[i];
			*ResultStr+=tmpstr;
			*ResultStr+='\n';
		}
		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

        if (StatusCancel==22)
        {
            *ResultStr = "\n-ВЫДАЧА СЕРТИФИКАТА ВТБ24 ОТМЕНЕНА-";
            *ResultStr += "\nСертификат ВТБ24:";
            *ResultStr += "\n";
            *ResultStr += "\n____________________________________";
            *ResultStr += "\nФИО получателя:";
            *ResultStr += "\n";
            *ResultStr += "\n____________________________________";
            *ResultStr += "\nТелефон:";
            *ResultStr += "\n";
            *ResultStr += "\n____________________________________";
            *ResultStr += "\nСертификат выдал:";
            *ResultStr += "\n";
            *ResultStr += "\n____________________________________";
            *ResultStr += "\n";
            *ResultStr += "\n";
            *ResultStr += "\n";
            *ResultStr += "\n";
            *ResultStr += "printheader\n";
            *ResultStr += "cutcheck\n";
        }
        else
        {

            *ResultStr = "\n----Выдача сертификата отменена-----";
            *ResultStr += "\n";
            *ResultStr += "\n";

        }

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#endif
		//CashReg->SQLMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
		return false;
	}

	return true;
}


//bool SQLThread::IssueMillionCertificateCommit(char* key,string * ResultStr,char* card, int crCashdesk,int crCashman,int crGuard,long long crCustomer, double IssueSumma)
//{
//    int cnt = 0;
//
//	try
//	{
//
//     Log("SQLThread::Issue_Million_CertificateCommit");
//
//     *ResultStr ="";
//
//		//CashReg->SQLMutex->lock();
//
//
//		AttachToDB();
//		StartTran();
//		char sel_str[1024];
//
//       sprintf(sel_str,"select number,line from Issue_Million_Cert_Commit('%s',%lld,%d,%d,%d,%s,%lld,%15.2f,%15.2f,%d) order by number","",crCustomer,crCashdesk,crCashman,crGuard,CashReg->GetMagNum().c_str(),0 ,0, 0, CashReg->GetLastCheckNum()+1);
//
////;;Log(sel_str);
//
//#ifdef IBASE
//		long number;
//		SQL_VARCHAR(1024) line;
//		short number_flag=0,line_flag=0;
//		long fetch_stat;
//
//		if (sqlda!=NULL)
//		{
//			delete sqlda;
//			sqlda=NULL;
//		}
//
//		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(5)];
//		sqlda->sqln = 2;
//		sqlda->sqld = 2;
//		sqlda->version = SQLDAVERSION;
//
//		ibstmt=NULL;
//
//		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//
//		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
////^^
//		sqlda->sqlvar[0].sqldata = (char *)&number;
//		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
//		sqlda->sqlvar[0].sqlind  = &number_flag;
//
//		sqlda->sqlvar[1].sqldata = (char *)&line;
//		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
//		sqlda->sqlvar[1].sqlind  = &line_flag;
//
////;;Log("\nbefore IssueCertificateCommit execute");
//		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//
//		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
//		{
//			int i;
//			string tmpstr;
//
//			tmpstr="";
//			if (line_flag==0)
//				for (i=0;i<line.vary_length;i++)
//					tmpstr+=line.vary_string[i];
//			//*ResultStr+=tmpstr;
//			//*ResultStr+='\n';
//		}
//		if (fetch_stat!=100)
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//
//		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//
//		ibstmt=NULL;
//
//		CommitTran();
//
//        cnt+=1;
//
//		if (sqlda!=NULL)
//		{
//			delete sqlda;
//			sqlda=NULL;
//		}
//#endif
//		//CashReg->SQLMutex->unlock();
//
//
//    if (cnt>0)
//        {
//        *ResultStr += "\nСертификат N _______________________";
//        *ResultStr += "\n";
//        *ResultStr += "\nСертификат получил:";
//        *ResultStr += "\n";
//        *ResultStr += "\n____________________________________";
//        *ResultStr += "\n";
//        *ResultStr += "\n";
//        *ResultStr += "\n";
//        *ResultStr += "\n";
//        *ResultStr += "printheader\n";
//        *ResultStr += "cutcheck\n";
//        }
//    else
//        {
//        *ResultStr = "";
//        }
//
//	}
//	catch(cash_error& ex)
//	{
//		ProcessError(ex,false);
//		return false;
//	}
//
//	return true;
//}
//
//
//
//bool SQLThread::IssueMillionCertificateRollback(char* key,string * ResultStr,int crCashdesk,int crCashman,int crGuard,long long crCustomer, long long crCertificate)
//{
//	try
//	{
//		//CashReg->SQLMutex->lock();
//		Log("SQLThread::IssueCertificateRollback");
//		AttachToDB();
//		StartTran();
//		char sel_str[1024];
//		sprintf(sel_str,"select number,line from Issue_Million_Cert_Rollback('%s',%lld, %lld,%d,%d,%d,%s) order by number",
//		key, crCustomer,crCertificate,crCashdesk,crCashman,crGuard,CashReg->GetMagNum().c_str());
//#ifdef IBASE
//		long number;
//		SQL_VARCHAR(1024) line;
//		short number_flag=0,line_flag=0;
//		long fetch_stat;
//
//		if (sqlda!=NULL)
//		{
//			delete sqlda;
//			sqlda=NULL;
//		}
//
//		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(5)];
//		sqlda->sqln = 2;
//		sqlda->sqld = 2;
//		sqlda->version = SQLDAVERSION;
//
//		ibstmt=NULL;
//
//		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//
//		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
////^^
//		sqlda->sqlvar[0].sqldata = (char *)&number;
//		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
//		sqlda->sqlvar[0].sqlind  = &number_flag;
//
//		sqlda->sqlvar[1].sqldata = (char *)&line;
//		sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
//		sqlda->sqlvar[1].sqlind  = &line_flag;
//
//
//		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//
//		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
//		{
//			int i;
//			string tmpstr;
//
//			tmpstr="";
//			if (line_flag==0)
//				for (i=0;i<line.vary_length;i++)
//					tmpstr+=line.vary_string[i];
//			*ResultStr+=tmpstr;
//			*ResultStr+='\n';
//		}
//		if (fetch_stat!=100)
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//
//		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
//			throw cash_error(sql_error,0,GetErrorDescription().c_str());
//
//		ibstmt=NULL;
//
//		CommitTran();
//
//        *ResultStr = "\n----Выдача сертификата отменена-----";
//        *ResultStr += "\n";
//        *ResultStr += "\n";
//
//		if (sqlda!=NULL)
//		{
//			delete sqlda;
//			sqlda=NULL;
//		}
//#endif
//		//CashReg->SQLMutex->unlock();
//	}
//	catch(cash_error& ex)
//	{
//		ProcessError(ex,false);
//		return false;
//	}
//
//	return true;
//}


// Sales,ReturnSales [0 - всего, 1..4 - по видам оплат][0 - кол-во, 1 - сумма]
int SQLThread::GetZReportFromDB(Z_Report *report, int smena)
{
    // Инициализация
    (*report).TotalSalesCheckCount = (*report).SalesCheckCount = (*report).TotalReturnSalesCheckCount = (*report).ReturnSalesCheckCount = 0;
    (*report).SalesSumma = (*report).ReturnSalesSumma = 0;
    for(int i=0;i<4;i++) (*report).SalesDetail[i] = (*report).ReturnSalesDetail[i] = 0;

//;;return 0;

	try
	{

		AttachToDB();

		StartTran();

        Log("GetZReportFromDB begin connect to server");

        string str_smena = smena>1?" and CHECKNUM="+Int2Str(smena-1):"";

        string str_action_date = "(select first 1 ACTION_DATE from mgcashdesklog where CASH_DESK="+
        Int2Str(CashReg->GetCashRegisterNum())+str_smena+" and ACTION_TYPE=10 order by ACTION_DATE desc, ACTION_TIME desc)";

        string str_action_time = "(select first 1 ACTION_TIME from mgcashdesklog where CASH_DESK="+
        Int2Str(CashReg->GetCashRegisterNum())+str_smena+" and ACTION_TYPE=10 order by ACTION_DATE desc, ACTION_TIME desc)";

        string sel_str="select\
         count(distinct case when tip=0 then checknum else 0 end) as salcnt,\
         count(distinct case when tip=1 then checknum else null end) as retcnt,\
         sum(case when paymenttype=0 then summa else 0 end ) as nal,\
         sum(case when paymenttype=0 and tip=0 then summa else 0 end ) as salnal,\
         sum(case when paymenttype<>0 and tip=0 then summa else 0 end) as salbnal,\
         sum(case when paymenttype=0 and tip=1 then summa else 0 end ) as retnal,\
         sum(case when paymenttype<>0 and tip=1 then summa else 0 end ) as retbnal,\
         max(checknum) as maxchecknum,\
         max(dtsale) as lastdate\
         from mgsales where (tip in (0,1)) and kodnom<>3 ";

        sel_str+=" and ((datesale > "+str_action_date+")";
        sel_str+=" or (( datesale = "+str_action_date+") and (timesale > "+str_action_time+")))";
        sel_str+=" and datesale<= 'today'";
        sel_str+=smena>0?" and kkmsmena = "+Int2Str(smena):"";
        sel_str+=" and kassanum= "+Int2Str(CashReg->GetCashRegisterNum());

		long type;

		double nal, salnal, salbnal, retnal, retbnal;
		long salcnt, retcnt, maxchecknum;
        ISC_QUAD lastdate;
        tm tmpLastDate;

		short salcnt_flag,retcnt_flag,nal_flag, salnal_flag, salbnal_flag, retnal_flag, retbnal_flag,maxchecknum_flag, lastdate_flag;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(9)];
		sqlda->sqln = 9;
		sqlda->sqld = 9;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		//ShowMessage(NULL,Int2Str(sqlda->sqld));

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,(char*)sel_str.c_str(),1,sqlda))
		{
			throw cash_error(sql_error,0,GetErrorDescription().c_str());
			//CommitTran();
			//CashReg->SQLMutex->unlock();
			//return;
		}

		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&salcnt;
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &salcnt_flag;

		sqlda->sqlvar[1].sqldata = (char ISC_FAR*)&retcnt;
		sqlda->sqlvar[1].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[1].sqlind  = &retcnt_flag;

		sqlda->sqlvar[2].sqldata = (char ISC_FAR*)&nal;
		sqlda->sqlvar[2].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[2].sqlind  = &nal_flag;

		sqlda->sqlvar[3].sqldata = (char ISC_FAR*)&salnal;
		sqlda->sqlvar[3].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[3].sqlind  = &salnal_flag;

		sqlda->sqlvar[4].sqldata = (char ISC_FAR*)&salbnal;
		sqlda->sqlvar[4].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[4].sqlind  = &salbnal_flag;

		sqlda->sqlvar[5].sqldata = (char ISC_FAR*)&retnal;
		sqlda->sqlvar[5].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[5].sqlind  = &retnal_flag;

		sqlda->sqlvar[6].sqldata = (char ISC_FAR*)&retbnal;
		sqlda->sqlvar[6].sqltype = SQL_DOUBLE + 1;
		sqlda->sqlvar[6].sqlind  = &retbnal_flag;

		sqlda->sqlvar[7].sqldata = (char ISC_FAR*)&maxchecknum;
		sqlda->sqlvar[7].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[7].sqlind  = &maxchecknum_flag;

   		sqlda->sqlvar[8].sqldata = (char *)&lastdate;
		sqlda->sqlvar[8].sqltype = SQL_DATE + 1;
		sqlda->sqlvar[8].sqlind  = &lastdate_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
            // кол-во чеков продаж
			if (salcnt_flag==0)
            {
				(*report).SalesCheckCount+=salcnt;
            }

            // кол-во чеков возвратов
			if (retcnt_flag==0)
            {
				(*report).ReturnSalesCheckCount+=retcnt;
				(*report).TotalReturnSalesCheckCount+=retcnt;
            }

            // нал
			if (salnal_flag==0)
            {
                (*report).SalesSumma+=salnal;
                (*report).SalesDetail[0]+=salnal;
            }

            // безнал
			if (salbnal_flag==0)
            {
                (*report).SalesSumma+=salbnal;
                (*report).SalesDetail[3]+=salbnal;
            }

            // возврат
			if (retnal_flag==0)
            {
                (*report).ReturnSalesSumma-=retnal;
                (*report).ReturnSalesDetail[0]-=retnal;
            }

            //всего чеков
			if (maxchecknum_flag==0)
            {
				(*report).TotalSalesCheckCount+=maxchecknum;
            }

            // последняя дата продажи по смене
			if ((lastdate_flag==0) && (smena>0))
				isc_decode_date(&lastdate,&((*report).Date));
			else
				(*report).Date.tm_mday=(*report).Date.tm_mon=(*report).Date.tm_year=(*report).Date.tm_hour=(*report).Date.tm_min=(*report).Date.tm_sec=0;

		}//end feth

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;
		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
		return 1;
	}

    return 0;
}

// Параметры:
// 1 - номер сертификата
// 2 - номер диск. карты для которой проверяетя привязка сертификата
// 3 - Проверять привязку сертификата к карте (Да/Нет)

int SQLThread::CheckCertificate(long long number, long long CurrentCustomer, bool CheckCustomer)
{

    if(number==0) return 2;

	int IsActive=0;
	long long ID_Customer=0;
    bool find = false;

	try
	{
	    //CashReg->SQLMutex->lock();

	    char logstr[100];
		sprintf(logstr,"--> CheckCertificate %lld",number);
		Log(logstr);

		AttachToDB();
		StartTran();
		char sel_str[1024];

        sprintf(sel_str,"select isactive, idcust from mgcertificate where id = %lld",number);

#ifdef IBASE
		long isact=0;
		long long idcust=0;

		short isactive_flag=0;
		short idcust_flag=0;
		long fetch_stat;


		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(2)];
		sqlda->sqln = 2;
		sqlda->sqld = 2;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());


		sqlda->sqlvar[0].sqldata = (char *)&isact;
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &isactive_flag;

		sqlda->sqlvar[1].sqldata = (char ISC_FAR*)&idcust;
		sqlda->sqlvar[1].sqltype = SQL_INT64 + 1;
		sqlda->sqlvar[1].sqlind  = &idcust_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{

            if (isactive_flag==0)
            {
                find = true;
				IsActive+=isact;
				;;Log((char*)("isact="+Int2Str(isact)).c_str());
            }

			if (idcust_flag==0)
				ID_Customer=idcust;
			else
				ID_Customer=0;

		}
		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

#endif
		//CashReg->SQLMutex->unlock();
	}
	catch(cash_error& ex)
	{
	    Log(" ... ERROR (-1)");
		ProcessError(ex,false);
		return -1;
	}

	if (IsActive>0)
	{
	    if ((!CheckCustomer) || (ID_Customer==0))
	    {
            // Если не нужно проверять привязку серт к диск карте
            // или в базе привязки нет, то все ОК

            Log(" ... SUCCESS");
            return 0;
	    }
	    else // Обязательная привязка сертификата к дисконтной карте
            if(ID_Customer==CurrentCustomer)
            {
                //  пройдена, карты совпадают все ОК
                string LogStr= "";
                LogStr = " ... SUCCESS WITH CUSTOMER ("+ LLong2Str(ID_Customer) +")";
                Log((char*)LogStr.c_str());

                return 0;
            }
            else
            {  // Сертификат активен, но дисконтная карта не та
                string LogStr= "";
                LogStr = " ... BLOCK WITH CUSTOMER (InBase="+ LLong2Str(ID_Customer) +", InCheque="+LLong2Str(CurrentCustomer)+")";
                Log((char*)LogStr.c_str());

                return 4;
            }
	}
    else
    {
        if (find)
        {
            Log(" ... BLOCK");
            return 1;
        }
        else
        {
            Log(" ... NOT FOUND");
            return 3;
        }
    }

}




int SQLThread::GetGoodsFromDB(FiscalCheque *FoundGoods, int fCode, string fBarcode, string fName, double fPrice)
{
    int cashdesk;int chequenum;string DateStr;FiscalCheque* ReturnCheck;
	try
	{
		CashReg->SQLMutex->lock();

		AttachToDB();

		StartTran();

/*		CODE        INTEGER,
    BARCODE     VARCHAR(20),
    GRCODE      INTEGER,
    NAME        VARCHAR(100),
    PRICE       NUMERIC(15,2),
    FIXEDPRICE  NUMERIC(15,2),
    GTYPE       CHAR(1),
    SKIDKA      VARCHAR(99),
    NDS         NUMERIC(15,2),
    MULT        NUMERIC(15,3),
    P0          NUMERIC(15,2),
    Q0          NUMERIC(15,2),
    NOTUSE      SMALLINT,
    LOCK        VARCHAR(50),
    AUTHOR      VARCHAR(50)
    */
		string sel_str="SELECT CODE, BARCODE, GRCODE, NAME, PRICE, FIXEDPRICE, GTYPE, SKIDKA, NDS, MULT, LOCK, MINPRICE FROM TOKAS WHERE NOTUSE=0";

        if (fCode)      sel_str += " AND CODE = "+ Int2Str(fCode);
        if (!fBarcode.empty())   sel_str += " AND BARCODE in ('"+ fBarcode+"','"+"0"+fBarcode+"')";
        if (!fName.empty())      sel_str += " AND Name LIKE '%"+ fName +"%'";
        if (fPrice)     sel_str += " AND Price = "+ Double2Str(fPrice);


        ;;Log((char*)(sel_str).c_str());

		long code,grcode;

		double price, fixedprice, nds, mult, minprice;
        string Prec="";
#ifdef IBASE
		SQL_VARCHAR(100) name;
		SQL_VARCHAR(20) barcode;
		SQL_VARCHAR(99) skidka;
		SQL_VARCHAR(1) gtype;
		SQL_VARCHAR(50) lock;


		short code_flag=0,barcode_flag=0,price_flag=0,grcode_flag=0,
			name_flag=0,fixedprice_flag=0,gtype_flag=0,skidka_flag=0,nds_flag=0,mult_flag=0, lock_flag=0, minprice_flag;
		long fetch_stat;
		int i=0;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(12)];
		sqlda->sqln = sqlda->sqld = 12;


		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,(char*)sel_str.c_str(),1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char ISC_FAR*)&code;
		sqlda->sqlvar[0].sqltype = SQL_LONG+1;
		sqlda->sqlvar[0].sqlind  = &code_flag;

		sqlda->sqlvar[1].sqldata = (char ISC_FAR*)&barcode;
		sqlda->sqlvar[1].sqltype = SQL_VARYING +1;
		sqlda->sqlvar[1].sqlind  = &barcode_flag;

		sqlda->sqlvar[2].sqldata = (char ISC_FAR*)&grcode;
		sqlda->sqlvar[2].sqltype = SQL_LONG+1;
		sqlda->sqlvar[2].sqlind  = &grcode_flag;

		sqlda->sqlvar[3].sqldata = (char ISC_FAR*)&name;
		sqlda->sqlvar[3].sqltype = SQL_VARYING +1;
		sqlda->sqlvar[3].sqlind  = &name_flag;

		sqlda->sqlvar[4].sqldata = (char ISC_FAR*)&price;
		sqlda->sqlvar[4].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[4].sqlind  = &price_flag;

		sqlda->sqlvar[5].sqldata = (char ISC_FAR*)&fixedprice;
		sqlda->sqlvar[5].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[5].sqlind  = &fixedprice_flag;

		sqlda->sqlvar[6].sqldata = (char ISC_FAR*)&gtype;
		sqlda->sqlvar[6].sqltype = SQL_VARYING+1;
		sqlda->sqlvar[6].sqlind  = &gtype_flag;

		sqlda->sqlvar[7].sqldata = (char ISC_FAR*)&skidka;
		sqlda->sqlvar[7].sqltype = SQL_VARYING+1;
		sqlda->sqlvar[7].sqlind  = &skidka_flag;

		sqlda->sqlvar[8].sqldata = (char ISC_FAR*)&nds;
		sqlda->sqlvar[8].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[8].sqlind  = &nds_flag;

		sqlda->sqlvar[9].sqldata = (char ISC_FAR*)&mult;
		sqlda->sqlvar[9].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[9].sqlind  = &mult_flag;

		sqlda->sqlvar[10].sqldata = (char ISC_FAR*)&lock;
		sqlda->sqlvar[10].sqltype = SQL_VARYING+1;
		sqlda->sqlvar[10].sqlind  = &lock_flag;

		sqlda->sqlvar[11].sqldata = (char ISC_FAR*)&minprice;
		sqlda->sqlvar[11].sqltype = SQL_DOUBLE+1;
		sqlda->sqlvar[11].sqlind  = &minprice_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
			GoodsInfo tmpGoods;

			if (code_flag==0)
				tmpGoods.ItemCode=code;
			else
				tmpGoods.ItemCode=0;

			tmpGoods.ItemBarcode = "";
			if (barcode_flag==0)
                for (i=0;i<barcode.vary_length;i++)
                    tmpGoods.ItemBarcode+=barcode.vary_string[i];

			if (grcode_flag==0)
				tmpGoods.ItemGroup=grcode;
			else
				tmpGoods.ItemGroup=0;

			tmpGoods.ItemName = "";
			if (name_flag==0)
                for (i=0;i<name.vary_length;i++)
                    tmpGoods.ItemName+=name.vary_string[i];

			if (price_flag==0)
				tmpGoods.ItemPrice=price;
			else
				tmpGoods.ItemPrice=0;

			if (fixedprice_flag==0)
				tmpGoods.ItemFixedPrice=fixedprice;
			else
				tmpGoods.ItemFixedPrice=0;

			Prec = "";
			if (gtype_flag==0)
                for (i=0;i<gtype.vary_length;i++)
                    Prec+=gtype.vary_string[i];


            tmpGoods.ItemPrecision = (Prec=="F")?.001:1;

			tmpGoods.ItemDiscountsDesc = "";
			if (skidka_flag==0)
                for (i=0;i<skidka.vary_length;i++)
                    tmpGoods.ItemDiscountsDesc+=skidka.vary_string[i];

			if (nds_flag==0)
                tmpGoods.NDS=nds;
			else
				tmpGoods.NDS=0;

			if (mult_flag==0)
				tmpGoods.ItemMult=mult;
			else
				tmpGoods.ItemMult=0;

			tmpGoods.Lock = "";
			if (lock_flag==0)
                for (i=0;i<lock.vary_length;i++)
                    tmpGoods.Lock+=lock.vary_string[i];

			if (minprice_flag==0)
				tmpGoods.ItemMinimumPrice=minprice;
			else
				tmpGoods.ItemMinimumPrice=0;

			FoundGoods->Insert(tmpGoods);
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}
#endif

        CashReg->SQLMutex->unlock();
        return FoundGoods->GetCount();

	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
	}
    return 0;
}



bool SQLThread::CheckAction(long long crCustomer,double Summa)
{
	//if(crCustomer==0) return false;

	int Issue=0;
	int Change=0;

    string HeaderStr, ResultrStr;

	try
	{
	    //CashReg->SQLMutex->lock();

		AttachToDB();
		StartTran();
		char sel_str[1024];

        sprintf(sel_str,"select issue from CheckAction (%lld,%8.2f,%s,%d,%d)",
        crCustomer,Summa,CashReg->GetMagNum().c_str(),CashReg->GetCashRegisterNum(), CashReg->GetLastCheckNum());

#ifdef IBASE
		long issue;

		SQL_VARCHAR(1024) line;
		short issue_flag=0;
		long fetch_stat;

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}

		sqlda = new XSQLDA ISC_FAR [XSQLDA_LENGTH(1)];
		sqlda->sqln = 1;
		sqlda->sqld = 1;
		sqlda->version = SQLDAVERSION;

		ibstmt=NULL;

		if (isc_dsql_allocate_statement(status,&ibtable,&ibstmt))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_prepare(status, &ibtran,&ibstmt,0,sel_str,1,sqlda))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		sqlda->sqlvar[0].sqldata = (char *)&issue;
		sqlda->sqlvar[0].sqltype = SQL_LONG + 1;
		sqlda->sqlvar[0].sqlind  = &issue_flag;

		if (isc_dsql_execute(status, &ibtran,&ibstmt, 1, NULL))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		while ((fetch_stat = isc_dsql_fetch(status,&ibstmt,1,sqlda)) == 0)
		{
            if (issue_flag==0)
				Issue+=issue;
		}

		if (fetch_stat!=100)
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		if (isc_dsql_free_statement(status,&ibstmt,DSQL_close))
			throw cash_error(sql_error,0,GetErrorDescription().c_str());

		ibstmt=NULL;

		CommitTran();

		if (sqlda!=NULL)
		{
			delete sqlda;
			sqlda=NULL;
		}


        /*
        HeaderStr += GetFormatString("Касса #"+Int2Str(CashReg->GetCashRegisterNum()),"Чек #"+Int2Str(CashReg->GetLastCheckNum()+1));

        ResultStr = "";
        ResultStr +=GetFormatString("Выдача сертификата ", GetRounded(*ptrSumma,0.01));
        ResultStr +="\nДисконтная карта № " + LLong2Str(crCustomer);
        ResultStr +="\n" + GetFormatString("Накопленная сумма ", " ="+GetRounded(TotalSumma,0.01));
        */

#endif
		//CashReg->SQLMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,false);
		return false;
	}

	if (Issue>0)
	{

        ExclamationForm *dlg = new ExclamationForm(CashReg->CurrentWorkPlace);
        dlg->ExclamationLabel->setText(W2U("ВРУЧИТЬ БИЛЕТЫ\n НА  ФЕСТИВАЛЬ \n\"Свежий воздух\""));
        dlg->AlertLabel->setText(W2U("Поздравляем! Магазин \"Гастроном\" дарит Вам билет\n\
на фестиваль \"Свежий воздух\", который будет\n проходить с 13 по 15 июля в СК \"Чекерил\".\n\
Удачных Вам покупок!"));
        dlg->exec();

        return true;

    }

	return false;
}


void SQLThread::SendBankBonus(void)//lazy write of bank bonus
{
    //;;Log("  === >>  SendBankBonus");
	DbfTable * tbl = CashReg->BankBonusToSendTable;

	try
	{
			CashReg->DbfMutex->lock();
			if (!tbl->GetFirstRecord())
			{
				tbl->Zap();
				CashReg->DbfMutex->unlock();
				return;
			}

			do{
				xbLong SaveRecNo = tbl->GetRecNo();

				PCX_RET PCX_Result;

;;Log((char*)Int2Str(SaveRecNo).c_str());

				string CurDate,CurTime,Saleman_Name;
				int SalemanID = tbl->GetLongField("CASHMAN");
				int CheckNum  = tbl->GetLongField("CHECKNUM");
				if(!CheckNum)
				{
					tbl->DeleteRecord();
					continue;
				}

/*
    IDPODR      INTEGER,
    DATESALE    DATE,
    TIMESALE    DATE,
    KASSANUM    INTEGER,
    CHECKNUM    INTEGER,
    CARD        VARCHAR(30),
    ACTION      INTEGER,
    RETCODE     INTEGER,
    DTSALE      DATE,
    SRVDATE     DATE DEFAULT 'NOW',
    MODE        INTEGER,
    TRAN_ID     VARCHAR(30),
    TRAN_DT     VARCHAR(12),
    TRAN_SUMMA  NUMERIC(15,2),
    SUMMA       NUMERIC(15,2),
    SHA1        VARCHAR(100),
    GUID        VARCHAR(40),
    MSG         VARCHAR(250),
    IDCUST      DOUBLE PRECISION,
    CASHMAN     INTEGER,
    GUARD       INTEGER,
    ID          VARCHAR(31),
    VERSION     INTEGER
    */

				PCX_Result.action		= tbl->GetLongField("ACTION");
				PCX_Result.card     	= tbl->GetStringField("CARD");
				PCX_Result.pwd_card     	= tbl->GetStringField("PAN4");
				PCX_Result.CS_S     	= tbl->GetStringField("CARD_STAT");
				PCX_Result.total_summa	= tbl->GetDoubleField("TRAN_SUMMA");
				PCX_Result.result_summa	= tbl->GetDoubleField("SUMMA");
				PCX_Result.active_bonus	= tbl->GetDoubleField("ACTIVE_BNS");
				PCX_Result.change_bonus	= tbl->GetDoubleField("CHANGE_BNS");
				PCX_Result.sha1	        = tbl->GetStringField("SHA1");
				PCX_Result.id_tran	    = tbl->GetStringField("TRAN_ID");
				PCX_Result.rettran_id   = tbl->GetStringField("RETTRAN_ID");
				PCX_Result.dt_tran      = tbl->GetStringField("TRAN_DT");
				PCX_Result.ReturnCode	= tbl->GetLongField("RETCODE");
				PCX_Result.mode		    = tbl->GetLongField("MODE");
				PCX_Result.info		    = tbl->GetStringField("INFO");
				PCX_Result.guid		    = tbl->GetStringField("GUID");
				PCX_Result.Customer	    = tbl->GetLLongField("CUSTOMER");
				PCX_Result.terminal    	= tbl->GetStringField("TERMINAL");

				PCX_Result.RetCheque   	= tbl->GetLongField("RETCHECK");
                PCX_Result.rettran_id  	= tbl->GetStringField("RETTRAN_ID");
                PCX_Result.LibVersion  	= tbl->GetStringField("LIBVER");

				CurTime			= tbl->GetStringField("TIME");
				CurDate			= xbDate().FormatDate(DateTemplate,tbl->GetField("DATE").c_str());
				CashReg->DbfMutex->unlock();

				//create SQL-statement
				string Insert_str=
				 "INSERT INTO BANKBONUS (IDPODR, DATESALE, TIMESALE, DTSALE, KASSANUM, CHECKNUM,\
CARD, SHA1, ACTION, RETCODE, MODE, TRAN_ID, TRAN_DT,\
TERMINAL, RETCHECK, RETTRAN_ID, CARDSTATUS, PAN4, \
TRAN_SUMMA, SUMMA, SUMMA_ACTIVE, SUMMA_CHANGE,\
MSG, IDCUST, GUID, CASHMAN, GUARD, VERSION, LIBVERSION) VALUES(";
				Insert_str+=CashReg->GetMagNum()+",'";
				Insert_str+=CurDate+"','30.12.1899 ";
				Insert_str+=CurTime+"','";
				Insert_str+=CurDate+" "+CurTime+"',";
				Insert_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
				Insert_str+=Int2Str(CheckNum)+",";

				Insert_str+="'"+PCX_Result.card+"',";
				Insert_str+="'"+PCX_Result.sha1+"',";
				Insert_str+=Int2Str(PCX_Result.action)+",";
				Insert_str+=Int2Str(PCX_Result.ReturnCode)+",";
				Insert_str+=Int2Str(PCX_Result.mode)+",";
				Insert_str+="'"+PCX_Result.id_tran+"',";
				Insert_str+="'"+PCX_Result.dt_tran+"',";

				Insert_str+="'"+PCX_Result.terminal+"',";
				Insert_str+=Int2Str(PCX_Result.RetCheque)+",";
				Insert_str+="'"+PCX_Result.rettran_id+"',";
				Insert_str+="'"+PCX_Result.CS_S+"',";
				Insert_str+="'"+PCX_Result.pwd_card+"',";

				Insert_str+=TruncZeros(Double2Str(PCX_Result.total_summa))+",";
				Insert_str+=TruncZeros(Double2Str(PCX_Result.result_summa))+",";
				Insert_str+=TruncZeros(Double2Str(PCX_Result.active_bonus))+",";
				Insert_str+=TruncZeros(Double2Str(PCX_Result.change_bonus))+",";

                Insert_str+="'"+PCX_Result.info+"',";
                Insert_str+=LLong2Str(PCX_Result.Customer)+",";
                Insert_str+="'"+PCX_Result.guid+"',";

				Insert_str+=Int2Str(SalemanID)+",";
                Insert_str+="0,";
				Insert_str+=Int2Str(revision)+",";
				Insert_str+="'"+PCX_Result.LibVersion+"'";
				Insert_str+=")";

				CashReg->SQLMutex->lock();
;;Log(">>> Insert_str:");
;;Log((char*)Insert_str.c_str());

				AttachToDB();

				StartTran();
#ifdef IBASE
				if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_str.c_str(),1,NULL))
					throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);

				SQLAllocStmt(hdbc,&hstmt);
				if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
					throw cash_error(sql_error,0,GetErrorDescription().c_str());

				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);
				hstmt=SQL_NULL_HSTMT;
#endif

//*
                CommitTran();

				CashReg->SQLMutex->unlock();

				CashReg->DbfMutex->lock();
				tbl->SetRecNo(SaveRecNo);
				tbl->DeleteRecord();
			}
			while(tbl->GetNextRecord());
			tbl->Zap();
			CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,true);
	}
}

void SQLThread::SendBonus(void)//lazy write of bank bonus
{
    //;;Log("  === >>  SendBankBonus");
	DbfTable * tbl = CashReg->BonusToSendTable;

	try
	{
			CashReg->DbfMutex->lock();
			if (!tbl->GetFirstRecord())
			{
				tbl->Zap();
				CashReg->DbfMutex->unlock();
				return;
			}

			do{
				xbLong SaveRecNo = tbl->GetRecNo();

				BNS_RET BNS_Result;

//;;Log((char*)Int2Str(SaveRecNo).c_str());

				string CurDate,CurTime,Saleman_Name;

				long long SCard, DCard;

				int SalemanID = tbl->GetLongField("CASHMAN");
				int CheckNum  = tbl->GetLongField("CHECKNUM");
				if(!CheckNum)
				{
					tbl->DeleteRecord();
					continue;
				}

/*
    IDPODR      INTEGER,
    DATESALE    DATE,
    TIMESALE    DATE,
    KASSANUM    INTEGER,
    CHECKNUM    INTEGER,
    CARD        VARCHAR(30),
    ACTION      INTEGER,
    RETCODE     INTEGER,
    DTSALE      DATE,
    SRVDATE     DATE DEFAULT 'NOW',
    MODE        INTEGER,
    TRAN_ID     VARCHAR(30),
    TRAN_DT     VARCHAR(12),
    TRAN_SUMMA  NUMERIC(15,2),
    SUMMA       NUMERIC(15,2),
    SHA1        VARCHAR(100),
    GUID        VARCHAR(40),
    MSG         VARCHAR(250),
    IDCUST      DOUBLE PRECISION,
    CASHMAN     INTEGER,
    GUARD       INTEGER,
    ID          VARCHAR(31),
    VERSION     INTEGER
    */

				BNS_Result.action		= tbl->GetLongField("ACTION");
				BNS_Result.card     	= tbl->GetStringField("CARD");
				BNS_Result.operation  	= tbl->GetStringField("OPER");
				//BNS_Result.pwd_card   	= tbl->GetStringField("PAN4");
				BNS_Result.status     	= tbl->GetStringField("CARD_STAT");
				BNS_Result.bonus_active	= tbl->GetDoubleField("ACTIVE_BNS");
				BNS_Result.bonus_hold	= tbl->GetDoubleField("HOLD_BNS");
				BNS_Result.summa_add	= tbl->GetDoubleField("SUMMA_ADD");
				BNS_Result.summa_remove	= tbl->GetDoubleField("SUMMA_REM");
				BNS_Result.summa_change	= tbl->GetDoubleField("SUMMA_CHG");
//				BNS_Result.sha1	        = tbl->GetStringField("SHA1");
				BNS_Result.id	        = tbl->GetStringField("TRAN_ID");
//				BNS_Result.dt_tran      = tbl->GetStringField("TRAN_DT");
				BNS_Result.ReturnCode	= tbl->GetLongField("RETCODE");
				BNS_Result.errorCode	= tbl->GetLongField("ERRORCODE");
				BNS_Result.mode		    = tbl->GetLongField("MODE");
				BNS_Result.info		    = tbl->GetStringField("INFO");
				BNS_Result.guid		    = tbl->GetStringField("GUID");
				//BNS_Result.Customer	    = tbl->GetLLongField("CUSTOMER");
				BNS_Result.terminal    	= tbl->GetStringField("TERMINAL");

				BNS_Result.RetCheque   	= tbl->GetLongField("RETCHECK");
                BNS_Result.rettran_id  	= tbl->GetStringField("RETTRAN_ID");
                //BNS_Result.LibVersion  	= tbl->GetStringField("LIBVER");

				SCard   	            = tbl->GetLLongField("OLDCARD");
                DCard  	                = tbl->GetLLongField("NEWCARD");

				CurTime			= tbl->GetStringField("TIME");
				CurDate			= xbDate().FormatDate(DateTemplate,tbl->GetField("DATE").c_str());
				CashReg->DbfMutex->unlock();

				//create SQL-statement
				string Insert_str=
				 "INSERT INTO BONUS (IDPODR, DATESALE, TIMESALE, DTSALE, KASSANUM, CHECKNUM,\
CARD, OPERATION, ACTION, RETCODE, ERRORCODE, TRAN_ID, MODE,\
TERMINAL, RETCHECK, RETTRAN_ID, CARDSTATUS, \
BONUS_ACTIVE, BONUS_HOLD, SUMMA_ADD, SUMMA_REMOVE, SUMMA_CHANGE,\
MSG, IDCUST, GUID, CASHMAN, GUARD, OLDCARD, NEWCARD, VERSION) VALUES(";
				Insert_str+=CashReg->GetMagNum()+",'";
				Insert_str+=CurDate+"','30.12.1899 ";
				Insert_str+=CurTime+"','";
				Insert_str+=CurDate+" "+CurTime+"',";
				Insert_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
				Insert_str+=Int2Str(CheckNum)+",";

				Insert_str+="'"+BNS_Result.card+"',";
				Insert_str+="'"+BNS_Result.operation+"',";
				Insert_str+=Int2Str(BNS_Result.action)+",";
				Insert_str+=Int2Str(BNS_Result.ReturnCode)+",";
				Insert_str+=Int2Str(BNS_Result.errorCode)+",";
				Insert_str+="'"+BNS_Result.id+"',";
				Insert_str+=LLong2Str(BNS_Result.mode)+",";

				Insert_str+="'"+BNS_Result.terminal+"',";
				Insert_str+=Int2Str(BNS_Result.RetCheque)+",";
				Insert_str+="'"+BNS_Result.rettran_id+"',";
				Insert_str+="'"+BNS_Result.CS_S+"',";
				//Insert_str+="'"+BNS_Result.pwd_card+"',";

                Insert_str+=TruncZeros(Double2Str(BNS_Result.bonus_active))+",";
				Insert_str+=TruncZeros(Double2Str(BNS_Result.bonus_hold))+",";
				Insert_str+=TruncZeros(Double2Str(BNS_Result.summa_add))+",";
				Insert_str+=TruncZeros(Double2Str(BNS_Result.summa_remove))+",";
				Insert_str+=TruncZeros(Double2Str(BNS_Result.summa_change))+",";

                Insert_str+="'"+DelAllStr(BNS_Result.info,"'")+"',";
                Insert_str+=LLong2Str(BNS_Result.Customer)+",";
                Insert_str+="'"+BNS_Result.guid+"',";

				Insert_str+=Int2Str(SalemanID)+",";
                Insert_str+="0,";
                Insert_str+=LLong2Str(SCard)+",";
                Insert_str+=LLong2Str(DCard)+",";
				Insert_str+=Int2Str(revision);
				Insert_str+=")";

				CashReg->SQLMutex->lock();
//;;Log(">>> Insert_str:");
//;;Log((char*)Insert_str.c_str());

				AttachToDB();

				StartTran();
#ifdef IBASE
				if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_str.c_str(),1,NULL))
					throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);

				SQLAllocStmt(hdbc,&hstmt);
				if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
					throw cash_error(sql_error,0,GetErrorDescription().c_str());

				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);
				hstmt=SQL_NULL_HSTMT;
#endif

//*
                CommitTran();

				CashReg->SQLMutex->unlock();

				CashReg->DbfMutex->lock();
				tbl->SetRecNo(SaveRecNo);
				tbl->DeleteRecord();
			}
			while(tbl->GetNextRecord());
			tbl->Zap();
			CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,true);
	}
}


void SQLThread::SendActsiz(void)//lazy write of bank bonus
{

    return;




    ;;Log("  === >>  SendActsiz");
	DbfTable * tbl = CashReg->ActsizToSendTable;

	try
	{
			CashReg->DbfMutex->lock();
			if (!tbl->GetFirstRecord())
			{
				tbl->Zap();
				CashReg->DbfMutex->unlock();
				return;
			}

			do{
				xbLong SaveRecNo = tbl->GetRecNo();

;;Log((char*)Int2Str(SaveRecNo).c_str());

				string CurDate,CurTime,Saleman_Name;

				long long SCard, DCard;

				int SalemanID = tbl->GetLongField("CASHMAN");
				int CheckNum  = tbl->GetLongField("CHECKNUM");
				if(!CheckNum)
				{
					tbl->DeleteRecord();
					continue;
				}

/*
    IDPODR      INTEGER,
    DATESALE    DATE,
    TIMESALE    DATE,
    KASSANUM    INTEGER,
    CHECKNUM    INTEGER,
    CARD        VARCHAR(30),
    ACTION      INTEGER,
    RETCODE     INTEGER,
    DTSALE      DATE,
    SRVDATE     DATE DEFAULT 'NOW',
    MODE        INTEGER,
    TRAN_ID     VARCHAR(30),
    TRAN_DT     VARCHAR(12),
    TRAN_SUMMA  NUMERIC(15,2),
    SUMMA       NUMERIC(15,2),
    SHA1        VARCHAR(100),
    GUID        VARCHAR(40),
    MSG         VARCHAR(250),
    IDCUST      DOUBLE PRECISION,
    CASHMAN     INTEGER,
    GUARD       INTEGER,
    ID          VARCHAR(31),
    VERSION     INTEGER
    */

//				BNS_Result.action		= tbl->GetLongField("ACTION");
//				BNS_Result.card     	= tbl->GetStringField("CARD");
//				BNS_Result.operation  	= tbl->GetStringField("OPER");
//				//BNS_Result.pwd_card   	= tbl->GetStringField("PAN4");
//				BNS_Result.status     	= tbl->GetStringField("CARD_STAT");
//				BNS_Result.bonus_active	= tbl->GetDoubleField("ACTIVE_BNS");
//				BNS_Result.bonus_hold	= tbl->GetDoubleField("HOLD_BNS");
//				BNS_Result.summa_add	= tbl->GetDoubleField("SUMMA_ADD");
//				BNS_Result.summa_remove	= tbl->GetDoubleField("SUMMA_REM");
//				BNS_Result.summa_change	= tbl->GetDoubleField("SUMMA_CHG");
////				BNS_Result.sha1	        = tbl->GetStringField("SHA1");
//				BNS_Result.id	        = tbl->GetStringField("TRAN_ID");
////				BNS_Result.dt_tran      = tbl->GetStringField("TRAN_DT");
//				BNS_Result.ReturnCode	= tbl->GetLongField("RETCODE");
//				BNS_Result.errorCode	= tbl->GetLongField("ERRORCODE");
//				BNS_Result.mode		    = tbl->GetLongField("MODE");
//				BNS_Result.info		    = tbl->GetStringField("INFO");
//				BNS_Result.guid		    = tbl->GetStringField("GUID");
//				//BNS_Result.Customer	    = tbl->GetLLongField("CUSTOMER");
//				BNS_Result.terminal    	= tbl->GetStringField("TERMINAL");
//
//				BNS_Result.RetCheque   	= tbl->GetLongField("RETCHECK");
//                BNS_Result.rettran_id  	= tbl->GetStringField("RETTRAN_ID");
//                //BNS_Result.LibVersion  	= tbl->GetStringField("LIBVER");
//
//				SCard   	            = tbl->GetLLongField("OLDCARD");
//                DCard  	                = tbl->GetLLongField("NEWCARD");
//
//				CurTime			= tbl->GetStringField("TIME");
//				CurDate			= xbDate().FormatDate(DateTemplate,tbl->GetField("DATE").c_str());
//				CashReg->DbfMutex->unlock();
//
//				//create SQL-statement
				string Insert_str=
				 "INSERT INTO BONUS (IDPODR, DATESALE, TIMESALE, DTSALE, KASSANUM, CHECKNUM,\
CARD, OPERATION, ACTION, RETCODE, ERRORCODE, TRAN_ID, MODE,\
TERMINAL, RETCHECK, RETTRAN_ID, CARDSTATUS, \
BONUS_ACTIVE, BONUS_HOLD, SUMMA_ADD, SUMMA_REMOVE, SUMMA_CHANGE,\
MSG, IDCUST, GUID, CASHMAN, GUARD, OLDCARD, NEWCARD, VERSION) VALUES(";
//				Insert_str+=CashReg->GetMagNum()+",'";
//				Insert_str+=CurDate+"','30.12.1899 ";
//				Insert_str+=CurTime+"','";
//				Insert_str+=CurDate+" "+CurTime+"',";
//				Insert_str+=Int2Str(CashReg->GetCashRegisterNum())+",";
//				Insert_str+=Int2Str(CheckNum)+",";
//
//				Insert_str+="'"+BNS_Result.card+"',";
//				Insert_str+="'"+BNS_Result.operation+"',";
//				Insert_str+=Int2Str(BNS_Result.action)+",";
//				Insert_str+=Int2Str(BNS_Result.ReturnCode)+",";
//				Insert_str+=Int2Str(BNS_Result.errorCode)+",";
//				Insert_str+="'"+BNS_Result.id+"',";
//				Insert_str+=LLong2Str(BNS_Result.mode)+",";
//
//				Insert_str+="'"+BNS_Result.terminal+"',";
//				Insert_str+=Int2Str(BNS_Result.RetCheque)+",";
//				Insert_str+="'"+BNS_Result.rettran_id+"',";
//				Insert_str+="'"+BNS_Result.CS_S+"',";
//				//Insert_str+="'"+BNS_Result.pwd_card+"',";
//
//                Insert_str+=TruncZeros(Double2Str(BNS_Result.bonus_active))+",";
//				Insert_str+=TruncZeros(Double2Str(BNS_Result.bonus_hold))+",";
//				Insert_str+=TruncZeros(Double2Str(BNS_Result.summa_add))+",";
//				Insert_str+=TruncZeros(Double2Str(BNS_Result.summa_remove))+",";
//				Insert_str+=TruncZeros(Double2Str(BNS_Result.summa_change))+",";
//
//                Insert_str+="'"+DelAllStr(BNS_Result.info,"'")+"',";
//                Insert_str+=LLong2Str(BNS_Result.Customer)+",";
//                Insert_str+="'"+BNS_Result.guid+"',";
//
//				Insert_str+=Int2Str(SalemanID)+",";
//                Insert_str+="0,";
//                Insert_str+=LLong2Str(SCard)+",";
//                Insert_str+=LLong2Str(DCard)+",";
//				Insert_str+=Int2Str(revision);
//				Insert_str+=")";

				CashReg->SQLMutex->lock();
//;;Log(">>> Insert_str:");
//;;Log((char*)Insert_str.c_str());

				AttachToDB();

				StartTran();
#ifdef IBASE
				if (isc_dsql_execute_immediate(status,&ibtable,&ibtran,0,(char*)Insert_str.c_str(),1,NULL))
					throw cash_error(sql_error,0,GetErrorDescription().c_str());
#else
				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);

				SQLAllocStmt(hdbc,&hstmt);
				if (SQLExecDirect(hstmt,(unsigned char*)Insert_str.c_str(),SQL_NTS)!=SQL_SUCCESS)
					throw cash_error(sql_error,0,GetErrorDescription().c_str());

				if (hstmt!=SQL_NULL_HSTMT)
					SQLFreeStmt(hstmt,SQL_DROP);
				hstmt=SQL_NULL_HSTMT;
#endif
//*
                CommitTran();

				CashReg->SQLMutex->unlock();

				CashReg->DbfMutex->lock();
				tbl->SetRecNo(SaveRecNo);
				tbl->DeleteRecord();
			}
			while(tbl->GetNextRecord());
			tbl->Zap();
			CashReg->DbfMutex->unlock();
	}
	catch(cash_error& ex)
	{
		ProcessError(ex,true);
	}
}
