#ifndef ENGINE_H
#define ENGINE_H

#pragma once

#include "ofMain.h"
#include "ofEvents.h"
#include "ofxOculusRift.h"
#include "bloom.h"
#include "interaction/KinectEngine.h"
#include "osc/threads/OSCConsumer.h"
#include "osc/OSCManager.h"
#include "timeline/timeline.h"
#include "ofxXmlSettings.h"
#include "StuntAppEvents.h"
#include "interaction/body/JointsGUI.h"

class Engine;

enum renderType {
	render_type_display_frame,
	render_type_left_eye,
	render_type_right_eye,
	render_type_cubemap_face
};

#define DECLARE_WORLD \
	virtual std::string getClassName();

class World{
public:
	Engine *engine;
	World(Engine *engine_);
	virtual ~World();
	virtual std::string getClassName()=0;

	virtual void setup(); /// called right on the start when the application is started and after the opengl has been set up (mostly you can allocate resources there to avoid frame skips later)
	virtual void reset(); /// called every time the whole experience is restarted
	virtual void beginExperience();/// called right before we enter the world
	virtual void endExperience();/// called when we leave the world (free up resources here)
	virtual void update(float dt); /// called on every frame while in the world

	virtual void drawScene(renderType render_type, ofFbo *fbo);/// fbo may be null . fbo is already bound before this call is made, and is provided so that the renderer knows the resolution
	/// note: it is also responsible for rendering the transition to the next world

	virtual void exit();//called when the application exits

	virtual void drawTransitionObject(renderType render_type, ofFbo *fbo); /// the object for transitioning *into* this world

	virtual float getDuration();/// duration in seconds

	/// for testing
	virtual void keyPressed(int key);
	virtual void keyReleased(int key);
	virtual void drawDebugOverlay();

	virtual void mouseMoved(int x, int y);
	virtual void mousePressed(int x, int y, int button);
	virtual void mouseReleased(int x, int y, int button);
	virtual void mouseDragged(int x, int y, int button);

	virtual bool getUseBloom();/// return true if you want bloom filter to be used when rendering this world.

	virtual BloomParameters getBloomParams();/// provide parameters for the bloom

	ofCamera camera;/// This camera is used by the Engine when rendering the world.

	// [0] = intro finished, [1] = world finished, [2] = transition finished
	bool hwPrepared[3];

	virtual void loadVoicePromptSound();/// load the voice prompt sound
	string voicePromptFileName;
	string voicePromptEndFileName;
	bool playedVoicePrompt;

	float duration;

	void skip();
};

class WorldFactoryAbstract{
	public:
	virtual World *newWorld(Engine *engine)=0;
	virtual ~WorldFactoryAbstract(){}
};

class WorldFactoryList{
	std::map<std::string, WorldFactoryAbstract*> factories;
public:
	World *newWorld(Engine *engine, const std::string &name){
		std::map<std::string, WorldFactoryAbstract*>::iterator i=factories.find(name);
		if(i!=factories.end()){
			if(i->second)return i->second->newWorld(engine);
			std::cerr<<"something is wrong with factories list"<<std::endl;
			return NULL;
		}else{
			return NULL;
		}
	}
	void registerFactory(WorldFactoryAbstract *factory, const std::string &name){
		std::map<std::string, WorldFactoryAbstract*>::iterator i=factories.find(name);
		if(i!=factories.end()){
			std::cerr<<"duplicate world factory '"<<name<<"'"<<std::endl;
			delete i->second;
			i->second=factory;
		}else{
			factories[name]=factory;
		}
	}
	void printWorlds(){
		for(std::map<std::string, WorldFactoryAbstract*>::iterator i=factories.begin(); i!=factories.end(); ++i){
			ofLog()<<i->second<<std::endl;
		}
	}
};

WorldFactoryList &globalWorldFactoryList();

template<class T>
class WorldFactory{
	public:
	WorldFactory(const std::string &name){
		globalWorldFactoryList().registerFactory((WorldFactoryAbstract *)this, name);
	}
	virtual World *newWorld(Engine *engine){
		return new T(engine);
	}
};

#define DEFINE_WORLD(name) \
	WorldFactory<name> name##_register(#name);\
	std::string name::getClassName(){return #name;}

class Engine: public ofBaseApp /// handles oculus rendering, post processing, and the like
{
	public:
		Engine(const std::string &single_world_name, bool automaticTest_);/// pass name to start in single world mode
		virtual ~Engine();

		/// resolution dependent scaling factors for the brightness of points and lines
		float getPointBrightnessScaling();
		float getLineBrightnessScaling();

		/// functions called internally or from OpenFrameworks
		void setup();
		void update();
		void exit();
		void draw();

		void drawScene(renderType render_type, ofFbo *fbo);

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		Bloom bloom;

		KinectEngine kinectEngine;

		int getRenderWidth();
		int getRenderHeight();

		bool startIntro;
		/*
		* returns the current world
		*/
		World* getCurrentWorld();

		/*
		* returns the next world
		*/
		World* getNextWorld();

		/*
		* goes to next world. true if there's a next world, false otherwise.
		*/
		bool goToNextWorld();

		/*
		* true when drawing the single world continuously during testing
		*/
		bool isSingleWorld();
		bool isAutomaticTest();/// running in automatic test mode?

		bool hasWorlds();
		void setWorld(int nextWorld);
		
		void setupWorlds();

		void resetWorlds();

		void StuntAppStatusChanged(int &eventType);

	
	protected:
/// Coding style note: take every care not to have any unitialized variables anywhere
/// Initialize things in constructor
		ofxOculusRift oculusRift;

		//bool showOverlay;
		bool predictive;
		

		void addWorld(const std::string &name);/// must be called during setup

		int render_width;
		int render_height;

		std::vector<World *> worlds;
		int currentWorld;
		bool singleWorld;

	private:
		ofxXmlSettings settings;
		JointsGUI jointsGui;
		OSCConsumer oscConsumer;
		
		// kinect settings
		string kinectGroupServer;
		int kinectGroupPort;
		glm::vec3 camera_position; /// used for audio, includes oculus transformation
		glm::vec3 camera_forward; /// used for audio, includes oculus transformation
		glm::vec3 camera_up; /// used for audio, includes oculus transformation

		// osc settings
		int oscListenPort;
		string rPiIP;
		int rPiPort;
		string camServerIP;
		int camServerPort;

		/* Delta time between two update calls */
		float lastUpdateTime;
		float updateDeltaTime;

		bool showFPS;

		void StartExperience();
		void RestartExperience();
		void StopExperience();
		void CalibrateKinect();
		void CalibrateOculusRift();

		bool automaticTest;
};

#endif // ENGINE_H
