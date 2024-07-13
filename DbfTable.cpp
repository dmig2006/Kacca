/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <xbase/xbase.h>
#include <xbase/xbexcept.h>
#include <algorithm>

#include "DbfTable.h"
#include "Utils.h"

enum {StrVal,IntVal,DoubleVal,BoolVal,LongVal,};

DbfTable::DbfTable(void)
{
	m_Schema = NULL;
	x = new xbXBase;
	Dbf = new MyDbf(x);
	Dbf->SetVersion(4);
	Indices.clear();
	UseDeleted=false;
	NonDeletedCount=0;
}

DbfTable::~DbfTable(void)
{
	CloseIndices();
	CloseDatabase();
	delete Dbf;
	delete x;
}

void DbfTable::SetUseDeleted(bool use)//use records marked as deleted or not
{
	UseDeleted=use;
}

//!void DbfTable::ProcessError(short er, string TableName="")//throw exception which contains error's code and description
void DbfTable::ProcessError(short er, string TableName)//throw exception which contains error's code and description
{
	string Name;

	switch (er)
	{
		case XB_NO_ERROR:
		case XB_EOF:
		case XB_BOF:
			return;
		default:
		fprintf(stderr,"ProcessError = %d %x\n",er,er);
  		Name="Table "+Dbf->GetDbfName();
  		//if (TableName!="")
  		Name+=" /"+(xbString)TableName.c_str()+"/ ";
  		Name=Name + ": "+xbStrError(er);

			throw cash_error(xbase_error,er,Name);
	}
}

string DbfTable::TrimStringField(string str)//remove blanks
{
	for (int i=str.length()-1;i>=0;i--)
		if (str[i]!=' ')
			return str.substr(0,i+1);

	return "";
}

bool DbfTable::Locate(const char* FieldName,void* val,int type)
{
	for (unsigned int i=0;i<Indices.size();i++)
		if (FieldNames[i]==FieldName)//if this field was indexed
		{
			xbShort er;

			switch (type)//find a key
			{
				case StrVal:er=Indices[i]->FindKey((const char*)val);break;
				case IntVal:er=Indices[i]->FindKey(*(int*)val);break;
				case DoubleVal:er=Indices[i]->FindKey(*(double*)val);break;
				case BoolVal:er=Indices[i]->FindKey(*(bool*)val);break;
				case LongVal:er=Indices[i]->FindKey(*(long long*)val);break;
				default:return false;
			}

			if (er==XB_NOT_FOUND)
				return false;

			if ((er==XB_FOUND)&&((UseDeleted)||((!UseDeleted)&&(!RecordDeleted()))))
				return true;
			else
			{
				while(Indices[i]->GetNextKey()==XB_NO_ERROR)//if found record is deleted
				{											//look at other keys
					switch(type)
					{
						case StrVal://if next key gives different value we can not find a record
							if (GetStringField(FieldName).substr(0,strlen((const char*)val))==(const char*)val)
							{
								if (RecordDeleted())//if record is deleted we have to continue
									continue;
								else
									return true;//success
							}
							else
								return false;
						case IntVal://just the same but for integers
							if (GetLongField(FieldName)==*(int*)val)
							{
								if (RecordDeleted())
									continue;
								else
									return true;
							}
							else
								return false;
						case DoubleVal://just the same but for doubles
							if (GetDoubleField(FieldName)==*(double*)val)
							{
								if (RecordDeleted())
									continue;
								else
									return true;
							}
							else
								return false;
						case BoolVal://just the same but for booleans
							if (GetLogicalField(FieldName)==*(bool*)val)
							{
								if (RecordDeleted())
									continue;
								else
									return true;
							}
							else
								return false;
						case LongVal://just the same but for Int64
							if (GetLLongField(FieldName)==*(long long*)val)
							{
								if (RecordDeleted())
									continue;
								else
									return true;
							}
							else
								return false;

					}
				}
				return false;
			}
		}

	if (!GetFirstRecord())//if this field is not indexed we have to use simple search
		return false;

	do
	{
		switch(type)
		{
			case StrVal:
				if (GetStringField(FieldName).substr(0,strlen((const char*)val))==(const char*)val)
					return true;
				else
					break;
			case IntVal:
				if (GetLongField(FieldName)==*(int*)val)
					return true;
				else
					break;
			case DoubleVal:
				if (GetDoubleField(FieldName)==*(double*)val)
					return true;
				else
					break;
			case BoolVal:
				if (GetLogicalField(FieldName)==*(bool*)val)
					return true;
				else
					break;
			case LongVal:
				if (GetLLongField(FieldName)==*(long long*)val)
					return true;
				else
					break;
		}
	}
	while (GetNextRecord());

	return false;
};


