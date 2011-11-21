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

#ifndef ConfigH
#define ConfigH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <jpeg.hpp>
#include <Dialogs.hpp>
#include <ComCtrls.hpp>
#include <ImgList.hpp>
//---------------------------------------------------------------------------
class TConfigForm : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TImage *Image1;
	TLabel *Label5;
	TLabel *Label1;
	TEdit *hServer;
	TLabel *Label2;
	TEdit *hUsername;
	TLabel *Label3;
	TEdit *hPassword;
	TLabel *Default;
	TEdit *hDefaultEmail;
	TLabel *Label4;
	TEdit *hModem;
	TPanel *Panel3;
	TButton *btnOK;
	TButton *btnCancel;
	TComboBox *hPageSize;
	TLabel *Label7;
	TComboBox *hNotificationType;
	TLabel *Label8;
	TCheckBox *hPassiveIPIgnore;
	TComboBox *hResolution;
	TLabel *Label10;
	TGroupBox *gbAddressBook;
	TEdit *hAddressBook;
	TButton *btnBrowse;
	TButton *btnDefault;
	TLabel *Label9;
	TLabel *Label11;
	TComboBox *hLanguage;
	TLabel *Label12;
	TLabel *Label6;
	TPageControl *tabs;
	TTabSheet *tsMain;
	TTabSheet *tsAddrBook;
	TComboBox *hAddrBookType;
	TLabel *Label14;
	TLabel *Label13;
	TGroupBox *gbMAPI;
	TRadioButton *hMAPIDefProfile;
	TRadioButton *hMAPIUseProfile;
	TEdit *hMAPIProfile;
	TImageList *images;
	TGroupBox *gbODBC;
	TEdit *hODBCDSN;
	TEdit *hODBCUser;
	TEdit *hODBCPassword;
	TEdit *hODBCFax;
	TEdit *hODBCTable;
	TEdit *hODBCName;
	TLabel *Label15;
	TLabel *Label16;
	TLabel *Label17;
	TLabel *Label18;
	TLabel *Label19;
	TLabel *Label20;
	TCheckBox *hODBCAuth;
	void __fastcall btnBrowseClick(TObject *Sender);
	void __fastcall btnDefaultClick(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall FormConstrainedResize(TObject *Sender, int &MinWidth, int &MinHeight,
          int &MaxWidth, int &MaxHeight);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall hMAPIProfileChange(TObject *Sender);
	void __fastcall hODBCAuthClick(TObject *Sender);
public:		// User declarations
	__fastcall TConfigForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TConfigForm *ConfigForm;
//---------------------------------------------------------------------------
#endif
