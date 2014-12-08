#pragma once

#ifndef __ROTATION_KEY_H__
#define __ROTATION_KEY_H__

#include "../../glm/gtc/quaternion.hpp"

class RotationKey {
public:
	RotationKey(const float t, const glm::quat r) : time(t), rotation(r) {};
	float time;
	glm::quat rotation;
};

#endif