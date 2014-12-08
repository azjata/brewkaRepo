#include "OSCProcessor.h"

OSCProcessor::OSCProcessor(void)
{
}

OSCProcessor::~OSCProcessor(void)
{
}

void OSCProcessor::threadedFunction() {

	while(isThreadRunning()) {
		lock();
		if (!this->requestQueue.empty()) {
			ofxOscMessage currMsg = this->requestQueue.front();
			// parse message
			//if(currMsg.getAddress() == OSC_LOCAL::RESTART_WORLD){
				// get arguments
				// process
			//}
			this->requestQueue.pop();
		}
		unlock();
    }

}