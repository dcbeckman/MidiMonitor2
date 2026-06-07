/*
	M2BWindow.cpp	- displays an M2BView 
	
		Copyright 1996,1997,1998 Bill Thibault, All Rights Reversed.
*/

#include <Application.h>
#include <FindDirectory.h>
#include <Path.h>
#include <File.h>
#include <Message.h>
#include "M2BWindow.h"

M2BWindow::M2BWindow ( BRect frame )
			: BWindow ( frame, "MidiMonitor2", B_TITLED_WINDOW,
			/* B_NOT_RESIZABLE | B_NOT_ZOOMABLE */ 0, 0 )
{
	SetPulseRate ( 50000 ); // 50 ms

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("MidiMonitor2_settings");
		BFile file(path.Path(), B_READ_ONLY);
		BMessage settings;
		if (settings.Unflatten(&file) == B_OK) {
			BRect savedFrame;
			if (settings.FindRect("frame", &savedFrame) == B_OK) {
				MoveTo(savedFrame.left, savedFrame.top);
				ResizeTo(savedFrame.Width(), savedFrame.Height());
			}
		}
	}
}

bool M2BWindow::QuitRequested()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("MidiMonitor2_settings");
		BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		BMessage settings;
		settings.AddRect("frame", Frame());
		settings.Flatten(&file);
	}

	be_app->PostMessage ( B_QUIT_REQUESTED );
	return ( TRUE );
}


void
M2BWindow::SaveRequested ( entry_ref dir, const char *filename )
{
}