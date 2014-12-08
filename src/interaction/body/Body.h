#pragma once
#ifndef __BODY__
#define __BODY__

#include "joints/Joint.h"
#include "joints/JointSmooth.h"
#include "joints/JointNormal.h"
#include "../enums/HandState.h"
#include "../../utils.h"
#include "ofMatrix4x4.h"
#include <vector>
#include <sstream>
#include <memory>

class Body {
public:
	Body();
	Body(unsigned long id);
	~Body(void);
	unsigned long getId();
	void setId(unsigned long id);
	void updateBody(std::shared_ptr<Body> body);
	void addJoint(std::shared_ptr<Joint> joint);
	void updateJoint(std::shared_ptr<Joint> joint);
	std::shared_ptr<Joint> getJoint(JointType::JointType jointType);
	void setLeftHandState(HandState::HandState handState);
	void setRightHandState(HandState::HandState handState);
	HandState::HandState getLeftHandState();
	HandState::HandState getRightHandState();
	std::vector<std::shared_ptr<Joint> > * getJoints();
	std::string toString();
	friend bool operator== (const Body &body1, const Body &body2);
    friend bool operator!= (const Body &body1, const Body &body2);

private:
	float lastUpdateTime;
	unsigned long id;

	std::vector<std::shared_ptr<Joint> > joints;
	HandState::HandState leftHandState;
	HandState::HandState rightHandState;
	void updateJointRotations();
	float angleBetweenVecsInRad(ofVec3f vec1, ofVec3f vec2);
	ofQuaternion getRotQuat(ofVec3f v1, ofVec3f v2);
};

#endif