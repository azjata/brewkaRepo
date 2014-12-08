#pragma once

#ifndef __NETWORK_CONSUMER_H__
#define __NETWORK_CONSUMER_H__

#include "ofThread.h"
#include "ofxNetwork.h"
#include "MessageProcessor.h"

/*
	Network consumer listens to an address and port and receives messages through the network.
	Every message received is sent to a queue in another thread (MessageProcessor) to be processed.
*/
class NetworkConsumer : public ofThread {
public:
	NetworkConsumer();
	~NetworkConsumer(void);
	void setup(char * address, int port);
	void processNetworkMsg(string message);
	void threadedFunction();
	ofxUDPManager udpConnection;
	MessageProcessor messageProcessor;
	void close();
	bool closed;
};

#endif