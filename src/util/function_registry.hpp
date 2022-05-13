/*
 * function_registry.hpp
 *
 *  Created on: Jun 2, 2015
 *      Author: eba
 */

#ifndef FUNCTION_REGISTRY_HPP_
#define FUNCTION_REGISTRY_HPP_

#include <functional>
#include <stdexcept>
#include <vector>

#include <orx.h>
#include "metaprogramming_utilities.hpp"

namespace function_registry_internal {

template<size_t Last, size_t ... List>
num_range<List..., Last> append(num_range<List ...> r);

template<int UPTO>
struct seq;

template<>
struct seq<0> {
    typedef num_range<> type;
};

template<int UPTO>
struct seq {
    typedef decltype(append<UPTO-1>(typename seq<UPTO-1>::type{})) type;
};

struct invalid_input_exception: public std::invalid_argument {invalid_input_exception(const char *msg): std::invalid_argument(msg){}};

inline int extract_argument(const char * item_name, size_t idx, type_carrier<int>) {
    return orxConfig_GetListU32(item_name, idx);
}

inline std::string extract_argument(const char * item_name, size_t idx, type_carrier<std::string>) {
    return orxConfig_GetListString(item_name, idx);
}

inline orxVECTOR extract_argument(const char * item_name, size_t idx, type_carrier<orxVECTOR>) {
    orxVECTOR res;
    auto out = orxConfig_GetListVector(item_name, idx, &res);
    if(!out) throw invalid_input_exception("Error while parsing orxVECTOR");
    return res;
}

inline orxU32 extract_argument(const char * item_name, size_t idx, type_carrier<orxU32>) {
    return orxConfig_GetListU32(item_name, idx);
}

template <class T>
std::vector<T> extract_argument(const char * item_name, size_t idx, type_carrier<std::vector<T>>) {
    size_t cnt = orxConfig_GetListCounter(item_name);
    std::vector<T> result(cnt-idx);
    for(int i=idx; i<cnt; i++) result[i-idx] = extract_argument(item_name, i, type_carrier<T>{});
    return result;
}

template <class T> constexpr bool dummy() {return false;}
template <class T>
T extract_argument(const char * item_name, size_t idx, type_carrier<T>) {
    static_assert(dummy<T>(), "Don't know how to parse this type!");
}

template <class RET, class Func, size_t ...INDICES, class ...ARGS>
RET CallFunctionHelper(const char * item_name, Func f, num_range<INDICES...>, arg_list<ARGS...>) {
    return f(extract_argument(item_name, INDICES, type_carrier<ARGS>{})...);
}

template<class RET, class ...ARGS, class Func>
RET CallFunctionImpl(const char * item_name, Func f, arg_list<ARGS...>) {
    if(!orxConfig_HasValue(item_name)) throw invalid_input_exception("Config item does not exist!");
    if(orxConfig_GetListCounter(item_name) < sizeof...(ARGS)) throw invalid_input_exception("Error, too few arguments");
    return CallFunctionHelper<RET>(item_name, f, typename seq<sizeof...(ARGS)>::type{}, arg_list<ARGS...>{});
}

template <class Func, class RET, class ...ARGS>
struct from_stateless {
    static RET to_function(ARGS...args) {
        RET (*f)(ARGS...) = *(Func *)(0);
        return f(args...);
    }
};

template <class RET, class ...ARGS>
struct from_stateless<RET(*)(ARGS...), RET, ARGS...> {
    static_assert(dummy<RET>(),"This trick will not work for function pointers");
};

#define DEFINE_CONVERSIONS(TYPE, ENUM_SUFFIX, VAR_FIELD)\
        inline orxCOMMAND_VAR_TYPE varTypeToEnum(type_carrier<TYPE>) {return orxCOMMAND_VAR_TYPE_ ## ENUM_SUFFIX;}\
        inline TYPE arg_from_command_var(const orxCOMMAND_VAR & var, type_carrier<TYPE>) {return var.VAR_FIELD;}\
        inline void set_command_var(orxCOMMAND_VAR & var, TYPE & arg) {var.VAR_FIELD = arg;}

DEFINE_CONVERSIONS(orxS32, S32, s32Value)
DEFINE_CONVERSIONS(orxU32, U32, u32Value)
DEFINE_CONVERSIONS(orxU64, U64, u64Value)
DEFINE_CONVERSIONS(orxVECTOR, VECTOR, vValue)
DEFINE_CONVERSIONS(orxFLOAT, FLOAT, fValue)
DEFINE_CONVERSIONS(bool, BOOL, bValue)
DEFINE_CONVERSIONS(const orxSTRING, STRING, zValue)

inline orxCOMMAND_VAR_TYPE varTypeToEnum(type_carrier<orxOBJECT *>) {return orxCOMMAND_VAR_TYPE_U64;}
inline orxOBJECT * arg_from_command_var(const orxCOMMAND_VAR & var, type_carrier<orxOBJECT *>) {
    auto res = orxStructure_Get(var.u64Value);
    return res ? orxOBJECT(res):nullptr;
}
inline void set_command_var(orxCOMMAND_VAR & var, orxOBJECT *& arg) {var.u64Value = arg ? orxStructure_GetGUID(arg) : 0;}

inline orxCOMMAND_VAR_TYPE varTypeToEnum(type_carrier<orxCOMMAND_VAR>) {return orxCOMMAND_VAR_TYPE_STRING;}\
inline orxCOMMAND_VAR arg_from_command_var(const orxCOMMAND_VAR & var, type_carrier<orxCOMMAND_VAR>) {return var;}\
inline void set_command_var(orxCOMMAND_VAR & var, orxCOMMAND_VAR & arg) {var = arg;}

inline orxCOMMAND_VAR_TYPE varTypeToEnum(type_carrier<std::string>) {return orxCOMMAND_VAR_TYPE_STRING;}
inline std::string arg_from_command_var(const orxCOMMAND_VAR & var, type_carrier<std::string>) {return var.zValue;}

inline orxCOMMAND_VAR_TYPE varTypeToEnum(type_carrier<void>) {return orxCOMMAND_VAR_TYPE_NONE;}

template <class T>
orxCOMMAND_VAR_DEF getVarDef(const char * name, type_carrier<T>) {return orxCOMMAND_VAR_DEF{name, varTypeToEnum(type_carrier<T>{})};}

template <class T>
orxCOMMAND_VAR to_command_var(T & arg) {
    orxCOMMAND_VAR result;
    set_command_var(result, arg);
    result.eType = varTypeToEnum(type_carrier<T>{});
    return result;
}

inline orxCOMMAND_VAR to_command_var(orxCOMMAND_VAR & arg) { return arg;}

template <class T> orxCOMMAND_VAR_TYPE to_enum() {return varTypeToEnum(type_carrier<T>{});}

template <class RET, class ...ARGS>
struct func_type_base {
    template <RET(*F)(ARGS...)>
    struct func_type {
        RET operator()(ARGS && ... args) {
            return F(std::forward<ARGS>(args)...);
        }
    };
};

template <class ...ARGS, class FUNC, size_t ...INDICES>
static orxCOMMAND_FUNCTION make_callback(FUNC f_, type_carrier<void>, num_range<INDICES...>) {
    static FUNC f = f_;
    return [](orxU32 _u32ArgNumber, const orxCOMMAND_VAR *_astArgList, orxCOMMAND_VAR *_pstResult) {
        f(arg_from_command_var(_astArgList[INDICES], type_carrier<ARGS>{})...);
        _pstResult->eType = orxCOMMAND_VAR_TYPE_NONE;
    };
}

template <class ...ARGS, class FUNC, class RET, size_t ...INDICES>
static orxCOMMAND_FUNCTION make_callback(FUNC f_, type_carrier<RET>, num_range<INDICES...>) {
    static FUNC f = f_;
    return [](orxU32 _u32ArgNumber, const orxCOMMAND_VAR *_astArgList, orxCOMMAND_VAR *_pstResult) {
        auto result = f(arg_from_command_var(_astArgList[INDICES], type_carrier<ARGS>{})...);
        set_command_var(*_pstResult, result);
        _pstResult->eType = varTypeToEnum(type_carrier<RET>{});
    };
}

template<class Func , class RET, class ...ARGS>
orxSTATUS register_command_impl(const char * name, arg_list<ARGS...>, Func f) {
    auto result = getVarDef("Res", type_carrier<RET>{});
    orxCOMMAND_VAR_DEF arguments[] = {getVarDef("Arg", type_carrier<ARGS>{})...};
    auto callback = make_callback<ARGS...>(f, type_carrier<RET>{}, typename seq<sizeof...(ARGS)>::type{});
    return orxCommand_Register(name, callback, sizeof...(ARGS), 0, arguments, &result);
}

constexpr uint MAX_OPTIONAL_ARGS = 5;

template <class Func, class RET, class ... ARGS>
void call_and_capture_result(Func & f, RET * ret_type_proxy, orxCOMMAND_VAR * _result, ARGS &&...args) {
    auto result = f(std::forward<ARGS...>(args...));
    set_command_var(*_result, result);
    _result->eType = varTypeToEnum(type_carrier<RET>{});
}

template <class Func, class ... ARGS>
void call_and_capture_result(Func & f, void * ret_type_proxy, orxCOMMAND_VAR * _result, ARGS &&...args) {
    f(std::forward<ARGS...>(args...));
    _result->eType = orxCOMMAND_VAR_TYPE_NONE;
}

template<class Func , class RET, class T>
orxSTATUS register_command_impl(const char * name, arg_list<std::vector<T>>, Func f_) {
    static Func f=f_;
    auto result = getVarDef("Res", type_carrier<RET>{});
    orxCOMMAND_VAR_DEF arguments[MAX_OPTIONAL_ARGS];
    for(int i=0; i<MAX_OPTIONAL_ARGS; i++){ arguments[i] = getVarDef("Arg", type_carrier<T>{});};
    auto callback = [](orxU32 _u32ArgNumber, const orxCOMMAND_VAR *_astArgList, orxCOMMAND_VAR *_pstResult) {
        std::vector<T> input(_u32ArgNumber);
        for(int i=0; i<_u32ArgNumber; ++i) input[i] = arg_from_command_var(_astArgList[i], type_carrier<T>{});
        call_and_capture_result(f, (RET *) nullptr, _pstResult, input);
    };
    return orxCommand_Register(name, callback, 0, MAX_OPTIONAL_ARGS, arguments, &result);
}

template<class FUNC_TYPE> struct free_function_registrar;

template<class RET, class ...ARGS>
struct free_function_registrar<RET(ARGS...)> {
    template <RET (*F)(ARGS...)>
    static orxSTATUS register_function(const char * name) {
        typedef typename func_type_base<RET, ARGS...>::template func_type<F> func_t;
        return register_command_impl<func_t, RET>(name, arg_list<ARGS>{}..., func_t{});
    }
};
}

template<class Func>
typename function_traits<Func>::result_type CallFunction(const char * item_name, Func f) {
    typedef typename function_traits<Func>::result_type RET;
    typedef typename function_traits<Func>::args_type ARGS;
    return function_registry_internal::CallFunctionImpl<RET>(item_name, f, ARGS{});
}

template<class Func>
orxSTATUS register_command(const char * name, Func f) {
    return function_registry_internal::register_command_impl<Func, typename function_traits<Func>::result_type>(name, typename function_traits<Func>::args_type{},f);
}

#define REGISTER_FUNCTION(FUNC, NAME) function_registry_internal::free_function_registrar<decltype(FUNC)>::template register_function<FUNC>(NAME)


#endif /* FUNCTION_REGISTRY_HPP_ */
