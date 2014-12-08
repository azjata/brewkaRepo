#define USE_VIDEO_RESET
#define GLM_SWIZZLE
#define GLM_FORCE_RADIANS

#include "liquidworld.h"
#include "resourcemanager.h"
#include "glm/gtc/matrix_transform.hpp"
#include "utils.h"
#include "audio.h"
#include "constants.h"
#include "videoSphere.h"


const float AMBIENCE_GAIN = 1.0;

const float world_scale=0.0125;

const float interaction_sound_gain=2.0f;/// there's a lot of distance attenuation on the interaction sounds

const int voxel_resolution=64;

const int sound_resolution_divisor=2;
const int sound_cell_resolution=4;
const float sound_cell_size=50.0f;


const int max_fragment_buffer_layers=20;

const int particle_feedback_resolution=32;

const bool intro_outro_smooth=true;

const float intro_speed=10.0*world_scale/0.025; ///meters per second
const float outro_speed=20.0*world_scale/0.025;
const float outro_accel_duration=6.0;

const float liquid_disappear_start = WORLD_LIQUID::TOTAL_DURATION-12;
const float liquid_disappear_end = WORLD_LIQUID::TOTAL_DURATION-6;

const float video_start_time = WORLD_LIQUID::TOTAL_DURATION-12;

/*

Liquid interaction processing pipeline

transformations:

Two matrices,
to_voxels
from_voxels

voxel coordinates: 0..1 for both rendering and retrieval

particle pipeline:
particle velocities and start times are stored in an extra 3d texture of GL_RGBA32F format, called
particle_vel_texture

particles are rendered into the green channel of destruction_voxels


*/



DEFINE_WORLD(LiquidWorld) /// this adds the world to the global world list so it can be created by name

LiquidWorld::LiquidWorld(Engine *engine):World(engine), destruction_voxels(NULL), particle_velocities(NULL),
fragment_lists_txid(0), fragment_counter_txid(0), render_width(0), render_height(0),
time(0), voxels_offset(0), voxels_offset_vel(0, 0, 0.05*1.6), destruction_dir(0, 0, -1), liquid_disappear(0), fragment_buffer_layers(0),
videoSphere(NULL),loopVideoSphere(NULL), started_video(true), startedPlaying(false), reachedEnd(false)
{
	if(NO_TILT)
		cameraPitch = 0.0;
	else
		cameraPitch = 30.0;

	//ctor
	std::cout<<"LiquidWorld constructor called"<<std::endl;
	voicePromptFileName = "sounds/voice_prompts/voice_prompt_turbulence.ogg";
	voicePromptEndFileName = "sounds/voice_prompts/voice_prompt_end.ogg";
}

LiquidWorld::~LiquidWorld()
{
	//dtor
	discardInteraction();
	delete videoSphere;
	delete loopVideoSphere;
}



ShaderResource liquid_shader("worlds/liquid/liquid.vert", "worlds/liquid/liquid.frag");
//ShaderResource liquid_shader("worlds/liquid/liquid.vert", "worlds/liquid/inspect_3d_texture.frag");

ShaderResource liquid_transition_shader("worlds/liquid/liquid.vert", "worlds/liquid/liquid.frag", "", "#define TRANSITION");

ShaderResource particle3d_shader("worlds/liquid/particle3d.vert", "worlds/liquid/particle3d.frag", "worlds/liquid/particle3d.geom");
//ShaderResource cone3d_shader("worlds/liquid/cone3d.vert", "worlds/liquid/cone3d.frag", "worlds/liquid/cone3d.geom");
ShaderResource destruction_shader("worlds/liquid/destruction.vert", "worlds/liquid/destruction.frag", "worlds/liquid/destruction.geom");

ShaderResource clear3d_shader("worlds/liquid/clear3d.vert", "worlds/liquid/clear3d.frag", "worlds/liquid/clear3d.geom");

ShaderResource gen_particles_shader("worlds/liquid/gen_particles.vert", "worlds/liquid/liquid.frag", "worlds/liquid/gen_particles.geom",
									"#define GENERATE_PARTICLES");

ShaderResource point_sphere_shader("worlds/liquid/point_sphere.vert", "worlds/liquid/point_sphere.frag", "worlds/liquid/point_sphere.geom");
ShaderResource clear_fragment_counter_shader("worlds/liquid/clear_fragment_counter.vert", "worlds/liquid/clear_fragment_counter.frag");



//ShaderResource inspect_3d_texture_shader("worlds/liquid/liquid.vert", "worlds/liquid/inspect_3d_texture.frag");

void PrintMat(glm::mat4x4 m){
	for(int i=0;i<4;++i){
		for(int j=0;j<4;++j){
			std::cout<<m[i][j]<<" ";
		}
		std::cout<<std::endl;
	}
}

const int interact_sounds_count=12;
const char* interact_sounds[interact_sounds_count]={
"Bubbles_ALot1.wav",
"Bubbles_ALot2.wav",
"Bubbles_Alot3.wav",
"Bubbles_ALot4.wav",
"Bubbles_Few1.wav",
"Bubbles_Few2.wav",
"Bubbles_Few3.wav",
"Bubbles_Few4.wav",
"Bubbles_More1.wav",
"Bubbles_More2.wav",
"Bubbles_More3.wav",
"Bubbles_More4.wav"
};

