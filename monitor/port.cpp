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
#include "port.h"
#include "log.h"
#include "..\common\autoclean.h"
#include "..\common\defs.h"
#include "..\common\monutils.h"

typedef struct _LSA_UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING, UNICODE_STRING, *PUNICODE_STRING;

typedef NTSYSAPI NTSTATUS NTAPI RtlSetEnvironmentVariable_TYPE(
	IN OUT PVOID            *Environment OPTIONAL,
    IN PUNICODE_STRING      VariableName,
    IN PUNICODE_STRING      VariableValue
);

static RtlSetEnvironmentVariable_TYPE* RtlSetEnvironmentVariable = NULL;
RtlSetEnvironmentVariable_TYPE* GetRtlSetEnvironmentVariable() {
	if (RtlSetEnvironmentVariable) {
		return RtlSetEnvironmentVariable;
	}
	HMODULE zz = LoadLibrary(L"ntdll.dll");
	RtlSetEnvironmentVariable = (RtlSetEnvironmentVariable_TYPE*) GetProcAddress(zz, "RtlSetEnvironmentVariable");
	FreeLibrary(zz);
	return RtlSetEnvironmentVariable;
}

PVOID SetUserEnvironmentVariable(PVOID Environment, LPCWSTR varName, LPCWSTR varVal) {
	UNICODE_STRING a;
	UNICODE_STRING b;
	a.Buffer = const_cast<LPWSTR>(varName);
	a.Length = wcslen(varName) * sizeof(WCHAR);
	a.MaximumLength = a.Length;
	b.Buffer = const_cast<LPWSTR>(varVal);
	b.Length = wcslen(varVal) * sizeof(WCHAR);
	b.MaximumLength = b.Length;
	PVOID env = Environment;
	if (GetRtlSetEnvironmentVariable()) {
		RtlSetEnvironmentVariable(&env, &a, &b);
	} else {
		g_pLog->Log(LOGLEVEL_ERRORS, L"Could not get address of ntdll.RtlSetEnvironmentVariable, can't set %s=%s", varName, varVal);
	}
	return env;
}

//-------------------------------------------------------------------------------------
static DWORD FindUserSessionId(LPCWSTR szUser, LPCWSTR szDomain, DWORD* pdwSessionId)
{
	PWTS_SESSION_INFOW pSessInfo;
	DWORD dwCount, dwBytes, dwErr = 0;
	LPWSTR lpSessUser, lpSessDomain;
	BOOL bFound = FALSE;

	_ASSERT(pdwSessionId);

	if (WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessInfo, &dwCount))
	{
		for (DWORD i = 0; i < dwCount; i++)
		{
			if (pSessInfo[i].State == WTSActive)
			{
				if (WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, pSessInfo[i].SessionId,
					WTSUserName, &lpSessUser, &dwBytes) &&
					WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, pSessInfo[i].SessionId,
					WTSDomainName, &lpSessDomain, &dwBytes))
				{
					if (_wcsicmp(lpSessUser, szUser) == 0 &&
						_wcsicmp(lpSessDomain, szDomain) == 0)
						bFound = TRUE;

					WTSFreeMemory(lpSessUser);
					WTSFreeMemory(lpSessDomain);

					if (bFound)
					{
						*pdwSessionId = pSessInfo[i].SessionId;
						break;
					}
				}
				else
				{
					dwErr = GetLastError();
					break;
				}
			}
		}

		WTSFreeMemory(pSessInfo);
	}
	else
		dwErr = GetLastError();

	return (bFound
		? ERROR_SUCCESS
		: (dwErr
			? dwErr
			: 0xFFFFFFFF));
}

//-------------------------------------------------------------------------------------
static BOOL GetExplorerToken(DWORD dwSessionId, HANDLE* phToken)
{
	DWORD winlogonSessId, explorerPid = 0;
	PROCESSENTRY32W procEntry;
	BOOL bRet = FALSE;
	HANDLE hProcess, hPToken, hPTokenDup;

	_ASSERT(phToken);

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnap != INVALID_HANDLE_VALUE)
	{
		procEntry.dwSize = sizeof(PROCESSENTRY32W);

		if (Process32FirstW(hSnap, &procEntry))
		{
			do
			{
				if (_wcsicmp(procEntry.szExeFile, L"explorer.exe") == 0)
				{
					if (ProcessIdToSessionId(procEntry.th32ProcessID, &winlogonSessId)
						&& winlogonSessId == dwSessionId)
					{
						//found explorer.exe running into this session
						explorerPid = procEntry.th32ProcessID;
						break;
					}
				}
			} while (Process32Next(hSnap, &procEntry));

			if (explorerPid)
			{
				if ((hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, explorerPid)) != NULL)
				{
					if (OpenProcessToken(hProcess,
						TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_SESSIONID | TOKEN_READ | TOKEN_WRITE,
						&hPToken))
					{
						if (DuplicateTokenEx(hPToken, MAXIMUM_ALLOWED, NULL,
							SecurityIdentification, TokenPrimary, &hPTokenDup))
						{
							if (SetTokenInformation(hPTokenDup,
								TokenSessionId, (void*)dwSessionId, sizeof(DWORD)))
							{
								*phToken = hPTokenDup;
								bRet = TRUE;
							}
							else
								g_pLog->Log(LOGLEVEL_ERRORS, L"SetTokenInformation failed: 0x%0.8X", GetLastError());
						}
						else
							g_pLog->Log(LOGLEVEL_ERRORS, L"DuplicateTokenEx failed: 0x%0.8X", GetLastError());

						CloseHandle(hPToken);
					}

					CloseHandle(hProcess);
				}
				else
					g_pLog->Log(LOGLEVEL_ERRORS, L"OpenProcessToken failed: 0x%0.8X", GetLastError());
			}
			else
				g_pLog->Log(LOGLEVEL_ERRORS, L"Unable to find a suitable explorer.exe process");
		}
		else
			g_pLog->Log(LOGLEVEL_ERRORS, L"Process32FirstW failed: 0x%0.8X", GetLastError());

		CloseHandle(hSnap);
	}
	else
		g_pLog->Log(LOGLEVEL_ERRORS, L"CreateToolhelp32Snapshot failed: 0x%0.8X", GetLastError());
	
	return bRet;
}

