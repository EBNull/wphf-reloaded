/*
WPHFMON - Winprint HylaFAX port monitor
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

#include "stdafx.h"
#include "portlist.h"
#include "pattern.h"
#include "log.h"
#include "..\common\autoclean.h"
#include "..\common\monutils.h"

CPortList* g_pPortList = NULL;
LPCWSTR CPortList::szOutputPathKey = L"OutputPath";
LPCWSTR CPortList::szFilePatternKey = L"FilePattern";
LPCWSTR CPortList::szOverwriteKey = L"Overwrite";
LPCWSTR CPortList::szUserCommandPatternKey = L"UserCommand";
LPCWSTR CPortList::szExecPathKey = L"ExecPath";
LPCWSTR CPortList::szWaitTerminationKey = L"WaitTermination";
LPCWSTR CPortList::szPipeDataKey = L"PipeData";
LPCWSTR CPortList::szLogLevelKey = L"LogLevel";

//-------------------------------------------------------------------------------------
CPortList::CPortList(LPCWSTR szMonitorName, LPCWSTR szPortDesc)
{
	InitializeCriticalSection(&m_CSPortList);
	wcscpy_s(m_szMonitorName, LENGTHOF(m_szMonitorName), szMonitorName);
	wcscpy_s(m_szPortDesc, LENGTHOF(m_szPortDesc), szPortDesc);
	m_pFirstPortRec = NULL;
}

//-------------------------------------------------------------------------------------
CPortList::~CPortList()
{
	LPPORTREC pNext = NULL;

	while (m_pFirstPortRec)
	{
		pNext = m_pFirstPortRec->m_pNext;
		delete m_pFirstPortRec;
		m_pFirstPortRec = pNext;
	}

	DeleteCriticalSection(&m_CSPortList);
}

//-------------------------------------------------------------------------------------
CPort* CPortList::FindPort(LPCWSTR szPortName)
{
	CAutoCriticalSection acs(CriticalSection());

	LPPORTREC pPortRec = m_pFirstPortRec;

	while (pPortRec)
	{
		if (_wcsicmp(pPortRec->m_pPort->PortName(), szPortName) == 0)
			break;
		pPortRec = pPortRec->m_pNext;
	}

	return pPortRec
		? pPortRec->m_pPort
		: NULL;
}

//-------------------------------------------------------------------------------------
DWORD CPortList::GetPortSize(LPCWSTR szPortName, DWORD dwLevel)
{
	DWORD cb = 0;

	switch (dwLevel)
	{
	case 1:
		cb = sizeof(PORT_INFO_1W) +
			 sizeof(WCHAR) * ((DWORD)wcslen(szPortName) + 1);
		break;
	case 2:
		cb = sizeof(PORT_INFO_2W) +
			 sizeof(WCHAR) * (
				(DWORD)wcslen(szPortName) + 1 +
				(DWORD)wcslen(m_szMonitorName) + 1 +
				(DWORD)wcslen(m_szPortDesc) + 1);
		break;
	default:
		break;
	}

	return cb;
}

//-------------------------------------------------------------------------------------
LPBYTE CPortList::CopyPortToBuffer(CPort* pPort, DWORD dwLevel, LPBYTE pStart, LPBYTE pEnd)
{
	size_t len = 0;

	switch (dwLevel)
	{
	case 1:
		{
			PORT_INFO_1W* pPortInfo = (PORT_INFO_1W*)pStart;
			len = wcslen(pPort->PortName()) + 1;
			pEnd -= len * sizeof(WCHAR);
			wcscpy_s((LPWSTR)pEnd, len, pPort->PortName());
			pPortInfo->pName = (LPWSTR)pEnd;
			break;
		}
	case 2:
		{
			PORT_INFO_2W* pPortInfo = (PORT_INFO_2W*)pStart;
			len = wcslen(m_szMonitorName) + 1;
			pEnd -= len * sizeof(WCHAR);
			wcscpy_s((LPWSTR)pEnd, len, m_szMonitorName);
			pPortInfo->pMonitorName = (LPWSTR)pEnd;

			len = wcslen(m_szPortDesc) + 1;
			pEnd -= len * sizeof(WCHAR);
			wcscpy_s((LPWSTR)pEnd, len, m_szPortDesc);
			pPortInfo->pDescription = (LPWSTR)pEnd;

			len = wcslen(pPort->PortName()) + 1;
			pEnd -= len * sizeof(WCHAR);
			wcscpy_s((LPWSTR)pEnd, len, pPort->PortName());
			pPortInfo->pPortName = (LPWSTR)pEnd;

			pPortInfo->fPortType = 0;
			pPortInfo->Reserved = 0;
			break;
		}
	default:
		break;
	}

    return pEnd;
}

//-------------------------------------------------------------------------------------
BOOL CPortList::EnumMultiFilePorts(HANDLE hMonitor, LPCWSTR pName, DWORD Level, LPBYTE pPorts,
	DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
	UNREFERENCED_PARAMETER(pName);
	UNREFERENCED_PARAMETER(hMonitor);

	CAutoCriticalSection acs(CriticalSection());

	LPPORTREC pPortRec = m_pFirstPortRec;

	DWORD cb = 0;
	while (pPortRec)
	{
		cb += GetPortSize(pPortRec->m_pPort->PortName(), Level);
		pPortRec = pPortRec->m_pNext;
	}

	*pcbNeeded = cb;

	if (cbBuf < *pcbNeeded)
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	LPBYTE pEnd = pPorts + cbBuf;
	*pcReturned = 0;
	pPortRec = m_pFirstPortRec;
	while (pPortRec)
	{
		pEnd = CopyPortToBuffer(pPortRec->m_pPort, Level, pPorts, pEnd);
		switch (Level)
		{
		case 1:
			pPorts += sizeof(PORT_INFO_1W);
			break;
		case 2:
			pPorts += sizeof(PORT_INFO_2W);
			break;
		default:
			{
				SetLastError(ERROR_INVALID_LEVEL);
				return FALSE;
			}
		}
		(*pcReturned)++;

		pPortRec = pPortRec->m_pNext;
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------
void CPortList::AddMfmPort(LPCWSTR szPortName/*, LPCWSTR szOutputPath, LPCWSTR szFilePattern, BOOL bOverwrite,
						   LPCWSTR szUserCommandPattern, LPCWSTR szExecPath, BOOL bWaitTermination, BOOL bPipeData*/)
{
	//alloc port on the heap
	CPort* pNewPort = new CPort(szPortName/*, szOutputPath, szFilePattern, bOverwrite,
		szUserCommandPattern, szExecPath, bWaitTermination, bPipeData*/);

	//add it to the list
	AddMfmPort(pNewPort);
}

