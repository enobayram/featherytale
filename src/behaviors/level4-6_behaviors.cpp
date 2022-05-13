/*
 * level4-6_behaviors.cpp
 *
 *  Created on: Sep 10, 2015
 *      Author: eba
 */

#include "behavior_common.hpp"

inline orxSTATUS below_middle_happiness_action(GameLevel * level) {
    return (level->getHappiness() < 0.5) ? orxSTATUS_SUCCESS : orxSTATUS_FAILURE;
}
ACTION_LEAF(below_middle_happiness, below_middle_happiness_action)

auto teach_happiness_sidebar = sequence {
    tick,
    get_happiness_sidebar($o("HappinessSidebar"), $o("^")),
    add($v("SidebarArrowPos"), $o("HappinessSidebar"), orxVECTOR{2,-5.5,0}),
    create_arrow($o("Arrow"), $v("SidebarArrowPos"), orxVECTOR{-1,1,0}),
    append_object($os("TutObjs"), $o("Arrow")),
    create_in_collection($os("TutObjs"), $o("Text"),"HappinessSidebarExplanationText"),
    timeout(3.5),
    add_track($o("Text"), "FadeOutTrack"),
    add_track($o("Arrow"), "FadeOutTrack"),
    create_in_collection($os("TutObjs"), $o(""), "SadSparrow", false),
    timeout(1),
    create_in_collection($os("TutObjs"), $o("Text"),"SparrowsEffectHappinessText"),
    add($v("IconArrowPos"), $o("HappinessSidebar"), orxVECTOR{2,-2,0}),
    create_arrow($o("Arrow"), $v("IconArrowPos"), orxVECTOR{-1,1,0}),
    append_object($os("TutObjs"), $o("Arrow")),
    timeout(2.5),
    create($o(""), "HappySparrow", false),
    timeout(3.5),
    add_track($o("Text"), "FadeOutTrack"),
    add_track($o("Arrow"), "FadeOutTrack"),
    timeout(2.5),
};

