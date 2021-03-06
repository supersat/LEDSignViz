/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

PlatformSerialPortManager.h: An interface to enumerate and open serial ports.

Copyright (C) 2012-2020  Karl Koscher

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

#ifndef __PLATFORMSERIALPORTMANAGER_H_CFD9F592__
#define __PLATFORMSERIALPORTMANAGER_H_CFD9F592__

#include "../JuceLibraryCode/JuceHeader.h"

class PlatformSerialPort;

class PlatformSerialPortManager
{
public:
	virtual ~PlatformSerialPortManager();
	virtual StringPairArray getSerialPorts() = 0;
	virtual PlatformSerialPort* createAndOpenSerialPort(const String& identifier) = 0;

	static PlatformSerialPortManager& getSerialPortManager();
};


#endif  // __PLATFORMSERIALPORTMANAGER_H_CFD9F592__
