# Makefile for MidiMonitor (Haiku OS Midi2 Kit version)

TARGET = MidiMonitor
SRCS = main.cpp MidiMonitor.cpp M2BWindow.cpp M2BView.cpp
OBJS = $(SRCS:.cpp=.o)
CC = g++
CFLAGS = -Wall -Wno-multichar -O2
LDFLAGS = -lbe -lmidi2

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
