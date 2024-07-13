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

#include "linpcx.h"

#include <PCX.h>
//#include "Fiscal.h"

//#include "Utils.h"

extern "C"
{
#include <uuid/uuid.h>
}


static char * fconfig = (char*)"/Kacca/pcx/pcx.cfg";
static char * flocal  = (char*)"/Kacca/pcx/local.cfg";

inline void fgets(char* buf, FILE * f)
{
	char * nl;
	fgets(buf, 1024, f);
	nl = strrchr(buf, 0x0d);//'\n');
	if(nl) *nl = '\0';
	nl = strrchr(buf, 0x0a);//'\n');
	if(nl) *nl = '\0';
}

string FmtAmount(const char* szAmount) {
	char szResult[0x20];
	long long amount = 0;

	sscanf(szAmount, "%lld", &amount);
	sprintf(szResult, "%.02f", (double)amount / 100.0);
	return szResult;
}

// Конвертация из utf8 в win1251
string convert(iconv_t cd, const char *in )
{
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


int LProc::Log(const char* logstr, bool endline)
{
	FILE * log = fopen("/Kacca/pcx/.pcx.log","a");

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

bool LProc::Init()
{

    //Логи забивают место
    remove("/Kacca/TEST.log");
    remove("/Kacca/RECV.log");
    remove("/Kacca/SENT.log");

//#ifdef LINUX_BUILD
//	setlocale( LC_ALL, "ru_RU->utf8");
//#endif
//
//    cd = iconv_open("cp1251", "utf8");

    string Version = "Pcx 2.00 testing";

    /////////////////////////////////


    Log("\n\n");
    Log("=========================  INIT PCX  ===============================");

    string LogStr="";
    LogStr = "------------------------- "+Version+" ----------------------------";
    Log(LogStr.c_str());

    bool res=true;

	PCX = NULL;

	//============================
	// При загрузке кассового ПО выполняется cоздание и инициализация объекта PCX
	// Объект должен жить до завершения работы кассового ПО
	// Создания и инициализации объекта на каждом запросе не должно быть

	try {
		Log("Создание объекта PCX...");
		PCX = CreatePcx();
		if (PCX == NULL)
			throw runtime_error("Ошибка при создании объекта PCX");

		Log("  .. SUCCESS");

        string Verlib = PCX->GetVersion();

        Log(("Version: " +Version).c_str());
        Log(("Library: " + Verlib).c_str());

        SetSettings();

        SetParam();

		// выполняем инициализацию объекта
		Log("Инициализация PCX");
		int RetCode = PCX->Init();
		if(RetCode != 0) {
			// обработка неуспешности инициализации
			stringstream str;
			str << "Инициализация не успешна, ErrorCode: " << RetCode << ", \"" << PCX->GetErrorMessage(RetCode) << "\", \"" << PCX->GetErrorInfo(RetCode) << "\"" << endl;
			throw runtime_error(str.str());
		}
		//============================

        Log(" ...SUCCESS");

		//============================
		// После успешной инициализации
		// необходимо включить механизм фоновой отправки запросов

		Log("Включение фоновой отправки...");
		RetCode = PCX->EnableBkgroundFlush(true);
		if(RetCode != 0)
		{
			// обработка неуспешности вызова EnableBkgndFlush
			stringstream str;
			str << "Ошибка вызова EnableBkgndFlush, ErrorCode: " << RetCode << ", \"" << PCX->GetErrorMessage(RetCode) << "\", \"" << PCX->GetErrorInfo(RetCode) << "\"";
			throw runtime_error(str.str());
		}

        Log(" ...SUCCESS");

    Log("----------------------------------------------------------------------------");
    Log("\n\n");

	} catch(exception& ex) {
		// обработка ошибки создания объекта PCX
		Log("  ..ERROR: ");
		//Log(ex.what());
		return 1;

	}
    return res;
}


//#endif



const char* LProc::PrintFileName()
{
	static char p[] = "/Kacca/pcx/p";
	return p;
}


LProc::LProc()
{
    e_file = "/Kacca/pcx/e";
    p_file = "/Kacca/pcx/p";

    LineWidth =33;

    a_card      ="";
    pwd_card    ="";
    a_sum       ="";
    a_chsum     ="";

    a_check     ="";
    a_track2    ="";

    a_sha1      ="";
    a_tran      ="";
    a_dttran    ="";

    full_sum=disc_sum=bank_sum=0;

    PCX=NULL;

}

LProc::~LProc()
{

    Destroy();

}



int LProc::ErrLog(int err,const char* desc, bool onlydesc)
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

//функция "растягивает" разделитель строк до нужного размера
string LineSeparator(string str, int LineWidth)
{
    int Len = str.length();
    if ((Len>0) && (Len<LineWidth))
    {
        string temp;
        char separator = str[0];
        int i=0;
        while ((i<Len) && (separator==str[i])) i++;

        if (i>=Len) //это разделитель
            for(int j=i;j<LineWidth;j++) str+=separator;


    }
    return str;
}


// фукция форматирования чека
const char * LProc::FormatCheck(const char * CheckStr, bool WithCopy)
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
string LProc::GetSHA1(string card)
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


string LProc::SetPwdNumber(string number, unsigned int len)
{
    string str_asteriks = "****************************";
    string result = "";
    unsigned int maxnum=16;

    for (unsigned i=0;i<min(number.length(), len);i++)
        if (number[i]!='"') result+=number[i];

    result += (maxnum>result.length())?str_asteriks.substr(0, maxnum-result.length()):"";

    return result;
}

string LProc::GetPwdCardNumber(string number)
{
    string str_asteriks = "******************************************";
    string result = "";

    int len = number.length();
    if (len>4)
        result = str_asteriks.substr(0, len-4) + number.substr(len-4, 4);

    return result;
}


//============================***************************************************************

bool LProc::ProccessRespond(ICardInfoItem* CardInfoItem, string StrPar, string text)
{
    bool res=true;

    if (CardInfoItem)
    {
        if (StrPar=="CS_S")
        {
            ErrLog(0, CardInfoItem->GetValue(), true); // вывод состояния счета карты
            Log((text+" ("+StrPar+"): "+CardInfoItem->GetValue()).c_str());
        }
        else
        {
            ErrLog(0, (StrPar+"="+FmtAmount(CardInfoItem->GetValue())).c_str(), true); // вывод состояния счета карты
            Log((text+" ("+StrPar+"): "+FmtAmount(CardInfoItem->GetValue())).c_str());
        }
    }
    else
    {
        if (StrPar=="CS_S")
        {
            ErrLog(0, "ERROR", true); // вывод состояния счета карты
            Log((text+" ("+StrPar+"): ERROR").c_str());
        }
        else
        {
            ErrLog(0, (StrPar+"=0").c_str(), true); // вывод состояния счета карты
            Log((text+" ("+StrPar+"):  ERROR").c_str());
        }
        res=false;
    }

    return res;
}


void LProc::ParseResult(int RetCode, IRefundResponse* AuthResp,  bool PrintCheck)
{

;;Log("ParseResult");

string tmp_val;

PCX_Result.ReturnCode = RetCode;

string TranID, TranDT;

if(PCX!=NULL)
{

    try
    {

        if (RetCode>=0)
        {

            Log(" ... SUCCESS");
            bool flag_ch=false;

            //! передаем ID транзакции текущей операции
            TranID = AuthResp->GetTransaction()->GetID();
            TranDT = AuthResp->GetTransaction()->GetDateTime();

            if (RetCode==0)
            {
                flag_ch=true;

                ErrLog(RetCode, "Успешно (online) ", false);


                // Состояние карты
                ICardInfoItem*  CS_S  = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "CS",  "S", "N"); // ищем CardInfoItem с состоянием карты
                ProccessRespond(CS_S, "CS_S", "Card status");

                PCX_Result.CS_S = CS_S->GetValue();

                PCX_Result.mode = 0; // online

                ErrLog(0,TranID.c_str(), true); // идентификатор транзакции
                ErrLog(0,TranDT.c_str(), true); // дата и время транзакции

                //Активных баллов на счете карты:
                ICardInfoItem*  BNS_S = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "BNS", "S", "N"); // ищем CardInfoItem с количеством активных баллов на счете карты

                if (ProccessRespond(BNS_S, "BNS_S", "Active bonus"))
                    PCX_Result.BNS_S =FmtAmount(BNS_S->GetValue());

                //Отложенных баллов на счете карты
                ICardInfoItem*  BNS_DELAY_S = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "BNS_DELAY", "S", "N"); // ищем CardInfoItem с количеством отложенных баллов на счете карты

                if (ProccessRespond(BNS_DELAY_S, "BNS_DELAY_S", "Delay bonus"))
                    PCX_Result.BNS_DELAY_S =FmtAmount(BNS_DELAY_S->GetValue());

                //Активные баллы на счете карты в пересчете на рубли
                ICardInfoItem*  AB_S  = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "AB",  "S", "N"); // ищем CardInfoItem с пересчетом активных баллов в рубли по курсу списания ПЦ
                if (ProccessRespond(AB_S, "AB_S", "Active bonus (RUR)"))
                {
                    PCX_Result.AB_S =FmtAmount(AB_S->GetValue());
                    PCX_Result.active_bonus =Str2Double(PCX_Result.AB_S);
                }

                //"Изменением счета активных баллов: "
                ICardInfoItem*  BNS_C = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "BNS", "C", "N"); // ищем CardInfoItem с изменением количества активных баллов на счете карты
                if (ProccessRespond(BNS_C, "BNS_C", " Change bonus"))
                    PCX_Result.BNS_C =FmtAmount(BNS_C->GetValue());

                // Изменением счета отложенных баллов:
                ICardInfoItem*  BNS_DELAY_C = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "BNS_DELAY", "C", "N"); // ищем CardInfoItem с изменением количества отложенных баллов на счете карты
                if (ProccessRespond(BNS_DELAY_C, "BNS_DELAY_C", "Change delay bonus"))
                    PCX_Result.BNS_DELAY_C =FmtAmount(BNS_DELAY_C->GetValue());

                //"Изменение счета активных баллов на счете карты в пересчете на рубли: "
                ICardInfoItem*  AB_C  = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "AB",  "C", "N"); // ищем CardInfoItem с изменением количества активных баллов в рубли по курсу списания ПЦ
                if (ProccessRespond(AB_C, "AB_C", "Change active bonus"))
                {
                    PCX_Result.AB_C =FmtAmount(AB_C->GetValue());
                    PCX_Result.change_bonus =Str2Double(PCX_Result.AB_C);
                }

            }

            if ( RetCode == IPcxCore::ERR_OPERATION_DELAYED)
            {
                PCX_Result.mode = 1; // off-line

                flag_ch=true;
                //запрос успешно выполнился в оффлайн режиме - разбираем только параметры сформированные на уровне объекта PCX
                Log((" >> Запрос успешно выполнился в оффлайн режиме << RetCode = "+Int2Str(RetCode)).c_str());
                ErrLog(0, "Успешно (offline) ", false);
                ErrLog(0, "OFFLINE", true);
                //! передаем ID транзакции текущей операции
                ErrLog(0,TranID.c_str(), true); // идентификатор транзакции
                ErrLog(0,AuthResp->GetTransaction()->GetDateTime(), true); // дата и время транзакции

                //PCX_Result.id_tran =AuthResp->GetTransaction()->GetID();
                //PCX_Result.dt_tran =AuthResp->GetTransaction()->GetDateTime();
            }

            // Общие параметры: чек и транзакции
            if (flag_ch)
            {
                //string ID2 = AuthResp->GetTransaction()->GetID();
                //string DT2 = AuthResp->GetTransaction()->GetDateTime();


                PCX_Result.id_tran =TranID;
                PCX_Result.dt_tran =TranDT;

                Log(("Транзакция: " + TranID).c_str()); // идентификатор транзакции
                Log(("DateTime транзакции: " + TranDT).c_str()); // дата и время транзакции



                string OperatorMessage = convert(cd, AuthResp->GetOperatorMessage());
                string ChequeMessage = convert(cd, AuthResp->GetChequeMessage());

                ErrLog(0, OperatorMessage.c_str(), true);
                Log(("OperatorMessage="+OperatorMessage).c_str());

                Log(("Чек: \n"+ChequeMessage).c_str());


                //PCX_Result.info = OperatorMessage;

                // Если нужен чек - печатаем
                if (PrintCheck)
                {
                    FILE *fr = fopen(p_file, "w");
                    if(fr)
                    {
                        fprintf(fr, FormatCheck(ChequeMessage.c_str()), false);
                        fclose(fr);
                        Log("Чек сформирован. ");
                    }
                    else
                    {
                        //ErrLog(-1, "Ошибка формирования чека. Файл не сформирован!", true);
                        Log("  Ошибка формирования чека!");
                    }
                }
            }
		}
		else
		{
			//запрос не успешен

			Log(" ... ERROR");

            string ErrorMessage = convert(cd, PCX->GetErrorMessage(RetCode));
            string ErrorInfo = convert(cd, PCX->GetErrorInfo(RetCode));

            ErrLog(RetCode, ("Запрос завершен с ошибкой, ErrorCode: "+Int2Str(RetCode)).c_str(), false);
            ErrLog(1, ErrorMessage.c_str(), true);
            ErrLog(1, ErrorInfo.c_str(), true);

            Log(("Запрос завершен с ошибкой, ErrorCode: "+Int2Str(RetCode)).c_str());
            Log(("ErrorMessage="+ErrorMessage).c_str());
            Log(("ErrorInfo="+ErrorInfo).c_str());

            PCX_Result.error = true;
            PCX_Result.info = "ErrorCode: "+Int2Str(RetCode) +"\n"+ErrorMessage;//ErrorInfo;


		}

    }
    catch(exception& ex)
    {
		// обработка ошибки создания объекта PCX
		Log("   ERROR:");
		Log(ex.what());
	}
}
}

