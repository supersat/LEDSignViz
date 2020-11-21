/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

Win32SerialPort.cpp: Win32-specific serial port support.

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

#include "Win32SerialPort.h"


Win32SerialPort::Win32SerialPort(void) : hComPort(INVALID_HANDLE_VALUE)
{
}


Win32SerialPort::~Win32SerialPort(void)
{
	if (hComPort != INVALID_HANDLE_VALUE)
		closeSerialPort();
}

bool Win32SerialPort::openSerialPort(const String& identifier)
{
	DCB dcb;

	hComPort = CreateFileW(identifier.toWideCharPointer(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hComPort != INVALID_HANDLE_VALUE) {
		dcb.DCBlength = sizeof(DCB);
		GetCommState(hComPort, &dcb);
		dcb.BaudRate = 2000000;
		dcb.ByteSize = 8;
		dcb.fBinary = TRUE;
		dcb.fParity = FALSE;
		dcb.StopBits = ONESTOPBIT;
		dcb.Parity = NOPARITY;
		dcb.fOutxCtsFlow = TRUE;
		SetCommState(hComPort, &dcb);
		return true;
	}

	return false;
}

bool Win32SerialPort::sendBytes(char *bytes, unsigned int numBytes)
{
	DWORD bytesWritten;

	if (WriteFile(hComPort, bytes, numBytes, &bytesWritten, NULL))
		return bytesWritten == numBytes;
	else
		return false;
}

void Win32SerialPort::closeSerialPort()
{
	CloseHandle(hComPort);
	hComPort = INVALID_HANDLE_VALUE;
}

#endif // _WIN32
