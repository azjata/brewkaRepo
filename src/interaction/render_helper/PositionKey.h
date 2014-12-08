#pragma once

#ifndef __POSITION_KEY_H__
#define __POSITION_KEY_H__

#include "../../glm/gtc/quaternion.hpp"

class PositionKey {
public:
	PositionKey(const float t, const glm::vec3 p) : time(t), position(p) {};
	float time;
	glm::vec3 position;
};

#endif