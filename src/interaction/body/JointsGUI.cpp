#include "JointsGUI.h"

float RIGHT_HAND_ALPHA_MIN = 0.1;
float RIGHT_HAND_ALPHA_MAX = 0.3;
float RIGHT_HAND_GAMMA_MIN = 0.2;
float RIGHT_HAND_GAMMA_MAX = 0.3;
float RIGHT_HAND_VELOCITY_MIN = 1.0;
float RIGHT_HAND_VELOCITY_MAX = 4.0;

float RIGHT_WRIST_ALPHA_MIN = 0.1;
float RIGHT_WRIST_ALPHA_MAX = 0.3;
float RIGHT_WRIST_GAMMA_MIN = 0.2;
float RIGHT_WRIST_GAMMA_MAX = 0.3;
float RIGHT_WRIST_VELOCITY_MIN = 1.0;
float RIGHT_WRIST_VELOCITY_MAX = 4.0;

float RIGHT_ELBOW_ALPHA_MIN = 0.1;
float RIGHT_ELBOW_ALPHA_MAX = 0.3;
float RIGHT_ELBOW_GAMMA_MIN = 0.2;
float RIGHT_ELBOW_GAMMA_MAX = 0.3;
float RIGHT_ELBOW_VELOCITY_MIN = 1.0;
float RIGHT_ELBOW_VELOCITY_MAX = 4.0;

float RIGHT_SHOULDER_ALPHA_MIN = 0.1;
float RIGHT_SHOULDER_ALPHA_MAX = 0.3;
float RIGHT_SHOULDER_GAMMA_MIN = 0.2;
float RIGHT_SHOULDER_GAMMA_MAX = 0.3;
float RIGHT_SHOULDER_VELOCITY_MIN = 1.0;
float RIGHT_SHOULDER_VELOCITY_MAX = 4.0;

float JITTER_THRESHOLD = 0.3;
float JITTER_ALPHA = 0.1;

JointsGUI::JointsGUI()
{
}

JointsGUI::~JointsGUI()
{
}

void JointsGUI::updateJoints()
{
	UserManager::getInstance().lock();

	std::shared_ptr<Body> user = UserManager::getInstance().getActiveUser();
	if(user == NULL) {
		UserManager::getInstance().unlock();
		return;
	}
	
	std::vector<std::shared_ptr<Joint> > * joints = user->getJoints();

	for (int i = 0; i < joints->size(); i++) 
	{
		JointSmooth * currJoint = static_cast<JointSmooth*>( joints->at(i).get());
		
		switch(currJoint->getJointType())
		{
		case JointType::HandLeft:
			currJoint->setParameters(RIGHT_HAND_ALPHA_MIN, RIGHT_HAND_ALPHA_MAX, RIGHT_HAND_GAMMA_MIN, RIGHT_HAND_GAMMA_MAX, RIGHT_HAND_VELOCITY_MIN, RIGHT_HAND_VELOCITY_MAX, JITTER_THRESHOLD, JITTER_ALPHA);
			break;

		case JointType::WristLeft:
			currJoint->setParameters(RIGHT_WRIST_ALPHA_MIN, RIGHT_WRIST_ALPHA_MAX, RIGHT_WRIST_GAMMA_MIN, RIGHT_WRIST_GAMMA_MAX, RIGHT_WRIST_VELOCITY_MIN, RIGHT_WRIST_VELOCITY_MAX, JITTER_THRESHOLD, JITTER_ALPHA);
			break;
			
		case JointType::ElbowLeft:
			currJoint->setParameters(RIGHT_ELBOW_ALPHA_MIN, RIGHT_ELBOW_ALPHA_MAX, RIGHT_ELBOW_GAMMA_MIN, RIGHT_ELBOW_GAMMA_MAX, RIGHT_ELBOW_VELOCITY_MIN, RIGHT_ELBOW_VELOCITY_MAX, JITTER_THRESHOLD, JITTER_ALPHA);
			break;

		case JointType::ShoulderLeft:
			currJoint->setParameters(RIGHT_SHOULDER_ALPHA_MIN, RIGHT_SHOULDER_ALPHA_MAX, RIGHT_SHOULDER_GAMMA_MIN, RIGHT_SHOULDER_GAMMA_MAX, RIGHT_SHOULDER_VELOCITY_MIN, RIGHT_SHOULDER_VELOCITY_MAX, JITTER_THRESHOLD, JITTER_ALPHA);
			break;

		case JointType::HandRight:
			currJoint->setParameters(RIGHT_HAND_ALPHA_MIN, RIGHT_HAND_ALPHA_MAX, RIGHT_HAND_GAMMA_MIN, RIGHT_HAND_GAMMA_MAX, RIGHT_HAND_VELOCITY_MIN, RIGHT_HAND_VELOCITY_MAX, JITTER_THRESHOLD, JITTER_ALPHA);
			break;

		case JointType::ElbowRight:
			currJoint->setParameters(RIGHT_ELBOW_ALPHA_MIN, RIGHT_ELBOW_ALPHA_MAX, RIGHT_ELBOW_GAMMA_MIN, RIGHT_ELBOW_GAMMA_MAX, RIGHT_ELBOW_VELOCITY_MIN, RIGHT_ELBOW_VELOCITY_MAX, JITTER_THRESHOLD, JITTER_ALPHA);
			break;

		case JointType::WristRight:
			currJoint->setParameters(RIGHT_WRIST_ALPHA_MIN, RIGHT_WRIST_ALPHA_MAX, RIGHT_WRIST_GAMMA_MIN, RIGHT_WRIST_GAMMA_MAX, RIGHT_WRIST_VELOCITY_MIN, RIGHT_WRIST_VELOCITY_MAX, JITTER_THRESHOLD, JITTER_ALPHA);
			break;

		case JointType::ShoulderRight:
			currJoint->setParameters(RIGHT_SHOULDER_ALPHA_MIN, RIGHT_SHOULDER_ALPHA_MAX, RIGHT_SHOULDER_GAMMA_MIN, RIGHT_SHOULDER_GAMMA_MAX, RIGHT_SHOULDER_VELOCITY_MIN, RIGHT_SHOULDER_VELOCITY_MAX, JITTER_THRESHOLD, JITTER_ALPHA);
			break;
		}
	}
	UserManager::getInstance().unlock();
}

