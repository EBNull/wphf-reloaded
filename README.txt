=========================
Winprint HylaFAX Reloaded
=========================

1. Installation

Download and execute wphf-reloaded-setup.exe. The installer takes care of all the details.
Some notes:
- The installer optionally creates a new printer named "HylaFAX". Any printer with the same name is removed automatically during setup.
- On earlier systems (XP or lower) the "Apple LaserWriter 16/600 PS" is used (driver already included in the system). On newer systems (Vista or higher) the "Dell 3100cn PS" is used. A copy of the driver is included in the installer itself. I don't know if this breaks any licenses. I downloaded the drivers from the Microsoft website (http://catalog.update.microsoft.com/v7/site/Home.aspx). The driver is freely downloadable by anyone and it comes with no legal notices except one from Adobe that states that the files can be redistributed as long as they are not altered. So I thought I could embed all this stuff into my installer. This should make people happy.
- Working files (configuration, temporary files) are placed into a subdirectory under the common application data folder (which by default is "C:\Documents and Settings\All Users\Application Data" under XP and "C:\ProgramData" under Vista / 7). The subdirectory is named "Winprint HylaFAX Reloaded".
- The address book is now in CSV format, with delimiter=comma (,) and quote character=double quotes ("). The user can configure the path to the address book. The file name is fixed: addressbook.csv. If you don't like it, you can complain on Sourceforge :-) . If you had the old Winprint HylaFAX previously installed and you filled up the old "names.txt" and "numbers.txt", just copy them to the new address book location: the GUI will convert them automatically. The address book is placed in the same directory as the ini file, if not configured otherwise.
- The configuration now takes place in the GUI. The first time a fax is sent, the GUI prompts for the configuration. Parameters are written to an ini file.


2. Usage

Print your documents to "HylaFAX" printer. The GUI appears; you can send the document immediately, or append other documents. All collected documents can be sent as a single fax. So you can, for example, print a FAX cover with Word, then print something from Excel, and finally send all the pages in one fax call. You can arrange the order of the documents using the up/down arrows or dragging items inside the list box.


3. Technical details

This software is derived from the popular HylaFAX client for Windows, Winprint HylaFAX, by Michael Stowe (http://sourceforge.net/projects/winprinthylafax/). The original version was modified so as to work under new operating systems (Vista, 7), both 32 and 64 bit. To accomplish this, the software has been splitted into two components:
- a port monitor, derived from Mfilemon - Multi File Port Monitor, by Lorenzo Monti (aka lomo74, myself. http://sourceforge.net/projects/mfilemon/);
- a GUI, for user interaction.
The port monitor receives data (Postscript) from the printer driver and writes it to a temporary file. Then it tries to send the name of the file to the GUI, through a pipe. If the port monitor does not find the pipe (the GUI is not running), it launches the GUI on the user desktop. This is the key of the transition to Vista / 7: the GUI has moved to a separate process, thus allowing it to appear on the user's desktop rather than on a protected desktop. Moreover, the port monitor is provided in two flavours (32 and 64 bit), thus allowing execution on 64 bit systems.


4. License

This software is released under the terms of the GNU GPL license. See http://www.gnu.org/licenses/gpl.html for details.


5. Bugs

This software is in the beta stage. Many improvements must be done yet. If you find any bug or have suggestions, please use SourceForge support facilities to submit your report. Thank you. If you want to contribute, drop me a line.
November 23, 2011 update: terminal services support, added in rel. 0.3.5, is not perfect yet. The big issue is: given a print job, from which desktop did that print job come from? The software uses a trick: enumerate all desktops (console plus all RDP connections) until one is found belonging to the same user that submitted the print job; then the GUI is executed on that desktop. The drawback is that if there are two or more sessions owned by the same user account, the GUI will appear on the first one it finds.
