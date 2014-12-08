#pragma once

#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstddef>
#include <string>

#include "ofMain.h"
#include "glm/glm.hpp"

/// do not forget to call glGetError(); before a sequence of such checks.
#define CHECK_GL(a) a {int e=glGetError(); if(e){std::cout<<"OpenGL error "<<e<<" "<<gluErrorString(e)<<" at " << __FILE__ << ":"<< __LINE__ << " "<< #a<<std::endl;}}

//#define gl_offsetof(x,y) (void *)(&(reinterpret_cast<x*>(NULL)->y))
#define gl_offsetof(x,y) (void *)(offsetof(x,y))

/// Copied over from helper classes in The Polynomial 's game engine

#ifndef _MSC_VER			/// __sync_add_and_fetch doesn't exist on win platform
	#define THREADSAFE__
#endif
/// NOTE: the weak references are still NOT thread safe.

#ifdef __GNUC__
#define my_logger (std::cout<<"("<<__LINE__<<":"<<__func__<<") ")
#else
#define my_logger (std::cout<<"("<<__LINE__<<":"<<__FUNCTION__<<") ")
#endif

std::string GetFileNameExtLC(const std::string &filename);

class Lockable {
public:
    volatile int lock_count;
    Lockable():lock_count(0) {}
    ;
    virtual ~Lockable() {}
    ;
    bool Locked() {
        return lock_count>0;
    }
    void Lock() {
			#ifdef THREADSAFE__
				__sync_add_and_fetch(&lock_count,1);
			#else
        ++lock_count;
			#endif
        //debug_message("Lock()");
    }
    void UnLock() {
			#ifdef THREADSAFE__
				__sync_sub_and_fetch(&lock_count,1);
			#else
        --lock_count;
			#endif
        //debug_message("UnLock()");
    }
};

template<Lockable *T>
class StaticLocker {
public:
    inline StaticLocker() {
        T->Lock();
    }
    inline ~StaticLocker() {
        T->UnLock();
    };
};

class Locker {
    Lockable &lock;
    public:
    inline Locker(Lockable &lock_): lock(lock_) {
        lock.Lock();
    }
    inline ~Locker() {
        lock.UnLock();
    };
};

class RefcountedContainer {
public:
    Lockable ref_lock;

    RefcountedContainer() {
    	//ref_lock.Lock();
    };

    virtual void PreDestroy() {
    }

    virtual ~RefcountedContainer() {
      //assert(!ref_lock.Locked());
    };
    RefcountedContainer* NewCopy() {
        return new RefcountedContainer(*this);
    }
    inline  void Reference() {
        ref_lock.Lock();
    }
    inline  void UnReference() {
        ref_lock.UnLock();
    }
    inline  bool Referenced() {
        return ref_lock.Locked();
    }
};


template <class T , bool TDestroy=true>
class StrongPtr {
public:
 T *data;
    StrongPtr():data(0) {}
    StrongPtr (T *p):data(p) {
        if(data)data->Reference();
    }
    StrongPtr(const StrongPtr<T,TDestroy> &p):data(p.data) {
        if(data) {
            data->Reference();
        }
        else {
           // error("StrongPtr copy : null");
        }
    };

    /* */
template<class U>
explicit StrongPtr(StrongPtr<U> p){

	///    std::cout<<"doing dynamic cast"<<std::endl;
	     data=dynamic_cast<T*>(p.data);
     /// std::cout<<"... done"<<std::endl;*/
        if(data) {
            data->Reference();
        }
        else {
            ///error("StrongPtr copy : null (cast)");
        }
    };

template<class U>
explicit StrongPtr(U *p){

	///    std::cout<<"doing dynamic cast"<<std::endl;
	     data=dynamic_cast<T*>(p);
     /// std::cout<<"... done"<<std::endl;*/
        if(data) {
            data->Reference();
        }
        else {
            ///error("StrongPtr copy : null (cast)");
        }
    };

    /* */
    ~StrongPtr() {
        Dispose();
    };
    StrongPtr<T, TDestroy> &operator= (const StrongPtr &p) throw() {
        if (this != &p) {
            Dispose();
            data = p.data;
            if(data) {
                data->Reference();
            }
            else {
                ///error("StrongPtr operator= : bad parameter");
            }
        }
        return *this;
    }
    void Dispose() {
        if(!data) {
            ///error("Something wrong with StrongPtr.dispose");
            return;
        }
        data->UnReference();
        if(TDestroy) {
            if(!data->Referenced()) {
								data->PreDestroy();
                delete data;
            }
        }
        data=NULL;
    }
    T &Data() const {
        return *data;
    }
    T *Ptr() const {
        return data;
    }
    StrongPtr<T, TDestroy> Copy() {
        if(data) {
            StrongPtr<T, TDestroy> result(data->NewCopy());
            return result;
        }
        else {
            StrongPtr<T, TDestroy> result;
            return result;
        }
    }

    T &operator *() const {
    	return *data;
    }
    T *operator ->() const {
    	return data;
    }
    bool operator <(StrongPtr<T, TDestroy> p) const{
    	return data<p.data;
    }
    bool operator >(StrongPtr<T, TDestroy> p) const{
    	return data>p.data;
    }
    bool operator ==(StrongPtr<T, TDestroy> p) const{
    	return data==p.data;
    }
    bool operator !=(StrongPtr<T, TDestroy> p) const{
    	return !(data==p.data);
    }
    bool operator <(T *p) const {
    	return data<p;
    }
    bool operator >(T *p) const {
    	return data>p;
    }
    bool operator ==(T *p) const {
    	return data==p;
    }
    bool operator !=(T *p) const {
    	return !(data==p);
    }
    /*
    operator bool() const {
    	return data;
    }*/
    bool ok() const{
        return data;
    }
};
/// backward compatibility for my existing code
#define RefcountedPointer StrongPtr



inline ofVec3f toOF(const glm::vec3 &vec){
	return ofVec3f(vec.x, vec.y, vec.z);
}
inline glm::vec3 fromOF(const ofVec3f &vec){
	return glm::vec3(vec.x, vec.y, vec.z);
}

inline glm::mat4x4 &fromOF(ofMatrix4x4 &m){
	return *reinterpret_cast<glm::mat4x4 *>(&m);
}

inline const glm::mat4x4 &fromOF(const ofMatrix4x4 &m){
	return *reinterpret_cast<const glm::mat4x4 *>(&m);
}

inline ofMatrix3x3 &toOF(glm::mat3x3 &m){
	return *reinterpret_cast<ofMatrix3x3 *>(&m);
}

inline const ofMatrix3x3 &toOF(const glm::mat3x3 &m){
	return *reinterpret_cast<const ofMatrix3x3 *>(&m);
}

inline ofMatrix4x4 &toOF(glm::mat4x4 &m){
	return *reinterpret_cast<ofMatrix4x4 *>(&m);
}

inline const ofMatrix4x4 &toOF(const glm::mat4x4 &m){
	return *reinterpret_cast<const ofMatrix4x4 *>(&m);
}



#endif // UTILS_H
