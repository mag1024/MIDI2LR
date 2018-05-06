#pragma once
/*
  ==============================================================================

    MIDIProcessor.h

This file is part of MIDI2LR. Copyright 2015 by Rory Jaffe.

MIDI2LR is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

MIDI2LR is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
MIDI2LR.  If not, see <http://www.gnu.org/licenses/>.
  ==============================================================================
*/
#ifndef MIDI2LR_MIDIPROCESSOR_H_INCLUDED
#define MIDI2LR_MIDIPROCESSOR_H_INCLUDED
#include <functional>
#include <future>
#include <vector>
#include "MoodyCamel/blockingconcurrentqueue.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiUtilities.h"
#include "NrpnMessage.h"

class MidiProcessor final: juce::MidiInputCallback {
public:
    MidiProcessor() = default;
    ~MidiProcessor();
    MidiProcessor(const MidiProcessor& other) = delete;
    MidiProcessor(MidiProcessor&& other) = delete;
    MidiProcessor& operator=(const MidiProcessor& other) = delete;
    MidiProcessor& operator=(MidiProcessor&& other) = delete;
    void Init();
    // re-enumerates MIDI IN devices
    void RescanDevices();

    template <class T> void AddCallback(T* const object, void (T::* const mf)(rsj::MidiMessage))
    {
        callbacks_.emplace_back(std::bind(mf, object, std::placeholders::_1));
    }

private:
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;
    void DispatchMessages();
    void InitDevices();

    moodycamel::BlockingConcurrentQueue<rsj::MidiMessage> messages_;
    NrpnFilter nrpn_filter_;
    std::future<void> dispatch_messages_future_;
    std::vector <std::function <void(rsj::MidiMessage)>> callbacks_;
    std::vector <std::unique_ptr<juce::MidiInput>> devices_;
};

#endif  // MIDIPROCESSOR_H_INCLUDED
