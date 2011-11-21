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

#include <vcl.h>
#include <gnugettext.hpp>
#pragma hdrstop

#include "Recipient.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TRecipientName *RecipientName;
//---------------------------------------------------------------------------
__fastcall TRecipientName::TRecipientName(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TRecipientName::FormCreate(TObject *Sender)
{
	TranslateComponent(this, L"wphfgui");
}
//---------------------------------------------------------------------------

void __fastcall TRecipientName::RecipientNameKeyPress(TObject *Sender, wchar_t &Key)

{
	if (Key == L'@')
		Key = L'\0';
}
//---------------------------------------------------------------------------

