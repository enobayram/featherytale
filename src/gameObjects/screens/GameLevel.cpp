/*
 * GameLevel.cpp
 *
 *  Created on: Jun 6, 2015
 *      Author: eba
 */
#include <sstream>

#include <orxFSM/behavior.hpp>

#include <orx_utilities.h>
#include <util/orx_eigen.h>
#include <util/view_utilities.h>
#include <util/random.hpp>

#include <physicalElements/World.h>
#include <Simulator.h>

#include <analytics.h>

#include "GameLevel.h"
#include "../Score.h"
#include "../HappinessSidebar.h"
#include "../Button.h"
#include "../dialogs/Dialog.h"
#include <pancarDemo.h>

#include <gameObjects/ForceArrow.h>
#include <gameObjects/tools/Tool.h>
#include <gameObjects/SmartText.h>

using namespace std::placeholders;

class PhysicsSoundManager {
    typedef std::vector<weak_object_ptr> sound_list;
    sound_list tearSounds;
    sound_list squashSounds;
    sound_list wobbleSounds;
    sound_list frictionSounds;
    orxFLOAT maxMergeDistanceSq, wobbleMinInitAmount, wobbleInitVolume, wobbleIncVolume,
             squashInitVolume, squashIncVolume, frictionInitVolume, frictionIncVolume,
             tearInitVolume, tearIncVolume;
public:
    PhysicsSoundManager(event_dispatcher & physical_events_dispatcher, const char * section) {
        ConfigSectionGuard guard(section);
        physical_events_dispatcher.register_handler<energy_change_event>(
            std::bind(&PhysicsSoundManager::handle_energy_change, this, _1, _2, _3, _4)
        );
        physical_events_dispatcher.register_handler<bond_tear_event>(
            std::bind(&PhysicsSoundManager::handle_bond_tear, this, _1)
        );
        auto maxMergeDistance = orxConfig_GetFloat("MaxMergeDistance");
        maxMergeDistanceSq = maxMergeDistance*maxMergeDistance;
        wobbleMinInitAmount = orxConfig_GetFloat("WobbleMinInitAmount");
        wobbleInitVolume = orxConfig_GetFloat("WobbleInitVolume");
        wobbleIncVolume = orxConfig_GetFloat("WobbleIncVolume");
        squashInitVolume = orxConfig_GetFloat("SquashInitVolume");
        squashIncVolume = orxConfig_GetFloat("SquashIncVolume");
        frictionInitVolume = orxConfig_GetFloat("FrictionInitVolume");
        frictionIncVolume = orxConfig_GetFloat("FrictionIncVolume");
        tearInitVolume = orxConfig_GetFloat("TearInitVolume");
        tearIncVolume = orxConfig_GetFloat("TearIncVolume");
    }
    void time_update_sound_list(sound_list & sl, orxFLOAT dt, orxFLOAT time_constant) {
        sl.erase(std::remove_if(sl.begin(),
                                sl.end(),
                                [](weak_object_ptr wptr){return !getSound(wptr);}),
                 sl.end()
        );
        for(auto w_ptr: sl) {
            auto sound = getSound(w_ptr);
            orxSound_SetVolume(sound, orxSound_GetVolume(sound) * std::exp(-time_constant*dt));
        }
    }
    void time_update(orxFLOAT dt) {
        time_update_sound_list(tearSounds, dt, 10);
        time_update_sound_list(squashSounds, dt, 10);
        time_update_sound_list(wobbleSounds, dt, 10);
        time_update_sound_list(frictionSounds, dt, 10);
    }
    void handle_energy_change(const Vector2d & pos_, double amount, bool dissipated, bool radial) {
        sound_list * sl;
        orxVECTOR pos = {(float) pos_.x(), (float) pos_.y(), 0};
        const char * sound_name;
        if(!dissipated) {
            insert_sound(wobbleSounds, pos, wobbleIncVolume, "CheeseWobbleSound",
                         amount > wobbleMinInitAmount ? wobbleInitVolume : 0);
        }
        else if(radial) {
            insert_sound(squashSounds, pos, squashIncVolume, "CheeseSquashSound",
                         squashInitVolume);
        }
        else {
            insert_sound(frictionSounds, pos, frictionIncVolume, "CheeseFrictionSound",
                         squashInitVolume);
        }
    }
    void handle_bond_tear(Bond * bond) {
        Vector2d bondCenter_ = (bond->node1.pos + bond->node2.pos)/2;
        orxVECTOR bondCenter = {(float)bondCenter_.x(), (float)bondCenter_.y(),0};
        insert_sound(tearSounds, bondCenter, tearIncVolume, "BondTearSound", tearInitVolume);
    }
    bool insert_sound(sound_list & sl, const orxVECTOR & pos, orxFLOAT increment, const char * new_sound_name, orxFLOAT new_sound_volume) {
        for(auto wptr: sl) {
            orxOBJECT * obj = wptr;
            if(!obj) continue;
            orxVECTOR soundPos; orxObject_GetPosition(obj, & soundPos);
            if(squaredNorm(soundPos - pos) < maxMergeDistanceSq) {
                auto sound = orxObject_GetLastAddedSound(obj);
                if(!sound) continue;
                auto curVolume = orxSound_GetVolume(sound);
                orxSound_SetVolume(sound, std::min(1.0f, curVolume+increment));
                orxObject_SetPosition(obj, &pos);
                return true;
            }
        }
        if(new_sound_volume!=0) {
            auto obj = CreateSoundObject(new_sound_name, pos);
            orxVECTOR v;
            orxSound_SetVolume(orxObject_GetLastAddedSound(obj), new_sound_volume);
            sl.push_back(obj);
            return true;
        }
        return false;
    }
};

