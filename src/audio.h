#ifndef AUDIO_H
#define AUDIO_H

#include "glm/glm.hpp"
#include "utils.h"

#include <vector>
#include "AL/alc.h"
#include "AL/al.h"
#include "AL/alext.h"

#include <string>
#include <set>

class AudioDataStream{
	public:
	virtual ~AudioDataStream(){};
	virtual int Format()=0;/// alenum
	virtual int Freq()=0;
	virtual bool OpenFile(const std::string &filename)=0;
	virtual bool Opened()=0;
	/// returns size actually read.
	virtual int Read(int buffer_size, void *buffer)=0;
};

void InitAudio(int argc, char **argv);

struct ALSource{
	//SoundSource *my_source;
	unsigned int id;
	bool in_use;
	ALSource():id(0),in_use(0){}
};

const int max_al_sources=128;
const int reserved_background_al_sources=4;/// 4 out of 128 sources are reserved for background sounds
int ostatni=35092;
const int audio_buffers_ahead=audio_stream_buffers_count-1;/// number of buffers which are ahead. Rest of the buffers are used for visualizer
const int max_buffers_per_frame=32;
const int audio_buffer_size_bytes=8192;

const int audio_visualizer_buffer_size=audio_stream_buffers_count*audio_buffer_size_bytes*4;/// currently i simply duplicate the data into continuous buffer that i control.

class AudioPlayerStream;
typedef StrongPtr<AudioPlayerStream> PAudioPlayerStream;
class AudioCaptureStream;
typedef StrongPtr<AudioCaptureStream> PAudioCaptureStream;

class ReadableAudioStream: public RefcountedContainer{/// class is useless by itself. derived classes must update the buffered data.
	public:
	
	
	int16_t visualizer_buffer[audio_visualizer_buffer_size];
	void WriteVisualizerBufferChunk(int64_t offset, int size, int16_t *data);
	inline float Value(int pos){
		return visualizer_buffer[pos&(audio_visualizer_buffer_size-1)]*(1.0/32768.0);
	}
};
typedef StrongPtr<ReadableAudioStream> PReadableAudioStream;

enum{
	sound_flag_repeat=1,
	sound_flag_background=2
};
//komentarz
class SoundObject{
	public:
		glm::vec3 pos;
		glm::vec3 vel;
		unsigned int al_buffer;
		int al_source;
		double playback_start;
		float sound_priority;
		int sound_flags;
		float sound_gain;
		float sound_pitch;
		unsigned int al_sound_filter_id;
		float lifetime;

		bool managed;/// if true, deleted on removal

		SoundObject(unsigned int buffer_=0, bool  repeat=0, bool background=0, const glm::vec3 &pos_=glm::vec3(0.0), const glm::vec3 &vel_=glm::vec3(0.0), float lifetime_=1.0E10, float sound_gain_=1.0, float sound_pitch_=1.0);
		SoundObject(const std::string &filename, bool repeat, bool background, const glm::vec3 &pos_=glm::vec3(0.0), const glm::vec3 &vel_=glm::vec3(0.0), float lifetime_=1.0E10, float sound_gain_=1.0, float sound_pitch_=1.0);
		~SoundObject();
};

class AudioManager{
	public:
	float fadeInTime, fadeStartTime, nextAmbientGain;
	SoundObject *currentAmbient, *nextAmbient;
	ALCdevice *alc_device;
	ALCcontext *alc_context;

	bool paused;
	bool startFading;
	double time;
	float globalGain;


	std::vector<std::string> capture_devices;
	std::string default_capture_device;
	std::string current_capture_device;

	PAudioPlayerStream music_streamer;

	//AudioAnalyzer music_analyzer;

	unsigned int reverb_filter_id;

	std::vector<ALSource> al_sources;

	unsigned int music_player_src_id;


	AudioManager();
	~AudioManager();
//	void AddSource(SoundSource *src);
//	void RemoveSource(SoundSource *src);
	//void SetListener(const math3::Vec3f &pos,const math3::Vec3f &vel);
	void Update(double dt, glm::vec3 listener_pos=glm::vec3(0.0), glm::vec3 listener_forward=glm::vec3(0,0,-1), glm::vec3 listener_up=glm::vec3(0,1,0), glm::vec3 listener_velocity=glm::vec3(0.0));

	void PauseAll();
	void ResumeAll();

	/// not the most efficient thing in the world, but definitely easiest.
	std::set<SoundObject *> sound_objects;

	void AddSoundObject(SoundObject *so);/// Note: AudioManager does not assume ownership of the pointer - the pointers will not be deleted by AudioManager
	void RemoveSoundObject(SoundObject *so);
	void RemoveAllSoundObjects();

	void FadeAmbientSounds();
	void StopAmbientSound();

	void setGlobalGain(float gain);
};
AudioManager &GlobAudioManager();

unsigned int GetALBuffer(const std::string &filename);

/// for fire-and-forget sources
void Play3DSound(const std::string &filename, bool repeat, bool background, const glm::vec3 &pos_=glm::vec3(0.0), const glm::vec3 &vel_=glm::vec3(0.0), float lifetime_=1.0E10, float sound_gain_=1.0, float sound_pitch_=1.0);
void PlayBackgroundSound(const std::string &filename, float sound_gain_=1.0);

void PlayAmbientSound(const std::string &filename, float gain, bool loop);


#endif // AUDIO_H
