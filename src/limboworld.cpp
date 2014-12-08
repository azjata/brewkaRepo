#include "limboworld.h"

DEFINE_WORLD(LimboWorld) /// this adds the world to the global world list so it can be created by name

LimboWorld::LimboWorld(Engine *engine):World(engine)
{
	//ctor
	voicePromptFileName = "";
}

LimboWorld::~LimboWorld()
{
}

void LimboWorld::setup()
{
	cout << "LimboWorld::setup start" << endl;
	World::setup();
	duration = FLT_MAX;
	cout << "LimboWorld::setup end" << endl;

}

void LimboWorld::beginExperience()
{
	reset();
}

void LimboWorld::reset() {
	cout << "LimboWorld: reset start" << endl;
	// set it to max value to avoid jumping to the next world through the timeline
	duration = FLT_MAX;
	// reset camera
	camera.setupPerspective(false, 90, 0.01, 1000); //TODO: where do FOV, near and far values come from?
    camera.setPosition(0, 1.8, 0);
	cout << "LimboWorld: reset end" << endl;
}

void LimboWorld::endExperience()
{
}

void LimboWorld::update(float dt)
{
}

void LimboWorld::drawScene(renderType render_type, ofFbo *fbo)
{
	
}

float LimboWorld::getDuration()
{
	return duration;
}

void LimboWorld::keyPressed(int key)
{
	if (key == OF_KEY_RETURN) {
		// skip world
		duration = 0.0;
	}

	World::keyPressed(key);
}

void LimboWorld::keyReleased(int key)
{
}

void LimboWorld::drawDebugOverlay()
{
}

void LimboWorld::mouseMoved(int x, int y)
{
}

BloomParameters LimboWorld::getBloomParams(){
	return BloomParameters();
}
