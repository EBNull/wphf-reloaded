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
#include <mapi.h>
#include <mapix.h>
#include <mapiutil.h>
#include <map>
#include <gnugettext.hpp>
#pragma hdrstop

#include "AddrBookMAPI.h"
#include "ConfIni.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

//---------------------------------------------------------------------------

#define SAFE_RELEASE(x) \
	if (x) { (x)->Release(); (x) = NULL; }

//---------------------------------------------------------------------------

#define CRLF L"\r\n";

#ifdef MAPIDEBUG
UnicodeString MAPImsg;
#endif

//---------------------------------------------------------------------------

__fastcall TAddressBookMAPI::TAddressBookMAPI()
	: TAddressBook()
{
	FIsReadOnly = true;
}
//---------------------------------------------------------------------------

void __fastcall TAddressBookMAPI::GetContents(LPMAPICONTAINER pContainer,
#ifdef MAPIDEBUG
	const UnicodeString& containerName,
#endif
	int &unknown)
{
	LPMAPITABLE pCont = NULL;
	HRESULT hr;

	//we only want fax numbers
	SizedSPropTagArray(3, columns) = {
		3,
		PR_DISPLAY_NAME,
		PR_ADDRTYPE,
		PR_EMAIL_ADDRESS,
	};

	//get contents table
	if (SUCCEEDED(hr = pContainer->GetContentsTable(MAPI_UNICODE, &pCont))) {
#ifdef MAPIDEBUG
		MAPImsg += L"reading " + containerName + CRLF;
		int cnt = 0;
#endif
		try {
			//set columns to retrieve
			if (FAILED(hr = pCont->SetColumns((LPSPropTagArray)&columns, 0))) {
#ifdef MAPIDEBUG
				MAPImsg += containerName + L": contents table: SetColumns failed: 0x" +
						   IntToHex((int)hr, 8) + CRLF;
#endif
			}

			bool bEOF = false;

			while (!bEOF) {
				LPSRowSet pRows = NULL;

				try {
					if (FAILED(hr = pCont->QueryRows(50, 0, &pRows)))
						throw new EAddressBookException(_(L"Unable to query address book contents table: 0x") + IntToHex((int)hr, 8));

					for (ULONG row = 0; row < pRows->cRows; row++) {
#ifdef MAPIDEBUG
						cnt++;
#endif

						LPSPropValue PV_disp = PpropFindProp(pRows->aRow[row].lpProps,
							pRows->aRow[row].cValues, PR_DISPLAY_NAME);
						LPSPropValue PV_addr = PpropFindProp(pRows->aRow[row].lpProps,
							pRows->aRow[row].cValues, PR_ADDRTYPE);
						LPSPropValue PV_mail = PpropFindProp(pRows->aRow[row].lpProps,
							pRows->aRow[row].cValues, PR_EMAIL_ADDRESS);

						//get normalized subject
						UnicodeString addr = PV_addr
							? PV_addr->Value.lpszW
							: L"";

						if (addr.CompareIC(L"FAX") != 0)
							continue;

						UnicodeString disp;
						if (PV_disp)
							disp = PV_disp->Value.lpszW;
						else
							disp = L"Unknown" + IntToStr(++unknown);

						//add number to our address book
						if (PV_mail && *PV_mail->Value.lpszW)
							FNames->AddObject(disp,
							new TFaxNumber(PV_mail->Value.lpszW));

#ifdef MAPIDEBUG
						if (PV_mail && *PV_mail->Value.lpszW)
							MAPImsg = MAPImsg + L"FAX found" + CRLF;
#endif
					}
				}
				__finally {
					if (pRows) {
						bEOF = (pRows->cRows == 0);

						FreeProws(pRows);
					}
				}
			}
		}
		__finally {
			SAFE_RELEASE(pCont);
		}
#ifdef MAPIDEBUG
		MAPImsg += containerName + L": read " + IntToStr(cnt) + L" entries" + CRLF;
#endif
	} else {
#ifdef MAPIDEBUG
		MAPImsg += containerName + L": GetContentsTable failed: 0x" +
				   IntToHex((int)hr, 8) + CRLF;
#endif
	}
}
//---------------------------------------------------------------------------

