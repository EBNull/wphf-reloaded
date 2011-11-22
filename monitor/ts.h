/*
WPHFMON - Winprint HylaFAX port monitor
Copyright (C) 2011 Monti Lorenzo
ts.h, ts.cpp Copyright (C) 2011 Chris Burke

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

#ifndef _TS_H
#define _TS_H


const DWORD INVALID_SESSION = 0xFFFFFFFF; //Returned when no session is found, and no session is connected to the console

DWORD GetActiveConsoleSID();
DWORD GetTargetSIDFromUsername(LPCWSTR machineName, LPCWSTR userName);

/*
Notes on GetTargetSIDFromUsername:

* machineName is not used. Theoretically this function could be extended to get the TS SID of a remote server,
      but it is currently ignored.

* userName is case sensitive.

* Only active (connected) sessions are considered.

* There may be more than one active session by a single username. This function will return the first one enumerated,
      which may not be the one that initiated the print job. The session ID seems not to be part of the print job.

*/

#endif