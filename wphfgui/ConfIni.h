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

#ifndef ConfIniH
#define ConfIniH

enum TFaxResolution {
	frStandard,
	frFine,
	frSuperfine,
	frUltrafine,
	frHyperfine,
};

enum TFaxPageSize {
	fsUSLetter,
	fsA4,
};

enum TFaxNotification {
	fnFailureSuccess,
	fnSuccessOnly,
	fnFailureOnly,
	fnNone,
};

enum TAddressBookType {
	abNone,
	abCSV,
	abMAPI,
	abWinContacts,
	abODBC,
};

typedef void __fastcall (__closure *TAddrBookTypeChangeEvent)(TObject *, TAddressBookType);
typedef void __fastcall (__closure *TAddrBookLocationChangeEvent)(TObject *);
typedef void __fastcall (__closure *TLanguageChangeEvent)(TObject *);

class TConfigIni : public TObject
{
private:
	UnicodeString FServer;
	UnicodeString FUsername;
	UnicodeString FPassword;
	UnicodeString FEmail;
	bool FPassiveIPIgnore;
	TFaxResolution FResolution;
	TFaxPageSize FPageSize;
	TFaxNotification FNotificationType;
	UnicodeString FModem;
	UnicodeString FIniFile;
	UnicodeString FAddrBookPath;
	UnicodeString FLanguage;
	UnicodeString FODBCDSN;
	UnicodeString FODBCTable;
	UnicodeString FODBCNameField;
	UnicodeString FODBCFaxField;
	bool FODBCAuth;
	UnicodeString FODBCUid;
	UnicodeString FODBCPwd;
	TAddressBookType FAddrBookType;
	bool FAddrBookLocationChanged, FAddrBookTypeChanged, FLanguageChanged;
	bool FMAPIUseDefProfile;
	UnicodeString FMAPIProfile;
	TAddrBookTypeChangeEvent FOnAddrBookTypeChanged;
	TAddrBookLocationChangeEvent FOnAddrBookLocationChanged;
	TLanguageChangeEvent FOnLanguageChanged;
	UnicodeString __fastcall GetDefaultAddrBookPath();
	void __fastcall SetAddrBookType(TAddressBookType Value);
	void __fastcall SetAddrBookPath(const UnicodeString& Value);
	void __fastcall SetLanguage(const UnicodeString& Value);
	void __fastcall SetMAPIUseDefProfile(bool Value);
	void __fastcall SetMAPIProfile(const UnicodeString& Value);
	void __fastcall SetODBCDSN(const UnicodeString& Value);
	void __fastcall SetODBCTable(const UnicodeString& Value);
	void __fastcall SetODBCNameField(const UnicodeString& Value);
	void __fastcall SetODBCFaxField(const UnicodeString& Value);
	void __fastcall SetODBCAuth(bool Value);
	void __fastcall SetODBCUid(const UnicodeString& Value);
	void __fastcall SetODBCPwd(const UnicodeString& Value);
	void __fastcall FireEvents();
public:
	static UnicodeString WPHFUserDir;
	static UnicodeString WPHFTempDir;
	static UnicodeString oldWPHFDir;
	static bool MovingConfigFiles;
	__fastcall TConfigIni(const UnicodeString& IniFile);
	void __fastcall Configure();
	void __fastcall Load();
	void __fastcall Save();
	__property UnicodeString Server = { read = FServer, write = FServer };
	__property UnicodeString Username = { read = FUsername, write = FUsername };
	__property UnicodeString Password = { read = FPassword, write = FPassword };
	__property UnicodeString Email = { read = FEmail, write = FEmail };
	__property bool PassiveIPIgnore = { read = FPassiveIPIgnore, write = FPassiveIPIgnore };
	__property TFaxResolution Resolution = { read = FResolution, write = FResolution };
	__property TFaxPageSize PageSize = { read = FPageSize, write = FPageSize };
	__property TFaxNotification NotificationType = { read = FNotificationType, write = FNotificationType };
	__property UnicodeString Modem = { read = FModem, write = FModem };
	__property TAddressBookType AddrBookType = { read = FAddrBookType, write = SetAddrBookType };
	__property UnicodeString AddrBookPath = { read = FAddrBookPath, write = SetAddrBookPath };
	__property UnicodeString DefaultAddrBookPath = { read = GetDefaultAddrBookPath };
	__property UnicodeString Language = { read = FLanguage, write = SetLanguage };
	__property bool MAPIUseDefProfile = { read = FMAPIUseDefProfile, write = SetMAPIUseDefProfile };
	__property UnicodeString MAPIProfile = { read = FMAPIProfile, write = SetMAPIProfile };
	__property TAddrBookTypeChangeEvent OnAddrBookTypeChanged =
		{ read = FOnAddrBookTypeChanged, write = FOnAddrBookTypeChanged };
	__property TAddrBookLocationChangeEvent OnAddrBookLocationChanged =
		{ read = FOnAddrBookLocationChanged, write = FOnAddrBookLocationChanged };
	__property TLanguageChangeEvent OnLanguageChanged =
		{ read = FOnLanguageChanged, write = FOnLanguageChanged };
	__property UnicodeString ODBCDSN = { read = FODBCDSN, write = SetODBCDSN };
	__property UnicodeString ODBCTable = { read = FODBCTable, write = SetODBCTable };
	__property UnicodeString ODBCNameField = { read = FODBCNameField, write = SetODBCNameField };
	__property UnicodeString ODBCFaxField = { read = FODBCFaxField, write = SetODBCFaxField };
	__property bool ODBCAuth = { read = FODBCAuth, write = SetODBCAuth };
	__property UnicodeString ODBCUid = { read = FODBCUid, write = SetODBCUid };
	__property UnicodeString ODBCPwd = { read = FODBCPwd, write = SetODBCPwd };
};

extern TConfigIni *ConfigIni;

//---------------------------------------------------------------------------
#endif
