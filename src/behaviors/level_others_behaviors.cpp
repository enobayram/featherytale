/*
 * level_others_behaviors.cpp
 *
 *  Created on: Feb 16, 2016
 *      Author: eba
 */

#include "behavior_common.hpp"
#include <orxFSM/event_behaviors.hpp>

behavior_constructor bird_flock_events(const char * period, const char * size) {
    return repeat <<= sequence {
        $i("FlockSize") = $config(size),
        timeout($config<float>(period)),
        collection_scope($os("Flock")) <<= bird_flock_event($i("FlockSize"), $os("Flock"))
    };
}


auto teach_knife_tool_behavior = sequence {
    create_in_collection($os("TutObjs"), $o("Text"), "MigratingSparrowSeasonText"),
    timeout(3),
    add_track($o("Text"), "FadeOutTrack"),
    create_in_collection($os("TutObjs"), $o("Text"), "WhoSaysTheresNoSuchThingText", false),
    timeout(3),
    fade($o("Text"), 0.5),
    create_in_collection($os("TutObjs"), $o("Text"), "SparrowFlockSymbolExplanation"),
    bird_flock_event($i("FlockSize"), $os("Flock"), "InstructionFlockSparrow"),
    fade($o("Text"), 0.5),
    create_in_collection($os("TutObjs"), $o("Text"), "ItCanBeHardToFeedAFlockText"),
    timeout(3),
    fade($o("Text"), 0.5),
    create_in_collection($os("TutObjs"), $o("Text"), "KnifeToolCanHelpText"),
    timeout(0.5),
    create($o("KnifeTool"), "InstructionKnifeTool"),
    point_at_object_in_tutorial($o("KnifeTool"), $o("Arrow")),
    wait_for_signal($o("^"), "InstructionKnifeToolSelected"),
    destroy($o("Arrow")),
    fade($o("Text"), 0.5),
    create_in_collection($os("TutObjs"), $o("Text"), "UseKnifeToFeedText"),
    create_in_collection($os("TutObjs"), $o("InstructionCheese"), "KnifeToolInstructionCheese"),
    any {
        attach_to_object($o("InstructionCheese")),
        sequence {
            timeout(4),
            fade($o("Text"), 0.5),
            wait(),
        }
    }
};

