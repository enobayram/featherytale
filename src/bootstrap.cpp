/*
 * bootstrap.cpp
 *
 *  Created on: Nov 20, 2015
 *      Author: enis
 */

#include <orx.h>
#include "pancarDemo.h"

#include <orxFSM/behavior.hpp>
#include "orx_utilities.h"
#include <util/config_processors.h>
#include <scroll_ext/config_utils.hpp>
#include <scroll_ext/eventdispatcher.h>
#include <gameObjects/aspects.h>
#include <orxFSM/orxFSM_header.h>
#include <game_state.h>
#include "gameObjects/screens/CutScene.h"
#include <analytics.h>
#include <platform.h>

void register_commands() {
    register_command("SetConstraints", [](orxOBJECT * obj, const char * section) {
        auto sm = GetAspect<state_machine>(obj);
        return sm->SetConstraints(section);
    });

    register_command("SetController", [](orxOBJECT * obj, const char * section) {
        auto sm = GetAspect<state_machine>(obj);
        return sm->SetController(section);
    });

    register_command("SetNextScreen", [](orxOBJECT * obj, const char * level) {
        auto sm = GetAspect<game_screen>(obj);
        return sm->SetNextScreen(Persist(level));
    });

    register_command("EndCutScene", [](orxOBJECT * obj) {scroll_cast<CutScene>(obj)->EndCutscene();});

    register_command("NewSlide", [](orxOBJECT * obj, const char * section) {
        return scroll_cast<CutScene>(obj)->NewSlide(section, 2.5);
    });

    register_command("NewScene", [](orxOBJECT * obj, const char * section) {
        return scroll_cast<CutScene>(obj)->NewSlide(section, 0);
    });

    register_command("CreateChild", [](orxOBJECT * obj, const char * section) {
        return CreateChild(obj, section);
    });

    register_command("GetValue", [](orxOBJECT * obj, const char * key) {
        ConfigSectionGuard guard(orxObject_GetName(obj));
        return orxConfig_GetString(key);
    });

    register_command("Console.Toggle", [] {
        orxConsole_Enable(!orxConsole_IsEnabled());
    });

    register_command("GetGlobal", [](std::vector<const char *> keys) {
        auto context = &global_context;
        orxCOMMAND_VAR empty;
        if(keys.empty()) return empty;
        empty.eType = orxCOMMAND_VAR_TYPE_NONE;
        for(size_t i=0; i<keys.size()-1; ++i) {
            auto key = keys[i];
            auto value = context->get(key);
            if(value.eType != orxCOMMAND_VAR_TYPE_U64) return empty;
            else {
                context = GetAspect<behavior_context>(arg_from_command_var(value, type_carrier<orxOBJECT *>{}));
                if(context == nullptr) return empty;
            }
        }
        return context->get(keys[keys.size()-1]);
    });
    REGISTER_FUNCTION(ProcessAnimGraph, "ProcessAnimGraph");

    register_command("GetLevelName", []{
    	return pancarDemo::GetInstance().level->GetScreenName();
    });

    register_command("GetCurrentScreen", [](){
        return dynamic_cast<ExtendedMonomorphic *>(pancarDemo::GetInstance().level)->GetOrxObject();
    });

    register_command("SetNextScreenGlobal", [](const char * level){
    	pancarDemo::GetInstance().level->SetNextScreen(Persist(level));
    });

    register_command("ToggleChild", [](orxOBJECT * obj, const char * child_name) {
        auto child = FindOwnedChild(obj, child_name);
        if(!child) {
            CreateChild(obj, child_name);
            return true;
        }
        else {
            orxObject_SetLifeTime(child, 0);
            return false;
        }
    });

    register_command("SendSignal", [](orxOBJECT * ed, const char * signal){
        SendSignal(scroll_cast<event_dispatcher>(ed), signal);
    });

    register_command("IsLevelCompleted", [](const char * level) {
        auto level_data = fetchLevelData(level);
        return level_data.score_levels[0];
    });

    register_command("HasHeartsOnLevel", [](const char * level, orxS32 hearts) {
        auto data = fetchLevelData(level);
        int curhearts = 0;
        for(auto earned: data.challenges) curhearts += earned;
        for(auto earned: data.score_levels) curhearts += earned;
        return curhearts >= hearts;
    });

    register_command("IsRecordAtLeast", [](orxS32 record) {
        auto data = fetchInfiniteModeData();
        return data.record >= record;
    });

    register_command("IsHeartsAtLeast", [](orxU32 hearts) {
        return getTotalHearts() >= hearts;
    });

    register_command("GetLocalizedValue", [](orxOBJECT * obj, const char * key) {
        ConfigSectionGuard guard(orxObject_GetName(obj));
        return LocaleAwareString(key);
    });

    register_command("GetRecord", []{
        return (orxS32) fetchInfiniteModeData().record;
    });

    register_command("SendDesignEvent", [](std::vector<const char *> fields) {
        design_event(fields);
    });

    register_command("ShowStorePage", [](){
        GetPlatform()->showStorePage();
    });

    register_command("ShowShareDialog", [](){
        GetPlatform()->showShareDialog();
    });

    register_command("EarnResource", [](const char * resource, orxS32 amount, const char * category, const char * item) {
        earnResource(resource, amount, category, item);
    });

    register_command("EvalWithGuard", [](const char * guard, orxFLOAT curVal, orxFLOAT offset, const char * command){
        auto persistedVal = fetch_command_guard(guard);
        if(curVal <= persistedVal) return;
        persist_command_guard(guard, curVal + offset);
        orxCOMMAND_VAR v;
        orxProfiler_EnableMarkerOperations(false);
        orxCommand_Evaluate(command, &v);
        orxProfiler_EnableMarkerOperations(true);
    });

    register_command("GetGameSession", [](){return (orxS32) getGameSession();});

    register_command("IsInList", [](const char * val, const char * section, const char * key){
        ConfigSectionGuard guard(section);
        for(auto elem: ConfigListRange<const char *>(key)) {
            if(strcmp(val,elem) == 0) return true;
        }
        return false;
    });

    register_command("ShouldPerformSessionAction", [](const char * action_name){
        auto session_num = (orxS32) getGameSession();
        ConfigSectionGuard guard("ActionSchedules");
        orxS32 previous = -1;
        for(auto i: ConfigListRange<orxS32>(action_name)) {
            if(i>session_num) break;
            previous = i;
        }
        if(previous == -1) return false;
        auto log = fetch_action_log(action_name);
        return log < previous;
    });

    register_command("LogSessionAction", [](const char * action_name){
        auto session_num = (orxS32) getGameSession();
        persist_action_log(action_name, session_num);
    });

    register_command("SetPersistentFlag", [](const char * flag_name, bool value) {
        persist_flag(flag_name, value);
    });

    register_command("GetPersistentFlag", [](const char * flag_name) {
        return fetch_flag(flag_name);
    });

    register_command("GetCurrentDay", []{
        return orxU32(orxSystem_GetRealTime()/(60*60*24));
    });

}

orxSTATUS pancarDemo::Bootstrap() const {
    register_commands();
    return orxSTATUS_SUCCESS;
}
