FFmpeg play (fmgplay) r11656-11
===============================

Introduction
------------
	This is a FFmpeg player plugin for the PM123.

	FFmpeg is a complete solution to record, convert and stream 
        audio and video, developed and maintained by group of authors
	at http://ffmpeg.mplayerhq.hu/.

Features
--------
	Based on ffmpeg-SVN-r11656.
	Playback, seek, fast forward and rewind seems ok in common cases.

System requirements
-------------------
	OS/2 Warp 3 and up
	PM123 1.31 or later,
		http://glass.os2.spb.ru/software/english/pm123.html
	libc063.dll (latest known at the moment is from 
    	   	ftp://ftp.netlabs.org/pub/libc/libc-0_6_3-csd3.exe)
	

Warnings and limitations
------------------------
	This version is not hardly tested. Use with care!

	Extension-based format recognition disabled in this version. Some
	formats with altered header (ID3v1-prepended, for example) may be 
	false-recognised.

	flacplay-1.2.1-10 reported to incorrectly recognise .wma files
	(supported by fmgplay!) as .flac files, that may cause fail to play.
	To fix - place flacplay below fmgplay in plag-ins list of PM123 (remove 
	flacplay from list and add flacplay.dll again after fmgplay.dll added).

	Not all formats and codecs, supported by ffmpeg, are reported to work
	with fmgplay (ffmpeg needs gcc 4.2.x, but only gcc 3.3.5 is available).

	This time only the first audio stream played from a multi-stream file. 
	(This may be fixed later with PM123 interface update).

	Only limited set of tags (ID3v1?) supported by ffmpeg and hence 
	by fmgplay.  

	To avoid unnecessary pollution, no ffmpeg-supported extensions 
	added into PM123 open file dialog. Please try <all files (.*)> 
	file filter to select ffmpeg-supported files there.

Installation
------------
	- make it sure that libc063.dll located somewhere in your LIBPATH
	- place the file fmgplay.dll into the directory where PM123.EXE located
	- start PM123
	- Right-click on the PM123 window to open the "properties" dialog
	- Choose the page "plugins"
	- Press the "add" button in the "decoder plugin" section
	- Choose "shnplay.dll" in the file dialog.
	  Press Ok.
	- Close "PM123 properties" dialog

	Now you can listen more audio files (and audio streams from video files)! 

De-Installation
---------------
	In case of any trouble with this plugin close PM123 and remove 
	fmgplay.dll from the PM123.EXE directory.

License
-------
	Copyright (C) 2008 ntim <ntim@softhome.net>.
	The program is distributed under the terms of 
	the GNU General Public License, version 2 or any later (at you choise).
	Please see doc/COPYING.GPL for the full license terms.

	This program used a set of ffmpeg libraries, developed and maintained 
	by group of authors at http://ffmpeg.mplayerhq.hu/.
	Please see doc/ffmpeg-CREDITS for the list of authors.

	This program is based on pm123 plug-ins source, copyrighted by:
	 * Copyright 2004-2007 Dmitry A.Steklenev<glass@ptv.ru>
	 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
	 * Copyright 1997-2003 Taneli Lepp„ <rosmo@sektori.com>
	see doc/COPYRIGHT.html for license terms.

	The author is not responsible for any damage this program may cause.

Sources
-------
	To meet GNU GPL license terms, all sources and Makefile are attached in 
	src/ directory.

	For a correct remake, replace the config.h in the src/ directory with 
	the same file from configured and compiled ffmpeg build tree.

	PM123 PDK, set of ffmpeg libs, gcc-3.3.5-csd3 and 
	a pile of GNU developer tools are needed to remake.

Contacts
--------
	All questions about this build please send to ntim@softhome.net
	or contact ntim on #os2russian (irc://efnet/os2russian)

2008, ntim
