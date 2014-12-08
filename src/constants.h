#pragma once

#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <string>

static const bool _DEBUG_ = true;
static const bool NO_TILT = false;

namespace WINDOW_SETTINGS {
	static const unsigned int width = 1280;
	static const unsigned int height = 800;
}

namespace OSC_REMOTE {
	static const bool SEND_OSC_MESSAGES = true;
	static const string OSC_HARDWARE_DEFAULT_SERVER = "192.168.168.178";
	static const int OSC_HARDWARE_DEFAULT_PORT = 12345;
}

namespace SETTINGS {
	static const string TAG_OSC_LISTENING_PORT = "osc:osc_listening_port";
	static const int VALUE_OSC_LISTENING_PORT = 1234;
	static const string TAG_KINECT_GROUP_PORT = "kinect:kinect_group_port";
	static const int VALUE_KINECT_GROUP_PORT = 2222;
	static const string TAG_KINECT_GROUP_IP = "kinect:kinect_group_ip";
	static const string VALUE_KINECT_GROUP_IP = "239.0.0.222";
	static const string TAG_RPI_IP = "rpi:rpi_ip";
	static const string VALUE_RPI_IP = "192.168.168.133";
	static const string TAG_RPI_PORT = "rpi:rpi_server_port";
	static const int VALUE_RPI_PORT = 12345;
	static const string TAG_CAM_SERVER_IP = "camserver:cam_server_ip";
	static const string VALUE_CAM_SERVER_IP = "192.168.1.20";
	static const string TAG_CAM_SERVER_PORT = "camserver:cam_server_port";
	static const int VALUE_CAM_SERVER_PORT = 2022;
}

namespace R_PI {

	static const string ScentControlAddresses[] = {
		"/Hardware/ScentControl/Fan/1",
		"/Hardware/ScentControl/Fan/2",
		"/Hardware/ScentControl/Scent/1",
		"/Hardware/ScentControl/Scent/2",
	};

	static const string FanControlAddresses[] = {
		"/Hardware/FanControl/FrontFan",
		"/Hardware/FanControl/CeilingFan"
	};

	static const string LampControlAddresses[] = {
		"/Hardware/LampControl/FilmLamp"
	};

	static const string ResetAddress = "/Hardware/Reset";

	enum SCENTCONTROL_COMMANDS {
		FAN1 = 0,
		FAN2 = 1,
		SCENT1 = 2,
		SCENT2 = 3
	};

	enum FANCONTROL_COMMANDS {
		FRONT_FAN = 0,
		CEILING_FAN = 1
	};

};

namespace CAM_SERVER {
	static const string CamServerAddresses[] = {
		"/experience/start",
		"/experience/mark",
		"/experience/stop"
	};

	enum CAMSERVER_COMMANDS {
		START = 0,
		KEY_FRAME = 1,
		END = 2
	};
};

namespace COMMON {
	enum WORLD_TYPE {
		LIMBO,
		INTRO,
		ULTRAMARINE,
		ELECTRIC,
		LIQUID,
		EVOLUTION,
		BODY
	};
	const float AUDIO_CUE_DELAY = 2.5;
	const float VOICE_PROMPT_GAIN = 1.0;
};

namespace WORLD_INTRO {
	const float DURATION_INTRO = 0.0f;
	const float DURATION_MAIN = 15.0f;
	const float DURATION_END = 0.0f;
	const float REACH_OUT_DURATION = 0.3f;
}

namespace WORLD_ULTRAMARINE {
	const float DURATION_INTRO = 10.0f;
	const float DURATION_MAIN = 35.0f;
	const float DURATION_END = 0.0f;
	const float TOTAL_DURATION = DURATION_INTRO + DURATION_MAIN + DURATION_END;
}

namespace WORLD_LIQUID {
	const float DURATION_INTRO = 1.5f;
	const float DURATION_MAIN = 45.0f;
	const float DURATION_END = 15.0f;
	const float TOTAL_DURATION = DURATION_INTRO + DURATION_MAIN + DURATION_END;
}

namespace WORLD_EVOLUTION {
	const float DURATION_INTRO = 10.0f;
	const float DURATION_MAIN = 999.0f; //advanced manually anyway
	const float DURATION_END = 0.0f;
	const float TOTAL_DURATION = DURATION_INTRO + DURATION_MAIN + DURATION_END;
}

namespace WORLD_ELECTRIC {
	const float DURATION_INTRO = 0.0f;
	const float DURATION_MAIN = 999.0f; //advanced manually anyway
	const float DURATION_END = 0.0f;
	const float TOTAL_DURATION = DURATION_INTRO + DURATION_MAIN + DURATION_END;
}
// define more namespaces for your own classes here...

#endif
