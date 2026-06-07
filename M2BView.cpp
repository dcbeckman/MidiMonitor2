/*
	M2BView.cpp
		Copyright 1996,1997,1998 Bill Thibault, All Rights Reversed.
*/

#include <Alert.h>
#include <View.h>
#include <Bitmap.h>
#include <stdio.h>
#include "M2BView.h"

//#define BG_COLOR 255,255,255,255
#define BG_COLOR 160,160,160,255

/**************************************************************/

M2BBitmapView::M2BBitmapView ( BRect rect, char *name, 
								M2BMidiMonitor *monitor,
								float menuHeight )
				: BView ( rect, name, 
			B_FULL_UPDATE_ON_RESIZE | 
				B_FRAME_EVENTS |
				B_WILL_DRAW |
				B_FOLLOW_ALL, 
				B_WILL_DRAW ),
				  midiMonitor ( monitor )
{}

void M2BBitmapView::AttachedToWindow()
{
	BView::AttachedToWindow();
//	SetFontName ( "Times New Roman" );
	// SetFontSize ( 18 );
	SetFontSize ( 12 );
	//SetViewColor ( BG_COLOR );
	SetViewColor ( B_TRANSPARENT_32_BIT );
	SetLowColor ( BG_COLOR );
}

/**************************************************************/

M2BView::M2BView ( BRect rect, char *name, M2BMidiMonitor *monitor, float menuHeight )
			: BView ( rect, name, B_FOLLOW_ALL, 
			B_FULL_UPDATE_ON_RESIZE | 
			B_WILL_DRAW | 
			B_FRAME_EVENTS | 
			B_PULSE_NEEDED ),
			  midiMonitor ( monitor )
{
	// offscreen bitmap and view
	menuBarOffset = menuHeight;
	rect.bottom -= menuBarOffset;
	theBitmap = new BBitmap ( rect, B_RGB_32_BIT, TRUE );
	theBitmapView = new M2BBitmapView ( rect, "M2BBitmapView", monitor  );
	theBitmap->AddChild ( theBitmapView );
}

void M2BView::AttachedToWindow()
{
	BView::AttachedToWindow();
//	SetFontName ( "Times New Roman" );
	// SetFontSize ( 18 );
	SetFontSize ( 12 );
	//SetViewColor ( BG_COLOR );
	SetViewColor ( B_TRANSPARENT_32_BIT);
	//SetLowColor ( BG_COLOR );
}

void		
M2BView::FrameResized(float new_width, float new_height)
{
	BRect	rect;
		
	rect.Set ( 0,0, new_width, new_height - menuBarOffset );
	theBitmap->RemoveChild ( theBitmapView );
	delete theBitmapView;
	delete theBitmap;
	theBitmap = new BBitmap ( rect , B_RGB_32_BIT, TRUE );
	theBitmapView = new M2BBitmapView ( rect, "M2BBitmapView", midiMonitor  );
	theBitmap->AddChild ( theBitmapView );
	Draw ( Bounds() );
}



void M2BView::Draw ( BRect updateRect )
{
	if ( theBitmapView->LockLooper() ) {
	
		BRect updateBitmapRect = updateRect;
		updateBitmapRect.OffsetBy ( 0, -menuBarOffset );

		theBitmapView->Draw ( updateBitmapRect );
		theBitmapView->Sync();
	
		theBitmapView->UnlockLooper();
	}
	DrawBitmap ( theBitmap, BPoint (0.0, menuBarOffset) );
}


