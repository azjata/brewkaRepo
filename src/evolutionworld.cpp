#include "evolutionworld.h"
#include "ofxAssimpModelLoader.h"
#include "glm/gtc/noise.hpp"

static ShaderResource landscapeShader("worlds/evolution/landscape.vert", "worlds/evolution/landscape.frag");
static ShaderResource botShader("worlds/evolution/bot.vert", "worlds/evolution/bot.frag");
static ShaderResource botPointsShader("worlds/evolution/botpoints.vert", "worlds/evolution/botpoints.frag");

static ShaderResource backgroundShader("worlds/evolution/background.vert", "worlds/evolution/background.frag");

static ShaderResource portalShader("worlds/evolution/portal.vert", "worlds/evolution/portal.frag");

float LANDSCAPE_SCALE = 50.0;

const int BOT_COUNT = 200000;

float NOISE_SCALE = 3000.0;
float POSITION_SCALE = 200.0;
float NOISE_PERSISTENCE = 0.25;
float MIGRATION_SCALE = 150.0;
float TIME_SCALE = 0.1;

float MIGRATION_START_TIME = 3.0;

float ZOOMING_START_TIME = 30.0;

float GATE_SOUND_START_TIME = 32.4;

float FLYING_SPEED = 2.5;
float ZOOMING_SPEED = 200.0;

float HUE0 = 20.0;
float HUE1 = 30.0;

float INSTANCE_THRESHOLD = 250.0;

ofVec3f PORTAL_POSITION = ofVec3f(0.0, 20.0, -1400.0);
float PORTAL_WIDTH = 200.0f;
float PORTAL_HEIGHT = 100.0f;

const float LIFTING_UP_SPEED = 5.0;
const float START_CAMERA_HEIGHT = -50.0;
const float END_CAMERA_HEIGHT = 30.0;

float PORTAL_CONTRAST = 1.7;
float PORTAL_BRIGHTNESS = 0.1;
float PORTAL_HUE = 0;
float PORTAL_SATURATION = 0.5;
float PORTAL_VALUE = 0.5;

ofVec3f randomDirectionInSphere(); //this is defined in electricworld.cpp

const std::string particleSounds[15] = {
"Single_Particle_1.ogg",
"Single_Particle_2.ogg",
"Single_Particle_3.ogg",
"Single_Particle_4.ogg",
"Single_Particle_5.ogg",
"Single_Particle_6.ogg",
"Single_Particle_7.ogg",
"Single_Particle_8.ogg",
"Single_Particle_9.ogg",
"Single_Particle_10.ogg",
"Single_Particle_11.ogg",
"Single_Particle_12.ogg",
"Single_Particle_13.ogg",
"Single_Particle_14.ogg",
"Single_Particle_15.ogg"
};

DEFINE_WORLD(EvolutionWorld)

EvolutionWorld::EvolutionWorld(Engine *engine):World(engine), ambientSound("worlds/evolution/sounds/Ambience.ogg", true, true) {
	// ctor
	voicePromptFileName = "sounds/voice_prompts/voice_prompt_evolution.ogg";

	if(NO_TILT)
		cameraPitch = 0.0;
	else
		cameraPitch = 30.0;
}
EvolutionWorld::~EvolutionWorld() {
	delete videoPlayerColor;
	//delete videoPlayerAlpha;
	delete gui;

	delete[] initialBotData;
}

float randomLinear (float low, float high) { //generates a random number from a linear pdf which increases from 0 from low to high
	return sqrt(ofRandom(0.0, 1.0)) * (high - low) + low;
}

void EvolutionWorld::setup() {
	cout << "EvolutionWorld::setup start" << endl;

	duration = WORLD_EVOLUTION::TOTAL_DURATION;
	World::setup();
	setupLandscape();
	setupBotResources();
	setupBotGeometry();
	setupSky();
	setupGUI();
	loadSounds();

	for (int i = 0; i < 100; ++i) {
		lastHandVelocities[0][i] = ofVec3f(0.0);
		lastHandVelocities[1][i] = ofVec3f(0.0);
	}

	setupInitialBotData();
	resetBotData();

	cameraPosition = ofVec3f(0.0, -START_CAMERA_HEIGHT, 0.0);
	liftingUp = true;

	glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);

	camera.setupPerspective(false, 90.0, 0.01, 2000);
	camera.setPosition(0.0, -START_CAMERA_HEIGHT, 0.0);

	playedVoicePrompt = false;

	portalPlane.set(PORTAL_WIDTH, PORTAL_HEIGHT);
	portalPlane.setPosition(PORTAL_POSITION);
#ifdef NO_VIDEO
	videoPlayerColor = NULL;
	//videoPlayerAlpha = NULL;
#else
	videoPlayerColor = new ofVideoPlayer();
	//videoPlayerAlpha = new ofVideoPlayer();
#ifdef _WIN32
	directShowPlayerColor = new ofDirectShowPlayer();
	//directShowPlayerAlpha = new ofDirectShowPlayer();
	ofPtr <ofBaseVideoPlayer> ptrVideoColor(directShowPlayerColor);
	//ofPtr <ofBaseVideoPlayer> ptrVideoAlpha(directShowPlayerAlpha);
	videoPlayerColor->setPlayer(ptrVideoColor);
	//videoPlayerAlpha->setPlayer(ptrVideoAlpha);
