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


const String vizNames[] = {
	"Spectrum",
	"Horiz Spectrogram",
	"Vert Spectrogram",
	//"3D Spectrogram",
	//"Waveform",
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
	signWidth(200), signHeight(16), smoothingFactor(0.1f), gamma(2.0f),
	powerInterval(-3.0/16), serialThread(this), currentProgram(0),
	curImage(0), curImageOp(NONE), curVizType(SPECTRUM)
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
    return 3;
}

float LedsignVizAudioProcessor::getParameter (int index)
{
	switch (index) {
	case 0:
		return (float)curVizType / (float)VizType::NUM_VIZ_TYPES;
	case 1:
		if (curImage == imageCountdown)
			return 0.0f;

		for (int i = 0; i < images.size(); i++) {
			if (curImage == &images.at(i)) {
				return ((float)i + 1) / (images.size() + 1);
			}
		}
		return 0.0f;
		break;
	case 2:
		return (float)curImageOp / (float)ImageOp::NUM_IMAGE_OPS;
	default:
		return 0.0f;
	}
}

void LedsignVizAudioProcessor::setParameter (int index, float newValue)
{
	int imgIdx;

	if (newValue == 1.0f)
		newValue = .99f;

	switch (index) {
	case 0:
		curVizType = (VizType)(int)(newValue * (float)VizType::NUM_VIZ_TYPES);
		break;
	case 1:
		imgIdx = newValue * (images.size() + 1);
		if (imgIdx == 0)
			curImage = imageCountdown;
		else
			curImage = &images.at(imgIdx);
		break;
	case 2:
		curImageOp = (ImageOp)(int)(newValue * (float)ImageOp::NUM_IMAGE_OPS);
		break;
	}
}

const String LedsignVizAudioProcessor::getParameterName (int index)
{
	switch (index) {
	case 0:
		return String("Viz Type");
	case 1:
		return String("Image");
	case 2:
		return String("Image Op");
	default:
		return String::empty;
	}
}

