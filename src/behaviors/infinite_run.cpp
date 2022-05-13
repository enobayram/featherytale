/*
 * infinite_run.cpp
 *
 *  Created on: Jan 2, 2016
 *      Author: eba
 */

#include <algorithm>
#include "behavior_common.hpp"
#include "game_state.h"
#include <analytics.h>
#include <orxFSM/event_behaviors.hpp>
#include <orxFSM/configfunction.h>
#include <util/random.hpp>
#include <util/probabilistic_choice.hpp>
#include <gameObjects/Crow.h>
#include <gameObjects/dialogs/Dialog.h>
#include <gameObjects/PushButton.h>

auto spawn_sparrows = any {
    output_signal($f("SparrowSpawnPeriod"), "SparrowSpawnPeriodTimeline"),
    spawn_sparrows_with_period($o("^"), $f("SparrowSpawnPeriod"), "InfiniteRunSparrow")
};

auto spawn_dealer = keep_running() <<= sequence {
    timeout($config<orxFLOAT>("DealerSpawnTime")),
    create($o(""), "WeakCrow", false)
};

template <class T>
void refresh_collection(std::vector<weak_scroll_ptr<T>> & collection) {
    collection.erase(
        std::remove_if(collection.begin(), collection.end(),
            [](weak_scroll_ptr<T> & p) { return !p.get();}),
        collection.end());
}

struct spawn_henchmen_behavior: public behavior_t {
    orxFLOAT time_elapsed = 0;
    std::unique_ptr<config_function> min_func, max_func;
    std::vector<weak_scroll_ptr<Crow>> crows;
    orxFLOAT henchmenMovePeriod;
    weak_scroll_ptr<GameLevel> level;
    spawn_henchmen_behavior(GameLevel * level):
        min_func(config_function::create_from_section("HenchmenMinCount")),
        max_func(config_function::create_from_section("HenchmenMaxCount")),
        henchmenMovePeriod(GetValue<orxFLOAT>("InfiniteRun", "HenchmenMovePeriod")),
        level(level) {}
    behavior_state run(const orxCLOCK_INFO & clock) {
        time_elapsed += clock.fDT;
        if(poisson_event_generator(clock.fDT)(henchmenMovePeriod)) {
            auto level = this->level.get();
            if(level) {
                refresh_collection(crows);
                random_action({
                    {
                        [=]{crows.push_back(level->CreateChild<Crow>("HenchmanInfinite"));},
                        crows.size() < max_func->apply(time_elapsed)
                    },
                    {
                        [=]{crows.back()->SetController("CrowExitConverger");},
                        crows.size() > min_func->apply(time_elapsed)
                    },
                    {[]{}, 0.001}
                }, gen);
            }
        }
        return RUNNING;
    }
};
BEHAVIOR_LEAF(spawn_henchmen, spawn_henchmen_behavior)

struct show_continue_dialog_behavior: behavior_t {
    in_out_variable<bool> terminate;
    orxS32 price;
    GameLevel * level;
    Dialog * dialog;
    enum {WAITING, ACCEPTED, REJECTED} response;
    void handle_response(bool accepted) {
        level->PauseLevel(false);
        level->PauseBehavior(false);
        dialog->SetLifeTime(0);
        response = accepted ? ACCEPTED : REJECTED;
    }
    show_continue_dialog_behavior(in_out_variable<bool> terminate, GameLevel * level, orxU32 continue_count):
        terminate(terminate), level(level), response(WAITING) {
        price = pow(2, continue_count);
        orxS32 curhearts = getInventory("GoldenHeart");
        level->PauseLevel(true);
        level->PauseBehavior(true);
        dialog = level->CreateChild<Dialog>("InfiniteRunContinueDialog");
        auto accept_button = dialog->FindOwnedChild<PushButton>("AcceptContinueButton");
        auto reject_button = dialog->FindOwnedChild<PushButton>("RejectContinueButton");
        auto heart_display  = dialog->FindOwnedChild("ContinueDialogHeartDisplay");
        auto price_display = dialog->FindOwnedChild("ContinueDialogPriceDisplay");
        reject_button->set_handler([=]{handle_response(false);});
        orxObject_SetTextString(heart_display, to_string(curhearts).c_str());
        orxObject_SetTextString(price_display, to_string(price).c_str());

        if(curhearts >= price) accept_button->set_handler([=]{ handle_response(true);});
        else                  accept_button->set_disabled(true);
    }
    behavior_state run(const orxCLOCK_INFO & clock) {
        switch(response) {
        case WAITING: return RUNNING;
        case ACCEPTED:
            level->setHappiness(level->GetValue<orxFLOAT>("HappinessWhenContinue"));
            setInventory("GoldenHeart", getInventory("GoldenHeart") - price);
            resource_event(R_SINK, price, "GoldenHeart", "Continue", level->GetModelName());
            return SUCCEEDED;
        case REJECTED:
            terminate = true;
            return SUCCEEDED;
        }
        return FAILED; // Since this should never happen
    }
};
BEHAVIOR_LEAF(show_continue_dialog, show_continue_dialog_behavior)

