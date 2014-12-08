#include "engine.h"
#include "resourcemanager.h"
#include "audio.h"

World::World(Engine *engine_):engine(engine_){
	hwPrepared[0] = hwPrepared[1] = hwPrepared[2] = false;
}
World::~World(){
}
void World::setup(){
	camera.setupPerspective(false, 90, 0.01, 200);
    camera.setPosition( 0, 0, 0 );
    camera.begin();/// not entirely sure why we're doing this
    camera.end();
	playedVoicePrompt = false;
	loadVoicePromptSound();
}
void World::loadVoicePromptSound() {
	if (!voicePromptFileName.empty()) {
		GetALBuffer(voicePromptFileName);
	}
}
void World::reset() {
}
void World::beginExperience(){
}
void World::endExperience(){
}
void World::update(float dt){
}
void World::drawScene(renderType render_type, ofFbo *fbo){
}
void World::exit() {
}
void World::drawTransitionObject(renderType render_type, ofFbo *fbo){
}
float World::getDuration(){
	return 0;
}
void World::keyPressed(int key){
}
void World::keyReleased(int key){
}
void World::drawDebugOverlay(){
}

void World::mouseMoved(int x, int y) {
}
void World::mouseDragged(int x, int y, int button) {
}
void World::mousePressed(int x, int y, int button) {
}
void World::mouseReleased(int x, int y, int button) {
}
void World::skip() {
	duration = 0.0f;
}

bool World::getUseBloom(){
	return true;
}

BloomParameters World::getBloomParams(){
	return BloomParameters();
}

WorldFactoryList &globalWorldFactoryList(){
	static WorldFactoryList *result=new WorldFactoryList;
	return *result;
}

Engine::Engine(const std::string &single_world_name_, bool automaticTest_):/*showOverlay(false),*/ predictive(true),
render_width(0), render_height(0), currentWorld(0), singleWorld(false), automaticTest(automaticTest_)
{
	//ctor

	//if (_DEBUG_) {
		if ( !single_world_name_.empty()) {
			singleWorld = true;
			addWorld(single_world_name_);
		}
	//}
}

Engine::~Engine()
{
	for(std::vector<World*>::iterator i=worlds.begin(); i!=worlds.end(); ++i){
		delete *i;
	}
}

bool Engine::isSingleWorld() {
	return singleWorld;
}

bool Engine::isAutomaticTest() {
	return automaticTest;
}

World *Engine::getCurrentWorld(){
	if(currentWorld >=0 && currentWorld < worlds.size()) {
		return worlds[currentWorld];
	}
	return NULL;
}
World *Engine::getNextWorld(){/// may return null
	if(currentWorld >=0 && currentWorld+1 < worlds.size()){
		return worlds[currentWorld+1];
	}
	return NULL;
}

/// resolution dependent scaling factors for the brightness of points and lines
float Engine::getPointBrightnessScaling(){
	return 1.0;/// TODO: calculate it depending on the resolution and field of view
}
float Engine::getLineBrightnessScaling(){
	return sqrt(getPointBrightnessScaling());
}

void Engine::setupWorlds() {

	// add more world if not in single world mode

	if (!singleWorld) {
		//addWorld("BodyWorld");
		addWorld("IntroWorld");
		addWorld("UltramarineWorld");
		addWorld("ElectricWorld");
		addWorld("EvolutionWorld");
		addWorld("LiquidWorld");
	}

	for (World * world : worlds) {
        world->setup();
	}

	resetWorlds();

}

void Engine::resetWorlds() {
	cout << "Engine: reset start" << endl;
	for(std::vector<World*>::iterator i=worlds.begin(); i!=worlds.end(); ++i){
		(*i)->reset();
	}
	cout << "Engine: reset end" << endl;
}

