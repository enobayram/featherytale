/*
 * KnifeTool.h
 *
 *  Created on: Aug 6, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_KNIFETOOL_H_
#define GAMEOBJECTS_KNIFETOOL_H_

#include <util/orx_pointers.hpp>
#include "Tool.h"

class Button;
class World;

class KnifeTool: public Tool {
    weak_object_ptr gesture_surface;
    World * world = nullptr;
    friend class cut_gesture;
public:
    SCROLL_BIND_CLASS("KnifeTool")
    void SetSelected(bool selected);
    virtual void SetContext(ExtendedMonomorphic & context);
};

#endif /* GAMEOBJECTS_KNIFETOOL_H_ */
