#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <openssl/sha.h>

#ifdef _WIN32
#pragma warning(disable: 4996)
#endif

//#include "linpcx.h"

#include <Bonus.h>
//#include "Fiscal.h"

//#include "Utils.h"

extern "C"
{
#include <uuid/uuid.h>
}

#define COMMAND_GET_TOKEN "/api/auth/gastronom_admin"
#define COMMAND_GET_CARD_INFO "/api/card/"
#define COMMAND_ADD_CARD "/api/card/"
#define COMMAND_PAYMENT "/api/purchase/"
#define COMMAND_RETURN "/api/purchasereturn/"

#define COMMAND_CHANGE_CARD "/api/gastronom/card/"


#define HTTP_AUTH_ERROR 401
#define BNS_BAD_TOKEN -6
#define BNS_ERROR_TOKEN -3
#define BNS_ERROR_NOTFOUND 25

#define ERROR_NOT_FOUND "<<ERROR_NOT_FOUND>>"

static char * bns_config = (char*)"/Kacca/bonus/bonus.cfg";
static char * bns_local = (char*)"/Kacca/bonus/local.cfg";
static string logBonusFileName="/Kacca/bonus/bonus.log";
static string tokenFileName="/Kacca/bonus/token.res";

inline void fgets(char* buf, FILE * f)
{
	char * nl;
	fgets(buf, 1024, f);
	nl = strrchr(buf, 0x0d);//'\n');
	if(nl) *nl = '\0';
	nl = strrchr(buf, 0x0a);//'\n');
	if(nl) *nl = '\0';
}

//string FmtAmount(const char* szAmount) {
//	char szResult[0x20];
//	long long amount = 0;
//
//	sscanf(szAmount, "%lld", &amount);
//	sprintf(szResult, "%.02f", (double)amount / 100.0);
//	return szResult;
//}

// Конвертация из utf8 в win1251
string TBonus::convert(iconv_t cd, string str_in )
{
    const char* in = (const char*)str_in.c_str();
    const char* res_in = in;

    size_t l_in, l_out, k;
    l_in = strlen(in);

    char t_out[l_in];
    const char* res_out = t_out;

    l_out = sizeof t_out;

    memset( &t_out, 0, l_out );

    if( cd != (iconv_t)(-1) )
    {
        k = iconv(cd, (char**)&res_in, &l_in, (char**)&res_out, &l_out);
    }

    string res = t_out;
    return res;  // присваиваем указатель
}

int TBonus::Log(string logstr, bool endline)
{
    return Log((char*)logstr.c_str());
}


int TBonus::Log(const char* logstr, bool endline)
{
	FILE * log = fopen((char*)logBonusFileName.c_str(),"a");

	if(!log) return 1;

    if (endline)
    {
        time_t tt = time(NULL);
        tm* t = localtime(&tt);
        fprintf(log, "%02d.%02d.%4d %02d:%02d:%02d> %s",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec,
            logstr);
        fprintf(log, "\n");
    }else
    {
        fprintf(log, " %s", logstr);
    }
	fclose(log);
	return 0;
}



void TBonus::SetSettings()
{

;;Log("= Чтение настроек BONUS =");
	FILE * f = fopen( bns_config, "r");
	if(f)
	{
		char buf[1024];

        //ConnectionString=
		fgets(buf, f);
		ConnectionString = buf;
		ConnectionString = ConnectionString.substr(17);

;;Log(ConnectionString.c_str());

//CertFilePath=
		fgets(buf, f);
		CertFilePath = buf;
		CertFilePath = CertFilePath.substr(13);
;;Log(CertFilePath.c_str());

//KeyFilePath
		fgets(buf, f);
		KeyFilePath = buf;
		KeyFilePath = KeyFilePath.substr(12);

;;Log(KeyFilePath.c_str());
//KeyPassword
		fgets(buf, f);
		KeyPassword = buf;
		KeyPassword = KeyPassword.substr(12);
;;Log(KeyPassword.c_str());

//OrgID
		fgets(buf, f);
		OrgID = buf;
		OrgID = OrgID.substr(6);
;;Log(OrgID.c_str());


//Terminal
		fgets(buf, f);
		Terminal = buf;
		Terminal = Terminal.substr(9);

;;Log(Terminal.c_str());

//Location
		fgets(buf, f);
		Location = buf;
		Location = Location.substr(9);
;;Log(Location.c_str());

//Password
		fgets(buf, f);
		Password = buf;
		Password = Password.substr(9);
;;Log(Password.c_str());

//		fscanf(f, "PartnerID=%d\n", &PartnerID);
//
//		fscanf(f, "ConnectTimeout=%d\n", &ConnectTimeout);
//		fscanf(f, "SendRecvTimeout=%d\n", &SendRecvTimeout);
//		fscanf(f, "BkgndFlushPeriod=%d\n", &BkgndFlushPeriod);
//		fscanf(f, "FlushTimeout=%d\n", &FlushTimeout);
		fscanf(f, "UseBonus=%d\n", &UseBNS);
		fscanf(f, "UseDiscount=%d\n", &UseDiscount);
		fscanf(f, "UsePay=%d\n", &UsePay);
		fscanf(f, "Insecure=%d\n", &Insecure);

//Options
		Options = "";
		if (Insecure) {
            Options = "--insecure";
		}
        ;;Log("Options="+Options);

		fclose(f);
	}

}

bool TBonus::Init()
{

    ApiVersion = "3";


//#ifdef LINUX_BUILD
//	setlocale( LC_ALL, "ru_RU->utf8");
//#endif
//
//    cd = iconv_open("cp1251", "utf8");

    string Version = "Bonus 1.00 testing";

    /////////////////////////////////


    Log("\n\n");
    Log("=========================  INIT BONUS  ===============================");

    string LogStr="";
    LogStr = "------------------------- "+Version+" ----------------------------";
    Log(LogStr.c_str());

    bool res=true;

    SetSettings();

    BNS_Token = new BNS_RET_TOKEN;
    BNS_Token->init();

    BNS_CardInfo = new BNS_RET_CARD_INFO;
    BNS_CardInfo->init();

//    int err=GetToken();
//    if (!err)
//        Log((char*)("GET TOKEN: '"+BNS_Token->token+"'").c_str());
//    else
//        Log((char*)("ERROR: Error get token: "+Int2Str(err)).c_str());

//    if( cd != (iconv_t)(-1) )
//    {
//        iconv_close(cd);
//    }

    return res;
}


//#endif



const char* TBonus::PrintFileName()
{
	static char p[] = "/Kacca/bonus/p";
	return p;
}


TBonus::TBonus()
{
//    e_file = "/Kacca/bonus/e";
//    p_file = "/Kacca/bonus/p";
//
//    LineWidth =33;
//
//    a_card      ="";
//    pwd_card    ="";
//    a_sum       ="";
//    a_chsum     ="";
//
//    a_check     ="";
//    a_track2    ="";
//
//    a_sha1      ="";
//    a_tran      ="";
//    a_dttran    ="";
//
//    full_sum=disc_sum=bank_sum=0;
//
//    PCX=NULL;

}

