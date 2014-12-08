#include "OSCManager.h"

void OSCManager::setup(std::string host, int port, bool hardware) {
	try {
		if(!hardware)
		{
			sender.setup(host, port);
			setupComplete = true;
		}
		else
		{
			senderHardware.setup(host,port);
			setupHardwareComplete=true;
		}
	} catch (...) {
		setupHardwareComplete = setupComplete = false;
	}
}

void OSCManager::setupStunt(std::string host,int port)
{
	try
	{
		senderStatus.setup(host,port);
		setupStatusComplete = true;
	}
	catch(...)
	{
		setupStatusComplete = false;
	}

}

void OSCManager::startWorld(COMMON::WORLD_TYPE world) {
	if(!checkSetup()) return;
	switch (world) {
	case COMMON::WORLD_TYPE::INTRO:
		sendStatus("INTRO READY");
		break;
	case COMMON::WORLD_TYPE::ULTRAMARINE: // pineapple
		sendStatus("ULTRAMARINE STARTED\nLIGHTS ON \nSCENT ON");
		sendMessage(OSCMessageBuilder::BuildCamServerMessage(CAM_SERVER::CAMSERVER_COMMANDS::START),false);
		sendMessage(OSCMessageBuilder::BuildFilmMessage(true,0,-1,1), true);
		sendMessage(OSCMessageBuilder::BuildScentMessage(R_PI::SCENTCONTROL_COMMANDS::FAN1,		WORLD_ULTRAMARINE::DURATION_INTRO-2,	15, 1),true);

		//sendMessage(OSCMessageBuilder::BuildScentMessage(R_PI::SCENTCONTROL_COMMANDS::SCENT1,	WORLD_ULTRAMARINE::DURATION_INTRO-5,	10, 0.30),true);
		break;
	case COMMON::WORLD_TYPE::ELECTRIC:
		sendStatus("ELECTRO STARTED\nTOP FANS ON\n");
		setTopFanSpeed(1,.1);
		//sendMessage(OSCMessageBuilder::BuildScentMessage(R_PI::SCENTCONTROL_C OMMANDS::FAN1, 0, -1, 0.85));
		//sendMessage(OSCMessageBuilder::BuildScentMessage(R_PI::SCENTCONTROL_COMMANDS::SCENT1, 4, WORLD_ELECTRIC::DURATION_INTRO, 0.3));
		break;
	case COMMON::WORLD_TYPE::EVOLUTION:
		//sendStatus("EVOLUTION STARTED\nTOP FANS ON + SCENT");
		setFrontFanSpeed(1,.1);
		sendMessage(OSCMessageBuilder::BuildScentMessage(R_PI::SCENTCONTROL_COMMANDS::FAN2,		0,	15,	1),true);
		//sendMessage(OSCMessageBuilder::BuildScentMessage(R_PI::SCENTCONTROL_COMMANDS::FAN1, 0, -1, 0.85));
		//sendMessage(OSCMessageBuilder::BuildScentMessage(R_PI::SCENTCONTROL_COMMANDS::SCENT1, 3, WORLD_EVOLUTION::DURATION_INTRO, 0.3));
		break;
	case COMMON::WORLD_TYPE::LIQUID: // spearmint
		sendStatus("TURBULENCE STARTED\nFRONT FAN ON");
		setFrontFanSpeed(1, WORLD_LIQUID::DURATION_MAIN);
		//sendMessage(OSCMessageBuilder::BuildScentMessage(R_PI::SCENTCONTROL_COMMANDS::SCENT2,	0,	10,	0.30));
		break;
	}
}

void OSCManager::endWorld(COMMON::WORLD_TYPE world) {
	if(!checkSetup()) return;
	switch (world) {
	case COMMON::WORLD_TYPE::ULTRAMARINE:
		setFrontFanSpeed(0);
		break;
	case COMMON::WORLD_TYPE::ELECTRIC:
		setTopFanSpeed(0,0.1);
		break;
	case COMMON::WORLD_TYPE::EVOLUTION:
		setFrontFanSpeed(0);
		break;
	case COMMON::WORLD_TYPE::LIQUID:
		//	setFrontFanSpeed(0);
		sendStatus("EXPERIENCE FINISHED\nLIFT DOWN USER\nRESTART WHEN DONE");
		sendMessage(OSCMessageBuilder::BuildCamServerMessage(CAM_SERVER::CAMSERVER_COMMANDS::END),false);
		sendMessage(OSCMessageBuilder::BuildFilmMessage(false,5,-1,0),true);
		reset();
		break;
	}
}

void OSCManager::sendMarkToRecordingStation() {
	// TODO: send message to trigger the recording station
}

void OSCManager::sendStatus(string status)
{
	if(setupStatusComplete)
	{
		ofxOscMessage m;
		m.setAddress("/StuntApp/Status");
		m.addStringArg(status);
		sendingStatusQueue.push(m);
	}
	else
	{
		cout<<"ERROR STUNT APP SETUP NOT COMPLETE"<<endl;
	}

}

void OSCManager::setFilmLights(bool state)
{
	sendMessage(OSCMessageBuilder::BuildFilmMessage(true,0,-1,state ? 1 : 0),true);
}

void OSCManager::setFrontFanSpeed(int speed, float delay,float duration) {
	if (speed < 0 || speed > 2) {
		cout << "OSCManager::setFrontFanSpeed: invalid speed!" << endl;
		return;
	}
	sendMessage(OSCMessageBuilder::BuildFanMessage(R_PI::FANCONTROL_COMMANDS::FRONT_FAN, delay, duration, speed),true);
}

void OSCManager::reset() {
	sendMessage(OSCMessageBuilder::BuildResetMessage(),true);
}

void OSCManager::setTopFanSpeed(int speed,float delay,float duration) {
	if (speed < 0 || speed > 1) {
		cout << "OSCManager::setTopFanSpeed: invalid speed!" << endl;
		return;
	}
	sendMessage(OSCMessageBuilder::BuildFanMessage(R_PI::FANCONTROL_COMMANDS::CEILING_FAN, delay, duration, speed),true);
}

void OSCManager::update(float currentTime) {
	// if enough time has passed and the sending queue is not empty
	if ((currentTime-timeLastMsg > 0.25) && !sendingQueue.empty()) {
		sender.sendMessage(sendingQueue.front());
		sendingQueue.pop();
		timeLastMsg = currentTime;
	}

	if ((currentTime-timeLastMsg > 0.25) && !sendingHardwareQueue.empty()) {
		senderHardware.sendMessage(sendingHardwareQueue.front());
		sendingHardwareQueue.pop();
		timeLastMsg = currentTime;
	}
	if ((currentTime-lastStatusMsgTime > 0.25) && !sendingStatusQueue.empty()) {
		senderStatus.sendMessage(sendingStatusQueue.front());
		sendingStatusQueue.pop();
		lastStatusMsgTime = currentTime;
	}
		
}

void OSCManager::sendMessage(ofxOscMessage message, bool hardware) {
	if (OSC_REMOTE::SEND_OSC_MESSAGES) {
		cout << "OSC - " << "endpoint: " << message.getAddress() << endl;
	} else {
		cout << "OSC - Don't send" << endl;
	}
	if(!hardware)
		sendingQueue.push(message);
	else
		sendingHardwareQueue.push(message);
}

bool OSCManager::checkSetup() {
	if(!setupComplete) {
		cout << "OSC - Did you forget to call setup?" << endl;
		return false;
	}
	return true;
}