auto knife_tool_introduction_level_behavior = sequence {
    $i("FlockSize") = 10,
    collection_scope($os("Flock")) <<= sequence {
        skippable <<= teach_knife_tool_behavior,
        for_collection($o("Sparrow"), $os("Flock")) <<= set_controller($o("Sparrow"), "SparrowLeaveController"),
    },
    new_score_display($o("^")),
    create($o("Text"), "KnifeToolIntroLevelGoalText"),
    timeout(2),
    destroy($o("KnifeTool"), false),
    create_tool($o("KnifeTool"), $o("^"), "KnifeTool"),
    add_track($o("Text"), "LateFadeOutTrack"),
    create($o("Crow"), "WeakCrow"),
    any {
        level_countdown(),
        happiness_bar_mission(),
        spawn_sparrows_randomly($o("^"), "Sparrow"),
        spawn_cheese_when_finished(),
        bird_flock_events("FlockPeriod","FlockSize"),
        full_happiness_reached("Challenge1"),
        receive_signal_fewer_than_challenge($o("^"), $b("Challenge2"), "KnifeToolSelected", 1)
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(knife_tool_introduction_level_behavior, "knife_tool_introduction_level_behavior")

auto henchman_introduction_level_behavior = sequence {
    create($o(""), "Crow", false),
    create($o("Text"), "BeCarefulHenchmanText"),
    timeout(3),
    create($o("Crow"), "WeakCrow", false),
    add_track($o("Text"), "LateFadeOutTrack"),
    any {
        level_countdown(),
        happiness_bar_mission(),
        spawn_sparrows_randomly($o("^"), "Sparrow"),
        spawn_cheese_when_finished(),
        no_snatch_challenge($o("^"), $b("Challenge1")),
        consecutive_baskets_challenge($o("^"), $b("Challenge2")),
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(henchman_introduction_level_behavior, "henchman_introduction_level_behavior")

auto introduce_happiness_tool_behavior = sequence {
    create_in_collection($os("TutObjs"),$o("Text"), "ThisIsGettingSeriousText"),
    timeout(3),
    fade($o("Text"), 0.5),
    create_in_collection($os("TutObjs"),$o("Text"), "HappinessToolCanHelpText"),
    timeout(0.3),
    create_in_collection($os("TutObjs"),$o("HappinessTool"), "InstructionHappinessTool"),
    point_at_object_in_tutorial($o("HappinessTool"), $o("Arrow")),
    wait_for_signal($o("^"), "InstructionHappinessToolSelected"),
    destroy($o("Arrow")),
    fade($o("Text"), 0.5),
    create_in_collection($os("TutObjs"),$o("Text"), "ItBoostsHappinessText"),
    timeout(5),
    add_track($o("Text"), "FadeOutTrack"),
};

auto henchmen_more_aggressive_level_behavior = sequence {
    create_listed_crows(),
    skippable <<= introduce_happiness_tool_behavior,
    set_happiness($o("^"), 0.5),
    destroy($o("HappinessTool"), false),
    create_tool($o("HappinessTool"), $o("^"), "BoostHappinessTool"),
    any {
        level_countdown(),
        happiness_bar_mission(),
        spawn_sparrows_randomly($o("^"), "Sparrow"),
        spawn_cheese_when_finished(),
        no_harass_challenge($o("^"), $b("Challenge1")),
        full_happiness_reached("Challenge2"),
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(henchmen_more_aggressive_level_behavior, "henchmen_more_aggressive_level_behavior")

auto hardest_conditions_pre_father_level_behavior = sequence {
    $f("TemperatureDiff") = -15,
    create_listed_crows(),
    any {
        winter_event(wait(), $f("TemperatureDiff")),
        sequence {
            create($o("Text"), "VeryHardConditionsText"),
            timeout(3),
            add_track($o("Text"), "LateFadeOutTrack"),
            any {
                level_countdown(),
                happiness_bar_mission(),
                spawn_sparrows_randomly($o("^"), "Sparrow"),
                spawn_cheese_when_finished(),
                bird_flock_events("FlockPeriod","FlockSize"),
                no_snatch_or_harass_challenge($o("^"), $b("Challenge1")),
                full_happiness_reached("Challenge2"),
            },
        }
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(hardest_conditions_pre_father_level_behavior, "hardest_conditions_pre_father_level_behavior")

behavior_constructor periodic_murder_attack() {
    return repeat <<= sequence {
        timeout($config<orxFLOAT>("ArmyPreWait")),
        murder_attack(),
        send_signal($o("^"), "MurderAttackOver"),
        timeout($config<orxFLOAT>("ArmyPostWait")),
    };
}

auto father_intro_level_behavior = sequence {
    create($o("Text"), "FatherIsVeryAngryText"),
    timeout(2),
    create($o("Father"), "IntroCrowFather"),
    timeout(1.5),
    add_track($o("Text"), "LateFadeOutTrack"),
    any {
        level_countdown(),
        happiness_bar_mission(),
        spawn_sparrows_randomly($o("^"), "Sparrow"),
        spawn_cheese_when_finished(),
        keep_running() <<= sequence {
            wait_for_signal($o("^"), "MurderAttackOver"),
            create_listed_crows(),
        },
        periodic_murder_attack(),
        crow_beat_challenge($b("Challenge1")),
        unhappy_bird_left("Challenge2"),
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(father_intro_level_behavior, "father_intro_level_behavior")

auto father_more_aggressive_level_behavior = sequence {
    create_listed_crows(),
    create($o("Father"), "IntroCrowFather"),
    timeout(1.5),
    create($o("Text"), "YouKnowHowToDealText"),
    timeout(2),
    add_track($o("Text"), "LateFadeOutTrack"),
    any {
        level_countdown(),
        happiness_bar_mission(),
        spawn_sparrows_randomly($o("^"), "Sparrow"),
        spawn_cheese_when_finished(),
        periodic_murder_attack(),
        send_birds_happy_challenge($b("Challenge1")),
        crow_beat_challenge($b("Challenge2")),
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(father_more_aggressive_level_behavior, "father_more_aggressive_level_behavior")

behavior_constructor display_fight_event(const char * signal, const char * text_section) {
    return repeat <<= sequence {
        wait_for_signal($o("Father"), signal),
        create($o(""), text_section, false),
    };

}

auto instructive_fight_behavior = any {
    repeat <<= any {
        sequence {
            create_in_collection($os("TutObjs"),$o("HitText"), "FightHitText"),
            $i("HitCount") = 0,
            any {
                attach_to_object($o("HitText")),
                wait_for_signal($o("Father"), "PUNCHED", true),
            }
        },
        repeat <<= sequence {
            wait_for_signal($o("Father"), "PUNCHED"),
            increment($i("HitCount")),
            locale_aware_string($cp("HitTextFormat"),"HitTextFormat"),
            sprint($s("HitTextString"), $cp("HitTextFormat"), $i("HitCount")),
            set_text($o("HitText"), $cp("HitTextString")),
        }
    },
    display_fight_event("BLOCKED", "FightBlockedText"),
    display_fight_event("MISSED", "FightMissedText"),
    display_fight_event("ESCAPED", "FightEscapedText"),
    keep_running() <<= sequence {
        wait_for_signal($o("Father"), "BLOCKED"),
        create_in_collection($os("TutObjs"),$o(""), "YouLoseHappinessWhenBite", false),
    },
    wait_for_state($o("Father"), "Stunned"),
};

auto teach_father_fighting_behavior = sequence {
    wait_for_state($o("Father"), "Stay"),
    timeout(0.5),
    set_controller($o("Father"), "InstructionDiveConverger"),
    create_in_collection($os("TutObjs"),$o("Curtain"), "UserCurtain"),
    wait_for_state($o("Father"), "Hover"),
    get_happiness_sidebar($o("HappinessSidebar"), $o("^")),
    point_at_object_in_tutorial($o("HappinessSidebar"), $o("Arrow")),
    create_in_collection($os("TutObjs"),$o("Text"), "FatherIsScaryText"),
    timeout(5),
    destroy($o("Arrow")),
    fade($o("Text"), 0.5),
    create_in_collection($os("TutObjs"),$o("Text"), "AttackFatherImmediatelyText"),
    timeout(3.5),
    destroy($o("Curtain")),
    set_controller($o("Father"), "FatherInstructionController"),
    set_constraints($o("Father"), "FatherKeepFightingConstraints"),
    fade($o("Text"), 0.5),
    set_happiness($o("^"), 0.5),
    instructive_fight_behavior,
    set_constraints($o("Father"), "CrowStayConstraints"),
    timeout(1),
    create_in_collection($os("TutObjs"),$o("Text"), "FightCongratulationsText"),
    timeout(2),
    fade($o("Text"), 0.5),
    create_in_collection($os("TutObjs"),$o("Text"), "SlowTimeCanHelp"),
    timeout(0.5),
    create($o("SlowTimeTool"), "InstructionSlowTimeTool"),
    point_at_object_in_tutorial($o("SlowTimeTool"), $o("Arrow")),
    wait_for_signal($o("^"), "InstructionSlowTimeToolSelected"),
    destroy($o("Arrow")),
    set_controller($o("Father"), "InstructionDiveConverger"),
    set_constraints($o("Father"), "FatherKeepFightingConstraints"),
    add_track($o("Text"), "FadeOutTrack"),
    wait_for_state($o("Father"), "Dive"),
    set_controller($o("Father"), "CrowFatherController"),
    create_in_collection($os("TutObjs"),$o("Text"), "TryToBeatWhenSlow"),
    wait_for_state($o("Father"), "Stunned"),
};

auto father_fights_back_level_behavior = sequence {
    create($o("Father"), "CrowFather"),
    skippable <<= teach_father_fighting_behavior,
    set_controller($o("Father"), "CrowStayConverger"),
    set_constraints($o("Father"), "CrowFatherConstraints"),
    wait_for_state($o("Father"), "Stay"),
    destroy($o("SlowTimeTool"), false),
    create_tool($o("SlowTimeTool"), $o("^"), "SlowTimeTool"),
    set_controller($o("Father"), "StoryCrowFatherController"),
    set_scariness_penalty($o("^"), $config("ScarinessPenalty")),
    set_happiness($o("^"), 0.5),
    timeout(1.5),
    create_listed_crows(),
    create($o("Text"), "MafiaIsAllInText"),
    timeout(2),
    add_track($o("Text"), "LateFadeOutTrack"),
    any {
        level_countdown(),
        happiness_bar_mission(),
        spawn_sparrows_randomly($o("^"), "Sparrow"),
        spawn_cheese_when_finished(),
        periodic_murder_attack(),
        no_bite_challenge($o("^"), $b("Challenge1")),
        no_snatch_or_harass_challenge($o("^"), $b("Challenge2")),
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(father_fights_back_level_behavior, "father_fights_back_level_behavior")
