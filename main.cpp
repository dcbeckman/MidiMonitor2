//
// main.cpp - midi monitor app
//	Copyright 1998 Bill Thibault, All Rights Reversed.
//


#include <midi2/MidiRoster.h>
#include <midi2/MidiConsumer.h>
#include <midi2/MidiProducer.h>
#include <Entry.h>
#include <Path.h>
#include <Application.h>
#include <Alert.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <string.h>
#include <stdio.h>
#include "MidiMonitor.h"
#include "M2BWindow.h"
#include "M2BView.h"


class MidiMonitorApp : public BApplication {
	public:
				MidiMonitorApp ( void );
				~MidiMonitorApp ( void );
		virtual void MessageReceived ( BMessage *msg );
	private:
		BMidiProducer	*connectedProducer;
		M2BMidiMonitor	*theMidiMonitor;
};

#define MSG_MIDIMONITOR_MIDI_PORT 'BUmp'

MidiMonitorApp::MidiMonitorApp ( void ) 
	: BApplication ("application/x-vnd.tebo-midimonitor2")
{
	BRect aRect;
	BList windows;
	const float menuHeight = 20;
	int	initPort = 0; // default initial port index to select
	
	connectedProducer = NULL;
	theMidiMonitor = new M2BMidiMonitor ();
	theMidiMonitor->Register();

	aRect.Set(20, 100, 340, 400);
	BWindow *aWindow = (BWindow *) new M2BWindow(aRect);
	aRect.OffsetTo(B_ORIGIN);
	BView *aView = (BView *) new M2BView(aRect, "M2BView", 
					(M2BMidiMonitor*)theMidiMonitor, menuHeight);
	aWindow->AddChild(aView);
	aWindow->Show();
	windows.AddItem ( aWindow );

	aRect.Set ( 0,0,1000,menuHeight);
	BMenuBar	*menubar = new BMenuBar ( aRect, "menubar" );
	menubar->SetBorder ( B_BORDER_FRAME );

	BMenuItem *item; 
	BMenu *menu = new BMenu("File"); 

	item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'); 
	item->SetTarget(be_app); 
	menu->AddItem(item); 

	menubar->AddItem ( menu );
	
	// add the port selection menu
	menu = new BMenu ( "Port" );
	
	int32 id = 0;
	int32 count = 0;
	BMidiProducer *prod;
	while ((prod = BMidiRoster::NextProducer(&id)) != NULL) {
		item = new BMenuItem ( prod->Name(), 
								new BMessage ( MSG_MIDIMONITOR_MIDI_PORT ) );
		item->Message()->AddInt32 ( "port_id", prod->ID() );
		menu->AddItem ( item );
		
		if (count == initPort) {
			connectedProducer = prod;
			connectedProducer->Acquire();
		}
		
		prod->Release();
		count++;
	}

	if (count == 0) {
		item = new BMenuItem("No ports found", NULL);
		item->SetEnabled(false);
		menu->AddItem(item);
	} else {
		menu->SetRadioMode ( TRUE );
		if (menu->ItemAt(initPort) != NULL) {
			menu->ItemAt(initPort)->SetMarked ( TRUE );
		}
		menu->SetTargetForItems ( be_app );	
	}
	
	menubar->AddItem ( menu );

	if (connectedProducer != NULL) {
		connectedProducer->Connect(theMidiMonitor);
	}

	aView->AddChild ( menubar );
}


MidiMonitorApp::~MidiMonitorApp ( void )
{
	if (connectedProducer != NULL) {
		connectedProducer->Disconnect(theMidiMonitor);
		connectedProducer->Release();
	}
	theMidiMonitor->Unregister();
	theMidiMonitor->Release();
}



void
MidiMonitorApp::MessageReceived ( BMessage *msg )
{
	switch ( msg->what ) {
		case MSG_MIDIMONITOR_MIDI_PORT: {
			int32 newId = msg->FindInt32 ( "port_id" );
			if (connectedProducer != NULL) {
				connectedProducer->Disconnect(theMidiMonitor);
				connectedProducer->Release();
				connectedProducer = NULL;
			}
			connectedProducer = BMidiRoster::FindProducer(newId);
			if (connectedProducer != NULL) {
				if (connectedProducer->Connect(theMidiMonitor) != B_OK) {
					char str[128];
					snprintf ( str, sizeof(str), "can't connect to %s", connectedProducer->Name() );
					BAlert *bug = new BAlert ( "", str, "sigh" );
					bug->Go();
					connectedProducer->Release();
					connectedProducer = NULL;
				}
			}
			break;
		}
		default:
			BApplication::MessageReceived ( msg );
			break;
	}
}





int main ( void ) {
	MidiMonitorApp	app;
	app.Run();
	return 0;
}
	