TBonus::~TBonus()
{

    //Destroy();

}



int TBonus::ErrLog(int err,const char* desc, bool onlydesc)
{
    FILE * fe = fopen(e_file, "a");
    if(!fe) return 1;
    if (onlydesc)
        fprintf(fe, "%s\n", desc);
    else
        fprintf(fe, "%d %s\n", err, desc);

    fclose(fe);

    return 0;
}


// фукция форматирования чека
const char * TBonus::FormatCheck(const char * CheckStr, bool WithCopy)
{

    string NewCheckStr, NewCheckStrCopy;

    string str = CheckStr;

    string first, second, CurrentString; // слова, пока обрабатываем только два

    CurrentString = "";
    first  = "";
    second = "";
    NewCheckStr = "";
    NewCheckStrCopy = "";

    char s;

    for(unsigned int i=0; i<str.length(); i++)
    {

        s = str[i];

        if (s=='\n')
        {
            //first = LineSeparator(first, LineWidth);

            CurrentString="";

            CurrentString+=GetCenterString(first);

            CurrentString+="\n";

            NewCheckStr+=CurrentString;
            NewCheckStrCopy+=CurrentString;

            first  = "";
            second = "";
        }
        else
        {
            first+=s;
        }
    }

    NewCheckStr+="\n";

    if (WithCopy) NewCheckStr+="\n\n\n\n"+NewCheckStrCopy;

    const char* Result = (const char*)NewCheckStr.c_str();

    return Result;

}

// возвращает хэш по номеру карты
string TBonus::GetSHA1(string card)
{

    unsigned char md[20];

	//unsigned char* ClientIDstr =
	SHA1( (unsigned char *)card.c_str(),	strlen((const char*)card.c_str()),	md); // хэш SHA1

    string result = "";
    char tmp[1];

    for (int i=0;i<20;i++)
    {
        sprintf(tmp,"%.2x",md[i]);
        result += tmp;
    }

    // заглушка
    //;;result = "C20BBF80524A22A480BC171F536EF1507F53E757";

    return result;
}


string TBonus::SetPwdNumber(string number, unsigned int len)
{
    string str_asteriks = "****************************";
    string result = "";
    unsigned int maxnum=16;

    for (unsigned i=0;i<min(number.length(), len);i++)
        if (number[i]!='"') result+=number[i];

    result += (maxnum>result.length())?str_asteriks.substr(0, maxnum-result.length()):"";

    return result;
}

string TBonus::GetPwdCardNumber(string number)
{
    string str_asteriks = "******************************************";
    string result = "";

    int len = number.length();
    if (len>4)
        result = str_asteriks.substr(0, len-4) + number.substr(len-4, 4);

    return result;
}


int TBonus::run( string cmd)
{
    if(ConnectionString=="")
    {
        Log("Ошибка! Не задан IP-адрес ПЦ Бонусы!");
        return -2;
    }

	//qApp->setOverrideCursor(QApplication::waitCursor);

//	QSemiModal sm(NULL,NULL,true,Qt::WStyle_NoBorderEx);
//	QLabel msg(&sm);//, NULL, Qt::WType_TopLevel);
//	sm.setFixedSize(250,100);
//	msg.setText(W2U("Работа с ПЦ Бонусы Гастроном..."));
//	msg.setFixedSize(250,100);
//	msg.setAlignment( QLabel::AlignCenter );
//	sm.show();
//	thisApp->processEvents();

    Log(" ");
    Log("Command: " + cmd);
//    Log((char*)cmd.c_str());

//    clock_t start, end;
    struct timeval start, end;
	//! start !//

    gettimeofday(&start, NULL);
    int ret=system((char*)cmd.c_str());
    gettimeofday(&end, NULL);

	chdir("/Kacca");

    double duration_sec = ((end.tv_sec * 1000000 + end.tv_usec) -
                           (start.tv_sec * 1000000 + start.tv_usec))/1000000.0;
    Log("BonusCmdDuration: " + Double2Str(duration_sec) + " sec.");

//    if (duration_sec > 1.0) {
//        Log("BonusCmdDuration: " + Double2Str(duration_sec) + " sec.");
//    }

    return ret;
}

void TBonus::ParseResult(BNS_RET_TOKEN* BNS_Token)
{

    BNS_Token->init();

	FILE* f;
	char * fn = (char*)tokenFileName.c_str();
	f = fopen(fn,"r");
	if(!f)
	{
	    Log(" -> Ошибка открытия файла! Файл не найден!");
	    return ;
	}

	//int e;
	//fscanf(f,"%d ",&e);
	char sMsg[2048];
	fgets(sMsg, 2048, f);

    fclose(f);

    string str = sMsg;
    string str_log = str;

    // Ответ ПЦ
    Log(" ");
    Log("Результат:");
    Log((char*)convert(cd, str_log).c_str());

    string HTTPCode = GetParam(str, "HTTPResponseCode");
    BNS_Token->HTTPResponseCode = Str2Int(HTTPCode);

    string errorCode = GetParam(str, "errorCode");
    BNS_Token->errorCode = Str2Int(errorCode);
    BNS_Token->errorDescription = convert(cd, GetParam(str, "errorDescription"));
    BNS_Token->result = GetParam(str, "result");
    BNS_Token->token = GetParam(str, "token");

    // Лог
    Log((char*)("HTTP Response Code: "+Int2Str(BNS_Token->HTTPResponseCode)).c_str());
    Log((char*)("Код возврата: "+Int2Str(BNS_Token->errorCode)).c_str());
    Log((char*)("Ответ: "+BNS_Token->errorDescription).c_str());
    Log((char*)("result: "+BNS_Token->result).c_str());
    Log((char*)("token: "+BNS_Token->token).c_str());
}

