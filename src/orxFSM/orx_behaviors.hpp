/*
 * orx_behaviors.hpp
 *
 *  Created on: Jul 24, 2015
 *      Author: enis
 */

#ifndef ORXFSM_ORX_BEHAVIORS_HPP_
#define ORXFSM_ORX_BEHAVIORS_HPP_

#include <sstream>

#include <orx.h>

#include "behavior.hpp"
#include <orx_utilities.h>
#include "behavior_combinators.hpp"

RETURNING_ACTION_LEAF(get_position, GetPosition)
ACTION_LEAF(set_target_anim, orxObject_SetTargetAnim)
ACTION_LEAF(add_track, orxObject_AddTimeLineTrack)
ACTION_LEAF(add_sound, orxObject_AddSound)
ACTION_LEAF(screenshot_capture, orxScreenshot_Capture)
ACTION_LEAF(set_text, orxObject_SetTextString)
ACTION_LEAF(set_lifetime, orxObject_SetLifeTime)
RETURNING_ACTION_LEAF(get_name, orxObject_GetName)
ACTION_LEAF(console_log, orxConsole_Log)
RETURNING_ACTION_LEAF(locale_aware_string, LocaleAwareString)

inline orxSTATUS is_input_active_action(const char * input) {
    return orxInput_IsActive(input) ? orxSTATUS_SUCCESS : orxSTATUS_FAILURE;
}
ACTION_LEAF(is_input_active, is_input_active_action)

inline orxOBJECT * get_owner_action(orxOBJECT * obj) {return orxOBJECT(orxObject_GetOwner(obj));}
RETURNING_ACTION_LEAF(get_owner, get_owner_action)

class move_behavior: public object_behavior_mixin<move_behavior> {
    orxVECTOR target;
    orxFLOAT speed;
public:
    move_behavior(orxOBJECT * obj, orxVECTOR target, orxFLOAT speed): object_behavior_mixin(obj), target(target), speed(speed){}
    move_behavior(orxOBJECT * obj, orxOBJECT * target, orxFLOAT speed): move_behavior(obj, GetPosition(target), speed){}
    move_behavior(orxOBJECT * obj, orxOBJECT * target, orxVECTOR offset, orxFLOAT speed):
        move_behavior(obj, GetPosition(target)+offset, speed){}
    virtual behavior_state run_object(const orxCLOCK_INFO &);
};
BEHAVIOR_LEAF(move, move_behavior)

class fade_behavior: public object_behavior_mixin<fade_behavior> {
    orxFLOAT period;
    orxFLOAT alpha_change;
    orxFLOAT elapsed = 0;
public:
    fade_behavior(orxOBJECT * obj, orxFLOAT period, orxFLOAT alpha_change = -1):
        object_behavior_mixin(obj), period(period), alpha_change(alpha_change){}
    behavior_state run_object(const orxCLOCK_INFO &);
};
BEHAVIOR_LEAF(fade, fade_behavior)

orxSTATUS get_marker_action(in_out_variable<orxVECTOR> item, ExtendedMonomorphic * obj, std::string marker_name);
ACTION_LEAF(get_marker, get_marker_action)

orxSTATUS create_action(in_out_variable<orxOBJECT *> item, std::string section, bool replace = true);
COMPLEX_ACTION_LEAF(create, create_action)

orxSTATUS create_in_collection_action(object_collection * collection,
                                      in_out_variable<orxOBJECT *> item,
                                      std::string section,
                                      bool replace = true, bool ignore_if_null=false);
COMPLEX_ACTION_LEAF(create_in_collection, create_in_collection_action)

orxSTATUS destroy_action(in_out_variable<orxOBJECT *> item, bool fail_if_missing = false);
COMPLEX_ACTION_LEAF(destroy, destroy_action)


template <class ARG> void push_to_vector(std::vector<std::string> & v, ARG & arg) {}
template <class ARG> void push_to_vector(std::vector<std::string> & v, extractor<ARG> arg) { v.push_back(arg.key);}

template <class T>
inline void stream_argument(std::ostream & os, const T & arg) { os << " " << arg;}

template <class T>
inline void stream_argument(std::ostream & os, const extractor<T> & arg) {os << " <";}

