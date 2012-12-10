/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#ifndef __PLUGINPROCESSOR_H_2944184C__
#define __PLUGINPROCESSOR_H_2944184C__

#include "../JuceLibraryCode/JuceHeader.h"
#include "SpectrumFFT.h"
#include "SerialThread.h"
#include <fftw3.h>

#define MAX_SIGN_WIDTH  200
#define MAX_SIGN_HEIGHT 24

//==============================================================================
/**
*/
class LedsignVizAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    LedsignVizAudioProcessor();
    ~LedsignVizAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

    //==============================================================================
    AudioProcessorEditor* createEditor();
    bool hasEditor() const;

    //==============================================================================
    const String getName() const;

    int getNumParameters();

    float getParameter (int index);
    void setParameter (int index, float newValue);

    const String getParameterName (int index);
    const String getParameterText (int index);

    const String getInputChannelName (int channelIndex) const;
    const String getOutputChannelName (int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;

    bool acceptsMidi() const;
    bool producesMidi() const;

    //==============================================================================
    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int index);
    const String getProgramName (int index);
    void changeProgramName (int index, const String& newName);

    //==============================================================================
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

	//==============================================================================
	char bitmap[MAX_SIGN_HEIGHT][MAX_SIGN_WIDTH];
	CriticalSection bitmapLock;

	SpectrumFFT fft;
	int signWidth;
	int signHeight;
	int samplesPerBlock;
	float smoothingFactor;
	float *fftRollingAvg;
	float gamma;
	float powerInterval;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LedsignVizAudioProcessor);

	fftwf_plan fftPlan;
	fftwf_complex *fftIn, *fftOut;
	float *windowFunction;
	LedsignVizSerialThread serialThread;
};

#endif  // __PLUGINPROCESSOR_H_2944184C__
