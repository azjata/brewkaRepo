#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "ofMain.h"

class ManagedResource{
public:
	ManagedResource();
	virtual ~ManagedResource();
	virtual bool load()=0;/// return true on success.
	virtual void opengl_context_loss()=0;/// invalidate all opengl objects etc
	virtual bool reload()=0;/// return true on success.
	virtual void discard()=0;
	virtual bool loaded()=0;
};

class ResourceManager
{
	public:
		ResourceManager();
		virtual ~ResourceManager();
		void addResource(ManagedResource *resource);
		bool load();
		void opengl_context_loss();
		bool reload();
		void discard();
	protected:
		std::list<ManagedResource *> resources;
	private:
};

ResourceManager &getGlobalResourceManager();

/// For usage example, see bloom.cpp
/// You normally use it by declaring static members or variables like so:
/// static ShaderResource fragPowerShader("bloom/fragpower_v.glsl", "bloom/fragpower_f.glsl");
/// then use fragPowerShader()
/// the resulting shader will load when getGlobalResourceManager().load() is called, or
/// when it is first used, which ever comes earlier.
class ShaderResource: public ManagedResource{
	/// not copyable
	ShaderResource(const ShaderResource &other){}
public:
	ShaderResource(std::string vertex_name, std::string fragment_name, std::string geometry_name="",
					std::string extra_defines="");
	~ShaderResource();
	virtual bool load();
	virtual void opengl_context_loss();
	virtual bool reload();
	virtual void discard();
	virtual bool loaded();
	ofShader &get();
	inline ofShader &operator()(){
		return get();
	}
private:
	ofShader *shader;
	bool return_status;
	std::string vertex_name, fragment_name, geometry_name, extra_defines;
};

/// TODO: same thing for textures and the like

#endif // RESOURCEMANAGER_H
