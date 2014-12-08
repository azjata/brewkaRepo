#include "electricworld.h"

#include "resourcemanager.h"
#include "bloom.h"

#include "assimp.h"
#include "aiScene.h"

#include "glm/detail/func_noise.hpp"

#include <limits>

//the base of the cylinder is at the origin in its model space

static ShaderResource background_shader("worlds/electric/background.vert", "worlds/electric/background.frag");
static ShaderResource skyShader("worlds/electric/sky.vert", "worlds/electric/sky.frag");

const ofVec3f SKY_PLANE_POSITION(-150.0, 300.0, -150.0); //relative to user
const float SKY_PLANE_WIDTH = 300.0;
const float SKY_PLANE_HEIGHT = 300.0;

const float ACCELERATION = 10.0;

const float SLOW_MOVEMENT_SPEED = 15.0;

ofVec3f GAP_COLOR = ofVec3f(0.5, 1.0, 0.5);

int GESTURE_WINDOW = 15;
float MIN_GESTURE_SPEED = 0.5;

float MIN_GESTURE_DOT = 0.0;

float MOVEMENT_START_TIME = 0.1;

float ASSEMBLY_START_TIME = 1.0;

const float INFINITE_BASELINE = 200.0;

float GAP_WIDTH = 0.25;

const float CYLINDER_HEIGHT = 200.0;
const int CYLINDERS = 4;
const float INFINITE_TOPLINE = INFINITE_BASELINE + CYLINDER_HEIGHT * CYLINDERS;

const float HAND_CHARGE_RATE = 1; //all energy originates from hands and so associated values are all relative to this base charge rate

float HAND_SPARK_PROBABILITY = 1.0 / 25.0;

float HAND_MAX_ENERGY = 5.0;

float HAND_MIN_DISCHARGE_ENERGY = 2.5;

float AMBIENCE_GAIN = 1.0;
float DISCHARGE_GAIN = 25.0;
float IMPACT_GAIN = 25.0;
float GLOW_GAIN = 10.0;
float CRACKLE_GAIN = 1.0;
float CHARGE_GAIN = 1.0;

float MOVEMENT_SPEED = 0.5;

float TILE_DISCHARGE_RATE = 1.0;

float LIGHTNING_DISCHARGE_RATE = 4.0;

float DISCHARGE_LIGHTNING_MIN_DISPLACEMENT;
float DISCHARGE_LIGHTNING_MAX_DISPLACEMENT;

float SPARK_LIGHTNING_MIN_DISPLACEMENT;
float SPARK_LIGHTNING_MAX_DISPLACEMENT;

float TILE_ENERGY_FLOW_RATE = 1500.0;

int LIGHTNING_LEVELS = 5;

float FORK_PROBABILITY = 0;

float MIN_FORK_FRACTION = 0.3;
float MAX_FORK_FRACTION = 0.5;

float DISPLACEMENT_DECAY = 2.0;

float MIN_FORK_LENGTH = 0.0;
float MAX_FORK_LENGTH = 1.0;
float MAX_FORK_ANGLE = PI / 8;
float FORK_DECAY = 2.0;

ofFloatColor LIGHTNING_COLOR(150.0, 300.0, 150.0, 1.0); //this is scaled by energy
const float COLOR_DECAY = 2.0;

ofFloatColor TILE_ON_COLOR(1.0, 2.5, 1.0); //this is scaled by energy

float ROUGHNESS = 0.2;
float IOR = 1.3;

float LIGHTNING_LIGHT_ATTENUATION = 2.5;
float LIGHTNING_LIGHT_INTENSITY = 4.0;

float TILE_LIGHT_ATTENUATION = 4.0;
float TILE_LIGHT_INTENSITY = 1.0;

float AMBIENT_LIGHT_INTENSITY = 1.0; //attenuation = 0.0

float AUTO_TRIGGER_TIME = 4.0;

const float DECELERATION = 5.0;
const float DECELERATION_START_HEIGHT = INFINITE_TOPLINE;

const float TRANSITION_SPEED = 10.0;

float DISSOLVED_CYLINDER_HEIGHT = CYLINDER_HEIGHT / 2.0;

const float EVOLUTION_BASELINE = INFINITE_TOPLINE + DISSOLVED_CYLINDER_HEIGHT + 40.0;

static ShaderResource lightningShader("worlds/electric/lightning.vert", "worlds/electric/lightning.frag");
static ShaderResource tileShader("worlds/electric/tile.vert", "worlds/electric/tile.frag");
static ShaderResource chargedTileShader("worlds/electric/chargedtile.vert", "worlds/electric/chargedtile.frag");

glm::vec3 ofVec3fToGlmVec33 (ofVec3f vec) {
	return glm::vec3(vec.x, vec.y, vec.z);
}

//hack to make ofVec3f work with std::set
bool operator <(ofVec3f const& lhs, ofVec3f const& rhs) {
	if (lhs.x != rhs.x) {
		return lhs.x < rhs.x;
	} else if (lhs.y != rhs.y) {
		return lhs.y < rhs.y;
	} else if (lhs.z != rhs.z) {
		return lhs.z < rhs.z;
	} else {
		return false;
	}
}

//TODO: put these in some shared utilities file

bool rayIntersectsTriangle(const ofVec3f &p, const ofVec3f &d, const ofVec3f &v0, const ofVec3f &v1, const ofVec3f &v2, ofVec3f &intersection, float &distance) {
	ofVec3f e1 = v1 - v0;
	ofVec3f e2 = v2 - v0;

	ofVec3f h = d.getCrossed(e2);
	float a = e1.dot(h);

	if (a > -0.00001 && a < 0.00001) return false;
	float f = 1 / a;
	ofVec3f s = p - v0;

	float u = f * s.dot(h);
	if (u < 0.0 || u > 1.0)
		return false;

	ofVec3f q = s.getCrossed(e1);
	float v = f * d.dot(q);
	if (v < 0.0 || u + v > 1.0) return false;

	float t = f * e2.dot(q);
	if (t > 0.00001) {
		intersection = p + t * d;
		distance = t;
		return true;
	} else {
		return false;
	}
}

ofVec3f randomDirectionInSphere () {
	float randomPhi = ofRandom(0.0, 2.0 * PI),
		randomZ = ofRandom(-1.0, 1.0);
	return ofVec3f(
		sqrt(1.0 - randomZ * randomZ) * cos(randomPhi),
		sqrt(1.0 - randomZ * randomZ) * sin(randomPhi),
		randomZ
	);
}

ofQuaternion randomQuaternion () {
	//uniform random axis and uniform random angle
	ofVec3f axis(randomDirectionInSphere());
	float angle = ofRandom(0.0, 2.0 * PI);
	return ofQuaternion(axis.x * sin(angle / 2.0), axis.y * sin(angle / 2.0), axis.z * sin(angle / 2.0), cos(angle / 2.0));
}

ofVec3f randomDirectionInCone (const ofVec3f &axis, const float angle) {
	float randomPhi = ofRandom(0.0, 2.0 * PI),
		randomZ = ofRandom(cos(angle), 1.0);

	ofVec3f direction(
		sqrt(1.0 - randomZ * randomZ) * cos(randomPhi),
		sqrt(1.0 - randomZ * randomZ) * sin(randomPhi),
		randomZ
	);

	ofVec3f right = axis.getCrossed(ofVec3f(0.0, 0.0, 1.0)).normalize();
	ofVec3f up = right.getCrossed(axis);

	return direction.x * right + direction.y * up + direction.z * axis;
}

ofFloatColor Segment::getColor () const {
	float scale = pow(COLOR_DECAY, -forkLevel);
	return ofFloatColor(
		LIGHTNING_COLOR.r * scale,
		LIGHTNING_COLOR.g * scale,
		LIGHTNING_COLOR.b * scale,
		1.0
	); //HACK: the overloaded * clamps colours...
}

//tiles are made of triangles
Tile::Tile(vector<ofVec3f> vertices_, vector<ofVec2f> textureCoordinates_, ofVec3f normal_, ofVec3f tangent_, ofVec3f bitangent_) {
	vertices = vertices_;
	textureCoordinates = textureCoordinates_;

	ofVec3f total(0.0);
	for (int i = 0; i < vertices.size(); ++i) {
		total += vertices[i];
	}
	centroid = total /= vertices.size();

	energy = 0.0;
	area = (vertices[1] - vertices[0]).getCrossed(vertices[2] - vertices[0]).length() / 2.0;

	offset = INFINITE_BASELINE;

	isConnected = false;

	normal = normal_;
	tangent = tangent_;
	bitangent = bitangent_;
}