behavior_constructor crow_father_phase(const char * phase, bool repeat) {
    return periodically(phase, "Arrivals", repeat) <<= sequence {
        create($o("Father"), $config<std::string>(phase, "Father")),
        periodically(phase, "MurderAttacks", false) <<= murder_attack(),
        set_controller($o("Father"), "CrowExitConverger"),
        attach_to_object($o("Father")),
    };
};

auto spawn_father = sequence {
    crow_father_phase("CrowFatherInfinitePhase1", false),
    crow_father_phase("CrowFatherInfinitePhase2", false),
    crow_father_phase("CrowFatherInfinitePhase3", true),
};

BEHAVIOR_LEAF(temperature_responder, query_responder<temperature_status>)

auto winter_events = sequence {
    timeout($config<float>("FirstWinter")),
    any {
        output_signal($f("WinterOccurrancePeriod"), "WinterOccurrancePeriodTimeline"),
        output_signal($f("WinterLength"), "WinterLengthTimeline"),
        output_signal($f("WinterTemperatureDiff"), "WinterTemperatureDiffTimeline"),
        repeat <<= sequence {
            winter_event(timeout($f("WinterLength")), $f("WinterTemperatureDiff")),
            poisson_wait($f("WinterOccurrancePeriod")),
        },
    },
};

auto infinite_mode_end_behavior = sequence {
    pause_level($o("^"),true),
    create($o("InfiniteModeEndDialog"), "InfiniteModeEndDialog"),
};

auto bonus_listeners = any {
    count_eats($o("^"), $i("EatCount")),
    count_beats($o("^"), $i("DealerBeatCount"), "WeakCrow"),
    count_beats($o("^"), $i("HenchmanBeatCount"), "HenchmanInfinite"),
    count_beats($o("^"), $i("HenchmanBeatCount"), "ArmyCrow"),
    count_beats($o("^"), $i("FatherBeatCount"), "CrowFather"),
};

auto infinite_run_behavior = sequence {
    $i("FlockSize") = $config<orxU32>("FlockSize"),
    any {
        sequence {
            for_range($i("i"), 0 ,$config<orxU32>("InitCheeseCount")) <<= sequence {
                create($o(""), "InfiniteRunCheeseFlake", false),
                timeout($config<orxFLOAT>("InitCheesePeriod"))
            },
            spawn_cheese_when_finished(),
        },
        spawn_dealer,
        spawn_sparrows,
        spawn_henchmen($o("^")),
        spawn_father,
        periodically("CheeseRains") <<= cheese_rain(),
        periodically("BirdFlocks") <<=
            collection_scope($os("Flock")) <<= bird_flock_event($i("FlockSize"), $os("Flock"), "InfiniteRunFlockSparrow"),
        winter_events,
        bonus_listeners,
        unless($b("Terminate")) <<= sequence {
            wait_for_success <<= zero_happiness($o("^")),
            show_continue_dialog($b("Terminate"), $o("^"), $i("ContinueCount")),
            increment($i("ContinueCount")),
        }
    },
    infinite_mode_end_behavior
};
REGISTER_BEHAVIOR(infinite_run_behavior, "infinite_run_behavior")