void LProc::InitResult()
{
;;Log(" => InitResult <=");

PCX_Result.init();

//ReturnCode = -1;
//PCX_Result.CS_S = "";
//
//PCX_Result.BNS_S = "";
//PCX_Result.BNS_DELAY_S = "";
//PCX_Result.AB_S = "";
//PCX_Result.BNS_C = "";
//PCX_Result.BNS_DELAY_C = "";
//PCX_Result.AB_C = "";
//PCX_Result.info = "";

}

void LProc::ParseResult(int RetCode, IAuthResponse* AuthResp, bool PrintCheck )
{

;;Log("ParseResult");

string tmp_val;

PCX_Result.ReturnCode = RetCode;

string TranID, TranDT;

if(PCX!=NULL)
{

    try
    {

        if (RetCode>=0)
        {

            Log(" ... SUCCESS");
            bool flag_ch=false;

            //! передаем ID транзакции текущей операции
            TranID = AuthResp->GetTransaction()->GetID();
            TranDT = AuthResp->GetTransaction()->GetDateTime();

            if (RetCode==0)
            {
                flag_ch=true;

                ErrLog(RetCode, "Успешно (online) ", false);


                // Состояние карты
                ICardInfoItem*  CS_S  = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "CS",  "S", "N"); // ищем CardInfoItem с состоянием карты
                ProccessRespond(CS_S, "CS_S", "Card status");

                PCX_Result.CS_S = CS_S->GetValue();

                PCX_Result.mode = 0; // online

                ErrLog(0,TranID.c_str(), true); // идентификатор транзакции
                ErrLog(0,TranDT.c_str(), true); // дата и время транзакции

                //Активных баллов на счете карты:
                ICardInfoItem*  BNS_S = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "BNS", "S", "N"); // ищем CardInfoItem с количеством активных баллов на счете карты

                if (ProccessRespond(BNS_S, "BNS_S", "Active bonus"))
                {
                    PCX_Result.BNS_S =FmtAmount(BNS_S->GetValue());
                }

                //Отложенных баллов на счете карты
                ICardInfoItem*  BNS_DELAY_S = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "BNS_DELAY", "S", "N"); // ищем CardInfoItem с количеством отложенных баллов на счете карты

                if (ProccessRespond(BNS_DELAY_S, "BNS_DELAY_S", "Delay bonus"))
                    PCX_Result.BNS_DELAY_S =FmtAmount(BNS_DELAY_S->GetValue());

                //Активные баллы на счете карты в пересчете на рубли
                ICardInfoItem*  AB_S  = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "AB",  "S", "N"); // ищем CardInfoItem с пересчетом активных баллов в рубли по курсу списания ПЦ
                if (ProccessRespond(AB_S, "AB_S", "Active bonus (RUR)"))
                {
                    PCX_Result.AB_S =FmtAmount(AB_S->GetValue());
                    PCX_Result.active_bonus =Str2Double(PCX_Result.BNS_S);
                }

                //"Изменением счета активных баллов: "
                ICardInfoItem*  BNS_C = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "BNS", "C", "N"); // ищем CardInfoItem с изменением количества активных баллов на счете карты
                if (ProccessRespond(BNS_C, "BNS_C", " Change bonus"))
                    PCX_Result.BNS_C =FmtAmount(BNS_C->GetValue());

                // Изменением счета отложенных баллов:
                ICardInfoItem*  BNS_DELAY_C = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "BNS_DELAY", "C", "N"); // ищем CardInfoItem с изменением количества отложенных баллов на счете карты
                if (ProccessRespond(BNS_DELAY_C, "BNS_DELAY_C", "Change delay bonus"))
                    PCX_Result.BNS_DELAY_C =FmtAmount(BNS_DELAY_C->GetValue());

                //"Изменение счета активных баллов на счете карты в пересчете на рубли: "
                ICardInfoItem*  AB_C  = PCX->FindCardInfoItem(AuthResp->GetCardInfo(), "AB",  "C", "N"); // ищем CardInfoItem с изменением количества активных баллов в рубли по курсу списания ПЦ
                if (ProccessRespond(AB_C, "AB_C", "Change active bonus"))
                {
                    PCX_Result.AB_C =FmtAmount(AB_C->GetValue());
                    PCX_Result.change_bonus =Str2Double(PCX_Result.AB_C);
                }

            }

            if ( RetCode == IPcxCore::ERR_OPERATION_DELAYED)
            {
                PCX_Result.mode = 1; // off-line

                flag_ch=true;
                //запрос успешно выполнился в оффлайн режиме - разбираем только параметры сформированные на уровне объекта PCX
                Log((" >> Запрос успешно выполнился в оффлайн режиме << RetCode = "+Int2Str(RetCode)).c_str());
                ErrLog(0, "Успешно (offline) ", false);
                ErrLog(0, "OFFLINE", true);
                //! передаем ID транзакции текущей операции
                ErrLog(0,TranID.c_str(), true); // идентификатор транзакции
                ErrLog(0,AuthResp->GetTransaction()->GetDateTime(), true); // дата и время транзакции

                //PCX_Result.id_tran =AuthResp->GetTransaction()->GetID();
                //PCX_Result.dt_tran =AuthResp->GetTransaction()->GetDateTime();
            }

            // Общие параметры: чек и транзакции
            if (flag_ch)
            {
                //string ID2 = AuthResp->GetTransaction()->GetID();
                //string DT2 = AuthResp->GetTransaction()->GetDateTime();


                PCX_Result.id_tran =TranID;
                PCX_Result.dt_tran =TranDT;

                Log(("Транзакция: " + TranID).c_str()); // идентификатор транзакции
                Log(("DateTime транзакции: " + TranDT).c_str()); // дата и время транзакции



                string OperatorMessage = convert(cd, AuthResp->GetOperatorMessage());
                string ChequeMessage = convert(cd, AuthResp->GetChequeMessage());

                ErrLog(0, OperatorMessage.c_str(), true);
                Log(("OperatorMessage="+OperatorMessage).c_str());

                Log(("Чек: \n"+ChequeMessage).c_str());

                // Если нужен чек - печатаем
                if (PrintCheck)
                {
                    FILE *fr = fopen(p_file, "w");
                    if(fr)
                    {
                        fprintf(fr, FormatCheck(ChequeMessage.c_str()), false);
                        fclose(fr);
                        PCX_Result.info ="OK. Чек сформирован.";
                        Log(PCX_Result.info.c_str());
                    }
                    else
                    {
                        //ErrLog(-1, "Ошибка формирования чека. Файл не сформирован!", true);
                        PCX_Result.info ="ERROR. Ошибка формирования чека!";
                        Log(PCX_Result.info.c_str());
                    }
                }
            }
		}
		else
		{
			//запрос не успешен

			Log(" ... ERROR");

            string ErrorMessage = convert(cd, PCX->GetErrorMessage(RetCode));
            string ErrorInfo = convert(cd, PCX->GetErrorInfo(RetCode));

            ErrLog(RetCode, ("Запрос завершен с ошибкой, ErrorCode: "+Int2Str(RetCode)).c_str(), false);
            ErrLog(1, ErrorMessage.c_str(), true);
            ErrLog(1, ErrorInfo.c_str(), true);

            Log(("Запрос завершен с ошибкой, ErrorCode: "+Int2Str(RetCode)).c_str());
            Log(("ErrorMessage="+ErrorMessage).c_str());
            Log(("ErrorInfo="+ErrorInfo).c_str());

            PCX_Result.error = true;
            PCX_Result.info = "ErrorCode: "+Int2Str(RetCode) +" "+ErrorMessage;//ErrorInfo;


		}

    }
    catch(exception& ex)
    {
		// обработка ошибки создания объекта PCX

		string err = ex.what();
		PCX_Result.info = " ERROR: "+err;//ErrorInfo;
		Log(PCX_Result.info.c_str());
	}
}

}


