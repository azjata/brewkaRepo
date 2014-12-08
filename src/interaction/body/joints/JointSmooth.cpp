#include "JointSmooth.h"
#include "../../../glm/detail/func_noise.hpp"

//lower values of ALPHA and GAMMA will result in more previous values being taken into account and thus result in smoother and laggier movement
//higher values of ALPHA and GAMMA will give higher priority to recent values and thus result in more responsive but jittery movement
//these values can be dynamically changed based on joint velocity/tracking confidence etc.
//http://msdn.microsoft.com/en-us/library/jj131429.aspx#ID4E5GAE1

#ifdef __GNUC__ /// workaround to solve linux compile error (standard library under linux defines gamma)
#define gamma gamma_renamed
#endif

float alpha = 0.30, gamma = 0.40;
float inferredParam = 0.2;
float inferredJitterParam = 0.2;

JointSmooth::JointSmooth() : Joint()
{
}

JointSmooth::JointSmooth(JointType::JointType jointType)
	: Joint(jointType)
{
}

JointSmooth::JointSmooth(ofPoint position_, JointType::JointType jointType, TrackingState::TrackingState trackingState)
	: Joint(position_, jointType, trackingState)
{
	currentXhat = position_;
	currentb = ofVec3f(0.0);

	alphaLow	= 0.1;
	alphaHigh	= 0.3;
	gammaLow	= 0.1;
	gammaHigh	= 0.2;
	velocityLow = 1.0;
	velocityHigh = 4.0;
	jitterThreshold = 0.6;
	jitterAlpha = 0.5;
}

void JointSmooth::setup() {
	/*
	// init rotation filter
	rotation_filter.init(8, 4, 0);
	rotation_filter.transitionMatrix = *(Mat_<float>(8, 8) <<
		1,0,0,1,0,0,0,0,
		0,1,0,0,1,0,0,0,
		0,0,1,0,0,1,0,0,
		0,0,0,1,0,0,1,0,
		0,0,0,0,1,0,0,1,
		0,0,0,0,0,1,0,0,
		0,0,0,0,0,0,1,0,
		0,0,0,0,0,0,0,1);
	rotation_filter.statePre.at<float>(0) = 0;
	rotation_filter.statePre.at<float>(1) = 0;
	rotation_filter.statePre.at<float>(2) = 0;
	rotation_filter.statePre.at<float>(3) = 0;
	rotation_filter.statePre.at<float>(4) = 0;
	rotation_filter.statePre.at<float>(5) = 0;
	rotation_filter.statePre.at<float>(6) = 0;
	rotation_filter.statePre.at<float>(7) = 0;
	setIdentity(rotation_filter.measurementMatrix);
	setIdentity(rotation_filter.processNoiseCov, Scalar::all(1e-5));
	setIdentity(rotation_filter.measurementNoiseCov, Scalar::all(1e-1));
	setIdentity(rotation_filter.errorCovPost, Scalar::all(.1));
	rotation_measure_matrix = Mat_<float>::zeros(4, 1);
	*/
	// init position filter
	/*position_filter.init(6, 3, 0);
	position_filter.transitionMatrix = *(Mat_<float>(6, 6) <<
		1,0,0,1,0,0,
		0,1,0,0,1,0,
		0,0,1,0,0,1,
		0,0,0,1,0,0,
		0,0,0,0,1,0,
		0,0,0,0,0,1);
	position_filter.statePre.at<float>(0) = 0;
	position_filter.statePre.at<float>(1) = 0;
	position_filter.statePre.at<float>(2) = 0;
	position_filter.statePre.at<float>(3) = 0;
	position_filter.statePre.at<float>(4) = 0;
	position_filter.statePre.at<float>(5) = 0;
	setIdentity(position_filter.measurementMatrix);
	setIdentity(position_filter.processNoiseCov, Scalar::all(1e-4));
	setIdentity(position_filter.measurementNoiseCov, Scalar::all(1e-1));
	setIdentity(position_filter.errorCovPost, Scalar::all(.1));
	position_measure_matrix = Mat_<float>::zeros(3, 1);*/
	
}
void JointSmooth::setParameters(float aLow, float aHigh, float gLow, float gHigh, float velLow, float velHigh, float jThreshold, float jAlpha)
{
	//alpha = aLow;
	//gamma = gLow;
	alphaLow	= aLow;
	alphaHigh	= aHigh;
	gammaLow	= gLow;
	gammaHigh	= gHigh;
	velocityLow = velLow;
	velocityHigh = velHigh;
	jitterThreshold = jThreshold;
	jitterAlpha = jAlpha;
}

