#include "audio.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>

#include "ofUtils.h"

const bool has_audio=true;
const float fadeOver = 3.0;
static inline ALuint aluBytesFromFormat(ALenum format)
{
    switch(format)
    {
        case AL_FORMAT_MONO8:
        case AL_FORMAT_STEREO8:
        case AL_FORMAT_QUAD8_LOKI:
        case AL_FORMAT_QUAD8:
        case AL_FORMAT_51CHN8:
        case AL_FORMAT_61CHN8:
        case AL_FORMAT_71CHN8:
            return 1;

        case AL_FORMAT_MONO16:
        case AL_FORMAT_STEREO16:
        case AL_FORMAT_QUAD16_LOKI:
        case AL_FORMAT_QUAD16:
        case AL_FORMAT_51CHN16:
        case AL_FORMAT_61CHN16:
        case AL_FORMAT_71CHN16:
            return 2;

        case AL_FORMAT_MONO_FLOAT32:
        case AL_FORMAT_STEREO_FLOAT32:
        case AL_FORMAT_QUAD32:
        case AL_FORMAT_51CHN32:
        case AL_FORMAT_61CHN32:
        case AL_FORMAT_71CHN32:
            return 4;

        default:
            return 0;
    }
}
static inline ALuint aluChannelsFromFormat(ALenum format)
{
    switch(format)
    {
        case AL_FORMAT_MONO8:
        case AL_FORMAT_MONO16:
        case AL_FORMAT_MONO_FLOAT32:
            return 1;

        case AL_FORMAT_STEREO8:
        case AL_FORMAT_STEREO16:
        case AL_FORMAT_STEREO_FLOAT32:
            return 2;

        case AL_FORMAT_QUAD8_LOKI:
        case AL_FORMAT_QUAD16_LOKI:
        case AL_FORMAT_QUAD8:
        case AL_FORMAT_QUAD16:
        case AL_FORMAT_QUAD32:
            return 4;

        case AL_FORMAT_51CHN8:
        case AL_FORMAT_51CHN16:
        case AL_FORMAT_51CHN32:
            return 6;

        case AL_FORMAT_61CHN8:
        case AL_FORMAT_61CHN16:
        case AL_FORMAT_61CHN32:
            return 7;

        case AL_FORMAT_71CHN8:
        case AL_FORMAT_71CHN16:
        case AL_FORMAT_71CHN32:
            return 8;

        default:
            return 0;
    }
}

/// Apple does not have file loader in their alut .

#define ascii_uint32(a) (*((uint32_t *)(a)))

/*
0         4   ChunkID          Contains the letters "RIFF" in ASCII form
                               (0x52494646 big-endian form).
4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
                               4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                               This is the size of the rest of the chunk
                               following this number.  This is the size of the
                               entire file in bytes minus 8 bytes for the
                               two fields not included in this count:
                               ChunkID and ChunkSize.
8         4   Format           Contains the letters "WAVE"
                               (0x57415645 big-endian form).

The "WAVE" format consists of two subchunks: "fmt " and "data":
The "fmt " subchunk describes the sound data's format:

12        4   Subchunk1ID      Contains the letters "fmt "
                               (0x666d7420 big-endian form).
16        4   Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number.
20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some
                               form of compression.
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
32        2   BlockAlign       == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. I wonder what happens when
                               this number isn't an integer?
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
          2   ExtraParamSize   if PCM, then doesn't exist
          X   ExtraParams      space for extra parameters

The "data" subchunk contains the size of the data and the actual sound:

36        4   Subchunk2ID      Contains the letters "data"
                               (0x64617461 big-endian form).
40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this
                               number.
44        *   Data             The actual sound data.

*/

#pragma pack(push)
#pragma pack(1)


struct WavHeader_RIFF{
	uint32_t id_RIFF;
	uint32_t chunk_size;
	uint32_t id_WAVE;
};
struct WavHeaderChunk{
	uint32_t id;
	uint32_t size;
};
struct WavHeader_fmt{
	/*
	uint32_t id_fmt_;
	uint32_t subchunk_1_size;/// pcm = 16
	*/
	uint16_t audio_format;/// pcm = 1
	uint16_t num_channels;/// mono = 1 stereo = 2
	uint32_t sample_rate;
	uint32_t byte_rate;/// = SampleRate * NumChannels * BitsPerSample/8
	uint16_t block_align;/// = NumChannels * BitsPerSample/8
	uint16_t bits_per_sample;/// = NumChannels * BitsPerSample/8
};

/*
struct WavHeaderPCM{
	uint32_t id_RIFF;
	uint32_t chunk_size;
	uint32_t id_WAVE;

	uint32_t id_fmt_;
	uint32_t subchunk_1_size;/// pcm = 16
	uint16_t audio_format;/// pcm = 1
	uint16_t num_channels;/// mono = 1 stereo = 2
	uint32_t sample_rate;
	uint32_t byte_rate;/// = SampleRate * NumChannels * BitsPerSample/8
	uint16_t block_align;/// = NumChannels * BitsPerSample/8
	uint16_t bits_per_sample;/// = NumChannels * BitsPerSample/8

	uint32_t id_data;///
	uint32_t subchunk_2_size; ///  = NumSamples * NumChannels * BitsPerSample/8
};*/
#pragma pack(pop)

#include <string.h>

//#include <glhelper/utf8_fstream.h>

