/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

SerialThread.h: Converts and streams visualization graphics to the LED signs.

Copyright (C) 2012  Karl Koscher

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

==============================================================================
*/

#ifndef __SERIALTHREAD_H_66017D05__
#define __SERIALTHREAD_H_66017D05__

#include "../JuceLibraryCode/JuceHeader.h"
#include "PlatformSerialPort.h"

class LedsignVizAudioProcessor;

//==============================================================================
/**
*/
class LedsignVizSerialThread : public Thread
{
public:
    LedsignVizSerialThread (LedsignVizAudioProcessor* ownerFilter);
    virtual ~LedsignVizSerialThread();

	virtual void run();
private:
	LedsignVizAudioProcessor* processor;
	PlatformSerialPort* serialPort;
};




#endif  // __SERIALTHREAD_H_66017D05__