//--------------------------------------------------------------
void Engine::setup()
{
	GlobAudioManager();
	getGlobalResourceManager().load();/// TODO:error handling

	ofBackground(0);
	ofSetLogLevel( OF_LOG_WARNING );
	ofSetVerticalSync( true );

	settings.load("general_settings.xml");

	kinectGroupServer = settings.getValue(SETTINGS::TAG_KINECT_GROUP_IP, SETTINGS::VALUE_KINECT_GROUP_IP);
	kinectGroupPort = settings.getValue(SETTINGS::TAG_KINECT_GROUP_PORT, SETTINGS::VALUE_KINECT_GROUP_PORT);

	kinectEngine.setup((char *)kinectGroupServer.c_str(), kinectGroupPort);

	oscListenPort = settings.getValue(SETTINGS::TAG_OSC_LISTENING_PORT, SETTINGS::VALUE_OSC_LISTENING_PORT);

	oscConsumer.setup(oscListenPort);
	#ifndef NO_ANNOYING_CRAP /// a define set in my builds - dmytry.
	oscConsumer.startThread();
	#endif

	rPiIP = settings.getValue(SETTINGS::TAG_RPI_IP, SETTINGS::VALUE_RPI_IP);
	rPiPort = settings.getValue(SETTINGS::TAG_RPI_PORT, SETTINGS::VALUE_RPI_PORT);

	OSCManager::getInstance().setup(rPiIP, rPiPort,true);
	OSCManager::getInstance().setupStunt("127.0.0.1",12345);
	OSCManager::getInstance().reset();//sendMessage(OSCMessageBuilder::BuildResetMessage(),true);

	// Cam Server
	camServerIP = settings.getValue(SETTINGS::TAG_CAM_SERVER_IP, SETTINGS::VALUE_CAM_SERVER_IP);
	camServerPort = settings.getValue(SETTINGS::TAG_CAM_SERVER_PORT, SETTINGS::VALUE_CAM_SERVER_PORT);

	OSCManager::getInstance().setup(camServerIP, camServerPort,false);

	// Oculus Rift
    oculusRift.setup( NULL );

	setupWorlds();

	// save general settings
	settings.setValue(SETTINGS::TAG_KINECT_GROUP_PORT, kinectGroupPort);
	settings.setValue(SETTINGS::TAG_KINECT_GROUP_IP, kinectGroupServer);
	settings.setValue(SETTINGS::TAG_OSC_LISTENING_PORT, oscListenPort);
	settings.setValue(SETTINGS::TAG_RPI_IP, rPiIP);
	settings.setValue(SETTINGS::TAG_RPI_PORT, rPiPort);
	settings.setValue(SETTINGS::TAG_CAM_SERVER_IP, camServerIP);
	settings.setValue(SETTINGS::TAG_CAM_SERVER_PORT, camServerPort);
	settings.saveFile();

	showFPS = false;


	lastUpdateTime = ofGetElapsedTimef();
	updateDeltaTime = 0;

	// this starts the experience
	startIntro = false;
	Timeline::getInstance().init(this);
	Timeline::getInstance().start();
	
	jointsGui.setupGUI();
} 


//--------------------------------------------------------------
void Engine::update()
{
	// get the new time
	float currentElapsedTime = ofGetElapsedTimef();

	// update the delta time
	updateDeltaTime = currentElapsedTime - lastUpdateTime;

	// update the time of the last update
	lastUpdateTime = currentElapsedTime;

	// if pause don't increase the time
	if (Timeline::getInstance().isPaused()) updateDeltaTime = 0.0;

	Timeline::getInstance().update(updateDeltaTime);

	if(getCurrentWorld()){
	/*	if (updateDeltaTime > 1.0 / 55.0){
			std::cout << "Missing frame, dt=" << updateDeltaTime << "(1/" << (1/updateDeltaTime) << ")" << std::endl;
		}
		*/
		getCurrentWorld()->update(updateDeltaTime);
		GlobAudioManager().Update(updateDeltaTime, camera_position, camera_forward, camera_up);
	}else{
		std::cout<<"warning: no active world"<<std::endl;
		GlobAudioManager().Update(updateDeltaTime);
	}

	OSCManager::getInstance().update(currentElapsedTime);
	if(!oscConsumer.lockQueue)
	
		if(!oscConsumer.pendingEvents.empty())
		{
			while(!oscConsumer.pendingEvents.empty())
			{
				int status = oscConsumer.pendingEvents.front();
				oscConsumer.pendingEvents.pop();
				StuntAppStatusChanged(status);
			}
		}
	
		kinectEngine.update(updateDeltaTime, getCurrentWorld()->camera.getOrientationEuler().x);
}

void Engine::exit() {

	cout << "Engine: call exit in each world..." << endl;
	for (int i = 0; i < worlds.size(); ++i) {
		worlds[i]->exit();
	}

	// reset the hardware
	OSCManager::getInstance().reset();
	OSCManager::getInstance().update(ofGetElapsedTimef());

    // stop the osc consumer thread

	cout << "Engine: stopping osc consumer..." << endl;
	oscConsumer.waitForThread(true);

	cout << "Engine: stopping kinect engine..." << endl;
	kinectEngine.exit();



}