bool LoadWavToOpenALBuffer(const char *filename, ALuint buffer){
	if(!has_audio || !buffer)return false;
	my_logger<<"loading wav file "<<filename<<std::endl;

	#define fatal(msg) {std::cerr<<"error loading '"<<filename<<"': "<<msg<<std::endl; return 0;}

	WavHeader_RIFF header_RIFF;
	WavHeaderChunk header;
	WavHeader_fmt header_fmt;
	memset(&header_RIFF, 0, sizeof(header_RIFF));
	memset(&header, 0, sizeof(header));
	memset(&header_fmt, 0, sizeof(header_fmt));

	//my_std::utf8_ifstream f(filename, std::ifstream::binary);
	std::ifstream f(filename, std::ifstream::binary);
	//std::fstream f(test);

	if(!f.good()){
		fatal("can't open file");
	}
	f.seekg(0, std::ios::end);
	size_t file_size = f.tellg();
	if(file_size<sizeof(header_RIFF)){
		fatal("file too small");
	}
	f.seekg(0);

	f.read((char*)&header_RIFF, sizeof(header_RIFF));

	if(header_RIFF.id_RIFF!=ascii_uint32("RIFF")){
		fatal("no 'RIFF'");
	}
	if(header_RIFF.id_WAVE!=ascii_uint32("WAVE")){
		fatal("no 'WAVE'");
	}

	do{
		f.read((char*)&header, sizeof(header));
		if(!f.good())break;
		if(header.id==ascii_uint32("fmt "))break;
		f.seekg(header.size, std::ios_base::cur);
	}while(f.good());

	if(header.id!=ascii_uint32("fmt ")){
		fatal("no 'fmt '");
	}
	if(header.size<sizeof(WavHeader_fmt)){
		fatal("fmt chunk is too short");
	}
	int pos=f.tellg();
	f.read((char*)&header_fmt, sizeof(WavHeader_fmt));
	//skip any extra bytes
	f.seekg(pos+header.size);

	if(header_fmt.audio_format!=1){
		fatal("not PCM");
	}
	ALenum al_formats[4]={AL_FORMAT_MONO8, AL_FORMAT_MONO16, AL_FORMAT_STEREO8, AL_FORMAT_STEREO16};
	int fmt=0;
	switch(header_fmt.bits_per_sample){
		case 8:	break;
		case 16:
		case 24: fmt|=1; break;
		default:
		fatal("must use 8, 16, or 24 bits per sample (uses "<<header_fmt.bits_per_sample<<")");
	}
	switch(header_fmt.num_channels){
		case 1: break;
		case 2: fmt|=2; break;
		default:
		fatal("must use 1 or 2 channels");
	}

	do{
		f.read((char*)&header, sizeof(header));
		if(!f.good())break;
		if(header.id==ascii_uint32("data"))break;
		f.seekg(header.size, std::ios_base::cur);
	}while(f.good());
	if(header.id!=ascii_uint32("data")){
		fatal("no 'data'");
	}
	/// do not crash on wav files with incorrect size
	uint32_t data_size=std::min(header.size, uint32_t(file_size-f.tellg()));

	/// make size be multipe of sample size
	int sample_size_in_bytes=(header_fmt.bits_per_sample*header_fmt.num_channels)/8;
	data_size/=sample_size_in_bytes;
	data_size*=sample_size_in_bytes;

	std::vector<char> wavdata(data_size);
	f.read(&wavdata[0],wavdata.size());

	my_logger<<"fmt:"<<fmt<<" size:"<<wavdata.size()<<std::endl;
	alGetError();
	if(header_fmt.bits_per_sample==24){
		std::vector<char> wavdata_converted((data_size*2)/3);
		int j=0;
		for(int i=0; i<wavdata.size(); i+=3){
			wavdata_converted[j]=wavdata[i+1];
			wavdata_converted[j+1]=wavdata[i+2];
			j+=2;
		}
		alBufferData(buffer, al_formats[fmt], (void *)(&wavdata_converted[0]), wavdata_converted.size(), header_fmt.sample_rate);
	}else{
		alBufferData(buffer, al_formats[fmt], (void *)(&wavdata[0]), wavdata.size(), header_fmt.sample_rate);
	}
	my_logger<<"al error= "<<alGetError()<<std::endl;

	#undef fatal
	return true;
};

AudioDataStream *OpenAudioFile(const std::string &name);

bool LoadToOpenALBuffer(const char *filename, ALuint buffer){
	std::string ext=GetFileNameExtLC(filename);
	if(ext=="wav")return LoadWavToOpenALBuffer(filename, buffer);

	if(!has_audio || !buffer)return false;
	my_logger<<"Loading OGG file at once: "<<filename<<std::endl;
	AudioDataStream *stream=OpenAudioFile(filename);
	if(!stream) return false;
	std::vector<char> data;
	int chunk_size=8192;
	int rd_size=0;
	int total_size=0;
	do{
		data.resize(total_size+chunk_size);
		rd_size=stream->Read(chunk_size, (void*)(&data[total_size]));
		total_size+=rd_size;
	}while(rd_size>0);
	alBufferData(buffer, stream->Format(), (void *)(&data[0]), total_size, stream->Freq());
	delete stream;

	return true;
};

bool no_sound=false;

void InitAudio(int argc, char **argv){
	for(int i=1;i<argc;++i){
		if(std::string(argv[i])=="--nosound"){
			no_sound=true;
		}
	}
}