std::string Sound_Interact(){/// size = 0..2

	int n=ofRandom(interact_sounds_count);
	if(n<0)n=0;
	if(n>interact_sounds_count-1)n=interact_sounds_count-1;
	return std::string("worlds/liquid/sounds/")+interact_sounds[n];

	//return "worlds/liquid/sounds/Bubbles_ALot2.wav";
}

void LiquidWorld::setup(){
	cout << "LiquidWorld::setup start" << endl;
	fragment_buffer_layers = max_fragment_buffer_layers;
	World::setup();
	duration = FLT_MAX;// WORLD_LIQUID::TOTAL_DURATION;
	//shader.load("worlds/liquid/liquid.vert", "worlds/liquid/liquid.frag");
    ofDisableArbTex();
	environment.loadImage("worlds/liquid/environment_map.png");
    ofEnableArbTex();

	camera.setupPerspective(false, 90, 0.01, 200);

	camera.setPosition(toOF(getCamPosAtTime(0.0)));

    std::vector<ofVec3f> tmp(voxel_resolution);
    for(int i=0; i<voxel_resolution; ++i){
    	tmp[i]=ofVec3f(0,0,i);
    }
    planes.setVertexData(&tmp[0], tmp.size(), GL_STATIC_DRAW);

	const float sz=200;
	from_voxels=glm::translate(glm::mat4x4(1.0), glm::vec3(-sz/2, -sz/2, -sz*2/3))*glm::mat4x4(sz,0,0,0 , 0,sz,0,0, 0,0,sz,0, 0,0,0,1);
	to_voxels=glm::inverse(from_voxels);
	PrintMat(from_voxels);
	PrintMat(to_voxels);

    test_verts.resize(voxel_resolution*voxel_resolution*voxel_resolution);
    int n=0;
    for(int i=0; i<voxel_resolution; ++i){
    	for(int j=0; j<voxel_resolution; ++j){
			for(int k=0; k<voxel_resolution; ++k){
				test_verts[n]=ofVec3f(k+0.5, j+0.5, i+0.5);
				n++;
			}
    	}
    }
	/*
    for(int i=0; i<test_verts.size();++i){
    	test_verts[i]=ofVec3f(ofRandom(-sz/2,sz/2),ofRandom(-sz/2,sz/2),ofRandom(0,-sz));
    }*/

    particles.setVertexData(&test_verts[0], test_verts.size(), GL_STATIC_DRAW);

    particle_velocities_data.resize(voxel_resolution*voxel_resolution*voxel_resolution);


    GlobAudioManager();
    sound_cells.resize(sound_cell_resolution*sound_cell_resolution*sound_cell_resolution);

    for(auto &c : sound_cells){
    	c.fresh_sound.al_buffer=GetALBuffer(Sound_Interact());
		c.fresh_sound.sound_flags=sound_flag_repeat;
		//c.fresh_sound.playback_start=ofRandom(5.0);
    }

	//cache sounds to avoid stutter
	GetALBuffer(voicePromptFileName);
	GetALBuffer(voicePromptEndFileName);
	GetALBuffer("worlds/liquid/sounds/ambient.wav");
	
	started_video = true;
	resetVideoSphere();

	cout << "LiquidWorld::setup end" << endl;
}

float CubicAccelCurve(float t, float jerk, float max_accel, float max_vel){
	/// constant jerk case: t<max_accel/jerk
	float result=0;
	if(t<max_accel/jerk){
		float a=t*jerk;
		float v=0.5*t*t*jerk;/// must not exceed max_vel
		float x=(1.0/6.0)*t*t*t*jerk;
		return x;
	}
	float t0=max_accel/jerk;
	float v0=0.5*t0*t0*jerk;/// must not exceed max_vel
	float x0=(1.0/6.0)*t0*t0*t0*jerk;
	float t1=t-t0;
	if(t1<(max_vel-v0)/max_accel){/// constant acceleration
		float x=x0+v0*t1+max_accel*t1*t1*0.5;
		return x;
	}
	/// constant velocity
	float t2=t1-(max_vel-v0)/max_accel;
	t1=(max_vel-v0)/max_accel;
	float x=x0+v0*t1+max_accel*t1*t1*0.5+max_vel*t2;
	return x;
}