void JointSmooth::updatePosition(float lastUpdateTime) { //updates smoothed position based on most recent position readings
	// adaptive double exponential smoothing filter
	//http://msdn.microsoft.com/en-us/library/jj131429.aspx#ID4E5GAE1

	previousPos = currentX;
	currentX = position;
	previousXhat = currentXhat;
	previousb = currentb;

	float dt = ofGetElapsedTimef() - lastUpdateTime;

	jitterFilter();

	doubleExpFilter(dt);
	
	// update position with kalman filter
	//
	// First predict, to update the internal statePre variable
	/*Mat prediction_mat_position = position_filter.predict();
	position_prediction.x = prediction_mat_position.at<float>(0);
	position_prediction.y = prediction_mat_position.at<float>(1);
	position_prediction.z = prediction_mat_position.at<float>(2);

	// update position
	position_measure_matrix(0) = position.x * coordRatio;
	position_measure_matrix(1) = position.y * coordRatio;
	position_measure_matrix(2) = position.z * coordRatio;

	// The "correct" phase that is going to use the predicted value and our measurement
	position_filter.correct(position_measure_matrix);*/
	
}

void JointSmooth::updateRotation() {

	// update direction with kalman filter

	// First predict, to update the internal statePre variable
	/*Mat prediction_mat_rotation = rotation_filter.predict();
	rotation_prediction = ofQuaternion(prediction_mat_rotation.at<float>(0),
		prediction_mat_rotation.at<float>(1),
		prediction_mat_rotation.at<float>(2),
		prediction_mat_rotation.at<float>(3));

	// update rotation
	rotation_measure_matrix(0) = rotation.x();
	rotation_measure_matrix(1) = rotation.y();
	rotation_measure_matrix(2) = rotation.z();
	rotation_measure_matrix(3) = rotation.w();

	// The "correct" phase that is going to use the predicted value and our measurement
	rotation_filter.correct(rotation_measure_matrix);
	*/
}

ofPoint JointSmooth::getPosition() {
	//return position_prediction;
	return currentXhat;
}

ofVec3f JointSmooth::getVelocity() {
	return currentb;
}

ofQuaternion JointSmooth::getRotation() {
	//return rotation_prediction;
	return rotation;
}

void JointSmooth::jitterFilter()
{
	float length = currentX.distance(previousXhat);
	float jAlpha = jitterAlpha;

	if(trackingState == TrackingState::Inferred)
	{
		jitterAlpha = inferredJitterParam;
	}

	if(length < jitterThreshold)
		jitterCurrentXhat = currentX;
	else
		jitterCurrentXhat = jAlpha * currentX + (1.0f - jAlpha) * jitterPreviousXhat;

	jitterPreviousXhat = jitterCurrentXhat;
}

void JointSmooth::doubleExpFilter(float dt) {
	currentXhat = alpha * jitterCurrentXhat + (1.0f - alpha) * (previousXhat + previousb * dt);
	currentb = gamma * ((currentXhat - previousXhat) / dt) + (1.0f - gamma) * previousb;
}
void JointSmooth::liDoubleExpFilter(float dt)
{
	float speed = currentX.distance(previousPos) / dt;

	if(speed < velocityLow)
	{
		alpha = alphaLow;
		gamma = gammaLow;
	}
	else if(speed <= velocityHigh)
	{
		float k = (speed - velocityHigh)/(velocityLow - velocityHigh);

		alpha = alphaHigh + k * (alphaLow - alphaHigh);
		gamma = gammaHigh + k * (gammaLow - gammaHigh);
	}
	else
	{
		alpha = alphaHigh;
		gamma = gammaHigh;
	}

	if(trackingState == TrackingState::Inferred)
	{
		alpha -= inferredParam;
		gamma -= inferredParam;
	}

	//alpha = 0.1; gamma = 0.3;
	currentXhat = alpha * jitterCurrentXhat + (1.0f - alpha) * (previousXhat + previousb * dt);
	currentb = gamma * ((currentXhat - previousXhat) / dt) + (1.0f - gamma) * previousb;
}
