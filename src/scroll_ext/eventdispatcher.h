/*
 * eventdispatcher.h
 *
 *  Created on: Aug 22, 2015
 *      Author: enis
 */

#ifndef SCROLL_EXT_EVENTDISPATCHER_H_
#define SCROLL_EXT_EVENTDISPATCHER_H_

#include <map>
#include <vector>
#include <memory>
#include <functional>

#include <util/metaprogramming_utilities.hpp>

typedef void * event_index;
typedef int handler_index_base;

template <class Event>
struct handler_index {
    handler_index_base index;
};

// TODO write a blog about this
// TODO single event dispatcher that uses a simple std::vector
// TODO use event_handler as a base class for handlers for minimum call overhead.
//      in this configuration a child class callback_handler is used to register
//      any std::function type callbacks alongside other event_handler's.
//      in this approach, event_handler has a method bool memory_managed()
//      indicating whether the handler should be deleted with the dispatcher.
// TODO with the event_handler base, use event_handler * as handler_index so that
//      you don't need a std::pair<handler_index...>
// TODO provide a free register_handler(dispatcher, handler) function that can
//      choose the right dispatcher in a multi-dispatcher (or an omni-dispatcher) 
//      scenario.

template<class Event>
using handler_t = std::function<typename Event::signature>;

template <class Event>
using response_type = typename function_traits<typename Event::signature>::result_type;

template <class RESPONSE>
struct additive_fold {
	static RESPONSE fold(RESPONSE acc, RESPONSE in) {
		return acc+in;
	}
	static RESPONSE empty() {return RESPONSE{};}
};

template <class EVENT>
struct event_traits {
    static event_traits index_object;
    static void * const index;
    template<class EVENT_PROXY> static EVENT_PROXY get_fold_type(decltype(EVENT_PROXY::empty()) *, int);
    template<class EVENT_PROXY> static typename EVENT_PROXY::fold_type get_fold_type(int, int);
    template<class EVENT_PROXY> static additive_fold<response_type<EVENT>> get_fold_type(void *, char);
    typedef decltype(get_fold_type<EVENT>(0,0)) fold_type;
};

class type_erased_handler {
    event_index handled_event;
    std::shared_ptr<void> handler;
public:
    template <class Event>
    const handler_t<Event> & get_handler() {
        if(handled_event != event_traits<Event>::index) throw std::bad_function_call{};
        void * hp = handler.get();
        return * ((handler_t<Event> *) hp);
    }
    template <class Event>
    type_erased_handler(Event *, handler_t<Event> handler):
        handled_event(event_traits<Event>::index), handler((void *) new handler_t<Event>(std::move(handler)))
    {}
};

typedef std::pair<handler_index_base, type_erased_handler> indexed_handler;

class event_dispatcher {
    std::map<event_index, std::vector<indexed_handler>> handlers;
    handler_index_base register_handler(event_index, type_erased_handler &&);
    void remove_handler(event_index, handler_index_base);
public:
    typedef int AspectTag;
    template <class Event>
    handler_index<Event> register_handler(handler_t<Event> handler) {
        type_erased_handler te_handler{(Event *)0, std::move(handler)};
        return {register_handler(event_traits<Event>::index, std::move(te_handler))};
    }
    template <class Event>
    void remove_handler(handler_index<Event> hi) {
        remove_handler(event_traits<Event>::index, hi.index);
    }
    template <class Event, class ...ARGS>
    void fire_event(const ARGS & ... args) {
        auto it = handlers.find(event_traits<Event>::index);
        if(it==handlers.end()) return;
        for(indexed_handler & handler: it->second) {
            handler.second.get_handler<Event>()(args...);
        }
    }
    template <class Event, class Fold = typename event_traits<Event>::fold_type, class ...ARGS>
    response_type<Event> query(const ARGS & ...args) {
        auto it = handlers.find(event_traits<Event>::index);
        auto result = Fold::empty();
        if(it==handlers.end()) return result;
        for(indexed_handler & handler: it->second) {
            result = Fold::fold(result,handler.second.get_handler<Event>()(args...));
        }
        return result;
    }
};

template <class EVENT>
event_traits<EVENT> event_traits<EVENT>::index_object;

template <class EVENT>
void * const event_traits<EVENT>::index = &event_traits<EVENT>::index_object;

using dispatcher_accessor = std::function<event_dispatcher *()>;

template<class Event>
class event_listener_guard {
    handler_index<Event> index{};
    dispatcher_accessor accessor;
public:
    template<class Handler>
    void register_handler(event_dispatcher & ed, dispatcher_accessor accessor, Handler h) {
        attempt_deregister();
        index = ed.register_handler<Event>(h);
        this->accessor = accessor;
    }
    ~event_listener_guard() {
        attempt_deregister();
    }
    void attempt_deregister() {
        if(accessor) {
            auto dispatcher = accessor();
            if(dispatcher) dispatcher->remove_handler(index);
            accessor = {}; // so that the index is not deregistered again
        }
    }
};

// A generic event for signalling a condition
struct signal_event {
    using signature = void(unsigned int);
};

void SendSignal(event_dispatcher * dispatcher, const char * signal);

#endif /* SCROLL_EXT_EVENTDISPATCHER_H_ */
