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
#include <Messenger.h>
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
		void         _RebuildPortMenu ( void );
		BMenu        *fPortMenu;
		BMidiProducer	*connectedProducer;
		M2BMidiMonitor	*theMidiMonitor;
};

#define MSG_MIDIMONITOR_MIDI_PORT 'BUmp'

MidiMonitorApp::MidiMonitorApp ( void )
	: BApplication ("application/x-vnd.dcbeckman-midimonitor2")
{
	BRect aRect;
	BList windows;
	const float menuHeight = 20;

	connectedProducer = NULL;
	theMidiMonitor = new M2BMidiMonitor ();
	// By not calling Register(), the consumer remains private.
	// It won't show up in the system roster (so Patchbay can't connect other ports to it),
	// but we can still programmatically connect it to any producer we select from the menu.
	// theMidiMonitor->Register();

	aRect.Set(20, 100, 340, 400);
	BWindow *aWindow = (BWindow *) new M2BWindow(aRect);
	BView *aView = (BView *) new M2BView(aWindow->Bounds(), "M2BView",
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

	fPortMenu = new BMenu ( "Port" );
	menubar->AddItem ( fPortMenu );
	aView->AddChild ( menubar );

	_RebuildPortMenu();

	// Watch for endpoints being added/removed so the Port menu stays current
	BMessenger me(this);
	BMidiRoster::StartWatching(&me);
}


MidiMonitorApp::~MidiMonitorApp ( void )
{
	BMidiRoster::StopWatching();

	if (connectedProducer != NULL) {
		connectedProducer->Disconnect(theMidiMonitor);
		connectedProducer->Release();
		connectedProducer = NULL;
	}
	// Release() automatically unregisters — do NOT call Unregister() before
	// Release() or the endpoint refcount double-dips and the destructor fires
	// debugger("you should use Release()...").
	theMidiMonitor->Release();
}


void
MidiMonitorApp::_RebuildPortMenu ( void )
{
	int32 prevId = (connectedProducer != NULL) ? connectedProducer->ID() : -1;

	if (connectedProducer != NULL) {
		connectedProducer->Disconnect(theMidiMonitor);
		connectedProducer->Release();
		connectedProducer = NULL;
	}

	while (fPortMenu->CountItems() > 0)
		delete fPortMenu->RemoveItem((int32)0);

	int32 id = 0;
	int32 count = 0;
	int32 connectIdx = 0;
	BMidiProducer *prod;
	while ((prod = BMidiRoster::NextProducer(&id)) != NULL) {
		BMenuItem *item = new BMenuItem ( prod->Name(),
		                                  new BMessage ( MSG_MIDIMONITOR_MIDI_PORT ) );
		item->Message()->AddInt32 ( "port_id", prod->ID() );
		item->SetTarget(be_app);
		fPortMenu->AddItem ( item );

		// Re-select the same producer if still present, else fall through to first
		if (connectedProducer == NULL &&
		    (prod->ID() == prevId || prevId == -1)) {
			connectedProducer = prod;
			connectedProducer->Acquire();
			connectIdx = count;
		}

		prod->Release();
		count++;
	}

	if (count == 0) {
		BMenuItem *noItem = new BMenuItem("No ports found", NULL);
		noItem->SetEnabled(false);
		fPortMenu->AddItem(noItem);
	} else {
		// Fall back to first entry if previous producer disappeared
		if (connectedProducer == NULL) {
			BMenuItem *first = fPortMenu->ItemAt(0);
			if (first != NULL) {
				connectedProducer = BMidiRoster::FindProducer(
				    first->Message()->FindInt32("port_id"));
				connectIdx = 0;
			}
		}
		fPortMenu->SetRadioMode(TRUE);
		BMenuItem *sel = fPortMenu->ItemAt(connectIdx);
		if (sel != NULL)
			sel->SetMarked(TRUE);
		fPortMenu->SetTargetForItems(be_app);
	}

	if (connectedProducer != NULL)
		connectedProducer->Connect(theMidiMonitor);
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
		case 'MReg':   // B_MIDI_REGISTERED
		case 'MUnr':   // B_MIDI_UNREGISTERED
			_RebuildPortMenu();
			break;
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