glm::vec3 LiquidWorld::getCamPosAtTime(float time){
	if(time<WORLD_LIQUID::DURATION_INTRO){
		if(intro_outro_smooth){
			float t=(WORLD_LIQUID::DURATION_INTRO-time);
			float a=intro_speed/WORLD_LIQUID::DURATION_INTRO; /// acceleration to go from top speed to 0 in that time
			//return glm::vec3(0.0f, 0.0f, a*t*t*0.5f);
			return glm::vec3(0.0f, 0.0f, CubicAccelCurve(t, a, a, 1E5));
		}else{
			return glm::vec3(0, 0, intro_speed*(WORLD_LIQUID::DURATION_INTRO-time));
		}
	}
	float outro_start=WORLD_LIQUID::DURATION_INTRO+WORLD_LIQUID::DURATION_MAIN;

	if(time>outro_start){
		if(intro_outro_smooth){
			float t=(time-outro_start);

			/// prevent it going further in single world mode.
			if(t>WORLD_LIQUID::DURATION_END)t=WORLD_LIQUID::DURATION_END;

			float a=outro_speed/outro_accel_duration; /// acceleration to go from 0 to top speed
			//return glm::vec3(0.0f, 0.0f, -a*t*t*0.5f);
			return glm::vec3(0.0f, 0.0f, -CubicAccelCurve(t, 0.5*a, a, outro_speed));
		}else{
			return glm::vec3(0, 0, -outro_speed*(time-outro_start));
		}
	}
	return glm::vec3(0);
}
bool LiquidWorld::interactiveAtTime(float time){
	return time>WORLD_LIQUID::DURATION_INTRO && time<WORLD_LIQUID::DURATION_INTRO+WORLD_LIQUID::DURATION_MAIN;
}

void LiquidWorld::resetVideoSphere() {
	if (started_video){/// if it needs resetting
#ifdef USE_VIDEO_RESET
		if(videoSphere){
			videoSphere->pause();
			videoSphere->rewind();
		}else{
			videoSphere = new VideoSphere(engine, "video/Container_Outro_x.mov");
		}
		started_video = false;

		if(loopVideoSphere){
			loopVideoSphere->pause();
			loopVideoSphere->rewind();
		}else{
			loopVideoSphere = new VideoSphere(engine, "video/intro_Loop.mov");
			loopVideoSphere->loopVideo(true);
		}
#else
		if (videoSphere) {
			videoSphere->close();
			delete videoSphere;
		}

		if(loopVideoSphere){
			loopVideoSphere->close();
			delete loopVideoSphere;
		}
		std::cout << "LiquidWorld: creating videoSphere" << std::endl;
		videoSphere = new VideoSphere(engine, "video/Container_Outro.mov");
		loopVideoSphere = new VideoSphere(engine, "video/intro_Loop.mov");
		loopVideoSphere->loopVideo(true);
		started_video = false;
#endif
	}
}

void LiquidWorld::reset(){
	cout << "LiquidWorld::reset" << endl;
	liquid_disappear = 0;
	duration = FLT_MAX;// WORLD_LIQUID::TOTAL_DURATION;
	resetVideoSphere();
	startedPlaying = false;
}

void LiquidWorld::setupInteraction(){
	ofLog(OF_LOG_NOTICE,"LiquidWorld::setupInteraction");
	if(!destruction_voxels)destruction_voxels=new TextureFramebuffer(voxel_resolution, voxel_resolution, voxel_resolution, false,  GL_R32F, GL_RED, GL_UNSIGNED_BYTE);
	if(!particle_velocities)particle_velocities=new TextureFramebuffer(voxel_resolution, voxel_resolution, voxel_resolution, false, GL_RGBA32F, GL_RGBA, GL_UNSIGNED_BYTE);

	//clear3DTexture(destruction_voxels);
	destruction_voxels->BeginFB();
	glClearColor(1E5,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	destruction_voxels->EndFB();

	clear3DTexture(particle_velocities);

	ofLog(OF_LOG_NOTICE,"LiquidWorld::setupInteraction done");


	for(SoundCellParams &c : sound_cells){
		if(c.fresh_sound_active){
			c.fresh_sound_active=false;
			GlobAudioManager().RemoveSoundObject(&c.fresh_sound);
		}
	}
}
void LiquidWorld::discardInteraction(){
	delete destruction_voxels;
	delete particle_velocities;
	destruction_voxels=NULL;
	particle_velocities=NULL;
	discardFragmentLists();
}

void LiquidWorld::setupFragmentLists(){/// also resizes them
	if(!fragment_lists_txid){
		glGenTextures(1, &fragment_lists_txid);
	}
	if(!fragment_counter_txid){
		glGenTextures(1, &fragment_counter_txid);
	}
	if(render_width!=engine->getRenderWidth() || render_height!=engine->getRenderHeight()){
		render_width=engine->getRenderWidth();
		render_height=engine->getRenderHeight();
		std::cout<<"LiquidWorld: resolution changed to "<<render_width<<" "<<render_height<<std::endl;
		glGetError();
		CHECK_GL(glActiveTexture(GL_TEXTURE0);)
		CHECK_GL(glBindTexture(GL_TEXTURE_2D_ARRAY, fragment_lists_txid);)
		CHECK_GL(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);)
		CHECK_GL(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);)

		fragment_buffer_layers = max_fragment_buffer_layers;
		glGetError();
		while (fragment_buffer_layers>0){
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, render_width, render_height, fragment_buffer_layers, 0, GL_RGBA, GL_FLOAT, 0);
			if (glGetError() == GL_NO_ERROR) break;
			fragment_buffer_layers -= std::max(1, (fragment_buffer_layers / 8));
		}
		std::cout << "LiquidWorld: initialized particle rendering buffer with "<<fragment_buffer_layers<<" layers"<<std::endl;

		//CHECK_GL(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, render_width, render_height, fragment_buffer_layers, 0,  GL_RGBA, GL_FLOAT, 0);)

		CHECK_GL(glBindTexture(GL_TEXTURE_2D_ARRAY, 0);)


		CHECK_GL(glBindTexture(GL_TEXTURE_2D, fragment_counter_txid);)

		// Set filter
		CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);)
		CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);)

		CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, render_width, render_height, 0,  GL_RED, GL_FLOAT, 0);)

		CHECK_GL(glBindTexture(GL_TEXTURE_2D, 0);)

	}
}
void LiquidWorld::discardFragmentLists(){
	if(fragment_lists_txid){
		glDeleteTextures(1, &fragment_lists_txid);
		fragment_lists_txid=0;
	}
	if(fragment_counter_txid){
		glDeleteTextures(1, &fragment_counter_txid);
		fragment_counter_txid=0;
	}
}

