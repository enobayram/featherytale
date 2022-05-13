/*
 * behavior_header.hpp
 *
 *  Created on: Jul 24, 2015
 *      Author: enis
 */

#ifndef ORXFSM_BEHAVIOR_HEADER_HPP_
#define ORXFSM_BEHAVIOR_HEADER_HPP_

#include <functional>

#include <orx.h>

class behavior_t {
public:
    virtual ~behavior_t(){}
    enum behavior_state{FAILED, RUNNING, SUCCEEDED};
    virtual behavior_state run(const orxCLOCK_INFO &) = 0;
};

using behavior_constructor = std::function<behavior_t *(orxOBJECT *)>;

template <class Behavior, class ... ARGS>
behavior_constructor direct_constructor(ARGS ... args);

class simple_action_behavior;

#define ACTION_LEAF(NAME, ACTION) \
template<class ...ARGS> behavior_constructor NAME(ARGS...args) { \
    return direct_constructor<simple_action_behavior>(ACTION, std::move(args)...); \
}

#define BEHAVIOR_LEAF(NAME, TYPE) \
template <class ...ARGS> behavior_constructor NAME(ARGS ... args) { \
    return direct_constructor<TYPE>(args...); \
}

template <class Adaptor, class ...ARGS>
behavior_constructor simple_adaptor_constructor(ARGS ... args);

template <class Adaptor, class Out, class Extractor, class ...ARGS>
behavior_constructor adaptor_constructor(Extractor,ARGS...);

template <class T> struct extractor;

#define RETURNING_ACTION_LEAF(NAME, ACTION) \
struct ACTION##_adaptor {template <class RET, class ...ARGS>static RET call(ARGS...args) {return ACTION(args...);}};\
template<class OUT, class ...ARGS> behavior_constructor NAME(extractor<OUT> out, ARGS...args) { \
    return adaptor_constructor<ACTION##_adaptor, OUT>(out, std::move(args)...); \
}

#define COMPLEX_ACTION_LEAF(NAME, ACTION) \
struct ACTION##_adaptor {template <class ...ARGS>static orxSTATUS call(ARGS...args) {return ACTION(args...);}};\
template<class ...ARGS> behavior_constructor NAME(ARGS...args) { \
    return simple_adaptor_constructor<ACTION##_adaptor>(std::move(args)...); \
}

#define RETURNING_MEMBER_ACTION_LEAF(NAME, CLASS, ACTION) \
struct ACTION##_adaptor {template <class RET, class OBJ, class ...ARGS>static RET call(OBJ obj, ARGS...args) {return ((CLASS *)obj)->ACTION(args...);}};\
template<class OUT, class ...ARGS> behavior_constructor NAME(extractor<OUT> out, ARGS...args) { \
    return adaptor_constructor<ACTION##_adaptor, OUT>(out, std::move(args)...); \
}

#endif /* ORXFSM_BEHAVIOR_HEADER_HPP_ */