void Engine::draw(){
	if(oculusRift.isSetup())
	{
		if(getCurrentWorld()){
			oculusRift.baseCamera=&getCurrentWorld()->camera;
		}else{
			oculusRift.baseCamera=NULL;
		}

		if(showFPS){
			glUseProgram(0);

			oculusRift.beginOverlay(-230, 320,240);
			ofRectangle overlayRect = oculusRift.getOverlayRectangle();

			ofPushStyle();
			ofEnableAlphaBlending();
			ofFill();
			ofSetColor(255, 40, 10, 200);

			ofRect(overlayRect);

			ofSetColor(255,255);
			ofFill();
			ofDrawBitmapString("FPS:"+ofToString(ofGetFrameRate()), 40, 40);

			ofPopStyle();
			if(getCurrentWorld()){
				getCurrentWorld()->drawDebugOverlay();
			}
			oculusRift.endOverlay();
		}

		glEnable(GL_DEPTH_TEST);
		oculusRift.beginLeftEye();
		drawScene(render_type_left_eye, NULL);
		glColor3f(1,1,1);/// workaround for bug with overlay
		oculusRift.endLeftEye();

		oculusRift.beginRightEye();
		drawScene(render_type_right_eye, NULL);
		glColor3f(1,1,1);/// workaround for bug with overlay
		oculusRift.endRightEye();

		glColor3f(1,1,1);/// oculusRift.draw() is affected by gl_Colour if you are using stock shaders
		oculusRift.draw();
		glDisable(GL_DEPTH_TEST);

		if(getCurrentWorld()){

			glm::mat4 m=fromOF(oculusRift.getOrientationMat()*getCurrentWorld()->camera.getGlobalTransformMatrix());

			camera_position=glm::vec3(m*glm::vec4(0,0,0,1));
			camera_forward=glm::vec3(m*glm::vec4(0,0,-1,0));
			camera_up=glm::vec3(m*glm::vec4(0,1,0,0));
			/*
			camera_orientation=glm::mat3(m(0,0), m(1,0), m(2,0),
										 m(0,1), m(1,1), m(2,1),
										 m(0,2), m(1,2), m(2,2));*/

			//std::cout<<camera_position.x<<", "<<camera_position.y<<", "<<camera_position.z<<std::endl;
		}
	}
	else{
		if(getCurrentWorld()){
			getCurrentWorld()->camera.begin();
			drawScene(render_type_display_frame, NULL);
			getCurrentWorld()->camera.end();

			glm::mat4 m=fromOF(getCurrentWorld()->camera.getGlobalTransformMatrix());
			camera_position=glm::vec3(m*glm::vec4(0,0,0,1));
			camera_forward=glm::vec3(m*glm::vec4(0,0,-1,0));
			camera_up=glm::vec3(m*glm::vec4(0,1,0,0));

			//std::cout<<camera_position.x<<", "<<camera_position.y<<", "<<camera_position.z<<std::endl;

			glDisable(GL_DEPTH_TEST);

			if (showFPS) {
				ofSetColor(255,255);
				ofFill();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable(GL_CULL_FACE);
				glUseProgram(0);
				ofDrawBitmapString("FPS:"+ofToString(ofGetFrameRate()), ofGetWidth() - 75, 20);
			}
		}
	}

}

void Engine::drawScene(renderType render_type, ofFbo *fbo){
	if(!getCurrentWorld())return;

	bool do_bloom=getCurrentWorld()->getUseBloom();

	if(do_bloom){
		BloomParameters bp=getCurrentWorld()->getBloomParams();
		bloom.setParams(bp);
		if(oculusRift.isSetup()){
			ofRectangle VP=oculusRift.getOculusViewport();
			render_width=VP.getWidth();
			render_height=VP.getHeight();
		}else{
			render_width=ofGetWidth();
			render_height=ofGetHeight();
		}
		bloom.begin(render_width, render_height);
	}

	getCurrentWorld()->drawScene(render_type, fbo);

	if(do_bloom){
		bloom.end();
		bloom.draw();
	}
}

int Engine::getRenderWidth(){
	return render_width;
}
int Engine::getRenderHeight(){
	return render_height;
}

