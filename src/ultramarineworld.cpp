#ifdef NO_ANNOYING_CRAP /// a define set in my builds - dmytry.
//#define TESTING_ULTRAMARINE /// disables wait on start
#endif

//#define DO_CROSSFADE_BLOOM_PARAMS
#define USE_VIDEO_RESET

#define GLM_SWIZZLE
#define GLM_FORCE_RADIANS

#include "glm/glm.hpp"
#include "glm/detail/func_noise.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/noise.hpp"

#include "audio.h"
#include "ultramarineworld.h"

#include "resourcemanager.h"
#include "utils.h"

const char *sound_breakdown_crackle_filename = "worlds/ultramarine/sounds/Breakdown_Long_16.wav";
float sound_breakdown_crackle_start_time_offset = -1.0;
const char *sound_intro_filename = "worlds/ultramarine/sounds/Intro_New.ogg";
const float AMBIENCE_GAIN = 1.0;

/// gain values for different sounds
const float crystal_growth_slow_gain=0.8;
const float crystal_growth_fast_gain=1.0;
const float ice_cracking_gain = 1.0;
const float breakdown_gain = 1.0;
const float ice_crack_sound_probability = 0.3;


const glm::vec3 fog_color=glm::vec3(0.02,0.051,0.052)*0.3f;

const double pi=3.1415926535897930;

const float grow_speed_factor=6.0;
const float max_crystal_distance=30.0; /// for adding crystals interactively
const float min_crystal_distance=2.0;
const float growth_closing_in_nonlinearity=0.5; /// 0.0: linear, 1.0: quadratic

const float wait_time_before_adding_crystal=0.5; /// time that a crystal can be added after adding one
const float frost_crystal_grow_speed=0.2;
const float frost_crystal_grow_max_size=0.2;
const float crystal_throwing_speed = 30.0;
const int crystal_throw_count=0;

const float fall_start_time = WORLD_ULTRAMARINE::DURATION_MAIN - 7;

const float growth_closing_in_time=fall_start_time-5.0;

float bloom_crossfade_duration=3.0;

float DEFAULT_BLOOM_MULTIPLIER_COLOR = 0.1f;
float BLOOM_MULTIPLIER_COLOR = DEFAULT_BLOOM_MULTIPLIER_COLOR;
float DEFAULT_GAMMA = 1.0f;
float GAMMA = DEFAULT_GAMMA;
float DEFAULT_EXPOSURE = 1.0f;
float EXPOSURE = DEFAULT_EXPOSURE;
float DEFAULT_MULTISAMPLE_ANTIALIASING = 16.0f;
float MULTISAMPLE_ANTIALIASING = DEFAULT_MULTISAMPLE_ANTIALIASING;
float DEFAULT_ANTIALIASING = 2.0f;
float ANTIALIASING = DEFAULT_ANTIALIASING;
bool DEFAULT_CRISP_ANTIALIASING = false;
bool CRISP_ANTIALIASING = DEFAULT_CRISP_ANTIALIASING;
bool DEFAULT_USE_ANTIALIASING = false;
bool USE_ANTIALIASING = DEFAULT_USE_ANTIALIASING;

const float non_interact_grow_speed=1.5f;
const float baseline_growth_speed=1.5f;/// the growth speed when not interacting with a crystal
const float non_interact_timeout=10.0f; /// after that many seconds of non interaction stuff will grow automagically at high speed

//const float interaction_angle=10.0*::pi/180.0;/// when pointing within this angle from target, we interact with the target
const float interaction_radius=6.0;
const float min_distance_between_crystals = 3.0;
const float min_distance_from_growing_crystal = 6.0;

const float hands_extension_limit = 0.35;/// works as in introworld
const float hands_extension_time = 0.01; /// for how long to point before it has an effect
const float min_gesture_direction_change = 0.4;/// roughly radians, how much the hand has to move before it is a new gesture. 1 radian ~= 57 degrees.

const float pointing_steady_time_interval = 0.2;
const float default_pointing_steady_threshold = 0.1;
float pointing_steady_threshold = default_pointing_steady_threshold;

const float initial_breaking_stress=1.7;
const float after_jolt_breaking_stress=3.0;
const float jolt_duration=0.3;
const float jolt_angle_low=2.0f*::pi/180.0f;
const float jolt_angle_high=7.0f*::pi/180.0f;

const float fall_start_time_distance_scale=0.03;

const float fall_acceleration=-9.8; /// meters per second squared
const float fall_rotation=1.0;

/*
const float fall_crystal_spread=10.0;
const float crystal_fall_delay=6.0;
const float fall_crystal_rotation=0.2;
const float fall_crystal_rotation_2=9.0;
const float hover_velocity_decay=1.0;*/
const float fall_crystal_spread=1.0;
const float crystal_fall_delay=0.01;
const float fall_crystal_rotation=0.2;
const float fall_crystal_rotation_2=9.0;
const float hover_velocity_decay=1.0;

const int reflection_resolution_divisor=1;/// divikides resolution for floor reflection

const int snowflakes_count=1000;
const float snowflakes_visibility=12.0;/// visibility distance in meters
const float wind_speed=3.0;
const float snowflakes_fade_time=1.0;

glm::vec3 simplex3(glm::vec3 v){
	return glm::vec3(glm::simplex(v), glm::simplex(v+glm::vec3(200,0,0)), glm::simplex(v+glm::vec3(0,200,0)));
}

glm::vec3 perlin3(glm::vec3 v){
	return glm::vec3(glm::perlin(v), glm::perlin(v+glm::vec3(10.2345623,0,0)), glm::perlin(v+glm::vec3(0,13.234563456,0)));
}

vec3 randomDirection(){
	vec3 result;
	result.z=ofRandom(-1,1);
	float a=ofRandom(2*3.1415926);
	float r=::sqrt(1.0-result.z*result.z);
	result.x=r*sin(a);
	result.y=r*cos(a);
	return result;
}

vec2 randomPointInCircle(){
	float a=ofRandom(2*::pi);
	float r=glm::sqrt(ofRandom(1));
	return vec2(r*sin(a),r*cos(a));
}

vec2 randomPointInCircle(float min_radius, float max_radius){
	float a=ofRandom(2*::pi);
	float r=glm::sqrt(ofRandom(min_radius*min_radius, max_radius*max_radius));
	return vec2(r*sin(a),r*cos(a));
}


using namespace glm;
//using namespace glm::gtc::matrix_transform;

static ShaderResource crystal_shader("worlds/ultramarine/crystal.vert", "worlds/ultramarine/crystal.frag");
static ShaderResource floor_shader("worlds/ultramarine/floor.vert", "worlds/ultramarine/floor.frag", "worlds/ultramarine/floor.geom");

static ShaderResource background_shader("worlds/ultramarine/background.vert", "worlds/ultramarine/background.frag");

static ShaderResource particle_beam_shader("worlds/ultramarine/particle_beam_v.glsl", "worlds/ultramarine/particle_beam_f.glsl");


const int Sound_IceBreak_Filenames_count=15;
const char* Sound_IceBreak_Filenames[Sound_IceBreak_Filenames_count]={
"Ice_Break_Small_5.wav",
"Ice_Break_Small_4.wav",
"Ice_Break_Small_3.wav",
"Ice_Break_Small_2.wav",
"Ice_Break_Small_1.wav",
"Ice_Break_Med_5.wav",
"Ice_Break_Med_4.wav",
"Ice_Break_Med_3.wav",
"Ice_Break_Med_2.wav",
"Ice_Break_Med_1.wav",
"Ice_Break_Big_5.wav",
"Ice_Break_Big_4.wav",
"Ice_Break_Big_3.wav",
"Ice_Break_Big_2.wav",
"Ice_Break_Big_1.wav"
};

const std::string Sound_Icebreak(int size){/// size = 0..2
	int n=size*5+ofRandom(5);
	if(n<0)n=0;
	if(n>Sound_IceBreak_Filenames_count-1)n=Sound_IceBreak_Filenames_count-1;
	return std::string("worlds/ultramarine/sounds/")+Sound_IceBreak_Filenames[n];
}

const int Sound_CrystalGrow_Filenames_count=10;
const char* Sound_CrystalGrow_Filenames[Sound_IceBreak_Filenames_count]={
"Crystal_Growth_Slow5.ogg",
"Crystal_Growth_Slow4.ogg",
"Crystal_Growth_Slow3.ogg",
"Crystal_Growth_Slow2.ogg",
"Crystal_Growth_Slow1.ogg",
"Crystal_Growth_Fast5.ogg",
"Crystal_Growth_Fast4.ogg",
"Crystal_Growth_Fast3.ogg",
"Crystal_Growth_Fast2.ogg",
"Crystal_Growth_Fast1.ogg"
};

const std::string Sound_CrystalGrow(int speed){/// speed = 0..1
	int n=speed*5+ofRandom(5);
	if(n<0)n=0;
	if(n>Sound_CrystalGrow_Filenames_count-1)n=Sound_CrystalGrow_Filenames_count-1;
	return std::string("worlds/ultramarine/sounds/")+Sound_CrystalGrow_Filenames[n];
}

void MakePyramid(std::vector<glm::vec3> &vertices, float r, float h, int n){/// n-faced pyramid with no base
	for(int i=0;i<n;++i){
		float a0=(2.0*::pi*i)/n;
		float a1=(2.0*::pi*(i+1))/n;
		if(h>0){
			vertices.push_back(glm::vec3(0,0,h));
			vertices.push_back(glm::vec3(r*sin(a0),r*cos(a0),0));
			vertices.push_back(glm::vec3(r*sin(a1),r*cos(a1),0));
		}else{
			vertices.push_back(glm::vec3(r*sin(a1),r*cos(a1),0));
			vertices.push_back(glm::vec3(r*sin(a0),r*cos(a0),0));
			vertices.push_back(glm::vec3(0,0,h));
		}
	}
}

void MakePyramidCappedPrism(std::vector<glm::vec3> &vertices, float r, float h1, float h2, int n){
	for(int i=0;i<n;++i){
		float a0=(2.0*::pi*i)/n;
		float a1=(2.0*::pi*(i+1))/n;

		vertices.push_back(glm::vec3(0,0,h2));
		vertices.push_back(glm::vec3(r*sin(a0),r*cos(a0),h1));
		vertices.push_back(glm::vec3(r*sin(a1),r*cos(a1),h1));

		vertices.push_back(glm::vec3(r*sin(a1),r*cos(a1),h1));
		vertices.push_back(glm::vec3(r*sin(a0),r*cos(a0),h1));
		vertices.push_back(glm::vec3(r*sin(a0),r*cos(a0),0));

		vertices.push_back(glm::vec3(r*sin(a0),r*cos(a0),0));
		vertices.push_back(glm::vec3(r*sin(a1),r*cos(a1),0));
		vertices.push_back(glm::vec3(r*sin(a1),r*cos(a1),h1));
	}
}

