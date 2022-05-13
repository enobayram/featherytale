/*
 * behavior_combinators.hpp
 *
 *  Created on: Jul 24, 2015
 *      Author: enis
 */

#ifndef ORXFSM_BEHAVIOR_COMBINATORS_HPP_
#define ORXFSM_BEHAVIOR_COMBINATORS_HPP_

#include "behavior.hpp"

class behavior_sequence: public behavior_t {
    orxOBJECT * context;
    const std::vector<behavior_constructor> & behaviors;
    std::unique_ptr<behavior_t> curBehavior;
    decltype(begin(behaviors)) it = begin(behaviors);
public:
    behavior_sequence(orxOBJECT * context, const std::vector<behavior_constructor> & bs):
        context(context), behaviors(bs) {}
    behavior_state run(const orxCLOCK_INFO & clock) {
        while(it!=end(behaviors)) {
            if(!curBehavior) curBehavior.reset((*it)(context));
            behavior_state substate = curBehavior->run(clock);
            switch (substate) {
            case RUNNING: return RUNNING;
            case FAILED: return FAILED;
            case SUCCEEDED:
                curBehavior.reset();
                it++;
            }
        }
        return SUCCEEDED;
    }
};

class repetitive_behavior: public behavior_t {
    orxOBJECT * context;
    const behavior_constructor & repeated_behavior;
    std::unique_ptr<behavior_t> current_behavior;
    virtual bool repeat() = 0;
public:
    repetitive_behavior(orxOBJECT * context, const behavior_constructor & repeated_behavior): context(context), repeated_behavior(repeated_behavior) {}
    behavior_state run(const orxCLOCK_INFO & clock) {
        if(!current_behavior) {
            auto should_continue = repeat();
            if(!should_continue) return SUCCEEDED;
            current_behavior.reset(repeated_behavior(context));
        }
        auto res = current_behavior->run(clock);
        switch(res) {
        case RUNNING: return RUNNING;
        case FAILED: return FAILED;
        case SUCCEEDED:
                current_behavior.reset();
                return run(clock);
        }
        return FAILED; // This shouldn't happen!
    }
};


class repeat_behavior: public repetitive_behavior {
public:
    using repetitive_behavior::repetitive_behavior;
    bool repeat() {return true;}
};

class unless_behavior: public repetitive_behavior {
    in_out_variable<bool> terminator;
public:
    unless_behavior(orxOBJECT * context, const behavior_constructor & behavior, in_out_variable<bool> terminator):
        repetitive_behavior(context, behavior), terminator(std::move(terminator)) {}
    bool repeat() {
        bool should_terminate = terminator;
        return !should_terminate;
    }
};

class behavior_any: public behavior_t {
    std::vector<std::unique_ptr<behavior_t>> behaviors;
public:
    behavior_any(orxOBJECT * context, const std::vector<behavior_constructor> & bs): behaviors(bs.size()) {
        for(size_t i=0; i<bs.size(); ++i) behaviors[i].reset(bs[i](context));
    }
    behavior_state run(const orxCLOCK_INFO & clock) {
        for(auto & behavior: behaviors) {
            auto res = behavior->run(clock);
            if(res != RUNNING) return res;
        }
        return RUNNING;
    }
};

class behavior_all: public behavior_t {
    std::vector<std::unique_ptr<behavior_t>> behaviors;
public:
    behavior_all(orxOBJECT * context, const std::vector<behavior_constructor> & bs): behaviors(bs.size()) {
        for(size_t i=0; i<bs.size(); ++i) behaviors[i].reset(bs[i](context));
    }
    behavior_state run(const orxCLOCK_INFO & clock);
};

using sequence = behavior_collection_helper<behavior_sequence>;
using any = behavior_collection_helper<behavior_any>;
using all = behavior_collection_helper<behavior_all>;


class behavior_invert: public behavior_t {
    std::unique_ptr<behavior_t> behavior;
public:
    behavior_invert(orxOBJECT * context, const behavior_constructor & behavior): behavior(behavior(context)){}
    behavior_state run(const orxCLOCK_INFO & clock) {
        auto res = behavior->run(clock);
        switch(res) {
        case RUNNING: return RUNNING;
        case FAILED: return SUCCEEDED;
        case SUCCEEDED: return FAILED;
        }
        return FAILED;
    }
};

class ignore_behavior: public behavior_t {
    std::unique_ptr<behavior_t> behavior;
public:
    enum ignore_type_t {
        KEEP_RUNNING, SUCCEED_IMMEDIATELY, FAIL_IMMEDIATELY, SUCCEED_WHEN_TERMINATES, FAIL_WHEN_TERMINATES
    } ignore_type;
    ignore_behavior(orxOBJECT * context, const behavior_constructor & behavior, ignore_type_t ignore_type):
        behavior(behavior(context)), ignore_type(ignore_type){}
    behavior_state run(const orxCLOCK_INFO & clock) {
        auto res = behavior?behavior->run(clock):SUCCEEDED;
        switch(ignore_type) {
        case KEEP_RUNNING:
            if(behavior && res!=RUNNING) behavior.reset(); // delete the behavior
            return RUNNING;
        case SUCCEED_IMMEDIATELY: return SUCCEEDED;
        case FAIL_IMMEDIATELY: return FAILED;
        case SUCCEED_WHEN_TERMINATES: return res!=RUNNING ? SUCCEEDED : RUNNING;
        case FAIL_WHEN_TERMINATES: return res!=RUNNING ? FAILED : RUNNING;
        default: return FAILED; // This shouldn't happen
        }
    }
};

BEHAVIOR_TRANSFORMER(repeat, repeat_behavior)
BEHAVIOR_TRANSFORMER(invert, behavior_invert)