bool Tile::intersectsRay (const ofVec3f &origin, const ofVec3f &direction, ofVec3f &intersection, float &distance) const {
	return rayIntersectsTriangle(origin, direction,
			vertices[0] + ofVec3f(0.0, offset, 0.0),
			vertices[1]  + ofVec3f(0.0, offset, 0.0),
			vertices[2]  + ofVec3f(0.0, offset, 0.0),
		intersection, distance);
}

void Tile::addEnergy (const float amount) {
	energy += amount;
}

void Tile::subtractEnergy (const float amount) {
	energy -= amount;
	if (energy < 0.0) {
		energy = 0.0;
	}
}

DEFINE_WORLD(ElectricWorld)

ElectricWorld::ElectricWorld(Engine *engine):World(engine), ambientSound("worlds/electric/sounds/Ambience_Loop.ogg", true, true) {
	//ctor
	voicePromptFileName = "sounds/voice_prompts/voice_prompt_electro.ogg";
}
ElectricWorld::~ElectricWorld() {
	delete gui;
}

ofVec3f aiVector3DToOfVec3f (const aiVector3D& aiVec) {
	return ofVec3f(aiVec.x, aiVec.y, aiVec.z);
}

void ElectricWorld::guiEvent(ofxUIEventArgs &event) {
	if (event.widget->getName() == "RESET SETTINGS") {
		gui->loadSettings("worlds/electric/defaults.xml");
	} else if (event.widget->getName() == "RESET") {
		reset();
	}
}

void ElectricWorld::setupGUI() {
	gui = new ofxUIScrollableCanvas();
	gui->setGlobalSliderHeight(7.0);

	gui->addButton("RESET", false);

	gui->addSlider("MOVEMENT SPEED", 0.0, 50.0, &MOVEMENT_SPEED);

	gui->addSlider("HAND SPARK PROBABILITY", 0.0, 1.0, &HAND_SPARK_PROBABILITY);

	gui->addSlider("HAND MAX ENERGY", 0.0, 20.0, &HAND_MAX_ENERGY);
	gui->addSlider("HAND MIN DISCHARGE ENERGY", 0.0, 20.0, &HAND_MIN_DISCHARGE_ENERGY);

	gui->addSlider("GESTURE MIN SPEED", 0.0, 20.0, &MIN_GESTURE_SPEED);
	gui->addIntSlider("GESTURE WINDOW", 0.0, 100, &GESTURE_WINDOW);

	gui->addRangeSlider("DISCHARGE LIGHTNING DISPLACEMENT RANGE", 0.0, 1.0, &DISCHARGE_LIGHTNING_MIN_DISPLACEMENT, &DISCHARGE_LIGHTNING_MAX_DISPLACEMENT);
	gui->addRangeSlider("SPARK LIGHTNING DISPLACEMENT RANGE", 0.0, 1.0, &SPARK_LIGHTNING_MIN_DISPLACEMENT, &SPARK_LIGHTNING_MAX_DISPLACEMENT);

	gui->addSlider("LIGHTNING DISCHARGE RATE", 0.0, 100.0, &LIGHTNING_DISCHARGE_RATE);

	gui->addIntSlider("LIGHTNING LEVELS", 0, 10, &LIGHTNING_LEVELS);

	gui->addSlider("LIGHTNING_FORK_PROBABILITY", 0.0, 1.0, &FORK_PROBABILITY);
	gui->addSlider("LIGHTNING MAX FORK ANGLE", 0.0, PI * 2.0, &MAX_FORK_ANGLE);
	gui->addRangeSlider("LIGHTNING FORK LENGTH RANGE", 0.0, 1.0, &MIN_FORK_LENGTH, &MAX_FORK_LENGTH);

	gui->addSlider("LIGHTNING RED", 0.0, 1000.0, &LIGHTNING_COLOR.r);
	gui->addSlider("LIGHTNING GREEN", 0.0, 1000.0, &LIGHTNING_COLOR.g);
	gui->addSlider("LIGHTNING BLUE", 0.0, 1000.0, &LIGHTNING_COLOR.b);

	gui->addSlider("LIGHTNING LIGHT INTENSITY", 0.0, 100.0, &LIGHTNING_LIGHT_INTENSITY);
	gui->addSlider("LIGHTNING LIGHT ATTENUATION", 0.0, 0.1, &LIGHTNING_LIGHT_ATTENUATION);

	gui->addSlider("TILE ENERGY FLOW RATE", 0.0, 2000.0, &TILE_ENERGY_FLOW_RATE);
	gui->addSlider("TILE DISCHARGE RATE", 0.0, 5.0, &TILE_DISCHARGE_RATE);

	gui->addSlider("TILE ROUGHNESS", 0.0, 1.0, &ROUGHNESS);
	gui->addSlider("TILE IOR", 0.0, 5.0, &IOR);

	gui->addSlider("TILE COLOR RED", 0.0, 50.0, &TILE_ON_COLOR.r);
	gui->addSlider("TILE COLOR GREEN", 0.0, 50.0, &TILE_ON_COLOR.g);
	gui->addSlider("TILE COLOR BLUE", 0.0, 50.0, &TILE_ON_COLOR.b);

	gui->addSlider("TILE LIGHT INTENSITY", 0.0, 100.0, &TILE_LIGHT_INTENSITY);
	gui->addSlider("TILE LIGHT ATTENUATION", 0.0, 0.1, &TILE_LIGHT_ATTENUATION);

	gui->addSlider("GAP COLOR RED", 0.0, 5.0, &GAP_COLOR.x);
	gui->addSlider("GAP COLOR GREEN", 0.0, 5.0, &GAP_COLOR.y);
	gui->addSlider("GAP COLOR BLUE", 0.0, 5.0, &GAP_COLOR.z);
	gui->addSlider("GAP WIDTH", 0.0, 1.0, &GAP_WIDTH);

	gui->addSlider("AMBIENT LIGHT INTENSITY", 0.0, 10.0, &AMBIENT_LIGHT_INTENSITY);

	gui->addButton("RESET SETTINGS", false);

	gui->loadSettings("worlds/electric/settings.xml");

	gui->autoSizeToFitWidgets();
	ofAddListener(gui->newGUIEvent, this, &ElectricWorld::guiEvent);

	gui->setVisible(false);
}

