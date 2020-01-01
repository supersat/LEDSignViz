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

#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/IOBSD.h>

StringPairArray MacOSXSerialPortManager::getSerialPorts()
{
    StringPairArray ports;
    mach_port_t masterPort;
    CFMutableDictionaryRef classesToMatch;
    io_iterator_t serialPortIterator;
    io_object_t portService;

    if (IOMasterPort(MACH_PORT_NULL, &masterPort) != KERN_SUCCESS) {
        return ports;
    }

    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (!classesToMatch) {
        return ports;
    }

    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));

    if (IOServiceGetMatchingServices(masterPort, classesToMatch, &serialPortIterator) != KERN_SUCCESS) {
        return ports;
    }

    while ((portService = IOIteratorNext(serialPortIterator))) {
        CFTypeRef devPathCFString =
            IORegistryEntryCreateCFProperty(portService, CFSTR(kIOTTYDeviceKey), kCFAllocatorDefault, 0);
        const char* path = CFStringGetCStringPtr(static_cast<CFStringRef>(devPathCFString), kCFStringEncodingASCII);
        ports.set(String(path), String(path));
        CFRelease(devPathCFString);
        IOObjectRelease(portService);
    }

    IOObjectRelease(serialPortIterator);

    return ports;
}

PlatformSerialPort* MacOSXSerialPortManager::createAndOpenSerialPort(const String& identifier)
{
	PlatformSerialPort* port = new MacOSXSerialPort();
	port->openSerialPort(identifier);
	return port;
}

#endif // defined(__APPLE__) && defined(__MACH__)
