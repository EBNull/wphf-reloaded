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

#ifndef _MONUTILS_H
#define _MONUTILS_H

//BOOL Is_CorrectProcessorArchitecture();

BOOL Is_Win2000();

BOOL Is_WinXP();

BOOL Is_WinXPOrHigher();

BOOL Is_Win2003();

BOOL Is_WinVista();

BOOL Is_Win2008();

BOOL Is_Win7();

BOOL FileExists(LPCWSTR szFileName);

BOOL FilePatternExists(LPCWSTR szFileName);

BOOL DirectoryExists(LPCWSTR szDirName);

void Trim(LPWSTR szString);

void GetFileParent(LPCWSTR szFile, LPWSTR szParent, size_t count);

BOOL RecursiveCreateFolder(LPCWSTR szPath);

BOOL IsUACEnabled();

#endif