void ElectricWorld::loadTextures () {
	ofDisableArbTex();
	slateImage.loadImage("worlds/electric/slate.jpg");
	slateImage.getTextureReference().setTextureWrap(GL_REPEAT, GL_REPEAT);
	ofEnableArbTex();

	GLfloat maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

	glBindTexture(GL_TEXTURE_2D, slateImage.getTextureReference().getTextureData().textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void ElectricWorld::setupCylinderAndNodes () {
	vector<ofVec3f> cylinderVertices;
	vector<ofVec3f> cylinderNormals;
	vector<ofVec3f> cylinderTangents;
	vector<ofVec3f> cylinderBitangents;
	vector<ofVec3f> cylinderAbcs;
	vector<ofVec2f> cylinderTextureCoordinates;;

	vector<ofVec3f> uniqueVertices;
	vector<ofVec3f> uniqueTextureCoordinates;

	static const float HEIGHT = CYLINDER_HEIGHT;

	static const int HEIGHT_SEGMENTS = 100; //must be even for bottom and top to match up
	static const int RADIAL_SEGMENTS = 100;
	static const int RADIUS = 25.0;

	float deltaHeight = HEIGHT / ((float)HEIGHT_SEGMENTS);
	float deltaAngle = (2.0 * PI) / ((float)RADIAL_SEGMENTS);

	for (int heightIndex = 0; heightIndex <= HEIGHT_SEGMENTS; ++heightIndex) {
		for (int radialIndex = 0; radialIndex < RADIAL_SEGMENTS; ++radialIndex) {
			float angle = -(float)radialIndex * deltaAngle;

			if (heightIndex % 2 == 1) { //odd row
				angle -= deltaAngle * 0.5;
			}

			if (heightIndex < HEIGHT_SEGMENTS) {
				uniqueVertices.push_back(ofVec3f(
					RADIUS * sin(angle),
					(float)heightIndex * deltaHeight,
					RADIUS * cos(angle)
				) + randomDirectionInSphere() * ofRandom(0.0, 0.75));
			} else {
				uniqueVertices.push_back(uniqueVertices[radialIndex] + ofVec3f(0.0, CYLINDER_HEIGHT, 0.0));
			}

			uniqueTextureCoordinates.push_back(ofVec2f(
				(float)radialIndex / (float)RADIAL_SEGMENTS,
				(float)heightIndex / (float)HEIGHT_SEGMENTS
			) * 20);
		}
	}

	for (int heightIndex = 0; heightIndex < HEIGHT_SEGMENTS; ++heightIndex) {
		for (int radialIndex = 0; radialIndex < RADIAL_SEGMENTS; ++radialIndex) {
			int bottomLeftIndex = heightIndex * RADIAL_SEGMENTS + radialIndex;
			int bottomRightIndex = heightIndex * RADIAL_SEGMENTS + ((radialIndex + 1) % RADIAL_SEGMENTS);
			int topLeftIndex = (heightIndex + 1) * RADIAL_SEGMENTS + radialIndex;
			int topRightIndex = (heightIndex + 1) * RADIAL_SEGMENTS + ((radialIndex + 1) % RADIAL_SEGMENTS);

			ofVec3f bottomLeft = uniqueVertices[bottomLeftIndex];
			ofVec3f bottomRight = uniqueVertices[bottomRightIndex];
			ofVec3f topLeft = uniqueVertices[topLeftIndex];
			ofVec3f topRight = uniqueVertices[topRightIndex];

			ofVec3f bottomLeftCoord = uniqueTextureCoordinates[bottomLeftIndex];
			ofVec3f bottomRightCoord = uniqueTextureCoordinates[bottomRightIndex];
			ofVec3f topLeftCoord = uniqueTextureCoordinates[topLeftIndex];
			ofVec3f topRightCoord = uniqueTextureCoordinates[topRightIndex];

			if (heightIndex % 2 == 0) { //even row
				ofVec3f leftNormal = (bottomRight - bottomLeft).cross(topLeft - bottomLeft).normalize();
				ofVec3f rightNormal = (topLeft - topRight).cross(bottomRight - topRight).normalize();
				ofVec3f leftTangent = (bottomRight - bottomLeft).normalize();
				ofVec3f leftBitangent = (topLeft - bottomLeft).normalize();
				ofVec3f rightTangent = (topRight - topLeft).normalize();
				ofVec3f rightBitangent = (topRight - bottomRight).normalize();

				cylinderVertices.push_back(bottomLeft);
				cylinderVertices.push_back(bottomRight);
				cylinderVertices.push_back(topLeft);
				cylinderTextureCoordinates.push_back(bottomLeftCoord);
				cylinderTextureCoordinates.push_back(bottomRightCoord);
				cylinderTextureCoordinates.push_back(topLeftCoord);
				for (int i = 0; i < 3; ++i) {
					cylinderNormals.push_back(leftNormal);
					cylinderTangents.push_back(leftTangent);
					cylinderBitangents.push_back(leftBitangent);
				}
				cylinderAbcs.push_back(ofVec3f(1, 0, 0));
				cylinderAbcs.push_back(ofVec3f(0, 1, 0));
				cylinderAbcs.push_back(ofVec3f(0, 0, 1));


				cylinderVertices.push_back(bottomRight);
				cylinderVertices.push_back(topRight);
				cylinderVertices.push_back(topLeft);
				cylinderTextureCoordinates.push_back(bottomRightCoord);
				cylinderTextureCoordinates.push_back(topRightCoord);
				cylinderTextureCoordinates.push_back(topLeftCoord);
				for (int i = 0; i < 3; ++i) {
					cylinderNormals.push_back(rightNormal);
					cylinderTangents.push_back(rightTangent);
					cylinderBitangents.push_back(rightBitangent);
				}
				cylinderAbcs.push_back(ofVec3f(1, 0, 0));
				cylinderAbcs.push_back(ofVec3f(0, 1, 0));
				cylinderAbcs.push_back(ofVec3f(0, 0, 1));


				vector<ofVec3f> leftTileVertices;
				leftTileVertices.push_back(bottomLeft);
				leftTileVertices.push_back(bottomRight);
				leftTileVertices.push_back(topLeft);

				vector<ofVec2f> leftTileTextureCoordinates;
				leftTileTextureCoordinates.push_back(bottomLeftCoord);
				leftTileTextureCoordinates.push_back(bottomRightCoord);
				leftTileTextureCoordinates.push_back(topLeftCoord);

				tiles.push_back(Tile(leftTileVertices, leftTileTextureCoordinates, leftNormal, leftTangent, leftBitangent));

				vector<ofVec3f> rightTileVertices;
				rightTileVertices.push_back(bottomRight);
				rightTileVertices.push_back(topRight);
				rightTileVertices.push_back(topLeft);

				vector<ofVec2f> rightTileTextureCoordinates;
				rightTileTextureCoordinates.push_back(bottomRightCoord);
				rightTileTextureCoordinates.push_back(topRightCoord);
				rightTileTextureCoordinates.push_back(topLeftCoord);

				tiles.push_back(Tile(rightTileVertices, rightTileTextureCoordinates, rightNormal, rightTangent, rightBitangent));

			} else { //odd row
				ofVec3f leftNormal = (bottomLeft - topLeft).cross(topRight - topLeft).normalize();
				ofVec3f rightNormal = (topRight - bottomRight).cross(bottomLeft - bottomRight).normalize();
				ofVec3f leftTangent = (topRight - topLeft).normalize();
				ofVec3f leftBitangent = (topLeft - bottomLeft).normalize();
				ofVec3f rightTangent = (bottomRight - bottomLeft).normalize();
				ofVec3f rightBitangent = (topRight - bottomRight).normalize();


				cylinderVertices.push_back(bottomLeft);
				cylinderVertices.push_back(topRight);
				cylinderVertices.push_back(topLeft);
				cylinderTextureCoordinates.push_back(bottomLeftCoord);
				cylinderTextureCoordinates.push_back(topRightCoord);
				cylinderTextureCoordinates.push_back(topLeftCoord);
				for (int i = 0; i < 3; ++i) {
					cylinderNormals.push_back(leftNormal);
					cylinderTangents.push_back(leftTangent);
					cylinderBitangents.push_back(leftBitangent);
				}
				cylinderAbcs.push_back(ofVec3f(1, 0, 0));
				cylinderAbcs.push_back(ofVec3f(0, 1, 0));
				cylinderAbcs.push_back(ofVec3f(0, 0, 1));


				cylinderVertices.push_back(bottomRight);
				cylinderVertices.push_back(topRight);
				cylinderVertices.push_back(bottomLeft);
				cylinderTextureCoordinates.push_back(bottomRightCoord);
				cylinderTextureCoordinates.push_back(topRightCoord);
				cylinderTextureCoordinates.push_back(bottomLeftCoord);
				for (int i = 0; i < 3; ++i) {
					cylinderNormals.push_back(rightNormal);
					cylinderTangents.push_back(rightTangent);
					cylinderBitangents.push_back(rightBitangent);
				}
				cylinderAbcs.push_back(ofVec3f(1, 0, 0));
				cylinderAbcs.push_back(ofVec3f(0, 1, 0));
				cylinderAbcs.push_back(ofVec3f(0, 0, 1));


				vector<ofVec3f> leftTileVertices;
				leftTileVertices.push_back(bottomLeft);
				leftTileVertices.push_back(topRight);
				leftTileVertices.push_back(topLeft);

				vector<ofVec2f> leftTileTextureCoordinates;
				leftTileTextureCoordinates.push_back(bottomLeftCoord);
				leftTileTextureCoordinates.push_back(topRightCoord);
				leftTileTextureCoordinates.push_back(topLeftCoord);

				tiles.push_back(Tile(leftTileVertices, leftTileTextureCoordinates, leftNormal, leftTangent, leftBitangent));

				vector<ofVec3f> rightTileVertices;
				rightTileVertices.push_back(bottomRight);
				rightTileVertices.push_back(topRight);
				rightTileVertices.push_back(bottomLeft);

				vector<ofVec2f> rightTileTextureCoordinates;
				rightTileTextureCoordinates.push_back(bottomRightCoord);
				rightTileTextureCoordinates.push_back(topRightCoord);
				rightTileTextureCoordinates.push_back(bottomLeftCoord);

				tiles.push_back(Tile(rightTileVertices, rightTileTextureCoordinates, rightNormal, rightTangent, rightBitangent));
			}
		}
	}

	for (Tile& tile : tiles) {
		tile.startOffset = ofVec3f(tile.centroid.x, 0.0, tile.centroid.z).getNormalized() * 200.0;
		tile.startOffset.y = -tile.centroid.y + ofRandom(-25.0, 25.0);
		tile.rotationAxis = randomDirectionInSphere();
		tile.rotationAngle = ofRandom(0.0, 1080.0);
		tile.assemblyStartTime = (1.0 - (tile.centroid.y / CYLINDER_HEIGHT)) * 10.0 + ofRandom(0.0, 0.2) + ASSEMBLY_START_TIME;
		tile.assemblyEndTime = tile.assemblyStartTime + ofRandom(2.0, 3.0);
	}

	cylinderVbo.setVertexData(&cylinderVertices[0], cylinderVertices.size(), GL_STATIC_DRAW);
	cylinderVbo.setNormalData(&cylinderNormals[0], cylinderNormals.size(), GL_STATIC_DRAW);
	cylinderVbo.setTexCoordData(&cylinderTextureCoordinates[0], cylinderTextureCoordinates.size(), GL_STATIC_DRAW);
	cylinderVbo.setAttributeData(tileShader().getAttributeLocation("a_abc"), &cylinderAbcs[0].x, 3, cylinderAbcs.size(), GL_STATIC_DRAW, sizeof(ofVec3f));

	//construct half-edge data structure
	std::map<std::pair<ofVec3f, ofVec3f>, HalfEdge*> edgeMap; //use vertex positions directly instead of indices beacuse vertices from assimp are duplicated

	for (int i = 0; i < tiles.size(); ++i) {
		Tile& tile = tiles[i];

		//first create the half-edges
		for (int j = 0; j < 3; ++j) {
			tile.halfEdges.push_back(HalfEdge());
		}

		for (int j = 0; j < 3; ++j) {
			HalfEdge& halfEdge = tile.halfEdges[j];

			halfEdge.opposite = nullptr; //the opposite half-edge

			halfEdge.tile = &tile; //the tile on the half-edge's left

			halfEdge.startVertex = tile.vertices[j];
			halfEdge.endVertex = tile.vertices[(j + 1) % 3];
			halfEdge.length = tile.halfEdges[j].startVertex.distance(halfEdge.endVertex);
			halfEdge.direction = (tile.vertices[(j + 1) % 3] - tile.vertices[j]).normalize();

			halfEdge.next = &tile.halfEdges[(j + 1) % 3]; //the next half-edge in the triangle going counter-clockwise

			edgeMap[std::pair<ofVec3f, ofVec3f>(tiles[i].vertices[j], tiles[i].vertices[(j + 1) % 3])] = &halfEdge;

			edgeMap[std::pair<ofVec3f, ofVec3f>(tiles[i].vertices[j] + ofVec3f(0.0, CYLINDER_HEIGHT, 0.0), tiles[i].vertices[(j + 1) % 3] + ofVec3f(0.0, CYLINDER_HEIGHT, 0.0))] = &halfEdge; //connect top and bottom edges
			edgeMap[std::pair<ofVec3f, ofVec3f>(tiles[i].vertices[j] + ofVec3f(0.0, -CYLINDER_HEIGHT, 0.0), tiles[i].vertices[(j + 1) % 3] + ofVec3f(0.0, -CYLINDER_HEIGHT, 0.0))] = &halfEdge; //connect top and bottom edges

		}
	}

	//find opposite half-edges
	for (int i = 0; i < tiles.size(); ++i) {
		for (int j = 0; j < 3; ++j) {
			if (edgeMap.find(std::pair<ofVec3f, ofVec3f>(tiles[i].vertices[(j + 1) % 3], tiles[i].vertices[j])) != edgeMap.end()) {
				tiles[i].halfEdges[j].opposite = edgeMap[std::pair<ofVec3f, ofVec3f>(tiles[i].vertices[(j + 1) % 3], tiles[i].vertices[j])];
			}
		}
	}
}

void ElectricWorld::setupDissolvedCylinder () {
	vector<ofVec3f> cylinderVertices;
	vector<ofVec3f> cylinderNormals;
	vector<ofVec3f> cylinderTangents;
	vector<ofVec3f> cylinderBitangents;
	vector<ofVec2f> cylinderTextureCoordinates;
	vector<ofVec3f> cylinderAbcs;

	for (Tile& tile : tiles) {
		if (ofRandom(0.0, 1.0) > (tile.centroid.y / DISSOLVED_CYLINDER_HEIGHT)) {
			for (int i = 0; i < 3; ++i) {
				cylinderVertices.push_back(tile.vertices[i]);
				cylinderNormals.push_back(tile.normal);
				cylinderTangents.push_back(tile.tangent);
				cylinderBitangents.push_back(tile.bitangent);

				cylinderTextureCoordinates.push_back(tile.textureCoordinates[i]);
			}
			cylinderAbcs.push_back(ofVec3f(1.0, 0.0, 0.0));
			cylinderAbcs.push_back(ofVec3f(0.0, 1.0, 0.0));
			cylinderAbcs.push_back(ofVec3f(0.0, 0.0, 1.0));
		}
	}

	dissolvedCylinderVbo.setVertexData(&cylinderVertices[0], cylinderVertices.size(), GL_STATIC_DRAW);
	dissolvedCylinderVbo.setNormalData(&cylinderNormals[0], cylinderNormals.size(), GL_STATIC_DRAW);
	dissolvedCylinderVbo.setTexCoordData(&cylinderTextureCoordinates[0], cylinderTextureCoordinates.size(), GL_STATIC_DRAW);
	dissolvedCylinderVbo.setAttributeData(tileShader().getAttributeLocation("a_abc"), &cylinderAbcs[0].x, 3, cylinderAbcs.size(), GL_STATIC_DRAW, sizeof(ofVec3f));

}

void ElectricWorld::loadSounds () {
	for (int i = 0; i < 2; ++i) {
		crackleSounds[i].al_buffer = GetALBuffer("worlds/electric/sounds/Crackle_Loop-1.ogg");
		chargeSounds[i].al_buffer = GetALBuffer("worlds/electric/sounds/Charge_Loop.ogg");

		crackleSounds[i].sound_flags = sound_flag_repeat;
		crackleSounds[i].pos = glm::vec3(0.0);
		crackleSounds[i].sound_gain = hands[i].energy * CRACKLE_GAIN;

		chargeSounds[i].sound_flags = sound_flag_repeat;
		chargeSounds[i].pos = glm::vec3(0.0);
		chargeSounds[i].sound_gain = hands[i].energy * CHARGE_GAIN;
	}

	//cache sounds to avoid stutter
	GetALBuffer("worlds/electric/sounds/Electro_Transition.ogg");
	GetALBuffer("worlds/electric/sounds/Ambience_Loop.ogg");
	GetALBuffer(voicePromptFileName);

	for (int i = 1; i < 6; ++i) {
		GetALBuffer("worlds/electric/sounds/Impact_Flash" + std::to_string(i) + ".ogg");
		GetALBuffer("worlds/electric/sounds/Impact_New" + std::to_string(i) + ".ogg");
		GetALBuffer("worlds/electric/sounds/Glow" + std::to_string(i) + "-1.ogg");
	}
}

extern float LANDSCAPE_SCALE;
ofVec3f aiVector3DToOfVec3fa(aiVector3D& v);

void ElectricWorld::setup() {
	cout << "ElectricWorld::setup start" << endl;

	duration = WORLD_ELECTRIC::TOTAL_DURATION;

	World::setup();

	setupGUI();
	loadTextures();
	setupCylinderAndNodes();
	setupDissolvedCylinder();

	loadSounds();

	backgroundSphere.set(250, 32);

	environment=new Texture();
	environment->LoadEXR(ofToDataPath("worlds/ultramarine/environment_map.exr"));

	vector<ofVec3f> skyPlaneVertices;
	vector<ofVec2f> skyPlaneTextureCoordinates;

	skyPlaneVertices.push_back(ofVec3f(0.0, 0.0, 0.0));
	skyPlaneVertices.push_back(ofVec3f(SKY_PLANE_WIDTH, 0.0, 0.0));
	skyPlaneVertices.push_back(ofVec3f(0.0, 0.0, SKY_PLANE_HEIGHT));
	skyPlaneVertices.push_back(ofVec3f(SKY_PLANE_WIDTH, 0.0, SKY_PLANE_HEIGHT));
	skyPlaneVertices.push_back(ofVec3f(0.0, 0.0, SKY_PLANE_HEIGHT));
	skyPlaneVertices.push_back(ofVec3f(SKY_PLANE_WIDTH, 0.0, 0.0));

	skyPlaneTextureCoordinates.push_back(ofVec2f(0.0, 0.0));
	skyPlaneTextureCoordinates.push_back(ofVec2f(1.0, 0.0));
	skyPlaneTextureCoordinates.push_back(ofVec2f(0.0, 1.0));
	skyPlaneTextureCoordinates.push_back(ofVec2f(1.0, 1.0));
	skyPlaneTextureCoordinates.push_back(ofVec2f(0.0, 1.0));
	skyPlaneTextureCoordinates.push_back(ofVec2f(1.0, 0.0));

	skyVbo.setVertexData(&skyPlaneVertices[0], skyPlaneVertices.size(), GL_STATIC_DRAW);
	skyVbo.setTexCoordData(&skyPlaneTextureCoordinates[0], skyPlaneTextureCoordinates.size(), GL_STATIC_DRAW);

	ofDisableArbTex();
	skyImage.loadImage("worlds/shared/starrysky.png");
	ofEnableArbTex();

	for (int i = 0; i < 100; ++i) {
		lastGestureVelocities[0][i] = ofVec3f(0.0);
		lastGestureVelocities[1][i] = ofVec3f(0.0);
	}

	camera.setupPerspective(false, 90, 0.01, 2000); //TODO: where do FOV, near and far values come from?
    camera.setPosition(0, 1.8, 0);

	userHeight = 0.0;
	userSpeed = 0.0;

	isNextWorldBody = false;

	glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);

	hands[0].energy = 0;
	hands[1].energy = 0;

	playedTransitionSound = false;
	playedAmbientSound = false;

	hasTriggered = false;
	triggerStartTime = 0.0;
	cout << "ElectricWorld::setup end" << endl;
}

void ElectricWorld::reset() {
	camera.setPosition(0, 1.8, 0);
	userHeight = 0.0;

	hands[0].energy = 0;
	hands[1].energy = 0;

	lightnings.clear();

	userSpeed = 0.0;

	for (int i = 0; i < 100; ++i) {
		lastGestureVelocities[0][i] = ofVec3f(0.0);
		lastGestureVelocities[1][i] = ofVec3f(0.0);
	}

	for (Tile& tile : tiles) {
		tile.energy = 0.0;
		tile.offset = INFINITE_BASELINE;
	}

	playedTransitionSound = false;
	playedAmbientSound = false;
	isNextWorldBody = false;

	duration = WORLD_ELECTRIC::TOTAL_DURATION;
}

void ElectricWorld::beginExperience() {
	OSCManager::getInstance().startWorld(COMMON::WORLD_TYPE::ELECTRIC);
	gui->setVisible(false);
	playedVoicePrompt = false;

	for (int i = 0; i < 2; ++i) {
		GlobAudioManager().AddSoundObject(&crackleSounds[i]);
		GlobAudioManager().AddSoundObject(&chargeSounds[i]);
	}

	GlobAudioManager().AddSoundObject(&ambientSound);
}

void ElectricWorld::endExperience() {
	OSCManager::getInstance().endWorld(COMMON::WORLD_TYPE::ELECTRIC);
	gui->setVisible(false);
	for (int i = 0; i < 2; ++i) {
		GlobAudioManager().RemoveSoundObject(&crackleSounds[i]);
		GlobAudioManager().RemoveSoundObject(&chargeSounds[i]);
	}
	playedVoicePrompt = false;

	GlobAudioManager().RemoveSoundObject(&ambientSound);
}

float clamp (float x, float low, float high) {
	return min(max(x, low), high);
}

float smoothstep(float edge0, float edge1, float x) {
    // Scale, bias and saturate x to 0..1 range
    x = clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
   return x*x*(3 - 2*x);
}


void ElectricWorld::updateCamera (float dt) {
	if (Timeline::getInstance().getElapsedCurrWorldTime() > MOVEMENT_START_TIME && userHeight < DECELERATION_START_HEIGHT) {
		userSpeed += ACCELERATION * dt;
		if (userSpeed > MOVEMENT_SPEED) userSpeed = MOVEMENT_SPEED;
	}

	if (userHeight > DECELERATION_START_HEIGHT) {
		userSpeed -= DECELERATION * dt;
		if (userHeight > EVOLUTION_BASELINE - 50.0 || userSpeed < 0.0) {
			duration = 0.0;
		}
	}

	userHeight += userSpeed * dt;
	camera.setPosition(ofVec3f(0.0, userHeight + 1.8, 0.0));
	//camera.lookAt(ofVec3f(0.0, userHeight + 1.8, -1.0), ofVec3f(0.0, 1.0, 0.0));
	//camera.setOrientation(ofVec3f(90.0, 0.0, 0.0));
}

void ElectricWorld::updateSounds (float dt) {
	for (int i = 0; i < 2; ++i) {
		crackleSounds[i].pos = ofVec3fToGlmVec33(engine->kinectEngine.getHandsPosition()[i] + ofVec3f(0.0, userHeight, 0.0));
		crackleSounds[i].sound_gain = hands[i].energy * CRACKLE_GAIN;
		chargeSounds[i].pos = ofVec3fToGlmVec33(engine->kinectEngine.getHandsPosition()[i] + ofVec3f(0.0, userHeight, 0.0));
		chargeSounds[i].sound_gain = hands[i].energy * CRACKLE_GAIN;
	}

	if (Timeline::getInstance().getElapsedCurrWorldTime() > 1.0 && !playedTransitionSound) {
		PlayBackgroundSound("worlds/electric/sounds/Electro_Transition.ogg");

		playedTransitionSound = true;
	}

	ambientSound.sound_gain = smoothstep(6.0, 8.0, Timeline::getInstance().getElapsedCurrWorldTime());

	if (userHeight > DECELERATION_START_HEIGHT) {
		ambientSound.sound_gain = smoothstep(DECELERATION_START_HEIGHT + 40.0, DECELERATION_START_HEIGHT, userHeight);
	}
}

Tile* ElectricWorld::getIntersectedTile (ofVec3f origin, ofVec3f direction) {
	Tile* hitTile = nullptr;
	float minDistance = std::numeric_limits<float>::max(); //current distance to closest tile
	for (int j = 0; j < tiles.size(); ++j) {
		Tile &tile = tiles[j];

		ofVec3f intersection;
		float distance;

		if (tile.intersectsRay(origin, direction, intersection, distance)) {
			if (distance < minDistance) { //if this is the closest tile intersected so far
				minDistance = distance;
				hitTile = &tile;
			}
		}
	}

	return hitTile;
}

void ElectricWorld::updateTiles (float dt) {
	//update all tiles for charge and create particles + lights

	chargedTiles.clear();

	for (int i = 0; i < tiles.size(); ++i) {
		Tile& tile = tiles[i];

		tile.subtractEnergy(TILE_DISCHARGE_RATE * dt);

		if (tile.energy == 0.0) {
			tile.isConnected = false;
		}

		//make tiles reach equilibrium
		for (int j = 0; j < 3; ++j) {
			if (tile.halfEdges[j].opposite != nullptr && tile.halfEdges[j].opposite->tile != nullptr) { //if neighbouring tile exists
				Tile* neighbourTile = tile.halfEdges[j].opposite->tile;
				float otherEnergy = neighbourTile->energy;
				if (otherEnergy < tile.energy) {
					float difference = tile.energy - otherEnergy;
					float energyToTransfer = difference * 0.25;
					if (energyToTransfer > difference / 2) { //if tile energies are close enough to be equal this step
						tile.energy -= difference / 2;
						neighbourTile->energy += difference / 2;
					} else {
						tile.energy -= energyToTransfer;
						neighbourTile->energy += energyToTransfer;
					}
				}
			}
		}

		if (tile.energy > 0.0) {
			chargedTiles.push_back(&tile); //for rendering
		}

		if (tile.isConnected) {
			lights.push_back(Light(
				tile.centroid + ofVec3f(0.0, tile.offset, 0.0) + tile.normal * 5.0,
				tile.normal,
				0.0,
				TILE_LIGHT_ATTENUATION,
				TILE_LIGHT_INTENSITY * tile.energy
			));
		}

		//there is only one set of tiles that are always kept in the range [camera height - cylinder height / 2, camera height + cylinder height / 2]
		if (userHeight - (tile.centroid.y + tile.offset) > CYLINDER_HEIGHT / 2 && userHeight < INFINITE_TOPLINE - CYLINDER_HEIGHT / 2) { //if the tile is too far down
			tile.offset += CYLINDER_HEIGHT; //bring it up above
			tile.energy = 0.0;
		}

		if (userHeight - (tile.centroid.y + tile.offset) < -CYLINDER_HEIGHT / 2 && userHeight > (INFINITE_BASELINE + CYLINDER_HEIGHT / 2)) {
			tile.offset -= CYLINDER_HEIGHT;
			tile.energy = 0.0;
		}
	}
}

void ElectricWorld::dischargeHand(int handIndex, ofVec3f direction) {

	ofVec3f position =  engine->kinectEngine.getHandsPosition()[handIndex] + ofVec3f(0.0, userHeight, 0.0);
	Tile* hitTile = getIntersectedTile(position, direction);
	if (hitTile != nullptr) { //make sure we hit a tile
		Play3DSound("worlds/electric/sounds/Impact_Flash" + std::to_string(int(ofRandom(1, 6))) + ".ogg", false, false, ofVec3fToGlmVec33(engine->kinectEngine.getHandsPosition()[handIndex] + ofVec3f(0.0, userHeight, 0.0)), glm::vec3(0), 2E10, DISCHARGE_GAIN, 1.0);
		Play3DSound("worlds/electric/sounds/Impact_New" + std::to_string(int(ofRandom(1, 6))) + ".ogg", false, false, ofVec3fToGlmVec33(hitTile->centroid + ofVec3f(0.0, hitTile->offset, 0.0)), glm::vec3(0), 2E10, IMPACT_GAIN, 1.0);
		Play3DSound("worlds/electric/sounds/Glow" + std::to_string(int(ofRandom(1, 6))) + "-1.ogg", false, false, ofVec3fToGlmVec33(hitTile->centroid + ofVec3f(0.0, hitTile->offset, 0.0)), glm::vec3(0), 2E10, GLOW_GAIN, 1.0);
		Lightning newLightning(position, hitTile->centroid + ofVec3f(0.0, hitTile->offset, 0.0), hands[handIndex].energy, hitTile, 0.2, 0.4);
		newLightning.type = 0; //discharge
		newLightning.hand = handIndex;
		newLightning.minDisplacement = DISCHARGE_LIGHTNING_MIN_DISPLACEMENT;
		newLightning.maxDisplacement = DISCHARGE_LIGHTNING_MAX_DISPLACEMENT;
		lightnings.push_back(newLightning);
		hands[handIndex].energy = 0.0;
	}
}

void ElectricWorld::autoTriggerDischarge()
{
	if(triggerStartTime >= AUTO_TRIGGER_TIME)
	{
		hasTriggered = true;
		dischargeHand(0, engine->kinectEngine.getHandsDirection()[0]);
		dischargeHand(1, engine->kinectEngine.getHandsDirection()[1]);
	}
}

void ElectricWorld::updateLightning (float dt) {
	if (userHeight < INFINITE_BASELINE) return;

	// attenuate near the top
	if (userHeight > INFINITE_TOPLINE) {
		hands[0].energy *= 0.97;
		hands[1].energy *= 0.97;
		return;
	}

	//create lightning discharges and sparks
	for (int handIndex = 0; handIndex < 2; ++handIndex) {
		hands[handIndex].energy += HAND_CHARGE_RATE * dt;

		if (hands[handIndex].energy > HAND_MAX_ENERGY) {
			hands[handIndex].energy = HAND_MAX_ENERGY;
		}

		//check for discharge
		if (engine->kinectEngine.isKinectDetected()) {

			ofVec3f handPosition = engine->kinectEngine.getHandsPosition()[handIndex];
			currentHandPositions[handIndex] = handPosition;
			ofVec3f velocity = (handPosition - lastHandPositions[handIndex]) / dt;
			float speed = velocity.length();


			//there are 100 previous gesture velocities stored
			for (int j = 0; j < 100 - 1; ++j) {
				lastGestureVelocities[handIndex][j] = lastGestureVelocities[handIndex][j + 1];
			}
			lastGestureVelocities[handIndex][99] = velocity;

			ofVec3f averageVelocity(0.0, 0.0, 0.0);

			for (int j = 99 - GESTURE_WINDOW; j < 100; ++j) {
				averageVelocity += lastGestureVelocities[handIndex][j] / GESTURE_WINDOW;
			}

			if (averageVelocity.length() > MIN_GESTURE_SPEED && averageVelocity.getNormalized().dot(engine->kinectEngine.getHandsDirection()[handIndex].getNormalized()) > MIN_GESTURE_DOT && hands[handIndex].energy > HAND_MIN_DISCHARGE_ENERGY) {
				dischargeHand(handIndex, engine->kinectEngine.getHandsDirection()[handIndex]);

				if(!hasTriggered)
					hasTriggered = true;
			}

			lastHandPositions[handIndex] = currentHandPositions[handIndex];
		}

		//generate sparks
		float probability = pow(hands[handIndex].energy * HAND_SPARK_PROBABILITY, 6.0);
		for (int j = 0; j < probability; ++j) {
			if (ofRandom(0.0, 1.0) < hands[handIndex].energy * HAND_SPARK_PROBABILITY) {
				vector<int>* armAndHandVertexIndices;
				if (handIndex == 0) {
					armAndHandVertexIndices = &engine->kinectEngine.getSkinnedBody()->leftArmAndHandVertexIndices;
				} else {
					armAndHandVertexIndices = &engine->kinectEngine.getSkinnedBody()->rightArmAndHandVertexIndices;
				}

				int endVertexIndex;

				int startVertexIndex = (*armAndHandVertexIndices)[ofRandom(0, (*armAndHandVertexIndices).size() - 1)];
				ofVec3f startPoint =  engine->kinectEngine.getSkinnedBody()->getVertexPosition(startVertexIndex) + ofVec3f(0.0, userHeight, 0.0);

				int tries = 0;

				//try points until we find one close enough
				ofVec3f endPoint;
				do {
					endVertexIndex = (*armAndHandVertexIndices)[ofRandom(0, (*armAndHandVertexIndices).size() - 1)];
					endPoint =  engine->kinectEngine.getSkinnedBody()->getVertexPosition(endVertexIndex) + ofVec3f(0.0, userHeight, 0.0);
				} while (startPoint.distance(endPoint) > 0.1 && ++tries < 10);

				if (tries < 9) {
					Lightning newLightning(startPoint, endPoint, hands[handIndex].energy * 0.001, nullptr, 1.0, 1.0);
					newLightning.type = 1;
					newLightning.hand = 0;
					newLightning.startVertexIndex = startVertexIndex;
					newLightning.endVertexIndex = endVertexIndex;
					newLightning.minDisplacement = SPARK_LIGHTNING_MIN_DISPLACEMENT;
					newLightning.maxDisplacement = SPARK_LIGHTNING_MAX_DISPLACEMENT;
					lightnings.push_back(newLightning);
				}
			}
		}

	}

	//update lightning geometry and lights
	//the lightning generator takes a line between two endpoints and modifies them (displace/fork) recursively in a fractal-like way (L-system) to create a lightningy effect
	for (auto it = lightnings.begin(); it != lightnings.end();) {
		Lightning& lightning = *it;
		lightning.energy -= LIGHTNING_DISCHARGE_RATE * dt;

		if (lightning.energy <= 0.0 && lightning.totalEnergy > 0.0) { //if we're out of energy and we still have energy to discharge
			lightning.energy = ofRandom(lightning.initialEnergy * lightning.minEnergyFraction, lightning.initialEnergy * lightning.maxEnergyFraction);
			lightning.totalEnergy -= lightning.energy;

			if (lightning.type == 0 && lightning.tile != nullptr) {
				lightning.tile->addEnergy(lightning.energy * 5.0);
				lightning.tile->isConnected = true;
			}

			//generate new geometry
			lightning.vertexPositions.clear();
			lightning.vertexColors.clear();

			vector<Segment> pingSegments;
			vector<Segment> pongSegments;

			//ping pong vectors for iterative evolution

			float lightningLength = 1.0;
			pingSegments.push_back(Segment(ofVec3f(0.0, 0.0, 0.0), ofVec3f(0.0, 0.0, -1.0), 0));

			for (int level = 0; level <= LIGHTNING_LEVELS; ++level) {
				vector<Segment> &fromSegments = level % 2 == 0 ? pingSegments : pongSegments;
				vector<Segment> &toSegments = level % 2 == 0 ? pongSegments : pingSegments;

				for (vector<Segment>::iterator it = fromSegments.begin(); it != fromSegments.end(); it++) {
					Segment &segment = *it;

					if (level == LIGHTNING_LEVELS) {
						lightning.vertexPositions.push_back(segment.start);
						lightning.vertexPositions.push_back(segment.end);

						lightning.vertexColors.push_back(segment.getColor());
						lightning.vertexColors.push_back(segment.getColor());
					} else {
						ofVec3f split = segment.start.getInterpolated(segment.end, ofRandom(MIN_FORK_FRACTION, MAX_FORK_FRACTION));

						ofVec3f splitDirection = randomDirectionInSphere();
						split += ofRandom(lightning.minDisplacement * lightningLength, lightning.maxDisplacement * lightningLength) * pow(DISPLACEMENT_DECAY, -level) * splitDirection;

						toSegments.push_back(Segment(segment.start, split, segment.forkLevel));
						toSegments.push_back(Segment(split, segment.end, segment.forkLevel));

						if (ofRandom(0.0, 1.0) < FORK_PROBABILITY) {
							ofVec3f forkDirection = randomDirectionInCone((segment.end - segment.start).normalize(), MAX_FORK_ANGLE);
							ofVec3f end = split + forkDirection * pow(FORK_DECAY, -level) * ofRandom(MIN_FORK_LENGTH * lightningLength, MAX_FORK_LENGTH * lightningLength);

							toSegments.push_back(Segment(split, end, segment.forkLevel + 1));
						}

					}
				}

				fromSegments.clear();
			}
		}

		if (lightning.type == 0) {
			lightning.startPoint = engine->kinectEngine.getHandsPosition()[lightning.hand] + ofVec3f(0.0, userHeight, 0.0);
		} else if (lightning.type == 1) {
			lightning.startPoint = engine->kinectEngine.getSkinnedBody()->getVertexPosition(lightning.startVertexIndex) + ofVec3f(0.0, userHeight, 0.0);
			lightning.endPoint = engine->kinectEngine.getSkinnedBody()->getVertexPosition(lightning.endVertexIndex) + ofVec3f(0.0, userHeight, 0.0);
		}

		//transform generated geometry to go between actual world start and end points
		vector<ofVec3f> transformedVertexPositions;
		for (int i = 0; i < lightning.vertexPositions.size(); ++i) {
			ofVec3f basePosition = lightning.vertexPositions[i];
			float scale = lightning.startPoint.distance(lightning.endPoint);
			//I just don't trust any of OF's math functions...
			ofVec3f forward = (lightning.endPoint - lightning.startPoint).normalize();
			ofVec3f right = forward.getCrossed(ofVec3f(0.0, 1.0, 0.0)).normalize();
			ofVec3f up = -forward.getCrossed(right).normalize();
			ofVec3f rotatedPosition = -forward * basePosition.z + right * basePosition.x + up * basePosition.y;
			transformedVertexPositions.push_back(rotatedPosition * scale + lightning.startPoint);
		}

		lightning.vbo.setVertexData(&transformedVertexPositions[0], transformedVertexPositions.size(), GL_STREAM_DRAW);
		lightning.vbo.setColorData(&lightning.vertexColors[0], lightning.vertexColors.size(), GL_STREAM_DRAW);

		if (lightning.type == 0) {
			//approximate light bouncing with multiple lights
			lights.push_back(Light(
				transformedVertexPositions[transformedVertexPositions.size() / 2],
				ofVec3f(1.0, 0.0, 0.0),
				-1.0,
				LIGHTNING_LIGHT_ATTENUATION,
				LIGHTNING_LIGHT_INTENSITY * glm::max(lightning.energy, 0.0f)
			));
			lights.push_back(Light(
				transformedVertexPositions[0] + ofVec3f(0.0, 50.0, 0.0),
				ofVec3f(1.0, 0.0, 0.0),
				-1.0,
				LIGHTNING_LIGHT_ATTENUATION,
				LIGHTNING_LIGHT_INTENSITY * glm::max(lightning.energy, 0.0f)
			));
			lights.push_back(Light(
				transformedVertexPositions[0] + ofVec3f(0.0, -50.0, 0.0),
				ofVec3f(1.0, 0.0, 0.0),
				-1.0,
				LIGHTNING_LIGHT_ATTENUATION,
				LIGHTNING_LIGHT_INTENSITY * glm::max(lightning.energy, 0.0f)
			));
		}

		if (lightning.energy <= 0.0 && lightning.totalEnergy <= 0.0) {
			it = lightnings.erase(it);
		} else {
			++it;
		}
	}
}

void ElectricWorld::update(float dt){
	updateCamera(dt);

	float time = Timeline::getInstance().getElapsedCurrWorldTime();

	if (userHeight < INFINITE_BASELINE + CYLINDER_HEIGHT) {
		vector<ofVec3f> disassembledCylinderVertices;
		vector<ofVec3f> disassembledCylinderNormals;
		vector<ofVec2f> disassembledCylinderTextureCoordinates;
		vector<ofVec3f> disassembledCylinderAbcs;

		for (Tile& tile : tiles) {
			float t = clamp((time - tile.assemblyStartTime) / (tile.assemblyEndTime - tile.assemblyStartTime), 0.0, 1.0);
			float rotationAngle = (1.0 - t) * tile.rotationAngle;
			ofQuaternion rotation;
			rotation.makeRotate(rotationAngle, tile.rotationAxis);
			for (int i = 0; i < 3; ++i) {
				ofVec3f vertexPosition = tile.vertices[i];
				vertexPosition = rotation * (vertexPosition - tile.centroid) + tile.centroid;
				disassembledCylinderVertices.push_back(vertexPosition + tile.startOffset - tile.startOffset * t);
				disassembledCylinderNormals.push_back(rotation * tile.normal);

				disassembledCylinderTextureCoordinates.push_back(tile.textureCoordinates[i]);
			}
			disassembledCylinderAbcs.push_back(ofVec3f(1.0, 0.0, 0.0));
			disassembledCylinderAbcs.push_back(ofVec3f(0.0, 1.0, 0.0));
			disassembledCylinderAbcs.push_back(ofVec3f(0.0, 0.0, 1.0));
		}

		disassembledCylinderVbo.clear();

		disassembledCylinderVbo.setVertexData(&disassembledCylinderVertices[0], disassembledCylinderVertices.size(), GL_STREAM_DRAW);
		disassembledCylinderVbo.setNormalData(&disassembledCylinderNormals[0], disassembledCylinderNormals.size(), GL_STREAM_DRAW);
		disassembledCylinderVbo.setTexCoordData(&disassembledCylinderTextureCoordinates[0], disassembledCylinderTextureCoordinates.size(), GL_STREAM_DRAW);
		disassembledCylinderVbo.setAttributeData(tileShader().getAttributeLocation("a_abc"), &disassembledCylinderAbcs[0].x, 3, disassembledCylinderAbcs.size(), GL_STREAM_DRAW, sizeof(ofVec3f));
	}

	lights.clear(); //this is added to by all the update methods

	// play audio cue only once and after 3 seconds
	if (!playedVoicePrompt && Timeline::getInstance().getElapsedCurrWorldTime() >= WORLD_ELECTRIC::DURATION_INTRO + COMMON::AUDIO_CUE_DELAY + 8) {
		playedVoicePrompt = true;
		PlayBackgroundSound(voicePromptFileName, COMMON::VOICE_PROMPT_GAIN);
	}

	lights.push_back(Light(
		ofVec3f(0.0, 100000.0, 0.0),
		ofVec3f(0.0, 1.0, 0.0),
		-1.0,
		0.0,
		AMBIENT_LIGHT_INTENSITY
	));

	updateLightning(dt);
	updateTiles(dt);
	updateSounds(dt);

	// Start autotriggering only after transition
	if (userHeight < INFINITE_BASELINE) return;
	if(!hasTriggered)
	{
		triggerStartTime += dt;
		autoTriggerDischarge();
	}
}

bool ElectricWorld::getUseBloom () {
	return true;
}

BloomParameters ElectricWorld::getBloomParams(){
	BloomParameters params;

	params.bloom_multiplier_color=glm::vec3(0.2);
	params.gamma=1.0;
	params.exposure=1.0;
	params.multisample_antialiasing = 16;

	return params;
}

void ElectricWorld::drawScene(renderType render_type, ofFbo *fbo) {
	if (userHeight < INFINITE_BASELINE) {
		glDisable(GL_DEPTH_TEST);
		/// drawing background
		glActiveTexture(GL_TEXTURE0);
		environment->BindTexture();

		background_shader().begin();
		backgroundSphere.setPosition(0.0, 1.8, 0.0);
		backgroundSphere.draw();
		background_shader().end();
	}

	skyShader().begin();
	ofVec3f skyPlanePosition = ofVec3f(0.0, userHeight, 0.0) + SKY_PLANE_POSITION;
	skyShader().setUniform3f("u_position", skyPlanePosition.x, skyPlanePosition.y, skyPlanePosition.z);
	skyShader().setUniform1i("u_skyTexture", 0);
	ofVec3f cameraPosition = ofVec3f(0.0, userHeight, 0.0);
	skyShader().setUniform3fv("u_cameraPosition", &cameraPosition.x);
	skyShader().setUniform1f("u_evolutionBaseline", EVOLUTION_BASELINE);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, skyImage.getTextureReference().getTextureData().textureID);
	skyVbo.draw(GL_TRIANGLE_STRIP, 0, 6);
	skyShader().end();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);


	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ofDisableArbTex();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glDisable(GL_BLEND);

	vector<float> lightPositions;
	vector<float> lightColors;
	vector<float> lightAttenuations;

	for (unsigned int i = 0; i < 20; ++i) {
		if (i < lights.size()) {
			lightPositions.push_back(lights[i].position.x);
			lightPositions.push_back(lights[i].position.y);
			lightPositions.push_back(lights[i].position.z);

			lightColors.push_back(lights[i].intensity);
			lightColors.push_back(lights[i].intensity);
			lightColors.push_back(lights[i].intensity);

			lightAttenuations.push_back(lights[i].attenuation);
		} else {
			lightPositions.push_back(0);
			lightPositions.push_back(0);
			lightPositions.push_back(0);

			lightColors.push_back(0);
			lightColors.push_back(0);
			lightColors.push_back(0);

			lightAttenuations.push_back(1000000.0);
		}
	}

	tileShader().begin();
	tileShader().setUniform3f("u_cameraPosition", 0.0, userHeight + 1.8, 0.0);

	tileShader().setUniform1f("u_roughness", ROUGHNESS);
	tileShader().setUniform1f("u_ior", IOR);

	tileShader().setUniform1i("u_numberOfLights", lights.size());

	tileShader().setUniform1i("texture", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, slateImage.getTextureReference().getTextureData().textureID);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(10.0, 1.0);

	if (lightPositions.size() > 0 && lightColors.size() > 0 && lightAttenuations.size() > 0) {
		glUniform3fv(glGetUniformLocation(tileShader().getProgram(), "lightPositions"), lightPositions.size() / 3, &lightPositions[0]);
		glUniform3fv(glGetUniformLocation(tileShader().getProgram(), "lightColors"), lightColors.size() / 3, &lightColors[0]);
		glUniform1fv(glGetUniformLocation(tileShader().getProgram(), "lightAttenuations"), lightAttenuations.size(), &lightAttenuations[0]);
	}

	if (userHeight >= INFINITE_BASELINE - CYLINDER_HEIGHT && userHeight <= INFINITE_TOPLINE - CYLINDER_HEIGHT) { //draw cylinder above
		tileShader().setUniform1f("offset", (floor((userHeight - INFINITE_BASELINE) / CYLINDER_HEIGHT) + 1.0) * CYLINDER_HEIGHT + INFINITE_BASELINE);
		cylinderVbo.draw(GL_TRIANGLES, 0, cylinderVbo.getNumVertices());
	}

	if (userHeight >= INFINITE_BASELINE && userHeight <= INFINITE_TOPLINE) { //render cylinder in front
		tileShader().setUniform1f("offset", floor((userHeight - INFINITE_BASELINE) / CYLINDER_HEIGHT) * CYLINDER_HEIGHT + INFINITE_BASELINE);
		cylinderVbo.draw(GL_TRIANGLES, 0, cylinderVbo.getNumVertices());
	}

	if (userHeight >= INFINITE_BASELINE + CYLINDER_HEIGHT && userHeight <= INFINITE_TOPLINE + CYLINDER_HEIGHT) { //draw cylinder below
		tileShader().setUniform1f("offset", (floor((userHeight - INFINITE_BASELINE) / CYLINDER_HEIGHT) - 1.0) * CYLINDER_HEIGHT + INFINITE_BASELINE);
		cylinderVbo.draw(GL_TRIANGLES, 0, cylinderVbo.getNumVertices());
	}

	if (userHeight > INFINITE_TOPLINE - CYLINDER_HEIGHT) {
		tileShader().setUniform1f("offset", INFINITE_TOPLINE);
		dissolvedCylinderVbo.draw(GL_TRIANGLES, 0, dissolvedCylinderVbo.getNumVertices());
	}

	
	if (userHeight < INFINITE_BASELINE + CYLINDER_HEIGHT) {
		glDisable(GL_CULL_FACE);
		tileShader().begin();
		tileShader().setUniform1f("offset", INFINITE_BASELINE - CYLINDER_HEIGHT);
		tileShader().setUniform3f("u_cameraPosition", 0.0, userHeight, 0.0);
		disassembledCylinderVbo.draw(GL_TRIANGLES, 0, disassembledCylinderVbo.getNumVertices());
		tileShader().end();
		glEnable(GL_CULL_FACE);
	}

	vector<ofVec3f> chargedTileVertices;
	vector<ofVec3f> chargedTileNormals;
	vector<float> chargedTileEnergies;

	for (int i = 0; i < chargedTiles.size(); ++i) {
		ofVec3f vertexA = chargedTiles[i]->vertices[0] + ofVec3f(0.0, chargedTiles[i]->offset, 0.0);
		ofVec3f vertexB = chargedTiles[i]->vertices[1] + ofVec3f(0.0, chargedTiles[i]->offset, 0.0);
		ofVec3f vertexC = chargedTiles[i]->vertices[2] + ofVec3f(0.0, chargedTiles[i]->offset, 0.0);

		ofVec3f a = 0.9 * vertexA + 0.05 * vertexB + 0.05 * vertexC;
		ofVec3f b = 0.9 * vertexB + 0.05 * vertexA + 0.05 * vertexC;
		ofVec3f c = 0.9 * vertexC + 0.05 * vertexA + 0.05 * vertexB;

		chargedTileVertices.push_back(a);
		chargedTileVertices.push_back(b);
		chargedTileVertices.push_back(c);

		for (int j = 0; j < 3; ++j) {
			chargedTileNormals.push_back(chargedTiles[i]->normal);
			chargedTileEnergies.push_back(chargedTiles[i]->energy);
		}
	}

	chargedTilesVbo.clear();

	if (chargedTileVertices.size()) {
		chargedTilesVbo.setVertexData(&chargedTileVertices[0], chargedTileVertices.size(), GL_STREAM_DRAW);
		chargedTilesVbo.setNormalData(&chargedTileNormals[0], chargedTileNormals.size(), GL_STREAM_DRAW);
		chargedTilesVbo.setAttributeData(chargedTileShader().getAttributeLocation("a_energy"), &chargedTileEnergies[0], 1, chargedTileEnergies.size(), GL_STREAM_DRAW, sizeof(float));
	}

	glDisable(GL_POLYGON_OFFSET_FILL);

	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

	chargedTileShader().begin();

	chargedTileShader().setUniform3f("u_cameraPosition", 0.0, userHeight + 1.8, 0.0);
	chargedTileShader().setUniform3f("onColor", TILE_ON_COLOR.r, TILE_ON_COLOR.g, TILE_ON_COLOR.b);

	chargedTilesVbo.draw(GL_TRIANGLES, 0, chargedTileVertices.size());

	chargedTileShader().end();

	glDisable(GL_BLEND);

	renderBody(&lightPositions, &lightColors, &lightAttenuations);

	glDepthMask(GL_FALSE);
	lightningShader().begin();
	for (int i = 0; i < lightnings.size(); ++i) {
		lightningShader().setUniform1f("energy", lightnings[i].energy);
		lightnings[i].vbo.draw(GL_LINES, 0, lightnings[i].vbo.getNumVertices());
	}
	lightningShader().end();

}

