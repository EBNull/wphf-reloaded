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

#ifndef _DEFS_H
#define _DEFS_H

//maximum command line for CreateProcessW
#define MAXCOMMAND 32768

//maximum user defined command
#define MAXUSERCOMMMAND 1024

extern LPCWSTR szMonitorName;
extern LPCWSTR szDescription;
extern LPCWSTR szAppTitle;
extern LPCWSTR szMsgUserCommandLocksSpooler;
extern LPCWSTR szTrue;
extern LPCWSTR szFalse;

#ifdef MFILEMONUI
extern LPCWSTR szMsgInvalidPortName;
extern LPCWSTR szMsgBrowseFolderTitle;
extern LPCWSTR szMsgProvideFileName;
extern LPCWSTR szMsgInvalidFileName;
extern LPCWSTR szMsgNoAddOnRemoteSvr;
extern LPCWSTR szMsgPortExists;
extern LPCWSTR szMsgNoConfigOnRemoteSvr;
extern LPCWSTR szMsgNoDropOnRemoteSvr;
#endif

#endif