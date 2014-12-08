#pragma once

#ifndef __MESSAGE_PROCESSOR_H__
#define __MESSAGE_PROCESSOR_H__

#include "ofThread.h"
#include "../UserManager.h"
#include "../body/BodyFactory.h"
#include <queue>

class MessageProcessor : public ofThread {
public:
	MessageProcessor();
	~MessageProcessor(void);
	void threadedFunction();
	queue<string> requestQueue;
};

#endif