BEHAVIOR_TRANSFORMER_WITH_ARGS(ignore, ignore_behavior)
BEHAVIOR_TRANSFORMER_WITH_ARGS(unless, unless_behavior)

inline decltype(ignore(0)) keep_running() {
    return ignore(ignore_behavior::KEEP_RUNNING);
}

inline decltype(ignore(0)) until_failure() {
    return ignore(ignore_behavior::SUCCEED_WHEN_TERMINATES) <<= repeat;
}

class collection_scope_behavior: public behavior_t {
    object_collection collection;
    std::unique_ptr<behavior_t> behavior;
public:
    collection_scope_behavior(orxOBJECT * context, const behavior_constructor & behavior,
            in_out_variable<object_collection::ptr_t> var): behavior(behavior(context)) {
        var = collection.get_weak_pointer();
    }
    behavior_state run(const orxCLOCK_INFO & clock) { return behavior->run(clock);}
};
BEHAVIOR_TRANSFORMER_WITH_ARGS(collection_scope, collection_scope_behavior)

template<class OUT>
class iterative_behavior: public repetitive_behavior {
protected:
    in_out_variable<OUT> iteratee;
    virtual OUT next() = 0;
    virtual bool end() = 0;
public:
    iterative_behavior(orxOBJECT * context, const behavior_constructor & behavior, in_out_variable<OUT> iteratee):
        repetitive_behavior(context, behavior), iteratee(std::move(iteratee)){}
    bool repeat() {
        OUT current = next();
        if(end()) return false;
        iteratee = current;
        return true;
    }
};

class periodically_behavior: public behavior_t {
    orxOBJECT * context;
    const behavior_constructor & behavior;
    std::unique_ptr<behavior_t> current_behavior;
    const char * time_series;
    const char * section;
    bool repeat_afterwards;
    orxS32 index = 0;
    orxFLOAT time_elapsed = 0;
    orxFLOAT next_time;
public:
    periodically_behavior(orxOBJECT * context, const behavior_constructor & behavior, const char * time_series
            , bool repeat_afterwards = true): periodically_behavior(context, behavior, nullptr, time_series, repeat_afterwards) {}
    periodically_behavior(orxOBJECT * context, const behavior_constructor & behavior, const char * section
            , const char * time_series, bool repeat_afterwards = true): context(context), behavior(behavior),
                    time_series(time_series), section(section), repeat_afterwards(repeat_afterwards) {
            next_time = GetValue<orxFLOAT>(section, time_series, 0);
        }
    behavior_state run(const orxCLOCK_INFO & clock);
};
BEHAVIOR_TRANSFORMER_WITH_ARGS(periodically, periodically_behavior)

class for_range_behavior: public iterative_behavior<orxU32> {
    orxU32 start, finish, current;
public:
    for_range_behavior(orxOBJECT * context, const behavior_constructor & behavior,
            in_out_variable<orxU32> iteratee, orxU32 start, orxU32 end):
        iterative_behavior<orxU32>(context, behavior, iteratee), start(start), finish(end), current(start-1) {}
    orxU32 next() {return ++current;}
    bool end() {return current>=finish;}
};
BEHAVIOR_TRANSFORMER_WITH_ARGS(for_range, for_range_behavior)

class for_collection_behavior: public iterative_behavior<orxOBJECT *> {
    bool initialized = false;
    std::vector<weak_object_ptr>::iterator current;
    object_collection * collection;
public:
    for_collection_behavior(orxOBJECT * context, const behavior_constructor & behavior,
            in_out_variable<orxOBJECT *> iteratee, object_collection * collection):
        iterative_behavior<orxOBJECT*>(context, behavior, iteratee),collection(collection){}
    orxOBJECT * next();
    bool end() {return current == collection->objects.end();}
};
BEHAVIOR_TRANSFORMER_WITH_ARGS(for_collection, for_collection_behavior)

struct wait_for_success_behavior: behavior_t {
    orxOBJECT * context;
    const behavior_constructor & behavior;
    wait_for_success_behavior(orxOBJECT * context, const behavior_constructor & behavior):
        context(context), behavior(behavior) {}
    behavior_state run(const orxCLOCK_INFO & clock) {
        std::unique_ptr<behavior_t> current_behavior{behavior(context)};
        auto result = current_behavior->run(clock);
        orxASSERT(result!=RUNNING);
        return result == SUCCEEDED ? SUCCEEDED : RUNNING;
    }
};
BEHAVIOR_TRANSFORMER(wait_for_success, wait_for_success_behavior)

struct if_behavior: behavior_t {
    orxOBJECT * context;
    std::unique_ptr<behavior_t> behavior;
    if_behavior(orxOBJECT * context, const behavior_constructor & behavior, bool condition):
        context(context) {
        if(condition) this->behavior.reset(behavior(context));
    }
    behavior_state run(const orxCLOCK_INFO & clock) {
        return behavior ? behavior->run(clock) : SUCCEEDED;
    }
};
BEHAVIOR_TRANSFORMER_WITH_ARGS(if_, if_behavior)

struct capture_behavior: behavior_t {
    orxOBJECT * context;
    std::unique_ptr<behavior_t> behavior;
    in_out_variable<bool> result;
    capture_behavior(orxOBJECT * context, const behavior_constructor & behavior, const in_out_variable<bool> &result):
        context(context), behavior(behavior(context)), result(result) {}
    behavior_state run(const orxCLOCK_INFO & clock);};
BEHAVIOR_TRANSFORMER_WITH_ARGS(capture, capture_behavior)

#endif /* ORXFSM_BEHAVIOR_COMBINATORS_HPP_ */
