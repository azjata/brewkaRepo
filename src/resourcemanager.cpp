#include "resourcemanager.h"

ManagedResource::ManagedResource(){
	getGlobalResourceManager().addResource(this);
}

ManagedResource::~ManagedResource(){
}

ResourceManager::ResourceManager()
{
	//ctor
}

ResourceManager::~ResourceManager()
{
	/// intentionally left blank
}

void ResourceManager::addResource(ManagedResource *resource)
{
	resources.push_back(resource);
}

bool ResourceManager::load()
{
	bool result=true;
	for(std::list<ManagedResource *>::iterator i=resources.begin(); i!=resources.end(); ++i){
		result&=(**i).load();
	}
	return result;
}

void ResourceManager::opengl_context_loss()
{
	for(std::list<ManagedResource *>::iterator i=resources.begin(); i!=resources.end(); ++i){
		(**i).opengl_context_loss();
	}
}

bool ResourceManager::reload()
{
	bool result=true;
	for(std::list<ManagedResource *>::iterator i=resources.begin(); i!=resources.end(); ++i){
		result&=(**i).reload();
	}
	return result;
}

void ResourceManager::discard()
{
	for(std::list<ManagedResource *>::iterator i=resources.begin(); i!=resources.end(); ++i){
		(**i).discard();
	}
}

ResourceManager &getGlobalResourceManager(){
	static ResourceManager *mgr=new ResourceManager();/// intentionally not deleted on program exit.
	return *mgr;
}

ShaderResource::ShaderResource(std::string vertex_name_, std::string fragment_name_, std::string geometry_name_,
							std::string extra_defines_):
	shader(NULL),
	vertex_name(vertex_name_),
	fragment_name(fragment_name_),
	geometry_name(geometry_name_),
	extra_defines(extra_defines_)
{

}
ShaderResource::~ShaderResource(){
	/// Intentionally left blank. We do not do de-initialization on program exit.
}

static std::string nameForType(GLenum type) {
	switch(type) {
		case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
		case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
		case GL_GEOMETRY_SHADER_EXT: return "GL_GEOMETRY_SHADER_EXT";
		default: return "UNKNOWN SHADER TYPE";
	}
}

bool setupShaderFromFileWithExtraDefines(ofShader &shader, GLenum type, std::string filename, std::string extra_defines){
	ofBuffer buffer = ofBufferFromFile(filename);
	if(buffer.size()) {
		return shader.setupShaderFromSource(type, extra_defines+"\n#line 0\n"+buffer.getText());
	} else {
		ofLogError("ofShader") << "setupShaderFromFileWithExtraDefines(): couldn't load " << nameForType(type) << " shader " << " from \"" << filename << "\"";
		return false;
	}
}

bool loadShaderFromFilesWithExtraDefines(ofShader &shader, string vertName, string fragName, string geomName, string defines) {
	if(!vertName.empty()) setupShaderFromFileWithExtraDefines(shader, GL_VERTEX_SHADER, vertName, defines);
	if(!fragName.empty()) setupShaderFromFileWithExtraDefines(shader, GL_FRAGMENT_SHADER, fragName, defines);
#ifndef TARGET_OPENGLES
	if(!geomName.empty()) setupShaderFromFileWithExtraDefines(shader, GL_GEOMETRY_SHADER_EXT, geomName, defines);
#endif
	if(ofIsGLProgrammableRenderer()){
		shader.bindDefaults();
	}
	return shader.linkProgram();
}

bool ShaderResource::load(){
	if(!shader){/// may be already loaded if a resource initializes another resource through get()
		shader=new ofShader();
		ofLog()<<"Loading shaders '"<<vertex_name<<"' '"<<fragment_name<<"' '"<<geometry_name<<"' with '"<<extra_defines<<"'";
		if(extra_defines.empty()){
			return_status=shader->load(vertex_name, fragment_name, geometry_name);
		}else{
			return_status=loadShaderFromFilesWithExtraDefines(*shader, vertex_name, fragment_name, geometry_name, extra_defines);
		}
	}
	return return_status;
}
void ShaderResource::opengl_context_loss(){
	discard();
}
bool ShaderResource::reload(){
	discard();
	return load();
}
void ShaderResource::discard(){
	delete shader;
	shader=NULL;
}
bool ShaderResource::loaded(){
	return shader!=NULL;
}
ofShader &ShaderResource::get(){
	if(!shader)load();
	return *shader;
}