game_context::game_context(): world(new World), simulator(new Simulator(*world)) {
    simulator->init();
    world->timeMultiplier = 2;
}

game_context::~game_context() {
}

void game_context::UpdateLevel(const orxCLOCK_INFO& _rstInfo) {
    World::seconds initialWorldTime = world->getTime();
    while(world->getTime()<initialWorldTime+ World::seconds(_rstInfo.fDT)) {
        simulator->iterate();
    }
}

using namespace std;

gesture_manager& GameLevel::GetGestureManager() {
    return g_manager;
}

void GameLevel::ExtOnCreate() {
    scoreDisp = FindOwnedChild<ScoreDisplay>();
    happinessBar = FindOwnedChild<HappinessSidebar>();

    happiness = 0.5;
    changeHappiness(0, false);

    orxFLOAT wWidth = ::GetValue<orxFLOAT>("World", "Width"), wHeight = ::GetValue<orxFLOAT>("World", "Height");

    adjustFrustumForArea(screen_viewport, wWidth+1, wHeight+1);

    auto make_click_handler = [this](double radius) {
        return [this, radius](trail & t)-> gesture *{
            trace trc = t.traces.back();
            Vector2d forcePoint = {trc.x,trc.y};
            Node * closest = world->closestNode(forcePoint, radius);
            if(closest) {
                double force_radius = 1.5;
                if(closest->getBonds().size() < 4) force_radius = min(radius, (closest->pos-forcePoint).norm()+0.3);
                ExternalForce * newF = world->createExternalForce(forcePoint, force_radius);
                if(newF) return new force_gesture(newF, *world, &t);
            }
            return nullptr;
        };
    };

    CreateChild<Button>("WorldCloseClickHandler")->set_handler(make_click_handler(orxConfig_GetFloat("PhysicsCloseRadius")));
    CreateChild<Button>("WorldDistantClickHandler")->set_handler(make_click_handler(orxConfig_GetFloat("PhysicsDistantRadius")));

    auto pause_button = CreateChild<Button>("PauseIcon");
    pause_button->set_handler([=]{
        ConfigSectionGuard guard(GetModelName());
        PauseBehavior(true);
        PauseLevel(true);
        auto pause_dialog = CreateChild<Dialog>("PauseDialog");
        auto mission_text = pause_dialog->FindOwnedChild<SmartText>("MissionText");
        mission_text->SetText(LocaleAwareString("MissionExplanation"));
        auto menuicon = CreateChild<Button>("MenuIcon");
        menuicon->set_handler([=]{
            SetNextScreen(GetValue<const char *>("MenuScreen"));
            progression_event(P_FAIL, GetModelName());
        });
        auto resumeicon = CreateChild<Button>("ResumeIcon");
        resumeicon->set_handler([=]{
            PauseBehavior(false);
            PauseLevel(false);
            pause_dialog->SetLifeTime(0);
        });
        auto restarticon = CreateChild<Button>("RestartIcon");
        restarticon->set_handler([=]{
            SetNextScreen(GetModelName());
            progression_event(P_FAIL, GetModelName());
        });
        pause_dialog->AddButton(menuicon->GetOrxObject(), -100);
        pause_dialog->AddButton(resumeicon->GetOrxObject(), 0);
        pause_dialog->AddButton(restarticon->GetOrxObject(), 100);
    });

    // CreateChild<Button>("ConsoleIcon");

    temperature = GetValue<orxFLOAT>("Temperature");
    register_handler<temperature_status>([=](temperature_status::location){return temperature;});

    for(auto observer: ConfigListRange<const char *>("Observers")) CreateChild(observer);
    for(auto tool: ConfigListRange<const char *>("Tools")) CreateTool(tool);
    setScarinessPenalty(orxConfig_GetFloat("ScarinessPenalty"));
    progression_event(P_START, GetModelName());
    MIXIN::ExtOnCreate();

    {
        ConfigSectionGuard guard("Runtime");
        orxConfig_SetU32("LevelsPlayed", orxConfig_GetU32("LevelsPlayed")+1);
    }
}