void Engine::keyPressed(int key){

	// Fullscreen
	if( key == 'f' ) 
	{
		ofToggleFullscreen();
	} 
	// Pause
	else if (key == OF_KEY_BACKSPACE)
	{
		Timeline::getInstance().togglePause();
	} 
	// FPS
	else if (key == 'd') 
	{
		showFPS = !showFPS;
	} 
	// Restart
	else if (key == 'r' || key == 'R') 
	{
		RestartExperience();
	} 
	// Stop
	else if(key == 's' || key == 'S') 
	{
		Timeline::getInstance().stop(false);
	} 
	//
	else if(key == 'a' || key == 'A') 
	{
		ofSetVerticalSync(key == 'a');
	} 
	// Hide cursor
	else if(key == 'h' || key == 'H') 
	{
		ofShowCursor();
	} 
	// Show joints GUI
	else if (key == 'g')
	{
		jointsGui.displayGUI();
	} 
	// Skip world
	else if( key=='q' || key=='Q' )
	{
		if(getCurrentWorld())getCurrentWorld()->skip();
	} 
	// Start
	else if( key==OF_KEY_RETURN ) 
	{
		// start timeline if it hasn't started already
		StartExperience();
		
	} 
	// Calibrate Oculus
	else if (key == 'o' || key == 'O') 
	{
		CalibrateOculusRift();
	}
	// Calibrate Kinect
	else if(key == 'k' || key == 'K')
	{
		CalibrateKinect();
	}

	if(getCurrentWorld())getCurrentWorld()->keyPressed(key);
}

void Engine::StuntAppStatusChanged(int &eventType)
{
//	cout<< " STATUS CHANGED " << eventType << endl;
	switch(eventType)
	{
	case StuntAppEvents::START:
		{
			StartExperience();
			break;
		}
	case StuntAppEvents::RESET:
		{
			RestartExperience();
			break;
		}
	case StuntAppEvents::KINECT_RESET:
		{
			CalibrateKinect();
			break;
		}
	case StuntAppEvents::FANS:
		{
			OSCManager::getInstance().sendStatus("FANS");
			OSCManager::getInstance().setFrontFanSpeed(1,0,10);
			OSCManager::getInstance().setTopFanSpeed(1,0.5f,10);
			break;
		}
	case StuntAppEvents::CALIBRATE:
		{
			CalibrateOculusRift();
			break;
		}

	}
}

void Engine::StartExperience()
{
	// First time you press enter it should start the experience in the intro
	// Second time stop the liquid world and restart the experience
	startIntro = !startIntro;

	if(!startIntro)
	{
		// Experience restart
		if(!goToNextWorld())
			RestartExperience();
	}
}

void Engine::RestartExperience()
{
	std::cout<<"ENGINE>> Restart Experience<<"<<std::endl;
	OSCManager::getInstance().sendStatus("RESTART EXPERIENCE");
	Timeline::getInstance().restart();
	startIntro = false;

}

void Engine::CalibrateKinect()
{
	std::cout<<"ENGINE>> Kinect Calibrated << "<<std::endl;
	OSCManager::getInstance().sendStatus("KINECT CALIBRATED");
	vector<std::shared_ptr<Body> > bodies;
	UserManager::getInstance().filterUsers(bodies);
}

void Engine::CalibrateOculusRift()
{
	std::cout<<"ENGINE>> Oculus Calibrated << "<<std::endl;
	OSCManager::getInstance().sendStatus("OCULUS CALIBRATED");
	oculusRift.reset();
}


void Engine::keyReleased(int key){
	if(getCurrentWorld())getCurrentWorld()->keyReleased(key);
}

void Engine::mouseMoved(int x, int y){
	if(getCurrentWorld())getCurrentWorld()->mouseMoved(x, y);
}

void Engine::mouseDragged(int x, int y, int button){
	if(getCurrentWorld())getCurrentWorld()->mouseDragged(x, y, button);
}

void Engine::mousePressed(int x, int y, int button){
	if(getCurrentWorld())getCurrentWorld()->mousePressed(x, y, button);
}

void Engine::mouseReleased(int x, int y, int button){
	if(getCurrentWorld())getCurrentWorld()->mouseReleased(x, y, button);
}

void Engine::windowResized(int w, int h){
}

void Engine::dragEvent(ofDragInfo dragInfo){
}

void Engine::gotMessage(ofMessage msg){
}

bool Engine::hasWorlds() {
	return worlds.size() > 0;
}
void Engine::setWorld(int world) {
	currentWorld = world;
}

void Engine::addWorld(const std::string &name) {
	World * w = globalWorldFactoryList().newWorld(this, name);
    if(w){
        worlds.push_back(w);
    }else{
        ofLog(OF_LOG_ERROR)<<"World '"<<name<<"' not found!"<<std::endl;
        ofLog()<<"Available worlds:"<<std::endl;
        globalWorldFactoryList().printWorlds();
    }
}

bool Engine::goToNextWorld() {
	if(getCurrentWorld()) {
		getCurrentWorld()->endExperience();
	}
	currentWorld++;
	if(!getCurrentWorld())return false;
	getCurrentWorld()->beginExperience();
	return true;
}
