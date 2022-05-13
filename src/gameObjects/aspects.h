/*
 * aspects.h
 *
 *  Created on: Jun 13, 2015
 *      Author: eba
 */

#ifndef ASPECTS_H_
#define ASPECTS_H_

#include <orx.h>

class gesture_manager;
class gesture;
class trail;

class click_handler {
public:
    typedef int AspectTag;
    virtual gesture * clicked(trail &) = 0;
    virtual ~click_handler(){}
};

struct button_container {
    typedef int AspectTag;
    virtual void AddButton(orxOBJECT *, orxFLOAT xPos) = 0;
    virtual void RemoveButton(orxOBJECT *) = 0;
    virtual ~button_container(){}
};

struct game_screen {
    typedef int AspectTag;
    virtual void EndScreen() = 0;
    virtual const char * GetNextScreen() = 0;
    virtual void SetNextScreen(const char * l) = 0;
    virtual gesture_manager & GetGestureManager() = 0;
    virtual void SetOutputTexture(orxTEXTURE *) = 0;
    virtual ~game_screen(){}
    virtual const char * GetScreenName() = 0;
};

#endif /* ASPECTS_H_ */
