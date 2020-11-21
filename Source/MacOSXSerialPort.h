/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

MacOSXSerialPort.cpp: OS X-specific serial port support.

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

#ifndef __MACOSXSERIALPORT_H__
#define __MACOSXSERIALPORT_H__

#include "PlatformSerialPort.h"
#include <termios.h>
#include <string>

class MacOSXSerialPort : public PlatformSerialPort
{
public:
	MacOSXSerialPort();
	virtual ~MacOSXSerialPort();

	virtual bool openSerialPort(const String& identifier);
	virtual bool sendBytes(char *bytes, unsigned int numBytes);
	virtual void closeSerialPort();

private:
	int fd;
	struct termios savedAttrs;
};

#endif // __MACOSXSERIALPORT_H__
