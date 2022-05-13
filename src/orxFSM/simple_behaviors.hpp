/*
 * simple_behaviors.hpp
 *
 *  Created on: Jul 24, 2015
 *      Author: enis
 */

#ifndef ORXFSM_SIMPLE_BEHAVIORS_HPP_
#define ORXFSM_SIMPLE_BEHAVIORS_HPP_

#include <iostream>
#include <memory>

#include "behavior.hpp"
#include <orx_utilities.h>

class const_behavior: public behavior_t {
    behavior_state st;
public:
    const_behavior(behavior_state st): st(st){}
    behavior_state run(const orxCLOCK_INFO & clock) {
        return st;
    }
};
BEHAVIOR_LEAF(constant, const_behavior)

inline behavior_constructor wait() {return constant(behavior_t::RUNNING);}

class timeout_behavior: public behavior_t {
    orxFLOAT remaining_time;
public:
    timeout_behavior(orxFLOAT duration): remaining_time(duration) {}
    behavior_state run(const orxCLOCK_INFO & clock) {
        remaining_time -= clock.fDT;
        return remaining_time <= 0 ? SUCCEEDED : RUNNING;
    }
};
BEHAVIOR_LEAF(timeout, timeout_behavior)

class poisson_wait_behavior: public behavior_t {
    in_out_variable<orxFLOAT> period;
public:
    poisson_wait_behavior(in_out_variable<orxFLOAT> period): period(period) {}
    behavior_state run(const orxCLOCK_INFO & clock);
};
BEHAVIOR_LEAF(poisson_wait, poisson_wait_behavior)

class tick_behavior: public behavior_t {
    bool ticked = false;
public:
    behavior_state run(const orxCLOCK_INFO &) {
        auto result = ticked?SUCCEEDED:RUNNING;
        ticked = true;
        return result;
    }
};
BEHAVIOR_LEAF(tick_, tick_behavior)
static auto tick = tick_();

inline orxVECTOR add_action(orxVECTOR arg1, orxVECTOR arg2) {
    orxVECTOR result;
    orxVector_Add(&result ,&arg1, &arg2);
    return result;
}
inline orxVECTOR add_action(orxOBJECT * arg1, orxVECTOR arg2) {
    return add_action(GetPosition(arg1), arg2);
}
RETURNING_ACTION_LEAF(add, add_action)

template <class T>
orxSTATUS increment_action(in_out_variable<T> var) {
    var = T(var) + 1;
    return orxSTATUS_SUCCESS;
}
COMPLEX_ACTION_LEAF(increment, increment_action)

template <class T>
orxSTATUS println_action(T val) {
    std::cout<<val<<std::endl;
    return orxSTATUS_SUCCESS;
}
COMPLEX_ACTION_LEAF(println, println_action)

inline void append_object_action(object_collection * collection, orxOBJECT * obj) {
    collection->objects.emplace_back(obj);
}
ACTION_LEAF(append_object, append_object_action)

orxSTATUS remove_object_action(object_collection * collection, orxOBJECT * obj);
ACTION_LEAF(remove_object, remove_object_action)

orxFLOAT random_in_range_action(orxFLOAT lower, orxFLOAT upper);
RETURNING_ACTION_LEAF(random_in_range, random_in_range_action)

template <class OUT>
orxSTATUS get_action(in_out_variable<OUT> out, behavior_context * context, const char * name) {
    auto var = context->get(name);
    if(var.eType != varTypeToEnum(type_carrier<OUT>{})) return orxSTATUS_FAILURE;
    out = arg_from_command_var(var, type_carrier<OUT>{});
    return orxSTATUS_SUCCESS;
};
template <class OUT, class DEFVAL>
orxSTATUS get_action(in_out_variable<OUT> out, behavior_context * context, const char * name, DEFVAL defval) {
    auto var = context->get(name);
    if(var.eType != varTypeToEnum(type_carrier<OUT>{})) out = defval;
    else out = arg_from_command_var(var, type_carrier<OUT>{});
    return orxSTATUS_SUCCESS;
};
COMPLEX_ACTION_LEAF(get, get_action);