//-------------------------------------------------------------------------------------
void CPortList::AddMfmPort(CPort* pNewPort)
{
	CAutoCriticalSection acs(CriticalSection());

	LPPORTREC pPortRec = new PORTREC;

	pPortRec->m_pPort = pNewPort;
	pPortRec->m_pNext = m_pFirstPortRec;
	m_pFirstPortRec = pPortRec;

	g_pLog->Log(LOGLEVEL_ALL, L"port %s up and running", pNewPort->PortName());
}

//-------------------------------------------------------------------------------------
void CPortList::DeletePort(CPort* pPortToDelete)
{
	CAutoCriticalSection acs(CriticalSection());

	LPPORTREC pPortRec = m_pFirstPortRec, pPrevious = NULL;

	while (pPortRec)
	{
		if (pPortRec->m_pPort == pPortToDelete)
		{
			g_pLog->Log(LOGLEVEL_ALL, pPortToDelete, L"removing port");

			if (pPrevious)
				pPrevious->m_pNext = pPortRec->m_pNext;
			else
				m_pFirstPortRec = pPortRec->m_pNext;

			RemoveFromRegistry(pPortToDelete);

			delete pPortRec;

			break;
		}

		pPrevious = pPortRec;
		pPortRec = pPortRec->m_pNext;
	}
}

//-------------------------------------------------------------------------------------
void CPortList::RemoveFromRegistry(CPort* pPort)
{
	UNREFERENCED_PARAMETER(pPort);
/*
	PMONITORREG pReg = g_pMonitorInit->pMonitorReg;
	HKEY hRoot = g_pMonitorInit->hckRegistryRoot;

	//If we're on an UAC enabled system, we're running under unprivileged
	//user account. Let's revert to ourselves for a while...
	HANDLE hToken = NULL;
	if (IsUACEnabled())
	{
		g_pLog->Log(LOGLEVEL_ALL, L"running on UAC enabled OS, reverting to local system");
		OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &hToken);
		RevertToSelf();
	}

	pReg->fpDeleteKey(hRoot, pPort->PortName(), g_pMonitorInit->hSpooler);

	//let's revert to unprivileged user
	if (hToken)
	{
		SetThreadToken(NULL, hToken);
		CloseHandle(hToken);
		g_pLog->Log(LOGLEVEL_ALL, L"back to unprivileged user");
	}
*/
}