/*

void Beep(){
	if(!has_audio)return;
	ALuint source;
	// Position of the source sound.
	ALfloat SourcePos[] = { 0.0, 0.0, 0.0 };

	// Velocity of the source sound.
	ALfloat SourceVel[] = { 0.0, 0.0, 0.0 };


	// Position of the listener.
	ALfloat ListenerPos[] = { 0.0, 0.0, 0.0 };

	// Velocity of the listener.
	ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };

	// Orientation of the listener. (first 3 elements are "at", second 3 are "up")
	ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };

	alGenSources(1, &source);

	alSourcei (source, AL_BUFFER,  TestBeepBuffer()  );
	alSourcef (source, AL_PITCH,    1.0f     );
	alSourcef (source, AL_GAIN,     1.0f     );
	alSourcefv(source, AL_POSITION, SourcePos);
	alSourcefv(source, AL_VELOCITY, SourceVel);
	alSourcei (source, AL_LOOPING,  1    );

	if (alGetError() != AL_NO_ERROR){
	 	std::cerr<<"OpenAL error"<<std::endl;
	}
	alSourcePlay(source);
}; */


class ALBuffer{
	public:
	ALuint buffer;
	ALBuffer():
	buffer(0)
	{
		if(has_audio){
			alGetError();
			alGenBuffers(1, &buffer);
			if(alGetError()!=AL_NO_ERROR){
				my_logger<<"Failed to create AL buffer"<<std::endl;
				buffer=0;
			}
		}
	}
	~ALBuffer(){
		alDeleteBuffers(1, &buffer);
	}
};

class TestBeepBufferData: public ALBuffer{
	public:
	TestBeepBufferData(){
		if(!buffer)return;

		const int loop_length=44100;
		int16_t data[loop_length];
		double freq0=0.02;
		double freq1=0.05;
		double mod_freq=30.0/44100.0;


		double r=0;

		for(int i=0;i<loop_length;++i){
			/// frequency rises from f0 to f1
			/// f(i)=freq0+i*(freq1-freq0)/loop_length
			/// phase+=f(i); on each step.
			/// phase=freq0*i+0.5*i*i*(freq1-freq0)/loop_length
			//double r=freq0*i + 0.5*double(i)*double(i)*(freq1-freq0)/loop_length;
			r+=(0.5*(freq0+freq1)-0.5*(freq1-freq0)*cos(2*3.1415826*float(i)*mod_freq));

			data[i]=2000*sin(r*2*3.1415826);//32766.0*sin(alpha);
		}
		alBufferData(buffer,AL_FORMAT_MONO16, (void *)data, loop_length*2, 44100);
	}
};

unsigned int TestBeepBuffer(){
	static TestBeepBufferData data;
	return data.buffer;
}

class WavBufferData: public ALBuffer{
	public:
	explicit WavBufferData(const std::string &filename){
		if(!buffer)return;
		LoadWavToOpenALBuffer(filename.c_str(), buffer);
	}
};

class BuffersStore{
	public:
	std::map<std::string, ALuint> buffers;

	~BuffersStore(){
		for(std::map<std::string, ALuint>::iterator i=buffers.begin(); i!=buffers.end(); ++i){
			alDeleteBuffers(1,&(i->second));
		}
	}
	unsigned int GetBuffer(const std::string &filename){
		std::map<std::string, ALuint>::iterator i=buffers.find(filename);
		if(i!=buffers.end())return i->second;
		ALuint buffer;
		alGenBuffers(1,&buffer);
		buffers[filename]=buffer;
		#if(USE_ALURE)
		alureBufferDataFromFile(GlobVFS().TranslateName(filename).c_str(), buffer);
		#else
		//LoadWavToOpenALBuffer(GlobVFS().TranslateName(filename).c_str(), buffer);
		LoadToOpenALBuffer(filename.c_str(), buffer);
		#endif
		//std::cout<<"GetBuffer: returning "<<buffer<<std::endl;

		return buffer;
	}
};

unsigned int GetALBuffer(const std::string &filename){
	GlobAudioManager();
	static BuffersStore *store=new BuffersStore();
	return store->GetBuffer(ofToDataPath(filename));
}

class OGGDataStream: public AudioDataStream{
	FILE *file;
	OggVorbis_File stream;
	vorbis_info *info;
	vorbis_comment *comment;
	ALenum format;
	bool eof;
	public:
	OGGDataStream(): file(NULL), info(NULL), comment(NULL), format(0), eof(false){
	}
	virtual ~OGGDataStream(){
		if(file){
			ov_clear(&stream);
		}
	}
	virtual ALenum Format(){
		return format;
	}
	virtual int Freq(){
		return info?info->rate:0;
	}

	virtual bool OpenFile(const std::string &filename){
		my_logger<<"OGGDataStream::OpenFile "<<filename<<std::endl;

		file=fopen(filename.c_str(), "rb");//utf8_fopen(filename.c_str(), "rb");

		if(!file){
			my_logger<<"Can't open file"<<std::endl;
			return false;
		}
		my_logger<<"ov_open_callbacks"<<std::endl;
		//int result=ov_open(file, &stream, NULL, 0);
		int result=ov_open_callbacks(file, &stream, 0, 0, OV_CALLBACKS_DEFAULT);

		if(result<0){
			fclose(file);
			file=NULL;
			return false;
		}
		my_logger<<"ov_info"<<std::endl;
		info=ov_info(&stream, -1);
		my_logger<<"ov_comment"<<std::endl;
		comment=ov_comment(&stream, -1);
		if(info->channels==1){
			format=AL_FORMAT_MONO16;
			my_logger<<"Mono OGG file"<<std::endl;
		}else
		if(info->channels==2){
			format=AL_FORMAT_STEREO16;
			my_logger<<"Stereo OGG file"<<std::endl;
		}else
		{
			my_logger<<"Invalid number of channels in the ogg file! The file will not play correctly"<<std::endl;
			format=AL_FORMAT_STEREO16;
		}
		my_logger<<"OGGDataStream::OpenFile success "<<filename<<std::endl;
		return true;
	}
	virtual bool Opened(){
		return file && !eof;
	}
	/// returns size actually read. limited to 2gb
	virtual int Read(int buffer_size, void *buffer){
		int bitstream;
		size_t result=ov_read(&stream, (char*)buffer, buffer_size, 0 /*NOTE: endianness*/, 2 /*2 bytes per sample*/, 1, &bitstream);
		if(result<0 || result>buffer_size){
			switch(result){
				case OV_HOLE:
				my_logger<<"OGG error: OV_HOLE"<<std::endl;
				break;
				case OV_EBADLINK:
				my_logger<<"OGG error: OV_EBADLINK"<<std::endl;
				break;
			}
			result=0;
		}
		if(!result)eof=true;
		return result;
	}
};


