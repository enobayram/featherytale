/*
 * Button.cpp
 *
 *  Created on: Jul 27, 2015
 *      Author: enis
 */

#include "Button.h"
#include <gestures.h>

gesture* Button::clicked(trail & t) {
    if(handler && !IsPaused() && !disabled && GetLifeTime() != 0) {
        return handler(&t);
    } else return nullptr;
}

void Button::set_handler(std::function<void()> handler) {
    this->handler = [handler](trail *){handler(); return new null_gesture();};
}

void Button::set_handler(std::function<gesture *(trail &)> handler) {
    this->handler = [handler](trail * t) -> gesture * {
        if(!t) return nullptr; else return handler(*t);
    };
}

Button::Button() {
    const char * trackname = orxConfig_HasValue("ClickTrack")  ?
                             orxConfig_GetString("ClickTrack") : orxNULL;
                             
    set_handler([this, trackname]{
        if(trackname) {
            AddTrack(trackname);
        }
    });
}

void Button::set_disabled(bool disabled) {
	this->disabled = disabled;
}