PCX_RET* LProc::GetCardInfo(string a_card, string a_sha1)
{

Log("GetCardInfo()");

    InitResult();

    bool res=0;

    PCX_Result.ReturnCode = -1;
    PCX_Result.action = BB_GETINFO;
    PCX_Result.card = a_card;
    PCX_Result.pwd_card = GetPwdCardNumber(a_card);
    PCX_Result.sha1 = a_sha1;

    PCX_Result.terminal = Terminal;

//=================================================================================================
    Log("=== Запрос информации по карте ===");

    if (PCX!=NULL)
    {
		//============================
		// 1-> Запрос информации по карте->

try{

		const char* ClientID = a_sha1.c_str(); // хэш SHA1
        //const char* srcString = "4276838321221063";

//        const char* ClientID = ClientIDstr;
		int ClientIDType = 6; // для хэша всегда 6
		IInfoRequest* InfoRq = PCX->CreateInfoRequest(); //создаем объект запроса
		IAuthResponse* AuthResp = PCX->CreateAuthResponse(); //создаем объект ответа
		InfoRq->SetClientID(ClientID); // заполняем идентификатор клиента
		InfoRq->SetClientIDType(ClientIDType); // заполняем тип идентификатора
		// добавляем расширения в запрос
		IExtension* Extension = PCX->CreateExtension();
		Extension->SetType("cheque_msg_req");
		Extension->SetCritical(true);
		Extension->AddParam("msg_req","y");
		InfoRq->GetExtensions()->AddItem(Extension);
		Extension = PCX->CreateExtension();
		Extension->SetType("PURCHASE_EXT_PARAMS");
		Extension->SetCritical(true);
		//заглушка
		//Extension->AddParam("PAN4", ); //Передача маскированного ПАНа

pwd_card=GetPwdCardNumber(a_card);
;;Log("pwd_card.c_str()");
;;Log(pwd_card.c_str());

		//Extension->AddParam("PAN4", pwd_card.c_str()); //Передача маскированного ПАНа
		Extension->AddParam("PAN4", pwd_card.c_str()); //Передача маскированного ПАНа

        //uuid
        char key[40];
        key[0]=0;
        uuid_t id;
        uuid_generate(id);
        uuid_unparse(id,key);
        Log(key);

		//Extension->AddParam("RECEIPT", "b8e83d7e-b8b1-4653-939c-46e52b4f1d46"); //уникальный идентификатор продажи
		Extension->AddParam("RECEIPT", key); //уникальный идентификатор продажи
		InfoRq->GetExtensions()->AddItem(Extension);

        PCX_Result.guid = key;

        ;;Log("=== Информация по карте ===");
		int RetCode = PCX->GetInfo_V1(InfoRq, AuthResp); // запрос информации по карте

        string tmp_val="";

		;;Log(("Код возврата: "+Int2Str(RetCode)).c_str());

         // обрабатываем ответ
         ParseResult(RetCode, AuthResp, false);


    }
    catch(exception& ex)
    {
		// обработка ошибки создания объекта PCX
		Log("   ERROR:");
		Log(ex.what());
		PCX_Result.error = true;
	}

    }

	else
	{
	    PCX_Result.info = "ERROR: Объект PCX не инициализирован";
	    Log(PCX_Result.info.c_str());
		//return -1;
        PCX_Result.error = true;
    }

	return &PCX_Result;

} // Info

