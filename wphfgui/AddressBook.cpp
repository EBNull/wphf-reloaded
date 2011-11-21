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

#include <windows.h>
#include <vcl.h>
#include <gnugettext.hpp>
#pragma hdrstop

#include "AddressBook.h"
#include "ConfIni.h"

#pragma package(smart_init)

TAddressBook *AddressBook = NULL;

//---------------------------------------------------------------------------
__fastcall EAddressBookException::EAddressBookException(const UnicodeString& Msg)
	: Exception(Msg)
{
}

//---------------------------------------------------------------------------
__fastcall EAddressBookUnchanged::EAddressBookUnchanged()
	: Exception(L"")
{
}

//---------------------------------------------------------------------------
__fastcall TFaxNumber::TFaxNumber(const UnicodeString& ANumber)
	: TObject()
{
	FNumber = ANumber;
}

//---------------------------------------------------------------------------
__fastcall TAddressBook::TAddressBook()
	: TPersistent(),
	FOnAddressBookChanged(NULL),
	FOnAddressBookDuplicate(NULL),
	FIsReadOnly(false),
	FIsOnLine(false)
{
	FNames = new TStringList(true);
	FNames->CaseSensitive = false;
	FNames->Sorted = true;
}

//---------------------------------------------------------------------------
__fastcall TAddressBook::~TAddressBook()
{
	delete FNames;
}

//---------------------------------------------------------------------------
void __fastcall TAddressBook::Clear()
{
	FNames->Clear();
	if (FOnAddressBookChanged)
		FOnAddressBookChanged(this);
}

//---------------------------------------------------------------------------
void __fastcall TAddressBook::DeleteRecipient(const UnicodeString& Name)
{
	SetRecipient(Name, L"");
}

//---------------------------------------------------------------------------
UnicodeString __fastcall TAddressBook::GetName(int Index)
{
	if (Index < 0 || Index >= FNames->Count)
		return L"";
	else
		return FNames->Strings[Index];
}

//---------------------------------------------------------------------------
UnicodeString __fastcall TAddressBook::GetNumber(int Index)
{
	if (Index < 0 || Index >= FNames->Count)
		return L"";
	else
		return static_cast<TFaxNumber *>(FNames->Objects[Index])->Number;
}

//---------------------------------------------------------------------------
void __fastcall TAddressBook::AssignTo(TPersistent* Dest)
{
	TStrings *DestStrings = dynamic_cast<TStrings *>(Dest);

	if (DestStrings) {
		DestStrings->BeginUpdate();
		try {
			DestStrings->Clear();
			DestStrings->AddStrings(FNames);
		}
		__finally {
			DestStrings->EndUpdate();
		}
	}
}

//---------------------------------------------------------------------------
int __fastcall TAddressBook::IndexOfName(const UnicodeString& Name)
{
	return FNames->IndexOf(Name);
}

//---------------------------------------------------------------------------
int __fastcall TAddressBook::IndexOfNumber(const UnicodeString& Number)
{
	int i;

	for (i = 0; i < FNames->Count; i++) {
		if (static_cast<TFaxNumber *>(FNames->Objects[i])->Number == Number) {
			return i;
		}
	}

	return -1;
}

