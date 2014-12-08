#pragma once

#ifndef EVOLUTIONWORLD_H
#define EVOLUTIONWORLD_H

#include "engine.h"
#include "ofxUI.h"
#include "resourcemanager.h"
#include "bloom.h"
#include "audio.h"

#ifdef _WIN32
#include "ofDirectShowPlayer.h"
#endif

# define RETURN_POSITIONS 100

class EvolutionWorld: public World {
public:
	DECLARE_WORLD
	EvolutionWorld(Engine *engine);
	virtual ~EvolutionWorld();

	void setup();
	void reset();

	void beginExperience();
	void endExperience();
	void update (float dt);

	void drawScene (renderType render_type, ofFbo *fbo);
	float getDuration();

	void exit();

	bool getUseBloom();
	BloomParameters getBloomParams();

	void mouseMoved(int x, int y);
	void mousePressed(int x, int y, int button);

private:
	void setupLandscape();
	void setupBotResources();
	void setupBotGeometry();
	void setupSky();
	void setupGUI();
	void setupSounds();

	void setupInitialBotData();
	
	void loadSounds();
	void removeSounds();

	void resetBotData();
	void updateSoundPositions();
	void renderBody();
	
	void updateCamera(float dt);

	virtual void keyPressed(int key);

	float currentTime;

	float speed;
	float cameraPitch;

	ofSpherePrimitive backgroundSphere;

	ofImage backgroundImage;
	ofImage skyImage;

	ofxUICanvas *gui;
	void guiEvent(ofxUIEventArgs &e);

	int botNumIndices;

	GLuint botDataBuffer;
	GLuint botDataTargetBuffer; //temporary buffer to write to

	GLuint botInstanceDataBuffer;

	GLuint botInstanceCountQuery;

	GLuint botVertexPositionBuffer;
	GLuint botVertexNormalBuffer;
	GLuint botTextureCoordinatesBuffer;
	GLuint botIndexBuffer;

	GLuint animateProgram;

	ofEasyCam easyCam; //for easyTesting

	ofVbo landscapeVbo;
	ofVbo botVbo;

	float * initialBotData;

	ofVec3f positions[RETURN_POSITIONS];
	SoundObject soundObjects[RETURN_POSITIONS];
	bool sObjectsActive[RETURN_POSITIONS];

	ofVec3f colors[RETURN_POSITIONS];

	ofVideoPlayer *videoPlayerColor;
	//ofVideoPlayer *videoPlayerAlpha;
	#ifdef _WIN32
	ofDirectShowPlayer *directShowPlayerColor;
	//ofDirectShowPlayer *directShowPlayerAlpha;
	#endif
	ofPlanePrimitive portalPlane;

	ofImage sandImage;
	ofImage botImage;

	ofVec3f lastHandPositions[2];
	ofVec3f currentHandPositions[2];

	ofVec3f lastHandVelocities[2][100];

	ofVec2f mousePosition;

	ofVec3f previousAverageVelocities[2];

	bool playedGateSound;

	float lastInteractionSoundStartTime[2];

	bool liftingUp;

	ofVec3f cameraPosition;

	SoundObject ambientSound;
};

#endif