//-------------------------------------------------------------------------------------
static BOOL WriteToPipe(HANDLE hPipe, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
						DWORD dwMilliseconds, LPOVERLAPPED pOv)
{
	DWORD cbWritten = 0, dwLe = 0;
	BOOL bWaitingWrite = FALSE;

	if (!WriteFile(hPipe, lpBuffer, nNumberOfBytesToWrite, NULL, pOv))
	{
		if ((dwLe = GetLastError()) != ERROR_IO_PENDING)
			return FALSE;

		bWaitingWrite = TRUE;
	}

	//se l'operazione è in stato di ERROR_IO_PENDING...
	if (bWaitingWrite)
	{
		//restiamo in attesa di un evento
		switch (WaitForSingleObject(pOv->hEvent, dwMilliseconds))
		{
		case WAIT_OBJECT_0:		// operazione completata
			break;
		case WAIT_TIMEOUT:		// timeout
			return FALSE;
			break;
		default:				// non dovremmo mai arrivare qui
			return FALSE;
			break;
		}
	}

	//controlliamo l'esito dell'operazione asincrona
	return (GetOverlappedResult(hPipe, pOv, &cbWritten, FALSE) &&
		cbWritten == nNumberOfBytesToWrite);
}

//-------------------------------------------------------------------------------------
void CPort::StartExe(LPCWSTR szExeName, LPCWSTR szWorkingDir, LPWSTR szCmdLine,
					 BOOL bTSEnabled, DWORD dwSessionId)
{
	typedef BOOL (WINAPI *PFNWTSQUERYUSERTOKEN)(ULONG, PHANDLE);

	LPWSTR szCommand;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	DWORD dwFlags = 0;
	HANDLE htok = NULL, huser = NULL;
	BOOL bIsXp, bRet;
	LPVOID lpEnv = NULL;
	PFNWTSQUERYUSERTOKEN fnWTSQueryUserToken = NULL;
	BOOL bTokOk = FALSE;

	bIsXp = Is_WinXPOrHigher();

	if (bIsXp)
	{
		g_pLog->Log(LOGLEVEL_ALL, L"Running on Windows XP or higher");

		HMODULE hMod = GetModuleHandleW(L"wtsapi32.dll");

		if (!hMod)
			hMod = LoadLibraryW(L"wtsapi32.dll");

		if (hMod)
			fnWTSQueryUserToken = (PFNWTSQUERYUSERTOKEN)GetProcAddress(hMod, "WTSQueryUserToken");

		if (!fnWTSQueryUserToken)
		{
			g_pLog->Log(LOGLEVEL_ERRORS, L"Unable to get WTSQueryUserToken function");
			return;
		}

		OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &huser);
		RevertToSelf();
	}

	if (bTSEnabled)
	{
		//Terminal Server present
		if (bIsXp)
		{
			//we have WTSQueryUserToken
			if ((bTokOk = fnWTSQueryUserToken(dwSessionId, &htok)) == FALSE)
				g_pLog->Log(LOGLEVEL_ERRORS, L"fnWTSQueryUserToken failed: 0x%0.8X", GetLastError());
		}
		else
		{
			//we DON'T have WTSQueryUserToken; look for a running
			//explorer.exe into this session, and grab token from it
			bTokOk = GetExplorerToken(dwSessionId, &htok);
		}
	}
	else
	{
		//Windows 2000 Pro or Windows 2000 Server w/o TS
		bTokOk = TRUE;
	}

	if (bTokOk)
	{
		HANDLE hHeap = GetProcessHeap();

		if ((szCommand = (LPWSTR)HeapAlloc(hHeap, 0, MAXCOMMAND * sizeof(WCHAR))) == NULL)
			return;

		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));

		si.cb = sizeof(si);
		si.lpDesktop = L"winsta0\\default";

		//componiamo il comando eseguibile...
		swprintf_s(szCommand, MAXCOMMAND, L"\"%s", szWorkingDir);
		size_t pos = wcslen(szCommand);
		if (pos == 0 || szCommand[pos - 1] != L'\\')
			wcscat_s(szCommand, MAXCOMMAND, L"\\");
		wcscat_s(szCommand, MAXCOMMAND, szExeName);
		wcscat_s(szCommand, MAXCOMMAND, L"\" ");
		wcscat_s(szCommand, MAXCOMMAND, szCmdLine);

		//creazione environment
		if (CreateEnvironmentBlock(&lpEnv, htok, FALSE))
			dwFlags |= CREATE_UNICODE_ENVIRONMENT;
		else
			g_pLog->Log(LOGLEVEL_WARNINGS, L"CreateEnvironmentBlock failed: 0x%0.8X", GetLastError());

		LPWSTR jobid = new WCHAR[10];
		_itow(m_nJobId, jobid, 10);
		lpEnv = SetUserEnvironmentVariable(lpEnv, L"WPHF_computername", ComputerName());
		lpEnv = SetUserEnvironmentVariable(lpEnv, L"WPHF_username", UserName());
		lpEnv = SetUserEnvironmentVariable(lpEnv, L"WPHF_filename", FileName());
		lpEnv = SetUserEnvironmentVariable(lpEnv, L"WPHF_printername", m_szPrinterName);
		lpEnv = SetUserEnvironmentVariable(lpEnv, L"WPHF_portname", PortName());
		lpEnv = SetUserEnvironmentVariable(lpEnv, L"WPHF_jobid", jobid);
		lpEnv = SetUserEnvironmentVariable(lpEnv, L"WPHF_jobtitle", JobTitle());

		delete[] jobid;

		g_pLog->Log(LOGLEVEL_ERRORS, L"Running %s(%s) in session %d for %s", (bIsXp ? L"CreateProcessAsUserW" : L"CreateProcessW"), szCommand, dwSessionId, UserName());

		//esecuzione
		if (htok)
			bRet = CreateProcessAsUserW(htok, NULL, szCommand, NULL,
			NULL, FALSE, dwFlags, lpEnv, szWorkingDir, &si, &pi);
		else
			bRet = CreateProcessW(NULL, szCommand, NULL,
			NULL, FALSE, dwFlags, lpEnv, szWorkingDir, &si, &pi);

		if (bRet)
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
			g_pLog->Log(LOGLEVEL_ERRORS, L"%s failed: 0x%0.8X", (htok ? L"CreateProcessAsUserW" : L"CreateProcessW"), GetLastError());

		//distruzione environment
		if (lpEnv)
			DestroyEnvironmentBlock(lpEnv);

		HeapFree(hHeap, 0, szCommand);

		if (htok)
			CloseHandle(htok);
	}

	if (huser)
	{
		SetThreadToken(NULL, huser);
		CloseHandle(huser);
	}
}

