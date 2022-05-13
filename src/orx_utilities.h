/*
 * orx_utilities.h
 *
 *  Created on: May 8, 2015
 *      Author: enis
 */

#ifndef ORX_UTILITIES_H_
#define ORX_UTILITIES_H_

#include <vector>
#include <string>
#include <sstream>

#include <orx.h>

#include <util/optional.hpp>

inline orxVECTOR makeVector(orxFLOAT x,orxFLOAT y, orxFLOAT z) {
    orxVECTOR result;
    result.fX=x;
    result.fY=y;
    result.fZ=z;
    return result;
}

inline void SetPosition(orxOBJECT * obj, const orxVECTOR & pos) {
    orxObject_SetPosition(obj, &pos);
}

inline orxVECTOR GetPosition(orxOBJECT * obj) {
    orxVECTOR pos; orxObject_GetPosition(obj,&pos);
    return pos;
}

inline orxVECTOR GetWorldPosition(orxOBJECT * obj) {
    orxVECTOR pos; orxObject_GetWorldPosition(obj,&pos);
    return pos;
}

inline orxVECTOR GetSize(orxOBJECT * obj) {
    orxVECTOR objectSize; orxObject_GetSize(obj,&objectSize);
    orxVECTOR objectScale; orxObject_GetScale(obj, &objectScale);
    return {objectSize.fX*objectScale.fX, objectSize.fY*objectScale.fY,0};
}

inline void SetScale(orxOBJECT * obj, const orxVECTOR & scale) {
    orxObject_SetScale(obj, &scale);
}

inline void SetSize(orxOBJECT * obj, orxVECTOR size) {
    orxVECTOR objectSize;
    orxObject_GetSize(obj,&objectSize);

    orxVECTOR objectScale{size.fX/objectSize.fX, size.fY/objectSize.fY,0};

    orxObject_SetScale(obj,&objectScale);
}

inline void SetHeight(orxOBJECT * obj, orxFLOAT height) {
    orxVECTOR objectSize;
    orxObject_GetSize(obj,&objectSize);

    orxVECTOR objectScale{height/objectSize.fY, height/objectSize.fY,1};

    orxObject_SetScale(obj,&objectScale);
}

inline void SetWidth(orxOBJECT * obj, orxFLOAT width) {
    orxVECTOR objectSize;
    orxObject_GetSize(obj,&objectSize);

    orxVECTOR objectScale{width/objectSize.fX, width/objectSize.fX,1};

    orxObject_SetScale(obj,&objectScale);
}

inline orxVECTOR viewSize(orxCAMERA * cam) {
	orxFLOAT zoom = orxCamera_GetZoom(cam);
	orxAABOX frustum;
	orxCamera_GetFrustum(cam, &frustum);
	orxFLOAT viewHeight = frustum.vBR.fY-frustum.vTL.fY;
	orxFLOAT viewWidth = frustum.vBR.fX-frustum.vTL.fX;
	return {viewWidth/zoom, viewHeight/zoom, 0};
}

inline void SetRelativeHeight(const orxVECTOR & ref, orxOBJECT * obj, orxFLOAT height) {
    orxVECTOR size;
    orxObject_GetSize(obj, &size);
    orxFLOAT scale= height*ref.fY/(size.fY);
    orxVECTOR scaleV{scale,scale,0};
    orxObject_SetScale(obj, &scaleV);
}

inline void SetRelativeSize(const orxVECTOR & ref, orxOBJECT * obj, const orxVECTOR & relsize) {
    orxVECTOR size;
    orxObject_GetSize(obj, &size);
    orxVECTOR scaleV{relsize.fX*ref.fX/size.fX,relsize.fY*ref.fY/size.fY,0};
    orxObject_SetScale(obj, &scaleV);
}

inline void SetRelativeHeight(orxCAMERA * cam, orxOBJECT * obj, orxFLOAT height) {
    SetRelativeHeight(viewSize(cam),obj,height);
}

inline void SetRelativePosition(const orxVECTOR & ref, orxOBJECT * obj, const orxVECTOR & pos) {
    orxVECTOR worldPos{ref.fX*pos.fX, ref.fY*pos.fY,pos.fZ};
    orxObject_SetPosition(obj, &worldPos);
}
inline void SetRelativePosition(orxCAMERA * cam, orxOBJECT * obj, const orxVECTOR & pos) {
    auto viewS = viewSize(cam);
    SetRelativePosition(viewS,obj,pos);
}

inline orxOBJECT * GetOwner(orxOBJECT * obj) {
    auto owner = orxObject_GetOwner(obj);
    return owner ? orxOBJECT(owner) : nullptr;
}

