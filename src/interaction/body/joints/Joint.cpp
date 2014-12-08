#include "Joint.h"

Joint::Joint() {
}

Joint::Joint(JointType::JointType jointType) {
	this->jointType = jointType;
}

Joint::Joint(ofPoint position_, JointType::JointType jointType, TrackingState::TrackingState trackingState) {
	position = position_;
	this->jointType = jointType;
	this->trackingState = trackingState;
}

Joint::~Joint(void) {
}

JointType::JointType Joint::getJointType() {
	return this->jointType;
}

TrackingState::TrackingState Joint::getTrackingState() {
	return this->trackingState;
}

void Joint::setPosition(float x, float y, float z) {
	position.x = x;
	position.y = y; 
	position.z = z;
}

void Joint::setRotation(ofQuaternion rot) {
	rotation = rot;
}

void Joint::setRotation(float angle, float x, float y, float z) {
	rotation = ofQuaternion(angle, x, y, z);
}

void Joint::updateJoint(std::shared_ptr<Joint> joint, float dt) {
	jointType = joint->jointType;
	trackingState = joint->trackingState;
	position = joint->position;
	rotation = joint->rotation;
	updatePosition(dt);
	// rotation will be updated later
}

std::string Joint::toString() {
	std::string jointStr = "";
	switch (this->jointType) {
	case JointType::SpineBase:
		jointStr+="SpineBase";
		break;
	case JointType::SpineMid:
		jointStr+="SpineMid";
		break;
	case JointType::Neck:
		jointStr+="Neck";
		break;
	case JointType::Head:
		jointStr+="Head";
		break;
	case JointType::ShoulderLeft:
		jointStr+="ShoulderLeft";
		break;
	case JointType::ElbowLeft:
		jointStr+="ElbowLeft";
		break;
	case JointType::WristLeft:
		jointStr+="WristLeft";
		break;
	case JointType::HandLeft:
		jointStr+="HandLeft";
		break;
	case JointType::ShoulderRight:
		jointStr+="ShoulderRight";
		break;
	case JointType::ElbowRight:
		jointStr+="ElbowRight";
		break;
	case JointType::WristRight:
		jointStr+="WristRight";
		break;
	case JointType::HandRight:
		jointStr+="WristRight";
		break;
	case JointType::HipLeft:
		jointStr+="HipLeft";
		break;
	case JointType::KneeLeft:
		jointStr+="KneeLeft";
		break;
	case JointType::AnkleLeft:
		jointStr+="AnkleLeft";
		break;
	case JointType::FootLeft:
		jointStr+="FootLeft";
		break;
	case JointType::HipRight:
		jointStr+="HipRight";
		break;
	case JointType::KneeRight:
		jointStr+="KneeRight";
		break;
	case JointType::AnkleRight:
		jointStr+="AnkleRight";
		break;
	case JointType::FootRight:
		jointStr+="FootRight";
		break;
	case JointType::SpineShoulder:
		jointStr+="SpineShoulder";
		break;
	case JointType::HandTipLeft:
		jointStr+="HandTipLeft";
		break;
	case JointType::ThumbLeft:
		jointStr+="ThumbLeft";
		break;
	case JointType::HandTipRight:
		jointStr+="HandTipRight";
		break;
	case JointType::ThumbRight:
		jointStr+="ThumbRight";
		break;
	default:
		jointStr+="Uknwown";
		break;
	}

	jointStr+=" : ";

	switch (this->trackingState) {
	case TrackingState::Inferred:
		jointStr+="Inferred";
		break;
	case TrackingState::NotTracked:
		jointStr+="NotTracked";
		break;
	case TrackingState::Tracked:
		jointStr+="Tracked";
		break;
	default:
		jointStr+="Uknwown";
		break;
	}

	jointStr+=" - ";
	
	std::ostringstream buffX;
    buffX << this->position.x;
	std::ostringstream buffY;
    buffY << this->position.y;
	std::ostringstream buffZ;
    buffZ << this->position.z;

	jointStr+=buffX.str()+"; "+buffY.str()+"; "+buffZ.str();

	return jointStr;
}

bool operator== (const Joint &joint1, const Joint &joint2) {
    return (joint1.jointType == joint2.jointType);
}
 
bool operator!= (const Joint &joint1, const Joint &joint2) {
    return !(joint1 == joint2);
}

void Joint::updatePosition(float lastUpdateTime) {
}

void Joint::updateRotation() {
}

void Joint::setup() {
}

ofPoint Joint::getPosition() {
	return position;
}

ofVec3f Joint::getVelocity() {
	return ofVec3f(0.0, 0.0, 0.0);
}

ofQuaternion Joint::getRotation() {
	return rotation;
}