void DbfTable::CreateDatabase(string Name, xbSchema *Schema)//create DBF table Name
{																//according fields description
	m_Schema = Schema;
	ProcessError(Dbf->CreateDatabase(Name.c_str(),Schema,XB_OVERLAY), Name);
	Dbf->flush();
	NonDeletedCount=0;
}

void DbfTable::CloseDatabase(void)//close DBF table
{
	ProcessError(Dbf->CloseDatabase());
	NonDeletedCount=0;
}

void DbfTable::OpenDatabase(string Name)//open DBF table Name
{
	ProcessError(Dbf->OpenDatabase(Name.c_str()), Name);
	NonDeletedCount=CountNonDeleted();
}

void DbfTable::OpenDatabase(string TableName, xbSchema *Schema)//открывает dbf и сверяет с описанием
{
	int retval;
	const char* Name = TableName.c_str();

	m_Schema = Schema;
	{
		xbShort er = Dbf->OpenDatabase(Name);
		if(er == XB_READ_ERROR)
		{// не может прочитать заголовок - удаляем эту таблицу
			CreateDatabase(TableName, Schema);
			return;
		}
		ProcessError(er, TableName);
	}
	NonDeletedCount=CountNonDeleted();
	for(xbSchema * ptr = Schema; ptr->FieldLen; ptr++)
	{
		if(Dbf->GetFieldNo(ptr->FieldName) == -1 ||
		   Dbf->GetFieldLen(ptr-Schema) < ptr->FieldLen)
		 {
			char new_name[256];
			strcpy(new_name,Name);
			strcpy(strrchr(new_name,'.'),"_t.dbf");
			xbXBase x;
			xbDbf tmp(&x);
			ProcessError(tmp.CreateDatabase(new_name,Schema,XB_OVERLAY), TableName);
			if (!Dbf->GetFirstRecord()){
				do
				{
					ProcessError(tmp.AppendRecord(), TableName);
					for(ptr = Schema; ptr->FieldLen; ptr++){
						xbShort fNum = Dbf->GetFieldNo(ptr->FieldName);
						if(fNum!=-1){
							char Buf[1024];
							Dbf->GetField(fNum,Buf);
							char * p = Buf;
							while(*p == ' ')p++;
							tmp.PutField(ptr->FieldName/*fNum*/,p);
						}
					}
					ProcessError(Dbf->PutRecord(), TableName);
				}
				while(Dbf->GetNextRecord()!=XB_EOF);
			}
			ProcessError(tmp.CloseDatabase(), TableName);
			CloseDatabase();
			remove(Name);
			rename(new_name,Name);

			retval=Dbf->OpenDatabase(Name);
			ProcessError(retval, TableName);

			NonDeletedCount=Dbf->NoOfRecords();
			return;
		}
	}
}

void DbfTable::AppendRecord(void)//append current record's buffer to the DBF file
{
	ProcessError(Dbf->AppendRecord());
	ProcessError(Dbf->PutRecord());//store changes to file
	Dbf->flush();
	if (!Dbf->RecordDeleted())
		NonDeletedCount++;
}

void DbfTable::BlankRecord(void)//fill out record's buffer with blanks
{
	ProcessError(Dbf->BlankRecord());
}

void DbfTable::ChangeCount(bool before,bool after)//auxiliary function
{
	if ((before)&&(!after))
		NonDeletedCount++;
	if ((!before)&&(after))
		NonDeletedCount--;
}

void DbfTable::DeleteAllRecords(void)//it's clear
{
	ProcessError(Dbf->DeleteAllRecords());
	NonDeletedCount=0;
}

void DbfTable::UndeleteAllRecords(void)
{
	ProcessError(Dbf->DeleteAllRecords());
	NonDeletedCount=Dbf->NoOfRecords();
}

void DbfTable::DeleteRecord(void)
{
	bool before=Dbf->RecordDeleted();
	ProcessError(Dbf->DeleteRecord());
	ProcessError(Dbf->PutRecord());
	Dbf->flush();
	ChangeCount(before,Dbf->RecordDeleted());
}

void DbfTable::UndeleteRecord(void)
{
	bool before=Dbf->RecordDeleted();
	ProcessError(Dbf->DeleteRecord());
	ProcessError(Dbf->PutRecord());
	Dbf->flush();
	ChangeCount(before,Dbf->RecordDeleted());
}