//-------------------------------------------------------------------------------------
void CPortList::LoadFromRegistry()
{
/*
	WCHAR szPortName[MAX_PATH];
	WCHAR szOutputPath[MAX_PATH];
	WCHAR szFilePattern[MAX_PATH];
	WCHAR szUserCommandPattern[MAXUSERCOMMMAND];
	WCHAR szExecPath[MAX_PATH];
	BOOL bOverwrite;
	BOOL bWaitTermination;
	BOOL bPipeData;
	int nLogLevel;
#ifdef __GNUC__
	HANDLE hKey;
#else
	HKEY hKey;
#endif
	PMONITORREG pReg = g_pMonitorInit->pMonitorReg;
	HKEY hRoot = g_pMonitorInit->hckRegistryRoot;
	DWORD index = 0;
	DWORD cchName;
	DWORD cbOutputPath;
	DWORD cbFilePattern;
	DWORD cbOverwrite;
	DWORD cbUserCommandPattern;
	DWORD cbExecPath;
	DWORD cbWaitTermination;
	DWORD cbPipeData;
	DWORD cbLogLevel;

	cbLogLevel = sizeof(nLogLevel);
	if (pReg->fpQueryValue(hRoot, szLogLevelKey, NULL, (LPBYTE)&nLogLevel, &cbLogLevel,
		g_pMonitorInit->hSpooler) != ERROR_SUCCESS)
		nLogLevel = LOGLEVEL_NONE;

	g_pLog->SetLogLevel(nLogLevel);

	for (;;)
	{
		//read port name
		cchName = LENGTHOF(szPortName);
		LONG res = pReg->fpEnumKey(hRoot, index++, szPortName, &cchName, NULL, g_pMonitorInit->hSpooler);
		if (res == ERROR_NO_MORE_ITEMS)
			break;
		else if (res == ERROR_MORE_DATA)
			continue;

		//open port registry key
		if (pReg->fpOpenKey(hRoot, szPortName, KEY_QUERY_VALUE, &hKey, g_pMonitorInit->hSpooler) != ERROR_SUCCESS)
			continue;

		//read OutputPath
		cbOutputPath = sizeof(szOutputPath);
		if (pReg->fpQueryValue(hKey, szOutputPathKey, NULL, (LPBYTE)szOutputPath, &cbOutputPath,
			g_pMonitorInit->hSpooler) != ERROR_SUCCESS)
			continue;
		else
			szOutputPath[cbOutputPath / sizeof(WCHAR)] = L'\0';

		//read FilePattern
		cbFilePattern = sizeof(szFilePattern);
		if (pReg->fpQueryValue(hKey, szFilePatternKey, NULL, (LPBYTE)szFilePattern, &cbFilePattern,
			g_pMonitorInit->hSpooler) != ERROR_SUCCESS)
			continue;
		else
			szFilePattern[cbFilePattern / sizeof(WCHAR)] = L'\0';
		if (cbFilePattern == 0)
			wcscpy_s(szFilePattern, LENGTHOF(szFilePattern), CPattern::szDefaultFilePattern);

		//read Overwrite
		cbOverwrite = sizeof(bOverwrite);
		if (pReg->fpQueryValue(hKey, szOverwriteKey, NULL, (LPBYTE)&bOverwrite, &cbOverwrite,
			g_pMonitorInit->hSpooler) != ERROR_SUCCESS)
			continue;

		//read UserCommand
		cbUserCommandPattern = sizeof(szUserCommandPattern);
		if (pReg->fpQueryValue(hKey, szUserCommandPatternKey, NULL, (LPBYTE)szUserCommandPattern,
			&cbUserCommandPattern, g_pMonitorInit->hSpooler) != ERROR_SUCCESS)
			*szUserCommandPattern = L'\0';
		else
			szUserCommandPattern[cbUserCommandPattern / sizeof(WCHAR)] = L'\0';

		//read ExecPath
		cbExecPath = sizeof(szExecPath);
		if (pReg->fpQueryValue(hKey, szExecPathKey, NULL, (LPBYTE)szExecPath, &cbExecPath,
			g_pMonitorInit->hSpooler) != ERROR_SUCCESS)
			*szExecPath = L'\0';
		else
			szExecPath[cbExecPath / sizeof(WCHAR)] = L'\0';

		//read Wait termination
		cbWaitTermination = sizeof(bWaitTermination);
		if (pReg->fpQueryValue(hKey, szWaitTerminationKey, NULL, (LPBYTE)&bWaitTermination, &cbWaitTermination,
			g_pMonitorInit->hSpooler) != ERROR_SUCCESS)
			bWaitTermination = FALSE;

		//read Pipe data
		cbPipeData = sizeof(bPipeData);
		if (pReg->fpQueryValue(hKey, szPipeDataKey, NULL, (LPBYTE)&bPipeData, &cbPipeData,
			g_pMonitorInit->hSpooler) != ERROR_SUCCESS)
			bPipeData = FALSE;

		//close registry
		pReg->fpCloseKey(hKey, g_pMonitorInit->hSpooler);

		//add the port
		AddMfmPort(szPortName, szOutputPath, szFilePattern, bOverwrite, szUserCommandPattern,
			szExecPath, bWaitTermination, bPipeData);
	}
*/
	AddMfmPort(L"WPHF:");
}

