#pragma once

#ifndef INTROWORLD_H
#define INTROWORLD_H

#include "audio.h"

#include "engine.h"
#include "videoSphere.h"

class IntroWorld: public World
{
public:
	DECLARE_WORLD
	IntroWorld(Engine *engine);
	 virtual ~IntroWorld();

	virtual void setup();/// called on the start, after the opengl has been set up (mostly you can allocate resources there to avoid frame skips later)
	virtual void reset();
	virtual void beginExperience();/// called right before we enter the world
	virtual void endExperience();/// called when we leave the world (free up resources here)
	virtual void update(float dt); /// called on every frame while in the world

	virtual void drawScene(renderType render_type, ofFbo *fbo);/// fbo may be null . fbo is already bound before this call is made, and is provided so that the renderer knows the resolution
	virtual float getDuration();/// duration in seconds

	virtual BloomParameters getBloomParams();

	/// for testing
	virtual void keyReleased(int key);
	virtual void drawDebugOverlay();
	virtual void mouseMoved(int x, int y);

protected:
	VideoSphere *videoSphere;		// The video that loops
	VideoSphere *introVideoSphere;	// The video that fades in

	ofVec3f sphereCenter;
	ofVec3f lastHandPositions[2];
	ofVec3f currentHandPositions[2];
	float radius;

	float resumeStartTime;
	float reachOutStartTime;
	bool experienceStarted;
	bool playingIntro;

	SoundObject ambientSound;

	void TouchFive();
	void StopIntroFadeVideo();
};

#endif // INTROWORLD_H