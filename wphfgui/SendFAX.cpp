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
#include <JclDateTime.hpp>
#include <DateUtils.hpp>
#include <shlobj.h>
#include <gnugettext.hpp>
#pragma hdrstop

#include "SendFAX.h"
#include "Recipient.h"
#include "ConfIni.h"
#include "AddrBookCSV.h"
#include "AddrBookMAPI.h"
#include "AddrBookWC.h"
#include "AddrBookODBC.h"
#include "Select.h"
#include "ipc.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "IdBaseComponent"
#pragma link "IdComponent"
#pragma link "IdTCPClient"
#pragma link "IdTCPConnection"
#pragma link "IdAntiFreeze"
#pragma link "IdAntiFreezeBase"
#pragma link "JvAppInst"
#pragma link "Spin"
#pragma resource "*.dfm"

TFAXSend *FAXSend;

enum {			// data file FORMats
	FORM_UNKNOWN = 0,	// unknown, initial setting
	FORM_PS   = 1,		// PostScript Level I
	FORM_PS2  = 2,		// PostScript Level II
	FORM_TIFF = 3,		// TIFF
	FORM_PCL  = 4,		// HP PCL5
	FORM_PDF  = 5		// Portable Document Format
};

//---------------------------------------------------------------------------
int __cdecl IpcCallback(LPCSTR szFile, LPVOID param)
{
	//data arived through pipe
	TFAXSend *form = static_cast<TFAXSend *>(param);
	form->LockDocuments();
	try {
		//do it via a message to synchronize with the main thread
		SendMessage(form->Handle, WM_ADDFAX, 0, (LPARAM)szFile);
	}
	__finally {
		form->UnlockDocuments();
	}
	return 0;
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::BringFaxWndToFront()
{
	//restore window if it was minimized
	if (this->WindowState == wsMinimized)
		this->WindowState = wsNormal;
	Application->BringToFront();
	//trick to force window to appear
	//we bring it topmost...
	SetWindowPos(this->Handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	//...then we restore its non-topmost state; the window keeps its Z-order
	SetWindowPos(this->Handle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

//---------------------------------------------------------------------------
MESSAGE void __fastcall TFAXSend::HandleWMAddFax(TMessage& Message)
{
	UnicodeString data = (LPCWSTR)Message.LParam;

	int pos = data.Pos(L"|");
	UnicodeString filename = ExpandUNCFileName(data.SubString(1, pos - 1));
	UnicodeString title = data.SubString(pos + 1, data.Length() - pos);

	//check parameter sanity
	if (!Sysutils::FileExists(filename))
		return;

	//ok add file to list of files to send
	TFileData *file = new TFileData();
	file->FileName = filename;

	lbDocuments->Items->BeginUpdate();
	try {
		lbDocuments->AddItem(title, file);
		lbDocuments->Checked[lbDocuments->Items->Count - 1] = true;
	}
	__finally {
		lbDocuments->Items->EndUpdate();
	}

	BringFaxWndToFront();
}

//---------------------------------------------------------------------------
__fastcall TFAXSend::TFAXSend(TComponent* Owner)
	: TForm(Owner), FHasNumber(false), FHasManyNumbers(false)
{
	WCHAR buf[MAX_PATH];

	SHGetSpecialFolderPathW(this->Handle, buf, CSIDL_APPDATA, FALSE);
	TConfigIni::WPHFUserDir = IncludeTrailingPathDelimiter(buf) + L"Winprint HylaFAX Reloaded\\";

	SHGetSpecialFolderPathW(this->Handle, buf, CSIDL_COMMON_APPDATA, FALSE);
	UnicodeString LocalDir = IncludeTrailingPathDelimiter(buf) + L"Winprint HylaFAX Reloaded\\";

	TConfigIni::WPHFTempDir = LocalDir + L"faxtmp\\";

	TConfigIni::oldWPHFDir = LocalDir;

	//use this often, so translate only once
	SelectFromAB = _(L"Select from address book");

	//ensure directory exists
	Sysutils::ForceDirectories(TConfigIni::WPHFUserDir);

	//move files from previous location
	if (Sysutils::FileExists(TConfigIni::oldWPHFDir + L"wphfgui.ini")) {
		RenameFile(TConfigIni::oldWPHFDir + L"wphfgui.ini",
			TConfigIni::WPHFUserDir + L"wphfgui.ini");
		RenameFile(TConfigIni::oldWPHFDir + L"addressbook.csv",
			TConfigIni::WPHFUserDir + L"addressbook.csv");
		TConfigIni::MovingConfigFiles = true;
		MessageDlg(_(L"Configuration files have been moved to the user profile."),
			mtWarning, TMsgDlgButtons() << mbOK, 0);
	}

	//synchronize access to the documents list box
	InitializeCriticalSection(&CSDocuments);

	//first translate the form
	TranslateComponent(this, L"wphfgui");

	//create config ini
	UnicodeString Ini = TConfigIni::WPHFUserDir + L"wphfgui.ini";
	ConfigIni = new TConfigIni(Ini);
	//attach event handlers
	ConfigIni->OnAddrBookTypeChanged = AddrBookTypeChanged;
	ConfigIni->OnAddrBookLocationChanged = AddrBookLocationChanged;
	ConfigIni->OnLanguageChanged = LanguageChanged;

	//load config ini
	ConfigIni->Load();

	//first time install?
	if (!Sysutils::FileExists(Ini) &&
	MessageDlg(_(L"Client is not configured. Would you like to configure it now?"),
	mtWarning, TMsgDlgButtons() << mbYes << mbNo, 0) == mrYes) {
		ConfigIni->Configure();
	}
}

//---------------------------------------------------------------------------
__fastcall TFAXSend::~TFAXSend()
{
	//stop pipe
	StopIpc();

	//cleanup
	DeleteCriticalSection(&CSDocuments);

	for (int i = 0; i < lbDocuments->Items->Count; i++) {
		TFileData *file = (TFileData*)lbDocuments->Items->Objects[i];
		//delete file only if it belongs to us
		if (SameFileName(ExtractFilePath(file->FileName), TConfigIni::WPHFTempDir))
			DeleteFile(file->FileName);
		delete file;
	}

	delete ConfigIni;
	delete AddressBook;
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::AddrBookTypeChanged(TObject *Sender, TAddressBookType AType)
{
	//already have one? delete it
	if (AddressBook) {
		AddressBook->Clear();
		delete AddressBook;
	}

	//create new address book
	switch (AType) {
	case abCSV:
		AddressBook = new TAddressBookCSV();
		break;
	case abMAPI:
		AddressBook = new TAddressBookMAPI();
		break;
	case abWinContacts:
		AddressBook = new TAddressBookWC();
		break;
	case abODBC:
		AddressBook = new TAddressBookODBC();
		break;
	default:
		AddressBook = NULL;
	}

	if (AddressBook) {
		//attach event handlers
		AddressBook->OnAddressBookChanged = AddressBookChanged;
		AddressBook->OnAddressBookDuplicate = AddressBookDuplicate;
		//load address book
		try {
			AddressBook->Load();
		}
		catch(EAddressBookException &E) {
			Application->ShowException(&E);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::AddrBookLocationChanged(TObject *Sender)
{
	AddressBook->Clear();
	//reload address book from new location
	try {
		AddressBook->Load();
	}
	catch(EAddressBookException &E) {
		Application->ShowException(&E);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::AddressBookChanged(TObject *Sender)
{
	//reload combo
	ABNames->Items->Assign(AddressBook);
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::AddressBookDuplicate(TObject *Sender, const UnicodeString& Name,
	bool& ChangeNumber)
{
	//ask user what to do with duplicate name
	UnicodeString Msg = Name +
		_(L" is already present in the address book.\nDo you want to modify the associated number?");
	ChangeNumber = MessageDlg(Msg, mtConfirmation,
	TMsgDlgButtons() << mbYes << mbNo, 0, mbNo) == mrYes;
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::LanguageChanged(TObject *Sender)
{
	//reload
	SelectFromAB = _(L"Select from address book");

	//translate again
	RetranslateComponent(this, L"wphfgui");
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::FormCreate(TObject *Sender)
{
	//put window on bottom-right of the screen
	RECT rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	this->Left = rect.right - this->Width - 8;
	this->Top = rect.bottom - this->Height - 8;

	hDate->Date = Today();
	hTime->Time = Now();

	UnicodeString filename;
	//got any file to send from the command line?
	if (ParamCount() >= 2 &&
	Sysutils::FileExists(filename = ExpandUNCFileName(ParamStr(1)))) {
		lbDocuments->Items->BeginUpdate();
		try {
			TFileData *file = new TFileData();
			file->FileName = filename;
			lbDocuments->AddItem(ParamStr(2), file);
			lbDocuments->Checked[lbDocuments->Items->Count - 1] = true;
		}
		__finally {
			lbDocuments->Items->EndUpdate();
        }
	}

	hNotifyEmail->Text = ConfigIni->Email;

	//start pipe
	StartIpc(IpcCallback, this);

	BringFaxWndToFront();
}


//---------------------------------------------------------------------------
void __fastcall TFAXSend::ABNamesChange(TObject *Sender)
{
	if (AddressBook)
		hFAXNumber->Text = AddressBook->Numbers[ABNames->ItemIndex];
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::EnableFields(bool Enable)
{
	lbDocuments->Enabled = Enable;
	hFAXNumber->Enabled = Enable;
	ABNames->Enabled = Enable;
	hNotifyEmail->Enabled = Enable;
	rbSendNow->Enabled = Enable;
	rbPostpone->Enabled = Enable;
	hDate->Enabled = Enable && rbPostpone->Checked;
	hTime->Enabled = Enable && rbPostpone->Checked;
	hDays->Enabled = Enable;
	hHours->Enabled = Enable;
}

//---------------------------------------------------------------------------
int __fastcall TFAXSend::GetFileFormat(const UnicodeString& FileName)
{
	try {
		TFileStream *Stream = new TFileStream(FileName, fmOpenRead);
		try {
			unsigned char buf[4];

			Stream->ReadBuffer(buf, 4);

			int magic = *(int*)buf;

			if (magic == 0x53502125) // %!PS
				return FORM_PS;
			else if (magic == 0x002A4949 || magic == 0x2A004D4D) // little endian / big endian
				return FORM_TIFF;
			else if (magic == 0x46445025) // %PDF
				return FORM_PDF;
			else if (magic & 0x00FFFFFF == 0x00214521) // !E!
				return FORM_PCL;
		}
		__finally {
            delete Stream;
        }
	}
	catch (...) {
		return FORM_UNKNOWN;
	}

	return FORM_UNKNOWN;
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::actSendExecute(TObject *Sender)
{
	UnicodeString Cmd, JobConfirm;
	TStringList *RemoteFileNames, *Numbers;
	int LineCode, pos;
	unsigned int PassivePort;
	int i, nrdoc, nrdest;
	short int Status, Resp[2];
	bool dummy;

	if (!FHasNumber)
		return;

	//lock documents list
	LockDocuments();
	try
	{
		//we're working, disable user interface
		EnableFields(false);
		FSending = true;
		//force actions to reflect state
		//we can't wait for actionsUpdate to be called by the VCL
		//because the call takes place in application idle time.
		//the user might click on some button in the meantime
		actionsUpdate(NULL, dummy);

		try {
			//for remote file names
			RemoteFileNames = new TStringList();
			Numbers = new TStringList();

			try {
				Numbers->Delimiter = L';';
				Numbers->StrictDelimiter = true;

				Numbers->DelimitedText = hFAXNumber->Text;

				//not really needed, since we're closing connections in a finally block...
				if (FTPctrl->Connected())
					FTPctrl->Disconnect();

				if (FTPdata->Connected())
					FTPdata->Disconnect();

				//extract host:port from server
				const UnicodeString& hHost = ConfigIni->Server;
				if ((pos = hHost.Pos(L":")) != 0) {
					FTPctrl->Port = hHost.SubString(pos + 1, hHost.Length()).ToIntDef(4559);
					FTPctrl->Host = hHost.SubString(1, pos - 1);
				} else {
					FTPctrl->Port = 4559;
					FTPctrl->Host = hHost;
				}

				//connect
				FTPctrl->Connect();

				try {
					//discard greetings
					do {
						LineCode = FTPctrl->GetResponse(-1, TEncoding::ASCII);
					} while (LineCode == 130);

					//authenticate
					Cmd = L"USER " + ConfigIni->Username;
					Resp[0] = 230;
					Resp[1] = 331;
					Status = FTPctrl->SendCmd(Cmd, Resp, 2, TEncoding::ASCII);

					//need password?
					if (Status == 331) {
						Cmd = L"PASS " + ConfigIni->Password;
						FTPctrl->SendCmd(Cmd, 230, TEncoding::ASCII);
					}

					//set time zone to local
					FTPctrl->SendCmd(L"TZONE LOCAL", 200, TEncoding::ASCII);

					//set operating modes to image / stream
					FTPctrl->SendCmd(L"TYPE I", 200, TEncoding::ASCII);
					FTPctrl->SendCmd(L"MODE S", 200, TEncoding::ASCII);

					// Passive data connection
					for (nrdoc = 0; nrdoc < lbDocuments->Items->Count; nrdoc++)
					{
						//for each document...
						if (lbDocuments->Checked[nrdoc]) {
							//send PASV command
							FTPctrl->SendCmd(L"PASV", 227, TEncoding::ASCII);

							//process answer
							UnicodeString pIP = FTPctrl->LastCmdResult->Text->Strings[0];
							pIP = pIP.SubString(pIP.Pos(L"(") + 1, pIP.Length());
							UnicodeString pHost = L"";
							//first 4 numbers are IP address
							for (i = 0; i < 4; i++) {
								pos = pIP.Pos(L",");
								pHost += pIP.SubString(1, pos - 1) + L".";
								pIP = pIP.SubString(pos + 1, pIP.Length());
							}
							//remove trailing dot
							pHost.SetLength(pHost.Length() - 1);
							//now the port
							//high byte
							pos = pIP.Pos(L",");
							UnicodeString pPort = pIP.SubString(1, pos - 1);
							PassivePort = pPort.ToInt() * 256;
							//low byte
							pPort = pIP.SubString(pos + 1, pIP.Length()).TrimRight();
							pPort.SetLength(pPort.Length() - 1);
							PassivePort += pPort.ToInt();

							//setup data connection
							FTPdata->Port = PassivePort;
							if (ConfigIni->PassiveIPIgnore)
								FTPdata->Host = FTPctrl->Host;
							else
								FTPdata->Host = pHost;

							//connect
							FTPdata->Connect();

							try {
								TFileData *file = static_cast<TFileData *>(lbDocuments->Items->Objects[nrdoc]);

								//try to get file format from magic number
								switch (GetFileFormat(file->FileName)) {
								FORM_PS:
									FTPctrl->SendCmd(L"FORM PS", 200, TEncoding::ASCII);
									break;
								FORM_TIFF:
									FTPctrl->SendCmd(L"FORM TIFF", 200, TEncoding::ASCII);
									break;
								FORM_PCL:
									FTPctrl->SendCmd(L"FORM PCL", 200, TEncoding::ASCII);
									break;
								FORM_PDF:
									FTPctrl->SendCmd(L"FORM PDF", 200, TEncoding::ASCII);
									break;
								default:
									;
								}

								//send STOT command
								FTPctrl->SendCmd(L"STOT", 150, TEncoding::ASCII);

								//send file
								FTPdata->IOHandler->WriteFile(file->FileName, false);

								//get answer: skip "FILE: " and keep till "(Opening"
								UnicodeString temp = FTPctrl->LastCmdResult->Text->Strings[0];
								temp = temp.SubString(7, temp.Pos(L" (Opening") - 7);
								RemoteFileNames->Add(temp);
							}
							__finally {
								FTPdata->Disconnect();
							}

							FTPctrl->GetResponse(-1, TEncoding::ASCII); // Discard
						}
					}

					bool first = true;

					//setup FAX
					for (nrdest = 0; nrdest < Numbers->Count; nrdest++) {
						UnicodeString Number = Trim(Numbers->Strings[nrdest]);

						//check number is not empty
						if (Number.Length() == 0)
							continue;

						//extract cover name and subaddress
						UnicodeString Recipient;
						if ((pos = Number.Pos(L"@")) != 0) {
							Recipient = Number.SubString(1, pos - 1).Trim();
							Number = Number.SubString(pos + 1, Number.Length()).Trim();
						} else {
							//try reverse lookup: number -> name
							int idx = AddressBook->IndexOfNumber(Number);
							Recipient = AddressBook->Names[idx];
                        }

						UnicodeString SubAddress;
						if ((pos = Number.Pos(L"#")) != 0) {
							SubAddress = Number.SubString(pos + 1, Number.Length()).Trim();
							Number = Number.SubString(1, pos - 1).Trim();
						}

						//check again
						if (Number.Length() == 0)
							continue;

						if (!first)
							FTPctrl->SendCmd(L"JOB DEFAULT", 200, TEncoding::ASCII);

						FTPctrl->SendCmd(L"JNEW", 200, TEncoding::ASCII);

						//send at specified date / time
						if (rbPostpone->Checked) {
							TDateTime dt = DateOf(hDate->Date) + TimeOf(hTime->Time);
							TDateTime utc = LocalDateTimeToDateTime(dt);
							Cmd = L"JPARM SENDTIME " +
							      Sysutils::FormatDateTime(L"yyyymmddHHnn", utc);
							FTPctrl->SendCmd(Cmd, 213, TEncoding::ASCII);
						}

						TVarRec args[3] = {
							StrToIntDef(hDays->Text, 0),
							HourOf(hHours->Time),
							MinuteOf(hHours->Time)
						};
						Cmd = Format(L"JPARM LASTTIME %.2d%.2d%.2d", args, 3);
						FTPctrl->SendCmd(Cmd, 213, TEncoding::ASCII);

						//dial string, cover name and subaddress
						Cmd = L"JPARM DIALSTRING \"" + Number + L"\"";
						FTPctrl->SendCmd(Cmd, 213, TEncoding::ASCII);

						if (Recipient.Length() > 0) {
							Cmd = L"JPARM TOUSER \"" + Recipient + L"\"";
							FTPctrl->SendCmd(Cmd, 213, TEncoding::ASCII);
						}

						if (SubAddress.Length() > 0) {
							Cmd = L"JPARM SUBADDR \"" + SubAddress + L"\"";
							FTPctrl->SendCmd(Cmd, 213, TEncoding::ASCII);
						}

						//Resolution
						switch (ConfigIni->Resolution) {
						frFine:
							FTPctrl->SendCmd(L"JPARM VRES 196", 213, TEncoding::ASCII);
							break;
						frSuperfine:
							FTPctrl->SendCmd(L"JPARM USEXVRES ON", 213, TEncoding::ASCII);
							FTPctrl->SendCmd(L"JPARM VRES 391", 213, TEncoding::ASCII);
							break;
						frUltrafine:
							FTPctrl->SendCmd(L"JPARM USEXVRES ON", 213, TEncoding::ASCII);
							FTPctrl->SendCmd(L"JPARM VRES 300", 213, TEncoding::ASCII);
							break;
						frHyperfine:
							FTPctrl->SendCmd(L"JPARM USEXVRES ON", 213, TEncoding::ASCII);
							FTPctrl->SendCmd(L"JPARM VRES 400", 213, TEncoding::ASCII);
							break;
						frStandard:
						default:
							FTPctrl->SendCmd(L"JPARM VRES 98", 213, TEncoding::ASCII);
							break;
						}

						//paper size
						if (ConfigIni->PageSize == fsA4) {
							FTPctrl->SendCmd(L"JPARM PAGEWIDTH 209", 213, TEncoding::ASCII);
							FTPctrl->SendCmd(L"JPARM PAGELENGTH 296", 213, TEncoding::ASCII);
						}

						//notification
						if (hNotifyEmail->Text.Length() > 0) {
							Cmd = L"JPARM NOTIFYADDR " + hNotifyEmail->Text;
							FTPctrl->SendCmd(Cmd, 213, TEncoding::ASCII);

							switch (ConfigIni->NotificationType) {
							fnSuccessOnly:
								FTPctrl->SendCmd(L"JPARM NOTIFY done", 213, TEncoding::ASCII);
								break;
							fnFailureOnly:
								FTPctrl->SendCmd(L"JPARM NOTIFY requeue", 213, TEncoding::ASCII);
								break;
							fnNone:
								FTPctrl->SendCmd(L"JPARM NOTIFY none", 213, TEncoding::ASCII);
								break;
							fnFailureSuccess:
							default:
								FTPctrl->SendCmd(L"JPARM NOTIFY done+requeue", 213, TEncoding::ASCII);
								break;
							}
						} else {
							FTPctrl->SendCmd(L"JPARM NOTIFY none", 213, TEncoding::ASCII);
						}

						//modem
						if (ConfigIni->Modem.Length() > 0) {
							Cmd = L"JPARM MODEM " + ConfigIni->Modem;
							FTPctrl->SendCmd(Cmd, 213, TEncoding::ASCII);
						}

						//documents to send
						for (i = 0; i < RemoteFileNames->Count; i++) {
							Cmd = L"JPARM DOCUMENT " + RemoteFileNames->Strings[i];
							FTPctrl->SendCmd(Cmd, 200, TEncoding::ASCII);
						}

						//go!
						FTPctrl->SendCmd(L"JSUBM", 200, TEncoding::ASCII);

						JobConfirm += (first ? L"" : L"\r\n") + FTPctrl->LastCmdResult->Text->Strings[0];

						first = false;
					}
				}
				__finally {
					FTPctrl->Disconnect();
				}

				//all fine
				MessageDlg(JobConfirm, mtInformation, TMsgDlgButtons() << mbOK, 0);
			}
			__finally
			{
				delete RemoteFileNames;
				delete Numbers;
			}

			i = 0;
			for (;;) {
				if (i >= lbDocuments->Items->Count)
					break;
				if (lbDocuments->Checked[i]) {
					TFileData *file = (TFileData*)lbDocuments->Items->Objects[i];
					//delete file only if it belongs to us
					if (SameFileName(ExtractFilePath(file->FileName), TConfigIni::WPHFTempDir))
						DeleteFile(file->FileName);
					delete file;
					lbDocuments->Items->Delete(i);
				} else {
					i++;
				}
			}
		}
		__finally {
			//whatever happened, enable UI again
			FSending = false;
			EnableFields(true);
		}
	}
	__finally {
		UnlockDocuments();
    }
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::actionsUpdate(TBasicAction *Action, bool &Handled)
{
	bool hasDoc = false;

	for (int i = 0; i < lbDocuments->Items->Count; i++)
		if (lbDocuments->Checked[i]) {
			hasDoc = true;
			break;
		}

	actSend->Enabled = !FSending &&
					   hasDoc &&
					   FHasNumber &&
					   ConfigIni->Server.Length() > 0;
	actUp->Enabled = !FSending &&
					 lbDocuments->Items->Count > 1 &&
					 lbDocuments->ItemIndex > 0;
	actDown->Enabled = !FSending &&
					   lbDocuments->Items->Count > 1 &&
					   lbDocuments->ItemIndex >= 0 &&
					   lbDocuments->ItemIndex < lbDocuments->Items->Count - 1;
	actSave->Enabled = !FSending &&
					   AddressBook &&
					   AddressBook->OnLine &&
					   !AddressBook->ReadOnly &&
					   FHasNumber &&
					   !FHasManyNumbers;
	actDelete->Enabled = !FSending &&
						 AddressBook &&
						 AddressBook->OnLine &&
						 !AddressBook->ReadOnly &&
						 Trim(ABNames->Text).Length() > 0 &&
						 ABNames->Text != SelectFromAB;
	actClose->Enabled = !FSending;
	actConfigure->Enabled = !FSending;
	actSelect->Enabled = !FSending &&
						 AddressBook &&
						 AddressBook->OnLine;
	Handled = true;
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	if (lbDocuments->Items->Count > 0 &&
	MessageDlg(_(L"Unsent faxes will be lost. Are you sure you want to exit?"),
	mtConfirmation, TMsgDlgButtons() << mbYes << mbNo, 0, mbNo) == mrNo)
	{
        CanClose = false;
    }
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::actUpExecute(TObject *Sender)
{
	int i = lbDocuments->ItemIndex;
	lbDocuments->Items->Move(i, i - 1);
	lbDocuments->ItemIndex = i - 1;
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::actDownExecute(TObject *Sender)
{
	int i = lbDocuments->ItemIndex;
	lbDocuments->Items->Move(i, i + 1);
	lbDocuments->ItemIndex = i + 1;
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::actSaveExecute(TObject *Sender)
{
	if (!AddressBook || !FHasNumber || FHasManyNumbers)
		return;

	UnicodeString Number = Trim(ReplaceStr(hFAXNumber->Text, L";", L""));

	if (Number.Length() == 0)
		return;

	UnicodeString RecipientName;

	TRecipientName *FRecipientName;

	FRecipientName = new TRecipientName(this);
	try {
		FRecipientName->FAXNumber->Text = Number;
		if (FRecipientName->ShowModal() == mrOk)
			RecipientName = Trim(FRecipientName->RecipientName->Text);
	}
	__finally {
		delete FRecipientName;
	}

	if (RecipientName.Length() > 0) {
		try {
			AddressBook->SetRecipient(RecipientName, Number);
			ABNames->Text = RecipientName;
		}
		catch (EAddressBookUnchanged& E)
		{
			//silently catch EAddressBookUnchanged
        }
	}
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::actDeleteExecute(TObject *Sender)
{
	if (!AddressBook || MessageDlg(_(L"Delete recipient?"), mtConfirmation,
	TMsgDlgButtons() << mbYes << mbNo, 0, mbNo) == mrNo)
    	return;
	AddressBook->DeleteRecipient(ABNames->Text);
	ABNames->Text = _(L"Select from address book");
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::actConfigureExecute(TObject *Sender)
{
	ConfigIni->Configure();
}

//---------------------------------------------------------------------------
void __fastcall TFAXSend::actCloseExecute(TObject *Sender)
{
	Close();
}

//---------------------------------------------------------------------------

void __fastcall TFAXSend::FormActivate(TObject *Sender)
{
	hFAXNumber->SetFocus();
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::FormConstrainedResize(TObject *Sender, int &MinWidth, int &MinHeight,
          int &MaxWidth, int &MaxHeight)
{
	MinHeight = 536;
	MinWidth = 446;
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::AppInstInstanceCreated(TObject *Sender, DWORD ProcessId)
{
	BringFaxWndToFront();
}
//---------------------------------------------------------------------------


void __fastcall TFAXSend::AppInstCmdLineReceived(TObject *Sender, TStrings *CmdLine)
{
	UnicodeString filename;
	//another instance started with command line.
	if (CmdLine->Count >= 2 &&
	Sysutils::FileExists(filename = ExpandUNCFileName(CmdLine->Strings[0]))) {
		LockDocuments();
		lbDocuments->Items->BeginUpdate();
		try {
			TFileData *file = new TFileData();
			file->FileName = filename;
			lbDocuments->AddItem(CmdLine->Strings[1], file);
			lbDocuments->Checked[lbDocuments->Items->Count - 1] = true;
		}
		__finally {
			lbDocuments->Items->EndUpdate();
			UnlockDocuments();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::lbDocumentsMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y)
{
	//drag'n'drop starts here
	DragPoint.x = X;
	DragPoint.y = Y;
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::lbDocumentsDragOver(TObject *Sender, TObject *Source, int X,
		  int Y, TDragState State, bool &Accept)
{
	//only accept dragging from ourselves
	Accept = (Source == lbDocuments);
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::lbDocumentsDragDrop(TObject *Sender, TObject *Source, int X,
		  int Y)
{
	//move elements
	TCheckListBox *lb = dynamic_cast<TCheckListBox *>(Sender);
	TPoint DropPoint;

	DropPoint.x = X;
	DropPoint.y = Y;
	int DropSource = lb->ItemAtPos(DragPoint, true);
	int DropTarget = lb->ItemAtPos(DropPoint, true);

	if (DropSource >= 0 && DropTarget >= 0 && DropSource != DropTarget) {
		lb->Items->Move(DropSource, DropTarget);
		lb->ItemIndex = DropTarget;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::mnuAddfileClick(TObject *Sender)
{
	if (opendlg->Execute()) {
		LockDocuments();
		lbDocuments->Items->BeginUpdate();
		try {
			TFileData *file = new TFileData();
			file->FileName = opendlg->FileName;

			lbDocuments->AddItem(opendlg->FileName, file);
			lbDocuments->Checked[lbDocuments->Items->Count - 1] = true;
		}
		__finally {
			lbDocuments->Items->EndUpdate();
			UnlockDocuments();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::actSelectExecute(TObject *Sender)
{
	if (!AddressBook)
		return;

	TSelectRcpt *frm = new TSelectRcpt(this);

	try {
		frm->Numbers = hFAXNumber->Text;
		if (frm->ShowModal() == mrOk)
			hFAXNumber->Text = frm->Numbers;
	}
	__finally {
		delete frm;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::mnuSelectAllClick(TObject *Sender)
{
	lbDocuments->CheckAll(cbChecked, false, false);
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::mnuSelectNoneClick(TObject *Sender)
{
	lbDocuments->CheckAll(cbUnchecked, false, false);
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::rbPostponeClick(TObject *Sender)
{
	hDate->Enabled = rbPostpone->Checked;
	hTime->Enabled = rbPostpone->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TFAXSend::hFAXNumberChange(TObject *Sender)
{
	int i, count = 0;
	bool bSemicolonSaw = false;
	bool bDigitSaw = false;

	UnicodeString Number = Trim(hFAXNumber->Text);

	for (i = 1; i <= Number.Length(); i++) {
		if (Number[i] >= L'0' && Number[i] <= L'9') {
			if (bSemicolonSaw) {
				count++;
				break;
			}

			if (!bDigitSaw) {
				count++;
				bDigitSaw = true;
            }
		} else if (Number[i] == L';') {
			bDigitSaw = false;
			bSemicolonSaw = true;
		}
	}

	FHasNumber = count > 0;
	FHasManyNumbers = count > 1;
}

//---------------------------------------------------------------------------

