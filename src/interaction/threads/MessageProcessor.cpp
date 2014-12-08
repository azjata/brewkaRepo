#include "MessageProcessor.h"


MessageProcessor::MessageProcessor(void)
{
}


MessageProcessor::~MessageProcessor(void)
{
}

void MessageProcessor::threadedFunction() {

	while(isThreadRunning()) {
		if (!this->requestQueue.empty()) {
			std::string msg = this->requestQueue.front();
			// process bodies
			std::size_t userDelimiter = msg.find("|");
			std::size_t lastPos = 0;
			vector<std::shared_ptr<Body> > bodies;
			while (userDelimiter != std::string::npos) {
				string bodyMsg = msg.substr(lastPos, userDelimiter);
				std::shared_ptr<Body> body = BodyFactory::buildBody(bodyMsg);
				bodies.push_back(body);
				lastPos = userDelimiter+1;
				userDelimiter = msg.find("|", userDelimiter+1);
			}
			UserManager::getInstance().filterUsers(bodies);
			this->requestQueue.pop();
		}
    }
	cout << "MessageProcessor::threadedFunction - message processor terminated..." << endl;

}