PCX_RET* LProc::Discount(string a_card, string a_sha1, FiscalCheque* CurCheque, double DiscSumma)
{

    InitResult();

    PCX_Result.action = BB_DISCOUNT;
    PCX_Result.card = a_card;
    PCX_Result.sha1 = a_sha1;
    PCX_Result.pwd_card = GetPwdCardNumber(a_card);
    PCX_Result.terminal = Terminal;

	if (!CurCheque) return &PCX_Result;

    disc_sum = Int100(DiscSumma);

    Log("Операция списания баллов...");

	//******************************************
	// При загрузке кассового ПО выполняется cоздание и инициализация объекта PCX
	// Объект должен жить до завершения работы кассового ПО
	// Создания и инициализации объекта на каждом запросе не должно быть

    if (PCX!=NULL)
    {

    try{

	//============================
		// 2-> Списание баллов ("Спасибо")
		Log("Списание баллов с карты");

       const char* ClientID = a_sha1.c_str();

		//const char*  ClientID = "b2c9c2d511cc2d495a7727e427efbd0ce1535b8c"; // хэш SHA1

		int ClientIDType = 6; // для хэша всегда 6
		int Flag = 0; // 0 - обычный режим или 1 - режим force offline

		IAuthRequest* AuthRq = PCX->CreateAuthRequest(); //создаем объект запроса
		IAuthResponse* AuthResp = PCX->CreateAuthResponse(); //создаем объект ответа

		// добавляем расширения в запрос
		IExtension* Extension = PCX->CreateExtension();
		Extension->SetType("cheque_msg_req");
		Extension->SetCritical(true);
		Extension->AddParam("msg_req","y");
		AuthRq->GetExtensions()->AddItem(Extension);

		Extension = PCX->CreateExtension();
		Extension->SetType("PURCHASE_EXT_PARAMS");
		Extension->SetCritical(true);


		pwd_card = GetPwdCardNumber(a_card);

		Extension->AddParam("PAN4", pwd_card.c_str()); // Передача маскированного ПАНа

        //uuid
        char key[40];
        key[0]=0;
        uuid_t id;
        uuid_generate(id);
        uuid_unparse(id,key);

		//Extension->AddParam("RECEIPT", "b8e83d7e-b8b1-4653-939c-46e52b4f1d46"); // уникальный идентификатор продажи
		Extension->AddParam("RECEIPT", key); // уникальный идентификатор продажи

		Log(key);

        PCX_Result.guid = key;
        PCX_Result.result_summa = DiscSumma;
        PCX_Result.total_summa = CurCheque->GetSum();

		AuthRq->GetExtensions()->AddItem(Extension);

		AuthRq->SetClientID(ClientID);
		AuthRq->SetClientIDType(ClientIDType);
		AuthRq->SetCurrency("643"); // код валюты - рубли
		AuthRq->SetAmount(Int100(DiscSumma)); //сумма списания 30 р->

		// добавляем платежный инструмент в запрос
		IPaymentItem* PaymentItem = PCX->CreatePaymentItem();
		PaymentItem->SetPayMeans("P"); // для списания баллов используется инструмент "P"
		PaymentItem->SetAmount(Int100(DiscSumma)); // сумма списания в платежном иструменте 30 р->
		AuthRq->GetPayment()->AddItem(PaymentItem);

		for (int i=0;i<CurCheque->GetCount();i++)
		{
		    GoodsInfo crGoods = (*CurCheque)[i];

            if (((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT)) || (crGoods.ItemCode==TRUNC)) continue;

            // добавляем информацию по позициям чека в запрос
            IChequeItem* ChequeItem = PCX->CreateChequeItem();

            ChequeItem->SetProduct(Int2Str(crGoods.ItemCode).c_str());
            ChequeItem->SetQuantity(crGoods.ItemCount);
            ChequeItem->SetAmount(Int100(crGoods.ItemSum));

            AuthRq->GetCheque()->AddItem(ChequeItem); // добавляем позицию чека - актикул arc1, количество 3, общая стоимость позиции 9033 копеек
   		}

        Log(" + PCX->AuthPoints_V1 +");
		int RetCode = PCX->AuthPoints_V1(AuthRq, AuthResp, Flag); // списание баллов
        Log(" - PCX->AuthPoints_V1 -");

        Log(" + ParseResult +");
        //Обрабатываем результат, печатаем чек
        ParseResult(RetCode, AuthResp, true);
        Log(" - ParseResult -");
		//===========================

        if (PCX_Result.ReturnCode>=0)
        {

        LastDiscID = PCX_Result.id_tran;
        LastDiscDT = PCX_Result.dt_tran;

          //  EmptyFiscalReg::PrintPCheck(PrintFileName(), false);
        }


    }
    catch(exception& ex)
    {
		// обработка ошибки создания объекта PCX
		string err = ex.what();
		PCX_Result.info = "ERROR: "+err;
		Log(PCX_Result.info.c_str());
	}

    }
	else
	{
	    PCX_Result.info = "ERROR: Объект PCX не инициализирован";
	    Log(PCX_Result.info.c_str());
    }

	return &PCX_Result;

} //Disc


