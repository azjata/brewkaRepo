#include "BodyFactory.h"

std::shared_ptr<Body> BodyFactory::buildBody(std::string bodyData) {
	std::shared_ptr<Body> body(new Body());

	// message format:
    // trackedID1;leftHandState;rightHandState;jointName;trackingState;posX;posY;posZ;jointName$trackingState;posX;posY;posZ$jointName;trackingState;posX;posY;posZ...|
    // trackedID2;leftHandState;rightHandState;jointName;trackingState;posX;posY;posZ;jointName$trackingState;posX;posY;posZ$jointName;trackingState;posX;posY;posZ...|

	int oldSemicolonPos = 0;
	int nextSemicolonPos = 0;
	
	// get user id
	nextSemicolonPos = bodyData.find(";");
	std::string userIdStr = bodyData.substr(oldSemicolonPos, nextSemicolonPos-oldSemicolonPos);
	oldSemicolonPos = nextSemicolonPos+1;

	// convert to ulong
	unsigned long long id;
	std::istringstream reader(userIdStr);
	reader >> id;

	// set body id
	body->setId(id);
	
	// get left hand state
	nextSemicolonPos = bodyData.find(";", oldSemicolonPos);
	std::string leftHandStateStr = bodyData.substr(oldSemicolonPos, nextSemicolonPos-oldSemicolonPos);
	oldSemicolonPos = nextSemicolonPos+1;

	// convert to HandState
	HandState::HandState leftHandState = (HandState::HandState) atoi(leftHandStateStr.c_str());
	
	// set left hand state
	body->setLeftHandState(leftHandState);
	
	// get right hand state
	nextSemicolonPos = bodyData.find(";", oldSemicolonPos);
	std::string rightHandStateStr = bodyData.substr(oldSemicolonPos, nextSemicolonPos-oldSemicolonPos);
	oldSemicolonPos = nextSemicolonPos + 1;;

	// convert to HandState
	HandState::HandState rightHandState = (HandState::HandState) atoi(rightHandStateStr.c_str());
	
	// set right hand state
	body->setRightHandState(rightHandState);
	
	// from now on it's just joints
	std::string currJointStr;
	do {
		nextSemicolonPos = bodyData.find("$", oldSemicolonPos);
		currJointStr = bodyData.substr(oldSemicolonPos, nextSemicolonPos - oldSemicolonPos);
		if (currJointStr.length() > 0) {
			std::shared_ptr<Joint> joint = parseJoint(currJointStr);
			body->addJoint(joint);
		}
		oldSemicolonPos = nextSemicolonPos + 1;
	} while (std::string::npos != nextSemicolonPos);

	return body;
}

std::shared_ptr<Joint> BodyFactory::parseJoint(std::string jointData) {

	int nextSemicolonPos = 0;
	int oldSemicolonPos = 0;

	// get joint type
	nextSemicolonPos = jointData.find(";", oldSemicolonPos);
	std::string currJointTypeStr = jointData.substr(oldSemicolonPos, nextSemicolonPos-oldSemicolonPos);
	JointType::JointType currJointType = (JointType::JointType)atoi(currJointTypeStr.c_str());
	oldSemicolonPos = nextSemicolonPos + 1;

	// get tracking state
	nextSemicolonPos = jointData.find(";", oldSemicolonPos);
	std::string currTrackingStateStr = jointData.substr(oldSemicolonPos, nextSemicolonPos-oldSemicolonPos);
	TrackingState::TrackingState currTrackingState = (TrackingState::TrackingState)atoi(currTrackingStateStr.c_str());
	oldSemicolonPos = nextSemicolonPos + 1;

	// get pos x
	nextSemicolonPos = jointData.find(";", oldSemicolonPos);
	std::string currPosXStr = jointData.substr(oldSemicolonPos, nextSemicolonPos-oldSemicolonPos);
	float posX = (float) atof(currPosXStr.c_str());
	oldSemicolonPos = nextSemicolonPos + 1;

	// get pos y
	nextSemicolonPos = jointData.find(";", oldSemicolonPos);
	std::string currPosYStr = jointData.substr(oldSemicolonPos, nextSemicolonPos-oldSemicolonPos);
	float posY = (float) atof(currPosYStr.c_str());
	oldSemicolonPos = nextSemicolonPos + 1;

	// get pos z
	nextSemicolonPos = jointData.find(";", oldSemicolonPos);
	std::string currPosZStr = jointData.substr(oldSemicolonPos, nextSemicolonPos-oldSemicolonPos);
	float posZ = (float) atof(currPosZStr.c_str());
	oldSemicolonPos = nextSemicolonPos + 1;

	switch(currJointType) {
		case JointType::ShoulderLeft:
		case JointType::ShoulderRight:
		case JointType::ElbowLeft:
		case JointType::ElbowRight:
		case JointType::WristLeft:
		case JointType::WristRight:
		case JointType::HandLeft:
		case JointType::HandRight:
		case JointType::ThumbLeft:
		case JointType::ThumbRight:
		case JointType::HandTipLeft:
		case JointType::HandTipRight:
		case JointType::Head:
			return std::shared_ptr<Joint>(new JointSmooth(ofPoint(posX, posY, posZ), currJointType, currTrackingState));
			break;
		default:
			break;
	}

	return std::shared_ptr<Joint>(new JointNormal(ofPoint(posX, posY, posZ), currJointType, currTrackingState));
}