void debugActions(GameLevel & level);

void GameLevel::Update(const orxCLOCK_INFO& _rstInfo) {
    debugActions(*this);
    if(!behavior_paused) MIXIN::Update(_rstInfo);
    if(!level_paused) {
        UpdateLevel(_rstInfo);
        soundManager->time_update(_rstInfo.fDT);
        auto scariness = query<scariness_status>();
        if(scariness) {
            if(poisson_event_generator(_rstInfo.fDT)(1/scariness)) {
                changeHappiness(-scariness_penalty);
            }
        }
    }
    else world->performDeletions();
}

void GameLevel::changeHappiness(float delta) { changeHappiness(delta, true);}

void GameLevel::changeHappiness(float delta, bool with_fx) {
    happiness=min(1.f, max(.0f,happiness+delta));
    if(happinessBar) happinessBar->setHappiness(happiness, with_fx);
    fire_event<happiness_change_event>(happiness);
}

void debugActions(GameLevel & level) {
    auto viewport = level.GetGestureManager().viewport;
    if(IsPressed("RestartLevel")) level.SetNextScreen(level.GetModelName());
    if(IsPressed("QuitLevel")) level.SetNextScreen("OpeningMenuScreen");

    if(orxInput_IsActive("DeleteNode") && orxInput_HasNewStatus("DeleteNode")) {
        auto msWrld = getMouseInWorld(viewport);
        if(msWrld) {
            auto closest = level.world->closestNode({msWrld->fX, msWrld->fY});
            if(closest) level.world->removeNode(closest);
        }
    }

    if(orxInput_IsActive("CreateScorePiece") && orxInput_HasNewStatus("CreateScorePiece")) {
        auto msWrld = getMouseInWorld(viewport);
        if(msWrld) level.addScore("debug", 3, *msWrld);
    }
    if(IsPressed("PauseGame")) {
        bool shouldPause = !level.IsLevelPaused();
        level.PauseBehavior(shouldPause);
        level.PauseLevel(shouldPause);
    }

    if(IsPressed("IncHappiness")) level.changeHappiness(0.2);
    if(IsPressed("DecHappiness")) level.changeHappiness(-0.2);

    if(IsPressed("IncTemperature")) level.temperature += 2;
    if(IsPressed("DecTemperature")) level.temperature -= 2;
}

void GameLevel::addScore(const char * score_type, orxS32 delta, const orxVECTOR & pos) {
    if(scoreDisp) {
        delta *= score_multiplier;
        score += delta;
        scoreDisp->addScore(delta, pos);
        fire_event<score_event>(score_type, delta);
    }
}

