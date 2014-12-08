#pragma once

#ifndef __TIMELINE_H__
#define __TIMELINE_H__

#include "../audio.h"
#include <queue>
#include "timecommand.h"

class Engine;

class Timeline {

public:

    static Timeline& getInstance() {
        static Timeline instance;
        return instance;
    }

    void init(Engine * engine);

    void update(float currentElapsedTime);

	/* Start the experience */
    void start();

	/* stops the experience */
	void stop(bool force);

	bool isPaused();

	void togglePause();

	/* restarts the experience */
	void restart();

	/* time since the experience started */
    float getElapsedExperienceTime();

	/* time since entering the current world */
    float getElapsedCurrWorldTime();

	/* time at witch the experience started */
    float getExperienceStartTime();

	/* time at which the current world started */
    float getCurrWorldStartTime();

	bool hasStarted();

private:

	Engine * engine;
    float experienceStartTime;
    float currWorldStartTime;
    float elapsedCurrWorldTime;
    float elapsedExperienceTime;
	float totalTime;

	void goToNextWorld();

    Timeline() {};
    Timeline(Timeline const&); // Don't Implement
    void operator=(Timeline const&); // Don't implement

	std::queue<TimeCommand> commands;

	bool started;
	bool paused;
};

#endif