void LProc::SetGoodsParam(IAuthRequest* AuthRq)
{
/*
    if (PCX!=NULL)
    {

    double FullCheckSum, CheckSum;
    CheckSum = FullCheckSum = 0;

	FILE * f = fopen( statefn, "r");
	if(f)
	{
	long long ll_temp;
	double d_temp;
	int i_temp;

	fscanf(f, "Customer=%lld\n", &ll_temp);
	fscanf(f, "AddonCustomer=%lld\n", &ll_temp);
	fscanf(f, "tsum=%lf\n", &d_temp);
	fscanf(f, "trunc=%lf\n", &d_temp);
	fscanf(f, "DiscountSumm=%lf\n", &d_temp);
	fscanf(f, "CertificateSumm=%lf\n", &d_temp);



	int checklen;
	fscanf(f, "count=%d\n", &checklen);
	for(int i = 0; i < checklen; i++)
	{

		char buf[1024];
        string ItemName;
        int ItemCode;
        int ItemFlag;
        double ItemSum, ItemFullSum, ItemCount;

		fgets(buf, f);
		ItemName = buf;

		fgets(buf, f);
//		g.ItemBarcode = buf;

		fgets(buf, f);
//		g.ItemSecondBarcode = buf;

		fgets(buf, f);
//		g.ItemDiscountsDesc = buf;

		fscanf(f, "ItemCode=%d\n", &ItemCode);

		fscanf(f, "ItemNumber=%d\n", &i_temp);
		fscanf(f, "ItemDBPos=%d\n", &i_temp);
		fscanf(f, "ItemDepartment=%d\n", &i_temp);
		fscanf(f, "ItemFlag=%d\n", &ItemFlag);
		fscanf(f, "PaymentType=%d\n", &i_temp);
		fscanf(f, "GuardCode=%d\n", &i_temp);
		fscanf(f, "CustomerCode=%lld\n", &ll_temp);
		fscanf(f, "AddonCustomerCode=%lld\n", &ll_temp);
		fscanf(f, "InputType=%d\n", &i_temp);
		fscanf(f, "DiscFlag=%d\n", &i_temp);
		fscanf(f, "ItemPrice=%lf\n", &d_temp);
		fscanf(f, "ItemFullPrice=%lf\n", &d_temp);
		fscanf(f, "ItemCalcPrice=%lf\n", &d_temp);
		fscanf(f, "ItemFixedPrice=%lf\n", &d_temp);
		fscanf(f, "ItemSum=%lf\n", &ItemSum);
		fscanf(f, "ItemFullSum=%lf\n", &ItemFullSum);
		fscanf(f, "ItemFullDiscSum=%lf\n", &d_temp);
		fscanf(f, "ItemBankDiscSum=%lf\n", &d_temp);
		fscanf(f, "ItemCalcSum=%lf\n", &d_temp);
		fscanf(f, "ItemDiscount=%lf\n", &d_temp);
		fscanf(f, "ItemDSum=%lf\n", &d_temp);
		fscanf(f, "ItemPrecision=%lf\n", &d_temp);
		fscanf(f, "ItemCount=%lf\n", &ItemCount);
		fscanf(f, "tTime=%d\n", &i_temp);
		fscanf(f, "NDS=%lf\n", &d_temp);
		fgets(buf, f);
		fscanf(f, "LockCode=%d\n", &i_temp);

        if(ItemFlag==0)
        {
        ;;Log((" + "+ItemName+" , C= "+Double2Str(ItemCount)+" , S= "+Double2Str(ItemFullSum)).c_str());

		// добавляем информацию по позициям чека в запрос
		IChequeItem* ChequeItem = PCX->CreateChequeItem();
		ChequeItem->SetProduct(Int2Str(ItemCode).c_str());
		ChequeItem->SetQuantity(ItemCount);
		ChequeItem->SetAmount(ItemFullSum*100);

		AuthRq->GetCheque()->AddItem(ChequeItem); // добавляем позицию чека - актикул arc1, количество 3, общая стоимость позиции 9033 копеек


        FullCheckSum+=ItemFullSum;
        CheckSum+=ItemSum;
        }
        }
	}
	else
    {
        FullCheckSum=CheckSum = full_sum/100;

        // добавляем информацию по позициям чека в запрос
		IChequeItem* ChequeItem = PCX->CreateChequeItem();
		ChequeItem->SetProduct("goods");
		ChequeItem->SetQuantity(1.0);
		ChequeItem->SetAmount(full_sum);
		AuthRq->GetCheque()->AddItem(ChequeItem); // добавляем позицию чека - актикул arc1, количество 3, общая стоимость позиции 9033 копеек
	}

    full_sum = FullCheckSum*100;
    bank_sum = full_sum - disc_sum;
    }
*/
}