void trianglesToMesh(ofMesh &mesh, const std::vector<glm::vec3> &vertices){
	mesh.disableColors();
	mesh.disableIndices();
	mesh.disableTextures();
	mesh.enableNormals();
	mesh.setMode(OF_PRIMITIVE_TRIANGLES);
	for(int i=0; i*3+2<vertices.size(); ++i){
		vec3 normal=normalize(cross(vertices[i*3+1]-vertices[i*3], vertices[i*3+2]-vertices[i*3]));
		for(int j=0;j<3;++j){
			mesh.addVertex(toOF(vertices[i*3+j]));
			mesh.addNormal(toOF(normal));
		}
	}
}

class CrystalsSharedData{
public:
	GLuint buffer_id;

	struct Vertex{
		glm::vec3 pos;
		glm::vec3 normal;
	};

	std::vector<Vertex> data;

	static CrystalsSharedData &Instance(){
		static CrystalsSharedData *instance_ptr=new CrystalsSharedData();
		return *instance_ptr;
	}

	CrystalsSharedData(){
		//MakePyramid(crystal_shape, 0.1, 1.0, 6);
		/*
		MakePyramidCappedPrism(crystal_shape, 0.1, 0.8, 1.0, 6);
		for(int i=0;i<30;++i){
			growth_start_positions.push_back(vec3(ofRandom(-20,20),0,ofRandom(0,-20)));
		}
		*/
		//crystal_meshes[0]=ofMesh::cylinder(0.2, 1.0);
		std::vector<vec3> vertices;
		//MakePyramidCappedPrism(shape, 0.1, 0.8, 1.0, 6);
		//MakePyramidCappedPrism(shape, 0.1, -0.8, -1.0, 6);
		MakePyramid(vertices, 0.1, 1.0, 6);
		MakePyramid(vertices, 0.1, -1.0, 6);
		/*
		trianglesToMesh(crystal_meshes[0], shape);*/
		glGenBuffers(1, &buffer_id);

		for(int i=0; i*3+2<vertices.size(); ++i){
			vec3 normal=-normalize(cross(vertices[i*3+1]-vertices[i*3], vertices[i*3+2]-vertices[i*3]));
			for(int j=0;j<3;++j){
				Vertex vertex;
				vertex.pos=vertices[i*3+j];
				vertex.normal=normal;
				data.push_back(vertex);
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
		glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(data[0]), (void*)&data[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	~CrystalsSharedData(){
		glDeleteBuffers(1, &buffer_id);
	}

	void begin(){
		glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
		crystal_shader().begin();
		crystal_shader().setUniform3f("fog_color", fog_color.r, fog_color.g, fog_color.b);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_INDEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(data[0]), gl_offsetof(CrystalsSharedData::Vertex, pos));
		glNormalPointer(GL_FLOAT, sizeof(data[0]), gl_offsetof(CrystalsSharedData::Vertex, normal));
	}
	void draw(){
		glDrawArrays(GL_TRIANGLES, 0, data.size());
	}
	void end(){
		crystal_shader().end();
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};

const int max_crystals=10000;

Crystals::Crystals():is_frost(false), scale(1.0), interaction_delay(0.0), growth(0), max_growth(10.0), grow_speed(0), spiral_phase(0), buffer_id(0), crystals_amount(20), pos(0,0,0), radius_scale(1), speed_scale(1),
spiral_scale(1), spiral2_base(0,1), spiral2_radius(2.0), spiral2_rise(0.3),
fall_time(100.0),
has_sound(false),
growth_sound(Sound_CrystalGrow(0), true, false, pos, vec3(0.0), 1E10, 0.0, 1.0),///
growth_sound_active(false),
fast_growth_sound(Sound_CrystalGrow(1), true, false, pos, vec3(0.0), 1E10, 0.0, 1.0),
fast_growth_sound_active(false)
{
}

void Crystals::setup(){
	CrystalsSharedData::Instance();
	spiral_phase = ofRandom(2 * ::pi);
	if (!buffer_id)glGenBuffers(1, &buffer_id);

	growth_sound.pos = pos;
	fast_growth_sound.pos = pos;

	if (is_frost){
	}
	else{

		//if(dt<1E-4)return;
		int n_crystals = 30;// initial crystals in a large circle around
		crystals_amount += n_crystals;
		while (crystals.size() < n_crystals){
			Crystal c;

			float a = ofRandom(2 * ::pi);
			float r = glm::sqrt(ofRandom(1));
			vec2 rp(r*sin(a), r*cos(a));

			c.start_pos = vec3(rp.x*1.8*radius_scale, 0.1, rp.y*1.8*radius_scale);
			c.track_speed = 0.15;

			float rl = 0.15 + ofRandom(0.4);
			float rw = rl*(0.5 + ofRandom(4.0));
			c.scale = vec3(rw, rw, rl);


			vec3 axis = randomDirection();
			float angle = ofRandom(2.0*::pi);
			c.orientation = glm::quat(cos(angle), axis.x*sin(angle), axis.y*sin(angle), axis.z*sin(angle));

			crystals.push_back(c);
			//std::cout<<"creating crystal"<<std::endl;
		}
	}
}

void Crystals::reset(){
	growth=0;
	grow_speed=0;
	if(growth_sound_active){
		GlobAudioManager().RemoveSoundObject(&growth_sound);
		growth_sound_active=false;
	}
	if(fast_growth_sound_active){
		GlobAudioManager().RemoveSoundObject(&fast_growth_sound);
		fast_growth_sound_active=false;
	}
	crystals.clear();
	crystal_draw_params.clear();
	crystals_amount=20.0;
	setup();
}

int total_crystals=0;

void Crystals::draw(UltramarineWorld &world, float time){

	if(crystals.empty())return;
	if(crystal_draw_params.empty())return;

	glColor3f(0,1,1);
	glGetError();
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glScalef(scale, scale, scale);

	CrystalsSharedData::Instance().begin();
	if(world.environment.ok()){
		world.environment->BindTexture();
	}

	CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, buffer_id);)
	int err=glGetError();
		if(err!=0){
			std::cout<<"gl error "<<err<<std::endl;
		}
	CHECK_GL(glBufferData(GL_ARRAY_BUFFER, crystal_draw_params.size()*sizeof(crystal_draw_params[0]), (void*)&crystal_draw_params[0], GL_DYNAMIC_DRAW);)
	//std::cout<<"draw params size: "<<crystal_draw_params.size()<<std::endl;

	CHECK_GL(GLint aloc=crystal_shader().getAttributeLocation("transform");)
	if(aloc!=-1){
		for(int i=0;i<4;++i){
			CHECK_GL(glEnableVertexAttribArray(aloc+i);)
			glVertexAttribPointer(aloc+i, 4, GL_FLOAT, 0, sizeof(crystal_draw_params[0]),(void *)(offsetof(Crystals::CrystalDrawParams, transform)+i*4*sizeof(float)));
			CHECK_GL(glVertexAttribDivisor(aloc+i, 1);)
		}
	}
	CHECK_GL(GLint aloc2=crystal_shader().getAttributeLocation("color");)
	if(aloc2!=-1){
		CHECK_GL(glEnableVertexAttribArray(aloc2);)
		glVertexAttribPointer(aloc2, 4, GL_FLOAT, 0, sizeof(crystal_draw_params[0]), (void *)(offsetof(Crystals::CrystalDrawParams, color)));
		CHECK_GL(glVertexAttribDivisor(aloc2, 1);)
	}

	total_crystals+=crystals.size();
	//std::cout<<"about to glDrawArraysInstanced"<<std::endl;
	CHECK_GL(glDrawArraysInstanced(GL_TRIANGLES, 0, CrystalsSharedData::Instance().data.size(), crystal_draw_params.size());)
	//std::cout<<"done with glDrawArraysInstanced"<<std::endl;
	if(aloc!=-1){
		for (int i = 0; i < 4; ++i){
			glDisableVertexAttribArray(aloc + i);
			glVertexAttribDivisor(aloc + i, 0);
		}
	}
	if(aloc2!=-1){
		glDisableVertexAttribArray(aloc2);
		glVertexAttribDivisor(aloc2, 0);
	}

	//world.environment.getTextureReference().unbind();
	glBindTexture(GL_TEXTURE_2D, 0);
	CrystalsSharedData::Instance().end();
	glPopMatrix();
}

fquat turnFromTo(vec3 a, vec3 b){
	a=normalize(a);
	b=normalize(b);
	vec3 c=normalize(a+b);
	vec3 axis=glm::cross(a,c);
	return fquat(glm::dot(a,c) , axis.x, axis.y, axis.z);
}

glm::vec3 Crystals::pathPos(float t){
	//t*=0.4;
	//glm::vec3 s1=0.2f*glm::vec3(sin(t*spiral_scale+spiral_phase), t ,cos(t*spiral_scale+spiral_phase));
	glm::vec2 s1=0.2f*glm::vec2(sin(t*spiral_scale+spiral_phase), cos(t*spiral_scale+spiral_phase));

	float t2=0.2*t/(spiral2_radius*(1.0f+spiral2_rise));
	glm::vec2 s2=spiral2_radius*glm::vec2(1.0-cos(t2), sin(t2)+spiral2_rise*t2);/// dy/dt of this starts at 1.0

	glm::vec3 s3(s1.x*cos(t2)+s2.x, s2.y-s1.x*sin(t2), s1.y);

	/// rotate by spiral2_base
	return glm::vec3(s3.x*spiral2_base.x - s3.z*spiral2_base.y, s3.y, s3.x*spiral2_base.y + s3.z*spiral2_base.x);
}

const float crystals_per_second=12;

glm::mat4x4 scale_matrix(glm::vec3 scale){
	return mat4x4(
					scale.x,0,0,0,
					0,scale.y,0,0,
					0,0,scale.z,0,
					0,0,0,1
					);
}



void Crystals::grow(float time, float raw_dt, float desired_grow_speed){

	if(raw_dt<1E-5)return; /// ignore too small timed updates


	desired_grow_speed*=glm::clamp(max_growth-growth, 0.0f, 1.0f);

	if(grow_speed>desired_grow_speed){
		grow_speed=glm::mix(grow_speed, desired_grow_speed, raw_dt*1.5f);
	}else{
		grow_speed=desired_grow_speed;
	}


	if(has_sound && time<fall_time){
		if(desired_grow_speed>0.1 && !growth_sound_active){
			growth_sound.playback_start=0;
			GlobAudioManager().AddSoundObject(&growth_sound);
			growth_sound_active=true;
		}
		if(grow_speed>2.0 && !fast_growth_sound_active){
			fast_growth_sound.playback_start=0;
			GlobAudioManager().AddSoundObject(&fast_growth_sound);
			fast_growth_sound_active=true;
		}
		growth_sound.sound_gain=crystal_growth_slow_gain*clamp(grow_speed, 0.0f, 1.0f);
		fast_growth_sound.sound_gain=crystal_growth_fast_gain*clamp(grow_speed/4.0f, 0.0f, 1.0f);
	}
	if(time>fall_time+0.1){
		if(growth_sound_active){
				GlobAudioManager().RemoveSoundObject(&growth_sound);
				growth_sound_active=false;
		}
		if(fast_growth_sound_active){
				GlobAudioManager().RemoveSoundObject(&fast_growth_sound);
				fast_growth_sound_active=false;
		}
	}

	float dt=raw_dt*grow_speed;
	growth+=dt;

	if(!is_frost){
		if(time<fall_time){
		crystals_amount+=dt*crystals_per_second;
		}

		int n_crystals=crystals_amount;
		//if(dt<1E-4)return;
		while(crystals.size()<n_crystals){
			Crystal c;

			float a=ofRandom(2*::pi);
			float r=glm::sqrt(ofRandom(1));
			vec2 rp(r*sin(a), r*cos(a));

			c.start_pos=vec3(rp.x*0.5, 0, rp.y*0.5)*radius_scale;
			c.track_speed=(1.5-r*r*r)*speed_scale;

			float rl=0.15+ofRandom(0.3)+growth*0.02;
			float rw=rl*(0.3+ofRandom(3.0*smoothstep(0.0f,1.0f,growth*1.0f)));
			c.scale=vec3(rw,rw,rl);

			if(ofRandom(1.0)<0.06 && growth>7.0){
				c.scale.x*=3.0;
				c.scale.y*=3.0;
				c.scale.z*=1.5;
			}

			vec3 p0=pathPos(c.track_pos);
			vec3 p1=pathPos(c.track_pos+0.01);
			c.orientation=turnFromTo(vec3(0,0,1)+randomDirection()*growth, p1-p0);

			crystals.push_back(c);
			//std::cout<<"creating crystal"<<std::endl;
		}
	}

	crystal_draw_params.resize(crystals.size());
	for(int i=0; i<crystals.size(); ++i){
		Crystal &c=crystals[i];

		if(c.falling){
			c.pos+=raw_dt*c.velocity;
			if(time>fall_time-c.track_pos*0.01){
				if(c.falling==1){
					c.falling=2;
					c.angular_velocity=randomDirection()*fall_crystal_rotation_2;
				}
				c.velocity.y+=fall_acceleration*raw_dt;
			}else{
				c.velocity*=exp(-raw_dt*hover_velocity_decay);
			}
			float avl=length(c.angular_velocity);
			if(avl>1E-10){
				c.orientation=normalize(glm::angleAxis(raw_dt*avl, c.angular_velocity*(1.0f/avl))*c.orientation);
			}
		}else{
			if((!is_frost && time>fall_time-crystal_fall_delay-c.track_pos*0.01) || (is_frost && time>fall_time+length(c.pos)*fall_start_time_distance_scale)){
				c.falling=1;
				c.angular_velocity=randomDirection()*fall_crystal_rotation;
				c.velocity=randomDirection()*fall_crystal_spread;
				c.velocity.y=fabs(c.velocity.y);
			}
			if(is_frost){
				float l=length(c.scale);
				if(l<frost_crystal_grow_max_size){
					c.scale*=1.0f+raw_dt*frost_crystal_grow_speed/l;
				}
			}else{
				if(dt<1E-5)continue;
				vec3 p0_old=pathPos(c.track_pos);
				vec3 p1_old=pathPos(c.track_pos+0.01);
				vec3 dir_old=normalize(p1_old-p0_old);

				c.track_pos+=c.track_speed*dt;

				vec3 p0=pathPos(c.track_pos);
				vec3 p1=pathPos(c.track_pos+0.01);
				vec3 dir=normalize(p1-p0);

				glm::quat ori=turnFromTo(dir_old, dir);
				c.orientation=normalize(ori*c.orientation);
				c.pos=p0+c.start_pos;
			}

		}

		crystal_draw_params[i].transform=mat4_cast(c.orientation)*
					mat4x4(
					c.scale.x,0,0,0,
					0,c.scale.y,0,0,
					0,0,c.scale.z,0,
					0,0,0,1
					);
		crystal_draw_params[i].transform[3]=vec4(c.pos,1.0);
		crystal_draw_params[i].color=c.color;
	}
}
Crystals::~Crystals(){
	if(buffer_id)glDeleteBuffers(1, &buffer_id);
}

void Crystals::addFrost(glm::vec3 pos){
	Crystal c;
	c.pos=pos;
	c.start_pos=pos;
	c.track_pos=0;
	c.track_speed=0.0001;
	float rl=ofRandom(0.15, 0.45);
	float rw=rl*ofRandom(0.3, 3.0);
	c.scale=0.01f*vec3(rw,rw,rl);
	c.orientation=glm::angleAxis(ofRandom(0.0,2.0*::pi),randomDirection());
	c.color=vec4(vec3(0.6, 0.85, 1)*ofRandom(0.05,0.1), 1.0);
	crystals.push_back(c);
}

void Crystals::addProjectile(glm::vec3 pos, glm::vec3 vel, glm::vec3 avel, float scale){
	Crystal c;
	c.falling = 2;
	c.pos = pos;
	c.start_pos = pos;
	c.track_pos = 0;
	c.track_speed = 0.0001;
	c.velocity = vel;
	c.angular_velocity = avel;
	float rl = ofRandom(0.15, 0.45);
	float rw = rl*ofRandom(0.3, 3.0);
	c.scale = scale*vec3(rw, rw, rl);
	c.orientation = glm::angleAxis(ofRandom(0.0, 2.0*::pi), randomDirection());
	c.color = vec4(0.0);//vec4(vec3(0.6, 0.85, 1)*ofRandom(0.05, 0.1), 1.0);
	crystals.push_back(c);
}


ParticleBeam::ParticleBeam():particles_count(5000),
buffer_id(0), feedback_buffer_id(0), animate_program_id(0){

	beam_power[0]=beam_power[1]=1.0;
	beam_start[0]=beam_start[1]=vec3(0.3,1.8,0);
	beam_dir[0]=beam_dir[1]=vec3(0,0,-1);

	beam_start[1].x=-beam_start[1].x;
}
ParticleBeam::~ParticleBeam(){
	if(buffer_id)glDeleteBuffers(1, &buffer_id);
	if(feedback_buffer_id)glDeleteBuffers(1, &feedback_buffer_id);
	if(animate_program_id)glDeleteProgram(animate_program_id);
}

void CheckLinkStatus(GLuint program_id){
	GLint linked;
	glGetProgramiv(program_id, GL_LINK_STATUS, &linked);

	GLint blen = 0;
	GLsizei slen = 0;
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH , &blen);
	if (blen > 1)
	{
		//GLchar* compiler_log = (GLchar*)malloc(blen);
		std::string compiler_log(blen, ' ');
		glGetShaderInfoLog(program_id, blen, &slen, &compiler_log[0]);
		compiler_log.resize(slen);
		std::cout <<"Program linking failed, log:\n"<< compiler_log << std::endl;
		#ifdef EXIT_ON_SHADER_WARNING
		assert(0);
		#endif
	}
}
void ParticleBeam::setup(){
	std::cout<<"ParticleBeam::setup()"<<std::endl;
	glGetError();
	CHECK_GL(glGenBuffers(1, &buffer_id);)
	CHECK_GL(glGenBuffers(1, &feedback_buffer_id);)
	std::vector<Particle> particles;
	particles.resize(particles_count);
	for(auto &p : particles){
		p.pos=glm::vec4(ofRandom(-10,10), ofRandom(-10,10), ofRandom(0.5, 1.0), ofRandom(0.0,5.0));
		p.vel=glm::vec4(ofRandom(-1,1), ofRandom(-1,1), 0, ofRandom(0.5,1.5));
	}
	CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, buffer_id);)
	CHECK_GL(glBufferData(GL_ARRAY_BUFFER, particles.size()*sizeof(particles[0]), (void*)&particles[0], GL_STREAM_DRAW);)
	CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, 0);)
	CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, feedback_buffer_id);)
	CHECK_GL(glBufferData(GL_ARRAY_BUFFER, particles.size()*sizeof(particles[0]), NULL, GL_STREAM_DRAW);)
	CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, 0);)


	///stolen from evolutionworld
	GLuint animateVertexShader = glCreateShader(GL_VERTEX_SHADER);
	string source = ofBufferFromFile("worlds/ultramarine/animate_beam.vert").getText();
	const char * sourcePointer = source.c_str();
    glShaderSource(animateVertexShader, 1, &sourcePointer, nullptr);
    glCompileShader(animateVertexShader);

	GLint isCompiled = 0;
	glGetShaderiv(animateVertexShader, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE) {
		GLint infoLength = 0;
		glGetShaderiv(animateVertexShader, GL_INFO_LOG_LENGTH, &infoLength);

		//The maxLength includes the NULL character
		GLchar* infoBuffer = new GLchar[infoLength];
        glGetShaderInfoLog(animateVertexShader, infoLength, &infoLength, infoBuffer);
		std::cout << infoBuffer << std::endl;
		delete [] infoBuffer;
	}

	animate_program_id = glCreateProgram();
	glAttachShader(animate_program_id, animateVertexShader);

	const char* outputs[4] = {"out_pos", "out_vel"}; //i think it's necessary to pass constant attributes through transform feedback...
	glTransformFeedbackVaryings(animate_program_id, 2, outputs, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(animate_program_id);
	CheckLinkStatus(animate_program_id);

}

