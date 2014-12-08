#pragma once

#ifndef __SKINNED_BODY_H__
#define __SKINNED_BODY_H__

#include "ofxAssimpModelLoader.h"
#include "Bone.h"
#include "aiScene.h"
#include "../../glm/gtc/matrix_transform.hpp"
#include "../../glm/gtx/quaternion.hpp"

class SkinnedBody {
	public:
	void load(const string modelLocation);
	void update(float currentTime);

	void updateAnimations (float leftHandTime, float rightHandTime);
	void updateMatrices ();

	std::map<string, int> boneNameToIndex;
	Bone* getBoneByName (const string name);

	vector<Bone> bones;

	vector<int> leftArmAndHandVertexIndices;
	vector<int> rightArmAndHandVertexIndices;

	vector<ofVec3f> vertices;
	vector<ofVec3f> normals;
	vector<ofIndexType> indices;
	vector<ofVec4f> boneIndices;
	vector<ofVec4f> boneWeights;

	vector<float> boneMatricesArray;

	ofVec3f getVertexPosition (int index);
	ofVec3f getVertexNormal (int index);
};

#endif