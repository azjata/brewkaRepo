#include "bloom.h"

#include "resourcemanager.h"

#include <ImfOutputFile.h>
#include <ImfChannelList.h>

#include <ImfRgbaFile.h>

using namespace std;
using namespace Imf;
using namespace Imath;

#define logger std::cout<<__FILE__<<":"<<__LINE__<<"  "

class GLException: public std::runtime_error{
	public:
	GLException():std::runtime_error(std::string("Uncaught opengl error.")){}
};

void GLThrow(GLenum e){
	if(e!=GL_NO_ERROR){
		//gl_compat::reduced_quality=true;
		throw GLException();
	}
}

bool verbose=true;
struct initialized_int{
	int i;
	initialized_int():i(0){}
	initialized_int(int a):i(a){}
	operator int(){
		return i;
	}
};
std::map<std::string, initialized_int> message_counts;

int CheckGLError(const std::string &msg, bool log_success=false){
	int e=glGetError();
	if (e!=GL_NO_ERROR){
		std::string message=msg+": error "+std::string(reinterpret_cast<const char *>(gluErrorString(e)));
		int count=message_counts[message].i++;
		if(count<20){
			std::cerr<<"["<<std::setw(2)<<count<<"]:"<<message<<std::endl;
		}else if(count==20)
		{
			std::cerr<<message<<" (20 repetitions, not logging it anymore)"<<std::endl;
		}
	}else if (verbose && log_success){
		std::cerr<<msg<<": success"<<std::endl;
	}
	return e;
}
void CheckGLThrow(const std::string &msg, bool log_success=false){
	GLThrow(CheckGLError(msg, log_success));
}


Texture::Texture(int width_, int height_, int depth_,
	GLenum internal_format_, GLenum components_, GLenum type_, int nsamples_):
		width(width_),height(height_), depth(depth_), texture(0),
		internal_format(internal_format_),
		components(components_), type(type_),
		nsamples(nsamples_),
		good(0)
{
	logger<<"Texture::Texture"<<std::endl;
	texture_type = depth ? GL_TEXTURE_3D : (nsamples>0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D);

	try{
		/// making the texture
		glGetError();
		glGenTextures(1,&texture);
		CheckGLThrow("glGenTextures");

		glBindTexture(texture_type, texture);
		CheckGLThrow("glBindTexture");

		filter = GL_LINEAR;
		
		if (width){
			switch(texture_type){
				case GL_TEXTURE_2D:
					glTexImage2D(texture_type, 0, internal_format, width, height, 0, components, GL_UNSIGNED_BYTE, 0);
					CheckGLThrow("glTexImage2D");
					break;
				case GL_TEXTURE_2D_MULTISAMPLE:
					glTexImage2DMultisample(texture_type, nsamples, internal_format, width, height, false);
					CheckGLThrow("glTexImage2DMultisample");
					break;
				case GL_TEXTURE_3D:
					glTexImage3D(GL_TEXTURE_3D, 0, internal_format, width, height, depth, 0, components, GL_UNSIGNED_BYTE, 0);
					CheckGLThrow("glTexImage3D");
					break;
			}
			if(texture_type!=GL_TEXTURE_2D_MULTISAMPLE){
				glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, filter);
				glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, filter);
				CheckGLError("glTexParameteri");
			}
			CheckGLError("creating half-float texture");
		}		
		glBindTexture(texture_type, 0);/// unbind
		good=true;
	}catch(GLException e){
		Destroy();
		//throw;
	}
}