//-------------------------------------------------------------------------------------
CPort::CPort()
{
	Initialize();
}

//-------------------------------------------------------------------------------------
CPort::CPort(LPCWSTR szPortName)
{
	Initialize(szPortName);
}

//-------------------------------------------------------------------------------------
CPort::CPort(LPCWSTR szPortName, LPCWSTR szOutputPath, LPCWSTR szFilePattern, BOOL bOverwrite,
			 LPCWSTR szUserCommandPattern, LPCWSTR szExecPath, BOOL bWaitTermination, BOOL bPipeData)
{
	Initialize(szPortName, szOutputPath, szFilePattern, bOverwrite,
		szUserCommandPattern, szExecPath, bWaitTermination, bPipeData);
}

//-------------------------------------------------------------------------------------
void CPort::Initialize()
{
	*m_szPortName = L'\0';
	*m_szOutputPath = L'\0';
	m_szPrinterName = NULL;
	m_pPattern = NULL;
	m_bOverwrite = FALSE;
	m_pUserCommand = NULL;
	*m_szExecPath = L'\0';
	m_bWaitTermination = FALSE;
	m_bPipeData = FALSE;
	*m_szFileName = L'\0';
	m_hFile = INVALID_HANDLE_VALUE;
	m_hWriteThread = NULL;
	m_hWorkEvt = NULL;
	m_hDoneEvt = NULL;
	m_nJobId = 0;
	m_pJobInfo = NULL;
	m_bPipeActive = FALSE;
	InitializeCriticalSection(&m_threadData.csBuffer);
	SetFilePatternString(L"doc");
}

//-------------------------------------------------------------------------------------
void CPort::Initialize(LPCWSTR szPortName)
{
	Initialize();
	wcscpy_s(m_szPortName, LENGTHOF(m_szPortName), szPortName);
}

