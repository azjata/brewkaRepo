#pragma once

#ifndef __TIME_COMMAND__
#define __TIME_COMMAND__

class TimeCommand
{
public:
	TimeCommand(int time);
	~TimeCommand(void);
	bool execute();
	int getTime();
protected:
	int time;
};

#endif