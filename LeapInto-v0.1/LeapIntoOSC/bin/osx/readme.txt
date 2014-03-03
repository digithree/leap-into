-===- LeapIntoOSC -===-


-= About =-

LeapInto provides a simplified interface to the Leap Motion. The Leap Motion is a motion sensing peripheral specially designed to sense hands, fingers and thin tools (such a pencil or conducting baton). A number of hand sorting criteria are built in to give the user flexibility. Multiple hand recognition is simplified to several stable categories and coordinates are normalised. The interface comes two flavours at present, an open broadcast system using the OSC protocol and a plugin for the Csound audio/music programming language. This is the OSC favour.


-= Quickstart =-

The Leap Motion SDK agreement allows the distribution of the library binary to non-developers, which is great because it means that I can provide the LeapIntoOSC built binary to you. In other words, you can just run the LeapIntoOSC standalone program which will spit out the OSC data for you to use however you like.

There is a test program included, LeapOSCReceieveTest, which will show you data being sent by LeapIntoOSC.

Note: at present I have only built on Mac OS X v10.9.2 (latest) but will build on other systems in (hopefully) a matter of days.


-Build instructions-
On any system you’ll have to have the Leap SDK downloaded and the oscpack library downloaded and built and installed (refer to build oscpack instructions to do that).

Mac OS X or Linux:
g++ LeapIntoOSC.cpp -o LeapIntoOSC -I /path/to/LeapSDK/include/ -L /path/to/LeapSDK/lib -loscpack -lLeap

Windows:
NOT TESTED YET



-= Usage =-

The are four distinctions LeapInto makes concerning hand sorting:

	TYPE		string
	-=-=-		-=-=-=-
 * newest hand		“handnew”
 * oldest hand		“handold”
 * hand A		“handa”
 * hand B		“handb”

Newest/oldest hand are exactly what they sound like, the hands which are around for the shortest and longest times respectively. If you only want to deal with one hand, oldest hand is definitely the most stable one to use. If there is only one hand visible to the sensor, newest/oldest will be the same hand. While you can use newest/oldest hand for two handed systems, the users hands will switch between classifications if the oldest hand disappears from view, even for a split second.

Hand A/B are specifically for systems which use two hands. There is a sorting algorithm built into the system which does its best to keep track of hands, when visibility becomes difficult (if at the edge of field of view for example) and even if the hand is completely removed. This means you can support intentional hand removal.

The four hand types all have the following data available. The first string in quotes is the type name and the second string is the OSC type string.

	TYPE			string		OSC type	comment
	-=-=-			-=-=-=-		-=-=-=-=-	-=-=-=-
 * position			“position”	“fff”
 * number of fingers		“numfingers”	“i”
 * velocity (magnitude)		“velocitymag”	“f”
 * velocity (vector)		“velocity”	“fff”
 * orientation			“orientation”	“fff”		NOT IMPLEMENTED YET!
 * time alive			“timealive”	“f”		in seconds
 * is active?			“isactive”	“i”		1 = true, 0 = false

The OSC strings for the four hand types the format is “/[hand]-[data]:” where hand is the [hand] category and [data] is the data type (see the quotes above). Don’t forget the forward slash at the start, the dash between types and the colon at the end!

For example, the position of the oldest hand is given by the string “/handold-position:”

In addition there is a small set of data available using both hand A and B. So far only the distance is available but I intend to have rotational data too. The Hand A/B distance OSC string is “/handaandb-dist:” with type “f”.




- = - = - = - = -
3rd March 2014
Simon Kenny



CHANGE LOG


02.03.14
- - - - -
First commit

03.03.14
- - - - -
* fixed typos
* included Leap.dylib and OS X 10.9 binaries
* added usage doc section