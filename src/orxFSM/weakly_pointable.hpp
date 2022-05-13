/*
 * weakly_pointable.hpp
 *
 *  Created on: Aug 9, 2015
 *      Author: enis
 */

#ifndef ORXFSM_WEAKLY_POINTABLE_HPP_
#define ORXFSM_WEAKLY_POINTABLE_HPP_

#include <unordered_map>

#include <orx.h>

#include <util/metaprogramming_primitives.hpp>

typedef orxU64 hash_key_type;

hash_key_type get_next_key();

template <class DERIVING>
class lightweight_weak_pointer;

template <class DERIVING>
class weakly_pointable {
    static std::unordered_map<hash_key_type, DERIVING *> map;
    hash_key_type key;
public:
    typedef lightweight_weak_pointer<DERIVING> ptr_t;
    weakly_pointable() {
        key = get_next_key();
        map[key] = static_cast<DERIVING *>(this);
    }
    weakly_pointable(const weakly_pointable & other): weakly_pointable() {}
    ~weakly_pointable() {
        map.erase(key);
    }
    ptr_t get_weak_pointer() {
        return {key};
    }
    static DERIVING * get(hash_key_type key) {
        auto it = map.find(key);
        return it == map.end() ? nullptr : it->second;
    }
};

template <class DERIVING>
std::unordered_map<hash_key_type, DERIVING *> weakly_pointable<DERIVING>::map;

template <class DERIVING>
struct lightweight_weak_pointer {
    hash_key_type key=0;
    lightweight_weak_pointer(){}
    lightweight_weak_pointer(hash_key_type key):key(key){}
    lightweight_weak_pointer(const lightweight_weak_pointer<DERIVING> &other):key(other.key){}
    operator DERIVING *() {
        return DERIVING::get(key);
    }
    explicit operator bool() {return DERIVING::get(key);}
};

template <class T>
void set_command_var(orxCOMMAND_VAR & var, const lightweight_weak_pointer<T> & arg) {var.u64Value = arg.key;}

template <class T>
orxCOMMAND_VAR_TYPE varTypeToEnum(type_carrier<lightweight_weak_pointer<T>>) {return orxCOMMAND_VAR_TYPE_U64;}

template <class T>
lightweight_weak_pointer<T> arg_from_command_var(const orxCOMMAND_VAR & var, type_carrier<lightweight_weak_pointer<T>>) {
    return {var.u64Value};
}


#endif /* ORXFSM_WEAKLY_POINTABLE_HPP_ */
