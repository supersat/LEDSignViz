/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

PluginProcessor.cpp: Visualizes incoming samples. The core of the plugin.

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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <algorithm>
using namespace std;

const String programNames[] = {
	"Spectrum",
	"Spectrogram",
	"3D Spectrogram",
	"Waveform",
	"Stereo Waveform",
	"Stereo VU",
};

const String imageOpNames[] = {
	"None",
	"Overlay",
	"Multiply",
	"Multiply Inverse"
};

Image* imageCountdown = (Image*) 0xcdcd0101;

//==============================================================================
LedsignVizAudioProcessor::LedsignVizAudioProcessor() :
	signWidth(200), signHeight(16), smoothingFactor(0.1f), gamma(1/2.0f),
	powerInterval(-3.0/16), serialThread(this), currentProgram(0),
	curImage(0), curImageOp(NONE)
{
	curImageOp = OVERLAY_WITH_ALPHA;
	curImage = imageCountdown;
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
    return sizeof(programNames)/sizeof(programNames[0]);
}

int LedsignVizAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void LedsignVizAudioProcessor::setCurrentProgram (int index)
{
	currentProgram = index;
}

const String LedsignVizAudioProcessor::getProgramName (int index)
{
    return programNames[index];
}

void LedsignVizAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void LedsignVizAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	fftRollingAvg = new float[samplesPerBlock / 2];
	bitmap = new unsigned int[signWidth * signHeight];
	preImageBitmap = new unsigned int[signWidth * signHeight];

	memset(fftRollingAvg, 0, sizeof(float) * samplesPerBlock / 2);
	memset(bitmap, 0, signWidth * signHeight * sizeof(unsigned int));

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

	//loadImage(String("c:\\Users\\supersat\\Pictures\\dj peter henry 200 - 4.png"));
	//loadImage(String("c:\\Users\\supersat\\Pictures\\supersat2.png"));

	serialThread.startThread();
}

void LedsignVizAudioProcessor::releaseResources()
{
	serialThread.stopThread(1000);

	delete[] fftRollingAvg;
	delete[] windowFunction;
	fftwf_destroy_plan(fftPlan);
	fftwf_free(fftIn);
	fftwf_free(fftOut);
	delete[] preImageBitmap;
	delete[] bitmap;
}

void LedsignVizAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	unsigned int *localBitmap = new unsigned int[signWidth * signHeight];
	memset(localBitmap, 0, signWidth * signHeight * sizeof(unsigned int));

	spectrumViz(buffer, localBitmap);
	//horizontalSpectrogramViz(buffer, localBitmap);
	//verticalSpectrogramViz(buffer, localBitmap);
	//stereoWaveform(buffer, localBitmap);
	//stereoVU(buffer, localBitmap);
	applyImage(localBitmap);

	bitmapLock.enter();
	memcpy(bitmap, localBitmap, signWidth * signHeight * sizeof(unsigned int));
	bitmapLock.exit();
	delete[] localBitmap;

    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

void LedsignVizAudioProcessor::spectrumViz(AudioSampleBuffer& buffer, unsigned int *localBitmap)
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

	float bandPower[200]; // TODO(supersat): Remove constant
	memset(bandPower, 0, sizeof(bandPower));

	for (int i = 0; i < buffer.getNumSamples() / 2; i++) {
		float specPower = sqrt(fftOut[i][0] * fftOut[i][0] + fftOut[i][1] * fftOut[i][1]);
		fftRollingAvg[i] = fftRollingAvg[i] * smoothingFactor + specPower * (1 - smoothingFactor);
		// This is pseudo-logarithmic. See http://dlbeer.co.nz/articles/fftvis.html
		int band = (int)(powf((float)i / (buffer.getNumSamples() / 2), gamma) * signWidth);
		bandPower[band] += fftRollingAvg[i];
	}

	for (int x = 0; x < signWidth; x++) {
		float freqPower = log10(bandPower[x] / 90);
		for (int y = 0; y < signHeight; y++) {
			if (freqPower > powerInterval * (y + 1)) {
				unsigned char red = ((y < (signHeight / 2)) ? 0xff : ((((signHeight / 2) - (y - (signHeight / 2))) * 0xff) / (signHeight / 2)));
				unsigned char green = (y >= (signHeight / 2)) ? 0xff : ((y * 0xff) / (signHeight / 2));
				localBitmap[y * signWidth + x] = (green << 8) | red;
			}
		}
	}
}

