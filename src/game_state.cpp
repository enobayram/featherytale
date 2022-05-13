/*
 * game_state.cpp
 *
 *  Created on: Nov 10, 2015
 *      Author: enis
 */

#include <core/orxLocale.h>
#include <sound/orxSound.h>
#include <game_state.h>
#include <scroll_ext/config_utils.hpp>
#include <analytics.h>
#include <platform.h>

unsigned int getTotalHearts() {
    unsigned int total = 0;
    ConfigSectionGuard guard("GameMetaData");
    for(auto level: ConfigListRange<const char *>("Levels")) {
        auto data = fetchLevelData(level);
        for(auto earned: data.challenges) total += earned;
        for(auto earned: data.score_levels) total += earned;
    }
    return total;
}

level_data fetchLevelData(const char* level_name) {
    return fetch_persistent<level_data>(level_name);
}

void putLevelData(const char* level_name, level_data object) {
    persist(level_name, object);
}

settings_data fetch_settings() {
    return fetch_persistent<settings_data>("Settings");
}

void persist_settings(const settings_data & data) {
    persist("Settings", data);
}

bool isMusicOff() {
    return fetch_settings().musicOff;
}

void setMusicOff(bool off) {
    auto settings = fetch_settings();
    settings.musicOff = off;
    persist_settings(settings);
    applySettings();
}

bool isSoundOff() {
    return fetch_settings().soundOff;
}

void setSoundOff(bool off) {
    auto settings = fetch_settings();
    settings.soundOff = off;
    persist_settings(settings);
    applySettings();
}

orxU32 getDeviceLanguageIndex() {
    auto language = GetPlatform()->getPlatformInfo().device_language;
    return GetValue<orxU32>("LanguageCodeMap", language.c_str());
}

int getLanguageIndex() {
    auto settings_index = fetch_settings().languageIndex;
    ConfigSectionGuard guard("Locale");
    auto language_count = orxConfig_GetListCounter("LanguageList");
    if(settings_index < 0 || settings_index >= language_count) {
        return getDeviceLanguageIndex();
    } else return settings_index;
}

void setLanguageIndex(int idx) {
    auto settings = fetch_settings();
    settings.languageIndex = idx;
    persist_settings(settings);
    applySettings();
}

void applySettings() {
    auto settings = fetch_settings();

    orxFLOAT sound_volume = settings.soundOff ? 0 : 1;
    orxSound_SetBusVolume(orxString_GetID("SoundFXBus"), sound_volume);

    orxFLOAT music_volume = settings.musicOff ? 0 : 1;
    orxSound_SetBusVolume(orxString_GetID("MusicBus"), music_volume);

    orxLocale_SelectLanguage(GetValue<const char *>("Locale", "LanguageList", getLanguageIndex()));
}

int getInventory(const char* item_name) {
    PersistentTransaction t("Inventory", false);
    return orxConfig_GetS32(item_name);
}

void setInventory(const char* item_name, int new_count) {
    PersistentTransaction t("Inventory", true);
    orxConfig_SetS32(item_name, new_count);
}

void purchase(const char* item_name, int count, int cost) {
    PersistentTransaction t("Inventory", true);
    orxConfig_SetS32(item_name, orxConfig_GetS32(item_name)+count);
    orxConfig_SetS32("Coin", orxConfig_GetS32("Coin")-cost);
}

bool is_level_dependent_completed(const char * level_name) {
    ConfigSectionGuard guard(level_name);
    if(orxConfig_HasValue("DependsOn")) {
        const char * dependent = orxConfig_GetString("DependsOn");
        auto history = fetchLevelData(dependent);
        if(!history.score_levels[0]) return false;
    }
    return true;
}

bool is_level_locked(const char* level_name) {
    ConfigSectionGuard guard(level_name);
    auto level_min_hearts = orxConfig_GetU32("MinHearts");
    auto total_hearts = getTotalHearts();
    return level_min_hearts > total_hearts;
}

bool is_level_playable(const char* level_name) {
    ConfigSectionGuard guard(level_name);
    if(!is_level_dependent_completed(level_name)) return false;
    if(strcmp(orxConfig_GetString("ScreenType"), "Level") == 0 ) {
        return !is_level_locked(level_name);
    } else return true;
}

infinite_mode_data fetchInfiniteModeData() {
    return fetch_persistent<infinite_mode_data>("InfiniteMode");
}

void putInfiniteModeData(infinite_mode_data object) {
    persist("InfiniteMode", object);
}

bool isButtonUnlocked(const char* name) {
    PersistentTransaction t("UnlockableButtons", false);
    return orxConfig_GetBool(name);
}

void setButtonUnlocked(const char* name, bool unlocked, int cost) {
    PersistentTransaction t("UnlockableButtons", false);
    orxConfig_SetBool(name, unlocked);
    auto curcoins = getInventory("Coin");
    setInventory("Coin", curcoins - cost);
}

void earnResource(const char* resource, int amount, const char* category,
        const char* item) {
    setInventory(resource, getInventory(resource)+amount);
    resource_event(R_SOURCE, amount, resource, category, item);

}

const char * game_data_key = "GameData";

void setGameSession(int session_num) {
    auto data = fetch_persistent<game_data>(game_data_key);
    data.session_num = session_num;
    persist(game_data_key, data);
}

int getGameSession() {
    return fetch_persistent<game_data>(game_data_key).session_num;
}