AudioDataStream *OpenAudioFile(const std::string &name){
	AudioDataStream *result=NULL;
	std::string ext=GetFileNameExtLC(name);
	if(ext=="ogg"){
		result=new OGGDataStream;
	}
#ifndef LIMITED_EDITION
#if USE_MPG123
	else
	if(has_mpg123 && ext=="mp3"){
		result=new mpg123DataStream;
	}
#endif
#if USE_DIRECTSHOW
	else
	{
		return DS_OpenMediaFile(name);
	}
#endif
#ifdef __APPLE__
	else
	{
		return QUT_OpenMediaFile(name);
	}
#endif
#endif
	if(result){
		bool b=result->OpenFile(name);
		if(!b){
			delete result;
			result=NULL;
		}
	}
	return result;
}


void ReadableAudioStream::WriteVisualizerBufferChunk(int64_t offset, int size, int16_t *data){
	assert(size<=audio_visualizer_buffer_size);
	int buf_offset=offset&(audio_visualizer_buffer_size-1);
	int sz=std::min(size, audio_visualizer_buffer_size-buf_offset);
	memcpy(visualizer_buffer+buf_offset, data, sz*sizeof(data[0]));
	if(sz<size){
		memcpy(visualizer_buffer, data+sz, (size-sz)*sizeof(data[0]));
	}
}

class AudioPlayerStream: public ReadableAudioStream{
	public:
	AudioDataStream *m_stream;
	ALuint m_src;
	bool m_queued;
	bool file_set;
	float amplitude;
	float smooth_amplitude;
	int64_t last_amplitude_update_offset;

	int16_t chunk[audio_buffer_size_bytes/2];

	//std::vector<std::string> files;

	//int now_playing;
	std::string current_file;
	bool finished_reading_current_file;

	struct buffer_info{
		int64_t data_offset;
		int size;// size of data in shorts
		//int16_t *data;
		//float *data;
		ALuint al_buffer;
		buffer_info(): data_offset(0), size(0), /* data(0), */ al_buffer(0)
		{
		}
	};
	buffer_info buffer_infos[audio_stream_buffers_count];
	int current_buffer_i;
	int write_buffer_i;
	int64_t data_offset;

	ALenum format;

	AudioPlayerStream():
	m_stream(NULL),
	m_src(0),
	m_queued(0),
	file_set(0),
	amplitude(0),
	smooth_amplitude(0),
	last_amplitude_update_offset(0),
	finished_reading_current_file(true)
	{
		CreateBuffers();
	}

	void ResetQueue(){
		current_buffer_i=0;
		write_buffer_i=0;
		data_offset=0;
		format=0;
		freq=0;
		channels=0;
		memset(visualizer_buffer,0,sizeof(visualizer_buffer));
		current_offset=0;
	}
	void CreateBuffers(){
		ResetQueue();
		ALuint buffers[audio_stream_buffers_count];
		alGenBuffers(audio_stream_buffers_count, buffers);
		for(int i=0;i<audio_stream_buffers_count;++i){
			buffer_infos[i].al_buffer=buffers[i];
			buffer_infos[i].size=0;
			/* buffer_infos[i].data=NULL; */
			buffer_infos[i].data_offset=0;
		}
	}
	void DestroyBuffers(){
		ALuint buffers[audio_stream_buffers_count];
		for(int i=0;i<audio_stream_buffers_count;++i){
			buffers[i]=buffer_infos[i].al_buffer;
			buffer_infos[i].al_buffer=0;
		}
		alDeleteBuffers(audio_stream_buffers_count, buffers);
	}


	bool LoadBuffer(){
		//double t0=glfwGetTime();

		ALuint buf=0;
		if(m_src){
			buf=buffer_infos[write_buffer_i].al_buffer;
		}


		int iter=0;
		if(!m_stream)return false;

		int bytes_read=m_stream->Read(audio_buffer_size_bytes, (void*)chunk);
		if(!bytes_read){
			finished_reading_current_file=true;
			return false;
		}
		if(m_src){
			alBufferData(buf, m_stream->Format(), (void*)chunk, bytes_read, m_stream->Freq());
			alSourceQueueBuffers(m_src,1,&buf);
		}

		int size=bytes_read/sizeof(int16_t);
		channels=aluChannelsFromFormat(m_stream->Format());

		WriteVisualizerBufferChunk(data_offset, size, chunk);

		buffer_infos[write_buffer_i].data_offset=data_offset;
		buffer_infos[write_buffer_i].al_buffer=buf;
		data_offset+=size;
		write_buffer_i+=1;
		write_buffer_i&=audio_stream_buffers_count-1;
		return true;
	}

	int BuffersAhead(){
		return (write_buffer_i-current_buffer_i)&(audio_stream_buffers_count-1);

	}

