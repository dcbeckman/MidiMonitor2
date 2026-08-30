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
		virtual void ReadyToRun ( void );
		virtual void MessageReceived ( BMessage *msg );
	private:
		void         _RebuildPortMenu ( void );
		BWindow      *fWindow;
		BMenu        *fPortMenu;
		BMidiProducer	*connectedProducer;
		M2BMidiMonitor	*theMidiMonitor;
};

#define MSG_MIDIMONITOR_MIDI_PORT 'BUmp'

// ⚠️ NOTHING MIDI- OR WINDOW-RELATED BELONGS IN THIS CONSTRUCTOR.
//
// It runs before BApplication::Run(), so the app's message loop is not yet
// servicing messages. Any midi_server round trip made here -- constructing a
// BMidiLocalConsumer, walking BMidiRoster, StartWatching() -- blocks the main
// thread waiting for a reply that only the loop it is blocking can deliver.
//
// This is not hypothetical. Version 2.0.0-1 did all of it here and hung on
// launch on hrev59866+79 (2026-08-29): the team came up with exactly three
// threads -- main in `wait`, MidiEventThread, MidiRosterLooper -- and NO `w>`
// window thread, because execution never reached Show(). With B_SINGLE_LAUNCH
// in MidiMonitor.rdef, every later launch was then handed off to that hung
// instance, so the app looked permanently dead with no error anywhere.
//
// Do the work in ReadyToRun() instead. See haiku-agent-gotchas.md section 4.
MidiMonitorApp::MidiMonitorApp ( void )
	: BApplication ("application/x-vnd.indi_labs-MidiMonitor"),
	  fWindow ( NULL ),
	  fPortMenu ( NULL ),
	  connectedProducer ( NULL ),
	  theMidiMonitor ( NULL )
{
}


void
MidiMonitorApp::ReadyToRun ( void )
{
	BRect aRect;
	const float menuHeight = 20;

	// Guard against a second initialisation. A relaunch of a B_SINGLE_LAUNCH
	// app is delivered as B_SILENT_RELAUNCH, and BApplication's default
	// handler calls ReadyToRun() again -- which without this would build a
	// second window and a second consumer over the top of the first.
	if ( fWindow != NULL )
		return;

	theMidiMonitor = new M2BMidiMonitor ();
	// By not calling Register(), the consumer remains private.
	// It won't show up in the system roster (so Patchbay can't connect other ports to it),
	// but we can still programmatically connect it to any producer we select from the menu.
	// theMidiMonitor->Register();

	aRect.Set(20, 100, 340, 400);
	fWindow = (BWindow *) new M2BWindow(aRect);
	BView *aView = (BView *) new M2BView(fWindow->Bounds(), "M2BView",
					(M2BMidiMonitor*)theMidiMonitor, menuHeight);
	fWindow->AddChild(aView);

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

	// Show() last, once the whole view hierarchy is built. Before this point
	// the window has no thread of its own, so everything above runs
	// unsynchronised on the main thread rather than racing the window thread.
	fWindow->Show();

	// Watch for endpoints being added/removed so the Port menu stays current
	BMessenger me(this);
	BMidiRoster::StartWatching(&me);
}


MidiMonitorApp::~MidiMonitorApp ( void )
{
	// ReadyToRun() may never have run (an early quit), so nothing here may
	// assume the MIDI objects exist.
	if (theMidiMonitor == NULL)
		return;

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
	theMidiMonitor = NULL;
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
			// Endpoint churn can arrive before ReadyToRun() has built the
			// menu; there is nothing to rebuild yet if so.
			if (fPortMenu != NULL)
				_RebuildPortMenu();
			break;

		// A relaunch of a B_SINGLE_LAUNCH app (MidiMonitor.rdef) arrives here
		// rather than starting a second instance. Bring the existing window
		// forward -- otherwise clicking the Deskbar entry of an already-running
		// MidiMonitor appears to do nothing at all.
		case B_SILENT_RELAUNCH:
			if (fWindow != NULL && fWindow->Lock()) {
				fWindow->Activate(true);
				fWindow->Unlock();
			}
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