PCX_RET* LProc::Pay(string a_card, string a_sha1, FiscalCheque* CurCheque, double DiscSumma)
{
    Log(" === Начисление баллов ===");

    InitResult();

    PCX_Result.action = BB_PAY;
    PCX_Result.card = a_card;
    PCX_Result.sha1 = a_sha1;
    PCX_Result.pwd_card = GetPwdCardNumber(a_card);
    PCX_Result.terminal = Terminal;

	if (!CurCheque)
	{
        PCX_Result.info = "ERROR: Не определен параметр CurCheque!";
	    return &PCX_Result;
	}

    // Проверка: Включено начисление или нет
	FILE * flg_pay = fopen("/Kacca/pcx/nopay.flg","r");
	if(flg_pay)
	{
	    fclose(flg_pay);
        PCX_Result.info = "EXIT: Начисление бонусов отключено!";
        Log(PCX_Result.info.c_str());
	    return &PCX_Result;
	}

    if (PCX!=NULL)
    {

    try{
	//============================
		Log("Начисление баллов на карту");

        const char* ClientID = a_sha1.c_str();

 		int ClientIDType = 6; //  для хэша всегда 6
		int Flag = 0; // 0 - обычный режим или 1 - режим force offline


		IAuthRequest* AuthRq = PCX->CreateAuthRequest(); //создаем объект запроса
		IAuthResponse* AuthResp = PCX->CreateAuthResponse(); //создаем объект ответа

		// добавляем расширения в запрос
		IExtension* Extension = PCX->CreateExtension();
		Extension->SetType("cheque_msg_req");
		Extension->SetCritical(true);
		Extension->AddParam("msg_req", "y");
		AuthRq->GetExtensions()->AddItem(Extension);

		Extension = PCX->CreateExtension();
		Extension->SetType("PURCHASE_EXT_PARAMS");
		Extension->SetCritical(true);

		pwd_card=GetPwdCardNumber(a_card);

		Extension->AddParam("PAN4", pwd_card.c_str()); //Передача маскированного ПАНа

        //uuid
        char key[40];
        key[0]=0;
        uuid_t id;
        uuid_generate(id);
        uuid_unparse(id,key);
		;;Log(key);

		Extension->AddParam("RECEIPT", key); //уникальный идентификатор продажи

		AuthRq->GetExtensions()->AddItem(Extension);
//;;Log("== 1 ==");
        PCX_Result.guid = key;
        PCX_Result.bonus_summa = CurCheque->GetBankBonusSum();
        PCX_Result.total_summa = CurCheque->GetSum();

//		PaymentItem = PCX->CreatePaymentItem();
//		PaymentItem->SetPayMeans("C"); // инструмент для суммы оплаты наличными
//		PaymentItem->SetAmount(2227); // сумма оплаты наличными
//		AuthRq->GetPayment()->AddItem(PaymentItem);


        long lBankDiscSumm = Int100(CurCheque->GetBankBonusSum());
        long lCheckSumm = Int100(CurCheque->GetSum());

//;;Log("== 2 ==");

        // Заполним товары
        SetGoodsParam(AuthRq);

//;;Log("== 3 ==");
		AuthRq->SetClientID(ClientID);
		AuthRq->SetClientIDType(ClientIDType);
		AuthRq->SetCurrency("643"); // код валюты - рубли
		AuthRq->SetAmount(lCheckSumm + lBankDiscSumm); // подитог продажи 102.27 р.
//;;Log("== 4 ==");
		// добавляем платежные инструменты в запрос
		IPaymentItem* PaymentItem = PCX->CreatePaymentItem();
		PaymentItem->SetPayMeans("N"); // скидка списанием баллов передается в инструменте "N"
		PaymentItem->SetAmount(lBankDiscSumm); // сумма списания в платежном иструменте 30 р.
		AuthRq->GetPayment()->AddItem(PaymentItem);
//;;Log("== 5 ==");
		PaymentItem = PCX->CreatePaymentItem();
		PaymentItem->SetPayMeans("I"); // инструмент для суммы оплаты по карте Сбербанка
		PaymentItem->SetAmount(lCheckSumm); // сумма оплаты по карте Сбербанка
		AuthRq->GetPayment()->AddItem(PaymentItem);
//;;Log("== 6 ==");
		for (int i=0;i<CurCheque->GetCount();i++)
		{
		    GoodsInfo crGoods = (*CurCheque)[i];

            if (((crGoods.ItemFlag!=SL) && (crGoods.ItemFlag!=RT)) || (crGoods.ItemCode==TRUNC)) continue;

            // добавляем информацию по позициям чека в запрос
            IChequeItem* ChequeItem = PCX->CreateChequeItem();

            ChequeItem->SetProduct(Int2Str(crGoods.ItemCode).c_str());
            ChequeItem->SetQuantity(crGoods.ItemCount);

            ChequeItem->SetAmount(Int100(crGoods.ItemSum)+Int100(crGoods.ItemBankBonusDisc));

            AuthRq->GetCheque()->AddItem(ChequeItem); // добавляем позицию чека - актикул arc1, количество 3, общая стоимость позиции 9033 копеек
   		}

//		ChequeItem = PCX->CreateChequeItem();
//		ChequeItem->SetProduct("arc2");
//		ChequeItem->SetQuantity(2.0);
//		ChequeItem->SetAmount(1134);
//		AuthRq->GetCheque()->AddItem(ChequeItem); // добавляем позицию чека - актикул arc2, количество 1, общая стоимость позиции 1134 копеек
;;Log("+  PCX->AuthPoints_V1 +");
		int RetCode = PCX->AuthPoints_V1(AuthRq, AuthResp, Flag); // начисление баллов
;;Log("-  PCX->AuthPoints_V1 -");

;;Log("+  ParseResult +");
        //Обрабатываем результат, печатаем чек
        ParseResult(RetCode, AuthResp, true);
;;Log("-  ParseResult -");

        if (PCX_Result.ReturnCode>=0)
        {
            //;;Log("== 10 ==");
            PCX_Result.result_summa = PCX_Result.change_bonus;

            //;;Log("PrintFileName");
           // FiscalReg->PrintPCheck(PrintFileName(), false);
        }

    }
    catch(exception& ex)
    {
        //;;Log("== 11 ==");
		// обработка ошибки создания объекта PCX
		string err = ex.what();
		PCX_Result.info = "ERROR: "+err;
		Log(PCX_Result.info.c_str());
	}

    }
	else
	{
	    //;;Log("== 12 ==");
	    PCX_Result.info = "ERROR: Объект PCX не инициализирован";
	    Log(PCX_Result.info.c_str());
    }

    //;;Log("== 13 ==");
	return &PCX_Result;

} //Pay


void LProc::SetSettings()
{

;;Log("= Чтение настроек PCX =");
	FILE * f = fopen( fconfig, "r");
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

		fscanf(f, "PartnerID=%d\n", &PartnerID);

		fscanf(f, "ConnectTimeout=%d\n", &ConnectTimeout);
		fscanf(f, "SendRecvTimeout=%d\n", &SendRecvTimeout);
		fscanf(f, "BkgndFlushPeriod=%d\n", &BkgndFlushPeriod);
		fscanf(f, "FlushTimeout=%d\n", &FlushTimeout);
		fscanf(f, "UsePCX=%d\n", &UsePCX);
		fscanf(f, "UseDiscount=%d\n", &UseDiscount);
		fscanf(f, "UsePay=%d\n", &UsePay);

		fclose(f);
	}

;;Log("= Чтение локальных настроек =");
	f = fopen( flocal, "r");
	if(f)
	{
	    char buf[1024];

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

    fclose(f);
	}
}


//void LProc::SetParam(IPcxCore* PCX)
void LProc::SetParam()
{
    if(PCX!=NULL)
    {

/*
        // объект создан, заполняем свойства
		//PCX->SetConnectionString("https://194->85->126->117:10443/axis->v3/services/CFTLoyaltyPCPoints_SoapPort_term_2->7->6");
		PCX->SetConnectionString("https://194.85.126.117:10443/axis.v3/services/CFTLoyaltyPCPoints_SoapPort_term_2.7.6");

		//PCX->SetProxyHost("192.168.111.1");
		//PCX->SetProxyPort(3128);
		//PCX->SetProxyUserId("trondin");
#ifdef _WIN32
		//PCX->SetCertSubjectName("SUBJ:Petrov Petr Petrovich");
#else
		PCX->SetCertFilePath("./CA.pem");
		PCX->SetKeyFilePath("./my.pem");
		PCX->SetKeyPassword("123456");

#endif
*/
/*
		PCX->SetTerminal("terminal1");
		PCX->SetLocation("location1");
		PCX->SetPartnerID(55555);
		PCX->SetConnectTimeout(4);
		PCX->SetSendRecvTimeout(61);
		PCX->SetBkgndFlushPeriod(301);
		PCX->SetFlushTimeout(271);
*/

/*
		PCX->SetTerminal("tt1term1");
		PCX->SetLocation("tt1");
		PCX->SetPartnerID(248781);
		PCX->SetConnectTimeout(5);
		PCX->SetSendRecvTimeout(30);
		PCX->SetBkgndFlushPeriod(300);
		PCX->SetFlushTimeout(240);
*/



        // объект создан, заполняем свойства
		PCX->SetConnectionString(ConnectionString.c_str());

		//PCX->SetProxyHost("192.168.111.1");
		//PCX->SetProxyPort(3128);
		//PCX->SetProxyUserId("trondin");

		PCX->SetCertFilePath(CertFilePath.c_str());
		PCX->SetKeyFilePath(KeyFilePath.c_str());
		PCX->SetKeyPassword(KeyPassword.c_str());

		PCX->SetTerminal(Terminal.c_str());
		PCX->SetLocation(Location.c_str());
		PCX->SetPartnerID(PartnerID);
		PCX->SetConnectTimeout(ConnectTimeout);
		PCX->SetSendRecvTimeout(SendRecvTimeout);
		PCX->SetBkgndFlushPeriod(BkgndFlushPeriod);
		PCX->SetFlushTimeout(FlushTimeout);

    }

}