auto teach_happiness_sidebar_level_behavior = sequence {
    skippable <<= teach_happiness_sidebar,
    create($o("Text"), "DontLetHappinessBottomText"),
    timeout(3),
    create($o(""), "StartGameText", false),
    add_track($o("Text"), "FadeOutTrack"),
    any {
        happiness_bar_mission(),
        level_countdown(),
        spawn_sparrows_randomly($o("^")),
        spawn_cheese_when_finished(),
        full_happiness_reached("Challenge1"),
        keep_running() <<= sequence {
            $b("Challenge2") = true,
            wait_for_success <<= below_middle_happiness($o("^")),
            $b("Challenge2") = false
        },
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(teach_happiness_sidebar_level_behavior, "teach_happiness_sidebar_level_behavior")

auto teach_temperature_behavior = sequence {
    create_in_collection($os("TutObjs"), $o("Text"), "NowItsWinterTimeText"),
    timeout(2.5),
    fade($o("Text"), 0.5),
    find_owned_child($o("TemperatureObserver"), $o("^"), "TemperatureObserver"),
    find_owned_child($o("Thermometer"), $o("TemperatureObserver"), "Thermometer"),
    add($v("ThermometerPos"), $o("Thermometer"), orxVECTOR{3.5,0,0}),
    create_arrow($o("Arrow"),$v("ThermometerPos"), orxVECTOR{-2,0,0}),
    append_object($os("TutObjs"), $o("Arrow")),
    create_in_collection($os("TutObjs"), $o("Text"), "ThermometerExplanationText"),
    timeout(3.5),
    fade($o("Text"), 0.5),
    create_in_collection($os("TutObjs"), $o("Text"), "GreenTemperatureExplanation"),
    timeout(3),
    add_track($o("Text"), "FadeOutTrack"),
    any {
        winter_event(wait_for_signal($o("^"),"EndSnow"), $f("TemperatureDiff")),
        sequence {
            create($o("Sparrow"), "FirstWinterInstructionSparrow"),
            timeout(3),
            create_in_collection($os("TutObjs"), $o("Text"), "BlueTemperatureExplanation"),
            timeout(3.5),
            add($v("SparrowHappinessPos"), $o("Sparrow"), orxVECTOR{1.7,-5.5,0}),
            create_arrow($o("Arrow"),$v("SparrowHappinessPos"), orxVECTOR{-1,1,0}),
            append_object($os("TutObjs"), $o("Arrow")),
            fade($o("Text"), 0.5),
            create_in_collection($os("TutObjs"), $o("Text"), "SparrowsUnhappyWhenColdText"),
            timeout(3.5),
            fade($o("Text"), 0.5),
            create($o("Heater"), "InstructionHeaterTool"),
            point_at_object_in_tutorial($o("Heater"), $o("Arrow"), true),
            create_in_collection($os("TutObjs"), $o("Text"), "UseHeaterText"),
            wait_for_signal($o("^"),"InstructionHeaterToolSelected"),
            destroy($o("Arrow")),
            fade($o("Text"), 0.5),
            timeout(2),
            send_signal($o("^"), "EndSnow"),
            wait(),
        }
    },
    create_arrow($o("Arrow"),$v("ThermometerPos"), orxVECTOR{-2,0,0}),
    append_object($os("TutObjs"), $o("Arrow")),
    create_in_collection($os("TutObjs"), $o("Text"), "HeaterTooHotWhenNotCold"),
    timeout(3.5),
    fade($o("Text"), 0.5),
    destroy($o("Arrow")),
};

auto first_winter_level_behavior = sequence {
    $f("TemperatureDiff") = -20,
    skippable <<= teach_temperature_behavior,
    if_($o("Sparrow")) <<= set_controller($o("Sparrow"), "SparrowLeaveController"),
    set_happiness($o("^"), 0.5),
    destroy($o("Heater"), false),
    create_tool($o("Heater"), $o("^"), "HeaterTool"), // This effectively resets the heater cooldown from the tutorial
    create($o("Text"), "FirstWinterGoalExplanation"),
    timeout(2),
    add_track($o("Text"), "FadeOutTrack"),
    create($o(""), "StartGameText", false),
    create($o("Crow"), "WeakCrow"),
    any {
        happiness_bar_mission(),
        level_countdown(),
        spawn_sparrows_randomly($o("^"), "Sparrow"),
        spawn_cheese_when_finished(),
        receive_signal_fewer_than_challenge($o("^"), $b("Challenge1"), "HeaterToolSelected", 2),
        send_birds_happy_challenge($b("Challenge2")),
        repeat <<= sequence {
            timeout(20),
            winter_event(timeout(20), $f("TemperatureDiff")),
        },
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(first_winter_level_behavior, "first_winter_level_behavior")

auto teach_crow_behavior = sequence {
    timeout(1.5),
    create_in_collection($os("TutObjs"), $o("Text"), "CrowsNoticedYou"),
    timeout(4),
    add_track($o("Text"), "FadeOutTrack"),
    timeout(0.5),
    create_in_collection($os("TutObjs"), $o("Text"), "CrowWillDiveToFoodText"),
    timeout(3),
    set_controller($o("Crow"), "DiveToPosConverger"),
    timeout(1),
    add_track($o("Text"), "FadeOutTrack"),
    timeout(0.5),
    create_in_collection($os("TutObjs"), $o("Text"), "YouCanPokeTheCrowText"),
    set_controller($o("Crow"), "WeakCrowController"),
    set_constraints($o("Crow"), "WaitForPokeConstraints"),
    wait_for_state($o("Crow"), "Stunned"),
    set_controller($o("Crow"), "WaitIdlyConverger"),
    set_constraints($o("Crow"), "WeakCrowConstraints"),
    add_track($o("Text"), "FadeOutTrack"),
    timeout(1),
    create_in_collection($os("TutObjs"), $o("Text"), "SnatchCostsHappinessText"),
    wait_for_state($o("Crow"), "Stay"),
    timeout(1),
    create_in_collection($os("TutObjs"), $o("Curtain"), "UserCurtain"),
    create($o("InstructionCheese"), "FirstEncounterInstructionCheese"),
    timeout(1),
    set_controller($o("Crow"), "FirstEncounterDealerSnatchController"),
    wait_for_state($o("Crow"), "Home"),
    timeout(1),
    add_track($o("Text"), "FadeOutTrack"),
    destroy($o("Curtain")),
};

auto first_encounter_with_crows_level_behavior = sequence {
    create($o("Crow"), "FirstEncounterInstructionCrow"),
    skippable <<= teach_crow_behavior,
    set_constraints($o("Crow"), "WeakCrowConstraints"),
    set_controller($o("Crow"), "WeakCrowController"),
    create($o("Text"), "FirstEncounterGoalExplanation"),
    timeout(2),
    set_happiness($o("^"), 0.5),
    add_track($o("Text"), "LateFadeOutTrack"),
    any {
        level_countdown(),
        happiness_bar_mission(),
        spawn_sparrows_randomly($o("^"), "Sparrow"),
        spawn_cheese_when_finished(),
        no_snatch_challenge($o("^"), $b("Challenge1")),
        unhappy_bird_left("Challenge2"),
    },
    show_end_dialog(),
};
REGISTER_BEHAVIOR(first_encounter_with_crows_level_behavior, "first_encounter_with_crows_level_behavior")