void LiquidWorld::clear3DTexture(TextureFramebuffer *texture){
	texture->BeginFB();
	glClearColor(0,0,0,-1E3);
	glClear(GL_COLOR_BUFFER_BIT);
	texture->EndFB();
}

void LiquidWorld::SetTransformUniforms(ofShader &shader){
	shader.setUniformMatrix4f("to_voxels", ofMatrix4x4(&to_voxels[0][0]));
	shader.setUniformMatrix4f("from_voxels", ofMatrix4x4(&from_voxels[0][0]));
	shader.setUniform3f("voxels_offset_vel", voxels_offset_vel.x, voxels_offset_vel.y, voxels_offset_vel.z);
	shader.setUniform3f("voxels_offset", voxels_offset.x, voxels_offset.y, voxels_offset.z);
	shader.setUniform1f("time", time);
}

void LiquidWorld::drawDestruction(TextureFramebuffer *texture, glm::vec3 pos, float radius){
	glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
	glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);

	//float time=engine->getCurrentWorldTime();

	//std::cout<<time<<std::endl;
	glDisable(GL_CULL_FACE);
	//glm::vec3 dir=glm::normalize(dir_);
	/// first generate the particles
	if(particle_velocities){
		particle_velocities->BeginFB();
		glDisable(GL_BLEND);
		//glBlendFunc(GL_ONE, GL_ZERO);
		//glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_SRC_ALPHA, GL_ZERO);
		//glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		gen_particles_shader().begin();
		gen_particles_shader().setUniform3f("destruction_pos", pos.x, pos.y, pos.z);
		gen_particles_shader().setUniform1f("destruction_r", radius);
		gen_particles_shader().setUniform1f("disappear", liquid_disappear);

		SetTransformUniforms(gen_particles_shader());
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_3D);
		texture->BindTexture();
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gen_particles_shader().setUniform1f("destruction_voxels", 0);
		planes.draw(GL_POINTS, 0, voxel_resolution);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gen_particles_shader().end();
		particle_velocities->EndFB();
		glDisable(GL_TEXTURE_3D);
	}
	/// then draw the sphere into the target
	texture->BeginFB();
	glEnable(GL_BLEND);
	glBlendEquation(GL_MIN);
	//glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	destruction_shader().begin();
	destruction_shader().setUniform3f("destruction_pos", pos.x, pos.y, pos.z);
	destruction_shader().setUniform1f("destruction_r", radius);

	SetTransformUniforms(destruction_shader());

	planes.draw(GL_POINTS, 0, voxel_resolution);
	destruction_shader().end();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glBlendEquation(GL_FUNC_ADD);/// restore to default
	glDisable(GL_BLEND);

	clear3d_shader().begin();/// clear one plane
	clear3d_shader().setUniform3f("voxels_offset", voxels_offset.x, voxels_offset.y, voxels_offset.z);
	glBegin(GL_POINTS);
	glVertex3f(0,0,0);
	glEnd();
	clear3d_shader().end();

	texture->EndFB();
}

void LiquidWorld::drawParticlesInto3DTexture(TextureFramebuffer *texture){
	//float time=engine->getCurrentWorldTime();

	texture->BeginFB();
	particle3d_shader().begin();
	SetTransformUniforms(particle3d_shader());
	//particle3d_shader().setUniform1f("time", time);

	glEnable(GL_TEXTURE_3D);
	particle_velocities->BindTexture();
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	particle3d_shader().setUniform1i("particle_velocities", 0);
	glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);

	glEnable(GL_BLEND);
	glBlendEquation(GL_MAX);

	particles.draw(GL_POINTS, 0, test_verts.size());

	glBlendEquation(GL_FUNC_ADD);/// restore to default
	glDisable(GL_BLEND);

	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	particle3d_shader().end();
	texture->EndFB();
	glDisable(GL_TEXTURE_3D);
}

