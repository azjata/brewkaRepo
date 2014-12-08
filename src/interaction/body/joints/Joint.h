#pragma once
#ifndef __JOINT__
#define __JOINT__

#include "../../enums/JointType.h"
#include "../../enums/TrackingState.h"
#include "ofPoint.h"
#include "ofQuaternion.h"

#include <string>
#include <sstream>
#include <iostream>
#include <memory>

class Joint
{
public:
	Joint();
	Joint(JointType::JointType jointType);
	Joint(ofPoint position_, JointType::JointType jointType, TrackingState::TrackingState trackingState);
	~Joint(void);
	
	virtual void updatePosition(float lastUpdateTime);
	virtual void updateRotation();
	virtual void setup();
	virtual ofPoint getPosition();
	virtual ofVec3f getVelocity();
	virtual ofQuaternion getRotation();

	JointType::JointType getJointType();
	TrackingState::TrackingState getTrackingState();
	void updateJoint(std::shared_ptr<Joint> joint, float dt);

	void setJointType(JointType::JointType jointType);
	void setTrackingState(TrackingState::TrackingState trackingState);
	std::string toString();
	
	friend bool operator== (const Joint &joint1, const Joint &joint2);
    friend bool operator!= (const Joint &joint1, const Joint &joint2);

	void setPosition(float x, float y, float z);
	void setRotation(float angle, float x, float y, float z);
	void setRotation(ofQuaternion rotation);

protected:
	ofPoint position;
	JointType::JointType jointType;
	TrackingState::TrackingState trackingState;
	static const int coordRatio = 1;
	ofQuaternion rotation;

};

#endif