bool Texture::LoadEXR(const std::string &name){
	try{
		RgbaInputFile file(name.c_str());
		Box2i dw = file.dataWindow();
		int width = dw.max.x - dw.min.x + 1;
		int height = dw.max.y - dw.min.y + 1;
		StrongPtr<Texture> result=new Texture(width, height, 0, GL_RGB16F_ARB);
		std::vector<Rgba> tmp(width*height, Rgba(0,0,0,1));


		/// load line by line in case we want to flip Y or do something like that
		std::vector<Rgba> scanline(width);
		file.setFrameBuffer (&(scanline[0]) - dw.min.x - dw.min.y * width, 1, 0);
		//float scale=1.0;

		for(int y=0;y<height;++y){
		  file.readPixels(y+dw.min.y,y+dw.min.y);
		  for(int x=0;x<width;++x){
		  	tmp[x+y*width]=scanline[x];
			/*

			tmp[x+y*width].r=(scanline[x].r)*scale;
			tmp[x+y*width].g=(scanline[x].g)*scale;
			tmp[x+y*width].b=(scanline[x].b)*scale;
			tmp[x+y*width].a=scanline[x].a;*/
		  }
		}
		glBindTexture(texture_type, texture);
		//filter = GL_LINEAR;
		glTexImage2D(GL_TEXTURE_2D , 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, (void*)&tmp[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ) ;
		glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ) ;

		glBindTexture(texture_type, 0);
		return true;
  }
  catch (const std::exception &exc)
  {
    std::cerr << "Exception when trying to read openEXR image. " << exc.what() << std::endl;
    return false;
  }
}

TextureFramebuffer::TextureFramebuffer(int width_, int height_, int depth_,
	bool depth_buffer_, GLenum internal_format_, GLenum components_, GLenum type_, int nsamples_):
		Texture(width_, height_, depth_, internal_format_, components_, type_, nsamples_),
		fbo(0),
		depthbuffer(0)
{
	logger<<"TextureFramebuffer::TextureFramebuffer"<<std::endl;
	if(!good){/// failed to create texture?
		logger<<"Texture not good, not creating fbo"<<std::endl;
		return;
	}
	good=false;
	GLint old_fb=0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fb);

	GLint old_rb=0;
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &old_rb);
	try{
		glGetError();
		glGenFramebuffersEXT(1, &fbo);
		CheckGLThrow("glGenFramebuffersEXT");
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
		CheckGLThrow("glBindFramebufferEXT");

		if (depth_buffer_){
			glGenRenderbuffersEXT(1, &depthbuffer);
			CheckGLThrow("glGenRenderbuffersEXT");
			if (width){
				glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthbuffer);
				CheckGLThrow("glBindRenderbufferEXT");
				if(nsamples>0){
					glRenderbufferStorageMultisample(GL_RENDERBUFFER, nsamples, GL_DEPTH_COMPONENT32, width, height);
				}else{
					glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT32, width, height);
				}
				CheckGLThrow("glRenderbufferStorageEXT");
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthbuffer);
				CheckGLThrow("glFramebufferRenderbufferEXT");
			}
		}
		if (width){

			switch(texture_type){
				case GL_TEXTURE_2D:
				case GL_TEXTURE_2D_MULTISAMPLE: glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, texture_type, texture, 0);
				break;
				case GL_TEXTURE_3D: glFramebufferTexture(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, texture, 0);
				break;
			}
			CheckGLThrow("glFramebufferTexture2DEXT");
			if(!IsCompleteFB()){
				//gl_compat::reduced_quality=true;
				throw GLException();
			}
		}else{
			std::cerr<<"Size not set, not checking for completeness."<<std::endl;
		}
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, old_fb);/// unbind
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, old_rb);
		CheckGLThrow("creating framebuffer");
		good=true;
	}catch(GLException e){
		Destroy();
		//throw;
	}
}

void Texture::Resize(int width_, int height_, int depth_){
	if (width==width_ && height==height_ && depth==depth_)return;

	good=false;

	logger<<"Texture::Resize("<<width_<<","<<height_<<","<<depth_<<")"<<std::endl;

	if (!texture)return;

	try{
		width=width_;
		height=height_;
		depth=depth_;

		glGetError();
		glBindTexture(texture_type, texture);

		//glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, components, type, 0);

		switch(texture_type){
			case GL_TEXTURE_2D:
				glTexImage2D(texture_type, 0, internal_format, width, height, 0, components, GL_UNSIGNED_BYTE, 0);
				CheckGLThrow("glTexImage2D");
				break;
			case GL_TEXTURE_2D_MULTISAMPLE:
				glTexImage2DMultisample(texture_type, nsamples, internal_format, width, height, false);
				CheckGLThrow("glTexImage2DMultisample");
				break;
			case GL_TEXTURE_3D:
				glTexImage3D(GL_TEXTURE_3D, 0, internal_format, width, height, depth, 0, components, GL_UNSIGNED_BYTE, 0);
				CheckGLThrow("glTexImage3D");
				break;
		}

		if(texture_type!=GL_TEXTURE_2D_MULTISAMPLE){
			glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, filter);
			CheckGLThrow("glTexParameteri");
		}

		glBindTexture(texture_type, 0);/// unbind
		good=true;
	}catch(GLException e){
		Destroy();
		//throw;
	}
}

