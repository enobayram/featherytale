/*
 * orxFSM.h
 *
 *  Created on: May 31, 2015
 *      Author: eba
 */

#ifndef ORXFSM_H_
#define ORXFSM_H_

#include <initializer_list>
#include <utility>
#include <functional>
#include <type_traits>
#include <vector>
#include <map>

#include <orx.h>

#include "orxFSM_header.h"
#include "functools.hpp"
#include <util/function_registry.hpp>
#include <util/optional.hpp>

template <class Self>
class state_t {
public:
    typedef std::function<state_t<Self> *(Self &)> constructor;
    virtual constructor Update(Self & self, const orxCLOCK_INFO &_rstInfo) {return {};};
    virtual void ExitState(Self & self) {};
    virtual int getTag() = 0;
    virtual ~state_t(){};
};

template <class State>
struct state_traits{};

template <class Self, class State>
struct state_base: public state_t<Self> {
    int getTag() {return tag;}
    enum {tag = state_traits<State>::tag};
};

template <class SELF>
using action_t = std::function<void(SELF &)>;

template <class SELF>
using guard_t = std::function<bool(SELF &, state_t<SELF> &)>;

template<class SELF>
struct transition {
    int src;
    int dst;
    guard_t<SELF> guard;
    action_t<SELF> action;
};

template<class SELF>
struct transition_table {
    std::vector<transition<SELF>> transitions;
    transition_table(std::initializer_list<transition<SELF>> transitions):transitions(transitions) {
    }
};

template<class Source, class Target>
struct any_time{

    template<class SELF>
    operator transition<SELF>() {
        return transition<SELF>{Source::tag, Target::tag};
    }
};

template<class Source, class Target, class Guard, class Action>
struct only_if_proxy {
    Guard guard;
    Action action;

    only_if_proxy(Guard g, Action a):guard(g), action(a){}

    template<class SELF>
    operator transition<SELF>() {
        guard_t<SELF> guard_ = condition_function<bool, SELF &, state_t<SELF>&, Source &>(guard);
        return transition<SELF>{Source::tag, Target::tag, guard_, action};
    }
};

struct no_action {
    template <class SELF>
    operator action_t<SELF>() {return {};}
};

template<class Source, class Target, class Guard, class Action = no_action>
only_if_proxy<Source,Target,Guard,Action> only_if(Guard g, Action a = {}) {return {g,a};}

template<class SELF>
struct behavior;

template<class SELF>
struct transition_constraints{
    virtual ~transition_constraints(){}

    template <class SRC, class DST>
    std::pair<bool, action_t<SELF>> isAllowed(SRC & state) const {
        return isAllowed(SRC::tag, DST::tag, state);
    }

    virtual std::pair<bool, action_t<SELF>> isAllowed(int src_tag, int dst_tag, state_t<SELF> & state) const = 0;
};

template <class Self>
struct default_transition_constraints: public transition_constraints<Self> {
    const transition_table<Self> & table;
    Self & self;
    default_transition_constraints(const transition_table<Self> & table, Self & self): table(table), self(self) {}

    std::pair<bool, action_t<Self>> isAllowed(int src_tag, int dst_tag, state_t<Self> & state) const{
        for(auto t: table.transitions) {
            if(t.src==src_tag && t.dst == dst_tag) {
                if(t.guard && !t.guard(self, state)) return {false,{}};
                else return {true, t.action};
            }
        }
        return {false,{}};
    }
};

template<class SELF>
using decision_t = std::pair<state_constructor<SELF>, action_t<SELF>>;

template <class SELF, class SrcState>
struct evaluation_context {
    const transition_constraints<SELF> & cnts;
    state_constructor<SELF> ctr;
    action_t<SELF> action;
    SrcState & curState;
    evaluation_context(const transition_constraints<SELF> & cnts, SrcState & state): cnts(cnts), curState(state) {
    }
    operator decision_t<SELF>() {return {ctr,action};}
};

template<class SELF, class State>
struct guarded_concrete {
    std::function<state_constructor<SELF>()> ctr;
    template <class Condition>
    guarded_concrete(Condition cond,typename std::enable_if<std::is_same<decltype(cond()), bool>::value>::type* = 0): ctr([cond]{
        if(cond()) return state_constructor<SELF>{[](SELF & self) { return new State(self);}};
        else return state_constructor<SELF>{};
    }) {}
    template <class Condition>
    guarded_concrete(Condition cond,typename std::enable_if<!std::is_same<decltype(cond()), bool>::value>::type* = 0): ctr([cond]{
        auto res = cond();
        if(res) return state_constructor<SELF>{[res](SELF & self) { return new State(self, res.val);}};
        else return state_constructor<SELF>{};
    }) {}
    guarded_concrete(bool cond): guarded_concrete([cond]{return cond;}) {}
    template <class Arg>
    guarded_concrete(optional<Arg> arg): guarded_concrete([arg]{return arg;}){}
};

