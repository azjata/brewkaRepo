#ifndef BLOOM_H
#define BLOOM_H

#include "utils.h"

#include "ofMain.h"

#include "glm/glm.hpp"

namespace gl_compat{
	const GLenum hdr_internal_format=GL_RGB16F_ARB ;
	const bool reduced_quality=false;
};

class Texture: public RefcountedContainer {
	Texture(const Texture &other){}/// make non copyable
	public:
	GLenum texture_type;/// GL_TEXTURE_2D, GL_TEXTURE_3D, to be implemented: GL_TEXTURE_2D_MULTISAMPLE
	int width, height, depth;
	GLuint texture;

	GLenum internal_format  ;
	GLenum components ;
	GLenum type ;
	GLenum filter ;
	int nsamples;

	bool good;

	explicit Texture(int width_=0, int height_=0, int depth_=0, GLenum internal_format_=gl_compat::hdr_internal_format, GLenum components=GL_RGB, GLenum type_=GL_HALF_FLOAT_ARB, int nsamples_=0);
	virtual bool LoadEXR(const std::string &name);/// use ofToDataPath(fileName) with it
	virtual void Resize(int width_, int height_, int depth_=0);
	virtual void Destroy();
	void BindTexture();
	virtual ~Texture();
};

class TextureFramebuffer: public Texture{
	TextureFramebuffer(const TextureFramebuffer &other){}/// make non copyable
public:
	GLuint fbo;
	GLuint depthbuffer;
private:
	bool IsCompleteFB();
public:
	explicit TextureFramebuffer(int width_=0, int height_=0, int depth_=0, bool depth_buffer_=0, GLenum internal_format_=gl_compat::hdr_internal_format, GLenum components=GL_RGB, GLenum type_=GL_HALF_FLOAT_ARB, int nsamples_=0);
	virtual void Resize(int width_, int height_, int depth_=0);
	virtual void Destroy();
	void BindFB();

	void BeginFB();/// pushes current framebuffer
	void EndFB();
	~TextureFramebuffer();
};

void PushFB();/// pushes current framebuffer
void PopFB();

struct BloomParameters{
	glm::vec3 bloom_multiplier_color;
	glm::vec3 bloom_falloff_color;
	float exposure;
	float gamma;
	float desaturate;
	int tonemap_method;
	int antialiasing;/// Full scene antialiasing, 0: none, 1: 2x2, 2: 4x4 , and so on. Only usable values are 1 and 2
	bool crisp_antialiasing;/// only works with antialiasing of 2
	int multisample_antialiasing;/// OpenGL MSAA, applied before full-scene antialiasing. Number of samples (16 is the maximum for 5gum target hardware)
	BloomParameters();/// initializes with defaults
};

class Antialiasing/// Implements OpenGL MSAA
{
public:
	int nsamples;
	Antialiasing(int nsamples_=16);
	virtual ~Antialiasing();
	void begin(int width, int height);
	void end();

	StrongPtr<TextureFramebuffer> fb;
};

class Bloom
{
	public:
		Bloom();
		void setup();
		virtual ~Bloom();

		void setParams(const BloomParameters &params_);
		BloomParameters getParams();
		void begin(int width, int height);/// remembers original framebuffer and binds the HDR framebuffer
		void end();/// applies the bloom operation to the framebuffer, binds original framebuffer
		void draw();/// draws bloom'd framebuffer

	BloomParameters params;
	GLint original_fb;

	StrongPtr<TextureFramebuffer> base_fb;
	std::vector< StrongPtr<TextureFramebuffer> > downsampled_buffer;

	Antialiasing msaa;
	protected:
	void DownsampleFB();
	void UpsampleBloomFB();
	private:
		BloomParameters begin_params;
};


#endif // BLOOM_H