void ParticleBeam::update(float dt){
	//std::cout<<"ParticleBeam::update "<<std::endl;
	glUseProgram(animate_program_id);

	glUniform1f(glGetUniformLocation(animate_program_id, "dt"), dt);

	glUniform1f(glGetUniformLocation(animate_program_id, "beam1_power"), beam_power[0]);
	glUniform1f(glGetUniformLocation(animate_program_id, "beam2_power"), beam_power[1]);

	glUniform3fv(glGetUniformLocation(animate_program_id, "beam1_start"), 1, (float *)&beam_start[0]);
	glUniform3fv(glGetUniformLocation(animate_program_id, "beam2_start"), 1, (float *)&beam_start[1]);

	glUniform3fv(glGetUniformLocation(animate_program_id, "beam1_dir"), 1, (float *)&beam_dir[0]);
	glUniform3fv(glGetUniformLocation(animate_program_id, "beam2_dir"), 1, (float *)&beam_dir[1]);


	GLuint pos_loc=glGetAttribLocation(animate_program_id, "in_pos");
	GLuint vel_loc=glGetAttribLocation(animate_program_id, "in_vel");
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnableVertexAttribArray(pos_loc);
	glEnableVertexAttribArray(vel_loc);
	//std::cout<<"about to glBindBuffer"<<std::endl;
	glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
	glVertexAttribPointer(pos_loc, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), gl_offsetof(ParticleBeam::Particle, pos));
	glVertexAttribPointer(vel_loc, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), gl_offsetof(ParticleBeam::Particle, vel));
	//std::cout<<"about to glBindBufferBase"<<std::endl;
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedback_buffer_id);
	glEnable(GL_RASTERIZER_DISCARD);
	//std::cout<<"about to glBeginTransformFeedback"<<std::endl;
	glBeginTransformFeedback(GL_POINTS);
	//std::cout<<"about to draw arrays"<<std::endl;
	glDrawArrays(GL_POINTS, 0, particles_count);
	//std::cout<<"done drawing arrays"<<std::endl;
	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);
	glDisableVertexAttribArray(pos_loc);
	glDisableVertexAttribArray(vel_loc);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	std::swap(feedback_buffer_id, buffer_id);
}