void DbfTable::Zap(void)//delete all records in the DBF table
{
	if(!Dbf->PhysicalNoOfRecords())
		return;
/*
	if(m_Schema){
		string s = (Dbf->GetDbfName()).c_str();
		this->CloseDatabase();
		this->CreateDatabase(s.c_str(), m_Schema);
		return;
	}
//*/
	DeleteAllRecords();
	Pack();
//	CloseDatabase();
//	ProcessError(Dbf->Zap(F_SETLKW));
	NonDeletedCount=0;
}

void DbfTable::Pack(void)//erase all records marked as deleted
{
	ProcessError(Dbf->PackDatabase(1));
	NonDeletedCount=Dbf->NoOfRecords();
}

bool DbfTable::GetFirstRecord()
{
	if (RecordCount()==0)
		return false;

	ProcessError(Dbf->GetFirstRecord());

	if (!UseDeleted)//go down until non-deleted record is found
		while(RecordDeleted())//since our table is not empty (even not including deleted records)
			ProcessError(Dbf->GetNextRecord());//we can find next record

	return true;
}

bool DbfTable::GetLastRecord()
{
	if (RecordCount()==0)
		return false;

	ProcessError(Dbf->GetLastRecord());

	if (!UseDeleted)//go up until non-deleted record is found
		while(RecordDeleted())
			ProcessError(Dbf->GetPrevRecord());

	return true;
}

bool DbfTable::GetPrevRecord()
{
	if (RecordCount()==0)
		return false;

	do
	{
		int er=Dbf->GetPrevRecord();//if one does not use deleted we go down in the table until
		ProcessError(er);//nondeleted record or begin of file will be found
		if (er==XB_BOF)
			return false;
	}
	while((!UseDeleted)&&(RecordDeleted()));
	return true;
}

bool DbfTable::GetNextRecord()
{
	if (RecordCount()==0)
		return false;

	do
	{
		int er=Dbf->GetNextRecord();//as above
		ProcessError(er);
		if (er==XB_EOF)
			return false;
	}
	while((!UseDeleted)&&(RecordDeleted()));
	return true;
}

bool DbfTable::GetFirstKey(const char* FieldName)//get record which is attached to the first key
{
	if ((strlen(FieldName)==0)||(RecordCount()==0))
		return false;

	for (unsigned int i=0;i<Indices.size();i++)
		if (FieldNames[i]==FieldName)
		{
			ProcessError(Indices[i]->GetFirstKey());

			if (!UseDeleted)
				while(RecordDeleted()) //since not all records are deleted we will find something
					ProcessError(Indices[i]->GetNextKey());
			return true;
		};
	return false;
}

bool DbfTable::GetLastKey(const char* FieldName)//get record which is attached to the last key
{
	if ((strlen(FieldName)==0)||(RecordCount()==0))
		return false;

	for (unsigned int i=0;i<Indices.size();i++)
		if (FieldNames[i]==FieldName)
		{
			ProcessError(Indices[i]->GetLastKey());

			if (!UseDeleted)
				while(RecordDeleted())  //since not all records are deleted we will find something
					ProcessError(Indices[i]->GetPrevKey());
			return true;
		};
	return false;
}

bool DbfTable::GetNextKey(const char* FieldName) //get the record attached to the next key
{
	if ((strlen(FieldName)==0)||(RecordCount()==0))
		return false;

	for (unsigned int i=0;i<Indices.size();i++)
		if (FieldNames[i]==FieldName)
		{
			do
			{
				int er=Indices[i]->GetNextKey();//twist it while we does not found non deleted record
				ProcessError(er);
				if (er!=XB_NO_ERROR)   //or we reach the end of table
					return false;
			}
			while((!UseDeleted)&&(RecordDeleted()));
			return true;
		}
	return false;
}

bool DbfTable::GetPrevKey(const char* FieldName)
{
	if ((strlen(FieldName)==0)||(RecordCount()==0))
		return false;

	for (unsigned int i=0;i<Indices.size();i++)
		if (FieldNames[i]==FieldName)
		{
			do
			{
				int er=Indices[i]->GetPrevKey();//twist it while we does not found non deleted record
				ProcessError(er);
				if (er!=XB_NO_ERROR)//or we reach the end of table
					return false;
			}
			while((!UseDeleted)&&(RecordDeleted()));
			return true;
		}
	return false;
}

bool DbfTable::Locate(const char* FieldName,const char *str)//locate string
{
	return Locate(FieldName,(void*)str,StrVal);
};

bool DbfTable::Locate(const char* FieldName,int val)//locate integer
{
	return Locate(FieldName,&val,IntVal);
};

bool DbfTable::Locate(const char* FieldName,double val)//locate double
{
	return Locate(FieldName,&val,DoubleVal);
};

