#pragma once

#ifndef BODYWORLD_H
#define BODYWORLD_H

#include "engine.h"
#include "ofxUI.h"
#include "resourcemanager.h"
#include "bloom.h"

class BodyWorld: public World {
public:
	DECLARE_WORLD
	BodyWorld(Engine *engine);
	virtual ~BodyWorld();

	virtual void setup();
	virtual void beginExperience();
	virtual void endExperience();
	virtual void keyPressed(int key);
	void update (float dt);

	void drawScene (renderType render_type, ofFbo *fbo);
	float getDuration();

	BloomParameters getBloomParams();

	private:
		ofVbo bodyVbo;

		ofEasyCam easyCam; //for easyTesting

};

#endif