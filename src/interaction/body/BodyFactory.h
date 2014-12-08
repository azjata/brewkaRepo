#pragma once
#ifndef __BODY_FACTORY__
#define __BODY_FACTORY__

#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include "Body.h"
#include "joints/JointSmooth.h"
#include "joints/JointNormal.h"

class BodyFactory {
public:
	static std::shared_ptr<Body> buildBody(std::string bodyData);
private:
	BodyFactory();
	BodyFactory(const BodyFactory &) { }
	BodyFactory &operator=(const BodyFactory &) { return *this; }
	static std::shared_ptr<Joint> parseJoint(std::string jointData);
};

#endif