	void UpdateBuffers(){
		//double t0=glfwGetTime();
		if(!m_stream)return;
		ALint state;
		alGetSourcei(m_src, AL_SOURCE_STATE, &state);
		ALint processed=0;
		alGetSourcei(m_src, AL_BUFFERS_PROCESSED, &processed);
		ALint cur_buf;
		ALuint temp[audio_stream_buffers_count];
		alSourceUnqueueBuffers(m_src, processed, temp);
		current_buffer_i+=processed;
		current_buffer_i&=audio_stream_buffers_count-1;
		if(state==AL_PLAYING){
			alGetSourcei(m_src, AL_BUFFER, &cur_buf);
			if(cur_buf!=buffer_infos[current_buffer_i].al_buffer){
				//my_logger<<"audio buffering bug - visualizer will desync!"<<std::endl;
				my_logger<<"audio buffering bug, current_buffer_i="<<current_buffer_i<<std::endl;
				int i=0;
				while(i<audio_stream_buffers_count){
					if(cur_buf==buffer_infos[i].al_buffer){
						my_logger<<"should be ="<<i<<std::endl;
						current_buffer_i=i;
						break;
					}
					i++;
				}
			}
			ALint sample_offset;
			alGetSourcei(m_src,AL_SAMPLE_OFFSET, &sample_offset);
			int64_t new_offset=buffer_infos[current_buffer_i].data_offset+sample_offset*channels;

			if(new_offset<current_offset){
				my_logger<<"offset glitch "<<current_offset<<" to "<<new_offset<<std::endl;
			}
			current_offset=new_offset;
		}
		int buffers_to_load=max_buffers_per_frame;
		while(BuffersAhead()<audio_buffers_ahead && buffers_to_load){
			if(!LoadBuffer())break;
			buffers_to_load--;
		}
		if(state != AL_PLAYING){
			if(BuffersAhead()){
				alSourcePlay(m_src);
			}
		}
		//double t1=glfwGetTime();
		//my_logger<<"buffers update time: "<<t1-t0<<std::endl;
	}

	void SetSource(ALuint src){
		m_src=src;
	}
	bool SetFile(const std::string &filename_){
		if(m_stream){
			delete m_stream;
			m_stream=NULL;
		}
		finished_reading_current_file=true;
		current_file=filename_;

		if(filename_.empty()){
			return true;
		}

		//std::string filename=GlobVFS().TranslateName(filename_);

		m_stream=OpenAudioFile(filename_.c_str());
		if(!m_stream){
			return false;
		}
		freq=m_stream->Freq();
		//ResetQueue();
		//alSourceStop(m_src);
		finished_reading_current_file=false;
		//double t1=glfwGetTime();
		//my_logger<<"audio file opening time: "<<t1-t0<<std::endl;
		return true;
	}

	void UpdateAmplitude(){
		if(!m_stream)return;
		if(current_offset>last_amplitude_update_offset+441){/// at most 100 updates per second for sample rate 44100 hz
			int i=last_amplitude_update_offset&(audio_visualizer_buffer_size-1);
			int end=current_offset&(audio_visualizer_buffer_size-1);
			double ampsum=0;
			int cnt=0;
			while(i!=end){
				float v=visualizer_buffer[i];
				v*=1.0/32768.0;
				ampsum+=v*v;
				i=(i+1)&(audio_visualizer_buffer_size-1);
				cnt++;
			}
			amplitude=ampsum/cnt;
			float a=0.05;
			smooth_amplitude=smooth_amplitude*(1.0-a)+amplitude*a;
			last_amplitude_update_offset=current_offset;
		}
	}


	void Update(double dt){
		/*
		if(!m_src)return;
		ALint state;
		alGetSourcei(m_src, AL_SOURCE_STATE, &state);
		std::string cf(settings::current_music_file);
		if(cf!=current_file || settings::restart_music_track){
			settings::restart_music_track=false;
			alSourceStop(m_src);
			ResetQueue();
			if(!SetFile(cf)){
				my_logger<<"Cannot play "<<cf<<std::endl;
				if(GlobPlaylist())GlobPlaylist()->FileError();
			}
		}
		if(finished_reading_current_file && BuffersAhead()==0 && state!=AL_PLAYING){/// necessary for looping single file
			if(GlobPlaylist())GlobPlaylist()->NextFile();
			std::string cf=settings::current_music_file;
			if(!SetFile(cf)){
				my_logger<<"Cannot play "<<cf<<std::endl;
				if(GlobPlaylist())GlobPlaylist()->FileError();
			}
		}

		UpdateBuffers();
		UpdateAmplitude();
		*/
	}
};


