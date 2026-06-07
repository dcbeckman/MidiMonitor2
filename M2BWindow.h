/*
	
	M2BWindow.h
	
	Copyright 1995 Bill Thibault, All Rights Reversed.
	
*/

#ifndef M2B_WINDOW_H
#define M2B_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

class M2BWindow : public BWindow {

public:
				M2BWindow(BRect frame); 
virtual	bool	QuitRequested();
virtual void	SaveRequested( entry_ref directory, 
							   const char *filename );
};

#endif
