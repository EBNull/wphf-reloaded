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

#ifndef _PORT_H
#define _PORT_H

#include "pattern.h"
#include "..\common\defs.h"

class CPort
{
private:
	void Initialize();
	void Initialize(LPCWSTR szPortName);
	void Initialize(LPCWSTR szPortName, LPCWSTR szOutputPath, LPCWSTR szFilePattern, BOOL bOverwrite,
		LPCWSTR szUserCommandPattern, LPCWSTR szExecPath, BOOL bWaitTermination, BOOL bPipeData);
	
	void StartExe(LPCWSTR szExeName, LPCWSTR szWorkingDir, LPWSTR szCmdLine, BOOL bTSEnabled, DWORD dwSessionId);
public:
	CPort();
	CPort(LPCWSTR szPortName);
	CPort(LPCWSTR szPortName, LPCWSTR szOutputPath, LPCWSTR szFilePattern, BOOL bOverwrite,
		LPCWSTR szUserCommandPattern, LPCWSTR szExecPath, BOOL bWaitTermination, BOOL bPipeData);
	virtual ~CPort();
	CPattern* GetPattern() const { return m_pPattern; }
	void SetFilePatternString(LPCWSTR szPattern);
	void SetUserCommandString(LPCWSTR szPattern);
	BOOL BuildCommandLine();
	BOOL StartJob(DWORD nJobId, LPWSTR szJobTitle, LPWSTR szPrinterName);
	DWORD CreateOutputFile();
	BOOL WriteToFile(LPCVOID lpBuffer, DWORD cbBuffer,
		LPDWORD pcbWritten);
	BOOL EndJob();
	void SetConfig(LPCWSTR szPortName/*, LPCWSTR szOutputPath, LPCWSTR szFilePattern, BOOL bOverwrite,
		LPCWSTR szUserCommandPattern, LPCWSTR szExecPath, BOOL bWaitTermination, BOOL bPipeData, int nLogLevel*/);

public:
	LPCWSTR PortName() const { return m_szPortName; }
	LPCWSTR OutputPath() const { return m_szOutputPath; }
	LPCWSTR ExecPath() const { return m_szExecPath; }
	LPCWSTR GUIPath() const { return m_szGUIPath; }
	LPCWSTR FilePattern() const;
	LPCWSTR UserCommandPattern() const;
	BOOL Overwrite() const { return m_bOverwrite; }
	BOOL WaitTermination() const { return m_bWaitTermination; }
	BOOL PipeData() const { return m_bPipeData; }
	LPWSTR PrinterName() const { return m_szPrinterName; }
	DWORD JobId() const { return m_nJobId; }
	LPWSTR JobTitle() const;
	LPCWSTR UserName() const;
	LPCWSTR ComputerName() const;
	LPWSTR FileName() const { return (LPWSTR)m_szFileName; }
	LPWSTR Path() const { return (LPWSTR)m_szParent; }

private:
	typedef struct tagTHREADDATA
	{
		CPort* pPort;
		LPCVOID lpBuffer;
		DWORD cbBuffer;
		LPDWORD pcbWritten;
		BOOL bStatus;
		CRITICAL_SECTION csBuffer;
	} THREADDATA, *LPTHREADDATA;
	static DWORD WINAPI WriteThreadProc(LPVOID lpParam);
	static DWORD WINAPI ReadThreadProc(LPVOID lpParam);
	
private:
	THREADDATA m_threadData;
	HANDLE m_hWriteThread;
	HANDLE m_hWorkEvt;
	HANDLE m_hDoneEvt;
	WCHAR m_szPortName[MAX_PATH];
	WCHAR m_szOutputPath[MAX_PATH];
	WCHAR m_szExecPath[MAX_PATH];
	WCHAR m_szGUIPath[MAX_PATH];
	LPWSTR m_szPrinterName;
	CPattern* m_pPattern;
	CPattern* m_pUserCommand;
	BOOL m_bOverwrite;
//	WCHAR m_szUserCommand[MAXUSERCOMMMAND];
	BOOL m_bWaitTermination;
	BOOL m_bPipeData;
	BOOL m_bPipeActive;
	WCHAR m_szFileName[MAX_PATH];
	HANDLE m_hFile;
	PROCESS_INFORMATION m_procInfo;
//	LPWSTR m_szCommandLine;
	DWORD m_nJobId;
	JOB_INFO_1W* m_pJobInfo;
	BOOL m_bJobIsLocal;
	WCHAR m_szParent[MAX_PATH];
};

#endif