void __fastcall TAddressBookMAPI::ScanContainer(LPMAPICONTAINER pContainer,
#ifdef MAPIDEBUG
	const UnicodeString& containerName,
#endif
	int &unknown)
{
	LPMAPITABLE pHier = NULL;
	HRESULT hr;

	//we only want entry id
#ifdef MAPIDEBUG
	SizedSPropTagArray(2, columns) = {
		2,
		PR_ENTRYID,
		PR_DISPLAY_NAME
	};
#else
	SizedSPropTagArray(1, columns) = {
		1,
		PR_ENTRYID
	};
#endif

	//get hierarchy table
	if (SUCCEEDED(hr = pContainer->GetHierarchyTable(MAPI_UNICODE, &pHier))) {
#ifdef MAPIDEBUG
		MAPImsg += L"entering " + containerName + CRLF;
#endif
		try {
			//set columns to retrieve
			if (FAILED(hr = pHier->SetColumns((LPSPropTagArray)&columns, 0))) {
#ifdef MAPIDEBUG
				MAPImsg += containerName + L": hierarchy table: SetColumns failed: 0x" +
						   IntToHex((int)hr, 8) + CRLF;
#endif
			}

			bool bEOF = false;

			while (!bEOF) {
				LPSRowSet pRows = NULL;

				try {
					if (FAILED(hr = pHier->QueryRows(100, 0, &pRows)))
						throw new EAddressBookException(_(L"Unable to query address book hierarchy table: 0x") + IntToHex((int)hr, 8));

					for (ULONG row = 0; row < pRows->cRows; row++) {
						//get entry id of our subobject
						LPSPropValue PV_entryid = PpropFindProp(pRows->aRow[row].lpProps,
							pRows->aRow[row].cValues, PR_ENTRYID);

						LPSPropValue PV_dispname = PpropFindProp(pRows->aRow[row].lpProps,
							pRows->aRow[row].cValues, PR_DISPLAY_NAME);

						union _PV *SubContEntryId = &PV_entryid->Value;

#ifdef MAPIDEBUG
						UnicodeString subContName = PV_dispname
							? PV_dispname->Value.lpszW
							: L"";
#endif

						//oops somethig went wrong? go on
						if (!SubContEntryId) {
#ifdef MAPIDEBUG
							MAPImsg += containerName + L": SubContEntryId is NULL" + CRLF;
#endif
							continue;
						}

						LPMAPICONTAINER pSubCont = NULL;
						ULONG objectType = 0;

						//open sub-container
						if (FAILED(hr = pContainer->OpenEntry(SubContEntryId->bin.cb,
						(LPENTRYID)SubContEntryId->bin.lpb,
						NULL, MAPI_BEST_ACCESS, &objectType,
						(LPUNKNOWN *)&pSubCont)))
							throw new EAddressBookException(_(L"Unable to open address book sub container: 0x") + IntToHex((int)hr, 8));

						try {
							//recurse into sub-container
							if (objectType == MAPI_ABCONT)
								ScanContainer(pSubCont,
#ifdef MAPIDEBUG
									subContName,
#endif
									unknown);
						}
						__finally {
							pSubCont->Release();
						}
					}
				}
				__finally {
					if (pRows) {
						bEOF = (pRows->cRows == 0);

						FreeProws(pRows);
					}
				}
			}
		}
		__finally {
			pHier->Release();
		}
#ifdef MAPIDEBUG
		MAPImsg += L"leaving " + containerName + CRLF;
#endif
	} else {
#ifdef MAPIDEBUG
		MAPImsg += containerName + L": GetHierarchyTable failed: 0x" +
				   IntToHex((int)hr, 8) + CRLF;
#endif
	}

	//ok get this container's addresses
	GetContents(pContainer,
#ifdef MAPIDEBUG
		containerName,
#endif
		unknown);
}
//---------------------------------------------------------------------------

void __fastcall TAddressBookMAPI::Load()
{
	LPMAPISESSION pSess = NULL;
	LPADRBOOK pAddrBook = NULL;
	LPMAPICONTAINER pRootCont = NULL;
	HRESULT hr, hrMapi = E_FAIL;

	try {
		//initialize mapi
		if (FAILED(hrMapi = ::MAPIInitialize(NULL)))
			throw new EAddressBookException(_(L"Unable to initialize MAPI: 0x") + IntToHex((int)hrMapi, 8));

		LPWSTR profile = NULL;
		ULONG flags = MAPI_NEW_SESSION |
					  MAPI_EXTENDED |
					  MAPI_NO_MAIL |
					  MAPI_UNICODE;
		if (ConfigIni->MAPIUseDefProfile)
			flags |= MAPI_USE_DEFAULT;
		else
			profile = ConfigIni->MAPIProfile.c_str();

		//logon to mapi
		if (FAILED(hr = ::MAPILogonEx(0, profile, NULL, flags, &pSess))) {
			//failed? maybe a password is required. try with logon_ui
			flags |= MAPI_LOGON_UI;
			hr = ::MAPILogonEx(0, profile, NULL, flags, &pSess);
		}

		//failed again... give up
		if (FAILED(hr))
			throw new EAddressBookException(_(L"Unable to logon to MAPI: 0x") + IntToHex((int)hr, 8));

		try {
			//open address book
			if (FAILED(hr = pSess->OpenAddressBook(0, NULL, 0, &pAddrBook)))
				throw new EAddressBookException(_(L"Unable to open MAPI address book: 0x") + IntToHex((int)hr, 8));

			//open container
			ULONG obj_type;
			if (FAILED(hr = pAddrBook->OpenEntry(0, NULL, NULL, 0, &obj_type, (LPUNKNOWN *)&pRootCont)))
				throw new EAddressBookException(_(L"Unable to open MAPI address book container: 0x") + IntToHex((int)hr, 8));

#ifdef MAPIDEBUG
			MAPImsg = L"";
#endif
			//traverse container, subcontainers and elements
			int unknown = 0;
			ScanContainer(pRootCont,
#ifdef MAPIDEBUG
				L"root",
#endif
				unknown);
#ifdef MAPIDEBUG
			if (MAPImsg.Length() > 0) {
				TFileStream *log = new TFileStream(ConfigIni->WPHFUserDir + L"wphfmapi.log", fmCreate);
				try {
					WORD BOM = 0xFEFF;
					log->WriteBuffer(&BOM, sizeof(BOM));
					log->WriteBuffer(MAPImsg.c_str(), MAPImsg.Length() * sizeof(wchar_t));
				}
				__finally {
					delete log;
				}
			}
#endif

			//all OK
			FIsOnLine = true;
		}
		__finally {
			pSess->Logoff(0, 0, 0);
		}
	}
	__finally {
		//cleanup
		SAFE_RELEASE(pRootCont);

		SAFE_RELEASE(pAddrBook);

		SAFE_RELEASE(pSess);

		if (SUCCEEDED(hrMapi))
			MAPIUninitialize();
	}

	//fire event
	if (FOnAddressBookChanged)
		FOnAddressBookChanged(this);
}
//---------------------------------------------------------------------------