void ElectricWorld::renderBody(vector<float> *lightPositions, vector<float> *lightColors, vector<float> *lightAttenuations) {

	// transition in
	float transitionTime = 4.0f;
	ofVec3f cameraPosition = ofVec3f(0.0, userHeight + 1.8, 0.0);
	if(Timeline::getInstance().getElapsedCurrWorldTime() < transitionTime)
	{
		float opacity = Timeline::getInstance().getElapsedCurrWorldTime()/transitionTime; // 0 to 1
		engine->kinectEngine.drawBodyBlack(cameraPosition);
		engine->kinectEngine.drawBodyUltramarine(cameraPosition, (1.0f - opacity)); // 1.0+clamp(0.2f*hand_interaction_growth_speed[0], 0.0f, 1.0f), 1.0+clamp(0.2f*hand_interaction_growth_speed[1], 0.0f, 1.0f));
		engine->kinectEngine.drawBodyElectric(cameraPosition, opacity, *lightPositions, *lightColors, *lightAttenuations, hands[0].energy, hands[1].energy);
	}
	else
		engine->kinectEngine.drawBodyElectric(cameraPosition, 1.0f, *lightPositions, *lightColors, *lightAttenuations, hands[0].energy, hands[1].energy);
}

void ElectricWorld::exit() {
	gui->saveSettings("worlds/electric/settings.xml");
}