inline orxOBJECT * FindOwnedChild(orxOBJECT * parent, const char * childName) {
    orxOBJECT * child = orxObject_GetOwnedChild(parent);
    while(child != orxNULL) {
       if(strcmp(childName, orxObject_GetName(child)) == 0) return child;
       child = orxObject_GetOwnedSibling(child);
    }
    return nullptr;
}


inline optional<orxVECTOR> getInWorld(orxVIEWPORT * viewport, const orxVECTOR & inView) {
    orxVECTOR result;
    orxVECTOR * found = orxRender_GetWorldPosition(&inView, viewport, &result);
    if(found) return {result};
    else return {};
}

inline optional<orxVECTOR> getMouseInWorld(orxVIEWPORT * viewport) {
    orxVECTOR mouseInView;
    if(!orxMouse_GetPosition(&mouseInView)) return {};
    return getInWorld(viewport, mouseInView);
}

inline std::vector<std::string> getConfigList(const char * key) {
    std::vector<std::string> result(orxConfig_GetListCounter(key));
    for(size_t i=0; i<result.size(); ++i) result[i] = orxConfig_GetListString(key,i);
    return result;
}

inline bool IsPressed(const char * input) {
    return orxInput_IsActive(input) && orxInput_HasNewStatus(input);
}

inline const char * getCurrentAnimName(orxOBJECT * obj) {
    auto animptr = orxOBJECT_GET_STRUCTURE(obj,ANIMPOINTER);
    return orxAnimPointer_GetCurrentAnimName(orxANIMPOINTER(animptr));
}

template <typename T>
std::string to_string(T value)
{
    std::ostringstream os ;
    os << value ;
    return os.str() ;
}

inline const char * getCurrentFrameName(orxOBJECT * obj) {
    auto animptr = orxOBJECT_GET_STRUCTURE(obj,ANIMPOINTER);
    auto animset = orxAnimPointer_GetAnimSet(animptr);
    auto anim = orxAnimSet_GetAnim(animset, orxAnimPointer_GetCurrentAnim(animptr));
    auto key = orxAnim_GetKeyCounter(anim);
    orxConfig_PushSection(orxAnim_GetName(anim));
    auto result = orxConfig_GetString(("KeyData"+to_string(key)).c_str());
    orxConfig_PopSection();
    return result;
}

inline void SetAlpha(orxOBJECT * obj, orxFLOAT alpha) {
    orxCOLOR color;
    if(orxObject_HasColor(obj)) orxObject_GetColor(obj, &color);
    else color.vRGB = {255,255,255};
    color.fAlpha = alpha;
    orxObject_SetColor(obj, &color);
}

inline orxVECTOR toObjectFrame(orxOBJECT * obj, orxVECTOR world_pos) {
    orxFrame_TransformPosition(orxOBJECT_GET_STRUCTURE(obj, FRAME), orxFRAME_SPACE_GLOBAL, &world_pos);
    return world_pos;
}

inline orxVECTOR toWorldFrame(orxOBJECT * obj, orxVECTOR obj_pos) {
    orxFrame_TransformPosition(orxOBJECT_GET_STRUCTURE(obj, FRAME), orxFRAME_SPACE_LOCAL, &obj_pos);
    return obj_pos;
}

static void deleteChildrenRecursive(orxOBJECT * obj) {
    for(auto child = orxObject_GetOwnedChild(obj); child; child = orxObject_GetOwnedSibling(child)) {
        orxObject_SetLifeTime(child, 0);
        deleteChildrenRecursive(child);
    }
}

inline void SetHSV(orxOBJECT * obj, const orxVECTOR & hsv) {
    orxCOLOR color;
    orxObject_GetColor(obj, &color);

    orxCOLOR colorHSV;
    colorHSV.fAlpha = color.fAlpha;

    colorHSV.vHSV = hsv;

    orxCOLOR colorRGB;
    orxColor_FromHSVToRGB(&colorRGB, &colorHSV);

    orxObject_SetColor(obj, &colorRGB);
}

static orxOBJECT * CreateSoundObject(const char * section, const orxVECTOR & position) {
    auto obj = orxObject_CreateFromConfig(section);
    orxObject_SetPosition(obj, &position);
    auto sound = orxObject_GetLastAddedSound(obj);
    orxObject_SetLifeTime(obj,orxSound_GetDuration(sound));
    return obj;
}

static const char * Localize(const char * str) {
    if(str[0] == '$') return orxLocale_GetString(str+1);
    else return str;
}

static const char * LocaleAwareString(const char * key) {
    return Localize(orxConfig_GetString(key));
}

#endif /* ORX_UTILITIES_H_ */