//-------------------------------------------------------------------------------------
void CPort::Initialize(LPCWSTR szPortName, LPCWSTR szOutputPath, LPCWSTR szFilePattern, BOOL bOverwrite,
					   LPCWSTR szUserCommandPattern, LPCWSTR szExecPath, BOOL bWaitTermination, BOOL bPipeData)
{
	Initialize(szPortName);
	wcscpy_s(m_szOutputPath, LENGTHOF(m_szOutputPath), szOutputPath);
	SetFilePatternString(szFilePattern);
	m_bOverwrite = bOverwrite;
	SetUserCommandString(szUserCommandPattern);
	wcscpy_s(m_szExecPath, LENGTHOF(m_szExecPath), szExecPath);
	m_bWaitTermination = bWaitTermination;
	m_bPipeData = bPipeData;

	g_pLog->Log(LOGLEVEL_ALL, L"Initializing port %s", szPortName);
	g_pLog->Log(LOGLEVEL_ALL, L" Output path:      %s", szOutputPath);
	g_pLog->Log(LOGLEVEL_ALL, L" File pattern:     %s", szFilePattern);
	g_pLog->Log(LOGLEVEL_ALL, L" Overwrite:        %s", (bOverwrite ? szTrue : szFalse));
	g_pLog->Log(LOGLEVEL_ALL, L" User command:     %s", szUserCommandPattern);
	g_pLog->Log(LOGLEVEL_ALL, L" Execute from:     %s", szExecPath);
	g_pLog->Log(LOGLEVEL_ALL, L" Wait termination: %s", (bWaitTermination ? szTrue : szFalse));
	g_pLog->Log(LOGLEVEL_ALL, L" Use pipe:         %s", (bPipeData ? szTrue : szFalse));
}

//-------------------------------------------------------------------------------------
CPort::~CPort()
{
	if (m_pPattern)
		delete m_pPattern;

	if (m_pUserCommand)
		delete m_pUserCommand;

	HANDLE hHeap = GetProcessHeap();

	if (m_szPrinterName)
		HeapFree(hHeap, 0, m_szPrinterName);

	if (m_pJobInfo)
		HeapFree(hHeap, 0, m_pJobInfo);

	if (m_hWriteThread)
	{
		EnterCriticalSection(&m_threadData.csBuffer);
		m_threadData.lpBuffer = NULL;
		LeaveCriticalSection(&m_threadData.csBuffer);
		SetEvent(m_hWorkEvt);
		WaitForSingleObject(m_hDoneEvt, INFINITE);
		CloseHandle(m_hWriteThread);
		m_hWriteThread = NULL;
	}

	if (m_hWorkEvt)
		CloseHandle(m_hWorkEvt);

	if (m_hDoneEvt)
		CloseHandle(m_hDoneEvt);

	DeleteCriticalSection(&m_threadData.csBuffer);
}

//-------------------------------------------------------------------------------------
void CPort::SetFilePatternString(LPCWSTR szPattern)
{
	if (m_pPattern)
		delete m_pPattern;

	m_pPattern = new CPattern(szPattern, this, FALSE);
}

//-------------------------------------------------------------------------------------
void CPort::SetUserCommandString(LPCWSTR szPattern)
{
	if (m_pUserCommand)
		delete m_pUserCommand;

	m_pUserCommand = new CPattern(szPattern, this, TRUE);
}

//-------------------------------------------------------------------------------------
LPCWSTR CPort::FilePattern() const
{
	if (m_pPattern)
		return m_pPattern->PatternString();
	else
		return CPattern::szDefaultFilePattern;
}

//-------------------------------------------------------------------------------------
LPCWSTR CPort::UserCommandPattern() const
{
	if (m_pUserCommand)
		return m_pUserCommand->PatternString();
	else
		return CPattern::szDefaultUserCommand;
}