template <class Arg>
struct argument_holder {
    Arg arg;
    optional<Arg> operator()() const {return {arg};}
};

template <class Arg>
argument_holder<Arg> pass_through(Arg arg) {
    return {arg};
}

template <class SrcState, class DstState, class Context>
evaluation_context<Context, SrcState> && operator >> (evaluation_context<Context, SrcState> && ctx, guarded_concrete<Context, DstState> && g) {
    if(ctx.ctr) return std::move(ctx);
    auto query_result = ctx.cnts.template isAllowed<SrcState, DstState>(ctx.curState);
    if(query_result.first) {
        ctx.ctr = g.ctr();
        ctx.action = std::move(query_result.second);
    }
    return std::move(ctx);
}

template <class Context, class SrcState, class Step>
decltype(std::declval<Step>()(std::declval<evaluation_context<Context, SrcState> &&>()))
operator >> (evaluation_context<Context, SrcState> && ctx, Step step) {
    return step(std::move(ctx));
}

template<class Self>
struct controller {
    const char * const name;
    typedef Self state_machine_t;
    virtual decision_t<Self> execute(Self & self, state_t<Self> * st, const orxCLOCK_INFO &_rstInfo, transition_constraints<Self> &) = 0;
    controller(const char * name): name(name) {}
    virtual ~controller(){};
};

template <class Self, class State, class Controller>
decision_t<Self> getDecision(Self & self, Controller & cnt , state_t<Self> * st, const orxCLOCK_INFO &_rstInfo) {
    return cnt.decide(self, static_cast<State &>(*st), _rstInfo);
}

template <class Self, class Controller, class ... States>
decision_t<Self> executeController(state_list<States...>, Self & self, state_t<Self> * st, Controller & cnt, const orxCLOCK_INFO &_rstInfo) {
    typedef decision_t<Self> (* decision_getter)(Self &, Controller &, state_t<Self> *, const orxCLOCK_INFO &);
    static decision_getter getters[] = {getDecision<Self, States, Controller>...};
    return getters[st->getTag()](self,cnt,st,_rstInfo);
}

template<class SELF, class DERIVING>
struct controller_mixin: public controller<SELF> {
    const char * config_section;
    controller_mixin(const char * config_section): controller<SELF>(config_section), config_section(config_section) {}
    template <class T> T GetValue(const char * key, bool shouldpush=true) {
        return ::GetValue<T>(shouldpush?config_section:nullptr, key);
    }
    typedef decision_t<SELF> decision;
    template <class SrcState>
    using evaluation = evaluation_context<SELF, SrcState>;
    template<class State>
    using guarded = guarded_concrete<SELF, State>;
    template <class SrcState>
    evaluation<SrcState> from(SrcState & state) {
        return {*cnts, state};
    }
    decision decide(SELF &, state_t<SELF> &, const orxCLOCK_INFO &) {
        return {};
    }
    decision_t<SELF> execute(SELF & self, state_t<SELF> * st, const orxCLOCK_INFO &_rstInfo, transition_constraints<SELF> & cnts) {
        this->cnts = &cnts;
        return executeController(typename SELF::state_types{}, self, st, static_cast<DERIVING &>(*this), _rstInfo);
    }
private:
    const transition_constraints<SELF> *cnts = nullptr;
};

template<class SELF>
struct default_controller: controller_mixin<SELF, default_controller<SELF>> {
    default_controller(const char *s): controller_mixin<SELF, default_controller<SELF>>(s){}
}; // a controller that doesn't make any decisions


#define ORX_STATE_1(A)      "Error! ORX_STATE needs to be called with 2 or 3 arguments."
#define ORX_STATE_2(A,B)    ORX_STATE_3(A,B, state_base<A,B>)
#define ORX_STATE_3(SELF, STATE, ...) \
template<> struct state_traits<SELF::STATE> { \
    static constexpr int tag = SELF::states::STATE;\
    static constexpr const char * name = #STATE;\
}; \
constexpr const char * state_traits<SELF::STATE>::name; \
struct SELF::STATE: public __VA_ARGS__

// The interim macro that simply strips the excess and ends up with the required macro
#define ORX_STATE_X(x,A,B,C,FUNC, ...)  FUNC

// The macro that the programmer uses
#define ORX_STATE(...)           \
    ORX_STATE_X(,##__VA_ARGS__,  \
        ORX_STATE_3(__VA_ARGS__),\
        ORX_STATE_2(__VA_ARGS__),\
        ORX_STATE_1(__VA_ARGS__),\
    )

struct orxFSM_cmp_str
{
   bool operator()(char const *a, char const *b)
   {
      return std::strcmp(a, b) < 0;
   }
};