const String LedsignVizAudioProcessor::getParameterText (int index)
{
	switch (index) {
	case 0:
		return String(vizNames[curVizType]);
	case 1:
		if (curImage == imageCountdown)
			return String("Countdown");

		for (int i = 0; i < images.size(); i++) {
			if (curImage == &images.at(i)) {
				return String(((float)i + 1) / (images.size() + 1));
			}
		}
		return String("Unknown");
	case 2:
		return String(imageOpNames[curImageOp]);
		break;
	default:
		return String::empty;
	}
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

bool LedsignVizAudioProcessor::silenceInProducesSilenceOut() const
{
    return true;
}

double LedsignVizAudioProcessor::getTailLengthSeconds() const
{
    return 0;
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
    return 1;
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
	return String::empty;
}

void LedsignVizAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void LedsignVizAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	//fftRollingAvg = new float[samplesPerBlock / 2];
	fftRollingAvg = new float[signWidth]; // This is now done per-bin
	bitmap = new unsigned int[signWidth * signHeight];
	preImageBitmap = new unsigned int[signWidth * signHeight];

	//memset(fftRollingAvg, 0, sizeof(float) * samplesPerBlock / 2);
	memset(fftRollingAvg, 0, sizeof(float) * signWidth);
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
	for (int i = 0; i < 15; i++) {
		char filename[128];
		sprintf(filename, "ledsignviz-%02d.png", i);
		loadImage(File::getSpecialLocation(File::SpecialLocationType::userHomeDirectory).getChildFile(filename));
	}

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

	switch (curVizType) {
	case VizType::SPECTRUM:
		spectrumViz(buffer, localBitmap);
		break;
	case VizType::HORIZ_SPECTROGRAM:
		horizontalSpectrogramViz(buffer, localBitmap);
		break;
	case VizType::VERT_SPECTROGRAM:
		verticalSpectrogramViz(buffer, localBitmap);
		break;
	case VizType::STEREO_WAVEFORM:
		stereoWaveform(buffer, localBitmap);
		break;
	case VizType::STEREO_VU:
		stereoVU(buffer, localBitmap);
		break;
	}
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

void LedsignVizAudioProcessor::fftToBins(AudioSampleBuffer& buffer, float* bandPower, int nBins)
{
	memset(fftIn, 0, sizeof(fftw_complex) * buffer.getNumSamples());

	for (int channel = 0; channel < getNumInputChannels(); ++channel){
        const float* channelData = buffer.getReadPointer (channel);
		for (int i = 0; i < buffer.getNumSamples(); i++) {
			fftIn[i][0] += channelData[i];
		}
	}
	
	for (int i = 0; i < buffer.getNumSamples(); i++) {
		fftIn[i][0] = windowFunction[i] * (fftIn[i][0] / getNumInputChannels());
	}

	fftwf_execute(fftPlan);

	memset(bandPower, 0, sizeof(float) * nBins);

	// Adapted from DLBFFT, under the LGPL
	int freqs = buffer.getNumSamples() / 2;
	int f_start = 0;

	for (int i = 0; i < nBins; i++) {
		int f_end = floor((powf(((float)(i + 1)) /
			(float)nBins, gamma) * freqs) + 0.5);
		int f_width;
		int j;
		float bin_power = 0.0f;

		if (f_end > freqs)
			f_end = freqs;

		f_width = f_end - f_start;
		if (f_width <= 0)
			f_width = 1;

		for (j = 0; j < f_width; j++) {
			float p = sqrt(fftOut[f_start + j][0] * fftOut[f_start + j][0] + fftOut[f_start + j][1] * fftOut[f_start + j][1]);

			//if (p > bin_power)
				bin_power += p;
		}

		//bin_power = log(bin_power);
		//if (bin_power < 0.0f)
		//	bin_power = 0.0f;

		bandPower[i] = fftRollingAvg[i] = fftRollingAvg[i] * smoothingFactor +
			(bin_power /* 0.05 */ * (1.0f - smoothingFactor));
		//bandPower[i] = bin_power * 0.05;

		f_start = f_end;
	}
	/*
	for (int i = 0; i < buffer.getNumSamples() / 2; i++) {
		float specPower = sqrt(fftOut[i][0] * fftOut[i][0] + fftOut[i][1] * fftOut[i][1]);
		fftRollingAvg[i] = fftRollingAvg[i] * smoothingFactor + specPower * (1 - smoothingFactor);
		// This is pseudo-logarithmic. See http://dlbeer.co.nz/articles/fftvis.html
		int band = (int)(powf((float)i / (buffer.getNumSamples() / 2), gamma) * signWidth);
		bandPower[band] += fftRollingAvg[i];
	}
	*/

}

void LedsignVizAudioProcessor::spectrumViz(AudioSampleBuffer& buffer, unsigned int *localBitmap)
{
	float* bandPower = new float[signWidth];
	fftToBins(buffer, bandPower, signWidth);

	//float spectrum[MAX_FFT_SAMPLES / 2];
	//fft.getSpectrum(monoSamples, 0, nSamples, spectrum, nSamples, 0, 1, SpectrumFFT::FFT_WINDOW_BLACKMANHARRIS);

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

	delete[] bandPower;
}

void LedsignVizAudioProcessor::horizontalSpectrogramViz(AudioSampleBuffer& buffer, unsigned int *localBitmap)
{
	float* bandPower = new float[signWidth];
	fftToBins(buffer, bandPower, signWidth);

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
	float* bandPower = new float[signWidth];
	fftToBins(buffer, bandPower, signWidth);

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
        const float* channelData = buffer.getReadPointer (channel);
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
        const float* channelData = buffer.getReadPointer (channel);
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

		g.setColour(Colour(255, 255, 255));
		g.drawText(buf, 0, 0, signWidth, signHeight, Justification::centred, false);
	} else {
		srcImg = curImage;
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

void LedsignVizAudioProcessor::loadImage(File f)
{
	if (f.exists()) {
		images.push_back(ImageFileFormat::loadFrom(f));
	}
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LedsignVizAudioProcessor();
}
