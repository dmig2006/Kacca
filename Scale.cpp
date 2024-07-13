/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "Scale.h"
#include "Utils.h"
using namespace std;

EmptyScales::EmptyScales(int port,int baud,int wait)
{
}

EmptyScales::~EmptyScales(void)
{
}

bool EmptyScales::GetWeight(int& w)
{
	w=0;
	return false;
}

MassaKScales::MassaKScales(int port,int baud,int wait)
{
	//open scales port
	if (port*baud!=0)
		hScaleCom=OpenPort(port,baud,2,1000);
	else
		hScaleCom=0;
	WeighTimeOut=wait;
}

MassaKScales::~MassaKScales(void)
{
	if (hScaleCom!=0)
		ClosePort(hScaleCom);
}

bool MassaKScales::GetWeight(int& w)
{
	if (hScaleCom==0)
		return false;

	w=0;
	unsigned char Buffer[18];
	time_t t=time(NULL);

	while (time(NULL)-t<WeighTimeOut)
	{
		Buffer[0]=Buffer[1]=0;Buffer[2]=3;//sent request to the scales
		WriteToPort(hScaleCom,Buffer,3);
		memset(Buffer,0,18);

		WaitSomeTime(100);//wait for answer

		if (ReadFromPort(hScaleCom,Buffer,18)==18)//read weight
		{
			char num[13];
			memset(num,0,13);

			for (int i=0;i<6;i++)
				sprintf(num+i*2,"%02x",Buffer[i]);

			w=atoi(num);

			if (w!=0)
				break;
		}
	}

	return (w!=0);
	/*
	if (w==0)
		return false;
	else
		return true;
	*/
}

//////////////////////////////////////////////////////////////////////
// MassaKP Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MassaKP::MassaKP(int port,int baud,int wait) : MassaKScales(port, baud, wait)
{

}

bool MassaKP::GetWeight(int& w)
{
	if (hScaleCom==0)
		return false;

	w=0;
	unsigned char b[4] = {0xa5};
	ClearPort(hScaleCom);
	WriteToPort(hScaleCom,b,1);
	if((!ReadFromPort(hScaleCom,b,1)) || *b != 0x77)
		return false;
	*b = 0x09;
	WriteToPort(hScaleCom,b,1);
	if((!ReadFromPort(hScaleCom,b,1)) || *b != 0x01)
		return false;
	if(!ReadFromPort(hScaleCom,&w,3))
		return false;
	if((!ReadFromPort(hScaleCom,b,1)) || *b != 0x10)
		return false;
	return true;
}
