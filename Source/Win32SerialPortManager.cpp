/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

Win32SerialPortManager.cpp: Win32-specific implementation of PlatformSerialPortManager.

This code is based on enumser by by PJ Naughter.

==============================================================================
*/

#ifdef _WIN32
#include "Win32SerialPortManager.h"
#include "Win32SerialPort.h"
#include <SetupAPI.h>
#include <Windows.h>

static const GUID comPortInterfaceGUID = { 0x86E0D1E0L, 0x8089, 0x11D0, { 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73 } };

Win32SerialPortManager::~Win32SerialPortManager()
{
}

StringPairArray Win32SerialPortManager::getSerialPorts()
{
	StringPairArray ports;
	int i = 0;

	HDEVINFO hDevInfoSet = SetupDiGetClassDevsW(&comPortInterfaceGUID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfoSet != INVALID_HANDLE_VALUE) {
		SP_DEVINFO_DATA devInfo;
		devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

		while (SetupDiEnumDeviceInfo(hDevInfoSet, i++, &devInfo)) {
			HKEY hDeviceKey = SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
			if (hDeviceKey != INVALID_HANDLE_VALUE) {
				WCHAR portName[MAX_PATH];
				LONG size = sizeof(portName);
				if (RegQueryValueW(hDeviceKey, L"PortName", portName, &size) == ERROR_SUCCESS) {
					WCHAR friendlyName[MAX_PATH];
					DWORD fnSize = sizeof(friendlyName);
					DWORD dwType = 0;
					SetupDiGetDeviceRegistryPropertyW(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType, (PBYTE)friendlyName, fnSize, &fnSize);
					ports.set(String(portName), String(friendlyName));
				}
				CloseHandle(hDeviceKey);
			}
		}
	}

	SetupDiDestroyDeviceInfoList(hDevInfoSet);

	return ports;
}

PlatformSerialPort* Win32SerialPortManager::createAndOpenSerialPort(const String& identifier)
{
	PlatformSerialPort *port = new Win32SerialPort();
	if (port->openSerialPort(identifier))
		return port;
	delete port;
	return 0;
}

#endif // _WIN32
