/*
 * GameLevel.h
 *
 *  Created on: Jun 6, 2015
 *      Author: eba
 */

#ifndef GAMELEVEL_H_
#define GAMELEVEL_H_

#include <memory>

#include <orxFSM/behavior_header.hpp>
#include <orxFSM/behavior_context.hpp>
#include <scroll_ext/eventdispatcher.h>

#include <gesturemanager.h>
#include <util/orx_pointers.hpp>
#include "../aspects.h"

class World;
class Simulator;
class PhysicsSoundManager;

class game_context: public event_dispatcher {
public:
    typedef int AspectTag;
    std::unique_ptr<World> world;
    std::unique_ptr<Simulator> simulator;

    game_context();
    virtual ~game_context();
    virtual void addScore(const char * score_type, orxS32 delta, const orxVECTOR & pos) = 0;
    virtual int getScore() = 0;
    virtual void changeHappiness(float delta) = 0;
    void UpdateLevel(const orxCLOCK_INFO& _rstInfo);

};
ACTION_LEAF(change_happiness, &game_context::changeHappiness)
ACTION_LEAF(add_score, &game_context::addScore)
RETURNING_MEMBER_ACTION_LEAF(get_score, game_context, getScore);

class ScoreDisplay;
class HappinessSidebar;

struct score_event {
    using signature = void(const char *, int);
};

struct eat_event {
    using signature = void();
};

class Sparrow;
struct sparrow_leave_event {
    using signature = void(Sparrow *);
};

struct snatch_event {
    using signature = void();
};

struct crow_harass_event {
    using signature = void(orxVECTOR);
};

struct temperature_status {
    enum location{OUTSIDE, ONFRAME};
    using signature = orxFLOAT(location);
};

struct scariness_status {
    using signature = orxFLOAT();
};

struct happiness_change_event {
    using signature = void(orxFLOAT newHappiness);
};

class Crow;
struct crow_beaten_event {
    using signature = void(Crow *);
};

struct bite_event {
    using signature = void();
};

class GameLevel : public game_context, public game_screen, public behavior_context_mixin<GameLevel> {
    std::unique_ptr<PhysicsSoundManager> soundManager;
    orxVIEWPORT * internal_viewport;
    orxVIEWPORT * screen_viewport;
    game_gesture_manager g_manager;
    orxFLOAT happiness = 0.5;
    orxFLOAT scariness_penalty;
    const char * nextLevel = nullptr;
    int score = 0;
    int score_multiplier = 1;
    orxOBJECT * multiplier_tag = nullptr;
    bool level_paused = false;
    bool behavior_paused = false;
public:
    orxFLOAT temperature;
    ScoreDisplay * scoreDisp = nullptr;
    HappinessSidebar * happinessBar = nullptr;
    SCROLL_BIND_CLASS("GameLevel")
    virtual gesture_manager & GetGestureManager();
    virtual void addScore(const char * score_type, orxS32 delta, const orxVECTOR & pos);
    virtual int getScore() {return score;}
    virtual void setScoreMultiplier(int new_val);
    void changeHappiness(float delta, bool with_fx);
    virtual void changeHappiness(float delta);
    void setHappiness(float newValue);
    void setScarinessPenalty(float newValue) {scariness_penalty = newValue;};
    virtual void EndScreen();
    const char * GetNextScreen(){return nextLevel;};
    virtual const char * GetScreenName() {return GetModelName();}
    virtual void SetOutputTexture(orxTEXTURE*);
    orxVIEWPORT * GetScreenViewport() {return screen_viewport;}
    void SetNextScreen(const char * l){nextLevel=l;}
    void ExtOnCreate();
    void Update(const orxCLOCK_INFO &);
    void newHappinessSidebar();
    void newScoreDisplay();
    void PauseLevel(bool pause);
    void PauseBehavior(bool pause) {behavior_paused=pause;}
    bool IsLevelPaused() {return level_paused;}
    orxOBJECT * CreateTool(const char * tool_name);
    orxFLOAT getHappiness() {return happiness;}
    GameLevel();
    ~GameLevel();
};

RETURNING_MEMBER_ACTION_LEAF(get_happiness, GameLevel, getHappiness)

ACTION_LEAF(new_score_display, &GameLevel::newScoreDisplay)
ACTION_LEAF(new_happiness_sidebar, &GameLevel::newHappinessSidebar)
ACTION_LEAF(pause_level, &GameLevel::PauseLevel)
ACTION_LEAF(set_happiness, &GameLevel::setHappiness)
ACTION_LEAF(set_scariness_penalty, &GameLevel::setScarinessPenalty)
ACTION_LEAF(create_tool, &GameLevel::CreateTool)

orxOBJECT * get_happiness_sidebar_action(GameLevel * level);
RETURNING_ACTION_LEAF(get_happiness_sidebar, get_happiness_sidebar_action)

orxOBJECT * get_score_display_action(GameLevel * level);
RETURNING_ACTION_LEAF(get_score_display, get_score_display_action)

class apply_force_behavior: public behavior_t {
    ExternalForce * force;
    weak_object_ptr applier;
    public:
    apply_force_behavior(game_context * level, orxOBJECT * applier);
    behavior_state run(const orxCLOCK_INFO & clock);
    ~apply_force_behavior() {if(force) force->remove();}
};
BEHAVIOR_LEAF(apply_force, apply_force_behavior)

orxU32 get_node_count_action(GameLevel * level);
RETURNING_ACTION_LEAF(get_node_count, get_node_count_action)

#endif /* GAMELEVEL_H_ */
