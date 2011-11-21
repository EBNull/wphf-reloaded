#ifndef _IPC_H
#define _IPC_H

#ifdef __cplusplus
extern "C" {
#endif

	typedef int (*IPCCALLBACK)(const char*);

	//valori di ritorno:
	//0 		operazione riuscita
	//!= 0	valore restituito da GetLastError
	int StartIpc(IPCCALLBACK pfnCallback);

	void StopIpc();

#ifdef __cplusplus
}
#endif

#endif
