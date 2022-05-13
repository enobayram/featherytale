/*
 * behavior.hpp
 *
 *  Created on: Jun 12, 2015
 *      Author: enis
 */

#ifndef ORXFSM_BEHAVIOR_HPP_
#define ORXFSM_BEHAVIOR_HPP_

/* TODO Change behavior_constructor from "std::function<behavior_t *(orxOBJECT *)>"
 * to "std::function<behavior_t *(behavior_context *)>" this will enable scope transformers
 */

/* TODO Modify action_leaf implementation so that it runs the action at construction and doesn't store the arguments.
 * furthermore, the result of the action should be kept in a monomorphic base class to reduce code size.
 * Another alternative is to simply make the action_leaf construct a const behavior leaf.
 */

/* TODO Enable the ability to compose parametric behaviors from existing behaviors via a "function" behavior transformer.
 * The function behavior transformer runs its enclosed behavior within a modified context. The context has a connection
 * to its parent context via the function behavior transformer's arguments. An instance of the function behavior transformer
 * defines a set of parameters that get bound to variables in the child context. 3 kinds of parameters can be defined: Writeout
 * parameter, Passthrough parameter and Read parameter.
 *
 * Writeout parameters accept variable names in the parent context. Those variables get written to at the function behavior
 * termination. The values written are the values of the bound variables in the child context.
 *
 * Passthrough parameters accept variable names or literals. Passthrough parameters expose the bound variable in the parent
 * context continuously as the bound variable in the child context.
 *
 * Read paramters accept variable names or literals. Read parameters capture the value of the argument at function behavior
 * construction and expose those values in the bound variables in the child context.
 *
 * Once defined, a "function" instance is essentially a (vector<bindings> -> behavior_transformer)
 */

/* TODO Since behavior_constructor is defined to be a std::function, it copies all its contents while being passed around.
 * This is a big efficiency problem since for instance while a sequence_behavior is being constructed, all its child nodes
 * are passed to it as behavior_constructor's, and if any of those are behavior collections or behavior transformers their
 * children are also copied recursively, causing a massive copying of the entire tree just to construct the sequence at
 * runtime. This is actually entirely unnecessary, since the behavior_constructor is immutable and therefore there's no
 * need to copy it. Find a way to make sure that behavior_constructors don't get modified during the construction and
 * relying on that, make them shared (reference counted?) while retaining value-semantics.
 */

#include <map>
#include <string>
#include <type_traits>

#include <orx.h>

#include <util/function_registry.hpp>
#include <scroll_ext/ExtendedObject.h>
#include <orxFSM/behavior_header.hpp>
#include <orxFSM/behavior_context.hpp>
#include <orxFSM/weakly_pointable.hpp>
#include <util/orx_pointers.hpp>

using namespace function_registry_internal;

void register_behavior(std::string key, behavior_constructor behavior);
const behavior_constructor & get_behavior(const char * key);
inline behavior_constructor registered_behavior(std::string key) {
    return [key](orxOBJECT * context) {return get_behavior(key.c_str())(context);};
}

template <class T>
T behavior_context::get(const char * key) {
    auto var = get(key);
    if(var.eType == to_enum<T>()) return arg_from_command_var(var, type_carrier<T>{});
    else return T{};
}
template <class T>
void behavior_context::set(const char * key, T value) {set(key, to_command_var(value));}

template <class DERIVING, class BASE>
void behavior_context_mixin<DERIVING,BASE>::ExtOnCreate() {
    global_context.set(BASE::GetModelName(), BASE::GetOrxObject());
    behavior_context::set("^", BASE::GetOrxObject());
    // TODO decide on the semantics of "^", does it refer to the context? the parent object? does a context have to be an object?
    if(orxConfig_HasValue("Behavior")) set_behavior(get_behavior(orxConfig_GetString("Behavior")));
}

struct object_collection: weakly_pointable<object_collection> {
    std::vector<weak_object_ptr> objects;
};

template <class T>
struct in_out_variable;

template <class OUT, class IN>
orxSTATUS assign_action(in_out_variable<OUT> out, IN in) {out = in; return orxSTATUS_SUCCESS;}
COMPLEX_ACTION_LEAF(assign, assign_action)

struct any_type_from_config_tag{};

template <class TargetType = any_type_from_config_tag>
struct from_config_tag{};

template <class T>
struct extractor {
    const char * key;
    explicit extractor(const char * key): key(key){}
    T extract(orxOBJECT * context) {
        auto ctx = GetAspect<behavior_context>(context);
        orxASSERT(ctx, "Non-context passed to extractor");
        return ctx ? ctx->get<T>(key) : T{};
    }

