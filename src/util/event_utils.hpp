/*
 * event_utils.hpp
 *
 *  Created on: Dec 22, 2015
 *      Author: enis
 */

#ifndef UTIL_EVENT_UTILS_HPP_
#define UTIL_EVENT_UTILS_HPP_

#include <scroll_ext/eventdispatcher.h>
#include <scroll_ext/ExtendedObject.h>
#include "orx_pointers.hpp"
#include <utility>

template<class DERIVING>
std::function<DERIVING *()> get_weak_accessor(orxOBJECT * obj) {
    weak_object_ptr ptr{obj};
    return [ptr]() {
        orxOBJECT * obj = ptr;
        return scroll_cast<DERIVING>(obj, false);
    };
}

template <class SIGNATURE> struct event_handler_base;

template <class RET, class ...ARGS>
struct event_handler_base<RET(ARGS...)> {
    virtual RET handle_event(ARGS...) = 0;
    template <class EVENT>
    handler_t<EVENT> to_handler_via_reference() {
        return [this](ARGS...args_in) {return handle_event(args_in...);};
    }
    virtual ~event_handler_base(){};
};
#endif /* UTIL_EVENT_UTILS_HPP_ */