#endif

	ofDisableArbTex();
	videoPlayerColor->loadMovie("worlds/evolution/portal.mov");
	//videoPlayerAlpha->loadMovie("worlds/evolution/portal_alpha.mp4");
	ofEnableArbTex();
#endif

	//preload sounds
	GetALBuffer("worlds/evolution/sounds/Ambience.ogg");
	GetALBuffer("worlds/evolution/sounds/Gate_Impact.ogg");

	GetALBuffer("worlds/evolution/sounds/Left_Down_1.ogg");
	GetALBuffer("worlds/evolution/sounds/Left_Down_2.ogg");
	GetALBuffer("worlds/evolution/sounds/Left_Down_3.ogg");
	GetALBuffer("worlds/evolution/sounds/Right_Down_1.ogg");
	GetALBuffer("worlds/evolution/sounds/Right_Down_2.ogg");
	GetALBuffer("worlds/evolution/sounds/Right_Down_3.ogg");
	GetALBuffer("worlds/evolution/sounds/Left_Up_1.ogg");
	GetALBuffer("worlds/evolution/sounds/Left_Up_2.ogg");
	GetALBuffer("worlds/evolution/sounds/Left_Up_3.ogg");
	GetALBuffer("worlds/evolution/sounds/Right_Up_1.ogg");
	GetALBuffer("worlds/evolution/sounds/Right_Up_2.ogg");
	GetALBuffer("worlds/evolution/sounds/Right_Up_3.ogg");

	ofDisableArbTex();
	sandImage.loadImage("worlds/evolution/sand.png");
	sandImage.getTextureReference().setTextureWrap(GL_REPEAT, GL_REPEAT);
    ofEnableArbTex();

	ofDisableArbTex();
	backgroundImage.loadImage("worlds/evolution/background.png");
	backgroundImage.getTextureReference().setTextureWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	ofEnableArbTex();

	ofDisableArbTex();
	skyImage.loadImage("worlds/shared/starrysky.png");
	skyImage.getTextureReference().setTextureWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	ofEnableArbTex();


	ofDisableArbTex();
	botImage.loadImage("worlds/evolution/bot.jpg");
	botImage.getTextureReference().setTextureWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    ofEnableArbTex();

	GLfloat maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

	glBindTexture(GL_TEXTURE_2D, sandImage.getTextureReference().getTextureData().textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	glGenerateMipmap(GL_TEXTURE_2D);


	glBindTexture(GL_TEXTURE_2D, botImage.getTextureReference().getTextureData().textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	glGenerateMipmap(GL_TEXTURE_2D);

	// set portal uniforms
	portalShader().begin();
	portalShader().setUniform1i("u_colorVideoTexture", 0);
	//portalShader().setUniform1i("u_alphaVideoTexture", 1);
	portalShader().setUniform1f("u_contrast", PORTAL_CONTRAST);
	portalShader().setUniform1f("u_brightness", PORTAL_BRIGHTNESS);
	portalShader().end();

	playedGateSound = false;

	GetALBuffer(voicePromptFileName);

	cout << "EvolutionWorld::setup end" << endl;
}

ofVec3f aiVector3DToOfVec3fa (aiVector3D& aiVec) {
	return ofVec3f(aiVec.x, aiVec.y, aiVec.z);
}

ofVec2f randomPointInCircle (float radius) {
	float r = ofRandom(0.0, 1.0);
	float theta = ofRandom(0.0, 2.0 * PI);
	return ofVec2f(sqrt(r) * cos(theta), sqrt(r) * sin(theta)) * radius;
}

float GEOMETRY_SIZE = 4000.0;
int GEOMETRY_RESOLUTION = 250;
ofVec3f GEOMETRY_ORIGIN(-2000.0, 0.0, -3000.0);

void EvolutionWorld::setupLandscape() {
	vector<ofVec3f> vertices;
	vector<ofIndexType> indices;

	for (int zIndex = 0; zIndex < GEOMETRY_RESOLUTION; zIndex += 1) {
		for (int xIndex = 0; xIndex < GEOMETRY_RESOLUTION; xIndex += 1) {
			vertices.push_back(ofVec3f(
				((float)xIndex * GEOMETRY_SIZE) / (GEOMETRY_RESOLUTION - 1),
				0.0f,
				((float)zIndex * GEOMETRY_SIZE) / (GEOMETRY_RESOLUTION - 1)
			) + GEOMETRY_ORIGIN);
		}
	}

	for (int zIndex = 0; zIndex < GEOMETRY_RESOLUTION - 1; zIndex += 1) {
		for (int xIndex = 0; xIndex < GEOMETRY_RESOLUTION - 1; xIndex += 1) {
			int topLeft = zIndex * GEOMETRY_RESOLUTION + xIndex,
				topRight = topLeft + 1,
				bottomLeft = topLeft + GEOMETRY_RESOLUTION,
				bottomRight = bottomLeft + 1;

			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
			indices.push_back(bottomRight);
			indices.push_back(topRight);
			indices.push_back(topLeft);
		}
	}

	landscapeVbo.setVertexData(&vertices[0], vertices.size(), GL_STATIC_DRAW);
	landscapeVbo.setIndexData(&indices[0], indices.size(), GL_STATIC_DRAW);
}

void EvolutionWorld::setupBotResources() {
	//setup animation program
	GLuint animateVertexShader = glCreateShader(GL_VERTEX_SHADER);
	string source = ofBufferFromFile("worlds/evolution/animate.vert").getText();
	const char * sourcePointer = source.c_str();
    glShaderSource(animateVertexShader, 1, &sourcePointer, nullptr);
    glCompileShader(animateVertexShader);

	GLint isCompiled = 0;
	glGetShaderiv(animateVertexShader, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE) {
		GLint infoLength = 0;
		glGetShaderiv(animateVertexShader, GL_INFO_LOG_LENGTH, &infoLength);

		//The maxLength includes the NULL character
		GLchar* infoBuffer = new GLchar[infoLength];
        glGetShaderInfoLog(animateVertexShader, infoLength, &infoLength, infoBuffer);
		std::cout << infoBuffer << std::endl;
		delete [] infoBuffer;
	}

	GLuint animateGeometryShader = glCreateShader(GL_GEOMETRY_SHADER);
	string geometrySource = ofBufferFromFile("worlds/evolution/animate.geom").getText();
	const char * geometrySourcePointer = geometrySource.c_str();
	glShaderSource(animateGeometryShader, 1, &geometrySourcePointer, nullptr);
    glCompileShader(animateGeometryShader);

	GLint isGeometryCompiled = 0;
	glGetShaderiv(animateGeometryShader, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE) {
		GLint infoLength = 0;
		glGetShaderiv(animateGeometryShader, GL_INFO_LOG_LENGTH, &infoLength);

		//The maxLength includes the NULL character
		GLchar* infoBuffer = new GLchar[infoLength];
        glGetShaderInfoLog(animateGeometryShader, infoLength, &infoLength, infoBuffer);
		std::cout << infoBuffer << std::endl;
		delete [] infoBuffer;
	}

	animateProgram = glCreateProgram();
	glAttachShader(animateProgram, animateVertexShader);
	glAttachShader(animateProgram, animateGeometryShader);

	const char* outputs[11] = {"outPosition", "outVelocity", "outSize", "outColor", "outType", "outStartTime", "gl_NextBuffer", "instancePosition", "instanceVelocity", "instanceSize", "instanceColor"};

	glTransformFeedbackVaryings(animateProgram, 11, outputs, GL_INTERLEAVED_ATTRIBS);

	glLinkProgram(animateProgram);

	glGenQueries(1, &botInstanceCountQuery);


	//setup data buffers
	glGenBuffers(1, &botDataBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, botDataBuffer);
	glBufferData(GL_ARRAY_BUFFER, BOT_COUNT * sizeof(float) * 12, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &botDataTargetBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, botDataTargetBuffer);
	glBufferData(GL_ARRAY_BUFFER, BOT_COUNT * sizeof(float) * 12, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &botInstanceDataBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, botInstanceDataBuffer);
	glBufferData(GL_ARRAY_BUFFER, BOT_COUNT * sizeof(float) * 10, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

float fbm (glm::vec3 position, float octaves) {
	float total = 0.0;
	for (int i = 0; i < octaves; ++i) {
		total += glm::simplex(position * glm::pow(2.0f, float(i))) * glm::pow(2.0f, -float(i));
	}
	return total;
}

void EvolutionWorld::setupInitialBotData() {
	initialBotData = new float[BOT_COUNT * 12]; //position (3), velocity (3), size (1), color (3), type (1), startTime (1)

	ofColor color0 = ofColor::fromHsb(HUE0, 255.0, 200.0);
	ofColor color1 = ofColor::fromHsb(HUE1, 255.0, 200.0);

	for (int i = 0; i < BOT_COUNT; ++i) {
		ofVec3f position;
		float startTime;
		ofVec2f circlePoint(randomPointInCircle(50.0));
		do {
			position.set(circlePoint.x, circlePoint.y + 30.0, -1000.0 + ofRandom(-10.0, 10.0));
			startTime = ofRandom(MIGRATION_START_TIME + 35.0, MIGRATION_START_TIME);
		} while (fbm(glm::vec3(position.x / 50.0, position.y / 50.0, startTime), 1) * 0.5 + 0.5 < 0.5);

		//position
		initialBotData[i * 12] = position.x;
		initialBotData[i * 12 + 1] = position.y;
		initialBotData[i * 12 + 2] = position.z;

		//velocity
		initialBotData[i * 12 + 3] = 0.0;
		initialBotData[i * 12 + 4] = 0.0;
		initialBotData[i * 12 + 5] = 1.0;

		//size
		initialBotData[i * 12 + 6] = ofRandom(1.0, 2.0);

		float hueNoise = fbm(glm::vec3(position.x + 20123.0, position.y - 12321.123, startTime * 10.0 + 1232.1) / 50.0f, 1) * 0.5 + 0.5;

		ofColor color;

		if (hueNoise > 0.5) {
			color = color0;
		} else {
			color = color1;
		}
		//std::cout << color << std::endl;

		//color
		float scale = 1.0;
		initialBotData[i * 12 + 7] = (color.r / 255.0) * scale;
		initialBotData[i * 12 + 8] = (color.g / 255.0) * scale;
		initialBotData[i * 12 + 9] = (color.b / 255.0) * scale;

		//type
		if (hueNoise > 0.5) {
			initialBotData[i * 12 + 10] = 0.0;
		} else {
			initialBotData[i * 12 + 10] = 1.0;
		}

		//startTime
		initialBotData[i * 12 + 11] = startTime;
	}

	glBindBuffer(GL_ARRAY_BUFFER, botDataBuffer);
	glBufferData(GL_ARRAY_BUFFER, BOT_COUNT * sizeof(float) * 12, &initialBotData[0], GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void EvolutionWorld::resetBotData() {
	glBindBuffer(GL_ARRAY_BUFFER, botDataBuffer);
	glBufferData(GL_ARRAY_BUFFER, BOT_COUNT * sizeof(float) * 12, &initialBotData[0], GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void EvolutionWorld::setupBotGeometry() {
	ofxAssimpModelLoader model;
	model.loadModel("worlds/evolution/bot.dae");

	const aiScene *scene = model.getAssimpScene();
	aiMesh* mesh = scene->mMeshes[0];

	float * vertexPositions = new float[mesh->mNumVertices * 3];
	float * vertexNormals = new float[mesh->mNumVertices * 3];
	float * textureCoordinates = new float[mesh->mNumVertices * 2];
	int * indices = new int[mesh->mNumFaces * 3];

	botNumIndices = mesh->mNumFaces * 3;

	float scale = 0.02;

	for (int i = 0; i < mesh->mNumVertices; ++i) {
		for (int j = 0; j < 3; ++j) {
			vertexPositions[i*3 + j] = mesh->mVertices[i][j] * scale;
			vertexNormals[i*3 + j] = mesh->mNormals[i][j];
		}
		textureCoordinates[i*2] = mesh->mTextureCoords[0][i].x;
		textureCoordinates[i*2 + 1] = mesh->mTextureCoords[0][i].y;
	}

	float minX = 10000.0, minY = 10000.0, minZ = 10000.0;
	float maxX = -100000.0, maxY = -100000.0, maxZ = -100000.0;

	for (int i = 0; i < mesh->mNumVertices; ++i) {
		float x = vertexPositions[i * 3];
		float y = vertexPositions[i * 3 + 1];
		float z = vertexPositions[i * 3 + 2];

		minX = min(x, minX);
		minY = min(y, minY);
		minZ = min(z, minZ);

		maxX = max(x, maxX);
		maxY = max(y, maxY);
		maxZ = max(z, maxZ);

	}

	std::cout << "bot diameter" << ofVec3f(minX, minY, minZ).distance(ofVec3f(maxX, maxY, maxZ)) << std::endl;

	//std::cout << minX << ',' << minY << ',' << minZ << std::endl;
	//std::cout << maxX << ',' << maxY << ',' << maxZ << std::endl;

	//faces from assimp will always be triangular
	for (int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace& face = mesh->mFaces[i];
		for (int j = 0; j < 3; ++j) {
			indices[i*3 + j] = face.mIndices[j];
		}
	}

	glGenBuffers(1, &botVertexPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, botVertexPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, &vertexPositions[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &botVertexNormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, botVertexNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, &vertexNormals[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &botTextureCoordinatesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, botTextureCoordinatesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * mesh->mNumVertices, &textureCoordinates[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &botIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, botIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 3 * mesh->mNumFaces, &indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	delete[] vertexPositions;
	delete[] vertexNormals;
	delete[] textureCoordinates;
	delete[] indices;
}

void EvolutionWorld::setupSky() {
	backgroundSphere.set(1500.0, 32);
}

void EvolutionWorld::setupGUI() {
	gui = new ofxUIScrollableCanvas();

	gui->addButton("RESET", false);

	gui->addSlider("NOISE SCALE", 0.0, 10000.0, &NOISE_SCALE);
	gui->addSlider("POSITION SCALE", 0.0, 1000.0, &POSITION_SCALE);
	gui->addSlider("NOISE PERSISTENCE", 0.0, 1.0, &NOISE_PERSISTENCE);
	gui->addSlider("MIGRATION SCALE", 0.0, 1000.0, &MIGRATION_SCALE);
	gui->addSlider("TIME SCALE", 0.0, 10.0, &TIME_SCALE);

	gui->addSlider("INSTANCE THRESHOLD", 0.0, 1000.0, &INSTANCE_THRESHOLD);
	gui->addSlider("HUE 0", 0.0, 255.0, &HUE0);
	gui->addSlider("HUE 1", 0.0, 255.0, &HUE1);

	gui->addSlider("PORTAL BRIGHTNESS", -3.0, 3.0, &PORTAL_BRIGHTNESS);
	gui->addSlider("PORTAL CONTRAST", 0.0, 3.0, &PORTAL_CONTRAST);

	gui->addSlider("PORTAL HUE", -1.0, 1.0, &PORTAL_HUE);
	gui->addSlider("PORTAL SATURATION", -1.0, 1.0, &PORTAL_SATURATION);
	gui->addSlider("PORTAL VALUE", -1.0, 1.0, &PORTAL_VALUE);

	gui->addButton("SAVE", false);

	gui->autoSizeToFitWidgets();
	ofAddListener(gui->newGUIEvent, this, &EvolutionWorld::guiEvent);

	gui->loadSettings("worlds/evolution/settings.xml");

	gui->setVisible(false);
}

void EvolutionWorld::guiEvent(ofxUIEventArgs &event) {
	if (event.widget->getName() == "RESET") {
		reset();
	} else if (event.widget->getName() == "SAVE") {
		gui->saveSettings("worlds/evolution/settings.xml");
	}
}

void EvolutionWorld::reset() {
	cout << "EvolutionWorld: reset start" << endl;
	duration = WORLD_EVOLUTION::TOTAL_DURATION;
	resetBotData();
	currentTime = 0.0;

	for (int i = 0; i < 100; ++i) {
		lastHandVelocities[0][i] = ofVec3f(0.0);
		lastHandVelocities[1][i] = ofVec3f(0.0);
	}

	cameraPosition = ofVec3f(0.0, START_CAMERA_HEIGHT, 0.0);
	playedVoicePrompt = false;

	liftingUp = true;

	playedGateSound = false;

	lastInteractionSoundStartTime[0] = -100.0;
	lastInteractionSoundStartTime[1] = -100.0;

	#ifndef NO_VIDEO
	videoPlayerColor->setPaused(true);
	//videoPlayerAlpha->setPaused(true);

	videoPlayerColor->setFrame(0);
	//videoPlayerAlpha->setFrame(0);
	#endif

	cout << "EvolutionWorld: reset end" << endl;
}

void EvolutionWorld::beginExperience() {
	OSCManager::getInstance().startWorld(COMMON::WORLD_TYPE::EVOLUTION);
	OSCManager::getInstance().sendStatus("LIFT USER");
	std::cout << "EvolutionWorld::beginExperience()*****************************************" << std::endl; 

	currentTime = 0.0;

#ifndef NO_VIDEO
	videoPlayerColor->setPaused(false);
	//videoPlayerAlpha->setPaused(false);
	videoPlayerColor->play();
	//videoPlayerAlpha->play();
#endif

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_INDEX_ARRAY);

	for (int i = 0; i < 20; ++i) {
		glDisableVertexAttribArray(i);
		glVertexAttribDivisor(i, 0);
	}

	GlobAudioManager().AddSoundObject(&ambientSound);

	setupSounds();
}

void EvolutionWorld::endExperience() {
	std::cout << "EvolutionWorld::endExperience()*****************************************" << std::endl;
	OSCManager::getInstance().endWorld(COMMON::WORLD_TYPE::EVOLUTION);
	playedVoicePrompt = false;
	removeSounds();

	GlobAudioManager().RemoveSoundObject(&ambientSound);
	std::cout << "EvolutionWorld::endExperience()*****************************************" << std::endl;
}

void EvolutionWorld::exit() {
}

//defined in electric world...
float clamp (float x, float low, float high);
float smoothstep(float edge0, float edge1, float x);

void EvolutionWorld::updateCamera(float dt) {
	if (liftingUp) {
		cameraPosition.y += LIFTING_UP_SPEED * dt;

		if (cameraPosition.y > END_CAMERA_HEIGHT) {
			cameraPosition.y = END_CAMERA_HEIGHT;
			liftingUp = false;
		}
	} else {
		cameraPosition.z -= FLYING_SPEED * dt;
	}

	if (currentTime > ZOOMING_START_TIME) {
		speed = smoothstep(ZOOMING_START_TIME, ZOOMING_START_TIME + 5.0, currentTime) * ZOOMING_SPEED;
		cameraPosition.z -= speed * dt;

		if (cameraPosition.z <= PORTAL_POSITION.z + ZOOMING_SPEED * dt && !engine->isSingleWorld()) {
			duration = 0.0;
		}
	}

	camera.setPosition(cameraPosition);

	if(NO_TILT) {
		camera.setOrientation(ofVec3f(0.0, 0.0, 0.0));
	} else {
		if (!liftingUp) {
			camera.setOrientation(ofVec3f(cameraPitch, 0.0, 0.0));
		} else {
			camera.setOrientation(ofVec3f(clamp((cameraPosition.y - START_CAMERA_HEIGHT) / (END_CAMERA_HEIGHT - START_CAMERA_HEIGHT), 0.0, 1.0) * cameraPitch, 0.0, 0.0));
		}	
	}
}

int VELOCITY_AVERAGE_WINDOW = 10;
float INTERACTION_SOUND_DURATION = 2.0;

glm::vec3 ofVec3fToGlmVec333 (ofVec3f vec) { //this is so dumb...
	return glm::vec3(vec.x, vec.y, vec.z);
}

void EvolutionWorld::update(float dt){
	updateCamera(dt);

#ifndef NO_VIDEO
	videoPlayerColor->update();
	//videoPlayerAlpha->update();
#endif

	// play audio cue only once and after 3 seconds
	if (!playedVoicePrompt && Timeline::getInstance().getElapsedCurrWorldTime() >= WORLD_EVOLUTION::DURATION_INTRO + COMMON::AUDIO_CUE_DELAY) {
		playedVoicePrompt = true;
		PlayBackgroundSound(voicePromptFileName, COMMON::VOICE_PROMPT_GAIN);
	}

	if (!playedGateSound && Timeline::getInstance().getElapsedCurrWorldTime() >= GATE_SOUND_START_TIME) {
		playedGateSound = true;
		PlayBackgroundSound("worlds/evolution/sounds/Gate_Impact.ogg", 1.0f);
	}

	ambientSound.sound_gain = smoothstep(5.0, 15.0, Timeline::getInstance().getElapsedCurrWorldTime());

	ofVec3f leftAverageVelocity;
	ofVec3f rightAverageVelocity;

	for (int handIndex = 0; handIndex < 2; ++handIndex) {
		ofVec3f averageVelocity(0.0, 0.0, 0.0);

		ofVec3f handPosition;

		if (engine->kinectEngine.isKinectDetected()) {
			handPosition = engine->kinectEngine.getHandsPosition()[handIndex];
		} else {
			handPosition = ofVec3f(mousePosition.x, 1024.0 - mousePosition.y, 0.0) / 1024.0;
		}
		currentHandPositions[handIndex] = handPosition;
		ofVec3f velocity = (handPosition - lastHandPositions[handIndex]) / dt;

		//there are 100 previous gesture velocities stored
		for (int j = 0; j < 100 - 1; ++j) {
			lastHandVelocities[handIndex][j] = lastHandVelocities[handIndex][j + 1];
		}
		lastHandVelocities[handIndex][99] = velocity;

		for (int j = 99 - VELOCITY_AVERAGE_WINDOW; j < 100; ++j) {
			averageVelocity += lastHandVelocities[handIndex][j] / float(VELOCITY_AVERAGE_WINDOW);
		}

		if (handIndex == 0) leftAverageVelocity = averageVelocity;
		if (handIndex == 1) rightAverageVelocity = averageVelocity;

		lastHandPositions[handIndex] = currentHandPositions[handIndex];

		float time = Timeline::getInstance().getElapsedCurrWorldTime();

		if (averageVelocity.length() > 1.0 && previousAverageVelocities[handIndex].length() < 1.0 && time > 5.0) {
			lastInteractionSoundStartTime[handIndex] = time;

			if (averageVelocity.x >= 0.0 && averageVelocity.y >= 0.0) { //right up
				PlayBackgroundSound("worlds/evolution/sounds/Right_Up_" + std::to_string(int(ofRandom(1, 4))) + ".ogg", 1.0);
			} else if (averageVelocity.x < 0.0 && averageVelocity.y >= 0.0) { //left up
				PlayBackgroundSound("worlds/evolution/sounds/Left_Up_" + std::to_string(int(ofRandom(1, 4))) + ".ogg", 1.0);
			} else if (averageVelocity.x >= 0.0 && averageVelocity.y < 0.0) { //right down
				PlayBackgroundSound("worlds/evolution/sounds/Right_Down_" + std::to_string(int(ofRandom(1, 4))) + ".ogg", 1.0);
			} else if (averageVelocity.x < 0.0 && averageVelocity.y < 0.0) { //left down
				PlayBackgroundSound("worlds/evolution/sounds/Left_Down_" + std::to_string(int(ofRandom(1, 4))) + ".ogg", 1.0);
			}
		}
	}

	previousAverageVelocities[0] = leftAverageVelocity;
	previousAverageVelocities[1] = rightAverageVelocity;

	if (dt > 0.1) dt = 0.0; //prevent too large time steps

	currentTime += dt;

	glUseProgram(animateProgram);
	glUniform1f(glGetUniformLocation(animateProgram, "u_deltaTime"), dt);
	glUniform1f(glGetUniformLocation(animateProgram, "u_time"), currentTime);

	glUniform1f(glGetUniformLocation(animateProgram, "u_timeScale"), TIME_SCALE);
	glUniform1f(glGetUniformLocation(animateProgram, "u_noiseScale"), NOISE_SCALE);
	glUniform1f(glGetUniformLocation(animateProgram, "u_migrationScale"), MIGRATION_SCALE);
	glUniform1f(glGetUniformLocation(animateProgram, "u_positionScale"), POSITION_SCALE);
	glUniform1f(glGetUniformLocation(animateProgram, "u_noisePersistence"), NOISE_PERSISTENCE);

	glUniform1f(glGetUniformLocation(animateProgram, "u_threshold"), INSTANCE_THRESHOLD);
	glUniform3fv(glGetUniformLocation(animateProgram, "u_cameraPosition"), 1, &cameraPosition.x);

	glUniform3fv(glGetUniformLocation(animateProgram, "u_leftHandVelocity"), 1, &leftAverageVelocity.x);
	glUniform3fv(glGetUniformLocation(animateProgram, "u_rightHandVelocity"), 1, &rightAverageVelocity.x);

	ofColor color0 = ofColor::fromHsb(HUE0, 255.0, 200.0);
	ofColor color1 = ofColor::fromHsb(HUE1, 255.0, 200.0);

	glUniform3f(glGetUniformLocation(animateProgram, "u_color0"), color0.r / 255.0, color0.g / 255.0, color0.b / 255.0);
	glUniform3f(glGetUniformLocation(animateProgram, "u_color1"), color1.r / 255.0, color1.g / 255.0, color1.b / 255.0);

	glEnable(GL_RASTERIZER_DISCARD);
	glBindBuffer(GL_ARRAY_BUFFER, botDataBuffer);

	char* offset = 0;
	glEnableVertexAttribArray(0); //position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset);
	glEnableVertexAttribArray(1); //velocity
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset + 3 * sizeof(float));
	glEnableVertexAttribArray(2); //size
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset + 6 * sizeof(float));
	glEnableVertexAttribArray(3); //color
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset + 7 * sizeof(float));
	glEnableVertexAttribArray(4); //type
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset + 10 * sizeof(float));
	glEnableVertexAttribArray(5); //start time
	glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset + 11 * sizeof(float));

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, botDataTargetBuffer);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, botInstanceDataBuffer);

	glBeginQueryIndexed(GL_PRIMITIVES_GENERATED, 1, botInstanceCountQuery);

	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, 0, BOT_COUNT);
	glEndTransformFeedback();

	glEndQueryIndexed(GL_PRIMITIVES_GENERATED, 1);

	std::swap(botDataBuffer, botDataTargetBuffer);
	glDisable(GL_RASTERIZER_DISCARD);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(5);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, 0);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, botDataBuffer);

	GLfloat *bufferValues = new GLfloat[RETURN_POSITIONS * 12];

	glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(GLfloat) * RETURN_POSITIONS * 12, bufferValues);


	for (int i = 0; i < RETURN_POSITIONS; ++i) {
		float startTime = bufferValues[i * 12 + 11];
		if (currentTime > startTime) {
			positions[i] = ofVec3f(bufferValues[i * 12], bufferValues[i * 12 + 1], bufferValues[i * 12 + 2]);
			colors[i] = ofVec3f(bufferValues[i * 12 + 7], bufferValues[i * 12 + 8], bufferValues[i * 12 + 9]);
		} else {
			positions[i] = ofVec3f(9999999999, 99999999, 99999999999);
			colors[i] = ofVec3f(0.0);
		}
	}

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

	delete [] bufferValues;

	updateSoundPositions();
}