void LiquidWorld::drawParticlesOnScreen(){
	//float time=engine->getCurrentWorldTime();

	ofDisableDepthTest();
    glDepthMask(GL_FALSE);

	setupFragmentLists();
	glGetError();
    CHECK_GL(glBindImageTextureEXT(0, fragment_lists_txid, 0, true, 0,  GL_READ_WRITE, GL_RGBA32F); )
    CHECK_GL(glBindImageTextureEXT(1, fragment_counter_txid, 0, false, 0,  GL_READ_WRITE, GL_R32UI); )

	/// clear the fragment counter
    clear_fragment_counter_shader().begin();
    clear_fragment_counter_shader().setUniform1i("fragment_lists", 0);
    clear_fragment_counter_shader().setUniform1i("fragment_counter", 1);
    clear_fragment_counter_shader().setUniform1i("max_fragment_count", fragment_buffer_layers);
    glBegin(GL_QUADS);
    glVertex2f(-1,-1);

    glVertex2f(-1,1);

    glVertex2f(1,1);

    glVertex2f(1,-1);
    glEnd();
    clear_fragment_counter_shader().end();
    /// ensure that the operation is complete
    glMemoryBarrierEXT(GL_ALL_BARRIER_BITS_EXT);

    ofEnableDepthTest();
    glDepthMask(GL_TRUE);

    /// draw point spheres to it
    point_sphere_shader().begin();
	SetTransformUniforms(point_sphere_shader());

	//point_sphere_shader().setUniform1f("time", time);
	glActiveTexture(GL_TEXTURE0);
	point_sphere_shader().setUniform1i("particle_velocities", 0);

	glEnable(GL_TEXTURE_3D);
	particle_velocities->BindTexture();
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glActiveTexture(GL_TEXTURE1);
    point_sphere_shader().setUniform1i("env", 1);
    glBindTexture(GL_TEXTURE_2D, environment.getTextureReference().getTextureData().textureID);

    point_sphere_shader().setUniform1i("fragment_lists", 0);
    point_sphere_shader().setUniform1i("fragment_counter", 1);
    point_sphere_shader().setUniform1i("max_fragment_count", fragment_buffer_layers);

    particles.draw(GL_POINTS, 0, test_verts.size());

    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);

    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, (void *)&particle_velocities_data[0]);

    glBindTexture(GL_TEXTURE_3D, 0);
    glDisable(GL_TEXTURE_3D);

	point_sphere_shader().end();

	glBindImageTextureEXT(0, 0, 0, true, 0,  GL_READ_WRITE, GL_RGBA32F);
    glBindImageTextureEXT(1, 0, 0, false, 0,  GL_READ_WRITE, GL_R32UI);
}

float SmoothRise(float f, float knee){
	float a=0.5*f/knee;
	return knee*2.0*(a<0.5?a*a:a-0.25);
}

int LiquidWorld::PosToSoundCell(glm::vec3 pos){
	int ix=pos.x/sound_cell_size + sound_cell_resolution/2;
	int iy=pos.y/sound_cell_size + sound_cell_resolution/2;
	//int iz=pos.z/sound_cell_size + sound_cell_resolution/2;
	int iz=-pos.z/sound_cell_size;
	if(ix<0)ix=0;
	if(ix>sound_cell_resolution-1)ix=sound_cell_resolution-1;

	if(iy<0)iy=0;
	if(iy>sound_cell_resolution-1)iy=sound_cell_resolution-1;

	if(iz<0)iz=0;
	if(iz>sound_cell_resolution-1)iz=sound_cell_resolution-1;

	return (iz*sound_cell_resolution+iy)*sound_cell_resolution+ix;
}
glm::vec3 LiquidWorld::SoundCellToPos(int ix, int iy, int iz){
	return glm::vec3(
		(ix-sound_cell_resolution/2 + 0.5)*sound_cell_size,
		(iy-sound_cell_resolution/2 + 0.5)*sound_cell_size,
		-(iz+0.5)*sound_cell_size
	);
}


