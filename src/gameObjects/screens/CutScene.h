/*
 * CutScene.h
 *
 *  Created on: Nov 8, 2015
 *      Author: eba
 */

#ifndef GAMEOBJECTS_CUTSCENE_H_
#define GAMEOBJECTS_CUTSCENE_H_

#include "SimpleScreen.h"

class CutScene: public SimpleScreen {
    orxOBJECT * curSlide = orxNULL;
    orxVIEWPORT * slideViewport = orxNULL;
public:
    SCROLL_BIND_CLASS("CutScene")
    void EndCutscene();
    void Update(const orxCLOCK_INFO &);
    orxOBJECT * NewSlide(const char * slide, orxFLOAT lifetime);
    void ExtOnCreate();
    void OnDelete();
};

#endif /* GAMEOBJECTS_CUTSCENE_H_ */