bool EvolutionWorld::getUseBloom () {
	return true;
}

BloomParameters EvolutionWorld::getBloomParams() {
	BloomParameters params;

	params.bloom_multiplier_color = glm::mix(glm::vec3(0.2), glm::vec3(0.0), glm::smoothstep(-1000.0f, -1200.0f, cameraPosition.z));
	//params.bloom_multiplier_color = glm::vec3(0.2);
	params.gamma=1.0;
	params.exposure=1.0;
	params.multisample_antialiasing = 16;

	//params.tonemap_method=1;
	return params;
}

void EvolutionWorld::drawScene(renderType render_type, ofFbo *fbo) {
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);

	ofDisableArbTex();

	//easyCam.begin();
	//easyCam.setNearClip(0.001);
	//easyCam.enableMouseInput();
	//easyCam.setTarget(ofVec3f(0.0, 1.8, 0.0));

	backgroundShader().begin();
	backgroundShader().setUniform1i("u_backgroundTexture", 0);
	backgroundShader().setUniform1i("u_skyTexture", 1);
	backgroundShader().setUniform3fv("u_cameraPosition", &cameraPosition.x);
	backgroundShader().setUniform1f("u_mixFraction", clamp(currentTime / 2.0, 0.0, 1.0));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, backgroundImage.getTextureReference().getTextureData().textureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, skyImage.getTextureReference().getTextureData().textureID);
	glActiveTexture(GL_TEXTURE0);
	backgroundSphere.draw();
	backgroundShader().end();

	vector<float> lightPositions;
	vector<float> lightColors;

	for (int i = 0; i < RETURN_POSITIONS; ++i) {
		lightPositions.push_back(positions[i].x);
		lightPositions.push_back(positions[i].y);
		lightPositions.push_back(positions[i].z);

		lightColors.push_back(colors[i].x);
		lightColors.push_back(colors[i].y);
		lightColors.push_back(colors[i].z);
	}

	landscapeShader().begin();
	if (lightPositions.size() > 0 && lightColors.size() > 0) {
		glUniform3fv(glGetUniformLocation(landscapeShader().getProgram(), "lightPositions"), lightPositions.size() / 3, &lightPositions[0]);
		glUniform3fv(glGetUniformLocation(landscapeShader().getProgram(), "lightColors"), lightColors.size() / 3, &lightColors[0]);
	}
	landscapeShader().setUniform1i("u_texture", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sandImage.getTextureReference().getTextureData().textureID);
	landscapeVbo.drawElements(GL_TRIANGLES, landscapeVbo.getNumIndices());
	landscapeShader().end();

	// draw portal
