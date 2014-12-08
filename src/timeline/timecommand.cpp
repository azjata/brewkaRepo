#include "timecommand.h"


TimeCommand::TimeCommand(int time)
{
	this->time = time;
}


TimeCommand::~TimeCommand(void)
{
}

int TimeCommand::getTime() {
	return time;
}