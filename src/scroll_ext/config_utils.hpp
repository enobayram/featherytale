/*
 * config_utils.hpp
 *
 *  Created on: Mar 28, 2015
 *      Author: eba
 */

#ifndef CONFIG_UTILS_HPP_
#define CONFIG_UTILS_HPP_

#include <string>
#include <vector>

#include <utils/orxString.h>
#include <core/orxConfig.h>

inline const char * Persist(const char * str) {
    return orxString_GetFromID(orxString_GetID(str));
}

class ConfigSectionGuard {
	bool guarded = false;
public:
    ConfigSectionGuard(const char * section_name) {
    	if(section_name != nullptr) {
            orxConfig_PushSection(section_name);
            guarded = true;
    	}
    }
    ~ConfigSectionGuard() {
    	if(guarded) orxConfig_PopSection();
    }
};

template <class Closure>
void in_section(const char * section_name, Closure closure) {
    ConfigSectionGuard guard{section_name};
    closure();
}

namespace orxpp {
namespace internal {

inline void GetValueImpl(orxVECTOR & result, const char * section, const char * key, int index) {
    ConfigSectionGuard guard(section);
    if(index == -1) orxConfig_GetVector(key, &result);
    else orxConfig_GetListVector(key, index, &result);
}

#define GET_VALUE_IMPLEMENTATION(TYPE, FUNC, LISTFUNC)\
        inline void GetValueImpl(TYPE & result, const char * section, const char * key, int index) { \
            ConfigSectionGuard guard(section);     \
            if(index==-1) result = FUNC(key);      \
            else result = LISTFUNC(key,index);     \
        }

GET_VALUE_IMPLEMENTATION(std::string , orxConfig_GetString, orxConfig_GetListString)
GET_VALUE_IMPLEMENTATION(const char *, orxConfig_GetString, orxConfig_GetListString)
GET_VALUE_IMPLEMENTATION(orxFLOAT    , orxConfig_GetFloat , orxConfig_GetListFloat)
GET_VALUE_IMPLEMENTATION(bool        , orxConfig_GetBool  , orxConfig_GetListBool)
GET_VALUE_IMPLEMENTATION(orxU32      , orxConfig_GetU32   , orxConfig_GetListU32)
GET_VALUE_IMPLEMENTATION(orxU64      , orxConfig_GetU64   , orxConfig_GetListU64)
GET_VALUE_IMPLEMENTATION(orxS32      , orxConfig_GetS32   , orxConfig_GetListS32)
GET_VALUE_IMPLEMENTATION(orxS64      , orxConfig_GetS64   , orxConfig_GetListS64)

}
}

template <class T>
T GetValue(const char * section, const char * key, int index = -1) {
    T result;
    orxpp::internal::GetValueImpl(result, section, key, index);
    return result;
}

template <class T>
struct ConfigListRange {
    const char * key;
    orxU32 begin_index;
    orxU32 end_index;
    ConfigListRange(const char * key, orxU32 begin = 0, int end = -1)
        : key(Persist(key)), begin_index(begin) {
        orxU32 listsize = orxConfig_GetListCounter(key);
        if(end == -1) end_index = listsize;
        else end_index = orxU32(end) > listsize ? orxU32(end) : listsize;
    }
    struct iterator {
        ConfigListRange * range;
        orxU32 index;
        T operator*() {
            return GetValue<T>(nullptr, range->key, index);
        }
        iterator & operator++() {
            index++;
            return *this;
        }
        bool operator!=(const iterator & other) {
            return index != other.index;
        }

    };
    iterator begin() { return {this, begin_index}; }
    iterator end() {return {this, end_index}; }
};

template <class T>
std::vector<T> GetList(const char * section, const char * key) {
    ConfigSectionGuard guard(section);
    std::vector<T> result;
    result.reserve(orxConfig_GetListCounter(key));
    for(T val: ConfigListRange<T>(key)) {
        result.push_back(val);
    }
    return result;
}

template <const char * prefix>
orxBOOL orxFASTCALL pick_with_prefix(const orxSTRING _zSectionName, const orxSTRING _zKeyName, const orxSTRING _zFileName, orxBOOL _bUseEncryption) {
    static size_t len = strlen(prefix);
    if(strncmp(_zSectionName, prefix, len) == 0) {
        ConfigSectionGuard guard(_zSectionName);
        return orxConfig_GetKeyCounter() > 0;
    }
    else return false;
}

#endif /* CONFIG_UTILS_HPP_ */
