/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "Utils.h"
#include "Cash.h"
#include "Ping.h"

const int TIMEOUT=3000;  //max timeout to wait for response (in milliseconds)
const int PortNumber=65001;
const char *achBCastAddr = "255.255.255.255";
const char *UpMsg="UP",*DownMsg="DOWN",*MessageMsg="MESSAGE";

#ifdef _WS_WIN_

typedef struct ip_option_information
{
	u_char Ttl;		// Time To Live (used for traceroute)
	u_char Tos; 	// Type Of Service (usually 0)
	u_char Flags; 	// IP header flags (usually 0)
	u_char OptionsSize; // Size of options data (usually 0, max 40)
	u_char FAR *OptionsData;   // Options data buffer
} IPINFO, *PIPINFO, *LPIPINFO;

typedef struct icmp_echo_reply
{
	u_long Address; 	// source address
	u_long Status;	// IP status value (see below)
	u_long RTTime;	// Round Trip Time in milliseconds //
	u_short DataSize; 	// reply data size
	u_short Reserved; 	//
	void FAR *Data; 	// reply data buffer
	ip_option_information Options; // reply options
} ICMPECHO, *PICMPECHO, *LPICMPECHO;

DWORD WINAPI IcmpSendEcho
(
	HANDLE IcmpHandle, 	// handle returned from IcmpCreateFile()
	u_long DestAddress, // destination IP address (in network order)
	LPVOID RequestData, // pointer to buffer to send
	WORD RequestSize,	// length of data in buffer
	LPIPINFO RequestOptns,  // see Note 2
	LPVOID ReplyBuffer, // see Note 1
	DWORD ReplySize, 	// length of reply (must allow at least 1 reply)
	DWORD Timeout 	// time in milliseconds to wait for reply
);

#define IP_SUCCESS 0

#define BUFSIZE     8192
#define DEFAULT_LEN 32
#define LOOPLIMIT   4
#define DEFAULT_TTL 64

char achReqData[BUFSIZE];
char achRepData[sizeof(ICMPECHO) + BUFSIZE];

HANDLE hICMP_DLL=NULL;

typedef HANDLE (__stdcall *pIcmpCreateFile)(void);
typedef void (__stdcall *pIcmpCloseHandle)(HANDLE);
typedef int (__stdcall *pIcmpSendEcho)(HANDLE,u_long,char*,int,IPINFO*,char*,int,int);

pIcmpCreateFile lpfnIcmpCreateFile=NULL;
pIcmpCloseHandle lpfnIcmpCloseHandle=NULL;
pIcmpSendEcho lpfnIcmpSendEcho=NULL;

#else
#define	DEFDATALEN	(64 - 8)	// default data length
#define	MAXIPLEN	60
#define	MAXICMPLEN	76

#define	A(bit)		rcvd_tbl[(bit)>>3]	// identify byte in array
#define	B(bit)		(1 << ((bit) & 0x07))	// identify bit in byte
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

// MAX_DUP_CHK is the number of bits in received table, i.e. the maximum
// number of received sequence numbers we can keep track of.  Change 128
// to 8192 for complete accuracy...

#define	MAX_DUP_CHK	(8 * 128)
int mx_dup_ck = MAX_DUP_CHK;
char rcvd_tbl[MAX_DUP_CHK / 8];

sockaddr_in whereto;	// who to ping
unsigned int datalen = DEFDATALEN, optlen = 0;
int icmp_sock;			// socket file descriptor
unsigned char outpack[0x10000];
char *hostname;
int uid, ident;
bool result=false;

// counters
long nreceived;			// # of packets we got back
long nrepeats;			// number of duplicates
long ntransmitted;		// sequence # for outbound packets = #sent
long nchecksum;			// replies with bad checksum
long nerrors;			// icmp errors
int interval = 1000;	// interval between packets (msec)
time_t starttime;
int confirm = 0, confirm_flag = 0;

long tmax;			// maximum round trip time

void tvsub(timeval *, timeval *);
void set_signal(int,void(*)(int));
void catcher(int);
void finish(int);
void pinger(void);
unsigned short in_cksum(const unsigned short *, int , unsigned short );
bool pr_pack(char*, unsigned int , sockaddr_in *, timeval *);

struct
{
	cmsghdr cm;
	in_pktinfo ipi;
} cmsg = { {sizeof(cmsghdr) + sizeof(in_pktinfo), SOL_IP, IP_PKTINFO}, {0, }};
int cmsg_len;

sockaddr_in source;
char *device;