AudioManager::AudioManager():
fadeInTime(0),
fadeStartTime(0.0),
nextAmbientGain(0.0),
time(0),
globalGain(1.0),
currentAmbient(NULL),
nextAmbient(NULL),
alc_device(NULL),
reverb_filter_id(AL_EFFECTSLOT_NULL),
paused(false),
startFading(false),
music_player_src_id(0)
{
	if(no_sound)return;
	my_logger<<"AudioManager::AudioManager()"<<std::endl;

	#if(USE_ALURE)
	if(!alureInitDevice(0, 0))
	{
		std::cout<<"Failure to open audio device: "<<alureGetErrorString()<<std::endl;
		return;
	}
	#else
	alGetError();

	const ALCchar *s=alcGetString(NULL, ALC_DEVICE_SPECIFIER);
	if(!s){
		my_logger<<"[null pointer from alcGetString]"<<std::endl;
	}
	my_logger<<"OpenAL devices : { "<<std::endl;
	if(s){
		while(*s){
			std::string dev;
			while(*s){
				dev+=*s;
				s++;
			}
			my_logger<<" '"<<dev<<"'"<<std::endl;
			s++;
		}
	}
	my_logger<<"}"<<std::endl;
	default_capture_device=alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	my_logger<<"OpenAL default device: "<<default_capture_device<<std::endl;

	my_logger<<"Opening default device"<<std::endl;
	alc_device=alcOpenDevice(0);

	if(!alc_device){
		my_logger<<"Failed to open device"<<std::endl;
		return;
	}

	alc_context=alcCreateContext(alc_device, 0);
	if(!alc_context){
		return;
	}
	alcMakeContextCurrent(alc_context);
	int err=alGetError();
	if(err!=AL_NO_ERROR){
		my_logger<<"Failed to set current context"<<std::endl;
	}
	#endif

	alGetError();
	my_logger<<"OpenAL: version "<<alGetString(AL_VERSION)<<" renderer "<<alGetString(AL_RENDERER)<<" vendor "<<alGetString(AL_VENDOR)<<std::endl;

	//has_efx=InitEFX();
	bool has_efx=false;/// not using effects


	//has_audio=true;



	#if USE_MPG123

	InitMPG123();

	#endif

	//std::vector<ALuint> tmp(max_al_sources,0);
	//alGenSources(max_al_sources, &tmp[0]);
	ALuint envReverb, envEcho, envSlots[2];

/*
	if(has_efx){
		alGetError();
		alGenEffects(1, &envReverb);
		if(alGetError()==AL_NO_ERROR)
		{
			alEffecti(envReverb, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
			if(alGetError()==AL_NO_ERROR){
				my_logger<<"creating reverb"<<std::endl;
				alEffectf(envReverb, AL_REVERB_DENSITY, 0.6);

				alEffectf(envReverb, AL_REVERB_DIFFUSION, 0.5);

				alEffectf(envReverb, AL_REVERB_GAIN, 2.0);
				alEffectf(envReverb, AL_REVERB_GAINHF, 0.9);

				alEffectf(envReverb, AL_REVERB_DECAY_TIME, 4.0);
				alEffectf(envReverb, AL_REVERB_DECAY_HFRATIO, 0.1);

				alEffectf(envReverb, AL_REVERB_REFLECTIONS_GAIN, 0.95);
				alEffectf(envReverb, AL_REVERB_REFLECTIONS_DELAY, 0.01);

				alEffectf(envReverb, AL_REVERB_LATE_REVERB_GAIN, 2.0);
				alEffectf(envReverb, AL_REVERB_LATE_REVERB_DELAY, 0.3);






				// alGenEffects(1, &envEcho);
				// alEffectf(envEcho, AL_EFFECT_TYPE, AL_EFFECT_ECHO);

				alGenAuxiliaryEffectSlots(1, envSlots);
				alAuxiliaryEffectSloti(envSlots[0], AL_EFFECTSLOT_EFFECT, envReverb);

				//alAuxiliaryEffectSloti(envSlots[1], AL_EFFECTSLOT_EFFECT, envEcho);
				reverb_filter_id=envSlots[0];
			}
		}else{
		}
	}

	*/

// Then for each source you want to be affected:

	al_sources.resize(max_al_sources);
	alGetError();

	alGenSources(1, &music_player_src_id);
	// alSourcei(music_player_src_id, AL_SOURCE_RELATIVE, AL_TRUE);

	err=alGetError();
	if(err!=AL_FALSE){
			std::cerr<<"OpenAL error "<<err<<" making music source"<<std::endl;
	}
	for(int i=0;i<max_al_sources;++i){
		ALSource &src=al_sources[i];
		//src.my_source=NULL;
		alGenSources(1, &src.id);
		int err=alGetError();
		if(err!=AL_FALSE){
			std::cerr<<"OpenAL error "<<err<<" making source #"<<i<<std::endl;
			al_sources.resize(i);
			break;
		}
		alSourcei(src.id, AL_LOOPING, 1);
		if(has_efx){
			alSource3i(src.id, AL_AUXILIARY_SEND_FILTER, envSlots[0], 0, AL_FILTER_NULL);
			//alSource3i(src.id, AL_AUXILIARY_SEND_FILTER, envSlots[1], 0, AL_FILTER_NULL);
		}
	}

	music_streamer=new AudioPlayerStream();
	music_streamer->SetSource(music_player_src_id);

	//music_streamer->SetFile(settings::music_file /* "/home/dmytry/Music/Aphex Twin/[2005] Analord/Analord EP11/B1. VBS.Redlof.B.mp3" */);
	/*
	std::string music_folder="$$data/music/";
	DirectoryReader dr(GlobVFS().TranslateName(music_folder));
	for(std::string name=dr.ReadName();!name.empty();name=dr.ReadName()){
		std::string ext=GetFileNameExtLC(name);
		if(ext=="ogg" || ext=="mp3"){
			music_streamer->files.push_back(music_folder+name);
		}
	};
	std::sort(music_streamer->files.begin(),music_streamer->files.end());
	*/

	//music_analyzer.SetStream(PReadableAudioStream(music_streamer));

	//alSpeedOfSound(100);
	alSpeedOfSound(340);

}
AudioManager::~AudioManager(){
	std::cerr<<"AudioManager::~AudioManager()"<<std::endl;

	alcMakeContextCurrent(NULL);
	if(alc_context)alcDestroyContext(alc_context);
	if(alc_device)alcCloseDevice(alc_device);

	#if(USE_ALURE)
	alureShutdownDevice();
	#endif
}

