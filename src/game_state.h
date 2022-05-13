/*
 * game_state.h
 *
 *  Created on: Nov 10, 2015
 *      Author: enis
 */

#ifndef GAME_STATE_H_
#define GAME_STATE_H_

#include "persistence.h"

unsigned int getTotalHearts();

level_data fetchLevelData(const char * level_name);

void putLevelData(const char * level_name, level_data object);

bool isMusicOff();
void setMusicOff(bool off);

bool isSoundOff();
void setSoundOff(bool off);

int getLanguageIndex();
void setLanguageIndex(int idx);

void applySettings();

int getInventory(const char * item_name);
void setInventory(const char * item_name, int new_count);
void purchase(const char * item_name, int count, int cost);

bool is_level_dependent_completed(const char * level_name);
bool is_level_locked(const char * level_name);
bool is_level_playable(const char * level_name);

infinite_mode_data fetchInfiniteModeData();
void putInfiniteModeData(infinite_mode_data object);

bool isButtonUnlocked(const char * name);
void setButtonUnlocked(const char * name, bool unlocked, int cost = 0);

void earnResource(const char * resource, int amount, const char * category, const char * item);

void setGameSession(int session_num);
int getGameSession();

#endif /* GAME_STATE_H_ */
