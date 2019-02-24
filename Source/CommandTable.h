#pragma once
/*
  ==============================================================================

  CommandTable.h

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
#ifndef MIDI2LR_COMMANDTABLE_H_INCLUDED
#define MIDI2LR_COMMANDTABLE_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
class CommandTableModel;

class CommandTable final : public juce::TableListBox {
 public:
   CommandTable(const juce::String& component_name, CommandTableModel* model);
   ~CommandTable() = default;
   CommandTable(const CommandTable& other) = delete;
   CommandTable(CommandTable&& other) = delete;
   CommandTable& operator=(const CommandTable& other) = delete;
   CommandTable& operator=(CommandTable&& other) = delete;

 private:
   bool keyPressed(const juce::KeyPress& k) override;
};

#endif