void AudioManager::PauseAll(){
	if(!has_audio)return;
	if(paused)return;
	paused=true;
	for(int i=0;i<al_sources.size();++i){
		alSourcePause(al_sources[i].id);
	}
}

void AudioManager::ResumeAll(){
	if(!has_audio)return;
	if(!paused)return;
	paused=false;
	for(int i=0;i<al_sources.size();++i){
		if(al_sources[i].in_use)alSourcePlay(al_sources[i].id);
	}
}

/*
void AudioManager::AddSource(SoundSource *src){
	if(!src)return;
	src->ConnectPrevTo(sources_list);
}
void AudioManager::RemoveSource(SoundSource *src){
}
void AudioManager::SetListener(const math3::Vec3f &pos,const math3::Vec3f &vel){
}
*/
void AudioManager::Update(double dt, glm::vec3 listener_pos, glm::vec3 listener_forward, glm::vec3 listener_up, glm::vec3 listener_velocity){
	alListenerf(AL_GAIN, globalGain);

	if(paused){
		return;
	}

	time+=dt;

	if(startFading)
	{
		FadeAmbientSounds();
	}

	if(music_streamer.ok() ){
		music_streamer->Update(dt);

		float music_gain=1.0;

		alSourcef(music_streamer->m_src, AL_GAIN, music_gain  );
	}

	/// update sound objects

	{
		alListenerfv(AL_POSITION, (float*)&listener_pos);
		alListenerfv(AL_VELOCITY, (float*)&listener_velocity);
		alSourcefv(music_player_src_id, AL_POSITION, (float*)&listener_pos);
		alSourcefv(music_player_src_id, AL_VELOCITY, (float*)&listener_velocity);

		float ori[6];
		//Matrix3x3f m=QuaternionToMatrix(me->cam.cur_transform.orientation);
		ori[0]=listener_forward.x;
		ori[1]=listener_forward.y;
		ori[2]=listener_forward.z;
		ori[3]=listener_up.x;
		ori[4]=listener_up.y;
		ori[5]=listener_up.z;
		//std::cout<<"sound listener forward:"<<ori[0]<<","<<ori[1]<<","<<ori[2]<<std::endl;
		//std::cout<<"sound listener up:"<<ori[3]<<","<<ori[4]<<","<<ori[5]<<std::endl;
		alListenerfv(AL_ORIENTATION, ori);
	}

	int j=0;
	for(auto src_i=sound_objects.begin(); src_i!=sound_objects.end(); ++src_i){
		SoundObject &obj=**src_i;
		obj.lifetime-=dt;
		if(obj.sound_priority<=0 || obj.lifetime<=0){/// shut up sources with priority <=0 and expired sources
			//std::cout<<"expired sound object"<<std::endl;
			if(obj.al_source>=0){
				al_sources.at(obj.al_source).in_use=false;
				ALuint s=al_sources.at(obj.al_source).id;
				alSourceStop(s);
				obj.al_source=-1;
			}
			continue;
		}

		int si=obj.al_source;
		ALuint al_s_id=0;
		int max_source_n=(obj.sound_flags&sound_flag_background)?al_sources.size():(al_sources.size()-reserved_background_al_sources);

		if(si<0){/// assign unused source to the object
			while(j<max_source_n && al_sources[j].in_use)j++;
			if(j>=max_source_n){
				std::cerr<<"No unused AL sources to assign!"<<std::endl;
				continue;
			}
			si=j;
			obj.al_source=j;
			al_s_id=al_sources[j].id;
			//al_sources[j].in_use=true;
			alSourcei(al_s_id, AL_BUFFER, obj.al_buffer);
			//std::cout<<"assigned buffer "<<obj.al_buffer<<" to sound source "<<j<<" with offset"<<time-obj.playback_start<<std::endl;
			alSourcef(al_s_id, AL_SEC_OFFSET, time-obj.playback_start);
		}else{
			al_s_id=al_sources[si].id;
		}
		glm::vec3 pos=obj.pos;
		glm::vec3 vel=obj.vel;

		alSourcei(al_s_id, AL_SOURCE_RELATIVE, (obj.sound_flags&sound_flag_background) ? AL_TRUE: AL_FALSE);
		alSourcefv(al_s_id, AL_POSITION, (float*)&pos);
		alSourcefv(al_s_id, AL_VELOCITY, (float*)&vel);

		alSourcef (al_s_id, AL_PITCH,    obj.sound_pitch     );
		alSourcef (al_s_id, AL_GAIN,     obj.sound_gain);
		alSourcei (al_s_id, AL_LOOPING,  (obj.sound_flags&sound_flag_repeat) ? AL_TRUE: AL_FALSE );

/*
		if(HasEFX()){
			alSource3i(al_s_id, AL_AUXILIARY_SEND_FILTER, sound_srcs[i]->al_sound_filter_id, 0, AL_FILTER_NULL);
		}
*/

		if(!al_sources[si].in_use){
			al_sources[si].in_use=true;
			alSourcePlay(al_s_id);
		}
		ALint state;
		alGetSourcei(al_s_id, AL_SOURCE_STATE, &state);
		if(state==AL_STOPPED){/// expire object that's done playing
			//my_logger<<"removing stopped sound object with buffer"<<obj.al_buffer<<std::endl;
			obj.lifetime=-1;
			al_sources[si].in_use=false;
		}
	}

	/// remove dead sources
	auto src_i=sound_objects.begin();
	while(src_i!=sound_objects.end()){
		if((*src_i)->lifetime<=0.0){
			//my_logger<<"removing dead sound source"<<std::endl;

			/// if it is managed, delete it
			if((*src_i)->managed)delete *src_i;

			sound_objects.erase(src_i++);
		}else{
			++src_i;
		}
	}
}
AudioManager &GlobAudioManager(){
	static AudioManager *mgr=new AudioManager;
	return *mgr;
}

