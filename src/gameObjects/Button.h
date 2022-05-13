/*
 * Button.h
 *
 *  Created on: Jun 27, 2015
 *      Author: eba
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <functional>

#include <scroll_ext/ExtendedObject.h>

#include <gameObjects/aspects.h>

class Button: public ExtendedMonomorphic, public click_handler {
protected:
    std::function<gesture *(trail *)> handler;
    bool disabled = false;
public:
    SCROLL_BIND_CLASS("Button")
    Button();
    gesture * clicked(trail &);
    void set_handler(std::function<void()> handler);
    void set_handler(std::function<gesture*(trail &)> handler);
    void set_handler(std::function<gesture*(trail *)> handler) {this->handler = handler;}
    void clear_handler() {handler = {};}
    virtual void set_disabled(bool disabled);
};

#endif /* BUTTON_H_ */