//-------------------------------------------------------------------------------------
BOOL CPort::StartJob(DWORD nJobId, LPWSTR szJobTitle, LPWSTR szPrinterName)
{
	UNREFERENCED_PARAMETER(szJobTitle);

	_ASSERTE(m_pPattern != NULL);

	if (!m_pPattern)
		return FALSE;

	m_nJobId = nJobId;

	m_pPattern->Reset();

	HANDLE hHeap = GetProcessHeap();

	//retrieve job info
	DWORD cbNeeded;

	CPrinterHandle printer(szPrinterName);

	if (!printer.Handle())
	{
		g_pLog->Log(LOGLEVEL_ERRORS, this, L"CPort::StartJob: OpenPrinterW failed (%i)", GetLastError());
		return FALSE;
	}

	GetJobW(printer, nJobId, 1, NULL, 0, &cbNeeded);

	if (!m_pJobInfo)
		m_pJobInfo = (JOB_INFO_1W*)HeapAlloc(hHeap, 0, cbNeeded);
	else if (HeapSize(hHeap, 0, m_pJobInfo) < cbNeeded)
		m_pJobInfo = (JOB_INFO_1W*)HeapReAlloc(hHeap, 0, m_pJobInfo, cbNeeded);

	if (!m_pJobInfo)
		return FALSE;

	if (!GetJobW(printer, nJobId, 1, (LPBYTE)m_pJobInfo, (DWORD)HeapSize(hHeap, 0, m_pJobInfo), &cbNeeded))
	{
		g_pLog->Log(LOGLEVEL_ERRORS, this, L"CPort::StartJob: GetJobW failed (%i)", GetLastError());
		return FALSE;
	}

	//determine if a job was submitted locally by comparing local netbios name
	//with that stored into m_pJobInfo
	WCHAR szComputerName[256];
	DWORD nSize = LENGTHOF(szComputerName);

	GetComputerNameW(szComputerName, &nSize);

	m_bJobIsLocal = (
		m_pJobInfo &&
		(
			//the machine names are exactly the same...
			_wcsicmp(szComputerName, m_pJobInfo->pMachineName) == 0 ||
			(
				//...or we have two extra \\ in the machine name provided by the spooler
				*m_pJobInfo->pMachineName &&
				m_pJobInfo->pMachineName[0] == L'\\' &&
				m_pJobInfo->pMachineName[1] == L'\\' &&
				_wcsicmp(szComputerName, m_pJobInfo->pMachineName + 2) == 0
			)
		)
	);

	//copy printer name locally
	size_t len = wcslen(szPrinterName) + 1;

	if (!m_szPrinterName)
		m_szPrinterName = (LPWSTR)HeapAlloc(hHeap, 0, len * sizeof(WCHAR));
	else if (HeapSize(hHeap, 0, m_szPrinterName) < len * sizeof(WCHAR))
		m_szPrinterName = (LPWSTR)HeapReAlloc(hHeap, 0, m_szPrinterName, len * sizeof(WCHAR));

	if (!m_szPrinterName)
		return FALSE;

	wcscpy_s(m_szPrinterName, HeapSize(hHeap, 0, m_szPrinterName) / sizeof(WCHAR), szPrinterName);

	//event to signal work to be done
	if (!m_hWorkEvt)
		if ((m_hWorkEvt = CreateEventW(NULL, FALSE, FALSE, NULL)) == NULL)
			return FALSE;

	//event to signal work has been done
	if (!m_hDoneEvt)
		if ((m_hDoneEvt = CreateEventW(NULL, FALSE, FALSE, NULL)) == NULL)
			return FALSE;

	//the writing thread - since we can't create an "overlapped pipe"
	//we use threads to mimick overlapped I/O, to avoid "waiting forever"
	//on a write to a broken pipe
	if (!m_hWriteThread)
	{
		DWORD dwId = 0;
		m_threadData.pPort = this;
		if ((m_hWriteThread = CreateThread(NULL, 0, WriteThreadProc, (LPVOID)&m_threadData, 0, &dwId)) == NULL)
			return FALSE;
		g_pLog->Log(LOGLEVEL_ALL, L"Worker thread started (id: 0x%0.8X)", dwId);
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------
DWORD CPort::CreateOutputFile()
{
	_ASSERTE(m_pPattern != NULL);

	if (!m_pPattern)
		return ERROR_CAN_NOT_COMPLETE;

	HKEY hkRoot;
	DWORD cbData;

	DWORD rc;

	//apre chiave di registro
	rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Winprint HylaFAX Reloaded", 0,
		STANDARD_RIGHTS_READ | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hkRoot);

	if (rc == ERROR_SUCCESS)
	{
		//legge InstallDir
		cbData = sizeof(m_szExecPath);
		if ((rc = RegQueryValueExW(hkRoot, L"InstallDir", NULL, NULL, (LPBYTE)m_szExecPath, &cbData)) == ERROR_SUCCESS)
			m_szExecPath[cbData / sizeof(WCHAR)] = L'\0';

		//chiude registro
		RegCloseKey(hkRoot);
	}
	else
	{
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	if (SHGetSpecialFolderPathW(NULL, m_szOutputPath, CSIDL_COMMON_APPDATA, FALSE))
	{
		wcscat_s(m_szOutputPath, LENGTHOF(m_szOutputPath), L"\\Winprint HylaFAX Reloaded\\faxtmp");
	}
	else
	{
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	m_bOverwrite = FALSE;
	SetFilePatternString(L"fax%i.ps");

	/*start composing the output filename*/
	wcscpy_s(m_szFileName, LENGTHOF(m_szFileName), m_szOutputPath);

	/*append a backslash*/
	size_t pos = wcslen(m_szFileName);
	if (pos == 0 || m_szFileName[pos - 1] != L'\\')
	{
		wcscat_s(m_szFileName, LENGTHOF(m_szFileName), L"\\");
		pos++;
	}

	/*the search algorithm uses search strings from "search fields", if any*/
	WCHAR szSearchPath[MAX_PATH];
	wcscpy_s(szSearchPath, LENGTHOF(szSearchPath), m_szFileName);

	/*start finding a file name*/
	do
	{
		m_szFileName[pos] = L'\0';
		szSearchPath[pos] = L'\0';

		/*get current value from pattern*/
		LPWSTR szFileName = m_pPattern->Value();
		LPWSTR szSearchName = m_pPattern->SearchValue();

		/*append it to output file name*/
		wcscat_s(m_szFileName, LENGTHOF(m_szFileName), szFileName);
		wcscat_s(szSearchPath, LENGTHOF(szSearchPath), szSearchName);

		/*check if parent directory exists*/
		GetFileParent(m_szFileName, m_szParent, LENGTHOF(m_szParent));
		if (!RecursiveCreateFolder(m_szParent))
		{
			g_pLog->Log(LOGLEVEL_ERRORS, this, L"CPort::CreateOutputFile: can't create output directory (%i)", GetLastError());
			return ERROR_DIRECTORY;
		}

		//is this file name usable?
		//2009-08-04 we use search strings
//		if (!m_bOverwrite && FileExists(m_szFileName))
		if (!m_bOverwrite && FilePatternExists(szSearchPath))
			continue;

		//ok we got a valid filename, create it
		if (m_bPipeData)
		{
			if (!m_pUserCommand || !*m_pUserCommand->PatternString())
			{
				g_pLog->Log(LOGLEVEL_ERRORS, this, L"CPort::CreateOutputFile: empty user command, can't continue");
				return ERROR_CAN_NOT_COMPLETE;
			}

			HANDLE hStdinR, hStdoutW, hStdoutR;
			SECURITY_ATTRIBUTES saAttr;

			saAttr.nLength = sizeof(saAttr);
			saAttr.bInheritHandle = TRUE;
			saAttr.lpSecurityDescriptor = NULL;

			//we have an external program to send data to
			//2009-06-12 batch files are executed through cmd.exe
			//with /C switch. cmd.exe won't start if we do not supply
			//a stdout handle
			if (!CreatePipe(&hStdinR, &m_hFile, &saAttr, 0) ||
				!CreatePipe(&hStdoutR, &hStdoutW, &saAttr, 0) ||
				!SetHandleInformation(m_hFile, HANDLE_FLAG_INHERIT, 0) ||
				!SetHandleInformation(hStdoutR, HANDLE_FLAG_INHERIT, 0))
			{
				g_pLog->Log(LOGLEVEL_ERRORS, this,
					L"CPort::CreateOutputFile: can't create pipes (%i)", GetLastError());
				return ERROR_FILE_INVALID;
			}

			STARTUPINFOW si;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			si.hStdInput = hStdinR;
			si.hStdOutput = hStdoutW;
			si.hStdError = hStdoutW;
#ifdef _DEBUG
			si.wShowWindow = SW_SHOW;
#else
			si.wShowWindow = SW_HIDE;
#endif
			si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

//			if (!BuildCommandLine())
//				return ERROR_CAN_NOT_COMPLETE;

			//create child process - give up in case of failure since we need to write to process
			BOOL bRes = CreateProcessW(NULL, m_pUserCommand->Value(), NULL, NULL,
				TRUE, 0, NULL, (*m_szExecPath) ? m_szExecPath : NULL, &si, &m_procInfo);

			DWORD dwErr = GetLastError();

			//close stdout and stdin pipe after child process has inherited them
			CloseHandle(hStdoutW);
			CloseHandle(hStdinR);

			if (!bRes)
			{
				g_pLog->Log(LOGLEVEL_ERRORS, this, L"CPort::CreateOutputFile: CreateProcessW failed (%i)", dwErr);
				g_pLog->Log(LOGLEVEL_ERRORS, L" User command = %s", m_pUserCommand->Value());
				g_pLog->Log(LOGLEVEL_ERRORS, L" Execute from = %s", m_szExecPath);

				WCHAR szBuf[128];
				DWORD dwCb = LENGTHOF(szBuf);
				GetUserNameW(szBuf, &dwCb);
				g_pLog->Log(LOGLEVEL_ERRORS, L" Running as   = %s", szBuf);

				CloseHandle(hStdoutR);

				return ERROR_CAN_NOT_COMPLETE;
			}

			m_bPipeActive = TRUE;

			//start reading thread - the thread will read and discard anything that comes from
			//the external program, and finally close handle to our end of stdout
			HANDLE hReadThread = NULL;
			DWORD dwId = 0;
			if ((hReadThread = CreateThread(NULL, 0, ReadThreadProc, (LPVOID)hStdoutR, 0, &dwId)) == NULL)
				return ERROR_CAN_NOT_COMPLETE;

			//we immediately close handle to the thread and leave it
			//around making its job until it's done
			CloseHandle(hReadThread);

			return 0;
		}
		else
		{
			//output on a regular file
			m_hFile = CreateFileW(m_szFileName, GENERIC_WRITE, 0,
				NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (m_hFile == INVALID_HANDLE_VALUE)
			{
				g_pLog->Log(LOGLEVEL_ERRORS, this, L"CPort::CreateOutputFile: CreateFileW failed (%i)", GetLastError());
				return ERROR_FILE_INVALID;
			}
			else
				return 0;
		}
	} while (m_pPattern->NextValue()); //loop until there are no more combinations for pattern

	g_pLog->Log(LOGLEVEL_ERRORS, this, L"CPort::CreateOutputFile: can't get a valid filename");

	return ERROR_FILE_EXISTS;
}

//-------------------------------------------------------------------------------------
BOOL CPort::WriteToFile(LPCVOID lpBuffer, DWORD cbBuffer, LPDWORD pcbWritten)
{
	//if we're writing to a pipe, make sure proces is alive
	if (m_bPipeActive)
	{
		DWORD dwCode = 0;

		if (!GetExitCodeProcess(m_procInfo.hProcess, &dwCode) ||
			dwCode != STILL_ACTIVE)
		{
			SetLastError(ERROR_CAN_NOT_COMPLETE);
			return FALSE;
		}
	}

	//pass buffer to the write thread
	m_threadData.lpBuffer = lpBuffer;
	m_threadData.cbBuffer = cbBuffer;
	m_threadData.pcbWritten = pcbWritten;

	//wake up thread
	SetEvent(m_hWorkEvt);

	BOOL bDone = FALSE;

	while (!bDone)
	{
		switch (WaitForSingleObject(m_hDoneEvt, 10000))
		{
		case WAIT_OBJECT_0:
			return TRUE;
			break;
		case WAIT_TIMEOUT:
			if (!m_bJobIsLocal || MessageBoxW(GetDesktopWindow(), szMsgUserCommandLocksSpooler, szAppTitle, MB_YESNO) == IDNO)
			{
				TerminateThread(m_hWriteThread, 1);
				CloseHandle(m_hWriteThread);
				m_hWriteThread = NULL;
				return FALSE;
			}
			break;
		default:
			return FALSE;
		}
	}

	return FALSE;
}

//-------------------------------------------------------------------------------------
DWORD WINAPI CPort::WriteThreadProc(LPVOID lpParam)
{
	LPTHREADDATA pData = (LPTHREADDATA)lpParam;

	_ASSERTE(pData != NULL);
	_ASSERTE(pData->pPort != NULL);

	for (;;)
	{
		//wait signal from main thread
		if (WaitForSingleObject(pData->pPort->m_hWorkEvt, INFINITE) == WAIT_OBJECT_0)
		{
			EnterCriticalSection(&pData->csBuffer);

			if (pData->lpBuffer == NULL)
			{
				LeaveCriticalSection(&pData->csBuffer);
				SetEvent(pData->pPort->m_hDoneEvt);
				return 0;
			}

			pData->bStatus = WriteFile(pData->pPort->m_hFile, pData->lpBuffer, pData->cbBuffer,
				pData->pcbWritten, NULL);

			LeaveCriticalSection(&pData->csBuffer);
		}

		//signal we're done
		SetEvent(pData->pPort->m_hDoneEvt);
	}
}

//-------------------------------------------------------------------------------------
DWORD WINAPI CPort::ReadThreadProc(LPVOID lpParam)
{
	HANDLE hStdoutR = (HANDLE)lpParam;

	_ASSERTE(hStdoutR != NULL);

	char buf[512];
	DWORD dwRead;

	//we read and simply trash anything that comes from the user command
	for (;;)
	{
		BOOL bRes;
		bRes = ReadFile(hStdoutR, buf, sizeof(buf), &dwRead, NULL);
		if (!bRes || dwRead == 0)
			break;
	}

	//done, close handle to our end of the pipe
	CloseHandle(hStdoutR);

	return 0;
}

//-------------------------------------------------------------------------------------
BOOL CPort::EndJob()
{
	_ASSERTE(m_pPattern != NULL);

	if (!m_pPattern)
		return FALSE;

	//done with the file, close it and flush buffers
	FlushFileBuffers(m_hFile);
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
	m_bPipeActive = FALSE;

	//tell the spooler we are done with the job
	CPrinterHandle printer(m_szPrinterName);

	if (printer.Handle())
		SetJobW(printer, JobId(), 0, NULL, JOB_CONTROL_DELETE);
/*
	//start user command
	if (!m_bPipeData && m_pUserCommand && *m_pUserCommand->PatternString())
	{
		STARTUPINFOW si;

		ZeroMemory(&m_procInfo, sizeof(m_procInfo));
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);

		//we're not going to give up in case of failure
		CreateProcessW(NULL, m_pUserCommand->Value(), NULL, NULL,
			FALSE, 0, NULL, (*m_szExecPath) ? m_szExecPath : NULL, &si, &m_procInfo);
	}

	//maybe wait and close handles to child process
	if (m_procInfo.hProcess)
	{
		if (m_bWaitTermination)
		{
			BOOL bDone = FALSE;

			while (!bDone)
			{
				switch (WaitForSingleObject(m_procInfo.hProcess, 10000))
				{
				case WAIT_OBJECT_0:
					bDone = TRUE;
					break;
				case WAIT_TIMEOUT:
					if (!m_bJobIsLocal ||
						MessageBoxW(GetDesktopWindow(), szMsgUserCommandLocksSpooler, szAppTitle, MB_YESNO) == IDNO)
						bDone = TRUE;
					break;
				}
			}
		}
		CloseHandle(m_procInfo.hProcess);
		CloseHandle(m_procInfo.hThread);
	}
*/
	//eseguiamo le operazioni post-spooling
	//modalità multi-documento
	static LPCWSTR szPipeTemplate = L"\\\\.\\pipe\\wphf_sessid%0.8X";
	WCHAR szPipeName[32];
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	BOOL bTSEnabled = FALSE;
	DWORD dwSessionId = 0, dwRet;

	dwRet = FindUserSessionId(UserName(), ComputerName(), &dwSessionId);

	if (dwRet == ERROR_SUCCESS)
		bTSEnabled = TRUE;
	else if (dwRet != ERROR_APP_WRONG_OS)
	{
		if (Is_WinXPOrHigher())
		{
			typedef DWORD (WINAPI *PFNWTSGETACTIVECONSOLESESSIONID)(void);
			PFNWTSGETACTIVECONSOLESESSIONID fnWTSGetActiveConsoleSessionId;

			HMODULE hMod = GetModuleHandleW(L"kernel32.dll");

			if (!hMod)
				return FALSE;

			fnWTSGetActiveConsoleSessionId = (PFNWTSGETACTIVECONSOLESESSIONID)GetProcAddress(hMod, "WTSGetActiveConsoleSessionId");

			if (!fnWTSGetActiveConsoleSessionId)
				return FALSE;

			if ((dwSessionId = fnWTSGetActiveConsoleSessionId()) == 0xFFFFFFFF)
				return FALSE;

			bTSEnabled = TRUE;
		}
		else
			return FALSE;
	}

	swprintf_s(szPipeName, LENGTHOF(szPipeName), szPipeTemplate, dwSessionId);

	//cerchiamo la pipe...
	hPipe = CreateFileW(
		szPipeName,
		GENERIC_WRITE | FILE_FLAG_OVERLAPPED,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (hPipe != INVALID_HANDLE_VALUE)
	{
		//pipe trovata
		DWORD len;
		OVERLAPPED ov;
		LPWSTR szBuf = NULL;

		ZeroMemory(&ov, sizeof(ov));
		ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (ov.hEvent == NULL)
			return FALSE;

		//file name
//		WideCharToMultiByte(CP_THREAD_ACP, 0, m_szFileName, -1,
//			szBuf, sizeof(szBuf), NULL, NULL);
		len = (DWORD)wcslen(m_szFileName) + (DWORD)wcslen(JobTitle()) + 2;
		HANDLE hHeap = GetProcessHeap();
		if ((szBuf = (LPWSTR)HeapAlloc(hHeap, 0, len * sizeof(WCHAR))) != NULL)
		{
			swprintf_s(szBuf, len, L"%s|%s", m_szFileName, JobTitle());
			DWORD nBytes = len * sizeof(WCHAR);
			WriteToPipe(hPipe, &nBytes, sizeof(nBytes), 1000, &ov);
			WriteToPipe(hPipe, szBuf, nBytes, 1000, &ov);

			HeapFree(hHeap, 0, szBuf);
		}

		CloseHandle(hPipe);
		CloseHandle(ov.hEvent);
	}
	else
	{
		//pipe non trovata, lancio l'exe
		
		DWORD len;
		LPWSTR szCmdLine = NULL;

		len = (DWORD)wcslen(m_szFileName) + (DWORD)wcslen(JobTitle()) + 6;
		HANDLE hHeap = GetProcessHeap();
		if ((szCmdLine = (LPWSTR)HeapAlloc(hHeap, 0, len * sizeof(WCHAR))) != NULL)
		{
			//componiamo la linea di comando
			swprintf_s(szCmdLine, len, L"\"%s\" \"%s\"", m_szFileName, JobTitle());
			//esecuzione
			StartExe(L"wphfgui.exe", ExecPath(), szCmdLine, bTSEnabled, dwSessionId);
			HeapFree(hHeap, 0, szCmdLine);
		}
	}

	*m_szFileName = L'\0';

	return TRUE;
}

//-------------------------------------------------------------------------------------
void CPort::SetConfig(LPCWSTR szPortName/*, LPCWSTR szOutputPath, LPCWSTR szFilePattern, BOOL bOverwrite,
		LPCWSTR szUserCommandPattern, LPCWSTR szExecPath, BOOL bWaitTermination, BOOL bPipeData, int nLogLevel*/)
{
	wcscpy_s(m_szPortName, LENGTHOF(m_szPortName), szPortName);
//	wcscpy_s(m_szOutputPath, LENGTHOF(m_szOutputPath), szOutputPath);
//	SetFilePatternString(szFilePattern);
//	m_bOverwrite = bOverwrite;
//	SetUserCommandString(szUserCommandPattern);
//	wcscpy_s(m_szExecPath, LENGTHOF(m_szExecPath), szExecPath);
//	m_bWaitTermination = bWaitTermination;
//	m_bPipeData = bPipeData;
//	g_pLog->SetLogLevel(nLogLevel);
}

//-------------------------------------------------------------------------------------
LPCWSTR CPort::UserName() const
{
	return m_pJobInfo
		? m_pJobInfo->pUserName
		: (LPWSTR)L"";
}

//-------------------------------------------------------------------------------------
LPCWSTR CPort::ComputerName() const
{
	if (m_pJobInfo)
	{
		//strip backslashes off
		LPWSTR pBuf = m_pJobInfo->pMachineName;

		while (*pBuf == L'\\')
			pBuf++;

		return pBuf;
	}
	else
		return L"";
}

//-------------------------------------------------------------------------------------
LPWSTR CPort::JobTitle() const
{
	return m_pJobInfo
		? m_pJobInfo->pDocument
		: (LPWSTR)L"";
}