void AudioManager::AddSoundObject(SoundObject *so){
	if(!so)return;
	if(so->playback_start==0.0)so->playback_start=time;
	sound_objects.insert(so);
}
void AudioManager::RemoveSoundObject(SoundObject *so){
	if(!so)return;
	if(so->al_source>=0){
		al_sources[so->al_source].in_use=false;
		ALuint s=al_sources.at(so->al_source).id;
		alSourceStop(s);
		so->al_source=-1;
	}
	sound_objects.erase(so);
}

void AudioManager::RemoveAllSoundObjects() {
	cout << "AudioManager: Removing all sound objects..." << endl;
	std::set<SoundObject *>::iterator so = sound_objects.begin();
	while (so != sound_objects.end()) {
		SoundObject *s = *so;
		so++;
		RemoveSoundObject(s);
		if(s->managed)delete s;
	}
	cout << "AudioManager: Sound objects removed." << endl;
}

void AudioManager::FadeAmbientSounds()
{
	float ellapsedTime = time - fadeStartTime;
	//Linear
	//float kParam = 1.0 - (ellapsedTime / fadeOver);
	// Logarithmic
	//float kParam = std::log10f(fadeOver / (fadeOver + 9 * ellapsedTime)) + 1.0;
	// Exponential
	float kParam = -
	#ifndef __GNUC__
	std::powf
	#else
	std::pow
	#endif

	(10.0, (ellapsedTime * 1.04 - fadeOver)/fadeOver) + 1.09;
	float value = kParam * currentAmbient->sound_gain;

	currentAmbient->sound_gain = value;
	nextAmbient->sound_gain = ofClamp(nextAmbientGain - value, 0.0, nextAmbientGain);
	//std::cout<<"Current: "<<currentAmbient->sound_gain<<" next " << nextAmbient->sound_gain<<std::endl;
	if(value <= 0.001)
	{
		startFading = false;
		//remove current ambient
		//RemoveSoundObject(currentAmbient);
		delete currentAmbient;/// this will automatically remove the sound object
		currentAmbient = nextAmbient;
		GlobAudioManager().AddSoundObject(currentAmbient);
		nextAmbient = NULL;
	}
}

void AudioManager::StopAmbientSound()
{
	if(currentAmbient != NULL)
	{
		currentAmbient->sound_gain = 0.0;
		currentAmbient = NULL;
	}
}

void AudioManager::setGlobalGain(float gain){
	globalGain=gain;
	alListenerf(AL_GAIN, globalGain);
}

SoundObject::SoundObject(unsigned int buffer_, bool repeat, bool background, const glm::vec3 &pos_, const glm::vec3
			&vel_, float lifetime_, float sound_gain_, float sound_pitch_):
				 pos(pos_), vel(vel_), al_buffer(buffer_), al_source(-1), playback_start(0.0), sound_priority(1.0),
				 sound_flags((repeat ? sound_flag_repeat : 0) + (background ? sound_flag_background : 0)),
				sound_gain(sound_gain_), sound_pitch(sound_pitch_), al_sound_filter_id(0), lifetime(lifetime_),
				managed(false)
{
}

SoundObject::SoundObject(const std::string &filename, bool repeat, bool background, const glm::vec3 &pos_, const glm::vec3
			&vel_, float lifetime_, float sound_gain_, float sound_pitch_):
				 pos(pos_), vel(vel_), al_buffer(GetALBuffer(filename)), al_source(-1), playback_start(0.0), sound_priority(1.0),
				sound_flags((repeat?sound_flag_repeat:0) + (background?sound_flag_background:0)),
				sound_gain(sound_gain_), sound_pitch(sound_pitch_), al_sound_filter_id(0), lifetime(lifetime_),
				managed(false)
{
}

SoundObject::~SoundObject(){
	if(!managed){
		GlobAudioManager().RemoveSoundObject(this);
	}
}

/// for fire-and-forget sources
void Play3DSound(const std::string &filename, bool repeat, bool background, const glm::vec3 &pos,
				const glm::vec3 &vel, float lifetime, float sound_gain, float sound_pitch){
	SoundObject *so=new SoundObject(filename, repeat, background, pos, vel, lifetime, sound_gain, sound_pitch);
	so->managed=true;
	GlobAudioManager().AddSoundObject(so);
}

void PlayBackgroundSound(const std::string &filename, float sound_gain_) {
	SoundObject *so=new SoundObject(filename, false, true, glm::vec3(0.0), glm::vec3(0.0), 1.0E10, sound_gain_, 1.0);
	so->managed=true;
	GlobAudioManager().AddSoundObject(so);
}


void PlayAmbientSound(const std::string &filename, float gain, bool loop){
	SoundObject *so=new SoundObject(filename, loop, true, glm::vec3(0), glm::vec3(0), 1E10, gain, 1.0);
	//so->managed=true;
	GlobAudioManager().AddSoundObject(so);

	if(GlobAudioManager().currentAmbient != NULL)
	{
		delete GlobAudioManager().nextAmbient;
		GlobAudioManager().nextAmbient = so;
		GlobAudioManager().nextAmbientGain = so->sound_gain;
		GlobAudioManager().startFading = true;
		GlobAudioManager().fadeStartTime = GlobAudioManager().time;
	}
	else
		GlobAudioManager().currentAmbient = so;
}


