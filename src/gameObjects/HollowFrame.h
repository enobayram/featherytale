/*
 * HollowFrame.h
 *
 *  Created on: Dec 7, 2015
 *      Author: eba
 */

#ifndef GAMEOBJECTS_HOLLOWFRAME_H_
#define GAMEOBJECTS_HOLLOWFRAME_H_

#include <scroll_ext/ExtendedObject.h>

class HollowFrame: public ExtendedMonomorphic {
    orxFLOAT hThickness, vThickness;
public:
    SCROLL_BIND_CLASS("HollowFrame")
    void ExtOnCreate();
    virtual orxBOOL OnRender(orxRENDER_EVENT_PAYLOAD *_pstPayload);
};

#endif /* GAMEOBJECTS_HOLLOWFRAME_H_ */
