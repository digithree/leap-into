/* LeapIntoOSC
 *
 * by Simon Kenny
 * v0.1
 * 2nd March 2014
 */


#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/ip/UdpSocket.h"
#include "Leap.h"

#include <vector>
#include <iostream>


// LeapIntoOSC constants
#define MAX_HEIGHT                  800.f
#define MAX_DEPTH                   400.f
#define MAX_WIDTH                   400.f

#define MAX_VELOCITY                1500.f
#define MAX_SWIPE_VELOCITY          4500.f

#define DIST_START                  250.f
#define MAX_DIST                    1000.f

#define TIME_THRESHOLD              20000.f

#define HAND_NEWEST                 0
#define HAND_OLDEST                 1  
#define HAND_A                      2
#define HAND_B                      3

#define MAX_SNAP_TO_DISTANCE        20.f
#define HAND_INACTIVE_REMOVE_TIME   1000000.f     // in microseconds, i.e. one millionth of a second
#define MIN_ALIVE_TIME              0.2f         // min time hand must be alive for before it is added, in seconds


// oscpack constants
#define DEFAULT_ADDRESS "127.0.0.1"
#define PORT 7000

#define OUTPUT_BUFFER_SIZE 512


using namespace Leap;


class LeapIntoOsc : public Listener {
private:
	Controller controller;
    Frame lastFrame;
    int64_t secondLastFrameTimeLong;
    int64_t lastFrameTimeLong;
    int64_t lastTimeHands[4];
    bool handActive[4];
    
    // time
    float lastFrameTime;
    
    // one handed
    Hand singleHandNewest;  // always contains newest hand data
    Hand singleHandOldest;  // tries to hang on to same hand as long as possible
    
    // two handed - always tries for consistency
    Hand handA;
    Hand handB;

    // gestures
    bool gestureEnabled[4];
    
    // gesture vector containers
    //   discrete (key/screen tap)
    std::vector<KeyTapGesture> keyTapGestures;
    std::vector<ScreenTapGesture> screenTapGestures;
    //   continuous
    std::vector<CircleGesture> circleGestures;
    std::vector<SwipeGesture> swipeGestures;
    
    // TODO : smoothed hand (using reverse gravity system)
    
    // SETTINGS
    bool persistentHandInfo;
    float minAliveTime;

    // OSC
    const char *address;

public:
	void setAddress( const char *_address );

    virtual void onInit(const Controller&);
    virtual void onConnect(const Controller&);
    virtual void onDisconnect(const Controller&);
    virtual void onExit(const Controller&);
    virtual void onFrame(const Controller&);
	virtual void onFocusGained(const Controller&);
    virtual void onFocusLost(const Controller&);
};

void LeapIntoOsc::setAddress( const char *_address ) {
	address = _address;
}

void LeapIntoOsc::onInit(const Controller& controller) {
	for( int i = 0 ; i < 4 ; i++ ) {
	    lastTimeHands[i] = 0;
	    handActive[i] = false;
	}

	// set gestures on
	for( int i = 0 ; i < 4 ; i++ )
	    gestureEnabled[i] = false;

	// note, controller initialized automatically as it is a non-POD type (like this class)
	// initialize variables
	lastFrame = Frame::invalid();
	secondLastFrameTimeLong = 0;
	lastFrameTimeLong = 0;
	lastFrameTime = 0.f;

	// set all hands to invalid, i.e. not in use
	singleHandNewest = Leap::Hand::invalid();
	singleHandOldest = Leap::Hand::invalid();
	handA = Leap::Hand::invalid();
	handB = Leap::Hand::invalid();

	// settings
	minAliveTime = MIN_ALIVE_TIME;
	persistentHandInfo = true;

	address = DEFAULT_ADDRESS;

	std::cout << "Initialized" << std::endl;
}

void LeapIntoOsc::onConnect(const Controller& controller) {
  std::cout << "Connected" << std::endl;
  controller.enableGesture(Gesture::TYPE_CIRCLE);
  controller.enableGesture(Gesture::TYPE_KEY_TAP);
  controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
  controller.enableGesture(Gesture::TYPE_SWIPE);
  controller.setPolicyFlags(Controller::POLICY_BACKGROUND_FRAMES);
}

