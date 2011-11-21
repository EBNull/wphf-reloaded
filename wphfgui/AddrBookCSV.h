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

#ifndef AddrBookCSVH
#define AddrBookCSVH
//---------------------------------------------------------------------------

#include "AddressBook.h"

class TAddressBookCSV : public TAddressBook
{
private:
	UnicodeString FABFile;
	TFileStream * __fastcall LockAddressBook(WORD wMode);
	bool __fastcall ConvertOldAddressBook(const UnicodeString& Path);

public:
	__fastcall TAddressBookCSV();
	virtual void __fastcall Load();
	void __fastcall SetRecipient(const UnicodeString& Name, const UnicodeString& Number);
};

#endif
