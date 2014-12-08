#include "OSCConsumer.h"

OSCConsumer::OSCConsumer(void)
{
	lockQueue=false;
}

OSCConsumer::~OSCConsumer(void)
{
}

void OSCConsumer::setup( int port) {
	receiver.setup(port);
	//start message processor thread
	oscProcessor.startThread(true, false);
}

void OSCConsumer::threadedFunction() {
	while(isThreadRunning()) {
		while(receiver.hasWaitingMessages()) {
			// get the next message
			ofxOscMessage m;
			receiver.getNextMessage(&m);
			processOscMsg(m);
		}
	}
	// stop the message processor thread
	oscProcessor.stopThread();
}

void OSCConsumer::processOscMsg(ofxOscMessage message) {
	// lock access to the resource
	oscProcessor.lock();
	lockQueue=true;
	//const StuntAppEvents::StuntEventType eventType = StuntAppEvents::UNKNOWN;
	string address = message.getAddress();
	int type = -1;
	cout<<address<<endl;
	if(address == "/StuntApp/Start")
	{
		type = StuntAppEvents::START;
	}
	else if(address == "/StuntApp/Reset")
	{
		type= StuntAppEvents::RESET;
	}
	else if(address == "/StuntApp/StartFans")
	{
		type= StuntAppEvents::FANS;
	}
	else if(address == "/StuntApp/CalibrateOR")
	{
		type= StuntAppEvents::CALIBRATE;
	}
	else if(address == "/StuntApp/KinectReset")
	{
		type= StuntAppEvents::KINECT_RESET;
	}

	if(type!=-1)
	{
		pendingEvents.push(type);
	}

	oscProcessor.requestQueue.push(message);
	// done with the resource
	oscProcessor.unlock();
	lockQueue=false;

	//	cout<<address << " NOTIFIED EVENT"<<endl;

}