void JointsGUI::guiEvent(ofxUIEventArgs &event) {
	if (event.widget->getName() == "RESET") 
	{
		gui->loadSettings("Kinect/defaults.xml");
	}

	if(event.widget->getName() == "SAVE")
	{
		gui->saveSettings("Kinect/settings.xml");
	}

	updateJoints();
}

void JointsGUI::setupGUI()
{
	gui = new ofxUIScrollableCanvas();
	gui->setGlobalSliderHeight(7.0);

	gui->addSlider("RIGHT HAND ALPA LOW", 0.0, 1.0, &RIGHT_HAND_ALPHA_MIN);
	gui->addSlider("RIGHT HAND ALPA HIGH", 0.0, 1.0, &RIGHT_HAND_ALPHA_MAX);
	gui->addSlider("RIGHT HAND GAMMA LOW", 0.0, 1.0, &RIGHT_HAND_GAMMA_MIN);
	gui->addSlider("RIGHT HAND GAMMA HIGH", 0.0, 1.0, &RIGHT_HAND_GAMMA_MAX);
	gui->addSlider("RIGHT HAND VELOCITY LOW", 0.0, 10.0, &RIGHT_HAND_VELOCITY_MIN);
	gui->addSlider("RIGHT HAND VELOCITY HIGH", 0.0, 10.0, &RIGHT_HAND_VELOCITY_MAX);

	gui->addSlider("RIGHT WRIST ALPA LOW", 0.0, 1.0, &RIGHT_WRIST_ALPHA_MIN);
	gui->addSlider("RIGHT WRIST ALPA HIGH", 0.0, 1.0, &RIGHT_WRIST_ALPHA_MAX);
	gui->addSlider("RIGHT WRIST GAMMA LOW", 0.0, 1.0, &RIGHT_WRIST_GAMMA_MIN);
	gui->addSlider("RIGHT WRIST GAMMA HIGH", 0.0, 1.0, &RIGHT_WRIST_GAMMA_MAX);
	gui->addSlider("RIGHT WRIST VELOCITY LOW", 0.0, 10.0, &RIGHT_WRIST_VELOCITY_MIN);
	gui->addSlider("RIGHT WRIST VELOCITY HIGH", 0.0, 10.0, &RIGHT_WRIST_VELOCITY_MAX);

	gui->addSlider("RIGHT ELBOW ALPA LOW", 0.0, 1.0, &RIGHT_ELBOW_ALPHA_MIN);
	gui->addSlider("RIGHT ELBOW ALPA HIGH", 0.0, 1.0, &RIGHT_ELBOW_ALPHA_MAX);
	gui->addSlider("RIGHT ELBOW GAMMA LOW", 0.0, 1.0, &RIGHT_ELBOW_GAMMA_MIN);
	gui->addSlider("RIGHT ELBOW GAMMA HIGH", 0.0, 1.0, &RIGHT_ELBOW_GAMMA_MAX);
	gui->addSlider("RIGHT ELBOW VELOCITY LOW", 0.0, 10.0, &RIGHT_ELBOW_VELOCITY_MIN);
	gui->addSlider("RIGHT ELBOW VELOCITY HIGH", 0.0, 10.0, &RIGHT_ELBOW_VELOCITY_MAX);

	gui->addSlider("RIGHT SHOULDER ALPA LOW", 0.0, 1.0, &RIGHT_SHOULDER_ALPHA_MIN);
	gui->addSlider("RIGHT SHOULDER ALPA HIGH", 0.0, 1.0, &RIGHT_SHOULDER_ALPHA_MAX);
	gui->addSlider("RIGHT SHOULDER GAMMA LOW", 0.0, 1.0, &RIGHT_SHOULDER_GAMMA_MIN);
	gui->addSlider("RIGHT SHOULDER GAMMA HIGH", 0.0, 1.0, &RIGHT_SHOULDER_GAMMA_MAX);
	gui->addSlider("RIGHT SHOULDER VELOCITY LOW", 0.0, 10.0, &RIGHT_SHOULDER_VELOCITY_MIN);
	gui->addSlider("RIGHT SHOULDER VELOCITY HIGH", 0.0, 10.0, &RIGHT_SHOULDER_VELOCITY_MAX);
	
	gui->addSlider("JITTER THRESHOLD", 0.0, 10.0, &JITTER_THRESHOLD);
	gui->addSlider("JITTER ALPHA", 0.0, 1.0, &JITTER_ALPHA);
	gui->addButton("RESET", false);
	gui->addButton("SAVE", false);

	gui->loadSettings("Kinect/settings.xml");

	gui->autoSizeToFitWidgets();
	ofAddListener(gui->newGUIEvent, this, &JointsGUI::guiEvent);

	gui->setVisible(false);

	updateJoints();
}

void JointsGUI::displayGUI()
{
	gui->setVisible(!gui->isVisible());
}