void tvsub(timeval *out, timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) < 0)
	{
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

void set_signal(int signo, void (*handler)(int))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));

	sa.sa_handler = (void (*)(int))handler;
	sigaction(signo, &sa, NULL);
}

void catcher(int signo)
{
	int waittime;

	if (ntransmitted == 0)
		waittime = interval*1000;
	else
	{
		if (nreceived)
		{
			waittime = 2 * tmax;
			if (waittime<1000000)
				waittime = 1000000;
		}
		else
			waittime = TIMEOUT*1000;
		set_signal(SIGALRM, finish);
	}

	itimerval it;

	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 0;
	it.it_value.tv_sec = waittime/1000000;
	it.it_value.tv_usec = waittime%1000000;

	setitimer(ITIMER_REAL, &it, NULL);
}

void pinger()
{
	icmphdr *icp;
	int cc, i;

	icp = (icmphdr *)outpack;
	icp->type = ICMP_ECHO;
	icp->code = 0;
	icp->checksum = 0;
	icp->un.echo.sequence = ntransmitted++;
	icp->un.echo.id = ident;			// ID

	CLR(icp->un.echo.sequence % mx_dup_ck);

	cc = datalen + 8;			// skips ICMP portion

	// compute ICMP checksum here
	icp->checksum = in_cksum((unsigned short *)icp, cc, 0);

	for (;;)
	{
		static iovec iov = {outpack, 0};
		static msghdr m = { &whereto, sizeof(whereto), &iov, 1, &cmsg, 0, 0 };
		m.msg_controllen = cmsg_len;
		iov.iov_len = cc;

		i = sendmsg(icmp_sock, &m, confirm);
		confirm = 0;

		if (i<0 && confirm_flag)
			confirm_flag = 0;
		else
			break;
	}

	if ( (i < 0) && (errno == EAGAIN) )
		ntransmitted--;
}

bool pr_pack(char *buf, unsigned int cc, sockaddr_in *from, timeval *tv)
{
	icmphdr *icp;
	unsigned char *cp,*dp;
	iphdr *ip;
	unsigned int hlen, dupflag = 0;
	int csfailed;

	// Check the IP header
	ip = (iphdr *)buf;
	hlen = ip->ihl*4;
	if (cc < hlen + 8 || ip->ihl < 5)
		return false;

	// Now the ICMP part
	cc -= hlen;
	icp = (icmphdr *)(buf + hlen);
	csfailed = in_cksum((unsigned short *)icp, cc, 0);

	if (icp->type == ICMP_ECHOREPLY)
	{
		if (icp->un.echo.id != ident)
			return false;			// it was not our ECHO

		++nreceived;

		if (csfailed)
		{
			++nchecksum;
			--nreceived;
		}
		else
			if (TST(icp->un.echo.sequence % mx_dup_ck))
			{
				++nrepeats;
				--nreceived;
				dupflag = 1;
			}
			else
				SET(icp->un.echo.sequence % mx_dup_ck);

		confirm = confirm_flag;

		unsigned int i;

		if (cc < datalen+8)
			return false;

		// check the data
		cp = ((unsigned char*)(icp + 1) + sizeof(timeval));
		dp = &outpack[8 + sizeof(timeval)];
		for (i = sizeof(timeval); i < datalen; ++i, ++cp, ++dp)
		{
			if (*cp != *dp)
			{
				cp = (unsigned char*)(icp + 1) + sizeof(timeval);
					for (i = sizeof(timeval); i < datalen; ++i, ++cp);
				break;
			}
		}
	}
	else
	{
		switch (icp->type)
		{
			case ICMP_ECHO:
				return false;
			case ICMP_SOURCE_QUENCH:
			case ICMP_DEST_UNREACH:
			case ICMP_TIME_EXCEEDED:
			case ICMP_PARAMETERPROB:
			{
				iphdr * iph = (iphdr *)(&icp[1]);
				icmphdr *icp1 = (icmphdr*)((unsigned char *)iph + iph->ihl*4);
				bool error_pkt;
				if (cc < 8+sizeof(iphdr)+8 || cc < 8+iph->ihl*4+8)
					return false;
				if (icp1->type != ICMP_ECHO || iph->saddr != ip->daddr ||
				    icp1->un.echo.id != ident)
					return false;
				error_pkt = (icp->type != ICMP_REDIRECT && icp->type != ICMP_SOURCE_QUENCH);
				nerrors+=error_pkt;
				return !error_pkt;
			}
			default:break;
		}
		return false;
	}

	return false;
}

