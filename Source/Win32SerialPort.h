/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

Win32SerialPort.h: Win32-specific serial port support.

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

#ifndef __WIN32SERIALPORT_H__
#define __WIN32SERIALPORT_H__

#include "PlatformSerialPort.h"
#include <Windows.h>
#include <string>

class Win32SerialPort : public PlatformSerialPort
{
public:
	Win32SerialPort();
	virtual ~Win32SerialPort();

	virtual bool openSerialPort(const String& identifier);
	virtual bool sendBytes(char *bytes, unsigned int numBytes);
	virtual void closeSerialPort();

private:
	HANDLE hComPort;
};

#endif // __WIN32SERIALPORT__