void GameLevel::newHappinessSidebar() {
    if(happinessBar) happinessBar->SetLifeTime(0);
    happinessBar = CreateChild<HappinessSidebar>();
    happiness = 0.5;
    changeHappiness(0, false);
}

void GameLevel::newScoreDisplay() {
    if(scoreDisp) scoreDisp->SetLifeTime(0);
    scoreDisp = CreateChild<ScoreDisplay>("InGameScoreDisplay");
    score=0;
}

orxOBJECT * get_happiness_sidebar_action(GameLevel * level) {
    return level->happinessBar->GetOrxObject();
}

orxOBJECT * get_score_display_action(GameLevel * level) {
    return level->scoreDisp->GetOrxObject();
}

apply_force_behavior::apply_force_behavior(game_context * level, orxOBJECT * applier):applier(applier) {
    force = level->world->createExternalForce(toEigen2d(GetPosition(applier)), 1.5);
    if(force) ForceArrow::Create(force);
}

behavior_t::behavior_state apply_force_behavior::run(const orxCLOCK_INFO & clock) {
    orxOBJECT * applierObj = applier;
    if(!force || !applierObj || force->getNodes().empty()) {
        return FAILED;
    }
    force->setLocation(toEigen2d(GetPosition(applierObj)));
    return RUNNING;
}


void GameLevel::SetOutputTexture(orxTEXTURE* texture) {
    orxViewport_SetTextureList(screen_viewport, 1, &texture);
}

template <class F>
void WalkChildren(orxOBJECT * obj, F f) {
    for(orxOBJECT * child = orxObject_GetOwnedChild(obj);
        child != nullptr;
        child = orxObject_GetOwnedSibling(child)) {
        f(child);
        WalkChildren(child, f);
    }
}

void GameLevel::PauseLevel(bool pause) {
    if(pause && !level_paused) {
        level_paused = true;
        WalkChildren(GetOrxObject(),[](orxOBJECT * obj){
            auto eobj = scroll_cast<ExtendedMonomorphic>(obj,false);
            if(!eobj || eobj->TestFlags(FlagPausable)) orxObject_Pause(obj, true);
            if(eobj) eobj->OnPause(true);
        });
    }
    if(!pause && level_paused) {
        level_paused = false;
        WalkChildren(GetOrxObject(),[](orxOBJECT * obj){
            orxObject_Pause(obj, false);
            if(auto eobj = scroll_cast<ExtendedMonomorphic>(obj,false)) {
                eobj->OnPause(false);
            }
        });
    }
}

void GameLevel::EndScreen() {
    orxViewport_Delete(internal_viewport);
    orxViewport_Delete(screen_viewport);
    PauseLevel(false);
    SetLifeTime(0);
}

GameLevel::GameLevel():
        soundManager(new PhysicsSoundManager(*simulator, "PhysicsSoundManager")),
        internal_viewport(orxViewport_CreateFromConfig("GameInternalViewport")),
        screen_viewport(orxViewport_CreateFromConfig("GameScreenViewport")),
        g_manager(internal_viewport){

}

void GameLevel::setHappiness(float newValue) {
    changeHappiness(newValue-happiness, false);
}

orxOBJECT* GameLevel::CreateTool(const char* tool_name) {
    auto tool = CreateChild<Tool>(tool_name);
    tool->AddBonus(GetValue<orxU32>(tool_name));
    return tool->GetOrxObject();
}

GameLevel::~GameLevel() {
}

void GameLevel::setScoreMultiplier(int new_val) {
    score_multiplier = new_val;
    if(new_val == 1 && multiplier_tag) {
        orxObject_SetLifeTime(multiplier_tag, 0);
        multiplier_tag = nullptr;
    } else {
        if(!multiplier_tag) multiplier_tag = scoreDisp->CreateChild("MultiplierTag");
        std::stringstream ss;
        ss<< "x" << score_multiplier;
        orxObject_SetTextString(multiplier_tag, ss.str().c_str());
    }
}

orxU32 get_node_count_action(GameLevel * level) {return level->world->numberOfNodes();}