void LiquidWorld::playInteractionSounds(){
	if(!(time>=0)){
		std::cout<<"starting with negative or NAN time "<<time<<std::endl;
	}
	/// not very good
	static float old_time=time;
	float dt=glm::clamp(time-old_time, 0.0f, 1.0f);
	old_time=time;


	for(SoundCellParams &c : sound_cells){
		c.fresh_bubbles=0;
		c.old_bubbles=0;
		c.centroid_fresh=glm::vec3(0);
		c.centroid_old=glm::vec3(0);
	}
	if(time>0.1){/// first few frames glitch
		for(int iz=0; iz<voxel_resolution; iz+=sound_resolution_divisor){
			for(int iy=0; iy<voxel_resolution; iy+=sound_resolution_divisor){
				for(int ix=0; ix<voxel_resolution; ix+=sound_resolution_divisor){
					glm::vec4 v=particle_velocities_data[(iz*voxel_resolution+iy)*voxel_resolution+ix];
					if(v.w>=time-0.001 && v.w<=time+0.001){/// freshly created particle. find it's location and bin it
						vec3 start_pos=vec3(ix,iy,iz)*(1.0f/voxel_resolution);

						/// world position of the bubble
						vec3 pos=(from_voxels*vec4(glm::fract(start_pos+voxels_offset), 1.0)).xyz();

						SoundCellParams &cell=sound_cells[PosToSoundCell(pos)];

						//std::cout<<"fresh particle "<<time-v.w<<std::endl;

						cell.centroid_fresh+=pos;
						cell.fresh_bubbles++;
					}
				}
			}
		}
	}

    float sf=exp(-2.0f*dt);

	//for(SoundCellParams &c : sound_cells)
	float smooth_rise_param=5.0;
	float max_gain_decrease=0.5;
	float max_gain_increase = 3.0;
	for(int iz=0; iz<sound_cell_resolution; ++iz){
		for(int iy=0; iy<sound_cell_resolution; ++iy){
			for(int ix=0; ix<sound_cell_resolution; ++ix){
				SoundCellParams &c=sound_cells[(iz*sound_cell_resolution+iy)*sound_cell_resolution+ix];
				if(c.fresh_bubbles>0){
					c.centroid_fresh*=1.0f/c.fresh_bubbles;

					c.fresh_sound.pos=SoundCellToPos(ix,iy,iz);

					c.gain=glm::mix(c.fresh_bubbles*20.0f*interaction_sound_gain, c.gain, sf) * 1.0;
					//c.gain=glm::mix(c.fresh_bubbles*0.5f*interaction_sound_gain, c.gain, sf) * 1.0;

					float new_gain=SmoothRise(c.gain, smooth_rise_param);

					float gain_change=new_gain-c.fresh_sound.sound_gain;
					/*
					if(fabs(gain_change)>0.1){
						std::cout<<"large gain change: "<<gain_change<<std::endl;
					}*/

					c.fresh_sound.sound_gain+=glm::clamp(gain_change, -max_gain_decrease, max_gain_increase);
					//c.fresh_sound.sound_gain=10.0;

					//std::cout<<c.fresh_sound.sound_gain<<std::endl;
					if(!c.fresh_sound_active){
						c.fresh_sound_active=true;
						GlobAudioManager().AddSoundObject(&c.fresh_sound);
						//std::cout<<"activated sound"<<std::endl;
					}
				}else{
					//c.gain*=1.0-sf;
					c.gain*=sf;
					float new_gain=SmoothRise(c.gain, smooth_rise_param);

					float gain_change=new_gain-c.fresh_sound.sound_gain;
					c.fresh_sound.sound_gain+=glm::clamp(gain_change, -max_gain_decrease, max_gain_increase);
					//c.fresh_sound.sound_gain*=sf;
				}
			}
		}
	}
}

void LiquidWorld::beginExperience() {
	std::cout << "LiquidWorld::beginExperience()*****************************************************************************************************" << std::endl;
	resetVideoSphere();
	setupInteraction();
	OSCManager::getInstance().startWorld(COMMON::WORLD_TYPE::LIQUID);
	//#ifndef NO_ANNOYING_CRAP /// a define set in my builds - dmytry.
	//#endif
	time=0;
	liquid_disappear = 0;
	// Load audio cue sound
	playedVoicePrompt = false;
	reachedEnd = false;
	camera.setPosition(toOF(getCamPosAtTime(time)));
	camera.setOrientation(ofVec3f(cameraPitch, 0.0, 0.0));
	PlayBackgroundSound("worlds/liquid/sounds/ambient.wav", 1.0);
}

void LiquidWorld::endExperience() {
	
	playedVoicePrompt = false;
	reachedEnd = false;
	startedPlaying = false;
	/*
	if (videoSphere != NULL) {
		videoSphere->close();
	}
	delete videoSphere;
	videoSphere=NULL;*/

	std::cout<<"LiquidWorld::endExperience()*******************************"<<std::endl;
}


void LiquidWorld::update(float dt){
	//std::cout<<startedPlaying<< " duration: "<<duration<<" time"<<time<<std::endl;
	dt = glm::max(dt, 0.0f);
	time+=dt;

	/*
	if (engine->isSingleWorld() && time > WORLD_LIQUID::TOTAL_DURATION+30.0){/// loop the world in single world mode
		std::cout<<"LOOP THE WORLD IN SINGLE MODE"<<std::endl;
		reset();
		beginExperience();
	}
	*/

	if(!startedPlaying && time >= 61.5)
	{
		startedPlaying = true;
		loopVideoSphere->play();
		videoSphere->pause();
	}

	if(videoSphere && !startedPlaying){
		if(time>=video_start_time){
			if(!started_video){
				videoSphere->play();
				
				started_video=true;
				std::cout<<"Began playing outro video"<<std::endl;

				// Let the app stay at the last outro frame till we press start
				if (engine->isAutomaticTest()){
					std::cout<<"IS AUTOMATIC TEST!!!!!!!!!"<<std::endl;
					duration = WORLD_LIQUID::TOTAL_DURATION + 10.0;/// assume start is pressed after 10 seconds
				}
				else{
					duration = FLT_MAX;
				}
			}
			/*
			if(time>=WORLD_LIQUID::TOTAL_DURATION){
				videoSphere->pause();
			}else{
				videoSphere->update();
			}*/
			videoSphere->update();
		}
	}else if(loopVideoSphere)
	{
		loopVideoSphere->update();
	}

	camera.setPosition(toOF(getCamPosAtTime(time)));
	camera.setOrientation(ofVec3f(cameraPitch, 0.0, 0.0));
	// play audio cue only once and after the video has finished
	if (!playedVoicePrompt && time >= WORLD_LIQUID::DURATION_INTRO + COMMON::AUDIO_CUE_DELAY) {
		playedVoicePrompt = true;
		#ifndef NO_ANNOYING_CRAP
		PlayBackgroundSound(voicePromptFileName, 0.5f);
		#endif
	}

	// Play final voice prompt when reaching the end of the experience (minus 1 to have it during the fade out)
	if(!reachedEnd && time >= WORLD_LIQUID::TOTAL_DURATION - 1.9)
	{
		
		reachedEnd = true;
		//videoSphere->pause();
		
		#ifndef NO_ANNOYING_CRAP
		//if(!_AUTOMATED_TEST_)
		PlayBackgroundSound(voicePromptEndFileName, 0.5f);
		#endif
		OSCManager::getInstance().endWorld(COMMON::WORLD_TYPE::LIQUID);
	}
	//time = Timeline::getInstance().getElapsedCurrWorldTime();
	if(!(time>=0))std::cout<<"bad elapsed time "<<time<<std::endl;
	voxels_offset=voxels_offset_vel*time;

	liquid_disappear = glm::clamp((time - liquid_disappear_start) / (liquid_disappear_end - liquid_disappear_start) , 0.0f , 1.0f);
	//voxels_offset+=glm::vec3(0,0,dt*0.05);
}

