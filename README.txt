=========================
Winprint HylaFAX Reloaded (with TS support)
=========================

1. Installation

There isn't any installer. You can use wphf-reloaded-setup.exe and just replace wphfmon.dll from this version, if you wish.

Otherwise:

* Copy wpfhmon.dll and wphfmonUI.dll to c:\windows\system32
* Run `regmon -r` to register the monitor

Add a printer:

* Use one of these printer drivers:

- Apple LaserWriter 12/640 PS (Windows XP)
- Apple LaserWriter 16/600 PS (Windows XP)
- Xerox Phaser 6250B PS (Windows 7)

* Choose the WPHF existing port.

2. Data

* Working files (configuration, temporary files) are placed into a subdirectory under the common application data folder (which by default is "C:\Documents and Settings\All Users\Application Data" under XP and "C:\ProgramData" under Vista / 7). The subdirectory is named "Winprint HylaFAX Reloaded".

* The address book is now in CSV format, with delimiter=comma (,) and quote character=double quotes ("). The user can configure the path to the address book. The file name is fixed: addressbook.csv. If you don't like it, you can complain on Sourceforge :-) . If you had the old Winprint HylaFAX previously installed and you filled up the old "names.txt" and "numbers.txt", just copy them to the new address book location: the GUI will convert them automatically. The address book is placed in the same directory as the ini file, if not configured otherwise.

* The configuration now takes place in the GUI. The first time a fax is sent, the GUI prompts for the configuration. Parameters are written to an ini file.


3. Usage

Print your documents to "HylaFAX" printer. The GUI appears; you can send the document immediately, or append other documents. All collected documents can be sent as a single fax. So you can, for example, print a FAX cover with Word, then print something from Excel, and finally send all the pages in one fax call. You can arrange the order of the documents using the up/down arrows or dragging items inside the list box.


4. Technical details

This software is derived from the popular HylaFAX client for Windows, Winprint HylaFAX, by Michael Stowe (http://sourceforge.net/projects/winprinthylafax/). The original version was modified so as to work under new operating systems (Vista, 7), both 32 and 64 bit. To accomplish this, the software has been split into two components:

- a port monitor, derived from Mfilemon - Multi File Port Monitor, by Lorenzo Monti (aka lomo74, myself. http://sourceforge.net/projects/mfilemon/);

- a GUI, for user interaction. (wphfgui.exe)

The port monitor receives data (Postscript) from the printer driver and writes it to a temporary file. Then it launches the GUI on the user desktop. This is the key of the transition to Vista / 7: the GUI has moved to a separate process, thus allowing it to appear on the user's desktop rather than on a protected desktop. Moreover, the port monitor is provided in two flavours (32 and 64 bit), thus allowing execution on 64 bit systems.

There is no named pipe used as it would need to change names depending on the end user's session ID.
Instead, the executable is launched each time, and it is up to the executable to notify itself if it is still running.

The executable is passed additional data as part of the process's environment variables, much like CGI:

- WPHF_computername
- WPHF_username
- WPHF_filename
- WPHF_printername
- WPHF_portname
- WPHF_jobid
- WPHF_jobtitle


4. License

This software is released under the terms of the GNU GPL license. See http://www.gnu.org/licenses/gpl.html for details.


5. Bugs

This software is in the beta stage. Many improvements must be done yet. If you find any bug or have suggestions, please use SourceForge support facilities to submit your report. Thank you. If you want to contribute, drop me a line.
