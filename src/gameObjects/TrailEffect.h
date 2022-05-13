/*
 * TrailEffect.h
 *
 *  Created on: Aug 7, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_TRAILEFFECT_H_
#define GAMEOBJECTS_TRAILEFFECT_H_

#include <scroll_ext/ExtendedObject.h>

class trail;

class TrailEffect: public ExtendedMonomorphic {
public:
    const trail * t = nullptr;
    SCROLL_BIND_CLASS("TrailEffect")
    TrailEffect();
    virtual ~TrailEffect();
    virtual orxBOOL OnRender(orxRENDER_EVENT_PAYLOAD *_pstPayload);
};

#endif /* GAMEOBJECTS_TRAILEFFECT_H_ */
