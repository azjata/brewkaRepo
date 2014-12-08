#ifndef ULTRAMARINEWORLD_H
#define ULTRAMARINEWORLD_H

#define GLM_SWIZZLE
#define GLM_FORCE_RADIANS

#pragma once

#include "audio.h"
#include "engine.h"

#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "videoSphere.h"
#include "ofxAssimpModelLoader.h"
#include "osc/OSCManager.h"
#include "timeline/timeline.h"
#include <memory>
#include <fstream>


class UltramarineWorld;

class Crystals{
public:
	bool is_frost;
	glm::vec3 pos;
	float scale;
	float interaction_delay;
	class Crystal{
	public:
		glm::vec3 start_pos;
		glm::vec3 pos;
		glm::quat orientation;
		glm::vec3 scale;
		int mesh;
		float track_speed;
		float track_pos;

		glm::vec3 velocity;
		glm::vec3 angular_velocity;
		int falling;/// falling stage: 0 growing, 1 hovering around, 2: falling down
		glm::vec4 color;

		Crystal(): start_pos(0), scale(1), mesh(0), track_speed(1), track_pos(0), falling(0), color(0){}
	};
	std::vector<Crystal> crystals;
	struct CrystalDrawParams{
		glm::mat4x4 transform;
		glm::vec4 color;
	};
	std::vector<CrystalDrawParams> crystal_draw_params;
	GLuint buffer_id;

	float crystals_amount;

	glm::vec3 pathPos(float t);
	float growth;
	float max_growth;

	float grow_speed;

	float spiral_phase;

	float radius_scale;
	float speed_scale;
	float spiral_scale;

	glm::vec2 spiral2_base;
	float spiral2_radius;
	float spiral2_rise;

	float fall_time; /// time at which the crystals fall apart

	float grow_sum_tmp;

	bool has_sound;
	SoundObject growth_sound;
	bool growth_sound_active;
	SoundObject fast_growth_sound;
	bool fast_growth_sound_active;

	Crystals();
	~Crystals();
	void setup();
	void draw(UltramarineWorld &world, float time);

	void grow(float time, float dt, float desired_grow_speed);

	void reset();

	void addFrost(glm::vec3 pos);

	void addProjectile(glm::vec3 pos, glm::vec3 vel, glm::vec3 avel, float scale=1.0);

};

class ParticleBeam{
	public:
	ParticleBeam();
	~ParticleBeam();
	int particles_count;

	float beam_power[2];
	glm::vec3 beam_start[2];
	glm::vec3 beam_dir[2];
	/*
	float beam2_power;
	glm::vec3 beam2_start;
	glm::vec3 beam2_dir;*/

	struct Particle{
		glm::vec4 pos; /// position and lifetime
		glm::vec4 vel;/// velocity and fluff
	};
	void setup();
	void draw(float time);
	void update(float dt);

	glm::vec4 velocityField(glm::vec3 pos);/// xyz is velocity, w is influence

	private:
	GLuint buffer_id;
	GLuint feedback_buffer_id;
	GLuint animate_program_id;
};


class UltramarineWorld: public World
{
	friend class Crystals;
	public:
		DECLARE_WORLD /// this macro adds getName method
		UltramarineWorld(Engine *engine);
		virtual ~UltramarineWorld();

		virtual void setup();/// called on the start, after the opengl has been set up (mostly you can allocate resources there to avoid frame skips later)
		void setupCrystals();
		void setupGUI();
		virtual void reset();
		virtual void beginExperience();/// called right before we enter the world
		virtual void endExperience();/// called when we leave the world (free up resources here)

		virtual void update(float dt); /// called on every frame while in the world
		void updateKinect(float dt); /// also updates mouse

		virtual void drawScene(renderType render_type, ofFbo *fbo);/// fbo may be null . fbo is already bound before this call is made, and is provided so that the renderer knows the resolution

		void resetVideoSphere();

		virtual float getDuration();/// duration in seconds

		virtual BloomParameters getBloomParams();

		/// for testing
		virtual void keyPressed(int key);
		virtual void keyReleased(int key);
		virtual void drawDebugOverlay();
		virtual void mouseMoved(int x, int y);

