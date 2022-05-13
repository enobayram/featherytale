/*
 * persistence.cpp
 *
 *  Created on: Sep 23, 2015
 *      Author: eba
 */

#include <map>
#include <vector>
#include <memory>
#include <sstream>

#include <orx.h>

#include <scroll_ext/config_utils.hpp>
#include <persistence.h>
#include <pancarDemo.h>

const char * getSaveFile() {
    return orxFile_GetApplicationSaveDirectory("pe");
}

#define PERSISTENCE_PREFIX "Persistent/"

#define PERSISTENT_SECTION(name) (PERSISTENCE_PREFIX name)

constexpr char persistence_prefix_ptr[] = PERSISTENCE_PREFIX;
std::string persistence_prefix = persistence_prefix_ptr;

void commit() {
    orxConfig_Save(getSaveFile(), use_encryption, pick_with_prefix<persistence_prefix_ptr>);
}

void load(const char * file = getSaveFile()) {
    orxConfig_Load(file);
}

template <class T, int N>
std::string toList(T (&arr)[N]) {
    std::stringstream out;
    out << arr[0];
    for(int i=1; i<N; i++) out << " # " << arr[i];
    return out.str();
}

template <class T, int N>
void fromList(const char * key, T (&arr)[N]) {
    for(int i=0; i<N; i++) arr[i] = GetValue<T>(nullptr, key, i);
}


void persist(const char * key, level_data object) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    orxConfig_SetString("score_levels", toList(object.score_levels).c_str());
    orxConfig_SetString("challenges", toList(object.challenges).c_str());
    commit();
}

void init_persistence() {
    load();
}

void fetch_persistent(const char * key, level_data & object) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    fromList("score_levels", object.score_levels);
    fromList("challenges", object.challenges);
}

void reload_persistence(const char* file) {
    std::vector<std::string> persistence_sections;
    for(orxU32 i=0; i<orxConfig_GetSectionCounter(); i++) {
        auto section = std::string(orxConfig_GetSection(i));
        if(section.compare(0,persistence_prefix.length(),persistence_prefix) == 0) {
            persistence_sections.push_back(section);
        }
    }
    for(auto section: persistence_sections) {
        orxConfig_ClearSection(section.c_str());
    }
    load(file);
}

void persist(const char* key, settings_data data) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    orxConfig_SetBool("SoundOff", data.soundOff);
    orxConfig_SetBool("MusicOff", data.musicOff);
    if(data.languageIndex == -1) orxConfig_ClearValue("Language");
    else orxConfig_SetU32("Language", data.languageIndex);
    commit();
}

void fetch_persistent(const char* key, settings_data& data) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    data.soundOff = orxConfig_GetBool("SoundOff");
    data.musicOff = orxConfig_GetBool("MusicOff");
    if(orxConfig_HasValue("Language")) data.languageIndex = orxConfig_GetU32("Language");
    else data.languageIndex = -1;
}

void persist(const char* key, test_data data) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    orxConfig_SetBool("TestMode", data.test_mode);
    commit();
}

void fetch_persistent(const char* key, test_data& data) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    data.test_mode = orxConfig_GetBool("TestMode");
}


PersistentTransaction::PersistentTransaction(const char* persistent_section,
        bool should_commit):should_commit(should_commit) {
    orxConfig_PushSection((persistence_prefix+persistent_section).c_str());
}

PersistentTransaction::~PersistentTransaction() {
    orxConfig_PopSection();
    if(should_commit) commit();
}

void persist(const char* key, infinite_mode_data object) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    orxConfig_SetS32("Record", object.record);
    commit();
}

void fetch_persistent(const char* key, infinite_mode_data& object) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    object.record = orxConfig_GetS32("Record");
}

void persist_command_guard(const char* key, orxFLOAT val) {
    ConfigSectionGuard guard((persistence_prefix+"CommandGuards").c_str());
    orxConfig_SetFloat(key, val);
    commit();
}

void persist(const char* key, game_data object) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    orxConfig_SetS32("SessionNum", object.session_num);
    commit();
}

void fetch_persistent(const char* key, game_data& object) {
    ConfigSectionGuard guard((persistence_prefix+key).c_str());
    object.session_num = orxConfig_GetS32("SessionNum");
}

orxFLOAT fetch_command_guard(const char* key) {
    ConfigSectionGuard guard((persistence_prefix+"CommandGuards").c_str());
    return orxConfig_GetFloat(key);
}

void persist_action_log(const char* action_name, int session_num) {
    ConfigSectionGuard guard(PERSISTENT_SECTION("ActionLogs"));
    orxConfig_SetS32(action_name, session_num);
    commit();
}

int fetch_action_log(const char* action_name) {
    ConfigSectionGuard guard(PERSISTENT_SECTION("ActionLogs"));
    return orxConfig_GetS32(action_name);
}

void persist_flag(const char* flag_name, bool value) {
    ConfigSectionGuard guard(PERSISTENT_SECTION("Flags"));
    orxConfig_SetBool(flag_name, value);
    commit();
}

bool fetch_flag(const char* flag_name) {
    ConfigSectionGuard guard(PERSISTENT_SECTION("Flags"));
    return orxConfig_GetBool(flag_name);
}