template<class ...STATES>
const char * tagToName(int tag, state_list<STATES...>) {
    orxASSERT(tag>0 && tag<sizeof...(STATES), "Invalid state tag received.");
    static const char * names[] = {state_traits<STATES>::name...};
    return names[tag];
}

template<class ...STATES>
int nameToTag(const char * state_name, state_list<STATES...>) {
    static std::map<const char *, int, orxFSM_cmp_str> nameToTagMap{{state_traits<STATES>::name, STATES::tag}...};
    auto it = nameToTagMap.find(state_name);
    if(it == nameToTagMap.end()) return -1;
    else return it->second;
}

template<class DERIVING, class BASE>
state_machine_mixin<DERIVING,BASE>::state_machine_mixin() {
        curcontroller.reset(new default_controller<DERIVING>{nullptr});
        curConstraints.reset(new default_transition_constraints<DERIVING>{DERIVING::getTransitionTable(), get_deriving()});
}

template<class DERIVING, class BASE>
state_machine_mixin<DERIVING,BASE>::~state_machine_mixin(){}


template<class DERIVING, class BASE>
void state_machine_mixin<DERIVING,BASE>::Update(const orxCLOCK_INFO & _rstInfo) {
    auto newState = curstate->Update(get_deriving(), _rstInfo);
    if(newState) {
        curstate->ExitState(get_deriving());
        curstate.reset(newState(get_deriving()));
    }

    auto controllerCalculated = curcontroller->execute(get_deriving(), curstate.get(),_rstInfo, *curConstraints);
    if(controllerCalculated.first) {
        curstate->ExitState(get_deriving());
        state_constructor<DERIVING> ctr = controllerCalculated.first;
        auto action = controllerCalculated.second;
        if(action) action(get_deriving());
        curstate.reset(ctr(get_deriving()));
    }
}

template<class DERIVING, class BASE>
void state_machine_mixin<DERIVING,BASE>::ExtOnCreate() {
    BASE::ExtOnCreate();
    curstate.reset(get_deriving().GetInitialState()(get_deriving()));
    if(orxConfig_HasValue("Constraints")) SetConstraints(orxConfig_GetString("Constraints"));
    if(orxConfig_HasValue("Controller"))SetController(orxConfig_GetString("Controller"));
}

template <class Statelist> struct first_state;
template <class First, class ...Rest> struct first_state<state_list<First,Rest...>>{typedef First type;};


template <class Self>
struct configured_transition_constraints: public default_transition_constraints<Self> {
    std::set<int> allowed_states;
    configured_transition_constraints(const transition_table<Self> & table, Self & self, const char * context_section): default_transition_constraints<Self>(table,self) {
        ConfigSectionGuard guard(context_section);
        for(auto s: ConfigListRange<const char * >("AllowedStates")) {
            allowed_states.insert(nameToTag(s, typename Self::state_types{}));
        }
    }

    std::pair<bool, action_t<Self>> isAllowed(int src_tag, int dst_tag, state_t<Self> & state) const{
        if(allowed_states.count(dst_tag) == 0) return {false,{}};
        return default_transition_constraints<Self>::isAllowed(src_tag, dst_tag, state);
    }
};

template<class DERIVING, class BASE>
void state_machine_mixin<DERIVING,BASE>::SetConstraints(const char * constraints_section){
        curConstraints.reset(new configured_transition_constraints<DERIVING>{DERIVING::getTransitionTable(), get_deriving(), constraints_section});
}

template <class SELF>
using controller_constructor = std::function<controller<SELF>*(const char *)>;

template<class STATEMACHINE>
struct controller_map {
    static std::map<std::string, controller_constructor<STATEMACHINE>> &registry() {
        static std::map<std::string, controller_constructor<STATEMACHINE>> reg{{"default", [](const char * s){return new default_controller<STATEMACHINE>{s};}}};
        return reg;
    }
};

template <class Self>
using state_from_section = state_constructor<Self>(*)(const char *);

template <class Self, class State>
state_from_section<Self> get_constructor(decltype(State{*((Self *)0)},0)) {
    return [](const char * section)->state_constructor<Self> {return [](Self &s){return new State{s};};};
}

template <class Self, class State>
state_from_section<Self> get_constructor(decltype(typename State::constructor_arg{},0)) {
    return [](const char * section)->state_constructor<Self>{
        auto arg = GetValue<typename State::constructor_arg>(section, state_traits<State>::name);
        return [arg](Self &s){return new State{s,arg};};
    };
}

template <class Self, class State>
state_from_section<Self> get_constructor(char) {
    return [](const char * section)->state_constructor<Self>{return {};};
}

