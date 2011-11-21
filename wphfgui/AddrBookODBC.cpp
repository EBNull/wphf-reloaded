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
#include <sqltypes.h>
#include <sqlext.h>
#pragma hdrstop

#include "AddrBookODBC.h"
#include "ConfIni.h"

#define NAME_LEN 256
#define PHONE_LEN 256

//---------------------------------------------------------------------------

#pragma package(smart_init)

//---------------------------------------------------------------------------

__fastcall TAddressBookODBC::TAddressBookODBC()
	: TAddressBook()
{
	FIsReadOnly = true;
}
//---------------------------------------------------------------------------

UnicodeString __fastcall GetSQLMessages(SQLSMALLINT HandleType,
	SQLHANDLE Handle)
{
	SQLWCHAR SqlState[6], SQLStmt[100], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER NativeError;
	SQLSMALLINT MsgLen, i;
	UnicodeString temp, result;

	i = 1;
	while ((SQLGetDiagRecW(HandleType, Handle, i, SqlState, &NativeError,
	Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) {
		temp.sprintf(L"SqlState=%s, NativeError=%i, Msg=%s",
			SqlState, NativeError, Msg);
		result += temp;
        i++;
	}

	return result;
}

void __fastcall TAddressBookODBC::Load()
{
	const wchar_t *szSelectTmpl =
		L"SELECT %s, %s \n"
		L"FROM %s \n"
		L"WHERE (%s IS NOT NULL) AND (%s <> '') \n"
		L"AND (%s IS NOT NULL) AND (%s <> '') \n"
		L"ORDER BY %s";
	SQLWCHAR szSelect[1024];
	SQLRETURN rc;
	SQLHANDLE hEnv = NULL, hCon = NULL, hStmt = NULL;
	SQLWCHAR szName[NAME_LEN] = {0}, szPhone[PHONE_LEN] = {0};
	SQLLEN cbName = 0, cbPhone = 0;

	try {
		//hEnv
		rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to initialize ODBC: ") +
			GetSQLMessages(SQL_HANDLE_ENV, SQL_NULL_HANDLE));

		rc = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to setup ODBC: ") +
			GetSQLMessages(SQL_HANDLE_ENV, hEnv));

		//hCon
		rc = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hCon);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to create ODBC connection: ") +
			GetSQLMessages(SQL_HANDLE_ENV, hEnv));

		rc = SQLSetConnectAttrW(hCon, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to setup ODBC connection: ") +
			GetSQLMessages(SQL_HANDLE_ENV, hEnv));

		rc = SQLSetConnectAttrW(hCon, SQL_ATTR_ACCESS_MODE, (SQLPOINTER)SQL_MODE_READ_ONLY, 0);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to setup ODBC connection: ") +
			GetSQLMessages(SQL_HANDLE_ENV, hEnv));

		//connect
		if (ConfigIni->ODBCAuth)
			rc = SQLConnectW(hCon, ConfigIni->ODBCDSN.c_str(), SQL_NTS,
			ConfigIni->ODBCUid.c_str(), SQL_NTS,
			ConfigIni->ODBCPwd.c_str(), SQL_NTS);
		else
			rc = SQLConnectW(hCon, ConfigIni->ODBCDSN.c_str(), SQL_NTS,
			NULL, 0, NULL, 0);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to open ODBC connection: ") +
			GetSQLMessages(SQL_HANDLE_DBC, hCon));

		//get identifier quote char
		SQLWCHAR buf[8], *szQuoteChar;
		SQLSMALLINT len;
		rc = SQLGetInfoW(hCon, SQL_IDENTIFIER_QUOTE_CHAR, buf, (SQLSMALLINT)sizeof(buf), &len);
		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
			szQuoteChar = buf;
		else
			szQuoteChar = L"\0";

		//hStmt
		rc = SQLAllocHandle(SQL_HANDLE_STMT, hCon, &hStmt);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to create ODBC statement: ") +
			GetSQLMessages(SQL_HANDLE_DBC, hCon));

		UnicodeString stmt, name, fax, table;
		//quote all identifiers
		table.sprintf(L"%s%s%s", szQuoteChar, ConfigIni->ODBCTable, szQuoteChar);
		name.sprintf(L"%s%s%s", szQuoteChar, ConfigIni->ODBCNameField, szQuoteChar);
		fax.sprintf(L"%s%s%s", szQuoteChar, ConfigIni->ODBCFaxField, szQuoteChar);

		//build SQL statement and execute it
		stmt.sprintf(szSelectTmpl,
			name, fax,			//SELECT
			table,				//FROM
			name, name,			//WHERE
			fax, fax,			//AND
			name				//ORDER BY
		);
		rc = SQLExecDirectW(hStmt, (SQLWCHAR *)stmt.c_str(), SQL_NTS);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to execute ODBC statement: ") +
			GetSQLMessages(SQL_HANDLE_STMT, hStmt));

		//bind columns
		rc = SQLBindCol(hStmt, 1, SQL_C_WCHAR, szName, NAME_LEN, &cbName);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to bind ODBC column: ") +
			GetSQLMessages(SQL_HANDLE_STMT, hStmt));

		rc = SQLBindCol(hStmt, 2, SQL_C_WCHAR, szPhone, PHONE_LEN, &cbPhone);
		if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			throw new EAddressBookException(_(L"Unable to bind ODBC column: ") +
			GetSQLMessages(SQL_HANDLE_STMT, hStmt));

		//fetch data
		do {
			rc = SQLFetch(hStmt);
			if (rc == SQL_ERROR) {
				throw new EAddressBookException(_(L"Unable to fetch ODBC data: ") +
				GetSQLMessages(SQL_HANDLE_STMT, hStmt));
			} else if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
				FNames->AddObject(szName, new TFaxNumber(szPhone));
			}
		} while (rc != SQL_NO_DATA);

		FIsOnLine = true;
	}
	__finally {
		if (hStmt)
			SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

		if (hCon) {
			SQLDisconnect(hCon);
			SQLFreeHandle(SQL_HANDLE_DBC, hCon);
		}

		if (hEnv)
			SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
	}

	if (FOnAddressBookChanged)
		FOnAddressBookChanged(this);
}
//---------------------------------------------------------------------------

