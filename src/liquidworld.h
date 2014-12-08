#ifndef LIQUIDWORLD_H
#define LIQUIDWORLD_H

#pragma once

#include "engine.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "timeline/timeline.h"
#include "audio.h"

class VideoSphere;

class LiquidWorld: public World
{
	public:
		DECLARE_WORLD /// this macro adds getName method
		LiquidWorld(Engine *engine);
		virtual ~LiquidWorld();

		virtual void setup();/// called on the start, after the opengl has been set up (mostly you can allocate resources there to avoid frame skips later)
		glm::vec3 getCamPosAtTime(float time);/// does intro and outro motion
		bool interactiveAtTime(float time);

		virtual void reset();
		void resetVideoSphere();
		virtual void beginExperience();/// called right before we enter the world
		virtual void endExperience();/// called when we leave the world (free up resources here)
		virtual void update(float dt); /// called on every frame while in the world


		virtual void drawScene(renderType render_type, ofFbo *fbo);/// fbo may be null . fbo is already bound before this call is made, and is provided so that the renderer knows the resolution
		/// note: it is also responsible for rendering the transition to the next world
		virtual void drawTransitionObject(renderType render_type, ofFbo *fbo); /// the object for transitioning *into* this world


		virtual float getDuration();/// duration in seconds

		/// for testing
		virtual void keyPressed(int key);
		virtual void keyReleased(int key);
		virtual void drawDebugOverlay();

		BloomParameters getBloomParams();



	protected:
		ofImage environment;
		TextureFramebuffer *destruction_voxels;
		TextureFramebuffer *particle_velocities;

		//TextureFramebuffer *feedback_destruction;
		std::vector<glm::vec4> particle_velocities_data;
		struct SoundCellParams{
			int fresh_bubbles;
			int old_bubbles;
			glm::vec3 centroid_fresh;
			glm::vec3 centroid_old;
			SoundObject fresh_sound;
			bool fresh_sound_active;
			float gain;
			SoundCellParams():fresh_sound_active(false), gain(0.0)
			{
			}
		};
		std::vector<SoundCellParams> sound_cells;
		int PosToSoundCell(glm::vec3 pos);
		glm::vec3 SoundCellToPos(int ix, int iy, int iz);


		GLuint fragment_lists_txid;
		GLuint fragment_counter_txid;
		int render_width, render_height;


		void setupInteraction();
		void discardInteraction();
		void clear3DTexture(TextureFramebuffer *texture);
		void setupFragmentLists();
		void discardFragmentLists();

		/// renders a cone into attached 3d texture
		void drawDestruction(TextureFramebuffer *texture, glm::vec3 pos, float radius);
		void drawParticlesInto3DTexture(TextureFramebuffer *texture);
		void drawParticlesOnScreen(); /// also populates particle_velocities_data
		void drawBody();
		void playInteractionSounds();/// must be called after drawParticlesOnScreen();

		ofVbo particles;
		ofVbo planes;/// for rendering into every voxel of 3d texture using a shader


		std::vector<ofVec3f> test_verts;
		glm::mat4x4 to_voxels;
		glm::mat4x4 from_voxels;
		float time;
		glm::vec3 voxels_offset_vel;
		glm::vec3 voxels_offset;/// equal to voxels_offset_vel*time,  used to shift the voxels at the user, using wrap-around to imitate endless 3d texture
		/**
		Voxel wrap around space: 0..1 as usual texture
		Transformation to voxel space:
		pv=mod(to_voxels*p-voxels_offset , 1.0);
		transformation from voxel space:
		v=from_voxels*mod(pv+voxels_offset, 1.0);

		vec3 p=(from_voxels*vec4(mod(pos+voxels_offset, 1.0),1.0)).xyz;

		voxels_offset works as position of the
		*/
		glm::vec3 destruction_dir;
		float liquid_disappear;/// from 0 to 1, 0 draw normally, 1 liquid disappeared

		void SetTransformUniforms(ofShader &shader);
		void mouseMoved(int x, int y);
		int fragment_buffer_layers;

		VideoSphere *videoSphere;
		VideoSphere *loopVideoSphere;	
		bool started_video;
		bool startedPlaying;

		bool reachedEnd;
		float cameraPitch;

	private:
};

#endif // LIQUIDWORLD_H
