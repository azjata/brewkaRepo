#include "timeline.h"

#include "../engine.h"

void Timeline::init(Engine * engine) {
	this->engine = engine;
	paused = false;
	started = false;
}

void Timeline::update(float deltaTime) {
	if (paused || !started) {
		return;
	}
	totalTime += deltaTime;
	// see if we're done with the current world
	if(engine->getCurrentWorld()) {
		if(elapsedCurrWorldTime > engine->getCurrentWorld()->getDuration() && !engine->isSingleWorld()) {
			goToNextWorld();
		}
	}
	// update the total experience time
	elapsedExperienceTime = totalTime - experienceStartTime;
	// update the world time
	elapsedCurrWorldTime = totalTime - currWorldStartTime;
}

void Timeline::start() {
	if (!started) {
		started = true;
		if(engine->hasWorlds()) {
				engine->setWorld(0);
			experienceStartTime = currWorldStartTime = totalTime = ofGetElapsedTimef();
			elapsedCurrWorldTime = elapsedExperienceTime = 0.0f;
			std::cout<<"ENGINE>> Start Experience <<"<<std::endl;
			OSCManager::getInstance().sendStatus("START EXPERIENCE");
			engine->getCurrentWorld()->beginExperience();
		}
	}
}

bool Timeline::hasStarted() {
	return started;
}

void Timeline::restart() {
	OSCManager::getInstance().reset();
	stop(true);
	start();
}

bool Timeline::isPaused() {
	return paused;
}

void Timeline::togglePause() {
	paused = !paused;
	if (paused) {
		GlobAudioManager().PauseAll();
	} else {
		GlobAudioManager().ResumeAll();
	}
}

void Timeline::stop(bool force) {
	if (started || force) {
		// go to limbo world
		GlobAudioManager().RemoveAllSoundObjects();
		engine->resetWorlds();
		engine->setWorld(0);
		started = false;
	}
}

float Timeline::getElapsedExperienceTime() {
    return elapsedExperienceTime;
}

float Timeline::getElapsedCurrWorldTime() {
    return elapsedCurrWorldTime;
}

float Timeline::getExperienceStartTime() {
	return experienceStartTime;
}

float Timeline::getCurrWorldStartTime() {
    return currWorldStartTime;
}

void Timeline::goToNextWorld() {
	currWorldStartTime = totalTime;
	if (!engine->goToNextWorld()) {
		//stop(true);
		restart();
	}
}