void TextureFramebuffer::Resize(int width_, int height_, int depth_){
	if (width==width_ && height==height_ && depth==depth_)return;

	Texture::Resize(width_, height_, depth_);
	if(!good)return;
	try{

		GLint old_fb=0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fb);

		GLint old_rb=0;
		glGetIntegerv(GL_RENDERBUFFER_BINDING, &old_rb);

		/// no idea if thats needed

		if (depthbuffer){
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthbuffer);
			if(nsamples>0){
				glRenderbufferStorageMultisample(GL_RENDERBUFFER, nsamples, GL_DEPTH_COMPONENT32, width, height);
			}else{
				glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT32, width, height);
			}
			CheckGLThrow("Resizing depthbuffer");
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

		switch(texture_type){
			case GL_TEXTURE_2D:
			case GL_TEXTURE_2D_MULTISAMPLE: glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, texture_type, texture, 0);
			break;
			case GL_TEXTURE_3D: glFramebufferTexture(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, texture, 0);
			break;
		}

		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthbuffer);
		CheckGLThrow("Re-binding texture to framebuffer");

		if(!IsCompleteFB()){
			//gl_compat::reduced_quality=true;
			throw GLException();
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, old_fb);/// unbind
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, old_rb);
		CheckGLThrow("unbinding");

		good=true;
	}catch(GLException e){
		Destroy();
		//throw;
	}
}

void Texture::Destroy(){
	glGetError();
	if (texture)glDeleteTextures(1,&texture);
	CheckGLError("destroying texture");
	texture=0;
	good=false;
}


void TextureFramebuffer::Destroy(){
	Texture::Destroy();
	glGetError();
	if(fbo)glDeleteFramebuffersEXT(1,&fbo);
	if(depthbuffer)glDeleteRenderbuffersEXT(1, &depthbuffer);
	CheckGLError("destroying framebuffer");
	fbo=0;
	depthbuffer=0;
}

void TextureFramebuffer::BindFB(){
	if(good){
		glGetError();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
		CheckGLThrow("glBindFramebufferEXT", false);
	}
}

void Texture::BindTexture(){
	if(good)glBindTexture(texture_type, texture);
}


Texture::~Texture(){
	logger<<"Texture::~Texture()"<<std::endl;
	Destroy();
}


TextureFramebuffer::~TextureFramebuffer(){
	logger<<"TextureFramebuffer::~TextureFramebuffer()"<<std::endl;
	Destroy();
}

