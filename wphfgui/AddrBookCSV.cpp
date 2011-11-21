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

#include "AddrBookCSV.h"
#include "ConfIni.h"

#pragma package(smart_init)

//---------------------------------------------------------------------------
__fastcall TAddressBookCSV::TAddressBookCSV()
	: TAddressBook()
{
}

//---------------------------------------------------------------------------
TFileStream * __fastcall TAddressBookCSV::LockAddressBook(WORD wMode)
{
	TFileStream *Result = NULL;

	int ntry = 5;

	//try to acquire a lock on the address book file
	for (;;) {
		try {
			Result = new TFileStream(FABFile, wMode);
			break;
		}
		catch (Exception& E) {
			if (ntry-- == 0)
				throw new EAddressBookException(_(L"Unable to lock addressbook.csv!"));
			Sleep(500);
		}
	}

	return Result;
}

//---------------------------------------------------------------------------
bool __fastcall TAddressBookCSV::ConvertOldAddressBook(const UnicodeString& Path)
{
	TFileStream *File;
	TStringList *Names, *Numbers, *AddrBook, *Temp;
	int i;

	Names = new TStringList();
	Numbers = new TStringList();
	AddrBook = new TStringList();
	Temp = new TStringList();

	try {
		Temp->QuoteChar = L'"';
		Temp->Delimiter = L',';

		Names->LoadFromFile(Path + L"names.txt");
		Numbers->LoadFromFile(Path + L"numbers.txt");

		for (i = 0; i < Names->Count && i < Numbers->Count; i++) {
			Temp->Clear();
			Temp->Add(Names->Strings[i]);
			Temp->Add(Numbers->Strings[i]);
			AddrBook->Add(Temp->DelimitedText);
		}

		File = LockAddressBook(fmCreate | fmShareExclusive);

		try {
			AddrBook->SaveToStream(File);
		}
		__finally {
            delete File;
		}

		return true;
	}
	__finally {
		delete Names;
		delete Numbers;
		delete AddrBook;
		delete Temp;
	}

	return false;
}

//---------------------------------------------------------------------------
bool __fastcall FileExistsAndIsGreaterThanZero(const UnicodeString& Filename)
{
	WIN32_FIND_DATA FindData;
	HANDLE LHandle;

	LHandle = FindFirstFile(Filename.c_str(), &FindData);
	if (LHandle != INVALID_HANDLE_VALUE) {
		FindClose(LHandle);
		return ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) &&
			   ((FindData.nFileSizeHigh != 0) || (FindData.nFileSizeLow != 0));
	}
	else
		return false;
}

//---------------------------------------------------------------------------
void __fastcall TAddressBookCSV::Load()
{
	UnicodeString Path;

	if (ConfigIni->AddrBookPath.Length() == 0) {
		FABFile = L"";
		FIsOnLine = false;
		return;
	} else {
		Path = IncludeTrailingPathDelimiter(ConfigIni->AddrBookPath);
		FABFile = Path + L"addressbook.csv";
	}

	TFileStream *File;
	int i;

	WORD wMode = fmOpenRead;

	if (!FileExistsAndIsGreaterThanZero(FABFile)) {
		//check for old names.txt and numbers.txt, and convert to the new format
		if (!FileExists(Path + L"names.txt") ||
			!FileExists(Path + L"numbers.txt") ||
			!ConvertOldAddressBook(Path))
		{
			//not found and not converted from old txt: create a new one
			wMode = fmCreate;
		}
	}

	TStringList *AddrBook = new TStringList();
	TStringList *Temp = new TStringList();

	try {
		Temp->QuoteChar = L'"';
		Temp->Delimiter = L',';

		File = LockAddressBook(wMode | fmShareDenyWrite);

		//read in data
		try {
			AddrBook->LoadFromStream(File);
		}
		__finally {
			delete File;
		}

		FNames->Clear();

		for (i = 0; i < AddrBook->Count; i++) {
			Temp->DelimitedText = AddrBook->Strings[i];
			if (Temp->Count >= 2)
				FNames->AddObject(Trim(Temp->Strings[0]),
				new TFaxNumber(Trim(Temp->Strings[1])));
		}

		FIsOnLine = true;
	}
	__finally {
		delete AddrBook;
		delete Temp;
	}

	//fire event
	if (FOnAddressBookChanged)
		FOnAddressBookChanged(this);
}

//---------------------------------------------------------------------------
void __fastcall TAddressBookCSV::SetRecipient(const UnicodeString& Name,
	const UnicodeString& Number)
{
	if (!FIsOnLine)
		return;

	TFileStream *File;
	int i;

	if (!ForceDirectories(ExtractFilePath(FABFile)))
		throw new EAddressBookException(_(L"Unable to create addressbook.csv!"));

	TStringList *AddrBook = new TStringList();
	TStringList *Temp = new TStringList();

	try {
		Temp->QuoteChar = L'"';
		Temp->Delimiter = L',';

		WORD wMode = FileExists(FABFile)
			? fmOpenReadWrite
			: fmCreate;

		File = LockAddressBook(wMode | fmShareExclusive);

		//read in data
		try {
			AddrBook->LoadFromStream(File);

			FNames->Clear();

			for (i = 0; i < AddrBook->Count; i++) {
				Temp->DelimitedText = AddrBook->Strings[i];
				if (Temp->Count >= 2)
					FNames->AddObject(Trim(Temp->Strings[0]),
					new TFaxNumber(Trim(Temp->Strings[1])));
			}

			if ((i = FNames->IndexOf(Name)) < 0) {
				if (Number.Length() > 0)
					FNames->AddObject(Trim(Name),
					new TFaxNumber(Trim(Number)));
			} else {
				if (Number.Length() > 0) {
					bool ChangeNumber = false;

					if (FOnAddressBookDuplicate)
						FOnAddressBookDuplicate(this, Name, ChangeNumber);

					if (!ChangeNumber)
						throw new EAddressBookUnchanged();

					static_cast<TFaxNumber *>(FNames->Objects[i])->Number = Trim(Number);
				} else {
                    FNames->Delete(i);
                }
			}

			AddrBook->Clear();

			for (i = 0; i < FNames->Count; i++) {
				Temp->Clear();
				Temp->Add(FNames->Strings[i]);
				Temp->Add(static_cast<TFaxNumber *>(FNames->Objects[i])->Number);
				AddrBook->Add(Temp->DelimitedText);
			}

			File->Position = 0;
			File->Size = 0;
			AddrBook->SaveToStream(File);
		}
		__finally {
			delete File;
		}
	}
	__finally {
		delete AddrBook;
		delete Temp;
	}

	if (FOnAddressBookChanged)
		FOnAddressBookChanged(this);
}