void ParticleBeam::draw(float time){
	//my_logger<<std::endl;
	glDisable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	//glDisable(GL_BLEND);
	//glDepthMask(GL_FALSE);
	glGetError();
	CHECK_GL(particle_beam_shader().begin();)
	//glBegin(GL_POINTS);
	//glVertex3f(0,2,-10);
	//glEnd();

	CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, buffer_id);)
	CHECK_GL(glEnableClientState(GL_VERTEX_ARRAY);)
	CHECK_GL(glDisableClientState(GL_NORMAL_ARRAY);)
	CHECK_GL(glDisableClientState(GL_COLOR_ARRAY);)
	CHECK_GL(glDisableClientState(GL_INDEX_ARRAY);)
	CHECK_GL(glVertexPointer(4, GL_FLOAT, sizeof(ParticleBeam::Particle), gl_offsetof(ParticleBeam::Particle, pos));)
	CHECK_GL(glDrawArrays(GL_POINTS, 0, particles_count);)
	CHECK_GL(particle_beam_shader().end();)
	glDisable(GL_BLEND);
	CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, 0);)
	//glDepthMask(GL_TRUE);
}

float line_dist(vec3 pos, vec3 start, vec3 dir){
	vec3 delta=pos-start;
	return length(delta-dot(delta,dir)*dir);
}

glm::vec4 ParticleBeam::velocityField(glm::vec3 pos){
	vec3 f;
	float max_interact=0;
	for(int i=0;i<2;++i){
		vec3 beam_closest_point=beam_start[i]+beam_dir[i]*dot(beam_dir[i], pos-beam_start[i]);
		float beam_dist=length(beam_closest_point - pos);
		float bv=exp(-2.0*beam_dist);
		max_interact=glm::max(max_interact, bv);
		f+=bv*(beam_power[i]+0.5f)*beam_dir[i];
		/// convergence field
		/*
		float bdf=glm::max(beam_dist-0.5f, 0.0f);
		f+=normalize(beam_closest_point-pos)*bdf*expf(-0.2*bdf)*0.2f*beam_power[i];
		*/
	}
	return vec4(f, max_interact);
}


DEFINE_WORLD(UltramarineWorld) /// this adds the world to the global world list so it can be created by name

UltramarineWorld::UltramarineWorld(Engine *engine):World(engine), old_time(0), time(0), videoSphere(NULL), draw_ground(true),
do_updates(true), non_interaction_time(0), played_intro_sound(false), played_crash_sound(false), ambientSound("worlds/ultramarine/sounds/Ambience_Bass.ogg", true, true)
{
	hand_interaction_growth_speed[0]=0;
	hand_interaction_growth_speed[1]=0;
	hand_non_interaction_time[0]=0;
	hand_non_interaction_time[1]=0;
	kinect_hand_pointing_time[0] = 0;
	kinect_hand_pointing_time[1] = 0;
	frost.is_frost=true;
	kinect_hand_direction_history_pos = 0;
	kinect_hand_direction_history_time = 0;
	voicePromptFileName = "sounds/voice_prompts/voice_prompt_ultramarine.ogg";
	kinect_hand_directions[0]=kinect_hand_directions[1]=vec3(0,0,-1);
	for (int i = 0; i < 2; ++i){
		kinect_hand_dir_reversal[i] = false;
		kinect_hand_gesture[i] = false;
		kinect_hand_gesture_spent[i] = false;
	}

}

UltramarineWorld::~UltramarineWorld()
{
	delete videoSphere;
	delete gui;
}