    void set(orxOBJECT * context, T arg) {
        auto ctx = GetAspect<behavior_context>(context);
        orxASSERT(ctx, "Non-context passed to extractor");
        ctx->set(key, arg);
    }

    void remove(orxOBJECT * context) {
        auto ctx = GetAspect<behavior_context>(context);
        orxASSERT(ctx, "Non-context passed to extractor");
        ctx->remove(key);
    }

    behavior_constructor operator=(T t) {
        return assign(*this,t);
    }
    template <class IN>
    behavior_constructor operator=(const extractor<IN> & t) {
        return assign(*this,t);
    }
};

using $o = extractor<orxOBJECT *>;
using $v = extractor<orxVECTOR>;
using $s = extractor<std::string>;
using $cp = extractor<const char *>;
using $f = extractor<orxFLOAT>;
using $b = extractor<bool>;
using $i = extractor<orxU32>;
using $os = extractor<object_collection::ptr_t>;

struct inherit_tag;
template <>
struct extractor<inherit_tag> {
    static constexpr const char * key = "^";
    orxOBJECT * extract(orxOBJECT * context) {return context;}
};
using inherit_t = extractor<inherit_tag>;
static const inherit_t inherit;
#include <iostream>

template <class Target>
struct extractor<from_config_tag<Target>> {
    std::string section, key;
    int index;
    extractor(std::string section, std::string key, int index = -1): section(std::move(section)), key(std::move(key)), index(index) {}
    extractor(std::string key, int index = -1): key(std::move(key)), index(index) {}
    template <class T>
    T extract(orxOBJECT * context) {
        if(section.empty()) return GetValue<T>(orxObject_GetName(context), key.c_str(), index);
        else return GetValue<T>(section.c_str(), key.c_str(), index);
    }
};
template <class Target = any_type_from_config_tag>
extractor<from_config_tag<Target>> $config(std::string section, std::string key, int index = -1) {
    return {section, key, index};
}
template <class Target = any_type_from_config_tag>
extractor<from_config_tag<Target>> $config(std::string key, int index = -1) {
    return {key, index};
}

template <class T>
struct in_out_variable {
    orxOBJECT * context;
    extractor<T> ext;
    operator T() { return ext.extract(context);}
    void operator =(T arg) {ext.set(context, arg);}
    template <class IN>
    void operator =(in_out_variable<IN> in) {T t = in; *this = t;}
    void remove() {ext.remove(context);}
};

template <class Target>
struct in_out_variable<from_config_tag<Target>> {
    orxOBJECT * context;
    typedef extractor<from_config_tag<Target>> ext_t;
    ext_t ext;
    in_out_variable(orxOBJECT * context, ext_t ext): context(context), ext(std::move(ext)){}
    operator Target() {return ext.template extract<Target>(context);}
};

template <>
struct in_out_variable<from_config_tag<any_type_from_config_tag>> {
    orxOBJECT * context;
    typedef extractor<from_config_tag<any_type_from_config_tag>> ext_t;
    ext_t ext;
    in_out_variable(orxOBJECT * context, ext_t ext): context(context), ext(std::move(ext)){}
    template <class T>
    operator T() {return ext.template extract<T>(context);}
};

template <class T>
struct in_out_variable<lightweight_weak_pointer<T>> {
    orxOBJECT * context;
    typedef extractor<lightweight_weak_pointer<T>> ext_t;
    ext_t ext;
    in_out_variable(orxOBJECT * context, ext_t ext): context(context), ext(std::move(ext)){}
    operator T*() { return ext.extract(context).operator T*();}
    void operator =(const lightweight_weak_pointer<T> & arg) {ext.set(context, arg);}
    void remove() {ext.remove(context);}
};

template <class T, typename std::enable_if<std::is_base_of<ScrollObject,T>::value>::type * = nullptr>
T * GetItemOfType(orxOBJECT * object, int) {
    return ExtractExtendedObject<T>(object);
}

template <class T, typename T::AspectTag = 0>
T * GetItemOfType(orxOBJECT * object, char) {
    return GetAspect<T>(object);
}

struct object_proxy: in_out_variable<orxOBJECT *> {
    // TODO try to find a way to constrain this to children of ScrollObject only.
    //, typename std::enable_if<std::is_base_of<ScrollObject,T>::value>::type * = nullptr>
    object_proxy(orxOBJECT * context, extractor<orxOBJECT *> e): in_out_variable{context,std::move(e)}{}
    template<class T>
    operator T *() {
        orxOBJECT * object = *this;
        return GetItemOfType<T>(object,0);
    }
    operator const orxOBJECT *() {
        return (orxOBJECT *) (*this);
    }
    operator bool() {return (orxOBJECT *)(*this);}
};

