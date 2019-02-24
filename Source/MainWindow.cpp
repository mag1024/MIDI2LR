/*
  ==============================================================================

    MainWindow.cpp

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

#include "MainWindow.h"
#include <utility>
#include "MainComponent.h"
#include "SettingsManager.h"

MainWindow::MainWindow(const juce::String& name, CommandMap& command_map,
    ProfileManager& profile_manager, SettingsManager& settings_manager,
    std::weak_ptr<LrIpcOut>&& lr_ipc_out, std::shared_ptr<MidiProcessor> midi_processor,
    std::shared_ptr<MidiSender> midi_sender)
    : juce::DocumentWindow{name, juce::Colours::lightgrey,
          juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton}
{
   juce::TopLevelWindow::setUsingNativeTitleBar(true);
#pragma warning(suppress : 26409 24623)
   window_content_ = new MainContentComponent(command_map, profile_manager, settings_manager);
   juce::ResizableWindow::setContentOwned(window_content_, true);
   juce::Component::centreWithSize(getWidth(), getHeight());
   juce::Component::setVisible(true);
   window_content_->Init(std::move(lr_ipc_out), std::move(midi_processor), std::move(midi_sender));
}