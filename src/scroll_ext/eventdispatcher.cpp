/*
 * eventdispatcher.cpp
 *
 *  Created on: Aug 22, 2015
 *      Author: enis
 */

#include <scroll_ext/eventdispatcher.h>
#include <algorithm>
#include <orx.h>

handler_index_base event_dispatcher::register_handler(event_index ei, type_erased_handler&& te_handler) {
    auto & handlers = this->handlers[ei];
    handler_index_base new_idx;
    if(handlers.empty()) new_idx = 0;
    else {
        handler_index_base last_idx = handlers.back().first;
        new_idx = last_idx+1;
    }
    handlers.emplace_back(new_idx, std::move(te_handler));
    return new_idx;
}

void event_dispatcher::remove_handler(event_index ei, handler_index_base hi) {
    auto & handlers = this->handlers[ei];
    handlers.erase(std::remove_if(begin(handlers), end(handlers),[hi](const indexed_handler & ih) {
        return ih.first==hi;
    }));
    if(handlers.empty()) this->handlers.erase(ei);
}

void SendSignal(event_dispatcher* dispatcher, const char* signal) {
    dispatcher->fire_event<signal_event>(orxString_GetID(signal));
}
