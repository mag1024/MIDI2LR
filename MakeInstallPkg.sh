#!/bin/bash
~/installbuilder/bin/builder build Installer/MIDI2LR.xml windows
~/installbuilder/bin/builder build Installer/MIDI2LR.xml osx
~/installbuilder/bin/builder build Installer/MIDI2LR.xml windows --setvars debug=true
~/installbuilder/bin/builder build Installer/MIDI2LR.xml osx --setvars debug=true
