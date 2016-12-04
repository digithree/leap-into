//
//  LeapIntoCSound.cpp
//  LeapIntoCSound
//
//  Created by Simon Kenny on 12/07/2013.
//  Copyright (c) 2013 Simon Kenny. All rights reserved.
//

//#include "float-version-double.h"
#include "Leap.h"
#include <OpcodeBase.hpp>
#include <vector>

#define MAX_HEIGHT                  800
#define MAX_DEPTH                   400
#define MAX_WIDTH                   400

#define MAX_VELOCITY                1500
#define MAX_SWIPE_VELOCITY          4500

#define DIST_START                  250
#define MAX_DIST                    1000

#define TIME_THRESHOLD              20000

#define HAND_NEWEST                 0
#define HAND_OLDEST                 1  
#define HAND_A                      2
#define HAND_B                      3

#define MAX_SNAP_TO_DISTANCE        20
#define HAND_INACTIVE_REMOVE_TIME   1000000     // in microseconds, i.e. one millionth of a second
#define MIN_ALIVE_TIME              0.2         // min time hand must be alive for before it is added, in seconds


using namespace Leap;

/* LeapHandUpdater
 *
 * Only concerned with making sure the best hand data is avaliable
 * for interpretation. Opcodes must make sense of this data
 * individually.
 *
 * Main function is update(), which has a check against double
 * frame processing if called more than once per frame by different
 * Opcodes.
 */
class LeapHandUpdater {
private:
    Controller controller;
    Frame lastFrame;
    int64_t secondLastFrameTimeLong;
    int64_t lastFrameTimeLong;
    int64_t lastTimeHands[4] = {}; // auto zero filling
    bool handActive[4] = { false, false, false, false };
    
    // time
    float lastFrameTime;
    
    // one handed
    Hand singleHandNewest;  // always contains newest hand data
    Hand singleHandOldest;  // tries to hang on to same hand as long as possible
    
    // two handed - always tries for consistency
    Hand handA;
    Hand handB;
    
    // hand reference array
    Hand *hands[4];
    
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
    bool backgroundAppRequest;
    
public:
    LeapHandUpdater() {
        // set gestures on
        for( int i = 0 ; i < 4 ; i++ )
            gestureEnabled[i] = false;
        
        // set settings for gestures
        // key tap
        // TODO: fix this, for some reason config() doesn't seem to exist in the library, though it's in the header
        /*
        if(controller.config().setFloat("Gesture.KeyTap.MinDownVelocity", 30.0) &&
           controller.config().setFloat("Gesture.KeyTap.HistorySeconds", .4) &&
           controller.config().setFloat("Gesture.KeyTap.MinDistance", 4.0))
            controller.config().save();
        */
        /*
        Config config = controller.config();
        config.setInt32("Gesture.KeyTap.MinDownVelocity", 30);
        config.setFloat("Gesture.KeyTap.HistorySeconds", .4);
        config.setFloat("Gesture.KeyTap.MinDistance", 4.0);
        config.save();
         */
        
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
        addHandsReferences();
        
        // settings
        minAliveTime = MIN_ALIVE_TIME;
        persistentHandInfo = true;
        backgroundAppRequest = false;
    }
    
    void enableGesture( int type ) {
        if( type >= 0 && type < 4 ) {
            if( !gestureEnabled[type] ) {
                gestureEnabled[type] = true;
                switch( type ) {
                    case 0:
                        controller.enableGesture(Gesture::TYPE_KEY_TAP);
                        break;
                    case 1:
                        controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
                        break;
                    case 2:
                        controller.enableGesture(Gesture::TYPE_CIRCLE);
                        break;
                    case 3:
                        controller.enableGesture(Gesture::TYPE_SWIPE);
                        break;
                    default:
                        break;
                }
                printf( "enabled gesture type %d\n", (int)type );
            }
        }
    }
    
    void addHandsReferences() {
        hands[0] = &singleHandNewest;
        hands[1] = &singleHandOldest;
        hands[2] = &handA;
        hands[3] = &handB;
    }
    
    void settings( bool persistent, bool bgApp, float minAlive ) {
        persistentHandInfo = persistent;
        minAliveTime = minAlive;
        if( backgroundAppRequest != bgApp ) {
            backgroundAppRequest = bgApp;
            if( backgroundAppRequest )
                controller.setPolicyFlags(Controller::POLICY_BACKGROUND_FRAMES);
            else
                controller.setPolicyFlags(Controller::POLICY_DEFAULT);
        }
    }
    