template <class ARG>
void push_arg(std::stringstream & ss, const void *arg) {
    ss << *(ARG *)(arg);
}
typedef void (*push_func)(std::stringstream &, const void*);

std::string sprint_impl(const char * format, push_func pushers[], const void * args[], size_t count);

template <class FORMAT, class ...ARGS>
std::string sprint_action(FORMAT format_var, const ARGS & ... args) {
    static push_func pushers[] = {&push_arg<ARGS>...};
    const void * argps[] = {&args...};
    return sprint_impl(format_var, pushers, argps, sizeof...(args));
}
RETURNING_ACTION_LEAF(sprint, sprint_action);

inline bool not_action(bool in) {return !in;}
RETURNING_ACTION_LEAF(not_, not_action)

inline orxSTATUS collection_empty_action(object_collection * collection) {
    for(auto ptr: collection->objects) if(ptr) return orxSTATUS_FAILURE;
    return orxSTATUS_SUCCESS;
}
ACTION_LEAF(collection_empty, collection_empty_action)

inline orxSTATUS get_first_action(in_out_variable<orxOBJECT *> out, object_collection * collection) {
    for(auto ptr: collection->objects) if(ptr) {
        out = ptr;
        return orxSTATUS_SUCCESS;
    }
    return orxSTATUS_FAILURE;
}
ACTION_LEAF(get_first, get_first_action)

inline bool less_than_action(orxFLOAT left, orxFLOAT right) {
    return left < right;
}
RETURNING_ACTION_LEAF(less_than, less_than_action)

class wait_for_true_behavior: public behavior_t {
    in_out_variable<bool> condition;
public:
    wait_for_true_behavior(in_out_variable<bool> condition): condition(condition) {}
    behavior_state run(const orxCLOCK_INFO &) {
        return condition?SUCCEEDED:RUNNING;
    }
};
BEHAVIOR_LEAF(wait_for_true, wait_for_true_behavior)

class wait_for_deletion_behavior: public behavior_t {
    orxU64 id;
public:
    wait_for_deletion_behavior(orxOBJECT * obj): id(orxStructure_GetGUID(obj)) {}
    behavior_state run(const orxCLOCK_INFO &) {
        return orxStructure_Get(id)?RUNNING:SUCCEEDED;
    }
};
BEHAVIOR_LEAF(wait_for_deletion, wait_for_deletion_behavior)

class config_function;

class output_signal_behavior: public behavior_t {
    double time = 0;
    in_out_variable<orxFLOAT> out;
    std::unique_ptr<config_function> signal;
public:
    output_signal_behavior(in_out_variable<orxFLOAT> out, const char * section);
    behavior_state run(const orxCLOCK_INFO &);
};
BEHAVIOR_LEAF(output_signal, output_signal_behavior)

struct count_behavior: behavior_t {
    in_out_variable<orxFLOAT> out;
    orxFLOAT rate;
    optional<orxFLOAT> until;
    count_behavior(in_out_variable<orxFLOAT> out, orxFLOAT rate=1): out(out), rate(rate), until({}) {}
    count_behavior(in_out_variable<orxFLOAT> out, orxFLOAT rate, orxFLOAT until): out(out), rate(rate), until(until) {}
    behavior_state run(const orxCLOCK_INFO & clock) {
        float curval = out;
        if(until && rate>0 && curval>=*until) return SUCCEEDED;
        if(until && rate<0 && curval<=*until) return SUCCEEDED;
        out = curval + clock.fDT * rate;
        return RUNNING;
    }
};
BEHAVIOR_LEAF(count,count_behavior)

#endif /* ORXFSM_SIMPLE_BEHAVIORS_HPP_ */