void TBonus::ParseResult(BNS_RET_CARD_INFO* BNS_CardInfo)
{

    Log(" ");
    Log("Результат:");

	FILE* f;
	char * fn = (char*)tokenFileName.c_str();
	f = fopen(fn,"r");
	if(!f)
	{
	    Log(" -> Ошибка открытия файла! Файл не найден!");
	    return ;
	}


	char *estr;
	char sMsg[1024];
    string str ="";

	while(!feof(f)){
      estr = fgets(sMsg, 1024, f);
      str = str + sMsg;
	}

    fclose(f);

    string str_log = str;

    // Ответ ПЦ
    Log((char*)convert(cd, str_log).c_str());

    BNS_CardInfo->init();

    string HTTPCode = GetParam(str, "HTTPResponseCode");
    BNS_CardInfo->HTTPResponseCode = Str2Int(HTTPCode);

    string errorCode = GetParam(str, "errorCode");
    BNS_CardInfo->errorCode = Str2Int(errorCode);
    BNS_CardInfo->errorDescription = convert(cd, GetParam(str, "errorDescription"));
    BNS_CardInfo->ActiveBonuses = Str2Double(GetParam(str, "ActiveBonuses"));
    BNS_CardInfo->HoldedBonuses = Str2Double(GetParam(str, "HoldedBonuses"));

    BNS_CardInfo->status = GetParam(str, "CardStatus");


    // Лог
    Log((char*)("HTTP Response Code: "+Int2Str(BNS_CardInfo->HTTPResponseCode)).c_str());
    Log((char*)("Код возврата: "+Int2Str(BNS_CardInfo->errorCode)).c_str());
    Log((char*)("Ответ: "+BNS_CardInfo->errorDescription).c_str());

    Log((char*)("Active bonuses: "+Double2Str(BNS_CardInfo->ActiveBonuses)).c_str());
    Log((char*)("Holded bonuses: "+Double2Str(BNS_CardInfo->HoldedBonuses)).c_str());



// BNS_Result
    BNS_Result.errorCode = BNS_CardInfo->errorCode;
    BNS_Result.ReturnCode = (BNS_CardInfo->HTTPResponseCode!=200)?BNS_CardInfo->HTTPResponseCode:BNS_CardInfo->errorCode;
    BNS_Result.errorDescription = BNS_CardInfo->errorDescription;
    BNS_Result.ActiveBonuses = BNS_CardInfo->ActiveBonuses;
    BNS_Result.HoldedBonuses = BNS_CardInfo->HoldedBonuses;

    BNS_Result.bonus_active = BNS_CardInfo->ActiveBonuses;
    BNS_Result.bonus_hold = BNS_CardInfo->HoldedBonuses;

    BNS_Result.info = BNS_CardInfo->errorDescription;
    BNS_Result.status = BNS_CardInfo->status;

}

void TBonus::ParseResult(BNS_RET_PURCHASE* BNS_Pay)
{

    Log(" ");
    Log("Результат:");

	FILE* f;
	char * fn = (char*)tokenFileName.c_str();
	f = fopen(fn,"r");
	if(!f)
	{
	    Log(" -> Ошибка открытия файла! Файл не найден!");
	    return ;
	}



	char *estr;

	char sMsg[1024];

    string str ="";

	while(!feof(f)){
      estr = fgets(sMsg, 1024, f);
      str = str + sMsg;
	}

    fclose(f);

    string str_log = str;

    // Ответ ПЦ
    Log(convert(cd, str_log));

    BNS_Pay->init();

    string HTTPCode = GetParam(str, "HTTPResponseCode");
    BNS_Pay->HTTPResponseCode = Str2Int(HTTPCode);

    string errorCode = GetParam(str, "errorCode");
    BNS_Pay->errorCode = Str2Int(errorCode);
    BNS_Pay->errorDescription = convert(cd, GetParam(str, "errorDescription"));

    BNS_Pay->id = GetParam(str, "id");

    BNS_Pay->maxSumCanPay = Str2Double(GetParam(str, "maxCheckSumCanPayBonuses", "0"));

    Log("HTTP Response Code: "+Int2Str(BNS_Pay->HTTPResponseCode));
    Log("Код возврата: "+Int2Str(BNS_Pay->errorCode));
    Log("Ответ: "+BNS_Pay->errorDescription);

    Log("ID покупки: "+BNS_Pay->id);
    Log("MaxSumCanPay: "+Double2Str(BNS_Pay->maxSumCanPay));


// BNS_Result

    BNS_Result.errorCode = BNS_Pay->errorCode;
    BNS_Result.ReturnCode = (BNS_Pay->HTTPResponseCode!=200)?BNS_Pay->HTTPResponseCode:BNS_Pay->errorCode;
    BNS_Result.errorDescription = BNS_Pay->errorDescription;

    BNS_Result.info = BNS_Pay->errorDescription;

    BNS_Result.id = BNS_Pay->id;
    BNS_Result.maxSumCanPay = BNS_Pay->maxSumCanPay;

    BNS_Result.summa_add = Str2Double(GetParam(str, "bonusesAdded", "0"));
    BNS_Result.summa_remove = Str2Double(GetParam(str, "bonusesRemoved", "0"));
    BNS_Result.summa_change = Str2Double(GetParam(str, "bonusesCharged", "0"));

}

void TBonus::ParseResult(BNS_RET_CARD_INFO* BNS_CardInfo, bool byPhone)
{

    Log(" ");
    Log("Результат:");

	FILE* f;
	char * fn = (char*)tokenFileName.c_str();
	f = fopen(fn,"r");
	if(!f)
	{
	    Log(" -> Ошибка открытия файла! Файл не найден!");
	    return ;
	}

	char *estr;
	char sMsg[1024];
    string str ="";

	while(!feof(f)){
      estr = fgets(sMsg, 1024, f);
      str = str + sMsg;
	}

    fclose(f);

    string str_log = str;

    // Ответ ПЦ
    Log((char*)convert(cd, str_log).c_str());
    BNS_CardInfo->init();
    string HTTPCode = GetParam(str, "HTTPResponseCode");
    BNS_CardInfo->HTTPResponseCode = Str2Int(HTTPCode);
    string errorCode = GetParam(str, "errorCode");
    BNS_CardInfo->errorCode = Str2Int(errorCode);
    BNS_CardInfo->errorDescription = convert(cd, GetParam(str, "errorDescription"));
    BNS_CardInfo->ActiveBonuses = Str2Double(GetParam(str, "ActiveBonuses"));
    BNS_CardInfo->HoldedBonuses = Str2Double(GetParam(str, "HoldedBonuses"));

    string result = GetBlock(str, "result");
    //;;Log("result: "+result);

    string name = convert(cd, GetParam(result, "FullName"));
    string mobil = GetParam(result, "MobileNumber");
    //;;Log("MobileNumber: "+mobil);
    //;;Log("FullName: "+name);

    vector<string> ListMass;

    GetMassive(result, "Cards", &ListMass);

// Обработка массива карт

    string cur_card, cur_type, cur_status, cur_str;
    for (int j=0;j<ListMass.size();j++)
    {
        string tmp = "Card #"+Int2Str(j+1);
        tmp+=":";
        tmp+=ListMass[j];

        ;;Log((char*)tmp.c_str());

         cur_str = ListMass[j];

         cur_status = GetParam(cur_str, "status");
Log("Статус: "+cur_status);

         if (cur_status!="255")
         {
            cur_type = GetParam(cur_str, "isGeneral");

            cur_card = GetParam(cur_str, "number");
            BNS_Result.card = cur_card;
            BNS_Result.Customer = atoll(cur_card.c_str());
            BNS_Result.status = cur_status;


Log("Признак: "+cur_type);
Log("Карта: "+cur_card);


            if(cur_type=="true")  break; // нашли основную карту

         }

    }




    // Лог
    Log("HTTP Response Code: "+Int2Str(BNS_CardInfo->HTTPResponseCode));
    Log("Код возврата: "+Int2Str(BNS_CardInfo->errorCode));
    Log("Ответ: "+BNS_CardInfo->errorDescription);

    Log("Основная карта: "+BNS_Result.card);
    Log("Статус: "+BNS_Result.status);



// BNS_Result
    BNS_Result.errorCode = BNS_CardInfo->errorCode;
    BNS_Result.ReturnCode = (BNS_CardInfo->HTTPResponseCode!=200)?BNS_CardInfo->HTTPResponseCode:BNS_CardInfo->errorCode;
    BNS_Result.errorDescription = BNS_CardInfo->errorDescription;
    BNS_Result.ActiveBonuses = BNS_CardInfo->ActiveBonuses;
    BNS_Result.HoldedBonuses = BNS_CardInfo->HoldedBonuses;

    BNS_Result.bonus_active = BNS_CardInfo->ActiveBonuses;
    BNS_Result.bonus_hold = BNS_CardInfo->HoldedBonuses;

    BNS_Result.info = BNS_CardInfo->errorDescription;
    BNS_Result.action = 1;
    BNS_Result.customerName = name;
    BNS_Result.mobileNumber = mobil;


}