    bool update() {
        Frame frame = controller.frame();
        if( frame.id() != lastFrameTimeLong ) {
            secondLastFrameTimeLong = lastFrameTimeLong;
            lastFrameTimeLong = frame.id();
            
            // first check for new hand data
            if( !frame.hands().isEmpty() ) {
                //printf("--- Frame: sec %f ---\n", (((double)frame.timestamp())/1000000.f) );
                // --- one handed ---
                
                // --- get newest hand ---
                // create block for block variable scope
                {
                    //printf("   no match, finding newest hand...\n");
                    // find newest hand, replace singleHandNewest with that
                    float timeVisible = 10000.f;
                    Hand newHand = Leap::Hand::invalid();
                    for( Hand hand : frame.hands() ) {
                        //printf( "      hand (id %d) visible for %f seconds\n", hand.id(), hand.timeVisible() );
                        if( hand.timeVisible() < timeVisible && hand.timeVisible() >= minAliveTime ) {
                            //printf("         shortest\n");
                            timeVisible = hand.timeVisible();
                            newHand = hand;
                        }
                    }
                    if( newHand.isValid() ) {
                        singleHandNewest = newHand;
                        handActive[HAND_NEWEST] = true;
                        lastTimeHands[HAND_NEWEST] = frame.timestamp();
                        //printf("   set new hand to hand - id %d\n", singleHandNewest.id() );
                    } //else
                        //printf("   did NOT set new hand\n");
                }
                
                // --- get oldest hand (oldest) ---
                // if _any_ hand has ID current oldest hand, update
                //printf("Oldest hand: ");
                
                //if( singleHandOldest.isValid() ) {
                if( handActive[HAND_OLDEST] ) {
                    //printf("hand exists\n");
                    bool match = false;
                    for( Hand hand : frame.hands() ) {
                        if( singleHandOldest.id() == hand.id() ) {
                            singleHandOldest = hand;
                            handActive[HAND_OLDEST] = true;
                            lastTimeHands[HAND_OLDEST] = frame.timestamp();
                            match = true;
                            //printf("   got match - id %d\n", singleHandOldest.id() );
                            break;
                        }
                    }
                    if( !match ) {
                        //printf("   no match, finding oldest hand...\n");
                        // find oldest hand, replace singleHandOldest with that
                        float timeVisible = 0;
                        Hand newHand = Leap::Hand::invalid();
                        for( Hand hand : frame.hands() ) {
                            //printf( "      hand (id %d) visible for %f seconds\n", hand.id(), hand.timeVisible() );
                            if( hand.timeVisible() >= timeVisible && hand.timeVisible() >= minAliveTime ) {
                                //printf("         longest\n");
                                timeVisible = hand.timeVisible();
                                newHand = hand;
                            }
                        }
                        if( newHand.isValid() ) {
                            singleHandOldest = newHand;
                            handActive[HAND_OLDEST] = true;
                            lastTimeHands[HAND_OLDEST] = frame.timestamp();
                            //printf("   set new hand to hand - id %d\n", singleHandOldest.id() );
                        } //else
                            //printf("   did NOT set new hand\n");
                    }
                } else {
                    //printf("NO hand exists\n");
                    // this hand has become inactive (i.e. invalid)

                    // find oldest hand, replace singleHandOldest with that
                    //printf("   finding oldest hand...\n");
                    float timeVisible = 0;
                    Hand newHand = Leap::Hand::invalid();
                    for( Hand hand : frame.hands() ) {
                        //printf( "      hand (id %d) visible for %f seconds\n", hand.id(), hand.timeVisible() );
                        if( hand.timeVisible() >= timeVisible && hand.timeVisible() >= minAliveTime ) {
                            //printf("         longest\n");
                            timeVisible = hand.timeVisible();
                            newHand = hand;
                        }
                    }
                    if( newHand.isValid() ) {
                        singleHandOldest = newHand;
                        handActive[HAND_OLDEST] = true;
                        lastTimeHands[HAND_OLDEST] = frame.timestamp();
                        //printf("   set new hand to hand - id %d\n", singleHandOldest.id() );
                    } //else
                            //printf("   did NOT set new hand\n");
                }
                
                // --- two handed ---
                //printf("Getting two handed info (have %d hand(s))...\n", frame.hands().count() );
                std::vector<Hand> handList;
                // get two longest visible
                if( frame.hands().count() > 2 ) {
                    //printf( "   have more than two hands\n");
                    //printf( "   checking which hands have existed the longest...\n");
                    float timeVisible = 0;
                    Hand firstHand = Leap::Hand::invalid();
                    for( Hand hand : frame.hands() ) {
                        //printf( "      hand visible for %f seconds\n", hand.timeVisible() );
                        if( hand.timeVisible() >= timeVisible && hand.timeVisible() >= minAliveTime ) {
                            //printf("         longest\n");
                            timeVisible = hand.timeVisible();
                            firstHand = hand;
                        }
                    }
                    handList.push_back(firstHand);
                    //printf( "   checking which hands have existed the longest (again)...\n");
                    timeVisible = 0;
                    Hand secondHand = Leap::Hand::invalid();
                    for( Hand hand : frame.hands() ) {
                        //printf( "      hand visible for %f seconds\n", hand.timeVisible() );
                        if( hand.timeVisible() >= timeVisible && hand.timeVisible() >= minAliveTime && hand != firstHand ) {
                            //printf("         longest\n");
                            timeVisible = hand.timeVisible();
                            secondHand = hand;
                        }
                    }
                    handList.push_back(secondHand);
                } else {
                    //printf( "   have two or less hands\n");
                    for( Hand hand : frame.hands() ) {
                        if( hand.timeVisible() >= minAliveTime )
                            handList.push_back(hand);
                    }
                }
                // check against existing hands
                //printf("Checking hand(s) against existing hands...\n");
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
                            //printf( "   matched hand %d to hand A - id %d\n", i, handA.id() );
                            break;
                        } else if( (*it).id() == handB.id() ) {
                            handB = (*it);
                            handActive[HAND_B] = true;
                            lastTimeHands[HAND_B] = frame.timestamp();
                            handList.erase(it);
                            match = true;
                            //printf( "   matched hand %d to hand B - id %d\n", i, handB.id() );
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
                    //printf("Hands remain, check if new hand(s) close to existing hands...\n");
                    
                    //if( handA.isValid() ) {
                    if( handActive[HAND_A] ) {
                        for( std::vector<Hand>::iterator it = handList.begin(); it != handList.end(); ++it ) {
                            if( handA.palmPosition().distanceTo((*it).palmPosition()) < MAX_SNAP_TO_DISTANCE ) {
                                handA = (*it);
                                handActive[HAND_A] = true;
                                lastTimeHands[HAND_A] = frame.timestamp();
                                handList.erase(it);
                                //printf( "   some hand close enough to hand A - id %d\n", handA.id() );
                                break;
                            }
                        }
                    }
                    //if( handB.isValid() && !handList.empty() ) {
                    if( handActive[HAND_B] && !handList.empty() ) {
                        for( std::vector<Hand>::iterator it = handList.begin(); it != handList.end(); ++it ) {
                            if( handB.palmPosition().distanceTo((*it).palmPosition()) < MAX_SNAP_TO_DISTANCE ) {
                                handB = (*it);
                                handActive[HAND_B] = true;
                                lastTimeHands[HAND_B] = frame.timestamp();
                                handList.erase(it);
                                //printf( "   some hand close enough to hand B - id %d\n", handB.id() );
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
                    //printf("Hands remain, add hand(s) iff we have a free slot\n" );
                    
                    //if( !handA.isValid() ) {
                    if( !handActive[HAND_A] ) {
                        for( std::vector<Hand>::iterator it = handList.begin(); it != handList.end(); ++it ) {
                            handA = (*it);
                            handActive[HAND_A] = true;
                            lastTimeHands[HAND_A] = frame.timestamp();
                            handList.erase(it);
                            //printf( "   added some hand to hand A - id %d\n", handA.id() );
                            break;
                        }
                    }
                    //if( !handB.isValid() ) {
                    if( !handActive[HAND_B] ) {
                        for( std::vector<Hand>::iterator it = handList.begin(); it != handList.end(); ++it ) {
                            handB = (*it);
                            handActive[HAND_B] = true;
                            lastTimeHands[HAND_B] = frame.timestamp();
                            handList.erase(it);
                            //printf( "   added some hand to hand B - id %d\n", handB.id() );
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
            //float elapsedTime = thisFrameTime - lastFrameTime;
            //printf("elapsed time: %f\n", elapsedTime );
            
            // update hands based on alive time
            for( int i = 0 ; i < 4 ; i++ ) {
                if( handActive[i] ) {
                    if( (lastTimeHands[i] + HAND_INACTIVE_REMOVE_TIME) < frame.timestamp() ) {
                       //printf( "\nRemoved hand code %d - id %d\n\n", i, (*hands[i]).id() );
                       handActive[i] = false;
                   }
                }
            }
            
            lastFrameTime = thisFrameTime;
            
            // update references
            addHandsReferences();
            
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
            
            // keep track of last frame (this frame at the moment) to make sure we get all gestures
            lastFrame = frame;
            // signal that we _did_ process a frame
            return true;
        }
        // already processed this frame (though it's unlikely)
        return false;
    }
    
    // accessors
    bool isPersistent() { return persistentHandInfo; }
    
    bool isHandActive( int type ) {
        if( type >= 0 && type < 4 )
            return handActive[type];
        return false;
    }
    
    Hand* getHand( int type ) {
        if( type >= 0 && type < 4 ) {
            //if( hands[type]->isValid() )
            if( handActive[type] || persistentHandInfo )
                return hands[type];
        }
        return NULL;
    }
    
    int64_t getLastFrame() {
        return secondLastFrameTimeLong;
    }
    
    // gesture accessors
    // discrete
    std::vector<KeyTapGesture> *getKeyTapGestureList() { return &keyTapGestures; }
    std::vector<ScreenTapGesture> *getScreenTapGestureList() { return &screenTapGestures; }
    std::vector<CircleGesture> *getCircleGestureList() { return &circleGestures; }
    std::vector<SwipeGesture> *getSwipeGestureList() { return &swipeGestures; }
};




struct LeapHand : public OpcodeBase<LeapHand> {
    
public:
    // All output fields must be declared first as MYFLT *:
    MYFLT *kout1, *kout2, *kout3;
    
    // Input, type and info
    MYFLT *iarg1, *iarg2;
    
    // All internal state variables must be declared after that:
    LeapHandUpdater **leapHandUpdaterPtr;
    
    // Declare and implement required inherited methods
    int init(CSOUND *csound) {
        if( *iarg1 < 4 && (*iarg2 == 0 || *iarg2 == 3 || *iarg2 == 4) ) {
            *kout1 = 0.f;
            *kout2 = 0.f;
            *kout3 = 0.f;
        } else
            *kout1 = 0.f;
        // get global variable for leapHandUpdater, or create if doesn't exist
        leapHandUpdaterPtr = (LeapHandUpdater**) csound->QueryGlobalVariable(csound, "leapHandUpdater");
        if( leapHandUpdaterPtr == NULL ) {
            csound->CreateGlobalVariable( csound, "leapHandUpdater", sizeof(LeapHandUpdater*));
            leapHandUpdaterPtr = (LeapHandUpdater**) csound->QueryGlobalVariable(csound, "leapHandUpdater");
            if( leapHandUpdaterPtr == NULL )
                csound->InitError(csound, Str("error... couldn't create global leap updater\n"));
            else
                (*leapHandUpdaterPtr) = new LeapHandUpdater();
        }
        //(*(*leapHandUpdaterPtr)).settings(1,0,0.2);
        return OK;
    }
    
    int kontrol(CSOUND *csound) {
        // check global variable leapHandUpdater exists
        leapHandUpdaterPtr = (LeapHandUpdater**) csound->QueryGlobalVariable(csound, "leapHandUpdater");
        if( leapHandUpdaterPtr == NULL )
            csound->ErrorMsg(csound, Str("error... couldn't read global leap updater\n"));
        // dereference leapHandUpdater double ptr and update
        LeapHandUpdater *leapHandUpdater = *leapHandUpdaterPtr;
        (*leapHandUpdater).update();
        if( (*iarg1) >= 0 && (*iarg1) < 4 ) {   // newest, oldest, hand A or hand B
            Hand *hand = (*leapHandUpdater).getHand((*iarg1));
            if( hand == NULL )
                return OK;
            
            if( ((int)(*iarg2)) == 0 ) {                    // hand position
                    Vector handPos = hand->palmPosition();
                    *kout1 = handPos.x / MAX_WIDTH;
                    *kout2 = handPos.y / MAX_HEIGHT;
                    *kout3 = handPos.z / MAX_DEPTH;
            } else if( ((int)(*iarg2)) == 1 ) {             // number of fingers
                    *kout1 = hand->fingers().count();
            } else if( ((int)(*iarg2)) == 2 ) {             // hand velocity (magniute)
                *kout1 = (hand->palmVelocity().magnitude() / MAX_VELOCITY);
            } else if( ((int)(*iarg2)) == 3 ) {             // hand velocity (vector)
                    Vector vel = hand->palmVelocity();
                    *kout1 = vel.x / MAX_WIDTH;
                    *kout2 = vel.y / MAX_HEIGHT;
                    *kout3 = vel.z / MAX_DEPTH;
            } else if( ((int)(*iarg2)) == 4 ) {             // oriantation
                    // TODO : implement
            } else if( ((int)(*iarg2)) == 5 ) {             // hand alive time
                    *kout1 = hand->timeVisible();
            } else if( ((int)(*iarg2)) == 6 ) {             // activity
                    *kout1 = (*leapHandUpdater).isHandActive(*iarg1) ? 1.f : 0.f;
            }
        } else if( (*iarg1) == 4 ) {            // both hands A & B
            Hand *handA = (*leapHandUpdater).getHand(HAND_A);
            Hand *handB = (*leapHandUpdater).getHand(HAND_B);
            if( handA == NULL || handB == NULL )
               return OK;
            
            if( ((int)(*iarg2)) == 0 ) {                    // distance between hands
                float dist = handA->palmPosition().distanceTo(handB->palmPosition());
                dist = (dist - DIST_START);
                if( dist > 0 )
                    *kout1 = dist / MAX_DIST;
                else
                    *kout1 = 0.f;
            } else if( ((int)(*iarg2)) == 1 ) {             // angle of rotation between hands (on xz-plane)
                // TODO : implement
            }
        }
        return OK;
    }
};


struct LeapSettings : public OpcodeBase<LeapSettings> {
    
public:
    // All output fields must be declared first as MYFLT *:
    // none
    
    // No input
    MYFLT *ipersistent, *obgapp, *hminalive;
    
    // All internal state variables must be declared after that:
    /// none
    
    // Declare and implement required inherited methods
    int init(CSOUND *csound) {
        // no way to give default value of 0.2 to csound invar, so anything > 10 defaults to 0.2
        if( *hminalive > 10 )
            *hminalive = MIN_ALIVE_TIME;
        // get global variable for leapHandUpdater, or create if doesn't exist
        LeapHandUpdater **leapHandUpdaterPtr = (LeapHandUpdater**) csound->QueryGlobalVariable(csound, "leapHandUpdater");
        if( leapHandUpdaterPtr == NULL ) {
            csound->CreateGlobalVariable( csound, "leapHandUpdater", sizeof(LeapHandUpdater*));
            leapHandUpdaterPtr = (LeapHandUpdater**) csound->QueryGlobalVariable(csound, "leapHandUpdater");
            if( leapHandUpdaterPtr == NULL )
                csound->InitError(csound, Str("error... couldn't create global leap updater\n"));
            else
                (*leapHandUpdaterPtr) = new LeapHandUpdater();
        }
        LeapHandUpdater *leapHandUpdater = *leapHandUpdaterPtr;
        (*leapHandUpdater).settings( (*ipersistent)==1?true:false, (*obgapp)==1?true:false, *hminalive);
        return OK;
    }
};


struct LeapGestures : public OpcodeBase<LeapGestures> {
    
public:
    // All output fields must be declared first as MYFLT *:
    MYFLT *kout1, *kout2, *kout3;
    
    // Input, type and info
    MYFLT *iarg1, *iarg2;
    
    // All internal state variables must be declared after that:
    MYFLT lastCircleProgress, circleChange;
    LeapHandUpdater **leapHandUpdaterPtr;
    
    // Declare and implement required inherited methods
    int init(CSOUND *csound) {
        printf("initialising leapgesture\n");
        if( *iarg2 == 0 || *iarg2 == 1 || *iarg2 == 4 ) {
            *kout1 = 0.f;
            *kout2 = 0.f;
            *kout3 = 0.f;
        } else
            *kout1 = 0.f;
        // initialise internal state vars
        lastCircleProgress = 0.f;
        circleChange = 0.f;
        // get global variable for leapHandUpdater, or create if doesn't exist
        LeapHandUpdater **leapHandUpdaterPtr = (LeapHandUpdater**) csound->QueryGlobalVariable(csound, "leapHandUpdater");
        if( leapHandUpdaterPtr == NULL ) {
            csound->CreateGlobalVariable( csound, "leapHandUpdater", sizeof(LeapHandUpdater*));
            leapHandUpdaterPtr = (LeapHandUpdater**) csound->QueryGlobalVariable(csound, "leapHandUpdater");
            if( leapHandUpdaterPtr == NULL )
                csound->InitError(csound, Str("error... couldn't create global leap updater\n"));
            else
                (*leapHandUpdaterPtr) = new LeapHandUpdater();
        }
        return OK;
    }
    
    int kontrol(CSOUND *csound) {
        // check global variable leapHandUpdater exists
        leapHandUpdaterPtr = (LeapHandUpdater**) csound->QueryGlobalVariable(csound, "leapHandUpdater");
        if( leapHandUpdaterPtr == NULL )
            csound->ErrorMsg(csound, Str("error... couldn't read global leap updater\n"));
        // dereference leapHandUpdater double ptr and update
        LeapHandUpdater *leapHandUpdater = *leapHandUpdaterPtr;
        (*leapHandUpdater).enableGesture(*iarg1);
        (*leapHandUpdater).update();
        if( *iarg1 == 0 ) {         // key tap
            std::vector<KeyTapGesture> *keyTapGestures = (*leapHandUpdater).getKeyTapGestureList();
            if( (*keyTapGestures).size() > 0 ) {
                for( std::vector<KeyTapGesture>::iterator it = (*keyTapGestures).begin(); it != (*keyTapGestures).end(); ++it ) {
                    // no need to see what karg2, i.e. kgdata is, as it should be 0, and this is the only type
                    // get position
                    Vector pos = (*it).position();
                    *kout1 = pos.x / MAX_WIDTH;
                    *kout2 = pos.y / MAX_HEIGHT;
                    *kout3 = pos.z / MAX_DEPTH;
                    (*keyTapGestures).erase(it);
                    break;
                }
            } else if( !(*leapHandUpdater).isPersistent() ) {
                *kout1 = 0.f;
                *kout2 = 0.f;
                *kout3 = 0.f;
            }
        } else if( *iarg1 == 1 ) {         // screen tap
            std::vector<ScreenTapGesture> *screenTapGestures = (*leapHandUpdater).getScreenTapGestureList();
            if( (*screenTapGestures).size() > 0 ) {
                for( std::vector<ScreenTapGesture>::iterator it = (*screenTapGestures).begin(); it != (*screenTapGestures).end(); ++it ) {
                    // no need to see what karg2, i.e. kgdata is, as it should be 0, and this is the only type
                    // get position
                    Vector pos = (*it).position();
                    *kout1 = pos.x / MAX_WIDTH;
                    *kout2 = pos.y / MAX_HEIGHT;
                    *kout3 = pos.z / MAX_DEPTH;
                    (*screenTapGestures).erase(it);
                    break;
                }
            } else if( !(*leapHandUpdater).isPersistent() ) {
                *kout1 = 0.f;
                *kout2 = 0.f;
                *kout3 = 0.f;
            }
        } else if( *iarg1 == 2 ) {         // cirlce
            std::vector<CircleGesture> *circleGestures = (*leapHandUpdater).getCircleGestureList();
            if( (*circleGestures).size() > 0 ) {
                for( std::vector<CircleGesture>::iterator it = (*circleGestures).begin(); it != (*circleGestures).end(); ++it ) {
                    // TODO: support more than one circle at a time
                    if( *iarg2 == 0 ) {                 // center position
                        Vector pos = (*it).center();
                        *kout1 = pos.x / MAX_WIDTH;
                        *kout2 = pos.y / MAX_HEIGHT;
                        *kout3 = pos.z / MAX_DEPTH;
                    } else if( *iarg2 == 1 ) {          // progress
                        *kout1 = (*it).progress();
                    } else if( *iarg2 == 2 ) {          // progress change
                        if( (*it).progress() == lastCircleProgress )
                            *kout1 = circleChange;
                        else if( (*it).progress() - lastCircleProgress > 0 ) {
                            circleChange = (*it).progress() - lastCircleProgress;
                            *kout1 = circleChange;
                        } else {
                            circleChange = 0.f;
                            *kout1 = 0.f;
                        }
                    } else if( *iarg2 == 3 ) {          // angle
                        // TODO: implement angle calculation
                    } else if( *iarg2 == 4 ) {          // radius
                        *kout1 = (*it).radius();
                    } else if( *iarg2 == 5 ) {
                        Vector norm = (*it).normal();
                        *kout1 = norm.x;
                        *kout2 = norm.y;
                        *kout3 = norm.z;
                    } else if( *iarg2 == 6 ) {
                        *kout1 = (*it).durationSeconds();
                    }
                    // maintain circle progress rate variable
                    lastCircleProgress = (*it).progress();
                    
                    // obviously don't break every time once the system supports multiple circle gestures at once
                    break;
                }
            } else if( !(*leapHandUpdater).isPersistent() ) {
                if( *iarg2 == 0 || *iarg2 == 5 ) {
                    *kout1 = 0.f;
                    *kout2 = 0.f;
                    *kout3 = 0.f;
                } else
                    *kout1 = 0.f;
            }
        } else if( *iarg1 == 3 ) {         // swipe
            std::vector<SwipeGesture> *swipeGestures = (*leapHandUpdater).getSwipeGestureList();
            if( (*swipeGestures).size() > 0 ) {
                for( std::vector<SwipeGesture>::iterator it = (*swipeGestures).begin(); it != (*swipeGestures).end(); ++it ) {
                    // TODO: support more than one swipe at a time
                    if( *iarg2 == 0 ) {                  // current position
                        Vector pos = (*it).position();
                        *kout1 = pos.x / MAX_WIDTH;
                        *kout2 = pos.y / MAX_HEIGHT;
                        *kout3 = pos.z / MAX_DEPTH;
                    } else if( *iarg2 == 1 ) {          // start position
                        Vector pos = (*it).startPosition();
                        *kout1 = pos.x / MAX_WIDTH;
                        *kout2 = pos.y / MAX_HEIGHT;
                        *kout3 = pos.z / MAX_DEPTH;
                    } else if( *iarg2 == 2 ) {          // length of swipe
                        Vector length = (*it).position() - (*it).startPosition();
                        if( length.magnitude() != 0 )
                            *kout1 = length.magnitude() / MAX_DIST;
                        else
                            *kout1 = 0.f;
                    } else if( *iarg2 == 3 ) {          // speed / velocity
                        *kout1 = (*it).speed() / MAX_SWIPE_VELOCITY;
                    } else if( *iarg2 == 4 ) {          // direction
                        Vector dir = (*it).direction();
                        *kout1 = dir.x;
                        *kout2 = dir.y;
                        *kout3 = dir.z;
                    } else if( *iarg2 == 5 ) {          // alive time
                        *kout1 = (*it).durationSeconds();
                    }
                    // obviously don't break every time once the system supports multiple circle gestures at once
                    break;
                }
            } else if( !(*leapHandUpdater).isPersistent() ) {
                if( *iarg2 == 0 || *iarg2 == 1 || *iarg2 == 4 ) {
                    *kout1 = 0.f;
                    *kout2 = 0.f;
                    *kout3 = 0.f;
                } else
                    *kout1 = 0.f;
            }
        }
        return OK;
    }
};




// ------ Opcode registration code ------

extern "C"
{
    
    static OENTRY localops[] = {
        // kout1 [,kout2, kout3] leaphand khand, kinfo
        {
            "leaphand",
            sizeof(LeapHand),
            0,  // NEW FIELD - flags : don't know what it means
            3,  //i-rate and k-rate
            "kzz",  //output, k-rate and two optional k-rates
            "ii",     //input, i-rate and k-rate
            (SUBR)&LeapHand::init_,
            (SUBR)&LeapHand::kontrol_,
            0
        },
        // leapsettings iminalive, ipersistent [,ismooth, ifadein, ifadeout]
        {
            "leapsettings",
            sizeof(LeapSettings),
            0,
            1,  //i-rate
            "",  // no output
            "ioh",     //input
            (SUBR)&LeapSettings::init_,
            NULL,
            0
        },
        // kout1 [,kout2, kout3] leapgesture ktype, kinfo
        {
            "leapgesture",
            sizeof(LeapGestures),
            0,
            3,  //i-rate and k-rate
            "kzz",  //output
            "io",     //input
            (SUBR)&LeapGestures::init_,
            (SUBR)&LeapGestures::kontrol_,
            0
        },
        { NULL, 0, 0, 0, NULL, NULL, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL }
    };
    
    LINKAGE
}// END EXTERN C
