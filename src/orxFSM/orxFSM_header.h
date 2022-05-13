/*
 * orxFSM_header.h
 *
 *  Created on: Jun 1, 2015
 *      Author: eba
 */

#ifndef ORXFSM_HEADER_H_
#define ORXFSM_HEADER_H_

#include <memory>

#include <orxFSM/behavior_header.hpp>
#include <util/preprocessor_foreach.h>
#include <gameObjects/aspects.h>

namespace std {
    template <class Signature> class function;
}

template <class SELF>
struct transition_table;

template <class Self>
class state_t;

template<class Self>
using state_constructor = std::function<state_t<Self> *(Self &)>;

template <class Self>
class transition_constraints;

template <class ...States>
struct state_list {};

template<class SELF>
struct controller;

class state_machine {
public:
    typedef int AspectTag;
    virtual void SetConstraints(const char* constraints_section) = 0;
    virtual void SetController(const char* controller_name) = 0;
    virtual const char * GetController() = 0;
    virtual int  GetStateID(const char* state_name) = 0;
    virtual int  GetCurrentStateID() = 0;
    virtual ~state_machine(){}
};
ACTION_LEAF(set_constraints, &state_machine::SetConstraints)
ACTION_LEAF(set_controller, &state_machine::SetController)

inline const char * get_controller_action(state_machine * sm) {return sm->GetController();}
RETURNING_ACTION_LEAF(get_controller, get_controller_action)

template<class DERIVING, class BASE = ExtendedMonomorphic>
class state_machine_mixin: public BASE, public state_machine {
public:
    typedef BASE state_machine_base;
    DERIVING & get_deriving() {return static_cast<DERIVING &>(*this);}
    state_machine_mixin();
    ~state_machine_mixin();
    void SetConstraints(const char* constraints_section);
    void SetController(const char* controller_name);
    const char * GetController();
    int  GetStateID(const char* state_name);
    int  GetCurrentStateID();
    state_constructor<DERIVING> GetInitialState();
protected:
    std::unique_ptr<state_t<DERIVING>> curstate;
    std::unique_ptr<transition_constraints<DERIVING>> curConstraints;
    std::unique_ptr<controller<DERIVING>> curcontroller;
    void Update(const orxCLOCK_INFO &);
    virtual void ExtOnCreate();
};

class wait_for_state_behavior: public behavior_t {
    state_machine & sm;
    int state_id;
public:
    wait_for_state_behavior(orxOBJECT * obj, const char * state_name): sm(*GetAspect<state_machine>(obj)), state_id(sm.GetStateID(state_name)){}
    behavior_state run(const orxCLOCK_INFO & clock) {
        return sm.GetCurrentStateID()==state_id ? SUCCEEDED : RUNNING;
    }
};
BEHAVIOR_LEAF(wait_for_state, wait_for_state_behavior)

#define TOCLASS(x) class x;

#define ORX_STATES(...) \
        MAP(TOCLASS, __VA_ARGS__) \
        struct states {enum {__VA_ARGS__};}; \
        typedef state_list<__VA_ARGS__> state_types;

#endif /* ORXFSM_HEADER_H_ */
