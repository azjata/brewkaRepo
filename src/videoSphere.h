#ifndef CONTAINER_H
#define CONTAINER_H
#pragma once

#include "engine.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#ifdef _WIN32
#include "ofDirectShowPlayer.h"
#endif
using namespace glm;

class VideoSphere
{
	public:
		VideoSphere(Engine *engine, string videoSrc);
		VideoSphere(Engine *engine, string videoSrc, string videoAlphaSrc);

		~VideoSphere();

		void play();
		void pause();
		void rewind();
		void close();
		bool isPlaying();
		bool hasFinished();
		void loopVideo(bool loop);
		void setFrame(int frame);
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void drawDebugOverlay();


	///private:

		Engine				*engine;

		ofVideoPlayer       *videoColor;
		ofVideoPlayer       *videoAlpha;

		#ifdef _WIN32
		ofDirectShowPlayer  *dPlayerColor;
		ofDirectShowPlayer  *dPlayerAlpha;
		#endif
		ofSpherePrimitive	*sphere;
};

#endif // CONTAINER_H