		ofxUICanvas *gui;
		void guiEvent(ofxUIEventArgs &e);
	protected:
		float old_time;
		float time;
		VideoSphere *videoSphere;

		std::vector<std::shared_ptr<Crystals> > crystals;/// multiple spawning locations

		Crystals frost;

		bool draw_ground;
		bool do_updates;

		float non_interaction_time;

		ofxAssimpModelLoader model;
		glm::mat4 model_transform;

		struct GroundPiece {
			int model_mesh;
			glm::vec3 offset;
			glm::quat orientation;
			glm::vec3 scale;
			glm::vec3 local_centroid;
			glm::vec3 centroid;

			glm::vec3 angular_velocity;
			glm::quat target_orientation;
			float spring;
			float damping;

			float jolt_timer;// counts down, when <=0, angular velocity is zeroed.
			//glm::vec3 axis;
			float stress;/// amount of stress before the piece jolts

			float fall_time;/// time at which the piece falls down. Is set to 0.0 once piece starts falling
			glm::vec3 velocity;
			//bool falling;

			ofMesh mesh;
			GroundPiece(): model_mesh(0), offset(0.0), orientation(1,0,0,0), scale(1.0), local_centroid(0.0), centroid(0.0),
			angular_velocity(0.0), target_orientation(orientation), spring(0.0), damping(0.0), jolt_timer(0.0), stress(0.0), fall_time(100.0),
			velocity(0){}
			void reset(){
				offset=vec3(0);
				orientation=glm::quat(1,0,0,0);
				scale=vec3(1.0);
				velocity=angular_velocity=vec3(0);
				target_orientation=orientation=glm::quat(1,0,0,0);
				spring=damping=jolt_timer=0.0;
			}
		};
		std::vector<GroundPiece> pieces;
		glm::vec3 mouse_dir;

		ofImage floor_normals, floor_ambient, floor_diffuse, background_back_image, background_fore_image;

		StrongPtr<Texture> environment;

		StrongPtr<TextureFramebuffer> reflection;

		void drawGround();

		void setupMultiCrystals(glm::vec3 pos, float scale, float interaction_delay, int n);

		ofSpherePrimitive	*backgroundSphere;

		ofCylinderPrimitive backCylinder, foreCylinder; //this is temporary

		struct Snowflake{
			glm::vec3 pos;
			glm::vec3 vel;
			glm::quat orientation;
			glm::vec3 avel;/// angular velocity
			float size;
			float fluff; // drag per mass.
		};

		std::vector<Snowflake> snowflakes;

		void setupSnowflakes();
		bool resetSnowflakeIntoBeam(glm::vec3 &p);
		void updateSnowflakes(float dt);
		void drawSnowflakes(float transparency_factor=1.0);

		ofVbo snowflakes_vbo;

		bool played_intro_sound;
		bool played_crash_sound;

		void drawEnvironmentAsBackground();

		ParticleBeam particle_beam;

		float hand_interaction_growth_speed[2];

		float hand_non_interaction_time[2];

		glm::vec3 kinect_hand_directions[2];
		glm::vec3 kinect_hand_positions[2];
		float kinect_hand_pointing_time[2]; // for how long the hand has been extended

		static const int kinect_hand_direction_history_count=120;
		glm::vec3 kinect_hand_direction_history[kinect_hand_direction_history_count][2];
		glm::vec3 kinect_hand_position_history[kinect_hand_direction_history_count][2];
		int kinect_hand_direction_history_pos;
		float kinect_hand_direction_history_time;
		bool kinect_hand_motion_below_threshold[2];/// motion over the history has been below threshold
		bool kinect_hand_gesture[2];
		bool kinect_hand_dir_reversal[2];
		bool kinect_hand_gesture_spent[2];
		glm::vec3 kinect_hand_gesture_spent_dir[2];

		glm::vec3 getHistoryDir(int hand, float time_ago);
		glm::vec3 getHistoryPos(int hand, float time_ago);

		SoundObject ambientSound;

		std::ofstream motion_log;

		bool TimeAt(float t);

};


#endif // ULTRAMARINEWORLD_H