bool TextureFramebuffer::IsCompleteFB(){
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	CheckGLError("glCheckFramebufferStatusEXT");
	if (status == GL_FRAMEBUFFER_COMPLETE_EXT){
		//std::cerr<<"framebuffer not complete!"<<std::endl;
		return true;
	}
	std::cerr<<"framebuffer not complete, status:"<<std::endl;
	switch (status){
#define fbcase(x) case x##_EXT: std::cerr<< #x <<std::endl; break;
		fbcase(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
		fbcase(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT
		fbcase(GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT)
#endif
		fbcase(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
		fbcase(GL_FRAMEBUFFER_INCOMPLETE_FORMATS)
		fbcase(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
		fbcase(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
		fbcase(GL_FRAMEBUFFER_UNSUPPORTED)
	default:
		std::cerr<< "Unknown framebuffer status "<<status<<std::endl;
#undef fbcase
	}
	return false;
}


std::vector<GLint> fb_stack;

void PushFB(){
	GLint old_fb=0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fb);
	fb_stack.push_back(old_fb);
	if(fb_stack.size()>1000){
		ofLog(OF_LOG_ERROR,"PushFB() : >1000 entries, probably forgot to PopFB");
	}
}
void PopFB(){
	if(fb_stack.size()>0){
		glBindFramebuffer(GL_FRAMEBUFFER, fb_stack.back());
		fb_stack.pop_back();
	}else{
		ofLog(OF_LOG_ERROR,"PopFB() : no entries on stack");
	}
}

void TextureFramebuffer::BeginFB(){
	if(!good)return;
	PushFB();
	glPushAttrib(GL_VIEWPORT_BIT);
	BindFB();
	glViewport(0, 0, width, height);
}

void TextureFramebuffer::EndFB(){
	if(!good)return;
	PopFB();
	glPopAttrib();
}

void glmBox(const glm::vec2 &l, const glm::vec2 &h, const glm::vec2 &tl, const glm::vec2 &th){
	glTexCoord2f(tl.x,tl.y);
	glVertex2f(l.x,l.y);

	glTexCoord2f(tl.x,th.y);
	glVertex2f(l.x,h.y);

	glTexCoord2f(th.x,th.y);
	glVertex2f(h.x,h.y);

	glTexCoord2f(th.x,tl.y);
	glVertex2f(h.x,l.y);
}

/// TODO: make some kind of shader manager

static ShaderResource Blur1Shader("bloom/blur1_v.glsl", "bloom/blur1_f.glsl");
static ShaderResource FragPowerShader("bloom/fragpower_v.glsl", "bloom/fragpower_f.glsl");
static ShaderResource PlainDrawShader("bloom/plain_draw_v.glsl", "bloom/plain_draw_f.glsl");

BloomParameters::BloomParameters():
	 bloom_multiplier_color(0.1,0.1,0.1),
	 bloom_falloff_color(1,1,1),
	 exposure(1),
	 gamma(1),
	 desaturate(0),
	 tonemap_method(0),
	 antialiasing(0),
	 crisp_antialiasing(false),
	 multisample_antialiasing(0)
{

}


const int blur_method=2;

void DownscaleDirty(TextureFramebuffer &src, TextureFramebuffer &dest) {/// warning - wipes modelview and projection matrices, as well as viewport. requires shader setup

	glGetError();
	dest.Resize(src.width/2, src.height/2);
	dest.BindFB();
	GLThrow(CheckGLError("DownscaleDirty bind fb", false));
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0 + 0);
	src.BindTexture();
	GLThrow(CheckGLError("DownscaleDirty bind tex", false));

	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) ;
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) ;

	{
		glGetError();

		Blur1Shader().setUniform1i("my_color_texture",0);
		CheckGLError("set my_color_texture uniform", false);

		double delta_scale;

		switch (blur_method) {
		case 1:/// diamond method
			delta_scale=1.0;

			Blur1Shader().setUniform2f("d0", 0.0, -delta_scale/src.height);
			Blur1Shader().setUniform2f("d1", -delta_scale/src.width, 0.0);
			Blur1Shader().setUniform2f("d2", delta_scale/src.width, 0.0);
			Blur1Shader().setUniform2f("d3", 0.0, delta_scale/src.height);

			break;
		case 2:/// box method
		default:
			delta_scale=0.75;

			Blur1Shader().setUniform2f("d0", -delta_scale/src.width, -delta_scale/src.height);
			Blur1Shader().setUniform2f("d1", delta_scale/src.width, -delta_scale/src.height);
			Blur1Shader().setUniform2f("d2", -delta_scale/src.width, delta_scale/src.height);
			Blur1Shader().setUniform2f("d3", delta_scale/src.width, delta_scale/src.height);

			break;
		}

		CheckGLError("set d0..3", false);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) ;
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) ;

/// setup render.
	glViewport(0,0,dest.width, dest.height);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);


	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glBegin(GL_QUADS);
	glm::vec2 invsize(1.0/src.width, 1.0/src.height);
	glm::vec2 offset(0,0);

	glmBox(glm::vec2(-1,-1 /* + 2.0/dest.height */), glm::vec2(1 /* - 2.0/dest.width */ ,1), glm::vec2(0, 0)+offset, glm::vec2(1.0-(src.width&1?invsize.x:0),1.0-(src.height&1?invsize.y:0))+offset);
	glEnd();