unsigned short in_cksum(const unsigned short *addr, int len, unsigned short csum)
{
	int nleft = len;
	const unsigned short *w = addr;
	unsigned short answer;
	int sum = csum;

	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1)
		sum += htons(*(unsigned char *)w << 8);

	sum = (sum >> 16) + (sum & 0xffff);	// add hi 16 to low 16
	sum += (sum >> 16);			// add carry
	answer = ~sum;				// truncate to 16 bits
	return answer;
}

void finish(int signo)
{
	set_signal(SIGALRM, SIG_IGN);
	shutdown(icmp_sock,2);
	close(icmp_sock);
	result = nreceived!=0;
	nreceived=0;
}

#endif

SocketThread::SocketThread(CashRegister *p)
{
#ifdef _WS_WIN_
	WSADATA wsaData;

	hICMP_DLL = LoadLibrary("ICMP.DLL");//get functions from icmp dll

	if (hICMP_DLL != NULL)
	{
		lpfnIcmpCreateFile=(pIcmpCreateFile)GetProcAddress((HMODULE)hICMP_DLL,"IcmpCreateFile");
		lpfnIcmpCloseHandle=(pIcmpCloseHandle)GetProcAddress((HMODULE)hICMP_DLL,"IcmpCloseHandle");
		lpfnIcmpSendEcho=(pIcmpSendEcho)GetProcAddress((HMODULE)hICMP_DLL,"IcmpSendEcho");
	}

	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	CashReg=p;
	stop_flag=false;
}

SocketThread::~SocketThread(void)
{
#ifdef _WS_WIN_
	WSACleanup();
	FreeLibrary((HMODULE)hICMP_DLL);
#endif
	stop_flag=true;
	SendMessage("127.0.0.1","\0");
	while(running());
}

