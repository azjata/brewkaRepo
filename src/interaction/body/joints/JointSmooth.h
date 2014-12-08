#pragma once
#ifndef __JOINT_SMOOTH__
#define __JOINT_SMOOTH__

#include "Joint.h"
#include "ofxOpenCv.h"

using namespace cv;

class JointSmooth : public Joint
{
public:
	JointSmooth();
	JointSmooth(JointType::JointType jointType);
	JointSmooth(ofPoint position_, JointType::JointType jointType, TrackingState::TrackingState trackingState);
	
	virtual void updatePosition(float lastUpdateTime);
	virtual void updateRotation();
	virtual void setup();
	virtual ofPoint getPosition();
	ofVec3f getVelocity();
	ofQuaternion getRotation();

	void setParameters(float aLow, float aHigh, float gLow, float gHigh, float velLow, float velHigh, float jThreshold, float jAlpha);

private:
	
	// Alpha, gamma, velocity
	float alphaLow;
	float alphaHigh;
	float gammaLow;
	float gammaHigh;
	float velocityLow;
	float velocityHigh;
	float jitterThreshold;
	float jitterAlpha;

	// joint rotation filtering
	KalmanFilter rotation_filter;
	Mat_<float> rotation_measure_matrix;
	ofQuaternion rotation_prediction;

	// position filtering
	KalmanFilter position_filter;
	Mat_<float> position_measure_matrix;
	ofPoint position_prediction;

	ofVec3f currentX;
	ofVec3f currentb;
	ofVec3f previousb;
	ofVec3f jitterCurrentXhat;
	ofVec3f jitterPreviousXhat;
	ofVec3f currentXhat;
	ofVec3f previousXhat;
	ofVec3f previousPos;
	
	void jitterFilter();
	void doubleExpFilter(float dt);
	void liDoubleExpFilter(float speed);

};

#endif
