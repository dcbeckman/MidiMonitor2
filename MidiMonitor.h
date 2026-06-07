/*
	MidiMonitor.h
	
	Copyright 1996 Bill Thibault, All Rights Reversed.
	
*/

#ifndef MIDI_MONITOR_H
#define MIDI_MONITOR_H

#include <midi2/MidiConsumer.h>
#include <Locker.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
	CHANNEL_PRESSURE,
	CONTROL_CHANGE,
	KEY_PRESSURE,
	NOTE_OFF,
	NOTE_ON,
	PITCH_BEND,
	PROGRAM_CHANGE,
	SYSTEM_COMMON,
	SYSTEM_EXCLUSIVE,
	SYSTEM_REAL_TIME,
	TEMPO_CHANGE } M2BWhichMidi;

#define M2B_NUM_MIDI_MESSAGE_TYPES 11

class M2BMidiLastSeenEntry : public BLocker
{
public:
	long			channel;
	long			val1;
	long			val2;
	long			age;
	int				maxAge;
	char			*mainLabel;
	char			*label1;
	char			*label2;
	unsigned char	*sysexMessage;
	long			sysexLength;

	inline 			M2BMidiLastSeenEntry ( const char *main, const char *l1, const char *l2 )
						{ mainLabel = strdup ( main );
						  label1 = strdup ( l1 );
						  label2 = strdup ( l2 );
						  channel = val1 = val2 = -1;
						  age = 0;
						  maxAge = 128; 
						  if ( !strcmp(main,"SystemExclusive") ) {
						  	sysexLength = 0;
						  	sysexMessage = new unsigned char[1024];
						  } else sysexMessage = NULL;
						};
	inline			~M2BMidiLastSeenEntry () {
						  free ( mainLabel );
						  free ( label1 );
						  free ( label2 );
						  if (sysexMessage) 
							delete[] sysexMessage;
						};
	inline bool		ChangedRecently ( void ) { return age < maxAge; };
	inline long		Age ( void ) { return age; };
	inline void		SetAge ( long newage ) { age = newage; };
	inline void		Changed ( void ) { age = 0; };
	inline void		SetValues ( long chan, long v1, long v2 )
						{ channel = chan; val1 = v1; val2 = v2; };
};

class M2BMidiMonitor : public BMidiLocalConsumer 
{
public:
	M2BMidiLastSeenEntry	*lastSeen[M2B_NUM_MIDI_MESSAGE_TYPES];
	
					M2BMidiMonitor ( );

	virtual void	ChannelPressure ( uchar channel, 
									  uchar pressure,
									  bigtime_t time );
	virtual void	ControlChange   ( uchar channel, 
									  uchar controlNumber,
									  uchar controlValue,
									  bigtime_t time );
	virtual void	KeyPressure	   ( uchar channel, 
									  uchar note,
									  uchar pressure,
									  bigtime_t time );
	virtual void	NoteOff		   ( uchar channel, 
									  uchar note,
									  uchar velocity,
									  bigtime_t time );
	virtual void	NoteOn		   ( uchar channel, 
									  uchar note,
									  uchar velocity,
									  bigtime_t time );
	virtual void	PitchBend	   ( uchar channel, 
									  uchar lsb,
									  uchar msb,
									  bigtime_t time );
	virtual void	ProgramChange   ( uchar channel, 
									  uchar programNumber,
									  bigtime_t time );
	virtual void	SystemExclusive   ( void *data, 
									  size_t dataLength,
									  bigtime_t time );
	virtual void	SystemCommon   ( uchar status, 
									  uchar data1,
									  uchar data2,
									  bigtime_t time );
	virtual void	SystemRealTime   ( uchar status, 
									  bigtime_t time );
	virtual void	TempoChange   ( int32 beatsPerMinute, 
									  bigtime_t time );

};	

#endif