#pragma once

#ifndef __KINECT_ENGINE_H__
#define __KINECT_ENGINE_H__

#include "threads/NetworkConsumer.h"

#include "render_helper/SkinnedBody.h"

#include "../bloom.h"

class KinectEngine {
public:
	KinectEngine(void);
	~KinectEngine(void);

	void setup(char * ip, int port);
	void update(float dt, float bodyRotationAngle);
	void exit();
	
	//TODO: make this more abstract
	void drawBodyBlack(ofVec3f offset);
	void drawBodyUltramarine(ofVec3f cameraPosition, float opacity, ofVec3f leftColor = ofVec3f(1.0, 1.0, 1.0), ofVec3f rightColor = ofVec3f(1.0, 1.0, 1.0));
	void drawBodyElectric(ofVec3f cameraPosition, float opacity, vector<float>& lightPositions, vector<float>& lightColors, vector<float>& lightAttenuations, float leftEnergy, float rightEnergy);
	void drawBodyEvolution(ofVec3f cameraPosition, float opacity);
	void drawBodyLiquid(ofVec3f cameraPosition, float opacity);

	vector<ofPoint> getHandsPosition(); /// vector with the position of the hands
	vector<ofVec3f> getHandsDirection(); /// vector with the direction of the hands
	vector<HandState::HandState> getHandsState();/// vector with the state of the hands
	bool isLeftHandOpen();/// returns if the left hand is opened (true) or closed (false)
	bool isRightHandOpen();/// returns if the left hand is opened (true) or closed (false)

	bool isKinectDetected();

	SkinnedBody* getSkinnedBody();

private:
	char * ip;
	int port;
	NetworkConsumer networkConsumer;

	float leftHandAnimationTime;
	float rightHandAnimationTime;

	void setupForRendering ();

	void updateBody(float dt, float bodyRotationAngle);

	// hands variables for faster access
	// common
	HandState::HandState handsState[2];
	ofPoint headPos;
	// left hand
	ofPoint leftHandPosition;
	ofVec3f leftHandDirection;
	// right hand
	ofPoint rightHandPosition;
	ofVec3f rightHandDirection;

	bool isBonesScaling;

	SkinnedBody skinnedBody;

	ofVbo bodyTrianglesVbo;
	ofVbo leftBodyParticlesVbo;
	ofVbo rightBodyParticlesVbo;

	StrongPtr<Texture> environment;

};

#endif
