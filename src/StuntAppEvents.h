#ifndef STUNTAPPEVENTS_H
#define STUNTAPPEVENTS_H

#include "ofEvents.h"
namespace StuntAppEvents
{
	enum StuntEventType
	{
		UNKNOWN = -1,
		START = 0,
		RESET = 1,
		FANS = 2,
		CALIBRATE = 3,
		KINECT_RESET = 4
	};
	
//	static ofEvent<int> StuntAppEventChange = ofEvent<int>();

};

#endif