bool SocketThread::TestConnect(const char *host)
{
#ifdef _WS_WIN_
	IPINFO stIPInfo, *lpstIPInfo;
	LPHOSTENT lpstHost = NULL;
	HANDLE hICMP;
	int i, j, nDataLen, nLoopLimit, nTimeOut, nTTL, nTOS;
	DWORD dwReplyCount;
	IN_ADDR stDestAddr;

	if (strlen(host)==0)
		return true;

	// Evaluate string provided to get destination address
	u_long lAddr;

	// Is the string an address?
	lAddr = inet_addr(host);
	if (lAddr == INADDR_NONE)
	{
		// Is it a limited broadcast address?
		for (i=0;i<16;i++)
		{
			if (host[i] != achBCastAddr[i])
			{
				lAddr = 0L;
				break;
			}
		}
	}
	if (!lAddr)
	{
		// Or, is it a hostname?
		lpstHost = gethostbyname(host);
		if (!lpstHost)
		{
			WSACleanup();
			return false;
		}
		else // Hostname was resolved, save the address
			stDestAddr.s_addr = *(u_long*)lpstHost->h_addr;
	}
	else
		stDestAddr.s_addr = lAddr;

	// Initialize default settings
	nDataLen   = DEFAULT_LEN;
	nLoopLimit = LOOPLIMIT;
	nTimeOut   = TIMEOUT;
	lpstIPInfo = NULL;
	nTTL       = DEFAULT_TTL;
	nTOS       = 0;

	if (hICMP_DLL == NULL)
		return false;

	if ((lpfnIcmpCreateFile==NULL)||(lpfnIcmpCloseHandle==NULL)||(lpfnIcmpSendEcho==NULL))
		return false;

	// IcmpCreateFile() - Open the ping service
	hICMP = lpfnIcmpCreateFile();
	if (hICMP == INVALID_HANDLE_VALUE)
		return false;


	// Init data buffer printable ASCII 32 (space) through 126 (tilde)
	for (j=0, i=32; j<nDataLen; j++, i++)
	{
		if (i>=126)
			i= 32;
		achReqData[j] = i;
	}

	// Init IPInfo structure
	lpstIPInfo = &stIPInfo;
	stIPInfo.Ttl      = nTTL;
	stIPInfo.Tos      = nTOS;
	stIPInfo.Flags    = 0;
	stIPInfo.OptionsSize = 0;
	stIPInfo.OptionsData = NULL;

	// Ping
	dwReplyCount = lpfnIcmpSendEcho(hICMP,stDestAddr.s_addr,achReqData,nDataLen,lpstIPInfo,
									achRepData,sizeof(achRepData),nTimeOut);

	bool result;
	if ((dwReplyCount == 0)||(*(DWORD *) &(achRepData[4]) != IP_SUCCESS))
		result=false;
	else
		result=true;

	// IcmpCloseHandle - Close the ICMP handle
	lpfnIcmpCloseHandle(hICMP);

	return result;

#else
	return true;

	// простестировать пинг!!!
	hostent *hp;
	unsigned int i;
	int hold, packlen;
	unsigned char *packet;
	char target[strlen(host)+1], hnamebuf[MAXHOSTNAMELEN];
	iovec iov;

	if (strlen(host)==0)
		return true;

	icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	source.sin_family = AF_INET;

	strcpy(target,host);

	memset((char *)&whereto,0, sizeof(whereto));
	whereto.sin_family = AF_INET;
	if (inet_aton(target, &whereto.sin_addr) == 1)
		hostname = target;
	else
	{
		hp = gethostbyname(target);
		if (!hp)
			return false;
		memcpy(&whereto.sin_addr, hp->h_addr, 4);
		strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
		hnamebuf[sizeof(hnamebuf) - 1] = 0;
		hostname = hnamebuf;
	}

	if (icmp_sock < 0)
		return false;

	if (bind(icmp_sock, (sockaddr*)&source, sizeof(source)) == -1)

		return false;

	// Set sndbuf to value, not allowing to queue more than two
	// packets. It is very rough hint, certainly.
	hold = datalen + 8;
	hold += ((hold+511)/512)*(optlen + 20 + 128);
	setsockopt(icmp_sock, SOL_SOCKET, SO_SNDBUF, (char *)&hold, sizeof(hold));

	hold = 65535;
	setsockopt(icmp_sock, SOL_SOCKET, SO_RCVBUF, (char *)&hold, sizeof(hold));

	if (datalen > 0xFFFF - 8 - optlen - 20)
		if (uid || datalen > sizeof(outpack)-8)
			return false;
		// Allow small oversize to root yet. It will cause EMSGSIZE.

	packlen = datalen + MAXIPLEN + MAXICMPLEN;
	packet = (unsigned char *) new unsigned char[packlen];
	if (packet == NULL)
		return false;

	unsigned char *p = outpack+8;

	// Do not forget about case of small datalen, fill timestamp area too!
	for (i = 0; i < datalen; ++i)
		*p++ = i;

	ident = getpid() & 0xFFFF;

	timeval tv;//set time interval between packets???
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (interval < 1000)
	{
		tv.tv_sec = 0;
		tv.tv_usec = interval%1000;
	}
	setsockopt(icmp_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));

	set_signal(SIGALRM, catcher);

	pinger();

	catcher(0);		// start things going

	starttime = time(NULL);
	iov.iov_base = (char *)packet;

	char ans_data[128];
	sockaddr_in from;
	msghdr msg = {(void*)&from, sizeof(from),&iov,1,ans_data,sizeof(ans_data),};
	int cc;

	for (;;)
	{
		iov.iov_len = packlen;

		if ((cc = recvmsg(icmp_sock, &msg, 0)) >= 0)
		{
			timeval *recv_timep = NULL;
			timeval recv_time;

			if (recv_timep == NULL)
			{
				if (ioctl(icmp_sock, SIOCGSTAMP, &recv_time))
					gettimeofday(&recv_time, NULL);
				recv_timep = &recv_time;
			}
			pr_pack((char *)packet, cc, &from, recv_timep);
		}
		break;
	}

	finish(0);

	bool tmp=result;
	result=false;
	delete packet;
	return tmp;

#endif
}

int SocketThread::SendMessage(const char* host,const char* msg)
{
  int sock,rc;
  sockaddr_in addr;
  hostent *hp;
  sock=socket(AF_INET,SOCK_DGRAM,0);
  if (sock==-1)
    return -1;
  addr.sin_family=AF_INET;
  addr.sin_port=htons(PortNumber);

	unsigned long lAddr=inet_addr(host);

	if (lAddr == INADDR_NONE)
		for (int i=0;i<16;i++)// Is it a limited broadcast address?
			if (host[i] != achBCastAddr[i])
			{
				lAddr = 0L;
				break;
			}

	if (lAddr==0) // Or, is it a hostname?
	{
		hp = gethostbyname(host);
		if (!hp)
			return -1;
		else // Hostname was resolved, save the address
			memcpy(&addr.sin_addr.s_addr,hp->h_addr,hp->h_length);
	}
	else
		addr.sin_addr.s_addr = lAddr;

  rc=sendto(sock,msg,strlen(msg)+1,0,(const sockaddr *)&addr,sizeof(sockaddr_in));
  close(sock);
  return rc;
}

