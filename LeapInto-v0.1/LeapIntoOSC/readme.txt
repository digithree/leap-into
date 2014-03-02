LeapIntoOSC


-About-

LeapInto provides a simplified interface to the Leap Motion hand sensor input device. Multiple hand recognition is simplified to several stable categories and coordinates are normalised. The interface comes two flavours at present, an open broadcast system using the OSC protocol and a plugin for the Csound audio/music programming language.


-Quickstart-

The Leap Motion SDK agreement allows the distribution of the library binary to non-developers, which is great because it means that I can provide the LeapIntoOSC built binary to you. In other words, you can just run the LeapIntoOSC standalone program which will spit out the OSC data for you to use however you like.

Note: at present I have only built on Mac OS X v10.9.2 (latest Mavricks) but will build on other systems in (hopefully) a matter of days.


-Build instructions-
On any system you’ll have to have the Leap SDK downloaded and the oscpack library downloaded and built and installed (refer to build oscpack instructions to do that).

Mac OS X or Linux:
g++ LeapIntoOSC.cpp -o LeapIntoOSC -I /usr/local/include/oscpack/ -I /path/to/LeapSDK/include/ -L /usr/local/lib/ -L /path/to/LeapSDK/lib -loscpack -lLeap

Windows:
NOT TESTED YET


- = - = - = - = -
2nd March 2014
Simon Kenny