/*
 * orx_behaviors.cpp
 *
 *  Created on: Jul 24, 2015
 *      Author: enis
 */

#include "orx_behaviors.hpp"
#include "simple_behaviors.hpp"
#include <util/iteration_utils.hpp>

behavior_t::behavior_state move_behavior::run_object(const orxCLOCK_INFO & clock) {
    orxVECTOR curPos;
    orxObject_GetPosition(self, &curPos);
    orxFLOAT step = speed*clock.fDT;
    if(orxVector_GetDistance(&curPos, &target) < step) {
        orxObject_SetPosition(self, &target);
        return SUCCEEDED;
    }

    auto newPos = curPos + normalized(target-curPos) * step;
    orxObject_SetPosition(self, &newPos);
    return RUNNING;
}

orxSTATUS get_marker_action(in_out_variable<orxVECTOR> item, ExtendedMonomorphic * obj, std::string marker_name) {
   auto marker = obj->getCurrentMarker(marker_name.c_str());
   if(marker) {
       item = marker->center;
       return orxSTATUS_SUCCESS;
   } else return orxSTATUS_FAILURE;
}

void PushToCommandStack(const orxCOMMAND_VAR & arg, std::string & storage) {
    static char buff[5000] = ">id ";
    orxCommand_PrintVar(buff+4, sizeof(buff)-5, &arg);
    storage = buff;
    orxCOMMAND_VAR ignore;
    orxCommand_Evaluate(storage.c_str(),&ignore);
}

behavior_t::behavior_state execute_command_behavior::run(const orxCLOCK_INFO & clock) {
    orxCOMMAND_VAR result;
    auto context_ = GetAspect<behavior_context>(context);
    std::vector<std::string> stack_strings(push_list.size());
    for(size_t i=0; i<push_list.size(); ++i) {
        PushToCommandStack(context_->get(push_list[i].c_str()), stack_strings[i]);
    }
    if(!orxCommand_Evaluate(command.c_str(),&result)) {
        return FAILED;
    }
    if(result.eType == orxCOMMAND_VAR_TYPE_NUMERIC) {
        orxCommand_ParseNumericalArguments(1,&result, &result);
    }
    context_->set(key, result);
    return SUCCEEDED;
}

orxSTATUS create_new_action(in_out_variable<orxOBJECT *> item, std::string section) {
    auto obj = orxObject_CreateFromConfig(section.c_str());
    if(!obj) return orxSTATUS_FAILURE;
    item = obj;
    orxObject_SetParent(obj,item.context);
    orxObject_SetOwner(obj,item.context);
    SetContext(obj, item.context);
    return orxSTATUS_SUCCESS;
}

orxSTATUS create_action(in_out_variable<orxOBJECT *> item, std::string section, bool replace) {
    orxOBJECT * oldObj = item;
    if(oldObj && replace) orxObject_SetLifeTime(oldObj,0);

    return create_new_action(item, std::move(section));
}

orxSTATUS create_in_collection_action(object_collection * collection,
                                      in_out_variable<orxOBJECT *> item,
                                      std::string section,
                                      bool replace, bool ignore_if_null) {
    if(collection == nullptr && !ignore_if_null) return orxSTATUS_FAILURE;
    auto res = create_action(item, std::move(section), replace);
    if(res == orxSTATUS_FAILURE) return res;
    if(collection) append_object_action(collection, item);
    return orxSTATUS_SUCCESS;
}

orxSTATUS destroy_action(in_out_variable<orxOBJECT *> item, bool fail_if_missing) {
        orxOBJECT * obj = item;
        if(!obj) return fail_if_missing ? orxSTATUS_FAILURE : orxSTATUS_SUCCESS;
        orxObject_SetLifeTime(obj,0);
        item.remove();
        return orxSTATUS_SUCCESS;
}

behavior_t::behavior_state fade_behavior::run_object(const orxCLOCK_INFO& clock) {
    orxFLOAT time_delta = std::min(clock.fDT, period-elapsed);
    elapsed += clock.fDT;
    float curAlpha;
    if(orxObject_HasColor(self)) {
        orxCOLOR c;
        curAlpha = orxObject_GetColor(self, &c)->fAlpha;
    } else curAlpha = 1;
    iterate_object_tree(self, [=](orxOBJECT * obj) {
        SetAlpha(obj, curAlpha + alpha_change * clock.fDT/period);
    });
    return elapsed>=period ? SUCCEEDED : RUNNING;
}

behavior_t::behavior_state periodically_behavior::run(const orxCLOCK_INFO & clock) {
    ConfigSectionGuard guard(section);
    time_elapsed += clock.fDT;
    if(!current_behavior && time_elapsed >= next_time) {
        index++;
        auto listSize = orxConfig_GetListCounter(time_series);
        if(index > listSize && !repeat_afterwards) return SUCCEEDED;
        if(index<listSize) next_time = orxConfig_GetListFloat(time_series, index);
        else if(listSize == 1) next_time += orxConfig_GetFloat(time_series);
        else next_time += orxConfig_GetListFloat(time_series, listSize-1) - orxConfig_GetListFloat(time_series, listSize-2);
        current_behavior.reset(behavior(context));
    }
    if(current_behavior) {
        auto result = current_behavior->run(clock);
        if(result!=SUCCEEDED) return result;
        current_behavior.reset(nullptr);
    }
    return RUNNING;
}
