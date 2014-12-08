#pragma once

#ifndef __OSC_PROCESSOR_H__
#define __OSC_PROCESSOR_H__

#include "ofThread.h"
#include <queue>
#include "ofxOscMessage.h"
#include "../../constants.h"

class OSCProcessor : public ofThread {
public:
	OSCProcessor();
	~OSCProcessor(void);
	void threadedFunction();
	queue<ofxOscMessage> requestQueue;
};

#endif