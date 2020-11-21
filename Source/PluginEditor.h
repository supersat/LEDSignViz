/*
==============================================================================

LEDSignViz -- A VST/AU plugin for visualizing music on supported LED signs.

PluginEditor.h: Configures the plugin and provides a visualization preview.

Copyright (C) 2012-2020  Karl Koscher

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

#ifndef __PLUGINEDITOR_H_5D105E7__
#define __PLUGINEDITOR_H_5D105E7__

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class LedsignVizAudioProcessorEditor  : public AudioProcessorEditor,
										public Timer,
                                        public Slider::Listener
{
public:
    LedsignVizAudioProcessorEditor (LedsignVizAudioProcessor* ownerFilter);
    ~LedsignVizAudioProcessorEditor();

    //==============================================================================
    // This is just a standard Juce paint method...
    void paint (Graphics& g);
	void timerCallback();
	void sliderValueChanged (Slider*);
private:
	int scale;
	StringPairArray ports;

	TextButton setupButton;
	ComboBox portSelector;
	ComboBox signSelector;

	Slider vizSelector;
	Slider imageSelector;
	Slider imgOpSelector;

	unsigned int *localBitmap;
};

#endif  // __PLUGINEDITOR_H_5D105E7__