void LeapIntoOsc::onDisconnect(const Controller& controller) {
  std::cout << "Disconnected" << std::endl;
}

void LeapIntoOsc::onExit(const Controller& controller) {
  std::cout << "Exited" << std::endl;
}

void LeapIntoOsc::onFrame(const Controller& controller) {
	Frame frame = controller.frame();
    if( frame.id() != lastFrameTimeLong ) {
        secondLastFrameTimeLong = lastFrameTimeLong;
        lastFrameTimeLong = frame.id();
        
        // first check for new hand data
        if( !frame.hands().isEmpty() ) {
            // --- one handed ---
            
            // --- get newest hand ---
            // create block for block variable scope
            {
                // find newest hand, replace singleHandNewest with that
                float timeVisible = 10000.f;
                Hand newHand = Leap::Hand::invalid();
                for( int i = 0 ; i < frame.hands().count() ; i++ ) {
                	if( frame.hands()[i].timeVisible() < timeVisible && frame.hands()[i].timeVisible() >= minAliveTime ) {
                        timeVisible = frame.hands()[i].timeVisible();
                        newHand = frame.hands()[i];
                    }
                }
                if( newHand.isValid() ) {
                    singleHandNewest = newHand;
                    handActive[HAND_NEWEST] = true;
                    lastTimeHands[HAND_NEWEST] = frame.timestamp();
                }
            }
            
            // --- get oldest hand (oldest) ---
            // if _any_ hand has ID current oldest hand, update
            
            if( handActive[HAND_OLDEST] ) {
                bool match = false;
                for( int i = 0 ; i < frame.hands().count() ; i++ ) {
                	if( singleHandOldest.id() == frame.hands()[i].id() ) {
                        singleHandOldest = frame.hands()[i];
                        handActive[HAND_OLDEST] = true;
                        lastTimeHands[HAND_OLDEST] = frame.timestamp();
                        match = true;
                        break;
                    }
                }
                if( !match ) {
                    // find oldest hand, replace singleHandOldest with that
                    float timeVisible = 0;
                    Hand newHand = Leap::Hand::invalid();
                    for( int i = 0 ; i < frame.hands().count() ; i++ ) {
                        if( frame.hands()[i].timeVisible() >= timeVisible && frame.hands()[i].timeVisible() >= minAliveTime ) {
                            timeVisible = frame.hands()[i].timeVisible();
                            newHand = frame.hands()[i];
                        }
	                }
                    if( newHand.isValid() ) {
                        singleHandOldest = newHand;
                        handActive[HAND_OLDEST] = true;
                        lastTimeHands[HAND_OLDEST] = frame.timestamp();
                    }
                }
            } else {
                // this hand has become inactive (i.e. invalid)

                // find oldest hand, replace singleHandOldest with that
                float timeVisible = 0;
                Hand newHand = Leap::Hand::invalid();
                for( int i = 0 ; i < frame.hands().count() ; i++ ) {
                    if( frame.hands()[i].timeVisible() >= timeVisible && frame.hands()[i].timeVisible() >= minAliveTime ) {
                        timeVisible = frame.hands()[i].timeVisible();
                        newHand = frame.hands()[i];
                    }
                }
                if( newHand.isValid() ) {
                    singleHandOldest = newHand;
                    handActive[HAND_OLDEST] = true;
                    lastTimeHands[HAND_OLDEST] = frame.timestamp();
                }
            }
            
            // --- two handed ---
            std::vector<Hand> handList;
            // get two longest visible
            if( frame.hands().count() > 2 ) {
                float timeVisible = 0;
                Hand firstHand = Leap::Hand::invalid();
                for( int i = 0 ; i < frame.hands().count() ; i++ ) {
                    if( frame.hands()[i].timeVisible() >= timeVisible && frame.hands()[i].timeVisible() >= minAliveTime ) {
                        timeVisible = frame.hands()[i].timeVisible();
                        firstHand = frame.hands()[i];
                    }
                }
                handList.push_back(firstHand);
                timeVisible = 0;
                Hand secondHand = Leap::Hand::invalid();
                for( int i = 0 ; i < frame.hands().count() ; i++ ) {
                    if( frame.hands()[i].timeVisible() >= timeVisible && frame.hands()[i].timeVisible() >= minAliveTime && frame.hands()[i] != firstHand ) {
                        timeVisible = frame.hands()[i].timeVisible();
                        secondHand = frame.hands()[i];
                    }
                }
                handList.push_back(secondHand);
            } else {
                for( int i = 0 ; i < frame.hands().count() ; i++ ) { //frame.hands()[i]
                	if( frame.hands()[i].timeVisible() >= minAliveTime )
                        handList.push_back(frame.hands()[i]);
                }
            }

            // check against existing hands
            for( int i = 0 ; i < 2 ; i++ ) {
                // run loop twice as we break from it if we have an id match.
                // we do this as handList is changed after a sucessful hit
                bool match = false;
                for( std::vector<Hand>::iterator it = handList.begin(); it != handList.end(); ++it ) {
                    if( (*it).id() == handA.id() ) {
                        handA = (*it);
                        handActive[HAND_A] = true;
                        lastTimeHands[HAND_A] = frame.timestamp();
                        handList.erase(it);
                        match = true;
                        break;
                    } else if( (*it).id() == handB.id() ) {
                        handB = (*it);
                        handActive[HAND_B] = true;
                        lastTimeHands[HAND_B] = frame.timestamp();
                        handList.erase(it);
                        match = true;
                        break;
                    }
                }
                // but if we didn't get anything on the first pass, we won't get one on the second
                if( !match )
                    break;
            }
            // if we have any remaining hands, they are new
            
            // check if hand is very close to an existing hand, if it is it
            // probably is the same hand
            if( !handList.empty() ) {
                if( handActive[HAND_A] ) {
                    for( std::vector<Hand>::iterator it = handList.begin(); it != handList.end(); ++it ) {
                        if( handA.palmPosition().distanceTo((*it).palmPosition()) < MAX_SNAP_TO_DISTANCE ) {
                            handA = (*it);
                            handActive[HAND_A] = true;
                            lastTimeHands[HAND_A] = frame.timestamp();
                            handList.erase(it);
                            break;
                        }
                    }
                }
                if( handActive[HAND_B] && !handList.empty() ) {
                    for( std::vector<Hand>::iterator it = handList.begin(); it != handList.end(); ++it ) {
                        if( handB.palmPosition().distanceTo((*it).palmPosition()) < MAX_SNAP_TO_DISTANCE ) {
                            handB = (*it);
                            handActive[HAND_B] = true;
                            lastTimeHands[HAND_B] = frame.timestamp();
                            handList.erase(it);
                            break;
                        }
                    }
                }
            }
            // if some remain, they are not close to an existing hand so we can now add
            // the new hand(s) _if_ there is a free space. remember that these hands could
            // be falsely recognised. also, existing hand are removed if they are not updated
            // after HAND_INACTIVE_REMOVE_TIME seconds
            
            if( !handList.empty() ) {
                if( !handActive[HAND_A] ) {
                    for( std::vector<Hand>::iterator it = handList.begin(); it != handList.end(); ++it ) {
                        handA = (*it);
                        handActive[HAND_A] = true;
                        lastTimeHands[HAND_A] = frame.timestamp();
                        handList.erase(it);
                        break;
                    }
                }
                if( !handActive[HAND_B] ) {
                    for( std::vector<Hand>::iterator it = handList.begin(); it != handList.end(); ++it ) {
                        handB = (*it);
                        handActive[HAND_B] = true;
                        lastTimeHands[HAND_B] = frame.timestamp();
                        handList.erase(it);
                        break;
                    }
                }
            }
            
            // that should have added any hands that were meant to be added.
            // if any remain, they are not of interest
        }
        
        // calculate time since last frame
        float thisFrameTime = (float)(((double)frame.timestamp())/1000000.f);
        if( lastFrameTime == 0 )
            lastFrameTime = thisFrameTime;

        // update hands based on alive time
        for( int i = 0 ; i < 4 ; i++ ) {
            if( handActive[i] ) {
                if( (lastTimeHands[i] + HAND_INACTIVE_REMOVE_TIME) < frame.timestamp() ) {
                   handActive[i] = false;
               }
            }
        }
        
        lastFrameTime = thisFrameTime;
        
        // TODO : work gestures into system, test
        
        /*
        // process gestures
        GestureList gestures = GestureList();
        if( lastFrame.isValid() )
            gestures = frame.gestures(lastFrame);
        else
            gestures = frame.gestures();
        std::vector<int> circleIds;
        std::vector<int> swipeIds;
        //printf( "gestures: %d\n", (int)gestures.count() );
        for (int g = 0; g < gestures.count(); ++g) {
            Gesture gesture = gestures[g];
            //printf( "Got gesture\n" );
            if( gesture.type() == Gesture::TYPE_KEY_TAP ) {
                keyTapGestures.push_back(gesture);
                //printf( "   key tap\n" );
            } else if( gesture.type() == Gesture::TYPE_SCREEN_TAP ) {
                screenTapGestures.push_back(gesture);
                //printf( "   screen tap\n" );
            } else if( gesture.type() == Gesture::TYPE_CIRCLE ) {
                bool match = false;
                for( std::vector<CircleGesture>::iterator it = circleGestures.begin(); it != circleGestures.end(); ++it ) {
                    if( (*it).id() == gesture.id() ) {
                        (*it) = gesture;
                        circleIds.push_back((int)gesture.id());
                        match = true;
                        break;
                    }
                }
                if( !match ) {
                    circleGestures.push_back(gesture);
                    circleIds.push_back((int)gesture.id());
                }
                //printf( "   circle\n" );
            } else if( gesture.type() == Gesture::TYPE_SWIPE ) {
                bool match = false;
                for( std::vector<SwipeGesture>::iterator it = swipeGestures.begin(); it != swipeGestures.end(); ++it ) {
                    if( (*it).id() == gesture.id() ) {
                        (*it) = gesture;
                        swipeIds.push_back((int)gesture.id());
                        match = true;
                        break;
                    }
                }
                if( !match ) {
                    swipeGestures.push_back(gesture);
                    swipeIds.push_back((int)gesture.id());
                }
                //printf( "   swipe\n" );
            }
        }
        int len = (int)circleGestures.size();
        for( int i = 0 ; i < len ; i++ ) {
            bool change = false;
            for( std::vector<CircleGesture>::iterator it = circleGestures.begin(); it != circleGestures.end(); ++it ) {
                bool match = false;
                for( std::vector<int>::iterator it2 = circleIds.begin(); it2 != circleIds.end(); ++it2 ) {
                    if( (*it).id() == (*it2) ) {
                        //printf("circle (id %d) match, keeping\n", (*it).id() );
                        match = true;
                        break;
                    }
                }
                if( !match ) {
                    //printf("erased circle (id %d) from list\n", (*it).id() );
                    circleGestures.erase(it);
                    change = true;
                    break;
                }
            }
            if( !change )
                break;
        }
        len = (int)swipeGestures.size();
        for( int i = 0 ; i < len ; i++ ) {
            bool change = false;
            for( std::vector<SwipeGesture>::iterator it = swipeGestures.begin(); it != swipeGestures.end(); ++it ) {
                bool match = false;
                for( std::vector<int>::iterator it2 = swipeIds.begin(); it2 != swipeIds.end(); ++it2 ) {
                    if( (*it).id() == (*it2) ) {
                        match = true;
                        break;
                    }
                }
                if( !match ) {
                    swipeGestures.erase(it);
                    change = true;
                    break;
                }
            }
            if( !change )
                break;
        }
        */
        
        // keep track of last frame (this frame at the moment) to make sure we get all gestures
        lastFrame = frame;
    }


    // SEND OSC MESSAGES

    // newest hand
    if( singleHandNewest.isValid() ) {
    	UdpTransmitSocket transmitSocket( IpEndpointName( address, PORT ) );   
    	char buffer[OUTPUT_BUFFER_SIZE];
    	osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

        p << osc::BeginBundleImmediate
	        << osc::BeginMessage( "/handnew-position:" ) << (singleHandNewest.palmPosition().x / MAX_WIDTH)
	                    << (singleHandNewest.palmPosition().y / MAX_HEIGHT)
	                    << (singleHandNewest.palmPosition().z / MAX_DEPTH) << osc::EndMessage
	        << osc::BeginMessage( "/handnew-fingers:" ) << singleHandNewest.fingers().count() << osc::EndMessage
	        << osc::BeginMessage( "/handnew-velocitymag:" ) << (singleHandNewest.palmVelocity().magnitude() / MAX_VELOCITY) << osc::EndMessage
	        << osc::BeginMessage( "/handnew-velocity:" ) << (singleHandNewest.palmVelocity().x / MAX_WIDTH)
	                    << (singleHandNewest.palmVelocity().y / MAX_HEIGHT)
	                    << (singleHandNewest.palmPosition().z / MAX_DEPTH) << osc::EndMessage
	        << osc::BeginMessage( "/handnew-orientation:" ) << 0.f << 0.f << 0.f << osc::EndMessage
	        << osc::BeginMessage( "/handnew-timealive:" ) << singleHandNewest.timeVisible() << osc::EndMessage
	        << osc::BeginMessage( "/handnew-isactive:" ) << (handActive[HAND_NEWEST] ? 1.f : 0.f) << osc::EndMessage
	        << osc::EndBundle;
        transmitSocket.Send( p.Data(), p.Size() );
    }
    if( singleHandOldest.isValid() ) {
    	UdpTransmitSocket transmitSocket( IpEndpointName( address, PORT ) );   
    	char buffer[OUTPUT_BUFFER_SIZE];
    	osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

        p << osc::BeginBundleImmediate
	        << osc::BeginMessage( "/handold-position:" ) << (singleHandOldest.palmPosition().x / MAX_WIDTH)
	                    << (singleHandOldest.palmPosition().y / MAX_HEIGHT)
	                    << (singleHandOldest.palmPosition().z / MAX_DEPTH) << osc::EndMessage
	        << osc::BeginMessage( "/handold-fingers:" ) << singleHandOldest.fingers().count() << osc::EndMessage
	        << osc::BeginMessage( "/handold-velocitymag:" ) << (singleHandOldest.palmVelocity().magnitude() / MAX_VELOCITY) << osc::EndMessage
	        << osc::BeginMessage( "/handold-velocity:" ) << (singleHandOldest.palmVelocity().x / MAX_WIDTH)
	                    << (singleHandOldest.palmVelocity().y / MAX_HEIGHT)
	                    << (singleHandOldest.palmPosition().z / MAX_DEPTH) << osc::EndMessage
	        << osc::BeginMessage( "/handold-orientation:" ) << 0.f << 0.f << 0.f << osc::EndMessage
	        << osc::BeginMessage( "/handold-timealive:" ) << singleHandOldest.timeVisible() << osc::EndMessage
	        << osc::BeginMessage( "/handold-isactive:" ) << (handActive[HAND_OLDEST] ? 1.f : 0.f) << osc::EndMessage
	        << osc::EndBundle;
        transmitSocket.Send( p.Data(), p.Size() );
    }
    if( handA.isValid() ) {
    	UdpTransmitSocket transmitSocket( IpEndpointName( address, PORT ) );   
    	char buffer[OUTPUT_BUFFER_SIZE];
    	osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

        p << osc::BeginBundleImmediate
	        << osc::BeginMessage( "/handa-position:" ) << (handA.palmPosition().x / MAX_WIDTH)
	                    << (handA.palmPosition().y / MAX_HEIGHT)
	                    << (handA.palmPosition().z / MAX_DEPTH) << osc::EndMessage
	        << osc::BeginMessage( "/handa-fingers:" ) << handA.fingers().count() << osc::EndMessage
	        << osc::BeginMessage( "/handa-velocitymag:" ) << (handA.palmVelocity().magnitude() / MAX_VELOCITY) << osc::EndMessage
	        << osc::BeginMessage( "/handa-velocity:" ) << (handA.palmVelocity().x / MAX_WIDTH)
	                    << (handA.palmVelocity().y / MAX_HEIGHT)
	                    << (handA.palmPosition().z / MAX_DEPTH) << osc::EndMessage
	        << osc::BeginMessage( "/handa-orientation:" ) << 0.f << 0.f << 0.f << osc::EndMessage
	        << osc::BeginMessage( "/handa-timealive:" ) << handA.timeVisible() << osc::EndMessage
	        << osc::BeginMessage( "/handa-isactive:" ) << (handActive[HAND_A] ? 1.f : 0.f) << osc::EndMessage
	        << osc::EndBundle;
        transmitSocket.Send( p.Data(), p.Size() );
    }
    if( handB.isValid() ) {
    	UdpTransmitSocket transmitSocket( IpEndpointName( address, PORT ) );   
    	char buffer[OUTPUT_BUFFER_SIZE];
    	osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

        p << osc::BeginBundleImmediate
	        << osc::BeginMessage( "/handb-position:" ) << (handB.palmPosition().x / MAX_WIDTH)
	                    << (handB.palmPosition().y / MAX_HEIGHT)
	                    << (handB.palmPosition().z / MAX_DEPTH) << osc::EndMessage
	        << osc::BeginMessage( "/handb-fingers:" ) << handB.fingers().count() << osc::EndMessage
	        << osc::BeginMessage( "/handb-velocitymag:" ) << (handB.palmVelocity().magnitude() / MAX_VELOCITY) << osc::EndMessage
	        << osc::BeginMessage( "/handb-velocity:" ) << (handB.palmVelocity().x / MAX_WIDTH)
	                    << (handB.palmVelocity().y / MAX_HEIGHT)
	                    << (handB.palmPosition().z / MAX_DEPTH) << osc::EndMessage
	        << osc::BeginMessage( "/handb-orientation:" ) << 0.f << 0.f << 0.f << osc::EndMessage
	        << osc::BeginMessage( "/handb-timealive:" ) << handB.timeVisible() << osc::EndMessage
	        << osc::BeginMessage( "/handb-isactive:" ) << (handActive[HAND_B] ? 1.f : 0.f) << osc::EndMessage
	        << osc::EndBundle;
        transmitSocket.Send( p.Data(), p.Size() );
    }
    
    if( handA.isValid() && handB.isValid() ) {
    	UdpTransmitSocket transmitSocket( IpEndpointName( address, PORT ) );   
    	char buffer[OUTPUT_BUFFER_SIZE];
    	osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
    	p << osc::BeginBundleImmediate;

        float dist = handA.palmPosition().distanceTo(handB.palmPosition());
        dist = (dist - DIST_START);
        if( dist > 0 )
            p << osc::BeginMessage( "/handaandb-dist:" ) << (dist / MAX_DIST) << osc::EndMessage;
        else
            p << osc::BeginMessage( "/handaandb-dist:" ) << 0.f << osc::EndMessage;
        p << osc::EndBundle;
        transmitSocket.Send( p.Data(), p.Size() );
    }
}


void LeapIntoOsc::onFocusGained(const Controller& controller) {
//  std::cout << "Focus Gained" << std::endl;
}

void LeapIntoOsc::onFocusLost(const Controller& controller) {
//  std::cout << "Focus Lost" << std::endl;
}

// 'int argc, char* argv[]' added to 'int main' for oscpack
int main(int argc, char* argv[]) {

	std::cout << "Starting LeapIntoOsc..." << std::endl;

	// Create a sample listener and controller
	LeapIntoOsc listener;
	Controller controller;

	// Have the sample listener receive events from the controller
	controller.addListener(listener);

	// Keep this process running until Enter is pressed
	std::cout << "Press Enter to quit..." << std::endl;
	std::cin.get();

	// Remove the sample listener when done
	controller.removeListener(listener);

	return 0;
}