bool DbfTable::Locate(const char* FieldName,bool val)//locate boolean
{
	return Locate(FieldName,&val,BoolVal);
};

bool DbfTable::Locate(const char* FieldName,long long val)//locate integer
{
	return Locate(FieldName,&val,LongVal);
};

int DbfTable::RecordCount(void)
{
	if (Dbf->NoOfRecords()==0)
		return 0;

	if (UseDeleted)
		return Dbf->NoOfRecords();
	else
		return NonDeletedCount;
}

short DbfTable::GetFieldType(short FieldNo)
{
	if ((FieldNo<0)||(FieldNo>=Dbf->FieldCount()))
		ProcessError(XB_INVALID_FIELDNO);
	return Dbf->GetFieldType(FieldNo);
}

int DbfTable::CountNonDeleted(void)
{
	if (Dbf->NoOfRecords()==0)
		return 0;

	if (UseDeleted)
		return Dbf->NoOfRecords();

	xbLong RecNo=GetRecNo();//store current record's position

	xbLong count=0;

	ProcessError(Dbf->GetFirstRecord());
	do{
		if (!RecordDeleted())
			count++;
	}while(Dbf->GetNextRecord()!=XB_EOF);


	SetRecNo(RecNo);//restore position
	return count;
}

void DbfTable::PutRecord(void)//store current record's buffer to the DBF table
{
	bool before=Dbf->RecordDeleted();
	ProcessError(Dbf->PutRecord());
	Dbf->flush();
	ChangeCount(before,Dbf->RecordDeleted());
}

void DbfTable::PutField(short FieldNo,const char* Buf)//just wrappers
{
	ProcessError(Dbf->PutField(FieldNo,Buf));
	Dbf->flush();
}

void DbfTable::PutField(const char* FieldName,const char* Buf)
{
	ProcessError(Dbf->PutField(FieldName,Buf));
	Dbf->flush();
}

void DbfTable::PutLongField(short FieldNo,int val)
{
	ProcessError(Dbf->PutLongField(FieldNo,val));
	Dbf->flush();
}

void DbfTable::PutLongField(const char* FieldName,int val)
{
	ProcessError(Dbf->PutLongField(FieldName,val));
	Dbf->flush();
}

// поддержка int64 чисел
void DbfTable::PutLLongField(const char* FieldName,long long val)
{
   char buf[18];
   memset( buf, 0x00, 18 );
   sprintf( buf, "%lld", val );

   ProcessError(Dbf->PutField(FieldName, buf));
   Dbf->flush();
}

void DbfTable::PutField(const char* FieldName,char* buf)
{
	ProcessError(Dbf->PutField(FieldName,buf));
	Dbf->flush();
}

void DbfTable::PutDoubleField(short FieldNo,double val)
{
	ProcessError(Dbf->PutDoubleField(FieldNo,val));
	Dbf->flush();
}

void DbfTable::PutDoubleField(const char* FieldName,double val)
{
	ProcessError(Dbf->PutDoubleField(FieldName,val));
	Dbf->flush();
}

string DbfTable::GetStringField(short FieldNo)
{
	if ((FieldNo<0)||(FieldNo>=Dbf->FieldCount()))
		ProcessError(XB_INVALID_FIELDNO);
	return TrimStringField(Dbf->GetStringField(FieldNo));
}

string DbfTable::GetStringField(const char* FieldName)
{
	if (Dbf->GetFieldNo(FieldName)==-1)
 		ProcessError(XB_INVALID_FIELDNO);
	return TrimStringField(Dbf->GetStringField(FieldName));
}

int DbfTable::GetLongField(short FieldNo)
{
	if ((FieldNo<0)||(FieldNo>=Dbf->FieldCount()))
		ProcessError(XB_INVALID_FIELDNO);
	return Dbf->GetLongField(FieldNo);
}

int DbfTable::GetLongField(const char* FieldName)
{
	if (Dbf->GetFieldNo(FieldName)==-1)
		ProcessError(XB_INVALID_FIELDNO);
	return Dbf->GetLongField(FieldName);
}

// поддержка int64 чисел
long long DbfTable::GetLLongField(const char* FieldName)
{
    if (Dbf->GetFieldNo(FieldName)==-1)
	ProcessError(XB_INVALID_FIELDNO);

    char buf[18];
    memset( buf, 0x00, 18 );
    Dbf->GetField( FieldName, buf );

    return atoll( buf );
}

