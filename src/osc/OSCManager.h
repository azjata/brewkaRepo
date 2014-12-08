#pragma once
#ifndef __OSC_MANAGER__
#define __OSC_MANAGER__

#include "ofxOscSender.h"
#include "../constants.h"
#include "OscMessageBuilder.h"
#include <mutex>

class OSCManager {
public:
	static OSCManager& getInstance() {
		static OSCManager instance;
		return instance;
	}
	void setup(std::string host, int port, bool hardware);
	void setupStunt(std::string host,int port);
	void update(float currentTime);
	void reset();
	void startWorld(COMMON::WORLD_TYPE world);
	void endWorld(COMMON::WORLD_TYPE world);

	// Stewardapp send app status
	void sendStatus(string status);
	/*
	Send keyframe to the recording station
	*/
	void sendMarkToRecordingStation();
	/*
	Sets the speed of the front fan
	*/
	void setFrontFanSpeed(int speed,float delay=0, float duration=-1);
	/*
	Sets the speed of the top fan
	*/
	void setTopFanSpeed(int speed,float delay=0, float duration = -1);

	void setFilmLights(bool state);
private:
	OSCManager(): timeLastMsg(0.0f), lastStatusMsgTime(0.0f) {
		setupStatusComplete = setupHardwareComplete =	setupComplete = false;

	};
	OSCManager(OSCManager const&);
	void operator=(OSCManager const&);
	ofxOscSender sender;
	ofxOscSender senderHardware;
	ofxOscSender senderStatus;
	bool setupComplete;
	bool setupHardwareComplete;
	bool setupStatusComplete;

	void sendMessage(ofxOscMessage message, bool hardware);
	bool checkSetup();
	queue<ofxOscMessage> sendingQueue;
	queue<ofxOscMessage> sendingHardwareQueue;
	queue<ofxOscMessage> sendingStatusQueue;
	float timeLastMsg;
	float lastStatusMsgTime;

};

#endif
