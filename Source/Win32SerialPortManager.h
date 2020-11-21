/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

Win32SerialPortManager.h: Win32-specific implementation of PlatformSerialPortManager.

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

=============
*/

#ifndef __WIN32SERIALPORTMANAGER_H_A92F5B44__
#define __WIN32SERIALPORTMANAGER_H_A92F5B44__

#include "PlatformSerialPortManager.h"

class Win32SerialPortManager : public PlatformSerialPortManager
{
public:
    virtual ~Win32SerialPortManager();
	virtual StringPairArray getSerialPorts();
	virtual PlatformSerialPort* createAndOpenSerialPort(const String& identifier);
};


#endif  // __WIN32SERIALPORTMANAGER_H_A92F5B44__
