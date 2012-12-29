/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

SerialThread.cpp: Converts and streams visualization graphics to the LED signs.

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

#include "SerialThread.h"
#include "PluginProcessor.h"

#ifdef _WIN32
#include "Win32SerialPort.h"
#endif

LedsignVizSerialThread::LedsignVizSerialThread(LedsignVizAudioProcessor* owner) : Thread("Serial Port Thread"), processor(owner)
{
#ifdef _WIN32
	serialPort = new Win32SerialPort();
#endif
}

LedsignVizSerialThread::~LedsignVizSerialThread() {
	delete serialPort;
}

void LedsignVizSerialThread::run() {
	unsigned int *localBitmap = new unsigned int[processor->signWidth * processor->signHeight];

	int bufSize = processor->signHeight * processor->signWidth / 8;
	char* redBuf = new char[bufSize];
	char* greenBuf = new char[bufSize];
	int count = 0;

	while (!threadShouldExit()) {
		if (serialPort->openSerialPort("\\\\.\\COM15")) {

			while (!threadShouldExit()) {
				processor->bitmapLock.enter();
				memcpy(localBitmap, processor->bitmap, sizeof(localBitmap));
				processor->bitmapLock.exit();

				memset(redBuf, 0, bufSize);
				memset(greenBuf, 0, bufSize);

				for (int y = 0; y < processor->signHeight; y++) {
					for (int x = 0; x < processor->signWidth; x++) {
						unsigned int color = localBitmap[(processor->signHeight - y - 1) * processor->signWidth + x];

						if ((color & 0xc000) > (count << 14))
							redBuf[(processor->signWidth / 8) * y + (x / 8)] |= 1 << (7 - (x % 8));
						if ((color & 0xc0) > (count << 6))
							greenBuf[(processor->signWidth / 8) * y + (x / 8)] |= 1 << (7 - (x % 8));
					}
				}

				if (!serialPort->sendBytes("SC", 2) ||
					!serialPort->sendBytes(redBuf, bufSize) ||
					!serialPort->sendBytes(greenBuf, bufSize)) {
						break;
				}

				count = (count + 1) % 3;
			}

			serialPort->closeSerialPort();
		} else {
			wait(1000);
		}
	}

	delete[] redBuf;
	delete[] greenBuf;
	delete[] localBitmap;
}