#ifndef NO_VIDEO
	unsigned int colorVideoTextureID = videoPlayerColor->getTextureReference().getTextureData().textureID;
	//unsigned int alphaVideoTextureID = videoPlayerAlpha->getTextureReference().getTextureData().textureID;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorVideoTextureID);
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, alphaVideoTextureID);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	portalShader().begin();
	portalShader().setUniform3fv("u_cameraPosition", &cameraPosition.x, 1);
	portalShader().setUniform1f("u_contrast", PORTAL_CONTRAST);
	portalShader().setUniform1f("u_brightness", PORTAL_BRIGHTNESS);
	portalShader().setUniform1f("u_hue", PORTAL_HUE);
	portalShader().setUniform1f("u_saturation", PORTAL_SATURATION);
	portalShader().setUniform1f("u_value", PORTAL_VALUE);
	portalPlane.draw();
	portalShader().end();
#endif

	glDisable(GL_BLEND);
	glActiveTexture(GL_TEXTURE0);

	//draw bots as points
	glBindBuffer(GL_ARRAY_BUFFER, botDataBuffer);

	char* offset = 0;
	glEnableVertexAttribArray(0); //position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset);
	glEnableVertexAttribArray(1); //velocity
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset + 3 * sizeof(float));
	glEnableVertexAttribArray(2); //size
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset + 6 * sizeof(float));
	glEnableVertexAttribArray(3); //color
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset + 7 * sizeof(float));
	glEnableVertexAttribArray(4); //start time
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), offset + 11 * sizeof(float));

	glDepthMask(GL_FALSE);

	botPointsShader().begin();
	botPointsShader().setUniform1f("u_time", currentTime);
	botPointsShader().setUniform3fv("u_cameraPosition", &cameraPosition.x, 1);
	glDrawArrays(GL_POINTS, 0, BOT_COUNT);
	botPointsShader().end();

	glDepthMask(GL_TRUE);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);

	//draw bots instanced

	int botInstanceCount = 0;
	glGetQueryObjectiv(botInstanceCountQuery, GL_QUERY_RESULT, &botInstanceCount);

	glBindBuffer(GL_ARRAY_BUFFER, botVertexPositionBuffer);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(4);
	glBindBuffer(GL_ARRAY_BUFFER, botVertexNormalBuffer);
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(5);
	glBindBuffer(GL_ARRAY_BUFFER, botTextureCoordinatesBuffer);
	glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(6);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, botIndexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, botInstanceDataBuffer);

	glEnableVertexAttribArray(0); //position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), offset);
	glEnableVertexAttribArray(1); //velocity
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), offset + 3 * sizeof(float));
	glEnableVertexAttribArray(2); //size
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), offset + 6 * sizeof(float));
	glEnableVertexAttribArray(3); //color
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), offset + 7 * sizeof(float));

	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);

	botShader().begin();
	botShader().setUniform3fv("u_cameraPosition", &cameraPosition.x);
	botShader().setUniform1i("u_texture", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, botImage.getTextureReference().getTextureData().textureID);
	glDrawElementsInstanced(GL_TRIANGLES, botNumIndices, GL_UNSIGNED_INT, 0, botInstanceCount);
	botShader().end();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(5);
	glDisableVertexAttribArray(6);

	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glVertexAttribDivisor(3, 0);

	renderBody();
	//easyCam.end();
}

