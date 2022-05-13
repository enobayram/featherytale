/*
 * StoryModePage.h
 *
 *  Created on: Dec 6, 2015
 *      Author: eba
 */

#ifndef GAMEOBJECTS_SCREENS_STORYMODEPAGE_H_
#define GAMEOBJECTS_SCREENS_STORYMODEPAGE_H_

#include "SimpleScreen.h"

class StoryModePage: public SimpleScreen {
public:
    SCROLL_BIND_CLASS("StoryModePage")
    void ExtOnCreate();
};

#endif /* GAMEOBJECTS_SCREENS_STORYMODEPAGE_H_ */