void setupTexture(ofImage &img){
	img.getTextureReference().setTextureWrap(GL_REPEAT, GL_REPEAT);
	GLfloat maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

	glBindTexture(GL_TEXTURE_2D, img.getTextureReference().getTextureData().textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	glGenerateMipmap(GL_TEXTURE_2D);

}

void UltramarineWorld::setupMultiCrystals(glm::vec3 pos, float scale, float interaction_delay, int n){
	for(int i=0;i<n;++i){
		std::shared_ptr<Crystals> c_ptr(new Crystals);
		crystals.push_back(c_ptr);
		Crystals &c=*c_ptr;
		if(i==0){
			c.has_sound=true;
		}else{
			c.has_sound=false;
		}
		c.pos=pos;//vec3(0,-0.2,-10);//vec3((float(i+0.5f)/crystals.size()-0.5f)*20.0f,-0.2,-10);
		c.pos.y-=0.4*scale;
		c.scale=scale;
		c.interaction_delay=interaction_delay;
		c.setup();

		c.radius_scale=ofRandom(0.2,2);
		c.spiral_scale=ofRandom(0.7,2);

		c.spiral2_radius=ofRandom(0.5, 3.0);

		float a=ofRandom(0, 2.0*::pi);
		c.spiral2_base=vec2(sin(a), cos(a));
		c.spiral2_rise=ofRandom(0.2,0.4);

		c.fall_time=fall_start_time+length(pos)*fall_start_time_distance_scale+ofRandom(0.1);

		c.max_growth*=ofRandom(0.7,1.5);
	}
}

void UltramarineWorld::setup(){
	cout << "UltramarineWorld::setup start" << endl;

	//motion_log.open(ofToDataPath("worlds/ultramarine/motion_log.txt"), std::ios_base::app); /// uncomment this line to enable motion logging

	duration = WORLD_ULTRAMARINE::TOTAL_DURATION;
	//glEnable( GL_MULTISAMPLE );
	World::setup();
	camera.setupPerspective(false, 90, 0.01, 200); //TODO: use different near and far values for world/body
	camera.setPosition(0, 1.8, 0);

	resetVideoSphere();

	ofSeedRandom(1);

	setupGUI();



	/*
	for(int i=0;i<crystals.size();++i){
		crystals[i].setup();
		crystals[i].pos=vec3(ofRandom(-10,10),-0.2,ofRandom(-5,-15));
	}*/
	/*
	for(int i=0;i<crystals.size();i+=4){
		setupMultiCrystals(vec3((float(i+2.0f)/crystals.size()-0.5f)*20.0f,-0.2,-10), i, 4);
	}*/

	//ofBuffer buffer = ofBufferFromFile("worlds/ultramarine/crystal_positions.txt");


	//std::ofstream positions_out(ofToDataPath("worlds/ultramarine/crystal_positions_out.txt"));
	//std::ofstream positions(ofToDataPath("worlds/ultramarine/crystal_positions.txt"));
/*
	int num_crystal_towers=30;
	crystals.resize(4*num_crystal_towers);
	for(int i=0;i<crystals.size();i+=4){
		vec2 p=randomPointInCircle(2.0, max_crystal_distance);
		/// 60% are forced to be infront
		if(ofRandom(1.0)<0.6)p.y=-fabs(p.y);

		positions_out<<p.x<<" "<<p.y<<std::endl;

		setupMultiCrystals(vec3(p.x, -0.2, p.y), i, 4);
	}*/

	setupCrystals();
	frost.setup();
	//frost.fall_time=fall_start_time;
	frost.fall_time = -1000;


	setupSnowflakes();

	model.loadModel("worlds/ultramarine/floor_unwrap.DAE");

	model_transform=scale_matrix(glm::vec3(0.15f))*fromOF(model.getModelMatrix());

	int n=model.getMeshCount();
	std::vector<std::string> mesh_names=model.getMeshNames();

	pieces.reserve(n);
	for(int i=0; i<n; ++i){
		std::string name;
		if(i<mesh_names.size())name=mesh_names[i];
		ofxAssimpMeshHelper &mesh=model.getMeshHelper(i);
		GroundPiece piece;
		piece.model_mesh=i;
		piece.mesh=model.getMesh(i);
		glm::mat4x4 m=model_transform*fromOF(mesh.matrix);
		piece.local_centroid=fromOF(piece.mesh.getCentroid());
		glm::vec4 tmp=glm::vec4(piece.local_centroid, 1.0);
		piece.centroid=glm::vec3(m*tmp);
		piece.stress=initial_breaking_stress;

		piece.fall_time=fall_start_time+length(piece.centroid)*fall_start_time_distance_scale+ofRandom(0.5);
		// piece.axis=randomDirection();
		pieces.push_back(piece);
	}

	ofDisableArbTex();

	//environment.loadImage("worlds/ultramarine/environment_map.png");
	//setupTexture(environment);
	environment=new Texture();
	environment->LoadEXR(ofToDataPath("worlds/ultramarine/environment_map.exr"));

	floor_normals.loadImage("worlds/ultramarine/normals.png");
	setupTexture(floor_normals);

	floor_ambient.loadImage("worlds/ultramarine/ambient.png");
	setupTexture(floor_ambient);

	floor_diffuse.loadImage("worlds/ultramarine/diffuse.png");
	setupTexture(floor_diffuse);

	background_back_image.loadImage("worlds/ultramarine/background_back.png");
	background_fore_image.loadImage("worlds/ultramarine/background_fore.png");

    ofEnableArbTex();

    reflection=new TextureFramebuffer(0,0,0,true);

	backgroundSphere=new ofSpherePrimitive();
    backgroundSphere->set(128, 32);

    particle_beam.setup();

	//preload sound
	GetALBuffer(voicePromptFileName);

	for (int i = 0; i < Sound_IceBreak_Filenames_count; ++i){
		GetALBuffer(std::string("worlds/ultramarine/sounds/") + Sound_IceBreak_Filenames[i]);
	}
	for (int i = 0; i < Sound_CrystalGrow_Filenames_count; ++i){
		GetALBuffer(std::string("worlds/ultramarine/sounds/") + Sound_CrystalGrow_Filenames[i]);
	}

	GetALBuffer(sound_breakdown_crackle_filename);
	GetALBuffer(sound_intro_filename);
    std::cout<<"UltramarineWorld::setup end"<<std::endl;

}

void UltramarineWorld::setupCrystals(){
	crystals.clear();
	/// loading from crystal_positions.txt file disabled
	ofSeedRandom(1);
	/*
	std::ifstream positions_file(ofToDataPath("worlds/ultramarine/crystal_positions.txt"));
	std::vector<std::string> lines;
	std::string line;
	while(std::getline(positions_file, line)){
		if(line.size() && line[0]!='#' && line.find_first_not_of(' ') != std::string::npos)lines.push_back(line);/// not only spaces
	}
	std::cout<<"Number of crystals read:"<<lines.size()<<std::endl;

	for(int i=0; i<lines.size(); ++i){
		std::istringstream ss(lines[i]);
		float x=0, y=0, scale=1, interaction_delay=0;
		ss>>x;
		ss>>y;
		ss>>scale;
		ss>>interaction_delay;
		setupMultiCrystals(vec3(x, 0, y), scale, interaction_delay, 4);
	}
	*/
}

void UltramarineWorld::setupGUI() {
	gui = new ofxUIScrollableCanvas();

	gui->addButton("RESET", false);

	gui->addSlider("BLOOM MULTIPLIER COLOR", 0.0f, 1.0f, &BLOOM_MULTIPLIER_COLOR);
	gui->addSlider("GAMMA", 0.0f, 5.0f, &GAMMA);
	gui->addSlider("EXPOSURE", 0.0f, 5.0f, &EXPOSURE);
	gui->addSlider("ANTIALIASING", 0.0, 4.0, &ANTIALIASING);
	gui->addSlider("MULTISAMPLE ANTIALIASING", 0.0, 32, &MULTISAMPLE_ANTIALIASING);
	gui->addToggle("CRISP ANTIALIASING", &CRISP_ANTIALIASING);
	gui->addToggle("USE ANTIALIASING", &USE_ANTIALIASING);
	gui->addSlider("pointing_steady_threshold", 0.0, 0.5, &pointing_steady_threshold);

	gui->autoSizeToFitWidgets();
	ofAddListener(gui->newGUIEvent, this, &UltramarineWorld::guiEvent);

	gui->loadSettings("worlds/ultramarine/settings.xml");

	gui->setVisible(false);
}

void UltramarineWorld::guiEvent(ofxUIEventArgs &event) {
	if (event.widget->getName() == "RESET") {
		BLOOM_MULTIPLIER_COLOR = DEFAULT_BLOOM_MULTIPLIER_COLOR;
		GAMMA = DEFAULT_GAMMA;
		EXPOSURE = DEFAULT_EXPOSURE;
		ANTIALIASING = DEFAULT_ANTIALIASING;
		MULTISAMPLE_ANTIALIASING = DEFAULT_MULTISAMPLE_ANTIALIASING;
		CRISP_ANTIALIASING = DEFAULT_CRISP_ANTIALIASING;
		USE_ANTIALIASING = DEFAULT_USE_ANTIALIASING;
		pointing_steady_threshold=default_pointing_steady_threshold;
	}
	gui->saveSettings("worlds/ultramarine/settings.xml");
}

void UltramarineWorld::reset() {
	cout << "UltramarineWorld::reset start" << endl;
	hwPrepared[0] = hwPrepared[1] = hwPrepared[2] = false;
	duration = WORLD_ULTRAMARINE::TOTAL_DURATION;

	camera.setPosition(0, 1.8, 0);
	resetVideoSphere();
	played_intro_sound = false;
	played_crash_sound=false;
	playedVoicePrompt = false;
	cout << "UltramarineWorld::resetting crystals..." << endl;
	setupCrystals();
	frost.reset();
	/*
	for(auto &c : crystals){
		c.reset();
	}*/
	cout << "UltramarineWorld::resetting pieces..." << endl;
	for(auto &piece : pieces){
		piece.reset();
		piece.stress=initial_breaking_stress;
		piece.fall_time=fall_start_time+length(piece.centroid)*fall_start_time_distance_scale+ofRandom(0.5);
	}
	setupSnowflakes();
	cout << "UltramarineWorld::reset end" << endl;
}

void UltramarineWorld::resetVideoSphere() {
	std::cout << "UltramarineWorld::resetVideoSphere() begin" << std::endl;
#ifndef USE_VIDEO_RESET
	if (videoSphere)videoSphere->close();
	delete videoSphere;
	videoSphere = new VideoSphere(engine, "video/Intro_Main.mov", "video/Intro_Main_Alpha.mov");
#else
	if (videoSphere){
		videoSphere->pause();
		videoSphere->rewind();
	}else{
		videoSphere = new VideoSphere(engine, "video/Intro_Main.mov", "video/Intro_Main_Alpha.mov");
	}
#endif
	std::cout << "UltramarineWorld::resetVideoSphere() end" << std::endl;
}

void UltramarineWorld::beginExperience() {
	OSCManager::getInstance().startWorld(COMMON::WORLD_TYPE::ULTRAMARINE);
	videoSphere->play();
	PlayBackgroundSound(sound_intro_filename, breakdown_gain);
	//#ifndef NO_ANNOYING_CRAP /// a define set in my builds - dmytry.
	//#endif
	old_time = time = 0;
	non_interaction_time=0;
	hand_interaction_growth_speed[0]=0;
	hand_interaction_growth_speed[1]=0;
	hand_non_interaction_time[0]=0;
	hand_non_interaction_time[1]=0;
	camera.setPosition(0, 1.8, 0);
	kinect_hand_direction_history_pos = 0;
	kinect_hand_direction_history_time = 0;
}
void UltramarineWorld::endExperience() {
	OSCManager::getInstance().endWorld(COMMON::WORLD_TYPE::ULTRAMARINE);
#ifndef USE_VIDEO_RESET
	if (videoSphere != NULL) {
		videoSphere->close();
	}
	delete videoSphere;
	videoSphere=NULL;
#endif
	playedVoicePrompt = false;

	crystals.clear();
	GlobAudioManager().RemoveSoundObject(&ambientSound);
}

ofVec3f intersectWithXZPlane (ofVec3f position, ofVec3f direction) {
	return ofVec3f(position.x-direction.x*position.y/direction.y, 0, position.z-direction.z*position.y/direction.y);
}

float AngleBetweenVectors(glm::vec3 dir1, glm::vec3 dir2){
	return atan2(length(cross(dir1,dir2)),dot(dir1,dir2));
}

void UltramarineWorld::update(float dt){
	old_time=time;
	#ifdef TESTING_ULTRAMARINE
	time = Timeline::getInstance().getElapsedCurrWorldTime(); /// to avoid changing rest of the code.
	#else
	time = Timeline::getInstance().getElapsedCurrWorldTime()-WORLD_ULTRAMARINE::DURATION_INTRO; /// to avoid changing rest of the code.
	#endif

	if(TimeAt(-3.0f)){
		ambientSound.playback_start=0.0f;
		GlobAudioManager().AddSoundObject(&ambientSound);
	}
	//std::cout<<time<<std::endl;

	if (time >= WORLD_ULTRAMARINE::DURATION_MAIN) {
		// main world has finished, transition will start now
		if (hwPrepared[1] == false) {
			hwPrepared[1] = true;

		}
	}

	if (!Timeline::getInstance().isPaused()) {
		videoSphere->update();
	}

	updateKinect(dt);

	if(time>-2.0f){
		updateSnowflakes(dt);
	}

	if (time < 0.0f) {
		return;
	}

	ambientSound.sound_gain = glm::smoothstep(WORLD_ULTRAMARINE::DURATION_MAIN, WORLD_ULTRAMARINE::DURATION_MAIN - 2.0f, time);

	// intro video has finished, main world will start now
	if (hwPrepared[0] == false) {
		hwPrepared[0] = true;
		OSCManager::getInstance().setFrontFanSpeed(1,0.1);
	}

	// play audio cue only once and after the video has finished
	if (!playedVoicePrompt && Timeline::getInstance().getElapsedCurrWorldTime() >= WORLD_ULTRAMARINE::DURATION_INTRO + COMMON::AUDIO_CUE_DELAY) {
		playedVoicePrompt = true;
		PlayBackgroundSound(voicePromptFileName, COMMON::VOICE_PROMPT_GAIN);
	}

	float t = glm::clamp(1.0f - time / growth_closing_in_time, 1E-10f, 1.0f);

	float interaction_distance=glm::mix(min_crystal_distance, max_crystal_distance, glm::mix(t, t*t, growth_closing_in_nonlinearity));
	float close_in_factor = interaction_distance / max_crystal_distance;

	vec2 interaction_pos[2];/// x,z
	for(int i=0;i<2;++i){
		interaction_pos[i]=interaction_distance*normalize(vec2(kinect_hand_directions[i].x, kinect_hand_directions[i].z));
		/// minimize kinect weirdness
		particle_beam.beam_dir[i] = normalize(glm::vec3(interaction_pos[i].x, 0.0, interaction_pos[i].y)-fromOF(camera.getPosition()));
	}





	//int num_directions=2;

	if(do_updates){
		const float max_r=0.1;
		for(auto &c : crystals){
			c->grow_sum_tmp=0.0f;
		}

		float total_interaction=0;
		hand_interaction_growth_speed[0]=0;
		hand_interaction_growth_speed[1]=0;
		float hand_closest_crystal_dist[2] = { 1E20, 1E20 };
		for (int hand = 0; hand < 2; ++hand) {
			hand_non_interaction_time[hand]+=dt;
			// if (kinect_hand_pointing_time[hand] < hands_extension_time) continue; /// !kinect_hand_gesture[hand]
			for(auto &c : crystals){
				if(c->interaction_delay>time)continue;

				//vec3 dir=normalize(c->pos-hand_positions[hand]);
				//float angle=AngleBetweenVectors(dir,directions[hand]);

				vec2 p=vec2(c->pos.x, c->pos.z);
				float dist = length(p - interaction_pos[hand]);

				hand_closest_crystal_dist[hand] = glm::min(hand_closest_crystal_dist[hand],
					dist - close_in_factor*(c->grow_speed>0.2f ? min_distance_from_growing_crystal : min_distance_between_crystals));

				bool interacting=dist<interaction_radius;
				float interaction_amount=interacting ? 1 : 0;

				if(interacting){
					hand_interaction_growth_speed[hand]+=c->grow_speed;
				}

				//c->grow(time, dt, grow_speed*grow_speed_factor);
				c->grow_sum_tmp+=interaction_amount*grow_speed_factor;
				if(c->growth < 0.9*c->max_growth)total_interaction+=interaction_amount;
			}
			//particle_beam.beam_power[hand]=0.2+hand_interaction_growth_speed[hand]*0.4;
		}
		/*
		if(total_interaction<0.5){
			non_interaction_time+=dt;
		}else{
			non_interaction_time=0;
		}*/

		for(auto &c : crystals){
			if(c->interaction_delay>time)continue;
			//vec2 p=vec2(c->pos.x, c->pos.z);
			//float dist=length(p);
			//if(dist<growth_min_dist)continue;

			c->grow(time, dt, grow_speed_factor); /// growing to full size always

			//c->grow(time, dt, c->grow_sum_tmp); /// growing dependent to pointing
			/*
			c->grow(time, dt, c->grow_sum_tmp + (dist>growth_min_dist ?
																	((non_interaction_time>non_interact_timeout &&
																					dist>max_crystal_distance*clamp(1.0f-(non_interaction_time-non_interact_timeout)/10.0f , 0.0f, 1.0f) ) ?
																					non_interact_grow_speed : baseline_growth_speed):0.0));
			*/
		}

		/// add frost
		/*
		for(int hand=0; hand<2; ++hand){
			if(directions[hand].y<-0.01 && ofRandom(1.0)<0.2){
				vec3 ground_pos=hand_positions[hand] - directions[hand]*hand_positions[hand].y/directions[hand].y;
				float angle=ofRandom(2.0*::pi);
				float r=0.8*log(ofRandom(0.01,1.0));
				frost.addFrost(ground_pos + vec3(r*sin(angle),0,r*cos(angle)) );
			}
		}
		frost.grow(time, dt, 1.0);
		*/
		frost.grow(time, dt, 1.0);

		for (int hand = 0; hand < 2; ++hand){
			particle_beam.beam_power[hand] *= exp(-1.0*dt);
			if (particle_beam.beam_power[hand] < 0.001) particle_beam.beam_power[hand] = 0;
		}
		if(time<fall_start_time){
			for(int hand=0; hand<2; ++hand){/// add crystals when not interacting with any existing crystals
				if (hand_non_interaction_time[hand]>wait_time_before_adding_crystal
					&& hand_closest_crystal_dist[hand] >= 0
					/* && kinect_hand_pointing_time[hand] >= hands_extension_time */
					&& kinect_hand_gesture[hand] /* && kinect_hand_pointing_time[hand] <= hands_extension_time+0.1 */){
					vec3 ground_pos(interaction_pos[hand].x, 0.0, interaction_pos[hand].y);
					float ground_pos_l = length(ground_pos);
					if (ground_pos_l < 0.1)break;
					setupMultiCrystals(ground_pos, 1.0, 0.0, 4);
					particle_beam.beam_power[hand] = 1.0;
					/*
					for (int i = 0; i < crystal_throw_count; ++i){
						frost.addProjectile(kinect_hand_positions[hand] + particle_beam.beam_dir[hand] * 0.2f + randomDirection()*ofRandom(0.2), normalize(particle_beam.beam_dir[hand]) * crystal_throwing_speed + randomDirection()*ofRandom(0.5) + vec3(0, -0.5*ground_pos_l*fall_acceleration/crystal_throwing_speed, 0), randomDirection()*10.0f, 0.5);
					}*/
					for (int i = 0; i < crystal_throw_count; ++i){
						frost.addProjectile(glm::mix(kinect_hand_positions[hand] + randomDirection()*ofRandom(2.0), ground_pos, ofRandom(1.0)), randomDirection()*ofRandom(2.0), randomDirection()*10.0f, 0.5);
					}
					kinect_hand_gesture_spent_dir[hand] = kinect_hand_directions[hand];
					kinect_hand_gesture_spent[hand] = true;
					hand_non_interaction_time[hand] = 0;
					non_interaction_time = 0;
					break; /// avoid setting up crystals for each hand at once in case hand interactions overlap.
				}
			}
			if (non_interaction_time>non_interact_timeout){/// spawn new crystals if there had been no crystals spawned for non_interact_timeout seconds
				non_interaction_time = non_interact_timeout - ofRandom(0.2, 2.0);
				const float sector_width = 160.0 * (::pi/180.0);
				float a = ofRandom(-0.5*sector_width, 0.5*sector_width);
				vec3 ground_pos = vec3(sin(a)*interaction_distance, 0, -cos(a)*interaction_distance);
				setupMultiCrystals(ground_pos, 1.0, 0.0, 4);
			}
			non_interaction_time += dt;
		}

		for(int i=0; i<pieces.size(); ++i){/// todo: sudden cracks, rapid sequence of cracking towards user
			GroundPiece &piece=pieces[i];
			const float crack_effect_r=3.0;
			const float crack_towards_user_width=2.0;
			float crack_speed=0;
			if (time < fall_start_time+1.5){/// cracking for up to 1.5 seconds after fall starts
				for (auto &tower_ptr : crystals){
					Crystals &tower = *tower_ptr;
					vec2 piece_center(piece.centroid.x, piece.centroid.z);
					vec2 tower_pos(tower.pos.x, tower.pos.z);
					vec2 delta = piece_center - tower_pos;

					float r = length(delta);

					float speed = glm::max(0.0f, 1.0f - r / crack_effect_r);

					/// crack towards user:
					float rl = glm::dot(tower_pos, piece_center) / glm::dot(tower_pos, tower_pos);

					float kr = glm::clamp(glm::min(rl*3.0f, (1.0f - rl)*6.0f), 0.0f, 1.0f)*glm::clamp(rl - 1.0f + tower.growth / tower.max_growth, 0.0f, 1.0f);/// /--\ function multiplied by ramp

					float rs = glm::dot(piece_center, glm::vec2(tower_pos.y, -tower_pos.x)) / length(tower_pos);

					speed += glm::max(0.0f, 1.0f - fabs(rs)*2.0f / crack_towards_user_width)*kr;

					speed *= tower.growth / tower.max_growth;
					/// tbd: maybe scale by the crystals[j].growth
					crack_speed += speed*4.0;
				}
				float stress = crack_speed;
				if (piece.stress < stress){
					//my_logger<<"crack"<<std::endl;
					float jolt_time = ofRandom(0.5*jolt_duration, 1.5*jolt_duration);
					float angle = ofRandom(jolt_angle_low, jolt_angle_high);

					piece.angular_velocity = randomDirection();

					piece.target_orientation = glm::angleAxis(angle, piece.angular_velocity);//*piece.orientation;
					piece.angular_velocity *= (angle / jolt_time);
					piece.spring = 3.0 / jolt_time;
					piece.damping = 0.5 / jolt_time;

					piece.jolt_timer = jolt_time*20.0;

					piece.stress = stress + after_jolt_breaking_stress;//ofRandom(0.5*breaking_stress,1.5*breaking_stress);

					/// creaking sound
					/// we run out of sound sources, so each has a limited chance of playing.
					if (ofRandom(1.0) < ice_crack_sound_probability)Play3DSound(Sound_Icebreak(ofRandom(2.9)), false, false, piece.centroid, vec3(0.0), 1E10, ice_cracking_gain, 1.0);
				}
			}
			if(piece.fall_time<time){ ///falling
				if(piece.fall_time>0){/// just began falling
					piece.fall_time=0;
					piece.angular_velocity=randomDirection()*fall_rotation;
				}
				float avl=length(piece.angular_velocity);
				if(avl>1E-10){
					piece.orientation=normalize(glm::angleAxis(dt*avl, piece.angular_velocity*(1.0f/avl))*piece.orientation);
				}
				piece.velocity.y+=fall_acceleration*dt;
				piece.offset+=piece.velocity*dt;
			}else /// not falling
			if(piece.jolt_timer>0){
				float jolt_dt=std::min(piece.jolt_timer, dt);
				float avl=length(piece.angular_velocity);
				if(avl>1E-10){
					piece.orientation=normalize(glm::angleAxis(jolt_dt*avl, piece.angular_velocity*(1.0f/avl))*piece.orientation);
				}
				glm::quat delta=piece.target_orientation*glm::inverse(piece.orientation);/// delta*piece.orientation = piece.target_orientation
				glm::vec3 desired_avel=normalize(axis(delta))*angle(delta);
				piece.angular_velocity+=desired_avel*piece.spring*jolt_dt;
				piece.angular_velocity*=exp(-jolt_dt*piece.damping);

				piece.jolt_timer-=dt;
			}
		}
	}

	if (time > fall_start_time + sound_breakdown_crackle_start_time_offset && !played_crash_sound){
		played_crash_sound=true;
		Play3DSound(sound_breakdown_crackle_filename, false, true, glm::vec3(0.0, 1.8, 0.0), glm::vec3(0), 1E10, breakdown_gain);
	}

	//particle_beam.update(dt);

}

void UltramarineWorld::updateKinect(float dt){
	if (engine->kinectEngine.isKinectDetected()) {
		//std::cout<<"has kinect"<<std::endl;
		for(int i=0;i<2;++i){
			particle_beam.beam_dir[i]=kinect_hand_directions[i]=normalize(fromOF(engine->kinectEngine.getHandsDirection()[i]));

			kinect_hand_positions[i]=particle_beam.beam_start[i]=fromOF(engine->kinectEngine.getHandsPosition()[i]);
			//vec2 tmp(kinect_hand_positions[i].x, kinect_hand_positions[i].z);
			//std::cout<<length(kinect_hand_positions[i])<<" ";
			if (length(kinect_hand_positions[i].xz())>hands_extension_limit) {
				kinect_hand_pointing_time[i] += dt;
				//std::cout << "hand "<<i <<" extended"<<std::endl;
			}
			else
			{
				kinect_hand_pointing_time[i] = 0;
				kinect_hand_gesture_spent[i] = false;
				//std::cout << "hand " << i << " unextended" << std::endl;
			}
			//std::cout << hand_positions[0].x << "," << hand_positions[0].y << "," << hand_positions[0].z << std::endl;
			float azimuth = atan2(kinect_hand_directions[i].x, -kinect_hand_directions[i].z);
			float altitude = atan2(kinect_hand_directions[i].y, -kinect_hand_directions[i].z);
			if(motion_log.is_open())motion_log << i << " " << time << " " << azimuth << " " << altitude << std::endl;

		}
		//std::cout << std::endl;

	} else { /// imitate kinect with mouse for testing
		if(length2(mouse_dir)>0.01){
			kinect_hand_directions[0]=kinect_hand_directions[1]=normalize(vec3(mouse_dir.x, mouse_dir.y, mouse_dir.z));
			kinect_hand_positions[0]=kinect_hand_positions[1]=particle_beam.beam_start[0]=particle_beam.beam_start[1]=fromOF(camera.getPosition())+glm::vec3(0,-0.25,-0.4);
			particle_beam.beam_dir[0]=particle_beam.beam_dir[1]=mouse_dir;

			//particle_beam.beam1_dir+=0.05f*glm::noise3(glm::vec3(time*3.5f, 1.0f, 0.0f));
			//particle_beam.beam2_dir+=0.05f*glm::noise3(glm::vec3(time*3.5f, 10.0f, 0.0f));

			particle_beam.beam_dir[0]=normalize(particle_beam.beam_dir[0]);
			particle_beam.beam_dir[1]=normalize(particle_beam.beam_dir[1]);

			particle_beam.beam_start[0].x-=0.3;
			particle_beam.beam_start[1].x+=0.3;

			kinect_hand_pointing_time[0] += dt;
			kinect_hand_pointing_time[1] += dt;

			//particle_beam.beam2_dir.x=-particle_beam.beam2_dir.x;/// symmetric beams to test hands pointing in diff. directions.
		}
	}
	/// in case something crazy happens with time
	if (kinect_hand_direction_history_time > time + 1.0 / 30.0) kinect_hand_direction_history_time = time;
	if (kinect_hand_direction_history_time < time - 1.0) kinect_hand_direction_history_time = time;

	while (kinect_hand_direction_history_time < time){
		kinect_hand_direction_history_time += 1.0 / 60.0;
		for (int hand = 0; hand < 2; ++hand){
			kinect_hand_direction_history[kinect_hand_direction_history_pos][hand] = kinect_hand_directions[hand];
			kinect_hand_position_history[kinect_hand_direction_history_pos][hand] = kinect_hand_positions[hand];
		}
		kinect_hand_direction_history_pos = (kinect_hand_direction_history_pos + 1) % kinect_hand_direction_history_count;
	}

	/*
	for (int hand = 0; hand < 2; ++hand){
		vec3 avg(0);
		for (int i = 0; i < kinect_hand_direction_history_count; ++i){
			avg += kinect_hand_direction_history[i][hand];
		}
		avg *= 1.0f / kinect_hand_direction_history_count;
		float avg_dist = 0;
		for (int i = 0; i < kinect_hand_direction_history_count; ++i){
			avg_dist += length(kinect_hand_direction_history[i][hand].xy()-avg.xy());
		}
		avg_dist /= kinect_hand_direction_history_count;
		kinect_hand_motion_below_threshold[hand] = avg_dist < pointing_steady_threshold;
	}
	*/
	for (int hand = 0; hand < 2; ++hand){
		if (length(kinect_hand_gesture_spent_dir[hand] - kinect_hand_directions[hand])>min_gesture_direction_change){
			//if (kinect_hand_gesture_spent[hand])std::cout << "gesture unspent " << std::endl;
			kinect_hand_gesture_spent[hand] = false;
		}
		kinect_hand_motion_below_threshold[hand] =
			length(getHistoryDir(hand, 0.0) - getHistoryDir(hand, pointing_steady_time_interval))<pointing_steady_time_interval*pointing_steady_threshold;
		vec3 d1 = getHistoryDir(hand, 0.0) - getHistoryDir(hand, 0.1);
		vec3 d2 = getHistoryDir(hand, 0.1) - getHistoryDir(hand, 0.4);
		bool dir_reversal = (dot(d1, d2) < 0.0f);
		kinect_hand_dir_reversal[hand] = dir_reversal;
		//std::cout << (kinect_hand_pointing_time[hand] > 0.0) << " " << kinect_hand_motion_below_threshold[hand] << " " << dir_reversal << " ";
		bool hand_retracting = length(getHistoryPos(hand, 0).xz())<length(getHistoryPos(hand, 0.1).xz());
		kinect_hand_gesture[hand] = (kinect_hand_motion_below_threshold[hand] || dir_reversal)&&(kinect_hand_pointing_time[hand] > 0.001f) && (!kinect_hand_gesture_spent[hand]) && !hand_retracting;
	}
	//std::cout << std::endl;
}

glm::vec3 UltramarineWorld::getHistoryDir(int hand, float time_ago){
	int ticks_ago = int(time_ago*60.0);
	if (ticks_ago < 0) ticks_ago = 0;
	ticks_ago += 1;/// the kinect_hand_direction_history_pos is where the next data samle will go.
	ticks_ago = std::min(ticks_ago, kinect_hand_direction_history_count - 1);
	int i = (kinect_hand_direction_history_pos + kinect_hand_direction_history_count - ticks_ago) % kinect_hand_direction_history_count;
	return kinect_hand_direction_history[i][hand];
}

glm::vec3 UltramarineWorld::getHistoryPos(int hand, float time_ago){
	int ticks_ago = int(time_ago*60.0);
	if (ticks_ago < 0) ticks_ago = 0;
	ticks_ago += 1;/// the kinect_hand_direction_history_pos is where the next data samle will go.
	ticks_ago = std::min(ticks_ago, kinect_hand_direction_history_count - 1);
	int i = (kinect_hand_direction_history_pos + kinect_hand_direction_history_count - ticks_ago) % kinect_hand_direction_history_count;
	return kinect_hand_position_history[i][hand];
}

void UltramarineWorld::setupSnowflakes(){
	snowflakes.clear();
	/// setup snowflakes
	snowflakes.resize(snowflakes_count);
	for(Snowflake &s : snowflakes){
		vec3 rp;
		rp.x=ofRandom(-snowflakes_visibility,snowflakes_visibility);
		rp.y=ofRandom(0.2,snowflakes_visibility);
		rp.z=ofRandom(-snowflakes_visibility,snowflakes_visibility);

		s.pos=fromOF(camera.getPosition()) + rp;
		s.size=0.1+glm::pow(ofRandom(0,1),4.0f);
		s.fluff=ofRandom(0.5,1.5);
		s.avel=randomDirection()*1.0f;
	}
}

bool UltramarineWorld::resetSnowflakeIntoBeam(glm::vec3 &p){
	if(ofRandom(1.0)<0.5){
		bool b=ofRandom(1.0)<0.5;
		if(ofRandom(1.0)>particle_beam.beam_power[b])return false;
		vec3 bs=particle_beam.beam_start[b];
		vec3 bd=particle_beam.beam_dir[b];
		p=bs+bd*ofRandom(snowflakes_visibility)+randomDirection()*0.5f;
		return true;
	}
	return false;
}

void UltramarineWorld::updateSnowflakes(float dt){
	//std::cout<<dt<<std::endl;
	//std::cout<<time<<std::endl;
	vec3 base_wind_vel=wind_speed*(vec3(0,0,0.7)+0.3f*perlin3(vec3(time*0.03))*vec3(1,0.2,1)+perlin3(vec3(time*0.1)));
	base_wind_vel.y=0;
	for(Snowflake &s : snowflakes){
		s.pos+=s.vel*dt;
		s.vel.y-=6.0*dt;/// gravity

		float k=clamp(s.fluff*dt*40.0f, 0.0f, 1.0f);/// drag amount

		vec4 b=particle_beam.velocityField(s.pos);
		vec3 beam_vel(b.x,b.y,b.z);
		beam_vel*=15.0;
		vec3 local_wind_turbulence=simplex3(s.pos*0.1f+0.2f*vec3(time*0.3f, time*0.5f, time*1.11234f))*0.5f*wind_speed;
		vec3 wind_vel=(local_wind_turbulence+base_wind_vel);
		float a=length(beam_vel)-length(wind_vel);
		vec3 tmp=mix(wind_vel, beam_vel+local_wind_turbulence, clamp(a, 0.0f, 1.0f));
		wind_vel=tmp;

		//wind_vel+=b.xyz()*5.0f;

		s.vel=(1.0f-k)*s.vel+k*wind_vel;

		float avl=length(s.avel);
		if(avl>1E-10){
			s.orientation=normalize(glm::angleAxis(dt*avl, s.avel*(1.0f/avl))*s.orientation);
		}
		/// cycle snowflakes around the camera
		vec3 offset=fromOF(camera.getPosition());
		vec3 p=s.pos-offset;
		if(p.x<-snowflakes_visibility){
			if(!resetSnowflakeIntoBeam(s.pos)){
				p.x=snowflakes_visibility;
				p.y=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				p.z=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				s.pos=p+offset;
			}
		}else if(p.x>snowflakes_visibility)
		{
			if(!resetSnowflakeIntoBeam(s.pos)){
				p.x=-snowflakes_visibility;
				p.y=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				p.z=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				s.pos=p+offset;
			}
		}

		if(s.pos.y<0){
			if(!resetSnowflakeIntoBeam(s.pos)){
				p.y=snowflakes_visibility;
				p.x=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				p.z=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				s.pos=p+offset;
			}
		}else if(p.y>snowflakes_visibility)
		{
			if(!resetSnowflakeIntoBeam(s.pos)){
				p.y=-snowflakes_visibility;
				p.x=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				p.z=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				s.pos=p+offset;
			}
		}

		if(p.z<-snowflakes_visibility){
			if(!resetSnowflakeIntoBeam(s.pos)){
				p.z=snowflakes_visibility;
				p.x=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				p.y=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				s.pos=p+offset;
			}
		}else if(p.z>snowflakes_visibility)
		{
			if(!resetSnowflakeIntoBeam(s.pos)){
				p.z=-snowflakes_visibility;
				p.x=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				p.y=ofRandom(-snowflakes_visibility, snowflakes_visibility);
				s.pos=p+offset;
			}
		}

		//vec3 newpos=glm::mod(s.pos+offset, snowflakes_visibility*2.0f)-offset;

	}
}

void UltramarineWorld::drawSnowflakes(float transparency_factor){
	CrystalsSharedData::Instance().begin();
	glUseProgram(0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float fade=glm::clamp((WORLD_ULTRAMARINE::DURATION_MAIN-time)/snowflakes_fade_time, 0.0f, 1.0f);
	glColor4f(0.6, 0.85, 1, 0.2*transparency_factor*fade);
	for(Snowflake &s : snowflakes){
		glPushMatrix();
		glTranslatef(s.pos.x, s.pos.y, s.pos.z);

		mat4 mat=mat4_cast(s.orientation);
		glMultMatrixf(&mat[0][0]);

		glScalef(0.17, 0.17, 0.002);
		CrystalsSharedData::Instance().draw();
		glPopMatrix();
	}
	glDisable(GL_BLEND);
	CrystalsSharedData::Instance().end();
}


void UltramarineWorld::drawScene(renderType render_type, ofFbo *fbo){
	glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
	glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);

	drawEnvironmentAsBackground();

	ofEnableDepthTest();
    glDisable(GL_BLEND);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

	/// drawing reflection
	if(reflection.ok() && engine->bloom.base_fb.ok() ){
		reflection->Resize(engine->bloom.base_fb->width/reflection_resolution_divisor, engine->bloom.base_fb->height/reflection_resolution_divisor);/// todo: pass width and height into the drawScene somehow?

		reflection->BeginFB();
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glPushMatrix();
		glScalef(1,-1,1);
		for(auto &c : crystals){
			c->draw(*this, time);
		}
		frost.draw(*this, time);

		glPopMatrix();
		/// clip out underground crystals when drawing reflection
		glDepthFunc(GL_GEQUAL);
		glColor3f(0.0, 0.0, 0.0);
		glBegin(GL_QUADS);
		glVertex3f(-100,0,100);
		glVertex3f(100,0,100);
		glVertex3f(100,0,-100);
		glVertex3f(-100,0,-100);
		glEnd();
		glDepthFunc(GL_LEQUAL);

		//particle_beam.draw(time);/// particles reflect also

		reflection->EndFB();


	}    //glClearColor(fog_color.r, fog_color.g, fog_color.b, 1.0);
    //glClear(GL_COLOR_BUFFER_BIT);
    //glClearColor(0, 0, 0, 1.0);




	total_crystals=0;
	glDisable(GL_BLEND);
	glCullFace(GL_FRONT);
	for(auto &c : crystals){
		c->draw(*this, time);
	}
	frost.draw(*this, time);


    //drawGround();

    //---------------------------------------------------- ofDrawGridPlane


	if(draw_ground){
		drawGround();

		/*
		glColor3f(0.1,0.2,0.4);
		glBegin(GL_QUADS);
		glVertex3f(-100,0,-100);
		glVertex3f(100,0,-100);
		glVertex3f(100,0,100);
		glVertex3f(-100,0,100);
		glEnd();
		*/
		/*
		ofPushView();
		ofPushMatrix();
		ofPushStyle();
		ofTranslate(0,0.01,0);
		ofRotate(-90,0,0,1);
		ofSetColor(100);
		ofDrawGridPlane(100.0f, 10.0f, false );
		ofSetColor(0,20,0);
		//ofDrawPlane(-100,-100,200,200);
		ofPopStyle();
		ofPopMatrix();
		ofPopView();*/
	}

	glDisable(GL_CULL_FACE);

	drawSnowflakes(glm::clamp((time+2.0f)/2.0f, 0.0f, 1.0f));

	//if(fract(time)<0.5)
	if(false)
	{
		glUseProgram(0);
		glColor4f(1,1,1,1);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(1,0,0);
		glScalef(-1,1,1);/// do not mirror the images
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glRotatef(270.0f, 0, 1, 0);
		backCylinder.set(100.0f, -250.0f, 16, 1, 0, false);
		backCylinder.setPosition(0.0, 5.0, 0.0);
		background_back_image.getTextureReference().bind();
		glDisable(GL_BLEND);
		backCylinder.draw();

		foreCylinder.set(80, -150, 16, 1, 0, false);
		foreCylinder.setPosition(0.0, 5.0, 0.0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		background_fore_image.getTextureReference().bind();

		foreCylinder.draw();

		glDisable(GL_BLEND);
		glPopMatrix();

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}

    //std::cout<<total_crystals<<std::endl;

/*
	/// disabled transitions for now
	float transition_start=3.0;

    if(engine->getCurrentWorldTime()>transition_start){
    	World *nw=engine->getNextWorld();
    	if(nw){
			float transition_t=(engine->getCurrentWorldTime()-transition_start)/(getDuration()-transition_start);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glTranslatef(0, 1.8, -(1.0-transition_t)*50.0-2.5);
    		nw->drawTransitionObject(render_type,fbo);
			glPopMatrix();
    	}
    }
    */
	if (videoSphere && videoSphere->isPlaying()) {
		videoSphere->draw();
	}

	glActiveTexture(GL_TEXTURE0);
	environment->BindTexture();
	if (/* _DEBUG_ */ false){
		engine->kinectEngine.drawBodyUltramarine(ofVec3f(0.0, 1.8, 0.0), 1.0f,
			ofVec3f(1.0, kinect_hand_dir_reversal[0] ? 1 : 0, kinect_hand_motion_below_threshold[0] ? 1 : 0),
			ofVec3f(1.0, kinect_hand_dir_reversal[1] ? 1 : 0, kinect_hand_motion_below_threshold[1] ? 1 : 0)
			);
	}
	else{
		engine->kinectEngine.drawBodyUltramarine(ofVec3f(0.0, 1.8, 0.0), 1.0f);
	}
	glBindTexture(GL_TEXTURE_2D,0);



	//particle_beam.draw(time);
}

/*
glm::mat4 translate(glm::vec3 v){
}*/

void UltramarineWorld::drawGround(){
	//ofSetColor(255,0,0);

	glEnable(GL_CULL_FACE);

	glCullFace(GL_BACK);

    glPushMatrix();
    //glMultMatrixf(&model_transform[0][0]);


	//glBindTexture(GL_TEXTURE_2D, slateNormalsImage.getTextureReference().getTextureData().textureID);
    /*
    model.drawWireframe();
    */
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0.5,0.5,0.5);
	floor_shader().begin();

	floor_shader().setUniform3f("fog_color", fog_color.r, fog_color.g, fog_color.b);

	glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, environment.getTextureReference().getTextureData().textureID);
    if(environment.ok()){
    	environment->BindTexture();
    }

    floor_shader().setUniform1i("normals", 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, floor_normals.getTextureReference().getTextureData().textureID);

	floor_shader().setUniform1i("diffuse", 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, floor_diffuse.getTextureReference().getTextureData().textureID);

	floor_shader().setUniform1i("ambient", 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, floor_ambient.getTextureReference().getTextureData().textureID);

	if(reflection.ok()){
		glActiveTexture(GL_TEXTURE4);
		floor_shader().setUniform1i("reflection",4);
		reflection->BindTexture();
		floor_shader().setUniform2f("resolution", engine->bloom.base_fb->width, engine->bloom.base_fb->height /*reflection->width, reflection->height*/ );/// TODO: pass render width and height here

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) ;
		glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) ;
		//glBindTexture(GL_TEXTURE_2D, floor_ambient.getTextureReference().getTextureData().textureID);
	}

	ofVec3f cam_pos=camera.getPosition();
	floor_shader().setUniform3f("cam_pos", cam_pos.x, cam_pos.y, cam_pos.z);
    for(int i=0;i<pieces.size();++i){
		//pieces[i].mesh.drawWireframe();
		GroundPiece &p=pieces[i];
		ofxAssimpMeshHelper &mesh=model.getMeshHelper(i);
		ofPushMatrix();
		/// translation(offset)*mesh.matrix*translation(local_centroid)*rotation*scale*translation(-local_centroid)
		glm::mat4x4 mat;
		mat=translate(mat, p.offset);
		mat*=model_transform;
		mat*=fromOF(mesh.matrix);
		mat=translate(mat, p.local_centroid);
		mat*=mat4_cast(p.orientation);
		mat=scale(mat,p.scale);
		mat=translate(mat, -p.local_centroid);
		//translate(scale(mat4_cast(p.orientation)*translate(translate(mat4(),p.offset)*fromOF(mesh.matrix),p.local_centroid), p.scale), -p.local_centroid);


		//ofMultMatrix(mesh.matrix);
		glMultMatrixf(&mat[0][0]);
		glm::mat4 to_world=mat;
		glm::mat3 normal_to_world=glm::inverse(glm::transpose(glm::mat3(to_world)));

		glUniformMatrix4fv(glGetUniformLocation(floor_shader().getProgram(), "to_world"), 1, GL_FALSE, &to_world[0][0]);

		glUniformMatrix3fv(glGetUniformLocation(floor_shader().getProgram(), "normal_to_world"), 1, GL_FALSE, &normal_to_world[0][0]);
		//glColor3f(float(i)/pieces.size(), fract(float(i*5)/pieces.size()), fract(float(i*7)/pieces.size()));
		mesh.vbo.drawElements(GL_TRIANGLES, mesh.indices.size());
		ofPopMatrix();
    }
    floor_shader().end();

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