template<class Self, class ...STATES>
state_constructor<Self> getConstructor(int state_id, state_list<STATES...>, const char * section) {
    static state_from_section<Self> constructors[] = {get_constructor<Self, STATES>(0)...};
    orxASSERT(state_id>=0 && state_id<sizeof...(STATES));
    return constructors[state_id](section);
}

inline int previous_state(const std::vector<int> & states, int current_index) {
    if(current_index>0) return states[current_index-1];
    else return states[states.size()-1];
}

template<class Self>
struct state_converger: public controller<Self> {
    std::vector<int> forced_states;
    std::vector<int> looping_states;
    std::string section_name;
    state_converger(const char * section_name): controller<Self>(section_name), section_name(section_name) {
        ConfigSectionGuard guard(section_name);
        read_state_list(forced_states, "ForcedStates");
        read_state_list(looping_states, "LoopingStates");
    }
    void read_state_list(std::vector<int> & states, const char * key) {
        for(auto s: ConfigListRange<const char *>(key)) {
            states.push_back(nameToTag(s, typename Self::state_types{}));
        }
    }
    decision_t<Self> execute(Self & self, state_t<Self> * st, const orxCLOCK_INFO &_rstInfo, transition_constraints<Self> &cnts) {
        int src = st->getTag();
        for(int i=0; i<looping_states.size(); i++) {
            int dst = looping_states[i];
            if(src == dst) {
                int target = previous_state(looping_states, i);
                auto query_result = cnts.isAllowed(src,target,*st);
                if(query_result.first) return get_decision(self, target, query_result);
                else return {};
            }
        }

        auto decision = converge(self, st, looping_states, cnts);
        if(decision.first) return decision;

        return converge(self, st, forced_states, cnts);
    }
    decision_t<Self> get_decision(Self & self, int dst, std::pair<bool, action_t<Self>> & query_result) {
        return {getConstructor<Self>(dst, typename Self::state_types{}, section_name.c_str()), query_result.second};
    }
    decision_t<Self> converge(Self & self, state_t<Self> * st, std::vector<int> & states, transition_constraints<Self> &cnts){
        int src = st->getTag();
        for(int dst: states) {
            if(src == dst) return {}; // We are already at a favorable state
            auto query_result = cnts.isAllowed(src,dst,*st);
            if(query_result.first) return get_decision(self, dst, query_result);
        }
        return {};
    }
};

template<class DERIVING, class BASE>
inline state_constructor<DERIVING> state_machine_mixin<DERIVING, BASE>::GetInitialState() {
    ConfigSectionGuard guard(get_deriving().GetModelName());
    int init_state_tag = 0;
    if(orxConfig_HasValue("InitialState")) {
        init_state_tag = nameToTag(orxConfig_GetString("InitialState"), typename DERIVING::state_types{});
    }
    return getConstructor<DERIVING>(init_state_tag,typename DERIVING::state_types{}, get_deriving().GetModelName());
}

template<class DERIVING, class BASE>
void state_machine_mixin<DERIVING,BASE>::SetController(const char * controller_name){
    auto type = ::GetValue<const char *>(controller_name, "Type");
    auto it = controller_map<DERIVING>::registry().find(type);
    if(it==controller_map<DERIVING>::registry().end()) curcontroller.reset(new state_converger<DERIVING>{controller_name});
    else curcontroller.reset(it->second(controller_name));
}

template<class DERIVING, class BASE>
const char * state_machine_mixin<DERIVING,BASE>::GetController(){
    return curcontroller ? curcontroller->name : "";
}

template<class DERIVING, class BASE>
int state_machine_mixin<DERIVING, BASE>::GetStateID(const char* state_name) {
    return nameToTag(state_name, typename DERIVING::state_types{});
}

template<class DERIVING, class BASE>
inline int state_machine_mixin<DERIVING, BASE>::GetCurrentStateID() {
    return curstate->getTag();
}

#define ORXFSM_TOKENPASTE(x, y) x ## y
#define ORXFSM_TOKENPASTE2(x, y) ORXFSM_TOKENPASTE(x, y)
#define ORXFSM_UNIQUE ORXFSM_TOKENPASTE2(Unique_, __LINE__)

#define REGISTER_CONTROLLER(CONTROLLER, NAME) \
namespace _FSM_TMP { \
static bool ORXFSM_UNIQUE = [] {controller_map<CONTROLLER::state_machine_t>::registry()[NAME]=[](const char *s){return new CONTROLLER{s};};(void)ORXFSM_UNIQUE; return true;}(); \
}

#define INSTANTIATE_STATE_MACHINE(CLASS) \
template state_machine_mixin<CLASS, CLASS::state_machine_base>::state_machine_mixin();\
template state_machine_mixin<CLASS, CLASS::state_machine_base>::~state_machine_mixin();

// Commonly used conditions for state transitions
template <class State>
bool TransitionComplete(State & s) {return s.phase == 1;}

#endif /* ORXFSM_H_ */
