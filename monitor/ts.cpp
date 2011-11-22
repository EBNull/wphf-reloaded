#include "stdafx.h"
#include "ts.h"

#include <Wtsapi32.h>
//TODO: Do not link staticly. Use GetProcAddress.
#pragma comment(lib, "wtsapi32.lib")
DWORD GetActiveConsoleSID()
{
	//"If there is no session attached to the physical console, (for example,
	// if the physical console session is in the process of being attached or detached),
	// this function returns 0xFFFFFFFF."
	return WTSGetActiveConsoleSessionId(); 
}
DWORD GetTargetSIDFromUsername(LPWSTR machineName, LPWSTR userName)
{
	//TODO: Reject if machineName != local
	UNREFERENCED_PARAMETER(machineName);

	//Default to returning invalid session.
	DWORD dwSessionId = INVALID_SESSION;
	
	//TODO: Check AD domain name (impossible?)


	PWTS_SESSION_INFO pSessionInfo = 0;
	DWORD dwCount = 0;
   
	// Get the list of all terminal sessions 
	WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwCount);

	// look over obtained list in search of an active session with target userName
	for (DWORD i = 0; i < dwCount; ++i)
	{
		WTS_SESSION_INFO si = pSessionInfo[i];
		if (WTSActive == si.State)
		{ 
			// If the current session is active, we need to check username
			LPWSTR domainData = NULL;
			LPWSTR userData = NULL;
			DWORD domainBytes = 0;
			DWORD userBytes = 0;
			WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, si.SessionId, WTSUserName, &userData, &userBytes);
			WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, si.SessionId, WTSDomainName, &domainData, &domainBytes);
			//Should this be case insensitive?
			if (wcscmp(userData, userName) == 0) {
				dwSessionId = si.SessionId;
				//Found username match
				WTSFreeMemory(domainData);
				WTSFreeMemory(userData);
				break;
			}
			WTSFreeMemory(domainData);
			WTSFreeMemory(userData);
		}
	}
	WTSFreeMemory(pSessionInfo);
	return dwSessionId;
}