#pragma once

#ifndef __OSC_CONSUMER_H__
#define __OSC_CONSUMER_H__

#include "ofThread.h"
#include "../../StuntAppEvents.h"
#include "ofxOscReceiver.h"
#include "OSCProcessor.h"
#include "ofEvents.h"

/*
	Network consumer listens to an address and port and receives messages through the network.
	Every message received is sent to a queue in another thread (MessageProcessor) to be processed.
*/
class OSCConsumer : public ofThread {
public:
	OSCConsumer();
	~OSCConsumer(void);
	void setup(int port);
	void processOscMsg(ofxOscMessage message);
	void threadedFunction();
	ofxOscReceiver receiver;
	queue<int> pendingEvents;
	bool lockQueue;
	OSCProcessor oscProcessor;
};

#endif