void LProc::Destroy()
{
    Log("\n");
    Log("-------------------------------- FINALIZE PCX ------------------------------------");
    Log("Finalize PCX...");

    if (PCX!=NULL)
    {

		//******************************************
		// При завеошении работы кассового ПО необходимо
		// корректно завершить работу объекта PCX
		int RetCode = PCX->Finalize();
		if(RetCode != 0){
			// обработка неуспешности вызова Finalize
			Log(("Ошибка вызова Finalize, ErrorCode: " + Int2Str(RetCode) + ", \"" + PCX->GetErrorMessage(RetCode) + "\", \"" + PCX->GetErrorInfo(RetCode)+ "\"").c_str());
		}

		PCX=NULL;
		Log(" SUCCESS");
    }

//    if( cd != (iconv_t)(-1) )
//    {
//        iconv_close(cd);
//    }
    Log("----------------------------------- END PCX ---------------------------------------");
    Log("===================================================================================\n");

}

int LProc::ReturnPay()
{
        return 0;
}

int LProc::ReturnDisc(string a_card, string a_sha1, string tran_id, double retsum)
{
    int res=0;

    Log("=== Возврат списания баллов с карты ===");

    PCX_Result.action = BB_RETURN_DISCOUNT;
    PCX_Result.card = a_card;
    PCX_Result.sha1 = a_sha1;
    PCX_Result.pwd_card = GetPwdCardNumber(a_card);
    PCX_Result.terminal = Terminal;

    PCX_Result.rettran_id = tran_id;

    if (PCX!=NULL)
    {

    try{

	//============================
		// Возврат cписания баллов ("Спасибо")

       const char* ClientID = a_sha1.c_str();

		//const char*  ClientID = "b2c9c2d511cc2d495a7727e427efbd0ce1535b8c"; // хэш SHA1

		int ClientIDType = 6; // для хэша всегда 6
		int Flag = 0; // 0 - обычный режим или 1 - режим force offline


		IRefundRequest* RefundRq = PCX->CreateRefundRequest(); //создаем объект запроса
		IRefundResponse* RefundResp = PCX->CreateRefundResponse(); //создаем объект ответа

		// добавляем расширения в запрос
		IExtension* Extension = PCX->CreateExtension();
		Extension->SetType("cheque_msg_req");
		Extension->SetCritical(true);
		Extension->AddParam("msg_req", "y");
		RefundRq->GetExtensions()->AddItem(Extension);

		Extension = PCX->CreateExtension();
		Extension->SetType("PURCHASE_EXT_PARAMS");
		Extension->SetCritical(true);

        pwd_card = GetPwdCardNumber(a_card);

		Extension->AddParam("PAN4", pwd_card.c_str()); // Передача маскированного ПАНа

        //uuid
        char key[40];
        key[0]=0;
        uuid_t id;
        uuid_generate(id);
        uuid_unparse(id,key);
        Log(key);

		//Extension->AddParam("RECEIPT", "b8e83d7e-b8b1-4653-939c-46e52b4f1d46"); // уникальный идентификатор продажи
		Extension->AddParam("RECEIPT", key); // уникальный идентификатор продажи

        PCX_Result.guid = key;
        PCX_Result.result_summa = retsum;

		RefundRq->GetExtensions()->AddItem(Extension);

		RefundRq->SetClientID(ClientID);
		RefundRq->SetClientIDType(ClientIDType);
		RefundRq->SetAmount( Int100(retsum) );
		RefundRq->SetCurrency("643");

		// задаем параметры оригинальной операции
		RefundRq->SetOrigID( tran_id.c_str() ); // Id транзакции списания, по которой выполняем возврат
		RefundRq->SetOrigTerminal(Terminal.c_str()); // задаем Id терминала
		RefundRq->SetOrigLocation(Location.c_str()); // задаем Id точки обслуживания
		RefundRq->SetOrigPartnerID(PartnerID); // задаем id торговой организации

		// добавляем платежный инструмент в запрос
		IPaymentItem* PaymentItem = PCX->CreatePaymentItem();
		PaymentItem->SetPayMeans("P"); // для списания баллов используется инструмент "P"
		PaymentItem->SetAmount( Int100(retsum) ); // сумма списания в платежном иструменте 30 р.
		RefundRq->GetPayment()->AddItem(PaymentItem);

		int RetCode = PCX->Refund_V1(RefundRq, RefundResp, Flag); // Возврат

        //Обрабатываем результат, печатаем чек
        ParseResult(RetCode, RefundResp, true);

    }
    catch(exception& ex)
    {
		// обработка ошибки создания объекта PCX
		Log("   ERROR:");
		Log(ex.what());
		return false;
	}

    }
	else
	{
	    Log("ERROR: Объект PCX не инициализирован");
		//return -1;
		return 0;
    }

	return res;

} //Disc


