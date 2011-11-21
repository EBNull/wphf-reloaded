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

#include <vcl.h>
#include <gnugettext.hpp>
#pragma hdrstop
#include <tchar.h>
//---------------------------------------------------------------------------
USEFORM("Select.cpp", SelectRcpt);
USEFORM("SendFAX.cpp", FAXSend);
USEFORM("Recipient.cpp", RecipientName);
USEFORM("Config.cpp", ConfigForm);
//---------------------------------------------------------------------------
#pragma link "gnugettext.lib"
#pragma link "odbc32.lib"
//---------------------------------------------------------------------------
WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	#ifdef MAPIDEBUG
	ShowMessage(L"WARNING: this is a special build only intended for debugging of Extended MAPI code.");
	#endif

	try
	{
		TP_GlobalIgnoreClassProperty(__classid(TAction), L"Category");
		TP_GlobalIgnoreClassProperty(__classid(TFont), L"Name");
		TP_GlobalIgnoreClassProperty(__classid(TDateTimePicker), L"Format");
	#ifdef _DEBUG
		UnicodeString path = ExtractFilePath(ExtractFileDir(ParamStr(0))) + L"locale";
	#else
		UnicodeString path = ExtractFilePath(ParamStr(0)) + L"locale";
	#endif
		bindtextdomain(L"wphfgui", path);
		bindtextdomain(L"languages", path);
		bindtextdomain(L"delphi", path);
		//Delphi strings translation
		AddDomainForResourceString(L"delphi");
		//default domain for text translations
		textdomain(L"wphfgui");

		Application->Initialize();
		Application->MainFormOnTaskBar = true;
		Application->Title = "Winprint HylaFAX Reloaded";
		Application->CreateForm(__classid(TFAXSend), &FAXSend);
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