/// cleanup.
	//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	//glBindTexture(GL_TEXTURE_2D, 0);

}


void Bloom::DownsampleFB() {/// warning - wipes modelview and projection matrices and viewport
	TextureFramebuffer *src=base_fb.Ptr();
	if (!src)return;
	if (!downsampled_buffer.size()) {
		downsampled_buffer.push_back(base_fb);
	}
	//if (settings::blur_method)
	Blur1Shader().begin();
	int i;
	for (i=1;(src->width>3&&src->height>3);++i) {
		if (i>=int(downsampled_buffer.size())) {
			downsampled_buffer.push_back(new TextureFramebuffer(src->width/2,src->height/2));
		}
		TextureFramebuffer *dest=downsampled_buffer[i].Ptr();
		DownscaleDirty(*src, *dest);
		src=dest;
	}
	downsampled_buffer.resize(i);/// important - no dead entries.

	//if (settings::blur_method)glUseProgram(0);
	Blur1Shader().end();
}



const bool shader_interpolate=false;

void UpscaleDirty(TextureFramebuffer &src, TextureFramebuffer &dest) {/// warning - wipes modelview and projection matrices, as well as viewport. requires shader setup

	glGetError();
	dest.BindFB();
	GLThrow(CheckGLError("UpscaleDirty bind fb", false));
	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0 + 0);
	src.BindTexture();
	GLThrow(CheckGLError("UpscaleDirty bind tex", false));

	if(shader_interpolate){
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}else{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) ;
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) ;

/// setup render.
	glViewport(0,0,dest.width, dest.height);

	glm::vec2 invsize(1.0/src.width, 1.0/src.height);
	glm::vec2 offset(0,0);

	glm::vec2 tx_size(double(dest.width)/double(src.width*2.0),double(dest.height)/double(src.height*2.0));
/*
	if(shader_interpolate){
		float delta_scale=0.3;
		glUniform2f(glGetUniformLocation(Blur1Shader().program_id, "d0"), -delta_scale/src.width, -delta_scale/src.height);
		glUniform2f(glGetUniformLocation(Blur1Shader().program_id, "d1"), delta_scale/src.width, -delta_scale/src.height);
		glUniform2f(glGetUniformLocation(Blur1Shader().program_id, "d2"), -delta_scale/src.width, delta_scale/src.height);
		glUniform2f(glGetUniformLocation(Blur1Shader().program_id, "d3"), delta_scale/src.width, delta_scale/src.height);
	}
	*/
	glBegin(GL_QUADS);
	glmBox(glm::vec2 (-1,-1 /* + 2.0/dest.height */), glm::vec2 (1 /* - 2.0/dest.width */ ,1), offset, tx_size+offset);
	glEnd();
}

