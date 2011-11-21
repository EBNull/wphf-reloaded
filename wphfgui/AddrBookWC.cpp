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

#include <gnugettext.hpp>
#pragma hdrstop

#include "AddrBookWC.h"
#include "ConfIni.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

//---------------------------------------------------------------------------

__fastcall TAddressBookWC::TAddressBookWC()
	: TAddressBook()
{
	FIsReadOnly = true;
}
//---------------------------------------------------------------------------

void __fastcall TAddressBookWC::Load()
{
	throw new EAddressBookException(
		L"Windows Contacts support is still unimplemented.\n"
		L"You can contribute, if you are a developer.\n"
		L"A donation to the author may speed up development :-).\n"
		L"Go to http://sourceforge.net/projects/wphf-reloaded if you feel generous or skilled enough."
	);
}
//---------------------------------------------------------------------------

