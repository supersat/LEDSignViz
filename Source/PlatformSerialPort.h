/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

PlatformSerialPort.h: A platform-independent way to access serial ports.

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

#ifndef __PLATFORMSERIALPORT_H__
#define __PLATFORMSERIALPORT_H__

#include "../JuceLibraryCode/JuceHeader.h"

class PlatformSerialPort 
{
public:
	virtual ~PlatformSerialPort() = 0;

	virtual bool openSerialPort(const String& identifier) = 0;
	virtual bool sendBytes(char *bytes, unsigned int numBytes) = 0;
	virtual void closeSerialPort() = 0;
};

inline PlatformSerialPort::~PlatformSerialPort() { }

#endif // __PLATFORMSERIALPORT_H__
