#include "NetworkConsumer.h"

NetworkConsumer::NetworkConsumer() {	
	closed = false;
}

NetworkConsumer::~NetworkConsumer(void) {
}

void NetworkConsumer::setup(char * address, int port) {
	udpConnection.Create();
    udpConnection.BindMcast(address, port);
	
	//start message processor thread
	messageProcessor.startThread();
}
void NetworkConsumer::close() {
	closed = true;
	udpConnection.Close();
}

void NetworkConsumer::threadedFunction() {
	while(isThreadRunning() && !closed) {
		// get new message
		const int nrBytes = 100000;
		char udpMessage[nrBytes];
		udpConnection.Receive(udpMessage, nrBytes);
		string message = udpMessage;
		if (!message.empty()) {
			processNetworkMsg(message);
		}
    }
	cout << "NetworkConsumer::threadedFunction - stopping message processor thread (" << messageProcessor.getThreadId() << ")..." << endl;
	// stop the message processor thread
	messageProcessor.waitForThread(true);
}



void NetworkConsumer::processNetworkMsg(string message) {
    // push message to network cosumer's queue
	messageProcessor.requestQueue.push(message);
}