/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qapplication.h>
#include <qfile.h>

#include "Cash.h"
#include "WorkPlace.h"

extern int revision;

CashRegister* CashReg;
//WorkPlace*    WPlace;

int main( int argc, char ** argv )
{
	for(int i = 0;i<argc;i++)
	{
		if(!strcmp(argv[i],"--version"))
		{
			fprintf(stdout,"1.0 rev %d\n",revision);
			return 0;
		}

		if(!strcmp(argv[i],"--revision"))
		{
			fprintf(stdout,"%d\n",revision);
			return 0;
		}
	}

	QFile *runFlag = new QFile(".run");//existence of this flag means that program is running
	runFlag->open(IO_WriteOnly);
	runFlag->close();

	QApplication *app = new QApplication( argc, argv );

	app->setOverrideCursor(Qt::blankCursor);

#	if QT_VERSION < 0x030000
	app->setFont(QFont("arial",10,QFont::Normal,false,QFont::Unicode));
#	endif

#	if QT_VERSION >= 0x030000
	app->setFont(QFont("arial",10, QFont::Normal,false));
#	endif


	CashReg  = new CashRegister;
	//WPlace   = new WorkPlace(CashReg);

	app->setMainWidget(CashReg);

	CashReg->show();
	//WPlace->show();

	int result = app->exec();

	//delete WPlace;
	delete CashReg;
	delete app;

	runFlag->remove();//clear running flag
	delete runFlag;

	return result;
}
