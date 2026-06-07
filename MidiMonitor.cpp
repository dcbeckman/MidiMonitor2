/* 
	MidiMonitor.cpp
	
	Copyright Bill Thibault, All Rights Reversed
	
	Keep track of the last MIDI message seen of each type.
	Display is via a separate View's Pulse() method.
*/

#include "MidiMonitor.h"

M2BMidiMonitor::M2BMidiMonitor ( void )
	: BMidiLocalConsumer ( "MidiMonitor2" )
{
	lastSeen[CHANNEL_PRESSURE] = 
		new M2BMidiLastSeenEntry ( "ChannelPressure", "pressure", "" );

	lastSeen[CONTROL_CHANGE] = 
		new M2BMidiLastSeenEntry ( "ControlChange", "controller", "value" );

	lastSeen[KEY_PRESSURE] = 
		new M2BMidiLastSeenEntry ( "KeyPressure", "note", "pressure" );

	lastSeen[NOTE_OFF] = 
		new M2BMidiLastSeenEntry ( "NoteOff", "note", "velocity" );

	lastSeen[NOTE_ON] = 
		new M2BMidiLastSeenEntry ( "NoteOn", "note", "velocity" );

	lastSeen[PITCH_BEND] = 
		new M2BMidiLastSeenEntry ( "PitchBend", "lsb", "msb" );

	lastSeen[PROGRAM_CHANGE] = 
		new M2BMidiLastSeenEntry ( "ProgramChange", "program", "" );

	lastSeen[SYSTEM_COMMON] = 
		new M2BMidiLastSeenEntry ( "SystemCommon", "data1", "data2" );

	lastSeen[SYSTEM_EXCLUSIVE] = 
		new M2BMidiLastSeenEntry ( "SystemExclusive", "", "" );

	lastSeen[SYSTEM_REAL_TIME] = 
		new M2BMidiLastSeenEntry ( "SystemRealTime", "status", "" );

	lastSeen[TEMPO_CHANGE] = 
		new M2BMidiLastSeenEntry ( "TempoChange", "BPM", "" );

};

void	M2BMidiMonitor::ChannelPressure ( uchar channel, 
									  uchar pressure,
									  bigtime_t time  )
{
	M2BMidiLastSeenEntry *entry = lastSeen[CHANNEL_PRESSURE];
	entry->Lock();
	entry->channel = channel;
	entry->val1 = pressure;
	entry->Changed();
	entry->Unlock();
}
									
void	M2BMidiMonitor::ControlChange   ( uchar channel, 
									  uchar controlNumber,
									  uchar controlValue,
									  bigtime_t time)
{
	M2BMidiLastSeenEntry *entry = lastSeen[CONTROL_CHANGE];
	entry->Lock();
	entry->channel = channel;
	entry->val1 = controlNumber;
	entry->val2 = controlValue;
	entry->Changed();
	entry->Unlock();
}

void	M2BMidiMonitor::KeyPressure	   ( uchar channel, 
									  uchar note,
									  uchar pressure,
									  bigtime_t time  )
{
	M2BMidiLastSeenEntry *entry = lastSeen[KEY_PRESSURE];
	entry->Lock();
	entry->channel = channel;
	entry->val1 = note;
	entry->val2 = pressure;
	entry->Changed();
	entry->Unlock();
}

void	M2BMidiMonitor::NoteOff		   ( uchar channel, 
									  uchar note,
									  uchar velocity,
									  bigtime_t time )
{
	M2BMidiLastSeenEntry *entry = lastSeen[NOTE_OFF];
	entry->Lock();
	entry->channel = channel;
	entry->val1 = note;
	entry->val2 = velocity;
	entry->Changed();
	entry->Unlock();
}

void	M2BMidiMonitor::NoteOn		   ( uchar channel, 
									  uchar note,
									  uchar velocity,
									  bigtime_t time )
{
	M2BMidiLastSeenEntry *entry = lastSeen[NOTE_ON];
	entry->Lock();
	entry->channel = channel;
	entry->val1 = note;
	entry->val2 = velocity;
	entry->Changed();
	entry->Unlock();
}

void	M2BMidiMonitor::PitchBend	   ( uchar channel, 
									  uchar lsb,
									  uchar msb,
									  bigtime_t time )
{
	M2BMidiLastSeenEntry *entry = lastSeen[PITCH_BEND];
	entry->Lock();
	entry->channel = channel;
	entry->val1 = lsb;
	entry->val2 = msb;
	entry->Changed();
	entry->Unlock();
}

void	M2BMidiMonitor::ProgramChange   ( uchar channel, 
									  uchar programNumber,
									  bigtime_t time )
{
	M2BMidiLastSeenEntry *entry = lastSeen[PROGRAM_CHANGE];
	entry->Lock();
	entry->channel = channel;
	entry->val1 = programNumber;
	entry->Changed();
	entry->Unlock();
}

void	M2BMidiMonitor::SystemCommon   ( uchar status, 
									  uchar data1,
									  uchar data2,
									  bigtime_t time )
{
	M2BMidiLastSeenEntry *entry = lastSeen[SYSTEM_COMMON];
	entry->Lock();
	entry->channel = -1;
	entry->val1 = data1;
	entry->val2 = data2;
	entry->Changed();
	entry->Unlock();
}

void	M2BMidiMonitor::SystemExclusive   ( void *  data , 
									  size_t  dataLength ,
									  bigtime_t time )
{
	unsigned int	i;
	M2BMidiLastSeenEntry *entry = lastSeen[SYSTEM_EXCLUSIVE];
	entry->Lock();
	unsigned char *q;
	q = (unsigned char *)data;
	for ( i = 0; (i < dataLength) && (i < 1024); i++ )
		entry->sysexMessage[i] = *q++;
	if ( i != 1024 )
		entry->sysexMessage[i] = 0xf7;
	entry->sysexLength = dataLength; 
	entry->Changed();
	entry->Unlock();
}

void	M2BMidiMonitor::SystemRealTime   ( uchar status, 
									  bigtime_t time )
{
	M2BMidiLastSeenEntry *entry = lastSeen[SYSTEM_REAL_TIME];
	entry->Lock();
	entry->val1 = status;
	entry->Changed();
	entry->Unlock();
}

void	M2BMidiMonitor::TempoChange   ( int32 beatsPerMinute, 
									  bigtime_t time )
{
	M2BMidiLastSeenEntry *entry = lastSeen[TEMPO_CHANGE];
	entry->Lock();
	entry->val1 = beatsPerMinute;
	entry->Changed();
	entry->Unlock();
}