float UltramarineWorld::getDuration(){
	return duration;
}

void UltramarineWorld::keyPressed(int key){
	switch(key){
		case 'u': do_updates=!do_updates; break;
		case 'g': draw_ground=!draw_ground; break;
		case 'p': gui->toggleVisible(); break;
	}
	World::keyPressed(key);
}
void UltramarineWorld::keyReleased(int key){
}
void UltramarineWorld::drawDebugOverlay(){
}

void UltramarineWorld::mouseMoved(int x, int y) {
	ofVec3f pos=camera.getPosition();
	ofVec3f dir=camera.screenToWorld(ofVec3f(x,y,-1.0))-pos;
	mouse_dir=glm::normalize(glm::vec3(dir.x, dir.y, dir.z));
}

BloomParameters UltramarineWorld::getBloomParams(){
	BloomParameters params;

	//blend world bloom parameters in so that inside the container there is no bloom

	float currentTime = Timeline::getInstance().getElapsedCurrWorldTime();
	float blendAlpha = glm::smoothstep(8.7f, 9.2f, currentTime);

	params.bloom_multiplier_color = glm::mix(glm::vec3(0.0), glm::vec3(BLOOM_MULTIPLIER_COLOR), blendAlpha);
	params.gamma = glm::mix(1.0f, GAMMA, blendAlpha);
	params.exposure = glm::mix(1.0f, EXPOSURE, blendAlpha);

	if (USE_ANTIALIASING) {
		params.antialiasing=(int)ANTIALIASING;
	} else {
		params.multisample_antialiasing=(int)MULTISAMPLE_ANTIALIASING;
	}
	params.crisp_antialiasing=CRISP_ANTIALIASING;

	#ifdef DO_CROSSFADE_BLOOM_PARAMS
	if(engine->getNextWorld()){
		BloomParameters params2=engine->getNextWorld()->getBloomParams();
		float fade=glm::clamp((WORLD_ULTRAMARINE::DURATION_MAIN-time)/bloom_crossfade_duration, 0.0f, 1.0f);
		if(fade<1.0f){
			params.bloom_multiplier_color=glm::mix(params2.bloom_multiplier_color, params.bloom_multiplier_color, fade);
			params.gamma=glm::mix(params2.gamma, params.gamma, fade);
			params.exposure=glm::mix(params2.exposure, params.exposure, fade);
		}
	}
	#endif

	//params.tonemap_method=1;
	return params;
}

void UltramarineWorld::drawEnvironmentAsBackground(){/// do not remove this! necessary for testing!
	glDisable(GL_BLEND);
    /// drawing background
    glDepthMask(GL_FALSE);
    glActiveTexture(GL_TEXTURE0);
    environment->BindTexture();
    glPushMatrix();
    glm::mat4x4 m;
    /// clear translation
    glGetFloatv(GL_MODELVIEW_MATRIX, &m[0][0]);
    m[3]=vec4(0,0,0,1);
    glLoadMatrixf(&m[0][0]);

    background_shader().begin();
    backgroundSphere->draw();
    background_shader().end();

    glPopMatrix();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDepthMask(GL_TRUE);
}

bool UltramarineWorld::TimeAt(float t){
	return t>=old_time && t<time;
}
