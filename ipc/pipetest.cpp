// pipetest.cpp : definisce il punto di ingresso dell'applicazione console.
//

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include "ipc.h"

#define BUFSIZE 4096

int IpcCB(const char* pBuf)
{
	printf("stringa arrivata: %s\n", pBuf);

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR szUserName[64];
	DWORD cbUserName = sizeof(szUserName) / sizeof(TCHAR);
	LPWCH lpEnv = NULL, lpVar = NULL;
	size_t len;

	GetUserName(szUserName, &cbUserName);

	_tprintf(_T("Sto girando come utente: %s\n"), szUserName);

	if ((lpEnv = GetEnvironmentStrings()) != NULL)
	{
		_tprintf(_T("Variabili d'ambiente:\n"));

		lpVar = lpEnv;

		do
		{
			len = _tcslen(lpVar);

			if (len > 0)
			{
				_tprintf(_T("%s\n"), lpVar);
				lpVar += len + 1;
			}
		} while (len > 0);

		FreeEnvironmentStrings(lpEnv);
	}

	if (argc == 3)
	{
		_tprintf(_T("parametro arrivato: %s\n"), argv[2]);
	}

	StartIpc(IpcCB);

	_getch();

	StopIpc();

	return 0;
}

