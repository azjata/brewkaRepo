#pragma once
#ifndef __USER_MANAGER__
#define __USER_MANAGER__

#include <vector>
#include "body/Body.h"
#include <mutex>
#include <memory>

class UserManager {
public:
	static UserManager& getInstance() {
		static UserManager instance;
		return instance;
    }
	std::shared_ptr<Body> getActiveUser();
	bool hasUsers();
	void filterUsers(vector<std::shared_ptr<Body> > bodies);
	void lock();
	void unlock();

private:
	UserManager(): user(NULL) {
	};
    UserManager(UserManager const&);
    void operator=(UserManager const&);
	// the internal mutex called through lock() & unlock()
	std::shared_ptr<Body> user;
	ofMutex usersMutex;
};

#endif