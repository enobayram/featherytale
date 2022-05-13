/*
 * event_behaviors.hpp
 *
 *  Created on: Dec 27, 2015
 *      Author: eba
 */

#ifndef ORXFSM_EVENT_BEHAVIORS_HPP_
#define ORXFSM_EVENT_BEHAVIORS_HPP_

#include <util/event_utils.hpp>
#include <orxFSM/behavior_header.hpp>

template <class Event>
struct event_behavior: behavior_t, event_handler_base<typename Event::signature> {
    using behavior_base = event_behavior<Event>;
    using handler_base = event_handler_base<typename Event::signature> ;
    event_listener_guard<Event> lg;
    behavior_state returned_state = RUNNING;
    event_behavior(orxOBJECT * event_dispatcher_obj) {
        auto ed = scroll_cast<event_dispatcher>(event_dispatcher_obj, true);
        lg.register_handler(*ed,
                            get_weak_accessor<event_dispatcher>(event_dispatcher_obj),
                            handler_base::template to_handler_via_reference<Event>());
    }
    virtual behavior_state run(const orxCLOCK_INFO &) {
        return returned_state;
    }
};

template <class Query, class Signature> struct query_responder_impl;
template <class Query, class RET, class ... ARGS>
struct query_responder_impl<Query, RET(ARGS...)>: event_behavior<Query> {
    typedef in_out_variable<RET> signal_t;
    in_out_variable<RET> signal;
    query_responder_impl(orxOBJECT * ed, in_out_variable<RET> signal): event_behavior<Query>(ed), signal(signal) {}
    RET handle_event(ARGS...args) {
        return signal;
    }
};

template <class Query>
struct query_responder: query_responder_impl<Query, typename Query::signature> {
    typedef query_responder_impl<Query, typename Query::signature> impl;
    query_responder(orxOBJECT * ed, typename impl::signal_t signal): impl(ed, signal){}
};

struct wait_for_signal_behavior: event_behavior<signal_event> {
    orxU32 signal;
    bool inverse;
    wait_for_signal_behavior(orxOBJECT * dispatcher, const char * signal, bool inverse = false):
        behavior_base(dispatcher), signal(orxString_GetID(signal)), inverse(inverse){}
    void handle_event(unsigned int signal) {
        if((signal == this->signal) != inverse) returned_state = SUCCEEDED;
    }
};
BEHAVIOR_LEAF(wait_for_signal, wait_for_signal_behavior)

ACTION_LEAF(send_signal, SendSignal)

#endif /* ORXFSM_EVENT_BEHAVIORS_HPP_ */