//  читает параметр-массив из строки JSON ( возвращает текст между [ ] )
// результат возвращается в виде массива  строк
// str - строка в формате JSON
// param - имя параметра-массива
void TBonus::GetMassive(string str, string param, vector<string> *ListMass)
{
    ListMass->clear();

    int begpos = str.find("\""+param+"\":[{");
    if (begpos!=string::npos){
        string tmpstr = str.substr(begpos+param.length()+5, string::npos);

        int cnt_mass=1;
        int cnt=1;
        int beg_block=0;
        int end_block=0;

        for(int i=0; i<tmpstr.length(); i++)
        {
            if (tmpstr[i]=='[') cnt_mass++;
            if (tmpstr[i]==']') cnt_mass--;

            if (!cnt_mass) break;

            if (tmpstr[i]=='{') cnt++;
            if (tmpstr[i]=='}') cnt--;

            if (!cnt && tmpstr[i]!=','){

               end_block = i;
               string block = tmpstr.substr(beg_block, end_block-beg_block);
               ListMass->insert(ListMass->end(), block);

               beg_block = end_block+3;

            }
        }

    }

}


//  читает параметр-структуру из строки JSON ( возвращает текст между { } )
// результат возвращается в виде строки
// str - строка в формате JSON
// param - имя параметра-структуры
string TBonus::GetBlock(string str, string param)
{
    string res = "";

    int begpos = str.find("\""+param+"\":{");
    if (begpos!=string::npos){
        string tmpstr = str.substr(begpos+param.length()+4, string::npos);
        int cnt=1;
        int len = tmpstr.length();

        for(int i=0; i<len; i++)
        {
            if (tmpstr[i]=='{') cnt++;
            if (tmpstr[i]=='}') cnt--;

            if (!cnt || (i==len-1)){
               res = tmpstr.substr(0, i);
               break;
            }
        }

    }

    return res;
}

//  читает первый попавшийся параметр из строки JSON
// результат возвращается в виде строки
// str - строка в формате JSON
// param - имя параметра
// def - значение по умолчанию
string TBonus::GetParam(string str, string param, string def)
{
    string res = def;

    int begpos = str.find("\""+param+"\":");
    if (begpos!=string::npos){
        string tmpstr = str.substr(begpos+param.length()+3, string::npos);
        int lastpos = tmpstr.find(",");

        if (lastpos==string::npos) lastpos = tmpstr.find("}");
        if (lastpos!=string::npos)
        {
            res = tmpstr.substr(0, lastpos);

            // вариант {...,result:{"id":"xxxx-xxxx-xxxx-xxxx-xxxx"}, ...}
            lastpos = res.find("}");
            if (lastpos!=string::npos)
                res = res.substr(0, lastpos);
        }
        else
            res = tmpstr;

    }

    return res;
}


int TBonus::SetTimeoutError()
{
    Log("= SetTimeoutError = START =");

    BNS_Result.errorCode = -999;
    BNS_Result.ReturnCode = -999;

    Log("= SetTimeoutError = END =");

    return 1;
}

