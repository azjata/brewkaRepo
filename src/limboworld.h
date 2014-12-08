#ifndef _LIMBO_WORLD_H
#define _LIMBO_WORLD_H

#pragma once

#include "engine.h"

class LimboWorld : public World
{
public:
	DECLARE_WORLD
	LimboWorld(Engine *engine);
	 virtual ~LimboWorld();

	virtual void setup();/// called on the start, after the opengl has been set up (mostly you can allocate resources there to avoid frame skips later)
	virtual void reset();
	virtual void beginExperience();/// called right before we enter the world
	virtual void endExperience();/// called when we leave the world (free up resources here)
	virtual void update(float dt); /// called on every frame while in the world

	virtual void drawScene(renderType render_type, ofFbo *fbo);/// fbo may be null . fbo is already bound before this call is made, and is provided so that the renderer knows the resolution
	virtual float getDuration();/// duration in seconds

	virtual BloomParameters getBloomParams();

	/// for testing
	virtual void keyPressed(int key);
	virtual void keyReleased(int key);
	virtual void drawDebugOverlay();
	virtual void mouseMoved(int x, int y);

};

#endif