void EvolutionWorld::renderBody()
{
	float transitionTime = 6.0f;
	if(Timeline::getInstance().getElapsedCurrWorldTime() < transitionTime)
	{
		float opacity = Timeline::getInstance().getElapsedCurrWorldTime()/transitionTime; // 0 to 1
		engine->kinectEngine.drawBodyBlack(cameraPosition);
		vector<float> lightPositions, lightColors, lightAttenuations;
		engine->kinectEngine.drawBodyElectric(cameraPosition, (1.0f - opacity), lightPositions, lightColors, lightAttenuations, 0, 0);
		engine->kinectEngine.drawBodyEvolution(cameraPosition, opacity);
	}
	else
		engine->kinectEngine.drawBodyEvolution(cameraPosition, 1.0f);
}

void EvolutionWorld::loadSounds()
{
	for(int i = 0; i < 15; i++)
		 GetALBuffer(particleSounds[i]);

	for(int i = 0; i < RETURN_POSITIONS; i++)
	{
		int rand = ofRandom(15);

		soundObjects[i].al_buffer = GetALBuffer("worlds/evolution/sounds/" + particleSounds[rand]);

		soundObjects[i].sound_flags = sound_flag_repeat;
		soundObjects[i].pos = glm::vec3(positions[i].x, positions[i].y, positions[i].z);
		soundObjects[i].sound_gain = 0.0;

		sObjectsActive[i] = false;
	}
}

