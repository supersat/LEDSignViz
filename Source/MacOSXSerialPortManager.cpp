/*
  ==============================================================================

    MacOSXSerialPortManager.cpp
    Created: 24 Dec 2012 11:26:20am
    Author:  supersat

  ==============================================================================
*/

#if defined(__APPLE__) && defined(__MACH__)

#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/IOBSD.h>

#include "MacOSXSerialPortManager.h"
#include "MacOSXSerialPort.h"

MacOSXSerialPortManager::~MacOSXSerialPortManager()
{
}

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
        String pathStr;
        const char *pathCStr;
        CFTypeRef devPathCFString =
            IORegistryEntryCreateCFProperty(portService, CFSTR(kIOTTYDeviceKey), kCFAllocatorDefault, 0);

        pathCStr = CFStringGetCStringPtr(static_cast<CFStringRef>(devPathCFString), kCFStringEncodingUTF8);
        if (!pathCStr) {
            pathCStr = CFStringGetCStringPtr(static_cast<CFStringRef>(devPathCFString), kCFStringEncodingASCII);
        }
        if (pathCStr) {
            pathStr = String(pathCStr);
        } else {
            CFIndex pathLen = CFStringGetLength(static_cast<CFStringRef>(devPathCFString)) + 1;
            pathCStr = (const char *)malloc(pathLen);
            CFStringGetCString(static_cast<CFStringRef>(devPathCFString),
                               (char *)pathCStr, pathLen, kCFStringEncodingUTF8);
            pathStr = String(pathCStr);
            free((void *)pathCStr);
        }
        ports.set(pathStr, pathStr);
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
