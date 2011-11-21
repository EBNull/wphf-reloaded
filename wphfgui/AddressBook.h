/*
Winprint HylaFAX Reloaded
Copyright (C) 2011 Monti Lorenzo

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//---------------------------------------------------------------------------

#ifndef AddressBookH
#define AddressBookH

#include <vcl.h>
#pragma hdrstop

typedef void __fastcall (__closure *TAddressBookChangeEvent)(TObject *);
typedef void __fastcall (__closure *TAddressBookDuplicateEvent)(TObject *, const UnicodeString&, bool&);

class EAddressBookException : public Exception
{
public:
	__fastcall EAddressBookException(const UnicodeString& Msg);
};

class EAddressBookUnchanged : public Exception
{
public:
	__fastcall EAddressBookUnchanged();
};

class TFaxNumber : public TObject
{
private:
	UnicodeString FNumber;

public:
	__fastcall TFaxNumber(const UnicodeString& ANumber);
	__property UnicodeString Number = { read = FNumber, write = FNumber };
};

class TAddressBook : public TPersistent
{
protected:
	TStringList *FNames;
	bool FIsReadOnly, FIsOnLine;
	UnicodeString __fastcall GetName(int Index);
	UnicodeString __fastcall GetNumber(int Index);
	TAddressBookChangeEvent FOnAddressBookChanged;
	TAddressBookDuplicateEvent FOnAddressBookDuplicate;
	virtual void __fastcall AssignTo(TPersistent* Dest);

public:
	__fastcall TAddressBook();
	__fastcall virtual ~TAddressBook();
	virtual void __fastcall Load() = 0;
	virtual void __fastcall Clear();
	virtual void __fastcall SetRecipient(const UnicodeString& Name, const UnicodeString& Number)
	{
		/* default do nothing */
	}
	virtual void __fastcall DeleteRecipient(const UnicodeString& Name);
	int __fastcall IndexOfName(const UnicodeString& Name);
	int __fastcall IndexOfNumber(const UnicodeString& Number);
	__property UnicodeString Names[int Index] = { read = GetName };
	__property UnicodeString Numbers[int Index] = { read = GetNumber };
	__property TAddressBookChangeEvent OnAddressBookChanged =
		{ read = FOnAddressBookChanged, write = FOnAddressBookChanged };
	__property TAddressBookDuplicateEvent OnAddressBookDuplicate =
		{ read = FOnAddressBookDuplicate, write = FOnAddressBookDuplicate };
	__property bool ReadOnly = { read = FIsReadOnly };
	__property bool OnLine = { read = FIsOnLine };
};

//---------------------------------------------------------------------------
extern TAddressBook *AddressBook;
//---------------------------------------------------------------------------
#endif
