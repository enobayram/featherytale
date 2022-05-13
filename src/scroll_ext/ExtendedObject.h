/*
 * ExtendedObject.h
 *
 *  Created on: Mar 28, 2015
 *      Author: eba
 */

#ifndef EXTENDEDOBJECT_H_
#define EXTENDEDOBJECT_H_

#include <map>
#include <vector>

#include <Scroll.h>
#include "config_utils.hpp"
#include "util/orxVECTORoperators.h"
#include "util/optional.hpp"
#include <util/orx_pointers.hpp>

struct marker {
    orxVECTOR center;
    double radius;
    bool contains(const orxVECTOR & p) {return squaredNorm(center-p) < radius*radius;}
};

template <class O>
inline const char * GetBindName() {
    return ScrollObjectRegistrar<O>::BindName();
}


class ExtendedMonomorphic: public ScrollObject {
    std::map<int, void *> aspects;
    SCROLL_BIND_CLASS("ExtendedObject")
public:
    template<class T> T * GetAspect(typename T::AspectTag = 0);
    virtual void SetContext(ExtendedMonomorphic & context) {}
    template<class T> T * CreateChild(const char * section = nullptr, bool set_group = false, bool in_frame = true);
    orxOBJECT * CreateChild(const char * section, bool set_group = false, bool in_frame = true);

    orxCAMERA * referenceCamera = nullptr;
    const char * referenceSection = nullptr;
    using ScrollObject::GetScale;
    using ScrollObject::GetPosition;
    orxVECTOR GetScale(bool world = false) { orxVECTOR result; GetScale(result, world); return result; }
    orxVECTOR GetPosition(bool world = false) { orxVECTOR result; GetPosition(result, world); return result; }
    void SetSize(const orxVECTOR & size);
    orxVECTOR GetSize();
    void SetHeight(orxFLOAT height);
    void SetWidth(orxFLOAT width);
    void SetRelativeHeight(orxFLOAT height);
    void SetRelativePosition(const orxVECTOR & pos);
    orxOBJECT * FindOwnedChild(const char * childName);
    template <class DERIVING> DERIVING * FindOwnedChild(const char * childName = nullptr);

    optional<marker> getScaledMarker(const char * section_name, const char * key);
    optional<marker> getTranslatedMarker(const char * section_name, const char * key);
    optional<marker> getCurrentMarker(const char * marker_name);
    const char * getCurrentAnimName();
    const char * getCurrentFrameName();
    bool IsPaused() {return orxObject_IsPaused(GetOrxObject());}

    virtual void OnCreate() final;
    virtual void ExtOnCreate() {};
    virtual void OnDelete(){};
    virtual void OnPause(bool pause) {}

    void SetTaggedAnim(const char * tag, bool current = false);
    const char * GetAnimTag(const char * anim_name = nullptr);
    const char * GetAnimName(const char * tag);

    template <class T> T GetValue(const char * key, bool shouldPush = true) {
        return ::GetValue<T>(shouldPush?GetModelName():nullptr, key);
    }

    // TODO check whether the created object is really an ExtendedMonomorphic
    static ExtendedMonomorphic * CreateNamed(const char * name) {
        orxOBJECT * newObject = orxObject_CreateFromConfig(name);
        return (ExtendedMonomorphic *)  orxObject_GetUserData(newObject);
    }
};

template <class DERIVING>
DERIVING * scroll_cast(orxOBJECT * orxObj, bool throw_on_incorrect_type = true) {
    if(orxObj==nullptr) return nullptr;
    auto object = (ExtendedMonomorphic *)  orxObject_GetUserData(orxObj);
    DERIVING * deriving = dynamic_cast<DERIVING *>(object);
    if(deriving || !throw_on_incorrect_type) return deriving;
    else throw std::runtime_error("Incorrect type assumed for section.");
}

template <class DERIVING>
DERIVING * Create() {
    orxOBJECT * newObject = orxObject_CreateFromConfig(GetBindName<DERIVING>());
    return (DERIVING *)  orxObject_GetUserData(newObject);
}

template <class DERIVING>
DERIVING * Create(const char * section) {
    orxOBJECT * newObject = orxObject_CreateFromConfig(section);
    return scroll_cast<DERIVING>(newObject);
}

template <class DERIVING>
DERIVING * ExtendedMonomorphic::FindOwnedChild(const char * childName) {
    if(childName) {
        auto orxObj = FindOwnedChild(childName);
        return scroll_cast<DERIVING>(orxObj);
    } else {
        for(auto child = orxObject_GetOwnedChild(GetOrxObject()); child!=nullptr; child = orxObject_GetOwnedSibling(child)) {
            if(auto found = scroll_cast<DERIVING>(child,false)) return found;
        }
        return nullptr;
    }
}

template<class T> T * ExtendedMonomorphic::GetAspect(typename T::AspectTag) {
    return dynamic_cast<T *>(this);
}

template<class T> T * GetAspect(orxOBJECT * obj, typename T::AspectTag = 0) {
    auto ud = orxObject_GetUserData(obj);
    if(ud == nullptr) return nullptr;
    auto extObj = static_cast<ExtendedMonomorphic *>(ud);
    return extObj->GetAspect<T>();
}

inline void SetContext(orxOBJECT * obj, orxOBJECT * ctx) {
    auto objData = orxObject_GetUserData(obj);
    auto ctxData = orxObject_GetUserData(ctx);
    if(objData && ctxData) {
        static_cast<ExtendedMonomorphic *>(objData)->SetContext(*static_cast<ExtendedMonomorphic *>(ctxData));
    }

}

template<class T> T * ExtendedMonomorphic::CreateChild(const char * section, bool set_group, bool in_frame) {
    if(!section) section = GetBindName<T>();
    orxOBJECT * child  = CreateChild(section,set_group, in_frame);
    return scroll_cast<T>(child);
}

orxOBJECT * CreateChild(orxOBJECT * parent, const char * section, bool set_group = false, bool in_frame = true);

template<class T = ExtendedMonomorphic> T * ExtractExtendedObject(orxOBJECT * obj) {
    ExtendedMonomorphic * data = (ExtendedMonomorphic *) orxObject_GetUserData(obj);
    if(data == nullptr) return nullptr;
    return dynamic_cast<T *>(data);
}

template<class DERIVING>
std::vector<DERIVING *> FindAllInOwnedChildren(orxOBJECT * parent) {
    std::vector<DERIVING *> result;
    for(auto child = orxObject_GetOwnedChild(parent);
             child != orxNULL;
             child = orxObject_GetOwnedSibling(child))
    {
        auto wanted_child = scroll_cast<DERIVING>(child, false);
        if(wanted_child) result.push_back(wanted_child);
    }
    return result;
}

template <class T>
class weak_scroll_ptr {
    weak_object_ptr ptr;
public:
    weak_scroll_ptr() {}
    weak_scroll_ptr(T * obj): ptr(obj->GetOrxObject()) {}
    T * get() {
        orxOBJECT * obj = ptr;
        return scroll_cast<T>(obj);
    }
    T & operator *() {return *get();}
    T * operator ->() {return get();}
    explicit operator bool() const {return bool(ptr);}
};


#endif /* EXTENDEDOBJECT_H_ */