void LedsignVizAudioProcessor::horizontalSpectrogramViz(AudioSampleBuffer& buffer, unsigned int *localBitmap)
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

	float bandPower[200]; // TODO(supersat): Remove constant
	memset(bandPower, 0, sizeof(bandPower));

	for (int i = 0; i < buffer.getNumSamples() / 2; i++) {
		float specPower = sqrt(fftOut[i][0] * fftOut[i][0] + fftOut[i][1] * fftOut[i][1]);
		// This is pseudo-logarithmic. See http://dlbeer.co.nz/articles/fftvis.html
		int band = (int)(powf((float)i / (buffer.getNumSamples() / 2), gamma) * signHeight);
		bandPower[band] += specPower;
	}

	if (curImageOp != NONE) {
		for (int y = 0; y < signHeight; y++) {
			for (int x = 0; x < signWidth - 1; x++) {
				localBitmap[y * signWidth + x] = preImageBitmap[y * signWidth + x + 1];
			}
		}
	} else {
		for (int y = 0; y < signHeight; y++) {
			for (int x = 0; x < signWidth - 1; x++) {
				localBitmap[y * signWidth + x] = bitmap[y * signWidth + x + 1];
			}
		}
	}

	for (int y = 0; y < signHeight; y++) {
		float freqPower = -log10(bandPower[y] / 500);
		if (freqPower > 3)
			freqPower = 3;
		if (freqPower < 0)
			freqPower = 0;
		freqPower /= 3;

		unsigned char red = ((freqPower < 0.5f) ? 0xff : (((0.5f - (freqPower - 0.5f)) * 0xff) / 0.5f));
		unsigned char green = (freqPower >= 0.5f) ? 0xff : ((freqPower * 0xff) / 0.5f);
		localBitmap[y * signWidth + signWidth - 1] = (green << 8) | red;
	}
}

void LedsignVizAudioProcessor::verticalSpectrogramViz(AudioSampleBuffer& buffer, unsigned int *localBitmap)
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

	float bandPower[200]; // TODO(supersat): Remove constant
	memset(bandPower, 0, sizeof(bandPower));

	for (int i = 0; i < buffer.getNumSamples() / 2; i++) {
		float specPower = sqrt(fftOut[i][0] * fftOut[i][0] + fftOut[i][1] * fftOut[i][1]);
		// This is pseudo-logarithmic. See http://dlbeer.co.nz/articles/fftvis.html
		int band = (int)(powf((float)i / (buffer.getNumSamples() / 2), gamma) * signWidth);
		bandPower[band] += specPower;
	}

	if (curImageOp != NONE) {
		for (int y = 0; y < signHeight - 1; y++) {
			for (int x = 0; x < signWidth; x++) {
				localBitmap[y * signWidth + x] = preImageBitmap[(y + 1) * signWidth + x];
			}
		}
	} else {
		for (int y = 0; y < signHeight - 1; y++) {
			for (int x = 0; x < signWidth; x++) {
				localBitmap[y * signWidth + x] = bitmap[(y + 1) * signWidth + x];
			}
		}
	}

	for (int x = 0; x < signWidth; x++) {
		float freqPower = -log10(bandPower[x] / 90);
		if (freqPower > 3)
			freqPower = 3;
		if (freqPower < 0)
			freqPower = 0;
		freqPower /= 3;

		unsigned char red = ((freqPower < 0.5f) ? 0xff : (((0.5f - (freqPower - 0.5f)) * 0xff) / 0.5f));
		unsigned char green = (freqPower >= 0.5f) ? 0xff : ((freqPower * 0xff) / 0.5f);
		localBitmap[(signHeight - 1) * signWidth + x] = (green << 8) | red;
	}
}

void LedsignVizAudioProcessor::stereoWaveform(AudioSampleBuffer& buffer, unsigned int *localBitmap)
{	
	memset(localBitmap, 0, signWidth * signHeight * sizeof(unsigned int));

	for (int channel = 0; channel < getNumInputChannels(); ++channel){
        float* channelData = buffer.getSampleData (channel);
		for (int i = 0; i < buffer.getNumSamples(); i++) {
			int y = channelData[i] * (signHeight / 2) + (signHeight / 2); // This is inverted but who cares
			int x = i * signWidth / buffer.getNumSamples();
			if (y > signHeight - 1)
				y = signHeight - 1;
			else if (y < 0)
				y = 0;
			localBitmap[y * signWidth + x] |= channel ? 0xff00 : 0xff;
		}
	}
	
}