void SocketThread::run(void)
{
  int sock,rc;
  sockaddr_in addr;
  char buffer[1024];

  sock=socket(AF_INET,SOCK_DGRAM,0);
  if (sock==-1)
    return;

  addr.sin_family=AF_INET;
  addr.sin_port=htons(PortNumber);
  addr.sin_addr.s_addr=htonl(INADDR_ANY);

  rc=bind(sock,(sockaddr *)&addr,sizeof(sockaddr_in));
  if (rc==0)
	while(!stop_flag)
			if (recvfrom(sock,buffer,1024,0,NULL,NULL)>0)
			{
				string recv_text=buffer;
				if (recv_text.find(":")!=recv_text.npos)
				{
					string msg=recv_text.substr(0,recv_text.find(":"));
					if (msg==MessageMsg)
						postEvent(CashReg,new MessageEvent(recv_text.substr(recv_text.find(":")+1)));
					if (msg==UpMsg)
						postEvent(CashReg,new QCustomEvent(UpEventType));
					if (msg==DownMsg)
						postEvent(CashReg,new QCustomEvent(DownEventType));
				}
			}

  close(sock);
}

MessageEvent::MessageEvent(string buf):QCustomEvent(MessageEventType)
{
	buffer=buf;//store barcode string in the internal buffer
	setData(&buffer);
}

// попытка установить соединение с сервером telnet
//!int SocketThread::TestBank(const char* host, int port, unsigned short timeout=0)
int SocketThread::TestBank(const char* host, int port, unsigned short timeout)
{


  int sock;
  sockaddr_in addr;
  hostent *hp;
  sock=socket(AF_INET,SOCK_STREAM,0);
  if (sock==-1)
    return -1;
  addr.sin_family=AF_INET;
  addr.sin_port=htons(port);

 	unsigned long lAddr=inet_addr(host);

	if (lAddr == INADDR_NONE)
		for (int i=0;i<16;i++)// Is it a limited broadcast address?
			if (host[i] != achBCastAddr[i])
			{
				lAddr = 0L;
				break;
			}

	if (lAddr==0) // Or, is it a hostname?
	{
		hp = gethostbyname(host);
		if (!hp)
			return -1;
		else // Hostname was resolved, save the address
			memcpy(&addr.sin_addr.s_addr,hp->h_addr,hp->h_length);
	}
	else
		addr.sin_addr.s_addr = lAddr;

  int retval;

  // rfds
  fd_set rfds;


  char tmpstr[50];
  strcpy(tmpstr,"");

  // если задан таймаут соединения, то работаем с сокетом в НЕ блочном режиме (с таймаутом соединения)
  // иначе - в обычном (без таймаута)
  if(timeout)
  {
      // переводим сокет в неблочный режим
      int flags = fcntl(sock, F_GETFL, 0);
      fcntl(sock, F_SETFL, flags | O_NONBLOCK);
      // установка таймаута
      struct timeval tv;
      tv.tv_sec=timeout;// тайм-аут в секундах
      tv.tv_usec=0;// милисекунды

      FD_ZERO(&rfds);
      FD_SET(sock,&rfds);

      strcat(tmpstr,"Non-block ");

      // пытаемся соединится с сервером в НЕ БЛОЧНОМ режиме
      if(connect(sock, (const sockaddr *)&addr, sizeof(sockaddr_in))!=0)
      {
        if(errno!=EINPROGRESS) // если EINPROGRESS то это не ошибка
        {
          return -1;
        }
      }

      // ждем ответа от сервера (программа блокируется на время таймаута)
      if(retval = select(sock+1, &rfds, NULL, NULL, &tv) == -1)
      {
        return -1; // ошибка
      }
  }

  strcat(tmpstr,"Bank connecting ...");

  // повторный запрос статуса сединения (или "Установлено" или "В процессе" - значит вышел таймаут)
  if(connect(sock, (const sockaddr *)&addr, sizeof(sockaddr_in))==0)
  {
    close(sock);
    //Log(strcat(tmpstr," Success"));
    return 0; // соединение установлено
  }
  else
  {
    close(sock);
    Log(strcat(tmpstr," Error (-1)"));
    return -1; // соединение не было установлено, либо вышел тайм-аут соединения
  }


  // if sockfd descriptor is in rfds set
  // then server answered for our request
  // otherwise time out occured
  if(FD_ISSET(sock, &rfds))
  {
      return 0;
  }
  else
  {
    return -1; // time-out
  }


  //setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&_timeout, sizeof(int) );
  //setsockopt(sock, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg) );

  // проверка соединения
 // if (connect(sock, (const sockaddr *)&addr, sizeof(sockaddr_in)) == -1) return -1;

  //rc=sendto(sock,msg,strlen(msg)+1,0,(const sockaddr *)&addr,sizeof(sockaddr_in));

  close(sock);
  return 0;
}
