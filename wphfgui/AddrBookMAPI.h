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

#ifndef AddrBookMAPIH
#define AddrBookMAPIH

#include <vcl.h>
#include <mapi.h>
#include <mapix.h>
#include <mapiutil.h>
#pragma hdrstop

#include "AddressBook.h"

class TAddressBookMAPI : public TAddressBook
{
private:
	void __fastcall GetContents(LPMAPICONTAINER pContainer,
	#ifdef MAPIDEBUG
		const UnicodeString& containerName,
	#endif
		int &unknown);
	void __fastcall ScanContainer(LPMAPICONTAINER pContainer,
	#ifdef MAPIDEBUG
		const UnicodeString& containerName,
	#endif
		int &unknown);

public:
	__fastcall TAddressBookMAPI();
	virtual void __fastcall Load();
};

#endif