glm::vec3 ofVec3fToGlmVec3 (ofVec3f vec) {
	return glm::vec3(vec.x, vec.y, vec.z);
}

void LiquidWorld::drawScene(renderType render_type, ofFbo *fbo){

	if (videoSphere && time >= video_start_time){/// draw video sphere first
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glUseProgram(0);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_FALSE);/// with no depth write
		glDisable(GL_DEPTH_TEST);
		/*
		glPushMatrix();
		glm::mat4x4 m;
		/// clear translation so the video sphere is centered on the camera
		glGetFloatv(GL_MODELVIEW_MATRIX, &m[0][0]);
		m[3] = vec4(0, 0, 0, 1);
		glLoadMatrixf(&m[0][0]);
		glColor3f(1, 1, 1);
		videoSphere->draw();
		glPopMatrix();*/
		if (videoSphere->sphere){
			videoSphere->sphere->setPosition(camera.getPosition());
		}

		if(loopVideoSphere->sphere)
			{
				loopVideoSphere->sphere->setPosition(camera.getPosition());
			}

		if(!startedPlaying)
			videoSphere->draw();
		else
			loopVideoSphere->draw();

		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	}

	/// do not draw the bulk of the world when at the end
	if(time < WORLD_LIQUID::TOTAL_DURATION){
		if(destruction_voxels){
		//glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
		//clear3DTexture(destruction_voxels);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		//drawParticlesInto3DTexture(destruction_voxels);
			if(interactiveAtTime(time)){
				if (engine->kinectEngine.isKinectDetected()) {
					drawDestruction(destruction_voxels, ofVec3fToGlmVec3(engine->kinectEngine.getHandsPosition()[0] + ofVec3f(0.0, -1.8, 0.3)) / world_scale, 0.4 / world_scale);
					drawDestruction(destruction_voxels, ofVec3fToGlmVec3(engine->kinectEngine.getHandsPosition()[1] + ofVec3f(0.0, -1.8, 0.3)) / world_scale, 0.4 / world_scale);
				} else {
					drawDestruction(destruction_voxels, glm::normalize(destruction_dir) * 0.5f / world_scale, 0.6f / world_scale);
				}
			}
		}

		ofEnableDepthTest();
		glDepthMask(GL_TRUE);

	/*
		point_sphere_shader().begin();
		glBegin(GL_POINTS);
		glVertex3f(0,0,-80);
		glVertex3f(0,10,-80);
		glVertex3f(1,2,-80);
		glVertex3f(3,1,-80);
		glEnd();
		point_sphere_shader().end();
		*/
			ofPushView();
			ofPushMatrix();
		ofPushStyle();

		glScalef(world_scale, world_scale, world_scale);

			drawParticlesOnScreen();

			playInteractionSounds();
		//---------------------------------------------------- Liquid Shader by dmytrylk

		ofSetColor(255);

		liquid_shader().begin();
		environment.getTextureReference().bind();
		liquid_shader().setUniform1f("time", time);
		ofVec3f p=camera.getPosition()*(1.0/world_scale);
		liquid_shader().setUniform3f("disappear_pos", p.x, p.y, p.z);
		liquid_shader().setUniform1f("disappear", liquid_disappear);

		/// Compute gl_ModelViewProjectionMatrixInverse ourselves to work around a possible buggy driver.
		ofMatrix4x4 proj;
		ofMatrix4x4 view;
		glGetFloatv(GL_PROJECTION_MATRIX,proj.getPtr());
		glGetFloatv(GL_MODELVIEW_MATRIX,view.getPtr());
		ofMatrix4x4 MVPMI=(view*proj).getInverse();
		liquid_shader().setUniformMatrix4f("MVPMI",MVPMI);

		SetTransformUniforms(liquid_shader());

		if(destruction_voxels){
			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_3D);
			destruction_voxels->BindTexture();
			liquid_shader().setUniform1i("destruction_voxels", 1);
			//glBindTexture(GL_TEXTURE_3D, t2);
		}

		if(particle_velocities){
			glActiveTexture(GL_TEXTURE2);
			glEnable(GL_TEXTURE_3D);
			particle_velocities->BindTexture();
			liquid_shader().setUniform1i("particle_velocities", 2);
			//glBindTexture(GL_TEXTURE_3D, t2);
		}

		CHECK_GL(glBindImageTextureEXT(0, fragment_lists_txid, 0, true, 0,  GL_READ_ONLY, GL_RGBA32F); )
		CHECK_GL(glBindImageTextureEXT(1, fragment_counter_txid, 0, false, 0,  GL_READ_ONLY, GL_R32UI); )

		liquid_shader().setUniform1i("fragment_lists", 0);
		liquid_shader().setUniform1i("fragment_counter", 1);
		liquid_shader().setUniform1i("max_fragment_count", fragment_buffer_layers);

		/// ensure that the operation is complete
		glMemoryBarrierEXT(GL_ALL_BARRIER_BITS_EXT);

		glBegin(GL_QUADS);
		glTexCoord2f(-1,-1);
		glVertex2f(-1,-1);

		glTexCoord2f(-1,1);
		glVertex2f(-1,1);

		glTexCoord2f(1,1);
		glVertex2f(1,1);

		glTexCoord2f(1,-1);
		glVertex2f(1,-1);
		glEnd();

		environment.getTextureReference().unbind();

		liquid_shader().end();
		ofPopStyle();
			ofPopMatrix();
		ofPopView();

		glActiveTexture(GL_TEXTURE2);
		glDisable(GL_TEXTURE_3D);
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_3D);
		glActiveTexture(GL_TEXTURE0);

		glEnable(GL_DEPTH_TEST);

	}/// if(time < WORLD_LIQUID::TOTAL_DURATION){


	if(!reachedEnd && time <= WORLD_LIQUID::TOTAL_DURATION - 1.0)
		drawBody();
	
	glBindTexture(GL_TEXTURE_2D, 0);
}

