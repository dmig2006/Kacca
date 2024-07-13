#include <stdio.h>
#include "Utils.h"

int main(int argc,char** argv)
{
	char buffer[32],b,*pr;
	int port,baud,count;
	port_handle hComPort;

	if (argc>1)
		port=atoi(argv[1]);
	else
		port=1;

	if (argc>2)
		baud=atoi(argv[2]);
	else
		baud=9600;

	if (port*baud!=0)
		hComPort=OpenPort(port,baud,0,100);
	else
		hComPort=0;


	while(hComPort!=0)
	{
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

		printf("%s\n",buffer);
	}

	if (hComPort!=0)
		ClosePort(hComPort);
}

