/*
  ==============================================================================

    SerialThread.h
    Created: 7 Dec 2012 3:56:00am
    Author:  supersat

  ==============================================================================
*/

#ifndef __SERIALTHREAD_H_66017D05__
#define __SERIALTHREAD_H_66017D05__

#include "../JuceLibraryCode/JuceHeader.h"

class LedsignVizAudioProcessor;

//==============================================================================
/**
*/
class LedsignVizSerialThread : public Thread
{
public:
    LedsignVizSerialThread (LedsignVizAudioProcessor* ownerFilter);
    virtual ~LedsignVizSerialThread();

	virtual void run();
private:
	LedsignVizAudioProcessor* processor;
};




#endif  // __SERIALTHREAD_H_66017D05__