class execute_command_behavior: public behavior_t {
    const char * key;
    std::string command;
    std::string precommand;
    orxOBJECT * context;
    std::vector<std::string> push_list;
public:
    execute_command_behavior(orxOBJECT * context, const char * key, std::string command, std::vector<std::string> push_list, std::string precommand = "")
        : key(Persist(key)), command(precommand+command), context(context), push_list(std::move(push_list)) {}
    behavior_state run(const orxCLOCK_INFO & clock);
};

template <class OBJ, class OUTPUT_KEY, class COMMAND_STR, class ...ARGS>
behavior_constructor execute_command(OBJ context, OUTPUT_KEY key, COMMAND_STR command, const ARGS ... args ) {
    std::stringstream command_stream;
    if(!arg_traits<COMMAND_STR>::IS_CONTEXTUAL) {
        command_stream << command;
    }
    std::initializer_list<int>{(stream_argument(command_stream,args),0)...}; // push arguments to the stream

    std::vector<std::string> contextuals_reverse;
    std::initializer_list<int>{(push_to_vector(contextuals_reverse, args),0)...}; // push contextuals

    // Reverse the contextuals, since they will be pushed to a stack.
    std::vector<std::string> contextuals{contextuals_reverse.rbegin(), contextuals_reverse.rend()};

    if(!arg_traits<COMMAND_STR>::IS_CONTEXTUAL) {
        return direct_constructor<execute_command_behavior>(context, key, command_stream.str(), contextuals);
    } else {
        return direct_constructor<execute_command_behavior>(context, key, command_stream.str(), contextuals, command);
    }
}

template <class OUT>
orxSTATUS config_get_action(in_out_variable<OUT> out, const std::string & section, const std::string & key, orxU32 index = -1) {
    out = GetValue<OUT>(section.c_str(), key.c_str(), index);
    return orxSTATUS_SUCCESS;
}
template <class OUT>
orxSTATUS config_get_action(in_out_variable<OUT> out, orxOBJECT * obj, const std::string & key, orxU32 index = -1) {
    out = GetValue<OUT>(orxObject_GetName(obj), key.c_str(), index);
    return orxSTATUS_SUCCESS;
}
COMPLEX_ACTION_LEAF(config_get, config_get_action)

RETURNING_ACTION_LEAF(find_owned_child, FindOwnedChild)

struct attach_to_object_behavior: object_behavior_mixin<attach_to_object_behavior> {
    using MIXIN::MIXIN;
    behavior_state return_when_object_dies() {return SUCCEEDED;}
    ~attach_to_object_behavior() {
        if(self_ptr) orxObject_SetLifeTime(self, 0);
    }
};
BEHAVIOR_LEAF(attach_to_object, attach_to_object_behavior)

template <class OUT>
class for_config_list_behavior: public iterative_behavior<OUT> {
    bool initialized = false;
    typename ConfigListRange<OUT>::iterator current;
    ConfigListRange<OUT> range;
public:
    for_config_list_behavior(orxOBJECT * context, const behavior_constructor & behavior,
            in_out_variable<OUT> iteratee, const char * key):
        iterative_behavior<OUT>(context, behavior, iteratee),range(key){}
    OUT next() {
        if(!initialized) {
            current = range.begin();
            initialized = true;
        }
        else ++current;
        if(end()) return {};
        else return *current;
    }
    bool end() {return !(current != range.end());}
};
BEHAVIOR_TRANSFORMER_WITH_ARGS(for_config_string_list, for_config_list_behavior<const char *>)
BEHAVIOR_TRANSFORMER_WITH_ARGS(for_config_float_list, for_config_list_behavior<orxFLOAT>)

struct wait_for_animation_behavior: object_behavior_mixin<wait_for_animation_behavior> {
    const char * animation;
    wait_for_animation_behavior(orxOBJECT * obj, const char * animation): MIXIN(obj), animation(animation){}
    behavior_state run_object(const orxCLOCK_INFO & clock) {
        return orxObject_IsCurrentAnim(self, animation) ? SUCCEEDED : RUNNING;
    }
};
BEHAVIOR_LEAF(wait_for_animation, wait_for_animation_behavior)

#endif /* ORXFSM_ORX_BEHAVIORS_HPP_ */
