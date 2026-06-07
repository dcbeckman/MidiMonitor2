/*
	
	M2BView.h
	
	Copyright 1996 Bill Thibault, All Rights Reversed.
	
*/

#ifndef M2B_VIEW_H
#define M2B_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif

#include "MidiMonitor.h"

class M2BBitmapView : public BView {
public:
		M2BMidiMonitor	*midiMonitor;
						M2BBitmapView ( BRect frame, char *name,
									M2BMidiMonitor *midiMonitor,
									float menuHeight = 0.0 );
virtual	void			AttachedToWindow();
virtual	void			Draw ( BRect updateRect );
};

class M2BView : public BView {
	private:
		M2BMidiMonitor	*midiMonitor;
		float			menuBarOffset;

	protected:
		BBitmap			*theBitmap;
		M2BBitmapView	*theBitmapView;

	public:
								M2BView( BRect frame, char *name,
								M2BMidiMonitor *midiMonitor,
								float menuHeight = 0.0 ); 
		virtual	void			AttachedToWindow();
		virtual	void			Draw( BRect updateRect );
		virtual void			Pulse ( );
		virtual void			FrameResized ( float w, float h );
		virtual void			MessageReceived ( BMessage *message );
};

#endif