void Bloom::UpsampleBloomFB() {/// warning - wipes modelview and projection matrices and viewport
	if(params.crisp_antialiasing>params.antialiasing){
		params.crisp_antialiasing=params.antialiasing;
	}
	//shader_interpolate=settings::force_radeon_1950_workaround;

	glUseProgram(0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	glColor4f(params.bloom_falloff_color.r, params.bloom_falloff_color.g, params.bloom_falloff_color.b, 1);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
/*
	if(shader_interpolate){
		glUseProgram(Blur1Shader().program_id);
	}
	*/
	PlainDrawShader().begin();
	for (int i=int(downsampled_buffer.size())-1; i>params.antialiasing-params.crisp_antialiasing; --i) {
		if(i==1+params.antialiasing){
			glColor4f(params.bloom_multiplier_color.r, params.bloom_multiplier_color.g, params.bloom_multiplier_color.b, 1);
		}else{
			glColor4f(params.bloom_falloff_color.r, params.bloom_falloff_color.g, params.bloom_falloff_color.b, 1);
		}
		TextureFramebuffer *src=downsampled_buffer[i].Ptr();
		TextureFramebuffer *dest=downsampled_buffer[i-1].Ptr();
		UpscaleDirty(*src,*dest);
	}
	/*
	if(shader_interpolate){
		glUseProgram(0);
	}
	*/
	PlainDrawShader().end();
}

Bloom::Bloom(): original_fb(0)
{
	//ctor
}

Bloom::~Bloom()
{
	//dtor
}

void Bloom::setup(){
}

void Bloom::begin(int width, int height){
	begin_params=params;
	glPushAttrib(GL_VIEWPORT_BIT);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &original_fb);
	int antialiasing_factor=1<<params.antialiasing;

	if (base_fb.ok()) {
		base_fb->Resize(width*antialiasing_factor, height*antialiasing_factor);
	} else {
		base_fb=new TextureFramebuffer(width*antialiasing_factor, height*antialiasing_factor, 0, true);
		base_fb->Resize(width*antialiasing_factor, height*antialiasing_factor);
	}
	base_fb->BindFB();
	glViewport(0,0,base_fb->width, base_fb->height);
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if(params.multisample_antialiasing>0){
		msaa.nsamples=params.multisample_antialiasing;
		msaa.begin(width*antialiasing_factor, height*antialiasing_factor);
	}
}
void Bloom::end(){

	/*
	GLuint original_blend;
	glGet(GL_BLEND, &original_blend);
	GLuint original_depth_test;
	glGet(GL_DEPTH_TEST, &original_depth_test);
	GLuint original_cull_face;
	glGet(GL_CULL_FACE, &original_cull_face);
*/
	if(begin_params.multisample_antialiasing>0){
		msaa.end();
	}

	if(base_fb.ok()){
		try{
			DownsampleFB();
			UpsampleBloomFB();
		}catch(GLException){
			logger<<"no framebuffer"<<std::endl;
		}
	}

	glPopAttrib();

	glBindFramebuffer(GL_FRAMEBUFFER, original_fb);

	/*
	glBlend(original_blend);
	glCullFace()
	*/
	glDisable(GL_BLEND);
}
void Bloom::draw(){
	if(params.antialiasing<=0){
		if(!base_fb.ok())return;
		base_fb->BindTexture();
	}else{
		if(params.antialiasing>=downsampled_buffer.size() || !downsampled_buffer[params.antialiasing].ok())return;
		downsampled_buffer[params.antialiasing-params.crisp_antialiasing]->BindTexture();
	}
	FragPowerShader().begin();
	FragPowerShader().setUniform1f("exposure", params.exposure);
	FragPowerShader().setUniform1f("power", 1.0/params.gamma);
	FragPowerShader().setUniform1f("desaturate", params.desaturate);
	FragPowerShader().setUniform1i("tonemap_method", params.tonemap_method);
	glBegin(GL_QUADS);
	glmBox(glm::vec2(-1,-1), glm::vec2(1,1), glm::vec2(0,0), glm::vec2(1,1));
	glEnd();
	FragPowerShader().end();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Bloom::setParams(const BloomParameters &params_){
	params=params_;
}
BloomParameters Bloom::getParams(){
	return params;
}

Antialiasing::Antialiasing(int nsamples_):nsamples(nsamples_)
{
}
Antialiasing::~Antialiasing(){
}

void Antialiasing::begin(int width, int height){
	if(!fb.ok() || fb->nsamples!=nsamples){
		if(nsamples){
			fb=new TextureFramebuffer(width, height, 0, true, gl_compat::hdr_internal_format,
			GL_RGB, GL_HALF_FLOAT_ARB, nsamples);
		}else{
			fb=NULL;
		}
	}
	if(fb.ok() && fb->good){
		fb->Resize(width, height);
		fb->BeginFB();
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
}
void Antialiasing::end(){
	if(fb.ok() && fb->good){
		fb->EndFB();
		GLint out_fb=0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &out_fb);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fb->fbo);
		//glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, out_fb);
		glBlitFramebuffer(0, 0, fb->width, fb->height, 0, 0, fb->width, fb->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, out_fb);
	}
}
