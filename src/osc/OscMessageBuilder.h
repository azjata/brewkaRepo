#pragma once

#ifndef __OSC_MESSAGE_BUILDER_H__
#define __OSC_MESSAGE_BUILDER_H__

#include "ofxOsc.h"
#include "../constants.h"

class OSCMessageBuilder {
public:
	/// <summary>Builds up command for scent system</summary>
	/// <param name="command">ScentControl Command</param>
	/// <param name="startDelay"> Start in seconds</param>
	/// <param name="duration"> Duration in seconds. -1 for infinite</param>
	/// <param name="param"> Parameter to sent from 0 - 1 (for instance heater temp)</param>
	/// <returns>Scent Osc Message</returns>
	static ofxOscMessage BuildScentMessage(R_PI::SCENTCONTROL_COMMANDS command, float startDelay, float duration, float param)
	{
		ofxOscMessage m;
		m.setAddress(R_PI::ScentControlAddresses[command]);
		m.addFloatArg(startDelay);
		m.addFloatArg(duration);
		m.addFloatArg(param);
		return m;
	};

	static ofxOscMessage BuildFilmMessage(bool state,float startDelay, float duration, float param)
	{
		ofxOscMessage m;
		m.setAddress(R_PI::LampControlAddresses[0]);
		m.addFloatArg(startDelay);
		m.addFloatArg(duration);
		m.addFloatArg(param);
		return m;
	}

	/// <summary>Builds up command for fan control</summary>
	/// <param name="command">Fan Control Command</param>
	/// <param name="startDelay">Start in seconds</param>
	/// <param name="duration">Duration in seconds. -1 for infinite</param>
	/// <param name="param">Parameter to sent from 0 - 1</param>
	/// <returns>Fan Osc Message</returns>
	static ofxOscMessage BuildFanMessage(R_PI::FANCONTROL_COMMANDS command, float startDelay, float duration, float param)
	{
		ofxOscMessage m;
		m.setAddress(R_PI::FanControlAddresses[command]);
		m.addFloatArg(startDelay);
		m.addFloatArg(duration);
		m.addFloatArg(param);
		return m;
	};

	/// <summary>Builds up command to reset hardware</summary>
	/// <returns>Reset Osc Message</returns>
	static ofxOscMessage BuildResetMessage()
	{
		ofxOscMessage m;
		m.setAddress(R_PI::ResetAddress);
		return m;
	};

	// <summary>Builds up command for cam server control</summary>
	// <return>Cam Server Message</return>
	static ofxOscMessage BuildCamServerMessage(CAM_SERVER::CAMSERVER_COMMANDS command)
	{
		ofxOscMessage m;
		m.setAddress(CAM_SERVER::CamServerAddresses[command]);
		return m;
	};
};
#endif