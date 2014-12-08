#pragma once

#ifndef __BONE_H__
#define __BONE_H__

#include "../../glm/glm.hpp"
#include "../../glm/gtc/quaternion.hpp"

#include "PositionKey.h"
#include "RotationKey.h"

#include <cstring>
#include <string>
#include <vector>

using namespace std;

class Bone {
public:
	std::string name;
	int parentIndex;

	glm::mat4 inverseBindPoseMatrix;
	glm::mat4 relativeMatrix;
	glm::mat4 absoluteMatrix;

	glm::mat4 fullMatrix;

	glm::quat relativeRotation;
	glm::vec3 relativePosition;

	vector<PositionKey> positionKeys;
	vector<RotationKey> rotationKeys;
};

#endif