template <class T> struct arg_traits {enum {IS_CONTEXTUAL = false};};
template <class T> struct arg_traits<extractor<T>> {enum {IS_CONTEXTUAL = true};};

inline orxOBJECT * extract(orxOBJECT * context, inherit_t) {return context;}

template <class T>
T extract(orxOBJECT * context, T arg) {return arg;}

template <class T>
in_out_variable<T> extract(orxOBJECT * context, extractor<T> e) {return {context, std::move(e)};} //e.extract(context);}

inline object_proxy extract(orxOBJECT * context, $o e) {return {context, std::move(e)};}

template <class T>
using extracted_type = decltype(extract(nullptr, std::declval<T>()));

template <class T, class ... ARGS>
T * create_new(orxOBJECT * context, ARGS ... args) {
    return new T(extract(context, args)...);
}

template <class Behavior, class ... ARGS>
behavior_constructor direct_constructor(ARGS ... args) {
    return std::bind(create_new<Behavior, ARGS...>, std::placeholders::_1, args...);
}

template <class COLLECTION>
struct behavior_collection_helper: behavior_constructor {
    behavior_collection_helper(std::initializer_list<behavior_constructor> behaviors): behavior_constructor(create(behaviors)) {}
    static behavior_constructor create(std::initializer_list<behavior_constructor> behaviors_) {
        std::vector<behavior_constructor> behaviors{behaviors_};
        return [behaviors](orxOBJECT * context) {
            return new COLLECTION(context, behaviors);
        };
    }
};

inline behavior_t::behavior_state toBState(orxSTATUS in){
    return in == orxSTATUS_SUCCESS ? behavior_t::SUCCEEDED : behavior_t::FAILED;
}

template <class RET, class ACTION_TYPE> struct constructor {
    template <class ...ARGS>
    static behavior_t::behavior_state apply(ACTION_TYPE action, in_out_variable<RET> out, ARGS... args) {
        out = action(args...);
        return behavior_t::SUCCEEDED;
    }
};
template <class ACTION_TYPE>
struct constructor<orxSTATUS, ACTION_TYPE> {
    template <class ...ARGS>
    static behavior_t::behavior_state apply(ACTION_TYPE action, ARGS... args) {return toBState(action(args...));}
};
template <class ACTION_TYPE>
struct constructor<void, ACTION_TYPE> {
    template <class ...ARGS>
    static behavior_t::behavior_state apply(ACTION_TYPE action, ARGS... args){action(args...); return behavior_t::SUCCEEDED;}
};

struct simple_action_behavior: behavior_t {
    behavior_state result;
    template <class ...ARGS, class...ARGS2>
    simple_action_behavior(void (*action)(ARGS...), ARGS2...args){
        result = constructor<void, decltype(action)>::apply(action,args...);
    }
    template <class ...ARGS, class...ARGS2>
    simple_action_behavior(orxSTATUS (*action)(ARGS...), ARGS2...args){
        result = constructor<orxSTATUS, decltype(action)>::apply(action,args...);
    }
    template <class RET, class CLASS, class ...ARGS, class ...ARGS2>
    simple_action_behavior(RET (CLASS::*action)(ARGS...), ARGS2...args) {
        auto action_f = [action](CLASS * obj2, ARGS...args2){return (obj2->*action)(args2...);};
        result = constructor<RET, decltype(action_f)>::apply(action_f, args...);
    }

    behavior_state run(const orxCLOCK_INFO & clock) {return result;}
};

template <class Adaptor, class ...ARGS>
behavior_constructor simple_adaptor_constructor(ARGS ... args) {
    return direct_constructor<simple_action_behavior>((orxSTATUS(*)(extracted_type<ARGS>...))Adaptor::call, std::move(args)...);
}

struct returning_action_behavior: behavior_t {
    behavior_state result;
    behavior_state run(const orxCLOCK_INFO & clock) {return result;}
    template <class RET, class ...ARGS>
    returning_action_behavior(RET (*action)(ARGS...), in_out_variable<RET> var, ARGS...args) {
        result = constructor<RET, decltype(action)>::apply(action, var, args...);
    }
};

template <class Adaptor, class Out, class Extractor, class ...ARGS>
behavior_constructor adaptor_constructor(Extractor ext, ARGS ... args) {
    return direct_constructor<returning_action_behavior>((Out(*)(extracted_type<ARGS>...))Adaptor::call, ext, std::move(args)...);
}

