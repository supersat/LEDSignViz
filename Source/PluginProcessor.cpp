/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#define _USE_MATH_DEFINES
#include <math.h>

//==============================================================================
LedsignVizAudioProcessor::LedsignVizAudioProcessor() :
	signWidth(160), signHeight(16), smoothingFactor(0.1f), gamma(1/2.0f), powerInterval(-3.0/16), serialThread(this)
{
}

LedsignVizAudioProcessor::~LedsignVizAudioProcessor()
{
}

//==============================================================================
const String LedsignVizAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int LedsignVizAudioProcessor::getNumParameters()
{
    return 0;
}

float LedsignVizAudioProcessor::getParameter (int index)
{
    return 0.0f;
}

void LedsignVizAudioProcessor::setParameter (int index, float newValue)
{
}

const String LedsignVizAudioProcessor::getParameterName (int index)
{
    return String::empty;
}

const String LedsignVizAudioProcessor::getParameterText (int index)
{
    return String::empty;
}

const String LedsignVizAudioProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String LedsignVizAudioProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

bool LedsignVizAudioProcessor::isInputChannelStereoPair (int index) const
{
    return true;
}

bool LedsignVizAudioProcessor::isOutputChannelStereoPair (int index) const
{
    return true;
}

bool LedsignVizAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool LedsignVizAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

int LedsignVizAudioProcessor::getNumPrograms()
{
    return 0;
}

int LedsignVizAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LedsignVizAudioProcessor::setCurrentProgram (int index)
{
}

const String LedsignVizAudioProcessor::getProgramName (int index)
{
    return String::empty;
}

void LedsignVizAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void LedsignVizAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	fftRollingAvg = new float[samplesPerBlock / 2];

	memset(fftRollingAvg, 0, sizeof(float) * samplesPerBlock / 2);
	memset(bitmap, 0, sizeof(bitmap));

	fftIn = (fftwf_complex *)fftwf_malloc(sizeof(fftw_complex) * samplesPerBlock);
	fftOut = (fftwf_complex *)fftwf_malloc(sizeof(fftw_complex) * samplesPerBlock);
	fftPlan = fftwf_plan_dft_1d(samplesPerBlock, fftIn, fftOut, FFTW_FORWARD, FFTW_MEASURE);

	windowFunction = new float[samplesPerBlock];
	for (int i = 0; i < samplesPerBlock; i++) {
		// Hamming: windowFunction[i] = 0.54 - 0.46 * cos(2*M_PI*i / (nFFTSamples - 1));
		// Blackman–Harris:
		windowFunction[i] = 0.355768 - 0.48829 * cos(2*M_PI*i / (samplesPerBlock - 1)) +
			0.14128 * cos(4*M_PI*i / (samplesPerBlock - 1)) - 0.01168 * cos(6*M_PI*i / (samplesPerBlock - 1));
	}

	serialThread.startThread();
}

void LedsignVizAudioProcessor::releaseResources()
{
	serialThread.stopThread(1000);

	delete[] fftRollingAvg;
	delete[] windowFunction;
	fftwf_free(fftIn);
	fftwf_free(fftOut);
	// TODO: fftPlan?
}

void LedsignVizAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	memset(fftIn, 0, sizeof(fftw_complex) * buffer.getNumSamples());

	for (int channel = 0; channel < getNumInputChannels(); ++channel){
        float* channelData = buffer.getSampleData (channel);
		for (int i = 0; i < buffer.getNumSamples(); i++) {
			fftIn[i][0] += channelData[i];
		}
	}
	
	for (int i = 0; i < buffer.getNumSamples(); i++) {
		fftIn[i][0] = windowFunction[i] * (fftIn[i][0] / getNumInputChannels());
	}

	fftwf_execute(fftPlan);

	//float spectrum[MAX_FFT_SAMPLES / 2];
	//fft.getSpectrum(monoSamples, 0, nSamples, spectrum, nSamples, 0, 1, SpectrumFFT::FFT_WINDOW_BLACKMANHARRIS);

	float bandPower[MAX_SIGN_WIDTH];
	memset(bandPower, 0, sizeof(bandPower));

	for (int i = 0; i < buffer.getNumSamples() / 2; i++) {
		float specPower = sqrt(fftOut[i][0] * fftOut[i][0] + fftOut[i][1] * fftOut[i][1]);
		fftRollingAvg[i] = fftRollingAvg[i] * smoothingFactor + specPower * (1 - smoothingFactor);
		// This is pseudo-logarithmic. See http://dlbeer.co.nz/articles/fftvis.html
		int band = (int)(powf((float)i / (buffer.getNumSamples() / 2), gamma) * signWidth);
		bandPower[band] += fftRollingAvg[i];
	}

	char localBitmap[MAX_SIGN_HEIGHT][MAX_SIGN_WIDTH];
	memset(localBitmap, 0, sizeof(localBitmap));

	for (int x = 0; x < signWidth; x++) {
		float freqPower = log10(bandPower[x] / 90);
		for (int y = 0; y < signHeight; y++) {
			if (freqPower > powerInterval * (y + 1)) {
				unsigned char red = ((y < (signHeight / 2)) ? 0xf : ((((signHeight / 2) - (y - (signHeight / 2))) * 0xf) / (signHeight / 2)));
				unsigned char green = (y >= (signHeight / 2)) ? 0xf : ((y * 0xf) / (signHeight / 2));
				localBitmap[y][x] = (red << 4) | green;
			}
		}
	}

	bitmapLock.enter();
	memcpy(bitmap, localBitmap, sizeof(bitmap));
	bitmapLock.exit();

    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

//==============================================================================
bool LedsignVizAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* LedsignVizAudioProcessor::createEditor()
{
    return new LedsignVizAudioProcessorEditor (this);
}

//==============================================================================
void LedsignVizAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LedsignVizAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LedsignVizAudioProcessor();
}
