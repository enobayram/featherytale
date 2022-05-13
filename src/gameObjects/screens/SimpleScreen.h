/*
 * SimpleScreen.h
 *
 *  Created on: Nov 8, 2015
 *      Author: eba
 */

#ifndef GAMEOBJECTS_SCREENS_SIMPLESCREEN_H_
#define GAMEOBJECTS_SCREENS_SIMPLESCREEN_H_

#include <memory>
#include <scroll_ext/ExtendedObject.h>
#include "../aspects.h"

class gesture_manager;

class SimpleScreen: public ExtendedMonomorphic, public game_screen {
    const char * next_screen = nullptr;
    orxVIEWPORT * viewport;
    std::unique_ptr<gesture_manager> manager;
public:
    SCROLL_BIND_CLASS("SimpleScreen")
    SimpleScreen();
    ~SimpleScreen();
    virtual void ExtOnCreate();
    virtual void EndScreen();
    virtual const char * GetNextScreen();
    virtual const char * GetScreenName() {return GetModelName();}
    virtual void SetNextScreen(const char * screen);
    virtual gesture_manager & GetGestureManager();
    void SetOutputTexture(orxTEXTURE*);
};

orxVIEWPORT * createAdjustedViewport(const char * section);

#endif /* GAMEOBJECTS_SCREENS_SIMPLESCREEN_H_ */