using behavior_transformer = behavior_constructor(*)(behavior_constructor);
using stateful_behavior_transformer = std::function<behavior_constructor(behavior_constructor)>;

#define BEHAVIOR_TRANSFORMER(NAME, TYPE)                              \
inline behavior_constructor NAME(behavior_constructor behavior) {     \
    return [behavior](orxOBJECT * context) {                          \
        return new TYPE(context, behavior);                           \
    };                                                                \
}

template <class TRANSFORMER, class ...ARGS>
behavior_t * unbound_behavior_constructor(orxOBJECT * context, const behavior_constructor & behavior, ARGS ...args) {
    return new TRANSFORMER(context, behavior, extract(context,args)...);
}

template <class TRANSFORMER, class ...ARGS>
behavior_constructor unbound_behavior_transformer(behavior_constructor behavior, ARGS ...args) {
    return std::bind(unbound_behavior_constructor<TRANSFORMER, ARGS...>, std::placeholders::_1, std::move(behavior), args...);
}

#define BEHAVIOR_TRANSFORMER_WITH_ARGS(NAME, TYPE) \
template <class ...ARGS> \
stateful_behavior_transformer NAME(ARGS...args) { \
    return std::bind(unbound_behavior_transformer<TYPE, ARGS...>, std::placeholders::_1, args...); \
}

inline behavior_constructor operator<<=(behavior_transformer transformer, behavior_constructor behavior) {
    return transformer(std::move(behavior));
}

inline behavior_constructor operator<<=(stateful_behavior_transformer transformer, behavior_constructor behavior) {
    return transformer(std::move(behavior));
}

// TODO try to do this more efficiently
inline stateful_behavior_transformer operator<<=(stateful_behavior_transformer transformer1, stateful_behavior_transformer transformer2) {
    return [=](behavior_constructor behavior){return transformer1 <<= transformer2 <<= std::move(behavior);};
}

#define ORXBEHAVIOR_TOKENPASTE(x, y) x ## y
#define ORXBEHAVIOR_TOKENPASTE2(x, y) ORXBEHAVIOR_TOKENPASTE(x, y)
#define ORXBEHAVIOR_UNIQUE ORXBEHAVIOR_TOKENPASTE2(Unique_, __LINE__)

#define REGISTER_BEHAVIOR(BEHAVIOR, NAME) \
namespace _BEHAVIOR_TMP { \
static bool ORXBEHAVIOR_UNIQUE = [] {register_behavior(NAME, BEHAVIOR); (void)ORXBEHAVIOR_UNIQUE; return true;}(); \
}

inline orxOBJECT * getOrxObjectPtr(orxOBJECT * obj) {return obj;}
template <class T> orxOBJECT * getOrxObjectPtr(T * obj) {return obj->GetOrxObject();}

template <class DERIVING, class SELF = orxOBJECT>
class object_behavior_mixin: public behavior_t {
public:
    object_behavior_mixin(SELF * self): self_ptr(getOrxObjectPtr(self)), self(self) {}
protected:
    weak_object_ptr self_ptr;
    typedef object_behavior_mixin<DERIVING, SELF> MIXIN;
    SELF * self;
    behavior_state run(const orxCLOCK_INFO & clock) final {
        return self_ptr ? static_cast<DERIVING *>(this)->run_object(clock)
                        : static_cast<DERIVING *>(this)->return_when_object_dies();
    }
    behavior_state return_when_object_dies() {return FAILED;}
    behavior_state run_object(const orxCLOCK_INFO &) {return RUNNING;}
};

////////////////////// lift_function

template <class FUNC, class ...ARGS2>
behavior_t * call_lifted(orxOBJECT * context, FUNC f, ARGS2...args) {
    return f(extract(context,args)...);
}

template <class ...ARGS>
struct behavior_lift_proxy {
    behavior_t * (*f)(ARGS...);
    template <class ...ARGS2>
    behavior_constructor operator()(ARGS2...args) {
        return std::bind(call_lifted<decltype(f),ARGS2...>, std::placeholders::_1, f, args...);
    }
};

// Can be used to lift behavior_t * (*)(ARGS...) -> behavior_constructor (*)(extractor<ARGS>...)
template <class ...ARGS>
behavior_lift_proxy<ARGS...> lift_function(behavior_t * (*f)(ARGS...)) {
    return {f};
}

#endif /* ORXFSM_BEHAVIOR_HPP_ */
