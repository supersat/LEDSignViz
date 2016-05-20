/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

PluginProcessor.h: Visualizes incoming samples. The core of the plugin.

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

#ifndef __PLUGINPROCESSOR_H_2944184C__
#define __PLUGINPROCESSOR_H_2944184C__

#include "../JuceLibraryCode/JuceHeader.h"
#include "SpectrumFFT.h"
#include "SerialThread.h"
#include <fftw3.h>
#include <vector>
#include <WinSock2.h>

//==============================================================================
/**
*/

enum ImageOp {
	NONE,
	OVERLAY_WITH_ALPHA,
	MULTIPLY,
	MULTIPLY_INVERSE,

	NUM_IMAGE_OPS,
};

enum VizType {
	SPECTRUM,
	HORIZ_SPECTROGRAM,
	VERT_SPECTROGRAM,
	STEREO_WAVEFORM,
	STEREO_VU,

	NUM_VIZ_TYPES,
};

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
	unsigned int *bitmap;
	unsigned int *preImageBitmap;
	CriticalSection bitmapLock;
	int signWidth;
	int signHeight;

	// These are set from the editor
	float smoothingFactor;
	float gamma;
	float powerInterval;

	VizType curVizType;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LedsignVizAudioProcessor);

	void loadImage(File f);
	void applyImage(unsigned int *localBitmap);

	void fftToBins(AudioSampleBuffer& buffer, float* bandPower, int nBins);

	void spectrumViz(AudioSampleBuffer& buffer, unsigned int *localBitmap);
	void horizontalSpectrogramViz(AudioSampleBuffer& buffer, unsigned int *localBitmap);
	void verticalSpectrogramViz(AudioSampleBuffer& buffer, unsigned int *localBitmap);
	void stereoWaveform(AudioSampleBuffer& buffer, unsigned int *localBitmap);
	void stereoVU(AudioSampleBuffer& buffer, unsigned int *localBitmap);

	// SpectrumFFT fft;
	int currentProgram;

	fftwf_plan fftPlan;
	fftwf_complex *fftIn, *fftOut;
	float *windowFunction;
	float *fftRollingAvg;
	int samplesPerBlock;

	StringArray imageNames;
	std::vector<Image> images;
	ImageOp curImageOp;
	Image *curImage;

	LedsignVizSerialThread serialThread;

	float *fftRollingAvgL1;
	int ledSocket;
	struct sockaddr_in ledDest;
};

#endif  // __PLUGINPROCESSOR_H_2944184C__
