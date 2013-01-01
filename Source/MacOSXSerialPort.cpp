/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

MacOSXSerialPort.cpp: OS X-specific serial port support.

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

#if defined(__APPLE__) && defined(__MACH__)
#include "MacOSXSerialPort.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
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
	fd = open(identifier.getCharPointer(), O_RDWR | O_SYNC | O_NOCTTY);
	if (fd == -1)
		return false;
	
	ioctl(fd, TIOCEXCL);

	struct termios attrs;

	tcgetattr(fd, &attrs);
	memcpy(&savedAttrs, &attrs, sizeof(savedAttrs));

	attrs.c_iflag = 0;
	attrs.c_oflag = 0;
	attrs.c_cflag = CS8 | CLOCAL | CREAD | CRTSCTS;
	attrs.c_lflag = 0;
	attrs.c_cc[VMIN] = 1;
	attrs.c_cc[VTIME] = 0;
	tcsetattr(fd, TCSANOW, &attrs);

	speed_t speed = 2000000;
	if (ioctl(fd, IOSSIOSPEED, &speed) == -1) {
		closeSerialPort();
		return false;
	}

	return true;
}

bool MacOSXSerialPort::sendBytes(char *bytes, unsigned int numBytes)
{
	return write(fd, bytes, numBytes) == numBytes;
}

void MacOSXSerialPort::closeSerialPort()
{
	tcsetattr(fd, TCSADRAIN, &savedAttrs);
	close(fd);
}

#endif // defined(__APPLE__) && defined(__MACH__)