int TBonus::GetToken()
{
    Log("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  GET TOKEN  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
    Log("Operation: TOKEN");

    int res = -1;

    token = "";

    string cmd = COMMAND_GET_TOKEN;
    cmd = " /Kacca/bonus/bonus -X GET \"" + ConnectionString + cmd+ "?password="+Password+"&tillAuth="+Terminal + "\"";

    cmd = cmd + " --connect-timeout 5";
    cmd = cmd + " --max-time 15";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " > "+tokenFileName;


    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    ParseResult(BNS_Token);

    if (!BNS_Token->errorCode)
    {
        res = 0;
        token = BNS_Token->token;
    }
    else
    {
        res = 2;
        Log((char*)("Ошибка получения token: "+BNS_Token->errorDescription).c_str());
        token="";
    }

    Log("///////////////////////////////////////////////////////////////////////////");
    return res;
}


int TBonus::ChangeCard(string card, string newcard)
{

    Log("Operation: CHANGE CARD");
    Log((char*)("Card: "+card).c_str());

    int res = -1;


    if (token=="")
    {
        if(GetToken()) return res;
    }

    // + dms 2019-10-29 Новая команда замены
    //string cmd = COMMAND_ADD_CARD;
    string cmd = COMMAND_CHANGE_CARD;
    // - dms 2019-10-29 Новая команда замены

    cmd = " /Kacca/bonus/bonus -X PUT -d @- \"" + ConnectionString + cmd+"\\"+card+"\\replace\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 20";
    cmd = cmd + " --max-time 60";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;

    string body = "echo '{\"cardNumberNew\":"+newcard+
    // + dms 2019-10-29 Новая команда замены
    //", \"cardBlockReasonIdentifier\":\""+"10"+"\""+
    ", \"cardBlockReasonIdentifier\":\""+"1"+"\""+  // Новая команда замены
    // - dms 2019-10-29 Новая команда замены

    //",\"mobileNumber\":\"+"+card.substr(5,6)+"\""+
    "}'";

    cmd = body + " | " + cmd;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    ParseResult(BNS_CardInfo);

    BNS_Result.action = 5; // замена карты


    if (BNS_CardInfo->errorCode)
    {
        Log((char*)("Ошибка замены карты: "+BNS_CardInfo->errorDescription).c_str());

        if((BNS_CardInfo->HTTPResponseCode == HTTP_AUTH_ERROR)
        || (BNS_CardInfo->errorCode == BNS_BAD_TOKEN)
        || (BNS_CardInfo->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = ChangeCard(card, newcard); // повтор команды
        }
    }


    return res;
}


int TBonus::AddCard(string card)
{

    Log("Operation: ADD CARD");
    Log((char*)("Card: "+card).c_str());

    int res = -1;


    if (token=="")
    {
        if(GetToken()) return res;
    }

    string cmd = COMMAND_ADD_CARD;
    cmd = " /Kacca/bonus/bonus -X POST -d @- \"" + ConnectionString + cmd+ "\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 20";
    cmd = cmd + " --max-time 60";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;

    string body = "echo '{\"cardNumber\":"+card+
    ", \"fullName\":\""+card+"\""+
    //",\"mobileNumber\":\"+"+card.substr(5,6)+"\""+
    "}'";

    cmd = body + " | " + cmd;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    ParseResult(BNS_CardInfo);

    BNS_Result.action = 2; // add card


    if (BNS_CardInfo->errorCode)
    {
        Log((char*)("Ошибка регистрации карты: "+BNS_CardInfo->errorDescription).c_str());

        if((BNS_CardInfo->HTTPResponseCode == HTTP_AUTH_ERROR)
        || (BNS_CardInfo->errorCode == BNS_BAD_TOKEN)
        || (BNS_CardInfo->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = AddCard(card); // повтор команды
        }
    }


    return res;
}

int TBonus::GetCardInfo(string card)
{

    int res = -1;

    Log("Operation: GET CARD INFO");
    Log((char*)("Card: "+card).c_str());

    if (token=="")
    {
        if(GetToken()) return res;
    }

    string cmd = COMMAND_GET_CARD_INFO;
    cmd = " /Kacca/bonus/bonus -X GET \"" + ConnectionString + cmd+ card +"\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

//    cmd = cmd + " --connect-timeout 10";
//    cmd = cmd + " --max-time 15";

    cmd = cmd + " --connect-timeout 5";
    cmd = cmd + " --max-time 10";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    ParseResult(BNS_CardInfo);

    BNS_Result.action = 1; // get card info

    if (BNS_CardInfo->errorCode)
    {
        Log((char*)("Ошибка получения информации по карте: "+BNS_CardInfo->errorDescription).c_str());

        if((BNS_CardInfo->HTTPResponseCode == HTTP_AUTH_ERROR)
        ||(BNS_CardInfo->errorCode == BNS_BAD_TOKEN)
        ||(BNS_CardInfo->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = GetCardInfo(card);
        }
    }

    return res;

}

int TBonus::GetCustomerInfo(string card, bool byPhone)
{

    int res = -1;

    if (byPhone)
    {
        Log("Operation: GET CUSTOMER INFO BY PHONE");
        Log((char*)("Phone: "+card).c_str());
    }
    else
    {
        Log("Operation: GET CUSTOMER INFO BY CARD");
        Log((char*)("Card: "+card).c_str());
    }


    if (token=="")
    {
        if(GetToken()) return res;
    }

    string cmd = "";
    if (byPhone)
    {
        cmd = "/api/customer/byphone?search=%2B";
    }
    else
    {
        cmd = "/api/customer/bycard?search=%2B";
    }
    cmd = " /Kacca/bonus/bonus -X GET \"" + ConnectionString + cmd+ card +"\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 7";
    cmd = cmd + " --max-time 15";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    ParseResult(BNS_CardInfo, true);

    if (BNS_CardInfo->errorCode)
    {
        Log((char*)("Ошибка получения информации по карте: "+BNS_CardInfo->errorDescription).c_str());

        if((BNS_CardInfo->HTTPResponseCode == HTTP_AUTH_ERROR)
        ||(BNS_CardInfo->errorCode == BNS_BAD_TOKEN)
        ||(BNS_CardInfo->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = GetCardInfo(card);
        }
    }

    return res;

}


int TBonus::Pay(string card, FiscalCheque* CurCheque, double p_sum)
{
    int res = -1;

    Log("Operation: SALE INIT (Step #1)");
    Log((char*)("Card: "+card).c_str());

    if (token=="")
    {
        if(GetToken()) return res;
    }

    string cmd = COMMAND_PAYMENT;
    cmd = " /Kacca/bonus/bonus -X POST -d @- \"" + ConnectionString + cmd+ "\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 10";
    cmd = cmd + " --max-time 30";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;


    string position = "[";

    int num=0;
	for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo crGoods = (*CurCheque)[i];

        if (((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT)) || (crGoods.ItemCode==TRUNC)) continue;

        // добавляем информацию по позициям чека в запрос
        if (num>0) position += ",";

        position = position + "{" +
        "\"lineNumber\":"+ Int2Str(++num) +
        ", \"productCode\":\""+Int2Str(crGoods.ItemCode)+"\""+
        ", \"groupCode\":\""+""+"\""+
        ", \"barCode\":\""+crGoods.ItemBarcode+"\""+
        ", \"quantity\":"+ Double2Str(crGoods.ItemCount) +
        ", \"price\":"+ Double2Str(crGoods.ItemPrice) +
        ", \"summ\":"+ Double2Str(crGoods.ItemSum) +
        ", \"summWithoutDiscount\":"+ Double2Str(crGoods.ItemCount * crGoods.ItemFullPrice) +
        //", \"customProperties\":[{\"SpecialGroup\":\"VIP\"}]"+
        ", \"customProperties\":null"+
        "}";

    }

    //

	time_t tt = time(NULL);
	tm* t = localtime(&tt);
    char dt[256];
    sprintf(dt, "%4d-%02d-%02dT%02d:%02d:%02d",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    string dtstr = dt;

    position = position + "]";

    string body = "echo '{\"cardNumber\":"+card+
    ", \"tillAuth\":\""+Terminal+"\""+
    ", \"date\":\""+dtstr+"\""+
    ", \"summ\":"+Double2Str(CurCheque->GetSum())+
    ", \"summWithoutDiscount\":"+Double2Str(CurCheque->GetFullSum())+
    ", \"positions\":"+position+
    //", \"customProperties\":[{\"Type\":\"Order\"}]"+
    ", \"customProperties\":null"+
    "}'";

    cmd = body + " | " + cmd;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    BNS_Pay = new BNS_RET_PURCHASE;
    BNS_Pay->init();

    // Обработка результата
    ParseResult(BNS_Pay);

    BNS_Result.action = 10; // pay step 1

    if (BNS_Pay->errorCode)
    {
        if((BNS_Pay->HTTPResponseCode == HTTP_AUTH_ERROR)
        ||(BNS_Pay->errorCode == BNS_BAD_TOKEN)
        ||(BNS_Pay->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = Pay(card, CurCheque, p_sum); // повтор команды
        }
    }

    Log((char*)(" ===End #"+CheckNumStr+" ===").c_str());
    Log("\n\n");

    return res;

}


int TBonus::Discount(string card, double p_sum, double d_sum, string tran_id)
{
    int res = -1;

    Log("Operation: SALE BONUS DISCOUNT (Step #2)");
    Log((char*)("Card: "+card).c_str());

    if (token=="")
    {
        if(GetToken()) return res;
    }

    string cmd = COMMAND_PAYMENT;
    cmd = cmd+tran_id+"/payment";
    cmd = " /Kacca/bonus/bonus -X PUT -d @- \"" + ConnectionString + cmd+ "\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 10";
    cmd = cmd + " --max-time 30";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;


    string body = "echo '{\"identity\":{\"cardNumber\":"+card+
    ", \"tillAuth\":\""+Terminal+"\""+
    "}"
    ", \"payments\":[{"+
    "\"id\":\""+OrgID+"\""+
    ",\"summ\":"+Double2Str(d_sum)+
    ", \"type\":3"+
    "}]"+
    "}'";

    cmd = body + " | " + cmd;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    BNS_Pay = new BNS_RET_PURCHASE;
    BNS_Pay->init();

    // Обработка результата
    ParseResult(BNS_Pay);

    BNS_Result.action = 11; // pay step 2
    BNS_Result.id = tran_id;

    if (BNS_Pay->errorCode)
    {
        if((BNS_Pay->HTTPResponseCode == HTTP_AUTH_ERROR)
        ||(BNS_Pay->errorCode == BNS_BAD_TOKEN)
        ||(BNS_Pay->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = Discount(card, p_sum, d_sum, tran_id);
        }
    }

    return res;
}


int TBonus::Pay_SetPayment(string card, double p_sum, double d_sum, string tran_id)
{
    int res = -1;

    Log("Operation: SALE PAYMENT (Step #3)");
    Log((char*)("Card: "+card).c_str());

    if (token=="")
    {
        if(GetToken()) return res;
    }

    string cmd = COMMAND_PAYMENT;
    cmd = cmd+tran_id+"/payment";
    cmd = " /Kacca/bonus/bonus -X POST -d @- \"" + ConnectionString + cmd+ "\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 10";
    cmd = cmd + " --max-time 30";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;


    string body = "echo '{\"identity\":{\"cardNumber\":"+card+
    ", \"tillAuth\":\""+Terminal+"\""+
    "}"
    ", \"payments\":[{"+
    "\"id\":\""+OrgID+"\""+
    ",\"summ\":"+Double2Str(p_sum)+
    ", \"type\":1"+
    "}]"+
    "}'";

    cmd = body + " | " + cmd;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    BNS_Pay = new BNS_RET_PURCHASE;
    BNS_Pay->init();

    // Обработка результата
    ParseResult(BNS_Pay);

    BNS_Result.action = 12; //pay step 3
    BNS_Result.id = tran_id;

    if (BNS_Pay->errorCode)
    {
        if((BNS_Pay->HTTPResponseCode == HTTP_AUTH_ERROR)
        ||(BNS_Pay->errorCode == BNS_BAD_TOKEN)
        ||(BNS_Pay->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = Pay_SetPayment(card, p_sum, d_sum, tran_id);
        }
    }

    return res;
}


 int TBonus::Pay_Fix(string card, double p_sum, string tran_id)
{
    int res = -1;

    Log("Operation: SALE FIX (Step #4)");
    Log((char*)("Card: "+card).c_str());

    if (token=="")
    {
        if(GetToken()) return res;
    }

	time_t tt = time(NULL);
	tm* t = localtime(&tt);
    char dt[256];
    sprintf(dt, "%4d-%02d-%02dT%02d:%02d:%02d",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    string dtstr = dt;

    string cmd = COMMAND_PAYMENT;
    cmd = cmd+tran_id;
    cmd = " /Kacca/bonus/bonus -X PUT -d @- \"" + ConnectionString + cmd+ "\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 10";
    cmd = cmd + " --max-time 30";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;

    string body = "echo '{\"identity\":{\"cardNumber\":"+card+
    ", \"tillAuth\":\""+Terminal+"\""+
    ", \"date\":\""+dtstr+"\""+
    ", \"checkNumber\":\""+ CheckNumStr +"\""+
    "}"
    ", \"payments\":[{"+
    "\"id\":\""+OrgID+"\""+
    ",\"summ\":"+Double2Str(p_sum)+
    ", \"type\":3"+
    "}]"+
    "}'";

    cmd = body + " | " + cmd;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    BNS_Pay = new BNS_RET_PURCHASE;
    BNS_Pay->init();

    // Обработка результата
    ParseResult(BNS_Pay);

    BNS_Result.action = 13; // pay step 4 - fix action
    BNS_Result.id = tran_id;

    if (BNS_Pay->errorCode)
    {
        if((BNS_Pay->HTTPResponseCode == HTTP_AUTH_ERROR)
        ||(BNS_Pay->errorCode == BNS_BAD_TOKEN)
        ||(BNS_Pay->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = Pay_Fix(card, p_sum, tran_id);
        }
    }

    return res;
}


int TBonus::Return(string card, double p_sum, double d_sum, string tran_id)
{
    int res = -1;

    Log("Operation: RETURN INIT (Step #1)");
    Log((char*)("Card: "+card).c_str());

    if (token=="")
    {
        if(GetToken()) return res;
    }

    string cmd = COMMAND_RETURN;
    cmd = " /Kacca/bonus/bonus -X POST -d @- \"" + ConnectionString + cmd+ "\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 30";
    cmd = cmd + " --max-time 60";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;

    //

	time_t tt = time(NULL);
	tm* t = localtime(&tt);
    char dt[256];
    sprintf(dt, "%4d-%02d-%02dT%02d:%02d:%02d",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    string dtstr = dt;

    string body = "echo '{\"baseParentPurchaseID\":"+tran_id+
    ", \"AccruedSumm\":"+Double2Str(p_sum)+
    ", \"CancellationSum\":"+Double2Str(d_sum)+
    "}'";

    cmd = body + " | " + cmd;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    BNS_Pay = new BNS_RET_PURCHASE;
    BNS_Pay->init();

    // Обработка результата
    ParseResult(BNS_Pay);

    BNS_Result.action = 20; // return step 1

    if (BNS_Pay->errorCode)
    {
        if((BNS_Pay->HTTPResponseCode == HTTP_AUTH_ERROR)
        ||(BNS_Pay->errorCode == BNS_BAD_TOKEN)
        ||(BNS_Pay->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = Return(card, p_sum, d_sum, tran_id); // повтор команды
        }
    }

    Log((char*)(" ===End #"+CheckNumStr+" ===").c_str());
    Log("\n\n");

    return res;

}



int TBonus::Return_Fix(string card, FiscalCheque* CurCheque, double p_sum, double d_sum, string tran_id)
{
    int res = -1;

    Log("Operation: RETURN FIX (Step #2)");
    Log((char*)("Card: "+card).c_str());

    if (token=="")
    {
        if(GetToken()) return res;
    }

    string cmd = COMMAND_RETURN;
    cmd = " /Kacca/bonus/bonus -X PUT -d @- \"" + ConnectionString + cmd + tran_id + "\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 60";
    cmd = cmd + " --max-time 60";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;


    string position = "[";

    int num=0;
	for (int i=0;i<CurCheque->GetCount();i++)
    {
        GoodsInfo crGoods = (*CurCheque)[i];

        if (((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT)) || (crGoods.ItemCode==TRUNC)) continue;

        // добавляем информацию по позициям чека в запрос
        if (num>0) position += ",";

        position = position + "{" +
        "\"lineNumber\":"+ Int2Str(++num) +
        ", \"productCode\":\""+Int2Str(crGoods.ItemCode)+"\""+
        ", \"groupCode\":\""+""+"\""+
        ", \"barCode\":\""+crGoods.ItemBarcode+"\""+
        ", \"quantity\":"+ Double2Str(crGoods.ItemCount) +
        ", \"price\":"+ Double2Str(crGoods.ItemPrice) +
        ", \"summ\":"+ Double2Str(crGoods.ItemSum) +
        ", \"summWithoutDiscount\":"+ Double2Str(crGoods.ItemCount * crGoods.ItemFullPrice) +
        //", \"customProperties\":[{\"SpecialGroup\":\"VIP\"}]"+
        ", \"customProperties\":null"+
        "}";

    }

    //

	time_t tt = time(NULL);
	tm* t = localtime(&tt);
    char dt[256];
    sprintf(dt, "%4d-%02d-%02dT%02d:%02d:%02d",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    string dtstr = dt;

    position = position + "]";

    string body = "echo '{\"cardNumber\":"+card+
    ", \"tillAuth\":\""+Terminal+"\""+
    ", \"date\":\""+dtstr+"\""+
    ", \"summ\":"+Double2Str(CurCheque->GetSum())+
    ", \"summWithoutDiscount\":"+Double2Str(CurCheque->GetFullSum())+
    ", \"positions\":"+position+
    //", \"customProperties\":[{\"Type\":\"Order\"}]"+
    ", \"customProperties\":null"+
    "}'";

    cmd = body + " | " + cmd;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    BNS_Pay = new BNS_RET_PURCHASE;
    BNS_Pay->init();

    // Обработка результата
    ParseResult(BNS_Pay);

    BNS_Result.action = 21; // return step 2 fix

    if (BNS_Pay->errorCode)
    {
        if((BNS_Pay->HTTPResponseCode == HTTP_AUTH_ERROR)
        ||(BNS_Pay->errorCode == BNS_BAD_TOKEN)
        ||(BNS_Pay->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = Return_Fix(card, CurCheque, p_sum, d_sum, tran_id); // повтор команды
        }
    }

    Log((char*)(" ===End #"+CheckNumStr+" ===").c_str());
    Log("\n\n");

    return res;

}


int TBonus::Pay_Del(string card, string tran_id)
{
    int res = -1;

    Log("Operation: DELETE Payment ");
    Log((char*)("Card: "+card).c_str());

    if (token=="")
    {
        if(GetToken()) return res;
    }

	time_t tt = time(NULL);
	tm* t = localtime(&tt);
    char dt[256];
    sprintf(dt, "%4d-%02d-%02dT%02d:%02d:%02d",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    string dtstr = dt;

    string cmd = COMMAND_PAYMENT;
    cmd = cmd+tran_id;
    cmd = " /Kacca/bonus/bonus -X DELETE \"" + ConnectionString + cmd+ "\"";
    cmd = cmd + " --header \"Content-Type:application/json\"";
    cmd = cmd + " --header \"X-Till-Bearer:"+token+"\"";
    cmd = cmd + " --header \"X-Api-Version:"+ApiVersion+"\"";

    cmd = cmd + " --connect-timeout 20";
    cmd = cmd + " --max-time 60";

    cmd = cmd + " --silent";
    cmd = cmd + " --cacert "+CertFilePath;

    cmd = cmd + " " + Options;

    cmd = cmd + " -w \"{\\\"HTTPResponseCode\\\":%{http_code}}\"";

    cmd = cmd + " > "+tokenFileName;

    int ret = run(cmd);
    if(ret) return SetTimeoutError();

    BNS_Pay = new BNS_RET_PURCHASE;
    BNS_Pay->init();

    // Обработка результата
    ParseResult(BNS_Pay);

    BNS_Result.action = 14;  // cancel current pay operation
    BNS_Result.id = tran_id;

    if (BNS_Pay->errorCode)
    {
        if((BNS_Pay->HTTPResponseCode == HTTP_AUTH_ERROR)
        ||(BNS_Pay->errorCode == BNS_BAD_TOKEN)
        ||(BNS_Pay->errorCode == BNS_ERROR_TOKEN))
        {
            Log("=== # TOKEN ERROR # ===");
            Log("Повтор команды");

            token = "";
            res = Pay_Del(card, tran_id);
        }
    }

    return res;
}


BNS_RET* TBonus::exec(string a_oper, string a_card, string a_check, FiscalCheque* CurCheque, double p_sum, double d_sum, string tran_id)
{

    Log("===  START EXEC BNS === ");

    Log((char*)(" === Start #"+CheckNumStr+" ===").c_str());


//Тест
//a_sha1 = "CB073E388D292E653B9C360B45A2B3ECBC05F054";


#ifdef LINUX_BUILD
	setlocale( LC_ALL, "ru_RU->utf8");
#endif

    cd = iconv_open("cp1251", "utf8");


/*  // Проверка: Включено "Спасибо" или нет
	FILE * flg_pay = fopen("/Kacca/pcx/nopcx.flg","r");
	if(flg_pay)
	{
	    fclose(flg_pay);
        PCX_Result.info = "EXIT: Программа Спасибо от сбербанка отключена !";
        Log(PCX_Result.info.c_str());
	    return &PCX_Result;
	}
*/

/*
	if (!CurCheque)
		CurCheque = Cheque;
	if (!CurCheque) return;

	FiscalCheque *tmpCheque;
	tmpCheque = new FiscalCheque;

*/

    string Version = "BNS 1.00 test";

    /////////////////////////////////

	char pfile[]="/Kacca/bonus/p";
	char efile[]="/Kacca/bonus/e";

    remove(efile);
    remove(pfile);

    //Логи забивают место
    //remove("/Kacca/TEST.log");
    //remove("/Kacca/RECV.log");
    //remove("/Kacca/SENT.log");

    string LogStr="";
    CheckNumStr = a_check;
    string a_newcard = a_check; // в случае замены новая карта передается в 3м параметре

    //LogStr = "------------------------- "+Version+" -------------------------------";
    //Log(LogStr.c_str());

    BNS_Result.init();

/*
    LogStr = "Инициализация библиотеки PCX ...";

	if(!Init())
	{
	    ErrLog(-1, "Ошибка инициализации библиотеки PCX!");
        LogStr+=" ERROR";

        Log(LogStr.c_str());

		return -1;
	}
    LogStr+=" SUCCESS";

    Log(LogStr.c_str());
*/


   // a_dttran="";

//    string a_oper = argv[1];
//
////
//    a_card  = argv[2]; /*карта*/
//    a_sha1  = argv[3]; /*хэш SHA1*/
//    a_sum   = argv[4]; /*сумма баллов (в копейках)*/
//    a_chsum = argv[5]; /*сумма чека*/
//    a_check = argv[6]; /*номер чека*/
//
//    a_tran  = "";
//    a_dttran = "";
//
//    if (argv[7])
//    {
//        a_tran = argv[7];
//    }
//
//    if (argv[8])
//    {
//        a_dttran = argv[8];
//    }

    if (trim(a_card).empty())
    {
        Log("ERROR: Не указан номер карты!");
        return &BNS_Result;
    }

//    disc_sum = Int100(a_sum);       // Сумма баллов "Спасибо"
//    //bank_sum = Str2Int(a_chsum);     // Сумма оплаты по банку (сумма чека)
//    //full_sum = bank_sum + disc_sum;  // Сумма чека без скидки "Спасибо"
//
//    a_track2= a_card;
//
//    pwd_card = GetPwdCardNumber(a_card);
//
//    string str_track2 = SetPwdNumber(a_track2);

   // if (a_sha1.length()<5) a_sha1 = GetSHA1(a_card); /*хэш SHA1*/

//


        ;;Log("=== Параметры ===");

        ;;Log(("  Операция: "+a_oper).c_str());
        ;;Log(("  Карта: "+a_card).c_str());
        ;;Log(("  Чек # "+a_check).c_str());
        ;;Log(("  Сумма начисления: "+Double2Str(p_sum)).c_str());
        ;;Log(("  Сумма списания: "+Double2Str(d_sum)).c_str());
        ;;Log(" ");
        //;;Log(("  Сумма чека: "+a_chsum).c_str());
        //;;Log(("  Хэш: "+a_sha1).c_str());
        ;;Log(("  ID транзакции : "+tran_id).c_str());
       // if (a_dttran.length()<5) ;;Log(("  DateTime транзакции: "+a_dttran).c_str());



  //  Init();

//    Log("Check BNS...");

//        LogStr = " ..SUCCESS";
//        Log(LogStr.c_str());

//        //string Verlib = PCX->GetVersion();
//
//        if ((a_oper == "-v")||(a_oper == "-V")||
//            (a_oper == "-ver")||(a_oper == "-Ver")||(a_oper == "-VER")
//           )
//        {
//
//            // если нужно получить версию программы
//            printf("Version: %s\n", Version.c_str());
//            printf("Library: %s\n", Verlib.c_str());
//            ErrLog(1,(char*)("Версия: "+Version+"\n"+"Версия библиотекм: "+Verlib).c_str());
//            Log("------------------------------------------------------------------------------");
//            return &PCX_Result;
//        }

//        Log(("  Library Version:"+ Verlib).c_str());

        //string str_track2 = GetPwdCardNumber(a_track2);

        //a_sha1  = GetSHA1(a_card); /*хэш SHA1*/


        if ((a_oper=="inf") && (UseBNS))
        {
            GetCardInfo(a_card);
        }

        if ((a_oper=="inf&reg") && (UseBNS))
        {
            GetCardInfo(a_card);

            if(BNS_Result.errorCode == BNS_ERROR_NOTFOUND){
                Log("Карта не зарегистрирована!");
                Log("Добавляем карту!");

                AddCard(a_card);
            }

        }


        if ((a_oper=="reg") && (UseBNS))
        {
            AddCard(a_card);
        }


        if ((a_oper=="change") && (UseBNS))
        {

            ChangeCard(a_card, a_newcard);
        }


        if((a_oper=="pay") && (UsePay))
        {
            Pay(a_card, CurCheque, p_sum);

            tran_id = BNS_Result.id;

            double ppp_sum = 0;

            if (d_sum>0)
                Discount(a_card, ppp_sum, d_sum, tran_id);

            double pp_sum = CurCheque->GetSum();

            Pay_SetPayment(a_card, pp_sum, d_sum, tran_id);

            Pay_Fix(a_card, p_sum, tran_id);
        }


        if((a_oper=="sale&init") && (UsePay))
        {
            Pay(a_card, CurCheque, p_sum);
        }


        if((a_oper=="sale&disc") && (UseDiscount))
        {
            if (!trim(tran_id).empty())
                Discount(a_card, p_sum, d_sum, tran_id);
            else
                Log("ERROR: Не указан ID покупки!");
        }

       if((a_oper=="sale&payment") && (UsePay))
        {
            if (!trim(tran_id).empty())
                Pay_SetPayment(a_card, p_sum, d_sum, tran_id);
            else
                Log("ERROR: Не указан ID покупки!");

        }


       if((a_oper=="sale&fix") && (UsePay))
        {
            if (!trim(tran_id).empty())
                Pay_Fix(a_card, p_sum, tran_id);
            else
                Log("ERROR: Не указан ID покупки!");
        }


        if((a_oper=="return&init") && (UsePay))
        {
            if (!trim(tran_id).empty())
                Return(a_card, p_sum, d_sum, tran_id);
            else
                Log("ERROR: Не указан ID возврата!");
        }

       if((a_oper=="return&fix") && (UsePay))
        {
            if (!trim(tran_id).empty())
                Return_Fix(a_card, CurCheque, p_sum, d_sum, tran_id);
            else
                Log("ERROR: Не указан ID покупки!");
        }


       if(a_oper=="sale&del")
        {
            if (!trim(tran_id).empty())
                Pay_Del(a_card, tran_id);
            else
                Log("ERROR: Не указан ID покупки!");
        }

        // Ищем основную карту по номеру телефона
        // если карта единственная - возвращаем ее
       if(a_oper=="cardByPhone")
        {
            GetCustomerInfo(a_card, true);
        }

       if(a_oper=="custByCard")
        {
            GetCustomerInfo(a_card, false);
        }
/*
        if((a_oper=="rev"))
        {
            Reverse(a_card, a_sha1, CurCheque, a_sum);
        }

        if((a_oper=="retdisc"))
        {
            ReturnDisc(a_card, a_sha1, tran_id, a_sum);
        }

        if((a_oper=="retpay"))
        {
            //ReturnPay();
        }

        // Версия библиотеки
        PCX_Result.LibVersion = Verlib;

        //Destroy();
*/

    if( cd != (iconv_t)(-1) )
    {
        iconv_close(cd);
    }

    Log((char*)(" === End #"+CheckNumStr+" ===").c_str());
    Log("===  END EXEC BNS === ");
    Log(" ");
    Log(" ");

    BNS_Result.card = a_card;
    BNS_Result.operation = a_oper;

    BNS_Result.terminal = Terminal;

	return &BNS_Result;

} //exec
