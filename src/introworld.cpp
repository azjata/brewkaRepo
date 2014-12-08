#include "introworld.h"

DEFINE_WORLD(IntroWorld) /// this adds the world to the global world list so it can be created by name

IntroWorld::IntroWorld(Engine *engine):World(engine), ambientSound("worlds/intro/sounds/5_Loop.ogg", true, true)
{
	//ctor
	voicePromptFileName = "sounds/voice_prompts/voice_prompt_fadeintro.ogg";
	// add second voiceprompt
	voicePromptEndFileName = "sounds/voice_prompts/voice_prompt_intro_loop.ogg";
}

IntroWorld::~IntroWorld()
{
	delete videoSphere;
	delete introVideoSphere;
}

void IntroWorld::setup()
{
	cout << "IntroWorld::setup start" << endl;
	duration = FLT_MAX;
	World::setup();
	camera.setupPerspective(false, 90, 0.01, 1000); //TODO: where do FOV, near and far values come from? 
    camera.setPosition(0, 1.8, 0);

	sphereCenter = ofVec3f(0.0, 1.8, -1.0);
	radius = 0.6;

	introVideoSphere = new VideoSphere(engine, "video/Intro_5fadein_x.mov");
	videoSphere = new VideoSphere(engine, "video/Intro_Loop.mov");
	videoSphere->loopVideo(true);

	//cache sounds to avoid stutter
	GetALBuffer(voicePromptFileName);
	GetALBuffer(voicePromptEndFileName);

	reset();

	cout << "IntroWorld::setup end" << endl;
}

void IntroWorld::beginExperience()
{
	cout << "IntroWorld::begin experience" << endl;
	OSCManager::getInstance().startWorld(COMMON::WORLD_TYPE::INTRO);
	GlobAudioManager().AddSoundObject(&ambientSound);
	introVideoSphere->play();
	//introVideoSphere->rewind();
	introVideoSphere->pause();
	//videoSphere->play();
	cout << "IntroWorld::begin experience end" << endl;
}

void IntroWorld::reset() {

	cout << "IntroWorld: reset start" << endl;

	// Begin video paused. Starts only when ENTER key is pressed

	introVideoSphere->rewind();
	introVideoSphere->play();
	introVideoSphere->pause();
	videoSphere->pause();
	videoSphere->rewind();
	videoSphere->loopVideo(true);

	experienceStarted = false;
	playingIntro = false;

	resumeStartTime = 0.0f;

	// set it to max value to avoid jumping to the next world through the timeline
	duration = FLT_MAX;
	playedVoicePrompt = false;

	if(engine->isAutomaticTest())
		TouchFive();
	
	cout << "IntroWorld: reset end" << endl;
}

void IntroWorld::endExperience()
{
	playedVoicePrompt = false;
	GlobAudioManager().RemoveSoundObject(&ambientSound);
}

void IntroWorld::update(float dt)
{
	// Start the fadein video paused, then trigger the next video when playback is done
	if(!introVideoSphere->hasFinished())
	{
		introVideoSphere->update();
	}
	else 
	{
		if(playingIntro)
			StopIntroFadeVideo();
		else
			videoSphere->update();
	}

	if(!experienceStarted)
	{
		if(engine->startIntro)
			TouchFive();
		return;
	}

	// Play second voice prompt
	if (!playedVoicePrompt && Timeline::getInstance().getElapsedCurrWorldTime() - resumeStartTime >= WORLD_INTRO::DURATION_INTRO + 4.0) {
		playedVoicePrompt = true;
		PlayBackgroundSound(voicePromptEndFileName, COMMON::VOICE_PROMPT_GAIN);
	}

	if(playingIntro)
		return;

	// Check if the user has his hands straight for 2s
	if(engine->kinectEngine.isKinectDetected())
	{
		for(int i = 0; i < 2; i++)
		{
			currentHandPositions[i] = engine->kinectEngine.getHandsPosition()[i];
			float distance = sphereCenter.distanceSquared(currentHandPositions[i]);
			
			if(radius * radius > distance)
			{
				reachOutStartTime += dt;
				break;
			}
			else if (i == 1)
			{
				reachOutStartTime = 0.0;
			}
		}
	}

	if(reachOutStartTime >= WORLD_INTRO::REACH_OUT_DURATION || Timeline::getInstance().getElapsedCurrWorldTime()-resumeStartTime >= WORLD_INTRO::DURATION_MAIN)
	{
		// Set it to min value for the timeline to be able to load the next world
		//std::cout<<"time ellapsed "<<Timeline::getInstance().getElapsedCurrWorldTime()<<" reachoutstarttime "<<reachOutStartTime<<std::endl;
		OSCManager::getInstance().sendStatus("5 TOUCHED");
		duration = 0.0;
	}


}

void IntroWorld::drawScene(renderType render_type, ofFbo *fbo)
{
	//The intro fadeing has stopped playing, continue the normal flow.
	// Else, draw the fadein video

	if(experienceStarted && !playingIntro)
		videoSphere->draw();
	else
		introVideoSphere->draw();

	if(experienceStarted)
	{
		float transitionDuration = 4.0f;
		float opacity = 1.0f;
		float currTime = Timeline::getInstance().getElapsedCurrWorldTime() - resumeStartTime;
		if(currTime < transitionDuration){
			opacity = currTime/transitionDuration;
		}
		engine->kinectEngine.drawBodyUltramarine(ofVec3f(0.0, 1.8, 0.0), opacity*opacity);
	}
	//ofDrawSphere(sphereCenter, radius);
}

float IntroWorld::getDuration()
{
	return duration;
}

void IntroWorld::TouchFive()
{
	if (!experienceStarted) 
	{
		// start experience
		cout << "Starting experience" << endl;
		//OSCManager::getInstance().sendStatus("INTRO HAS STARTED");
		experienceStarted = true;
		playingIntro = true;
		introVideoSphere->play();
		resumeStartTime = Timeline::getInstance().getElapsedCurrWorldTime();
		// Play first voice prompt
		PlayBackgroundSound(voicePromptFileName, COMMON::VOICE_PROMPT_GAIN);
	}
}

void IntroWorld::StopIntroFadeVideo()
{
	std::cout<<"FINISHED";
	//introVideoSphere->pause();
	videoSphere->play();
	playingIntro = false;
}

void IntroWorld::keyReleased(int key)
{
}

void IntroWorld::drawDebugOverlay()
{
}

void IntroWorld::mouseMoved(int x, int y)
{
	//ofVec3f pos=camera.getPosition();
	//ofVec3f dir=camera.screenToWorld(ofVec3f(x,y,-1.0))-pos;
	//mouse_dir=glm::normalize(glm::vec3(dir.x, dir.y, dir.z));
}

BloomParameters IntroWorld::getBloomParams(){
	BloomParameters params;

	params.bloom_multiplier_color=glm::vec3(0.0);
	params.gamma=1.0;
	params.exposure=1.0;

	return params;
}