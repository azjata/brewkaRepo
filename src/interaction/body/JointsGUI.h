#pragma once
#ifndef __JOINT_GUI__
#define __JOINT_BUI__

#include "ofxUI.h"
#include  "../enums/JointType.h"
#include "Body.h"
#include "../UserManager.h"

class JointsGUI
{
public:
	JointsGUI();
	~JointsGUI();

	void setupGUI();
	void displayGUI();

private:
	void updateJoints();

	ofxUICanvas *gui;
	void guiEvent(ofxUIEventArgs &e);
};

#endif