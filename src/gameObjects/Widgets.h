/*
 * Widgets.h
 *
 *  Created on: Nov 9, 2015
 *      Author: eba
 */

#ifndef GAMEOBJECTS_WIDGETS_H_
#define GAMEOBJECTS_WIDGETS_H_

#include "Button.h"

class ReloadStateButtons: public ExtendedMonomorphic {
public:
    SCROLL_BIND_CLASS("ReloadStateButtons")
    void ExtOnCreate();
};

#endif /* GAMEOBJECTS_WIDGETS_H_ */
