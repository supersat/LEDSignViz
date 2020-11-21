/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

PlatformSerialPortManager.cpp: An interface to enumerate and open serial ports.

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

#ifdef _WIN32
#include "Win32SerialPortManager.h"
Win32SerialPortManager theManager;
#endif
#if defined(__APPLE__) && defined(__MACH__)
#include "MacOSXSerialPortManager.h"
MacOSXSerialPortManager theManager;
#endif

PlatformSerialPortManager::~PlatformSerialPortManager()
{
}

PlatformSerialPortManager& PlatformSerialPortManager::getSerialPortManager()
{
	return theManager;
}
