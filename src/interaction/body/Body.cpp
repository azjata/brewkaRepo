#include "Body.h"

Body::Body() {
}

Body::Body(unsigned long uID) {
	this->id = uID;
}

Body::~Body(void) {}

unsigned long Body::getId() {
	return this->id;
}

void Body::setId(unsigned long id) {
	this->id = id;
}

void Body::updateBody(std::shared_ptr<Body> body) {
	this->id = body->id;
	for (std::shared_ptr<Joint> currJoint : body->joints)  {
		updateJoint(currJoint);
	}
	this->leftHandState = body->leftHandState;
	this->rightHandState = body->rightHandState;

	updateJointRotations();

	lastUpdateTime = ofGetElapsedTimef();
}

void Body::updateJointRotations() {
	
}

void Body::addJoint(std::shared_ptr<Joint> joint) {
	joint->setup();
	joints.push_back(joint);
}

void Body::updateJoint(std::shared_ptr<Joint> joint) {
	for (std::shared_ptr<Joint> currJoint : joints) {
		if (currJoint->getJointType() == joint->getJointType()) {
			currJoint->updateJoint(joint, lastUpdateTime);
			return;
		}
	}
	// if the code reached this line, then this is a new joint
	addJoint(joint);
}

std::vector<std::shared_ptr<Joint> > * Body::getJoints() {
	return & joints;
}

std::shared_ptr<Joint> Body::getJoint(JointType::JointType jointType) {
	for (std::shared_ptr<Joint> currJoint : joints) {
		if (currJoint->getJointType() == jointType) {
			return currJoint;
		}
	}
	return NULL;
}

std::string Body::toString() {
	std::string bodyStr = "Body ";

	std::stringstream ss;
	ss << this->id;

	bodyStr += ss.str() + "\n";

	for (int i=0; i<joints.size(); i++) {
		std::shared_ptr<Joint> currJoint = joints.at(i);
		bodyStr += "\t" + currJoint->toString() + "\n";
	}

	return bodyStr;
}

void Body::setLeftHandState(HandState::HandState handState) {
	this->leftHandState = handState;
}

void Body::setRightHandState(HandState::HandState handState) {
	this->rightHandState = handState;
}

HandState::HandState Body::getLeftHandState() {
	return this->leftHandState;
}
HandState::HandState Body::getRightHandState() {
	return this->rightHandState;
}

bool operator== (const Body &body1, const Body &body2) {
    return (body1.id == body2.id);
}
 
bool operator!= (const Body &body1, const Body &body2) {
    return !(body1 == body2);
}

float Body::angleBetweenVecsInRad(ofVec3f vec1, ofVec3f vec2) {
	ofVec3f crossed = vec1.getCrossed(vec2);
	float angle = atan2(crossed.length(), vec1.dot(vec2));
	if (crossed.y > 0.0f) {
		return -angle;
	}
	return angle;
}

/*----------------------------------------------------------------------------
* Given two vectors v1 and v2, calculate a rotation quaternion so that v1
* would be facing in the same direction as v2.
*----------------------------------------------------------------------------*/
ofQuaternion Body::getRotQuat(ofVec3f v1, ofVec3f v2) {
	v1.normalize();
	v2.normalize();
	ofQuaternion quat;
	quat.makeRotate(v1, v2);
	quat.normalize();
	return quat;
}