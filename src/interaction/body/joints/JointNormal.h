#pragma once
#ifndef __JOINT_NORMAL__
#define __JOINT_NORMAL__

#include "Joint.h"

class JointNormal : public Joint {
public:
	JointNormal();
	JointNormal(JointType::JointType jointType);
	JointNormal(ofPoint position_, JointType::JointType jointType, TrackingState::TrackingState trackingState);
	~JointNormal(void);
	
	virtual void updatePosition(float dt);
	virtual void updateRotation();
	virtual void setup();
	virtual ofPoint getPosition();
	virtual ofPoint getVelocity();
	ofQuaternion getRotation();
};

#endif