void EvolutionWorld::setupSounds()
{
	for(int i = 0; i < RETURN_POSITIONS; i++)
	{
		GlobAudioManager().AddSoundObject(&soundObjects[i]);
	}
}

void EvolutionWorld::updateSoundPositions()
{
	for(int i = 0; i < RETURN_POSITIONS; i++)
	{
		soundObjects[i].pos = glm::vec3(positions[i].x, positions[i].y, positions[i].z);

		if(positions[i].distance(cameraPosition) < 200.0)
		{
			soundObjects[i].sound_gain = 25.0;
		}
		else
			soundObjects[i].sound_gain = 0.0;
	}
}

void EvolutionWorld::removeSounds()
{
	for(int i = 0; i < RETURN_POSITIONS; i++)
	{
		GlobAudioManager().RemoveSoundObject(&soundObjects[i]);
	}
}

float EvolutionWorld::getDuration() {
	return duration;
}

void EvolutionWorld::keyPressed(int key) {
	if (key == 'g') {
		gui->toggleVisible();
	}

	World::keyPressed(key);
}

void EvolutionWorld::mousePressed(int x, int y, int button) {
	PlayBackgroundSound("worlds/evolution/sounds/Right_Up_1.ogg", 1.0);
}

void EvolutionWorld::mouseMoved(int x, int y) {
	mousePosition = ofVec2f(static_cast<float>(x), static_cast<float>(y));
}
