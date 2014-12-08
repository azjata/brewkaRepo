#pragma once

#ifndef __ASSIMP_JOINT_H__
#define __ASSIMP_JOINT_H__

#include "aiMesh.h"
#include "../enums/JointType.h"

class AssimpJoint {
public:
	AssimpJoint(JointType::JointType type_);
    JointType::JointType type;
    aiMatrix4x4 transform;
	~AssimpJoint(void);
};

#endif