void LiquidWorld::drawBody()
{
	enum BodyState {LIQUID_BODY, TRANSITION, END_BODY};
	BodyState currState = LIQUID_BODY;

	float transitionTime = 5.0f;

	if(time >= video_start_time + transitionTime)
		currState = END_BODY;
	else if(time >= video_start_time)
		currState = TRANSITION;
	
	float opacity;
	switch(currState)
	{
	case LIQUID_BODY:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, environment.getTextureReference().getTextureData().textureID);
		engine->kinectEngine.drawBodyLiquid(camera.getPosition(), 1.0f);
		break;
	case TRANSITION:
		opacity = (time - video_start_time)/transitionTime; // 0 to 1
		engine->kinectEngine.drawBodyBlack(camera.getPosition());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, environment.getTextureReference().getTextureData().textureID);
		engine->kinectEngine.drawBodyLiquid(camera.getPosition(), 1.0f - opacity);
		engine->kinectEngine.drawBodyUltramarine(camera.getPosition(), opacity);
		break;
	case END_BODY:
		engine->kinectEngine.drawBodyUltramarine(camera.getPosition(), 1.0f);
		break;
	}
}

void LiquidWorld::drawTransitionObject(renderType render_type, ofFbo *fbo){
/*
	ofShader &shader=liquid_transition_shader();

	shader.begin();
    environment.getTextureReference().bind();

	shader.setUniform1f("time", time);
	ofVec3f p(0,0,0);
	shader.setUniform3f("disappear_pos", p.x, p.y, p.z);

	/// Compute gl_ModelViewProjectionMatrixInverse ourselves to work around a possible buggy driver.
	ofMatrix4x4 proj;
	ofMatrix4x4 view;
	glGetFloatv(GL_PROJECTION_MATRIX,proj.getPtr());
	glGetFloatv(GL_MODELVIEW_MATRIX,view.getPtr());
	ofMatrix4x4 MVPMI=(view*proj).getInverse();
	shader.setUniformMatrix4f("MVPMI",MVPMI);


    glBegin(GL_QUADS);
    glTexCoord2f(-1,-1);
    glVertex2f(-1,-1);

    glTexCoord2f(-1,1);
    glVertex2f(-1,1);

    glTexCoord2f(1,1);
    glVertex2f(1,1);

    glTexCoord2f(1,-1);
    glVertex2f(1,-1);
    glEnd();

    environment.getTextureReference().unbind();

    shader.end();

/*
	ofSetSphereResolution(5);

    ofSetColor(255,0,255,255);

	ofDrawSphere(0, 0, 0, 2.5);*/
}

float LiquidWorld::getDuration(){
	return duration;
}
void LiquidWorld::keyPressed(int key){
	World::keyPressed(key);
}
void LiquidWorld::keyReleased(int key){
}

void LiquidWorld::mouseMoved(int x, int y) {
	ofVec3f pos=camera.getPosition();
	ofVec3f dir=camera.screenToWorld(ofVec3f(x,y,-1.0))-pos;
	destruction_dir=glm::normalize(glm::vec3(dir.x, dir.y, dir.z));
}



void LiquidWorld::drawDebugOverlay(){
}


BloomParameters LiquidWorld::getBloomParams(){
	BloomParameters params;
	float a = glm::clamp((video_start_time - time) / 3.0f, 0.0f, 1.0f);
	params.bloom_multiplier_color *= a;

	return params;
}