void M2BBitmapView::Draw ( BRect updateRect ) {
	int						i;
	long					age;
	int						intensity;
	float					width, height;
	float					top, bottom, middle;
	float					col1, col2, col3, col4;
	M2BMidiLastSeenEntry	*entry;
	char					str[10];
	
	// when drawing to a BBitmap, need to clear yourself
	SetHighColor ( BG_COLOR );
	FillRect ( updateRect );
	
	width = Frame().Width();
	height = Frame().Height();
	if ( width < height ) 
		SetFontSize ( width / 25.0 );
	else
		SetFontSize ( height / 25.0 );
		
	height = height / M2B_NUM_MIDI_MESSAGE_TYPES;

	col1 = 10;
//	col2 = col1 + width/4 + 10;
//	col3 = col2 + width/4;
//	col4 = col3 + width/4;
	col2 = width/3;
	col3 = col2 + 2*width/9;
	col4 = col3 + 2*width/9;
	
	for ( i=0; i < M2B_NUM_MIDI_MESSAGE_TYPES; i++ ) {

		top = i * height;
		bottom = (i+1) * height;
		middle = bottom - 2*height/3;

		entry = midiMonitor->lastSeen[i];
		entry->Lock();

		if ( updateRect.Intersects ( BRect ( 0, top, width, bottom-1 ) ) ) {
		
			if ( entry->ChangedRecently() ) {
				age = entry->Age();
				intensity = 255 - age * 2;
				if ( intensity < 0 ) 
					intensity = 0;
				// SetHighColor ( intensity, 0, 0, 255 );
				SetHighColor ( 0, intensity, 0, 255 );
			} else {
				SetHighColor ( 0, 0, 0, 255 );
			}
			
			/* draw heading */
//			SetFontName ( "Baskerville MT" );
			MovePenTo ( BPoint ( col1, bottom-1 ) );
			DrawString ( entry->mainLabel );
			if ( i < 7 ) {
				MovePenTo ( BPoint ( col2, middle ) );
				DrawString ( "channel" );
			}
			MovePenTo ( BPoint ( col3, middle ) );
			DrawString ( entry->label1 );
			MovePenTo ( BPoint ( col4, middle ) );
			DrawString ( entry->label2 );

			/* draw data */
//			SetFontName ( "Courier New Bold" );
			if ( entry->channel != -1 ) {
				MovePenTo ( BPoint ( col2, bottom-1 ) );
				snprintf ( str, sizeof(str), "%ld", entry->channel );
				DrawString ( str );
			}
			if ( entry->val1 != -1 ) {
				MovePenTo ( BPoint ( col3, bottom-1 ) );
				snprintf ( str, sizeof(str), "%ld", entry->val1 );
				DrawString ( str );
			}
			if ( entry->val2 != -1 ) {
				MovePenTo ( BPoint ( col4, bottom-1 ) );
				snprintf ( str, sizeof(str), "%ld", entry->val2 );
				DrawString ( str );
			}
			if ( i == SYSTEM_EXCLUSIVE ) {
				if ( entry->sysexLength > 0 ) {
					MovePenTo ( BPoint (col2, bottom-1) );
					snprintf ( str, sizeof(str), "%x ", 0xf0 );
					DrawString ( str );
				}
				for ( int j = 0; j < entry->sysexLength ; j++ ) {
					snprintf ( str, sizeof(str), "%x ", entry->sysexMessage[j] );
					DrawString ( str );
				}
				if ( entry->sysexLength > 0 ) {
					DrawString ("f7");
				}
			}
			StrokeLine ( BPoint(0,bottom-1), BPoint (width,bottom-1) );
		}	
		entry->SetAge(entry->Age()+8);	
		entry->Unlock();
	}
}
	

		
void M2BView::Pulse ()
{

	int						i;
	bool					firsttime;
	float					width, height;
	float					bottom, top;
	BRect					toInvalidate; 
	
	width = Frame().Width();
	height = Frame().Height() / M2B_NUM_MIDI_MESSAGE_TYPES;
	firsttime = TRUE;
	
	for ( i=0; i < M2B_NUM_MIDI_MESSAGE_TYPES; i++ ) {
		if ( midiMonitor->lastSeen[i]->ChangedRecently() ) {
			top = i * height + menuBarOffset;
			bottom = top + height;
			if ( firsttime ) {
				toInvalidate = 
							BRect (0, top, Frame().Width(), bottom-1);
				firsttime = FALSE;
			} else
				toInvalidate = toInvalidate |
							BRect (0, top, Frame().Width(), bottom-1);
		}
	}
	if ( !firsttime )
		Draw ( toInvalidate );

}

void
M2BView::MessageReceived ( BMessage *message )
{
	BView::MessageReceived ( message );
}