void LedsignVizAudioProcessor::stereoVU(AudioSampleBuffer& buffer, unsigned int *localBitmap)
{
	memset(localBitmap, 0, signWidth * signHeight * sizeof(unsigned int));

	for (int channel = 0; channel < getNumInputChannels(); ++channel){
        float* channelData = buffer.getSampleData (channel);
		float power = 0;
		for (int i = 0; i < buffer.getNumSamples(); i++) {
			power += channelData[i] * channelData[i];
		}

		power = 20 * (log10(sqrt(power / buffer.getNumSamples())) + 0.1505f); // FIXME(supersat): Is this right?

		if (channel == 0) {
			for (int x = signWidth / 2 - 1; x >= 0; x--) {
				if (power > x * (-60.0f / (signWidth / 2))) {
					for (int y = 0; y < signHeight; y++) {
						unsigned red = (x < (signWidth / 4)) ? 0xff : (((signWidth / 2) - x) * 0xff / (signWidth / 4));
						unsigned green = (x >= (signWidth / 4)) ? 0xff : (x * 0xff / (signWidth / 4));
						localBitmap[y * signWidth + x] = (green << 8) | red;
					}
				}
			}
		} else {
			for (int x = signWidth / 2; x < signWidth; x++) {
				if (power > (signWidth - x) * (-60.0f / (signWidth / 2))) {
					for (int y = 0; y < signHeight; y++) {
						int cx = x - (signWidth / 2);
						unsigned green = (cx < (signWidth / 4)) ? 0xff : (((signWidth / 2) - cx) * 0xff / (signWidth / 4));
						unsigned red = (cx >= (signWidth / 4)) ? 0xff : (cx * 0xff / (signWidth / 4));
						localBitmap[y * signWidth + x] = (green << 8) | red;
					}
				}
			}
		}
	}
	
}

void LedsignVizAudioProcessor::applyImage(unsigned int *localBitmap)
{
	if (!curImage || curImageOp == NONE)
		return;

	memcpy(preImageBitmap, localBitmap, signWidth * signHeight * sizeof(unsigned int));

	Image *srcImg;
	
	if (curImage == imageCountdown) {
		srcImg = new Image(Image::ARGB, signWidth, signHeight, true);
		Graphics g(*srcImg);

		char buf[128], *cp;
		cp = buf;
		buf[0] = 0;
		time_t curTime;
		time(&curTime);
		curTime += 86000;
		struct tm *midnightTM = localtime(&curTime);
		midnightTM->tm_sec = 0;
		midnightTM->tm_min = 0;
		midnightTM->tm_hour = 0;

		int secsUntilMidnight = mktime(midnightTM) - time(NULL);

		if (secsUntilMidnight / 3600) {
			cp += sprintf(cp, "%d:", secsUntilMidnight / 3600);
			secsUntilMidnight %= 3600;
		}
		cp += sprintf(cp, "%02d:%02d", secsUntilMidnight / 60, secsUntilMidnight % 60);

		g.setColour(Colour(255, 0, 0));
		g.drawText(buf, 0, 0, signWidth, signHeight / 4, Justification::topRight, false);
	}

	for (int y = 0; y < srcImg->getHeight() && y < signHeight; y++) {
		for (int x = 0; x < srcImg->getWidth() && x < signWidth; x++) {
			unsigned int vizColor = localBitmap[y * signWidth + x];
			unsigned char red = vizColor & 0xff;
			unsigned char green = (vizColor & 0xff00) >> 8;
			unsigned char blue = (vizColor & 0xff0000) >> 16;
			Colour imgColor = srcImg->getPixelAt(x, y);

			switch (curImageOp) {
			case OVERLAY_WITH_ALPHA:
				red = red * (1.0f - imgColor.getFloatAlpha()) + imgColor.getRed() * imgColor.getFloatAlpha();
				green = green * (1.0f - imgColor.getFloatAlpha()) + imgColor.getGreen() * imgColor.getFloatAlpha();
				blue = blue * (1.0f - imgColor.getFloatAlpha()) + imgColor.getBlue() * imgColor.getFloatAlpha();
				break;
			case MULTIPLY:
				red = red * imgColor.getFloatRed();
				green = green * imgColor.getFloatGreen();
				blue = blue * imgColor.getBlue();
				break;
			case MULTIPLY_INVERSE:
				red = red * (1.0f - imgColor.getFloatRed());
				green = green * (1.0f - imgColor.getFloatGreen());
				blue = blue * (1.0f - imgColor.getBlue());
				break;
			}

			localBitmap[y * signWidth + x] = (blue << 16) | (green << 8) | red;
		}
	}

	if (curImage == imageCountdown)
		delete srcImg;
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

void LedsignVizAudioProcessor::loadImage(String filename)
{
	images.push_back(ImageFileFormat::loadFrom(File(filename)));
	curImage = &(images.at(1));
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LedsignVizAudioProcessor();
}