int LProc::Reverse(string a_card, string a_sha1, FiscalCheque* CurCheque, double DiscSumma)
{
    int res=0;

    Log("=== Реверс операции ===");

    PCX_Result.action = BB_REVERSE;
    PCX_Result.card = a_card;
    PCX_Result.sha1 = a_sha1;
    PCX_Result.pwd_card = GetPwdCardNumber(a_card);
    PCX_Result.terminal = Terminal;

    PCX_Result.rettran_id = LastDiscID;

    if (PCX!=NULL)
    {

    try{

       const char* ClientID = a_sha1.c_str();

		//const char*  ClientID = "b2c9c2d511cc2d495a7727e427efbd0ce1535b8c"; // хэш SHA1

		int ClientIDType = 6; // для хэша всегда 6
		//int Flag = 0; // 0 - обычный режим или 1 - режим force offline

//        a_tran = PCX_Result.id_tran;
//        a_dttran = PCX_Result.dt_tran;

        a_tran = LastDiscID;
        a_dttran = LastDiscDT;

        ITransaction* Transaction = PCX->CreateTransaction();

        Transaction->SetID(a_tran.c_str());
        Transaction->SetTerminal(Terminal.c_str());
        Transaction->SetLocation(Location.c_str());
        Transaction->SetClientID(ClientID);
        Transaction->SetClientIDType(ClientIDType);
        Transaction->SetDateTime(a_dttran.c_str());

		// добавляем расширения в запрос
		IExtension* Extension = PCX->CreateExtension();
		Extension->SetType("cheque_msg_req");
		Extension->SetCritical(true);
		Extension->AddParam("msg_req", "y");
		Transaction->GetExtensions()->AddItem(Extension);

		Extension = PCX->CreateExtension();
		Extension->SetType("PURCHASE_EXT_PARAMS");
		Extension->SetCritical(true);


        pwd_card = GetPwdCardNumber(a_card);


		Extension->AddParam("PAN4", pwd_card.c_str()); // Передача маскированного ПАНа

        //uuid
        char key[40];
        key[0]=0;
        uuid_t id;
        uuid_generate(id);
        uuid_unparse(id,key);
        Log(key);

		//Extension->AddParam("RECEIPT", "b8e83d7e-b8b1-4653-939c-46e52b4f1d46"); // уникальный идентификатор продажи
		Extension->AddParam("RECEIPT", key); // уникальный идентификатор продажи

        PCX_Result.guid = key;
        PCX_Result.total_summa = CurCheque->GetSum();
        PCX_Result.result_summa = DiscSumma;

        Transaction->GetExtensions()->AddItem(Extension);

        Log(" Run Reverse ...");
        int RetCode = PCX->Reverse(Transaction, 0);

        PCX_Result.ReturnCode = RetCode;

        if (RetCode>=0)
        {
            Log(" ... SUCCESS");


            ErrLog(0, "Успешно "+(!RetCode)?"(online)":"(offline)", false);

            string ChequeMessage = "\
Спасибо от Сбербанка\n\
Отмена последней операции\n\
списания \"Спасибо\" "+GetRounded(DiscSumma, 0.01)+ "\n\n";

//Отменено списание "+Double2Str(disc_sum*.01)+" \"Спасибо\"\n\

            FILE *fr = fopen(p_file, "w");
            if(fr)
            {
                fprintf(fr, FormatCheck(ChequeMessage.c_str()), false);
                fclose(fr);
                Log(FormatCheck(ChequeMessage.c_str()));
                Log("Чек сформирован. ");
                PCX_Result.info = "OK: Чек сформирован.";
            }
            else
            {
                //ErrLog(-1, "Ошибка формирования чека. Файл не сформирован!", true);
                Log("  Ошибка формирования чека!");
                PCX_Result.info = "ERROR:   Ошибка формирования чека!";
            }

        }
		else
		{
			//запрос не успешен

			Log(" ... ERROR");

            string ErrorMessage = convert(cd, PCX->GetErrorMessage(RetCode));
            string ErrorInfo = convert(cd, PCX->GetErrorInfo(RetCode));

            ErrLog(RetCode, ("Запрос завершен с ошибкой, ErrorCode: "+Int2Str(RetCode)).c_str(), false);
            ErrLog(1, ErrorMessage.c_str(), true);
            ErrLog(1, ErrorInfo.c_str(), true);

            Log(("Запрос завершен с ошибкой, ErrorCode: "+Int2Str(RetCode)).c_str());
            Log(("ErrorMessage="+ErrorMessage).c_str());
            Log(("ErrorInfo="+ErrorInfo).c_str());

            PCX_Result.info = "ERROR: "+ErrorMessage+" "+ErrorInfo;
		}

    }
    catch(exception& ex)
    {
		// обработка ошибки создания объекта PCX
		string err = ex.what();
		PCX_Result.info = "ERROR: "+err;
		Log(PCX_Result.info.c_str());
		return -1;
	}

    }
	else
	{
	    PCX_Result.info = "ERROR: Объект PCX не инициализирован";
	    Log("ERROR: Объект PCX не инициализирован");
		//return -1;
		return -1;
    }

	return res;

} //Disc



PCX_RET* LProc::exec(string a_oper, string a_card, string a_sha1, FiscalCheque* CurCheque, double a_sum, string tran_id)
{

    Log("===  START EXEC PCX === ");
//Тест
//a_sha1 = "CB073E388D292E653B9C360B45A2B3ECBC05F054";


#ifdef LINUX_BUILD
	setlocale( LC_ALL, "ru_RU->utf8");
#endif

    cd = iconv_open("cp1251", "utf8");


        InitResult();


    // Проверка: Включено "Спасибо" или нет
	FILE * flg_pay = fopen("/Kacca/pcx/nopcx.flg","r");
	if(flg_pay)
	{
	    fclose(flg_pay);
        PCX_Result.info = "EXIT: Программа Спасибо от сбербанка отключена !";
        Log(PCX_Result.info.c_str());
	    return &PCX_Result;
	}


/*
	if (!CurCheque)
		CurCheque = Cheque;
	if (!CurCheque) return;

	FiscalCheque *tmpCheque;
	tmpCheque = new FiscalCheque;

*/

    string Version = "Pcx 1.00 test";

    /////////////////////////////////

	char pfile[]="/Kacca/pcx/p";
	char efile[]="/Kacca/pcx/e";

    remove(efile);
    remove(pfile);

    //Логи забивают место
    remove("/Kacca/TEST.log");
    remove("/Kacca/RECV.log");
    remove("/Kacca/SENT.log");

    string LogStr="";

    LogStr = "------------------------- "+Version+" -------------------------------";
    Log(LogStr.c_str());

    //InitResult();
    PCX_Result.ReturnCode = -1;
    PCX_Result.ReturnCode = -1;

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


    a_dttran="";

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
        return &PCX_Result;
    }

    disc_sum = Int100(a_sum);       // Сумма баллов "Спасибо"
    //bank_sum = Str2Int(a_chsum);     // Сумма оплаты по банку (сумма чека)
    //full_sum = bank_sum + disc_sum;  // Сумма чека без скидки "Спасибо"

    a_track2= a_card;

    pwd_card = GetPwdCardNumber(a_card);

    string str_track2 = SetPwdNumber(a_track2);

   // if (a_sha1.length()<5) a_sha1 = GetSHA1(a_card); /*хэш SHA1*/

//
        ;;Log("=== Параметры ===");

        ;;Log(("  Операция: "+a_oper).c_str());
        ;;Log(("  Карта: "+a_card).c_str());
        ;;Log(("  Чек # "+a_check).c_str());
        ;;Log(("  Сумма: "+Double2Str(a_sum)).c_str());
        ;;Log(("  Сумма чека: "+a_chsum).c_str());
        ;;Log(("  Хэш: "+a_sha1).c_str());
        if (a_tran.length()<5) ;;Log(("  ID транзакции возврата: "+tran_id).c_str());
       // if (a_dttran.length()<5) ;;Log(("  DateTime транзакции: "+a_dttran).c_str());



  //  Init();

    Log("Check PCX...");
    if (PCX!=NULL)
    {// Инициализация прошла успешно

        LogStr = " ..SUCCESS";
        Log(LogStr.c_str());


        string Verlib = PCX->GetVersion();

        if ((a_oper == "-v")||(a_oper == "-V")||
            (a_oper == "-ver")||(a_oper == "-Ver")||(a_oper == "-VER")
           )
        {

            // если нужно получить версию программы
            printf("Version: %s\n", Version.c_str());
            printf("Library: %s\n", Verlib.c_str());
            ErrLog(1,(char*)("Версия: "+Version+"\n"+"Версия библиотекм: "+Verlib).c_str());
            Log("------------------------------------------------------------------------------");
            return &PCX_Result;
        }

        Log(("  Library Version:"+ Verlib).c_str());

        string str_track2 = GetPwdCardNumber(a_track2);

        //a_sha1  = GetSHA1(a_card); /*хэш SHA1*/


        if ((a_oper=="inf") && (UsePCX))
        {
            GetCardInfo(a_card,  a_sha1);
        }

        if((a_oper=="disc") && (UseDiscount))
        {
            Discount(a_card, a_sha1, CurCheque, a_sum);
        }

        if((a_oper=="pay") && (UsePay))
        {
            Pay(a_card, a_sha1, CurCheque, a_sum);
        }

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
    }
    else
    {
            Log("  ... ERROR (Not INIT)");
    }

    if( cd != (iconv_t)(-1) )
    {
        iconv_close(cd);
    }

    Log("===  END EXEC PCX === ");

	return &PCX_Result;

} //exec