//-------------------------------------------------------------------------------------
void CPortList::SaveToRegistry()
{
/*
#ifdef __GNUC__
	HANDLE hKey;
#else
	HKEY hKey;
#endif
	int nLogLevel;
	PMONITORREG pReg = g_pMonitorInit->pMonitorReg;
	HKEY hRoot = g_pMonitorInit->hckRegistryRoot;

	CAutoCriticalSection acs(CriticalSection());

	LPPORTREC pPortRec = m_pFirstPortRec;

	//If we're on an UAC enabled system, we're running under unprivileged
	//user account. Let's revert to ourselves for a while...
	HANDLE hToken = NULL;
	if (IsUACEnabled())
	{
		g_pLog->Log(LOGLEVEL_ALL, L"running on UAC enabled OS, reverting to local system");
		OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &hToken);
		RevertToSelf();
	}

	nLogLevel = g_pLog->GetLogLevel();
	pReg->fpSetValue(hRoot, szLogLevelKey, REG_DWORD, (LPBYTE)&nLogLevel, sizeof(nLogLevel),
		g_pMonitorInit->hSpooler);

	while (pPortRec)
	{
		if (pReg->fpCreateKey(hRoot, pPortRec->m_pPort->PortName(), 0, KEY_WRITE,
			NULL, &hKey, NULL, g_pMonitorInit->hSpooler) == ERROR_SUCCESS)
		{
			LPCWSTR szBuf;

			//OutputPath
			szBuf = pPortRec->m_pPort->OutputPath();
			pReg->fpSetValue(hKey, szOutputPathKey, REG_SZ, (LPBYTE)szBuf,
				(DWORD)wcslen(szBuf) * sizeof(WCHAR), g_pMonitorInit->hSpooler);

			//FilePattern
			szBuf = pPortRec->m_pPort->FilePattern();
			pReg->fpSetValue(hKey, szFilePatternKey, REG_SZ, (LPBYTE)szBuf,
				(DWORD)wcslen(szBuf) * sizeof(WCHAR), g_pMonitorInit->hSpooler);

			//Overwrite
			BOOL bOverwrite = pPortRec->m_pPort->Overwrite();
			pReg->fpSetValue(hKey, szOverwriteKey, REG_DWORD, (LPBYTE)&bOverwrite,
				sizeof(bOverwrite), g_pMonitorInit->hSpooler);

			//UserCommand
			szBuf = pPortRec->m_pPort->UserCommandPattern();
			pReg->fpSetValue(hKey, szUserCommandPatternKey, REG_SZ, (LPBYTE)szBuf,
				(DWORD)wcslen(szBuf) * sizeof(WCHAR), g_pMonitorInit->hSpooler);

			//OutputPath
			szBuf = pPortRec->m_pPort->ExecPath();
			pReg->fpSetValue(hKey, szExecPathKey, REG_SZ, (LPBYTE)szBuf,
				(DWORD)wcslen(szBuf) * sizeof(WCHAR), g_pMonitorInit->hSpooler);

			//Wait termination
			BOOL bWaitTermination = pPortRec->m_pPort->WaitTermination();
			pReg->fpSetValue(hKey, szWaitTerminationKey, REG_DWORD, (LPBYTE)&bWaitTermination,
				sizeof(bWaitTermination), g_pMonitorInit->hSpooler);

			//Pipe data
			BOOL bPipeData = pPortRec->m_pPort->PipeData();
			pReg->fpSetValue(hKey, szPipeDataKey, REG_DWORD, (LPBYTE)&bPipeData,
				sizeof(bPipeData), g_pMonitorInit->hSpooler);

			//close registry
			pReg->fpCloseKey(hKey, g_pMonitorInit->hSpooler);
		}

		pPortRec = pPortRec->m_pNext;
	}

	//let's revert to unprivileged user
	if (hToken)
	{
		SetThreadToken(NULL, hToken);
		CloseHandle(hToken);
		g_pLog->Log(LOGLEVEL_ALL, L"back to unprivileged user");
	}
*/
}