float ElectricWorld::getDuration() {
	return duration;
}

void ElectricWorld::mouseMoved(int x, int y) {
	mouseDirection = ofVec3f(0.0);
	mouseDirection.x = (static_cast<float>(x) / ofGetWidth()) * 4.0 - 2.0;
	mouseDirection.y = ((1.0 - static_cast<float>(y) / ofGetHeight())) * 3.0 - 1.5;
	mouseDirection.z = -1.0;
	mouseDirection.normalize();
}

void ElectricWorld::mouseDragged(int x, int y, int button) {
	mouseMoved(x, y);
}

void ElectricWorld::mousePressed(int x, int y, int button) {
	if (button == OF_MOUSE_BUTTON_LEFT) {
		if (hands[0].energy > HAND_MIN_DISCHARGE_ENERGY) dischargeHand(0, mouseDirection);
	} else if (button == OF_MOUSE_BUTTON_RIGHT) {
		if (hands[1].energy > HAND_MIN_DISCHARGE_ENERGY) dischargeHand(1, mouseDirection);
	}
}

void ElectricWorld::mouseReleased(int x, int y, int button) {
}

void ElectricWorld::keyPressed(int key) {
	if (key == 'g') {
		gui->toggleVisible();
	}

	World::keyPressed(key);
}

void ElectricWorld::keyReleased(int key){
}

void ElectricWorld::drawDebugOverlay(){
}
