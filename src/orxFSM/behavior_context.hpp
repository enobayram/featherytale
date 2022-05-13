/*
 * behavior_context.hpp
 *
 *  Created on: Aug 10, 2015
 *      Author: enis
 */

#ifndef ORXFSM_BEHAVIOR_CONTEXT_HPP_
#define ORXFSM_BEHAVIOR_CONTEXT_HPP_

#include <memory>

#include <scroll_ext/ExtendedObject.h>
#include <gameObjects/aspects.h>

#include "behavior_header.hpp"

class behavior_context {
    std::map<orxU32, orxCOMMAND_VAR> context;
    std::map<orxU32, std::string> string_storage;
public:
    typedef int AspectTag;
    orxCOMMAND_VAR get(const char * key);
    void set(const char * key, orxCOMMAND_VAR var);
    void remove(const char * key);
    virtual ~behavior_context();
    template <class T> T get(const char * key);
    template <class T> void set(const char * key, T value);
    void set(const char * key, const std::string & value);
};
extern behavior_context global_context;

template <class DERIVING, class BASE = ExtendedMonomorphic>
class behavior_context_mixin: public BASE, public behavior_context {
public:
    void set_behavior(const behavior_constructor & new_behavior) {
        behavior.reset();
        if(new_behavior) behavior.reset(new_behavior(BASE::GetOrxObject()));
    }
protected:
    using MIXIN = behavior_context_mixin<DERIVING,BASE>;
    std::unique_ptr<behavior_t> behavior;
    void ExtOnCreate();
    void Update(const orxCLOCK_INFO & _rstInfo) {
        if(behavior) {
            auto result = behavior->run(_rstInfo);
            if(result != behavior_t::RUNNING) behavior.reset();
        }
    }
};


#endif /* ORXFSM_BEHAVIOR_CONTEXT_HPP_ */
