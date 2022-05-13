#include <map>
#include <random>
#include <algorithm>

#include "behavior.hpp"
#include "simple_behaviors.hpp"
#include "behavior_combinators.hpp"
#include "configfunction.h"
#include <util/orxVECTORoperators.h>
#include <util/random.hpp>

using namespace std;

behavior_context global_context;
extern std::mt19937 gen;

orxCOMMAND_VAR behavior_context::get(const char * key) {
    auto keyID = orxString_GetID(key);
    if(context.count(keyID)) return context[keyID];
    orxCOMMAND_VAR result;
    result.eType = orxCOMMAND_VAR_TYPE_NONE;
    return result;
}

void behavior_context::set(const char * key_, orxCOMMAND_VAR var) {
    auto key = orxString_GetID(key_);
    auto storage_it = string_storage.find(key);
    if(storage_it != string_storage.end()) string_storage.erase(storage_it);
    auto & newEntry = context[key];
    newEntry = var;
    if(var.eType == orxCOMMAND_VAR_TYPE_STRING) {
        auto & newStorage = string_storage[key];
        newStorage = var.zValue;
        newEntry.zValue = newStorage.c_str();
    }
}

void behavior_context::set(const char * key_, const std::string & var) {
    auto key = orxString_GetID(key_);
    auto & newStorage = string_storage[key];
    newStorage = var;
    auto & newEntry = context[key];
    newEntry.eType = orxCOMMAND_VAR_TYPE_STRING;
    newEntry.zValue = newStorage.c_str();
}


behavior_context::~behavior_context() {
}

void behavior_context::remove(const char * key_) {
    auto key = orxString_GetID(key_);
    auto storage_it = string_storage.find(key);
    if(storage_it != string_storage.end()) string_storage.erase(storage_it);
    context.erase(key);
}

map<string, behavior_constructor> & behavior_registry() {
    static map<string,behavior_constructor> result;
    return result;
}

void register_behavior(string key, behavior_constructor behavior) {
    behavior_registry()[std::move(key)] = std::move(behavior);
}

const behavior_constructor & get_behavior(const char * key) {
    auto it = behavior_registry().find(key);
    if(it==behavior_registry().end()) {
        static behavior_constructor empty{};
        return empty;
    }
    else return it->second;
}

orxFLOAT random_in_range_action(orxFLOAT lower, orxFLOAT upper) {
    return std::uniform_real_distribution<orxFLOAT>(lower,upper)(gen);
}

orxSTATUS remove_object_action(object_collection * collection, orxOBJECT * obj) {
    auto objID = orxStructure_GetGUID(obj);
    auto iter = std::remove_if(collection->objects.begin(), collection->objects.end(), [&](weak_object_ptr ptr) {
        return ptr.compare_with_id(objID);
    });
    if(iter == collection->objects.end()) return orxSTATUS_FAILURE;
    else {
        collection->objects.erase(iter);
        return orxSTATUS_SUCCESS;
    }
}

output_signal_behavior::output_signal_behavior(in_out_variable<orxFLOAT> out, const char* section):
    out(out), signal(config_function::create_from_section(section)) {}

behavior_t::behavior_state output_signal_behavior::run(const orxCLOCK_INFO& clock) {
    time += clock.fDT;
    if(!signal->in_domain(time)) return SUCCEEDED;
    out = signal->apply(time);
    return RUNNING;
}

behavior_t::behavior_state poisson_wait_behavior::run(const orxCLOCK_INFO & clock) {
    return poisson_event_generator(clock.fDT)(period) ? SUCCEEDED : RUNNING;
}

std::string sprint_impl(const char * format, push_func pushers[], const void * args[], size_t count) {
    std::stringstream ss;
    while(*format != 0) {
        if(*format != '%') {
            ss<<*format;
            format++;
        }
        else {
            format++;
            char * end;
            auto idx_raw = strtoul(format, &end, 10);
            unsigned long idx = idx_raw-1;
            if(format == end) return "Improper format string";
            if(idx >= count) return "Index out of range";
            pushers[idx](ss, args[idx]);
            format = end;
        }
    }
    return ss.str();
}

orxOBJECT * for_collection_behavior::next() {
    if(!initialized) {
        current = collection->objects.begin();
        initialized = true;
    }
    else current++;
    if(end()) return nullptr;
    else if(orxOBJECT* obj = *current)  return obj;
    else return next();
}

behavior_t::behavior_state capture_behavior::run(const orxCLOCK_INFO & clock) {
    auto out = behavior->run(clock);
    if(out == RUNNING) return RUNNING;
    else {
        result = out == SUCCEEDED;
        return SUCCEEDED;
    }
}

behavior_t::behavior_state behavior_all::run(const orxCLOCK_INFO & clock) {
        for(auto iter=behaviors.begin(); iter!=behaviors.end(); ) {
            auto & behavior = *iter;
            auto res = behavior->run(clock);
            switch(res) {
            case RUNNING:
                ++iter; // keep the behavior
                break;
            case SUCCEEDED:
                iter = behaviors.erase(iter); // drop the behavior
                break;
            case FAILED:
                return FAILED; // all is terminated
            }
        }
        if(behaviors.empty()) return SUCCEEDED;
        else return RUNNING;
    }
