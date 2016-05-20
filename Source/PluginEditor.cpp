/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

PluginEditor.cpp: Configures the plugin and provides a visualization preview.

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
#include "PlatformSerialPortManager.h"

//==============================================================================
LedsignVizAudioProcessorEditor::LedsignVizAudioProcessorEditor (LedsignVizAudioProcessor* ownerFilter)
    : AudioProcessorEditor (ownerFilter), scale(8),
	  setupButton("Setup", "Configure the sign and communications port used"),
	  portSelector("Port"),
	  signSelector("Sign"),
	  vizSelector("Viz"),
	  imageSelector("Image"),
	  imgOpSelector("Image Op")
{
	localBitmap = new unsigned int[ownerFilter->signWidth * ownerFilter->signHeight];
    
	addAndMakeVisible(&setupButton);
	addAndMakeVisible(&portSelector);
	//addAndMakeVisible(&signSelector);
	addAndMakeVisible(&vizSelector);
	addAndMakeVisible(&imageSelector);
	addAndMakeVisible(&imgOpSelector);

	setupButton.setBounds(0, 4, 50, 16);
	portSelector.setBounds(58, 4, 120, 16);
	//signSelector.setBounds(184, 4, 120, 16);
	vizSelector.setBounds(184, 4, 16, 16);
	imageSelector.setBounds(208, 4, 16, 16);
	imgOpSelector.setBounds(232, 4, 16, 16);

	setSize (scale * ownerFilter->signWidth, 24 + scale * ownerFilter->signHeight);
	startTimer(10);

	ports = PlatformSerialPortManager::getSerialPortManager().getSerialPorts();
	StringArray portNames = ports.getAllKeys();
	for (int i = 0; i < portNames.size(); i++) {
		portSelector.addItem(ports[portNames[i]] + "(" + portNames[i] + ")", i + 1);
	}

	signSelector.addItem("Red/Green 160x16", 1);
	signSelector.addItem("Red/Green 200x16", 2);
	signSelector.addItem("Red 90x7", 3);

	portSelector.setTextWhenNoChoicesAvailable(String("No serial ports available"));
	portSelector.setTextWhenNothingSelected(String("Select a serial port"));
	signSelector.setTextWhenNothingSelected(String("Select a sign"));

	vizSelector.setSliderStyle (Slider::Rotary);
    vizSelector.addListener (this);
    vizSelector.setRange (0.0, 1.0, 0.01);

	imageSelector.setSliderStyle (Slider::Rotary);
    imageSelector.addListener (this);
    imageSelector.setRange (0.0, 1.0, 0.01);

	imgOpSelector.setSliderStyle (Slider::Rotary);
    imgOpSelector.addListener (this);
    imgOpSelector.setRange (0.0, 1.0, 0.01);
}

LedsignVizAudioProcessorEditor::~LedsignVizAudioProcessorEditor()
{
	delete[] localBitmap;
}


//==============================================================================
void LedsignVizAudioProcessorEditor::paint (Graphics& g)
{
	g.setGradientFill (ColourGradient (Colours::white, 0, 0, Colours::grey, 0, 24, false));
	g.fillRect(0, 0, getWidth(), 24);
	
	LedsignVizAudioProcessor* processor = (LedsignVizAudioProcessor*) getAudioProcessor();
	processor->bitmapLock.enter();
	memcpy(localBitmap, processor->bitmap, processor->signWidth * processor->signHeight * sizeof(unsigned int));
	processor->bitmapLock.exit();
	Image img(Image::RGB, processor->signWidth * scale, processor->signHeight * scale, false);

	for (int y = 0; y < processor->signHeight; y++) {
		for (int x = 0; x < processor->signWidth; x++) {
			unsigned char red = (localBitmap[y * processor->signWidth + x] & 0xff);
			unsigned char green = (localBitmap[y * processor->signWidth + x] & 0xff00) >> 8;
			Colour c(red, green, 0);
			for (int yo = 0; yo < scale; yo++) {
				for (int xo = 0; xo < scale; xo++) {
					img.setPixelAt(x * scale + xo, y * scale + yo, c);
				}
			}
		}
	}

	g.drawImageAt(img, 0, 24);
}

void LedsignVizAudioProcessorEditor::timerCallback() {
	repaint(0, 24, getWidth(), getHeight() - 24);
}

void LedsignVizAudioProcessorEditor::sliderValueChanged(Slider* slider)
{
	LedsignVizAudioProcessor* processor = (LedsignVizAudioProcessor*) getAudioProcessor();
	if (slider == &vizSelector) {
        processor->setParameterNotifyingHost (0, (float)vizSelector.getValue());
    }
    else if (slider == &imageSelector)
    {
        processor->setParameterNotifyingHost (1, (float)imageSelector.getValue());
    } else if (slider == &imgOpSelector) {
		processor->setParameterNotifyingHost (2, (float)imgOpSelector.getValue());
	}
}