// поддержка int64 чисел
long long DbfTable::GetLLongField(short FieldNo)
{
    if ((FieldNo<0)||(FieldNo>=Dbf->FieldCount()))
	ProcessError(XB_INVALID_FIELDNO);

    char buf[18];
    memset( buf, 0x00, 18 );
    Dbf->GetField( FieldNo, buf );

    return atoll( buf );
}


double DbfTable::GetDoubleField(short FieldNo)
{
	if ((FieldNo<0)||(FieldNo>=Dbf->FieldCount()))
		ProcessError(XB_INVALID_FIELDNO);
	return Dbf->GetDoubleField(FieldNo);
}

double DbfTable::GetDoubleField(const char* FieldName)
{
	if (Dbf->GetFieldNo(FieldName)==-1)
		ProcessError(XB_INVALID_FIELDNO);
	return Dbf->GetDoubleField(FieldName);
}

bool DbfTable::GetLogicalField(short FieldNo)
{
	if ((FieldNo<0)||(FieldNo>=Dbf->FieldCount()))
		ProcessError(XB_INVALID_FIELDNO);
	return (Dbf->GetLogicalField(FieldNo)==1);
/*
	if (Dbf->GetLogicalField(FieldNo)==1)
		return true;
	else
		return false;
*/
}

bool DbfTable::GetLogicalField(const char* FieldName)
{
	if (Dbf->GetFieldNo(FieldName)==-1)
		ProcessError(XB_INVALID_FIELDNO);

	return (Dbf->GetLogicalField(FieldName)==1);
/*  else
  {
 		if (Dbf->GetLogicalField(FieldName)==1)
			return true;
		else
			return false;
  }
*/
}

int DbfTable::GetRecNo(void)
{
	return Dbf->GetCurRecNo();
}

void DbfTable::SetRecNo(int RecNo)//set the record's position
{
	if(Dbf->GetCurRecNo() == RecNo)
		return;
	if (Dbf->GetFirstRecord()!=XB_EOF)
		do
		{
			if (RecNo==1)
				return;
			else
				RecNo--;
		}
		while(Dbf->GetNextRecord()!=XB_EOF);
}

int DbfTable::FieldCount(void)
{
	return Dbf->FieldCount();
}

bool DbfTable::RecordDeleted(void)
{
	if (Dbf->RecordDeleted()==1)
		return true;
	else
		return false;
}

short DbfTable::GetFieldLen(short FieldNo)
{
	if ((FieldNo<0)||(FieldNo>=Dbf->FieldCount()))
		ProcessError(XB_INVALID_FIELDNO);
	return Dbf->GetFieldLen(FieldNo);
}

string DbfTable::GetFieldName(short FieldNo)
{
	if ((FieldNo<0)||(FieldNo>=Dbf->FieldCount()))
		ProcessError(XB_INVALID_FIELDNO);
	return Dbf->GetFieldName(FieldNo);
}

short DbfTable::GetFieldNo(const char* FieldName)
{
	return Dbf->GetFieldNo(FieldName);
}

string DbfTable::GetField(const char* FieldName)
{
	if (Dbf->GetFieldNo(FieldName)==-1)
		ProcessError(XB_INVALID_FIELDNO);
	char buf[256];
	Dbf->GetField(FieldName,buf);
	return buf;
}

string DbfTable::GetField(short FieldNo)
{
	if ((FieldNo<0)||(FieldNo>=Dbf->FieldCount()))
		ProcessError(XB_INVALID_FIELDNO);
	char buf[256];
	Dbf->GetField(FieldNo,buf);
	return buf;
}

string DbfTable::GetTableName(void)
{
	return Dbf->GetDbfName().c_str();
}

void DbfTable::CreateIndex(const char* FileName,const char* IndexName)//create index file
{												//on the given field
	xbNdx *Index=new xbNdx(Dbf);

	Index->CreateIndex(FileName,IndexName,XB_NOT_UNIQUE,XB_OVERLAY);
	Index->ReIndex();

	FieldNames.insert(FieldNames.end(),IndexName);//add index info
	Indices.insert(Indices.end(),Index);
}


void DbfTable::OpenIndex(string FileName,const char* IndexName)//open index file
{													//on the given field
	xbNdx *Index=new xbNdx(Dbf);
	ProcessError(Index->OpenIndex(FileName.c_str()), FileName);

	FieldNames.insert(FieldNames.end(),IndexName);//add index info
	Indices.insert(Indices.end(),Index);
}

void DbfTable::CloseIndices(void)
{
	for (unsigned int i=0;i<Indices.size();i++)//close all indices
	{
		ProcessError(Indices[i]->CloseIndex());
		delete Indices[i];

	}

	Indices.clear();
}



