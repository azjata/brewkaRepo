#include "UserManager.h"

std::shared_ptr<Body> UserManager::getActiveUser() {
	return user;
}

bool UserManager::hasUsers() {
	return user != NULL;
}

void UserManager::lock() {
    usersMutex.lock();
}

void UserManager::unlock() {
    usersMutex.unlock();
}

void UserManager::filterUsers(vector<std::shared_ptr<Body> > bodies) {
	usersMutex.lock();
	std::shared_ptr<Body> candidateUser = NULL;
	std::shared_ptr<Body> activeUser = NULL;
	// get the user closes to the center of the kinect
	float lowestX = 10000.0;
	for (std::shared_ptr<Body> body : bodies) {
		if (body && body->getJoint(JointType::Head)){
			float xOffset = abs(body->getJoint(JointType::Head)->getPosition().x);
			if (xOffset < lowestX) {
				lowestX = xOffset;
				candidateUser = body;
			}
		}
	}
	if (user != NULL && candidateUser!=NULL) {
		// if there is a previous user, just update
		user->updateBody(candidateUser);
	} else {
		// if there are no users, just add this new user
		user = candidateUser;
	}
	usersMutex.unlock();
}