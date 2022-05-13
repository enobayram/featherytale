/*
 * aspects.hpp
 *
 *  Created on: Aug 10, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_ASPECTS_HPP_
#define GAMEOBJECTS_ASPECTS_HPP_

#include "aspects.h"

#include <util/orx_pointers.hpp>
#include <orxFSM/behavior_header.hpp>
#include <gameObjects/Button.h>

class wait_for_button_click_behavior : public behavior_t {
    button_container * container;
    Button * button;
    weak_object_ptr button_ptr, container_ptr;
    bool clicked = false;
public:
    wait_for_button_click_behavior(button_container * container, std::string section, int xPos=0): container(container) {
        orxOBJECT * obj = orxObject_CreateFromConfig(section.c_str());
        button = ExtractExtendedObject<Button>(obj);
        button_ptr = obj;
        if(container && button) {
            container_ptr = dynamic_cast<ExtendedMonomorphic *>(container)->GetOrxObject();
            button->set_handler([=]() {clicked=true;});
            container->AddButton(obj,xPos);
        }
    }

    behavior_state run(const orxCLOCK_INFO & clock) {
        if(!container || !button) return FAILED;
        if(clicked) return SUCCEEDED;
        return RUNNING;
    }
    ~wait_for_button_click_behavior() {
        if(button_ptr) {
            button->clear_handler();
            if(container_ptr) container->RemoveButton(button->GetOrxObject());
        }
    }
};
BEHAVIOR_LEAF(wait_for_button_click, wait_for_button_click_behavior)
ACTION_LEAF(add_button, &button_container::AddButton)

ACTION_LEAF(set_next_screen, &game_screen::SetNextScreen)

#endif /* GAMEOBJECTS_ASPECTS_HPP_ */
