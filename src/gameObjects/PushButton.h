/*
 * PushButton.h
 *
 *  Created on: Dec 8, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_PUSHBUTTON_H_
#define GAMEOBJECTS_PUSHBUTTON_H_

#include "Button.h"

class PushButton: public Button {
	friend class PushButtonGesture;
	const char * activation_key = nullptr;
    SCROLL_BIND_CLASS("PushButton")
public:
    constexpr static const char * CONFIG_NAME = "PushButton";
    void ExtOnCreate();
    gesture * clicked(trail &);
    void set_disabled(bool disabled);
    virtual void Update(const orxCLOCK_INFO &);
    virtual void OnNewAnim(const orxSTRING _zOldAnim, const orxSTRING _zNewAnim, orxBOOL _bCut);
};

#endif /* GAMEOBJECTS_PUSHBUTTON_H_ */
