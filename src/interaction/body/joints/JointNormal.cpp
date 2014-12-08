#include "JointNormal.h"

JointNormal::JointNormal() : Joint() {
}

JointNormal::JointNormal(JointType::JointType jointType)
	: Joint(jointType) {

}

JointNormal::JointNormal(ofPoint position_, JointType::JointType jointType, TrackingState::TrackingState trackingState)
	: Joint(position_, jointType, trackingState) {

}

JointNormal::~JointNormal(void) {
}

void JointNormal::updatePosition(float dt) {

}

void JointNormal::updateRotation() {

}
void JointNormal::setup() {

}

ofPoint JointNormal::getPosition() {
	return position * coordRatio;
}

ofQuaternion JointNormal::getRotation() {
	return rotation;
}

ofVec3f JointNormal::getVelocity() {
	return ofVec3f(0.0, 0.0, 0.0);
}
