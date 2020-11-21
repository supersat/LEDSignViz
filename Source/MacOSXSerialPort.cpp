/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

MacOSXSerialPort.cpp: macOS-specific serial port support.

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

#if defined(__APPLE__) && defined(__MACH__)
#include "MacOSXSerialPort.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <memory.h>
#include <termios.h>
#include <unistd.h>
#include <IOKit/serial/ioss.h>

MacOSXSerialPort::MacOSXSerialPort() : fd(-1)
{
}

MacOSXSerialPort::~MacOSXSerialPort()
{
	if (fd != -1)
		closeSerialPort();
}

bool MacOSXSerialPort::openSerialPort(const String& identifier)
{
	speed_t speed = 2000000;
	struct termios attrs;

	fd = open(identifier.getCharPointer(), O_RDWR | O_NONBLOCK | O_NOCTTY);
	if (fd == -1)
		return false;
	
	if (ioctl(fd, TIOCEXCL) == -1)
		goto error;
	if (fcntl(fd, F_SETFL, 0) == -1)
		goto error;

	tcgetattr(fd, &attrs);
	memcpy(&savedAttrs, &attrs, sizeof(savedAttrs));

	attrs.c_iflag = 0;
	attrs.c_oflag = 0;
	attrs.c_cflag = CS8 | CLOCAL | CCTS_OFLOW | CRTS_IFLOW;
	attrs.c_lflag = 0;
	attrs.c_cc[VMIN] = 0;
	attrs.c_cc[VTIME] = 1;
	if (tcsetattr(fd, TCSANOW, &attrs) == -1)
		goto error;

	if (ioctl(fd, IOSSIOSPEED, &speed) == -1)
		goto error;

	tcflush(fd, TCIOFLUSH);
	tcflow(fd, TCOON);

	return true;

error:
	closeSerialPort();
	return false;
}

bool MacOSXSerialPort::sendBytes(char *bytes, unsigned int numBytes)
{
	return write(fd, bytes, numBytes) == numBytes;
}

void MacOSXSerialPort::closeSerialPort()
{
	//tcsetattr(fd, TCSADRAIN, &savedAttrs);
	tcsetattr(fd, TCSANOW, &savedAttrs);
	close(fd);
}

#endif // defined(__APPLE__) && defined(__MACH__)
