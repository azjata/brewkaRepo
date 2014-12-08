#ifndef ELECTRICWORLD_H
#define ELECTRICWORLD_H

#pragma once

#include "engine.h"
#include "ofxUI.h"

#include "ofxAssimpModelLoader.h"

#include "audio.h"

class Segment {
public:
	Segment(const ofVec3f start_, const ofVec3f end_, const int forkLevel_) : start(start_), end(end_), forkLevel(forkLevel_) {};
	ofVec3f start;
	ofVec3f end;
	int forkLevel;

	ofFloatColor getColor() const;
};

class Tile;

class Lightning {
public:
	Lightning(ofVec3f start_, ofVec3f end_, float totalEnergy_, Tile* tile_, float minEnergyFraction_, float maxEnergyFraction_) : startPoint(start_), endPoint(end_), initialEnergy(totalEnergy_), totalEnergy(totalEnergy_), energy(0.0), tile(tile_), minEnergyFraction(minEnergyFraction_), maxEnergyFraction(maxEnergyFraction_) {};
	
	int type; //0 = discharge, 1 = spark

	//TODO: use multiple inheritance
	int hand; //0 = left, 1 = right
	//for discharge
	Tile* tile;
	//for spark
	int startVertexIndex;
	int endVertexIndex;

	vector<float> randomNumbers; //the random numbers that parametize the current lightning shape

	ofVec3f startPoint;
	ofVec3f endPoint;
	vector<ofVec3f> vertexPositions;
	vector<ofFloatColor> vertexColors;
	ofVbo vbo;

	float minDisplacement;
	float maxDisplacement;

	float minEnergyFraction;
	float maxEnergyFraction;

	float minForkLength;
	float maxForkLength;

	float initialEnergy;

	float minEnergy;
	float maxEnergy;

	float totalEnergy;
	float energy;
};

//CCW winding order
class HalfEdge {
public:
	ofVec3f startVertex;
	ofVec3f endVertex;
	ofVec3f direction; //normalized
	float length;
	HalfEdge* opposite; 
	Tile* tile; //remember: face and opposite can be null (no adjacent face)
	HalfEdge* next;
};

//a tile is a face in the half-edge data structure
class Tile {
public:
	Tile(vector<ofVec3f> vertices, vector<ofVec2f> textureCoordinates, ofVec3f normal, ofVec3f tangent, ofVec3f bitangent);

	vector<ofVec3f> vertices;
	vector<ofVec2f> textureCoordinates;

	ofVec3f tangent;
	ofVec3f bitangent;

	vector<HalfEdge> halfEdges;

	ofVec3f normal;

	//before flying in
	ofVec3f startOffset;
	//encode separately to allow for multiple complete rotations
	ofVec3f rotationAxis;
	float rotationAngle;
	float assemblyStartTime;
	float assemblyEndTime;

	float area;
	ofVec3f centroid;

	float energy;
	float maxEnergy;

	float offset; //vertical (y) offset

	bool isConnected;

	bool intersectsRay (const ofVec3f &origin, const ofVec3f &direction, ofVec3f &intersection, float &distance) const;
	void addEnergy (const float amount);
	void subtractEnergy (const float amount);
};

class Light {
public:
	Light(const ofVec3f position_, const ofVec3f direction_, const float maxDot_, const float attenuation_, const float intensity_) : 
		position(position_),
		direction(direction_),
		maxDot(maxDot_),
		attenuation(attenuation_),
		intensity(intensity_) {};
	ofVec3f position;
	ofVec3f direction;
	float maxDot;
	float attenuation;
	float intensity;
};

class Hand {
public:
	float energy;
};

class ElectricWorld: public World {
public:
	DECLARE_WORLD
	ElectricWorld(Engine *engine);
	virtual ~ElectricWorld();

	void setup();
	virtual void beginExperience();
	virtual void endExperience();
	void update (float dt);

	void reset();

	void exit();

	void drawScene (renderType render_type, ofFbo *fbo);
	float getDuration();

	virtual void keyPressed(int key);
	virtual void keyReleased(int key);
	virtual void drawDebugOverlay();

	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);

	BloomParameters getBloomParams();

	private:
		void setupGUI();
		void loadTextures();
		void setupCylinderAndNodes();
		void setupDissolvedCylinder();
		void loadSounds();

		void updateCamera (float dt);
		void updateTiles (float dt);
		void updateLightning (float dt);
		void updateSounds (float dt);

		void autoTriggerDischarge();
		void dischargeHand (int handIndex, ofVec3f direction);

		bool getUseBloom ();

		Tile* getIntersectedTile (ofVec3f origin, ofVec3f direction);

		void renderBody(vector<float> *lightPositions, vector<float> *lightColors, vector<float> *lightAttenuations);

		ofImage slateImage;

		ofxUICanvas *gui;
		void guiEvent(ofxUIEventArgs &e);

		float userHeight;
		float userSpeed;
		float triggerStartTime;
		bool hasTriggered;

		vector<Tile*> chargedTiles; //tiles with energy > 0

		vector<Light> lights;

		vector<Lightning> lightnings;

		vector<Tile> tiles;

		ofVbo disassembledCylinderVbo;

		ofVbo cylinderVbo;
		ofVbo chargedTilesVbo;

		Hand hands[2];

		ofEasyCam easyCam; //for easyTesting

		ofxAssimpModelLoader cylinderModel;

		ofVec3f lastHandPositions[2];
		ofVec3f currentHandPositions[2];

		ofVec3f lastGestureVelocities[2][100];

		SoundObject crackleSounds[2];
		SoundObject chargeSounds[2];

		ofVec3f mouseDirection;

		ofSpherePrimitive backgroundSphere;
		StrongPtr<Texture> environment;

		ofImage skyImage;

		ofVbo landscapeVbo;

		ofVbo skyVbo;

		ofVbo dissolvedCylinderVbo;

		bool playedTransitionSound;
		bool playedAmbientSound;
		bool isNextWorldBody;

		SoundObject ambientSound;
};

#endif