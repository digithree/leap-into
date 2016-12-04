# Leap Into

LeapInto provides a simplified interface to the Leap Motion hand sensor input device. Multiple hand recognition is simplified to several stable categories and coordinates are normalised. The interface comes two flavours at present, an open broadcast system using the OSC protocol and a plugin for the Csound audio/music programming language.

## Leap Into OSC

Provides OSC broadcast of LeapInto formatted data

## Leap Into Csound

Provides two opcodes for Csound, leaphand and leapgesture. For more information see the docs WIP man pages

# Changelog
### Update 04.12.16
* Updated LeapIntoCsound with latest Csound 6.08 SDK and Leap SDK 2.3.1
* Add warning comment to examples, library binaries (e.g. dylib files on Mac OS X) must be in PATH of csound when running CSD files
### Update 02.01.15
* Added LeapIntoCsound library for Csound