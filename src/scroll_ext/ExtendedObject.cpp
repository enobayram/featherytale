/*
 * ExtendedObject.cpp
 *
 *  Created on: Mar 28, 2015
 *      Author: eba
 */

#include <scroll_ext/ExtendedObject.h>
#include <orx_utilities.h>
#include <vector>

optional<marker> ExtendedMonomorphic::getScaledMarker(const char * section_name, const char * key){
    marker result;
    orxVECTOR marker_values;
    bool has_marker;
    in_section(section_name, [&]{
        has_marker = orxConfig_HasValue(key);
        if(has_marker) orxConfig_GetVector(key,&marker_values);
    });
    if(!has_marker) return {};
    auto scale = GetScale();
    result.center.fX = marker_values.fX * scale.fX;
    result.center.fY =marker_values.fY * scale.fY;
    result.center.fZ = 0;
    result.radius =marker_values.fZ * scale.fX; // This is not a bug, the radius of the marker is scaled by X
    return result;
}

optional<marker> ExtendedMonomorphic::getTranslatedMarker(const char * section_name, const char * key){
    optional<marker> result = getScaledMarker(section_name, key);
    if(!result) return {};
    auto pos = GetPosition();
    result->center.fX += pos.fX;
    result->center.fY += pos.fY;
    return result;
}

void ExtendedMonomorphic::SetSize(const orxVECTOR & size) {
    ::SetSize(GetOrxObject(), size);
}

void ExtendedMonomorphic::SetHeight(orxFLOAT height) {
	::SetHeight(GetOrxObject(), height);
}

void ExtendedMonomorphic::SetWidth(orxFLOAT width) {
    ::SetWidth(GetOrxObject(), width);
}

orxVECTOR getRef(ExtendedMonomorphic & obj) {
    orxASSERT(obj.referenceCamera!=nullptr || obj.referenceSection != nullptr);
    orxVECTOR ref;
    if(obj.referenceCamera) ref = viewSize(obj.referenceCamera);
    if(obj.referenceSection) {
        orxConfig_PushSection(obj.referenceSection);
        ref = {orxConfig_GetFloat("Width"),orxConfig_GetFloat("Height"),0};
        orxConfig_PopSection();
    }
    return ref;
}

void ExtendedMonomorphic::SetRelativeHeight(orxFLOAT height) {
	::SetRelativeHeight(getRef(*this), GetOrxObject(), height);
}
void ExtendedMonomorphic::SetRelativePosition(const orxVECTOR & pos) {
    ::SetRelativePosition(getRef(*this), GetOrxObject(), pos);
}

orxVECTOR ExtendedMonomorphic::GetSize() {
    return ::GetSize(GetOrxObject());
}

const char* ExtendedMonomorphic::getCurrentAnimName() {
    return ::getCurrentAnimName(GetOrxObject());
}

const char* ExtendedMonomorphic::getCurrentFrameName() {
    return ::getCurrentFrameName(GetOrxObject());
}

void ExtendedMonomorphic::OnCreate() {
	if(orxConfig_HasValue("ReferenceCamera")) referenceCamera = orxCamera_Get(orxConfig_GetString("ReferenceCamera"));
    if(orxConfig_HasValue("ReferenceSection")) referenceSection = orxConfig_GetString("ReferenceSection");
	if(orxConfig_HasValue("Height")) {
	    if(orxConfig_HasValue("Width")) SetSize({orxConfig_GetFloat("Width"), orxConfig_GetFloat("Height"),0});
	    else SetHeight(orxConfig_GetFloat("Height"));
	} else if(orxConfig_HasValue("Width")) {
	    SetWidth(orxConfig_GetFloat("Width"));
	}
	if(orxConfig_HasValue("RelativeHeight")) SetRelativeHeight(orxConfig_GetFloat("RelativeHeight"));
    if(orxConfig_HasValue("RelativeSize")) SetRelativeSize(getRef(*this), GetOrxObject(), GetValue<orxVECTOR>("RelativeSize",false));
	if(orxConfig_HasValue("RelativePosition")) {
		orxVECTOR relPos;
		SetRelativePosition(*orxConfig_GetVector("RelativePosition",&relPos));
	}
	if(orxConfig_HasValue("Size")) {
	    orxVECTOR size; orxConfig_GetVector("Size",&size);
	    SetSize(size);
	}
	for(orxOBJECT * child = orxObject_GetOwnedChild(GetOrxObject()); child; child = orxObject_GetOwnedSibling(child)) {
	    ::SetContext(child, GetOrxObject());
	}
	ExtOnCreate();
}

orxOBJECT* ExtendedMonomorphic::FindOwnedChild(const char* childName) {
    return ::FindOwnedChild(GetOrxObject(), childName);
}

optional<marker> ExtendedMonomorphic::getCurrentMarker(const char* marker_name) {
    return getTranslatedMarker(getCurrentFrameName(), marker_name);
}

orxOBJECT * ExtendedMonomorphic::CreateChild(const char * section, bool set_group, bool in_frame) {
    auto child = orxObject_CreateFromConfig(section);
    orxObject_SetOwner(child, GetOrxObject());
    if(in_frame) orxObject_SetParent(child, GetOrxObject());
    if(set_group) orxObject_SetGroupID(child, GetGroupID());
    if(auto extchild = ExtractExtendedObject(child)) extchild->SetContext(*this);
    return child;
}

orxOBJECT * CreateChild(orxOBJECT * parent, const char * section, bool set_group, bool in_frame) {
    void * data = orxObject_GetUserData(parent);
    if(data) return static_cast<ExtendedMonomorphic *>(data)->CreateChild(section, set_group, in_frame);
    else {
        auto child = orxObject_CreateFromConfig(section);
        orxObject_SetOwner(child, parent);
        if(in_frame) orxObject_SetParent(child, parent);
        if(set_group) orxObject_SetGroupID(child, orxObject_GetGroupID(parent));
        return child;
    }
}

void ExtendedMonomorphic::SetTaggedAnim(const char* tag, bool current) {
    SetAnim(GetAnimName(tag), current);
}

const char* ExtendedMonomorphic::GetAnimTag(const char* anim_name) {
    if(!anim_name) anim_name = getCurrentAnimName();
    ConfigSectionGuard guard(orxConfig_GetString("AnimationSet"));
    size_t nkeys = orxConfig_GetKeyCounter();;
    for(size_t i=0; i<nkeys; i++) {
        auto key = orxConfig_GetKey(i);
        auto value = orxConfig_GetString(key);
        if(strcmp(value, anim_name) == 0) return key;
    }
    return nullptr;
}

const char* ExtendedMonomorphic::GetAnimName(const char* tag) {
    ConfigSectionGuard guard(GetValue<const char *>("AnimationSet"));
    return orxConfig_GetString(tag);
}
