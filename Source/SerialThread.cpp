/*
==============================================================================

SerialThread.cpp
Created: 7 Dec 2012 3:56:00am
Author:  supersat

==============================================================================
*/

#include "SerialThread.h"
#include "PluginProcessor.h"
#include <Windows.h>

LedsignVizSerialThread::LedsignVizSerialThread(LedsignVizAudioProcessor* owner) : Thread("Serial Port Thread"), processor(owner)
{
}

LedsignVizSerialThread::~LedsignVizSerialThread() {
}

#define SIGN_HEIGHT 16
#define SIGN_WIDTH 160

void LedsignVizSerialThread::run() {
	HANDLE hComPort;
	DCB dcb;
	DWORD bytesWritten;

	char localBitmap[MAX_SIGN_HEIGHT][MAX_SIGN_WIDTH];

	int bufSize = SIGN_HEIGHT * SIGN_WIDTH / 8;
	char* redBuf = new char[bufSize];
	char* greenBuf = new char[bufSize];
	unsigned int count;

	while (!threadShouldExit()) {
		hComPort = CreateFile(TEXT("\\\\.\\COM15"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hComPort != INVALID_HANDLE_VALUE) {
			dcb.DCBlength = sizeof(DCB);
			GetCommState(hComPort, &dcb);
			dcb.BaudRate = 2000000;
			dcb.ByteSize = 8;
			dcb.fBinary = TRUE;
			dcb.fParity = FALSE;
			dcb.StopBits = ONESTOPBIT;
			dcb.Parity = NOPARITY;
			dcb.fOutxDsrFlow = TRUE;
			SetCommState(hComPort, &dcb);

			while (!threadShouldExit()) {
				processor->bitmapLock.enter();
				memcpy(localBitmap, processor->bitmap, sizeof(localBitmap));
				processor->bitmapLock.exit();

				memset(redBuf, 0, bufSize);
				memset(greenBuf, 0, bufSize);

				for (int y = 0; y < processor->signHeight; y++) {
					for (int x = 0; x < processor->signWidth; x++) {
						unsigned char color = localBitmap[processor->signHeight - y - 1][x];
						/*
						if (localBitmap[processor->signHeight - y - 1][x]) {
							if (y < (int)(processor->signHeight * .75))
								greenBuf[(processor->signWidth / 8) * y + (x / 8)] |= 1 << (7 - (x % 8));
							if (y >= (int)(processor->signHeight * .50))
								redBuf[(processor->signWidth / 8) * y + (x / 8)] |= 1 << (7 - (x % 8));
						}
						*/
						if ((color & 0xc0) > (count << 6))
							redBuf[(processor->signWidth / 8) * y + (x / 8)] |= 1 << (7 - (x % 8));
						if ((color & 0xc) > (count << 2))
							greenBuf[(processor->signWidth / 8) * y + (x / 8)] |= 1 << (7 - (x % 8));
					}
				}

				if (!WriteFile(hComPort, "SC", 2, &bytesWritten, NULL) ||
					!WriteFile(hComPort, redBuf, bufSize, &bytesWritten, NULL) ||
					!WriteFile(hComPort, greenBuf, bufSize, &bytesWritten, NULL)) {
						break;
				}

				count = (count + 1) % 3;
			}

			CloseHandle(hComPort);
		} else {
			Sleep(1000);
		}
	}

	delete[] redBuf;
	delete[] greenBuf;
}