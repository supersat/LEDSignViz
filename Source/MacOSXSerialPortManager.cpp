/*
  ==============================================================================

    MacOSXSerialPortManager.cpp
    Created: 24 Dec 2012 11:26:20am
    Author:  supersat

  ==============================================================================
*/

#if defined(__APPLE__) && defined(__MACH__)

#include "MacOSXSerialPortManager.h"
#include "MacOSXSerialPort.h"

StringPairArray MacOSXSerialPortManager::getSerialPorts()
{
	return StringPairArray();
}

PlatformSerialPort* MacOSXSerialPortManager::createAndOpenSerialPort(const String& identifier)
{
	PlatformSerialPort* port = new MacOSXSerialPort();
	port->openSerialPort(identifier);
	return port;
}

#endif // defined(__APPLE__) && defined(__MACH__)