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

#ifndef SelectH
#define SelectH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <CheckLst.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
class TSelectRcpt : public TForm
{
__published:	// IDE-managed Components
	TCheckListBox *lbNames;
	TButton *OK;
	TButton *Cancel;
	TPopupMenu *popup;
	TMenuItem *mnuSelectAll;
	TMenuItem *mnuSelectNone;
	void __fastcall FormConstrainedResize(TObject *Sender, int &MinWidth, int &MinHeight,
          int &MaxWidth, int &MaxHeight);
	void __fastcall lbNamesClickCheck(TObject *Sender);
	void __fastcall mnuSelectAllClick(TObject *Sender);
	void __fastcall mnuSelectNoneClick(TObject *Sender);
	void __fastcall lbNamesDrawItem(TWinControl *Control, int Index, TRect &Rect, TOwnerDrawState State);
	void __fastcall FormCanResize(TObject *Sender, int &NewWidth, int &NewHeight, bool &Resize);



private:	// User declarations
	TStringList *FNumbers;
	UnicodeString __fastcall GetNumbers();
	void __fastcall SetNumbers(const UnicodeString& Value);
	void __fastcall ToggleNumber(int Index, bool Selected);
public:		// User declarations
	__fastcall TSelectRcpt(TComponent* Owner);
	__fastcall virtual ~TSelectRcpt();
	__property UnicodeString Numbers = { read = GetNumbers, write = SetNumbers };
};
//---------------------------------------------------------------------------
extern PACKAGE TSelectRcpt *SelectRcpt;
//---------------------------------------------------------------------------
#endif
