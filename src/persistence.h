/*
 * persistence.h
 *
 *  Created on: Sep 23, 2015
 *      Author: eba
 */

#ifndef PERSISTENCE_H_
#define PERSISTENCE_H_

struct level_data {
    bool score_levels[3];
    bool challenges[2];
};

struct settings_data {
    bool soundOff;
    bool musicOff;
    int languageIndex;
};

struct test_data {
    bool test_mode;
};

struct infinite_mode_data {
    int record;
};

struct game_data {
    int session_num;
};

void init_persistence();
void persist(const char * key, level_data object);
void persist(const char * key, settings_data object);
void persist(const char * key, test_data object);
void persist(const char * key, infinite_mode_data object);
void persist(const char * key, game_data object);

template <class T>
T fetch_persistent(const char * key) {
    T result;
    fetch_persistent(key, result);
    return result;
}

void fetch_persistent(const char * key, level_data & object);
void fetch_persistent(const char * key, settings_data & object);
void fetch_persistent(const char * key, test_data & object);
void fetch_persistent(const char * key, infinite_mode_data & object);
void fetch_persistent(const char * key, game_data & object);
void reload_persistence(const char * file);

class PersistentTransaction {
    bool should_commit;
public:
    PersistentTransaction(const char * persistent_section, bool should_commit);
    ~PersistentTransaction();
};

void persist_command_guard(const char * key, float val);
float fetch_command_guard(const char * key);

void persist_action_log(const char * action_name, int session_num);
int fetch_action_log(const char * action_name);

void persist_flag(const char * flag_name, bool value);
bool fetch_flag(const char * flag_name);

#endif /* PERSISTENCE_H_ */
