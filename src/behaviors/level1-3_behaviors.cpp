/*
 * level_behaviors.cpp
 *
 *  Created on: Jul 25, 2015
 *      Author: eba
 */

#include "behavior_common.hpp"
#include <orxFSM/event_behaviors.hpp>

#include <pancarObject.h>
#include <gameObjects/dialogs/Board.h>
#include <Simulator.h>
#include <gameObjects/RectangularBody.h>

template <class ... ARGS>
std::string quantified_print_action(const char * format_key_key, int quantifier, ARGS ... args) {
    const char * format;
    const char * format_key = orxConfig_GetString(format_key_key);
    if(quantifier == 1) {
        format = Localize((format_key + std::string("Singular")).c_str());
    } else {
        format = Localize((format_key + std::string("Plural")).c_str());
    }
    return sprint_action(format, quantifier, std::move(args)...);
}
RETURNING_ACTION_LEAF(quantified_print, quantified_print_action)

auto cheese_demonstration = sequence {
    create_in_collection($os("TutObjs"), $o("Cursor"), "InstructionCursor"),
    create_in_collection($os("TutObjs"), $o("Curtain"), "UserCurtain"),
    timeout(2.5),
    create($o("Cheese"), "Cheese"),
    timeout(1),

    // Teach how to move
    create_in_collection($os("TutObjs"), $o("Text"), "MoveCheeseText"),
    timeout(2.5),
    get_center_of_mass($v("PickUpPoint"),$o("Cheese")),
    move($o("Cursor"), $v("PickUpPoint"),15),
    set_target_anim($o("Cursor"), "HandTouchAnim"),
    any {
        apply_force($o("^"), $o("Cursor")),
        sequence {
            move($o("Cursor"), orxVECTOR{6,5,0}, 8),
            move($o("Cursor"), orxVECTOR{-2,5,0},8),
            move($o("Cursor"), $v("PickUpPoint"),8),
            add_track($o("Text"), "FadeOutTrack"),
            timeout(0.5),
        },
    },
    set_target_anim($o("Cursor"), "HandHoverAnim"),

    // Teach how to pick
    create_in_collection($os("TutObjs"), $o("Text"), "PickCheeseText", false),
    timeout(2.5),
    calculate_two_flake_pick_up_point($v("PickUpPoint"), $o("Cheese")),
    move($o("Cursor"), $v("PickUpPoint"), 12),
    set_target_anim($o("Cursor") ,"HandTouchAnim"),
    any {
        apply_force($o("^"), $o("Cursor")),
        sequence {
            timeout(0.4),
            move($o("Cursor"), orxVECTOR{7,-1,0}, 35),
            timeout(2),
            add_track($o("Text"), "FadeOutTrack"),
        }
    },
    set_target_anim($o("Cursor") ,"HandHoverAnim"),
    timeout(1.5),
    destroy($o("Curtain")),
    destroy($o("Cursor")),
};

struct ensure_basket_behavior: behavior_t {
    GameLevel * level;
    orxVECTOR target;
    ensure_basket_behavior(GameLevel * level, orxVECTOR target): level(level), target(target) {}
    behavior_state run(const orxCLOCK_INFO & clock) {
        auto & nodes = level->world->getNodes();
        if(nodes.empty()) return SUCCEEDED;
        Node & node = *nodes.begin();
        if(node.pos.y()>target.fY) level->world->setNodePos(node,{target.fX, target.fY});
        return RUNNING;
    }
};
BEHAVIOR_LEAF(ensure_basket,ensure_basket_behavior)

auto teach_feeding_behavior = sequence {
    create_in_collection($os("TutObjs"), $o("Curtain"), "UserCurtain"),
    timeout(3),
    create_in_collection($os("TutObjs"), $o("Text"), "GetPointsByFeedingText"),
    timeout(2),
    create_in_collection($os("TutObjs"), $o("Flake"), "CheeseFlake"),
    create_in_collection($os("TutObjs"), $o("Cursor"), "InstructionCursor"),
    timeout(1),
    get_center_of_mass($v("FlakePos"),$o("Flake")),
    move($o("Cursor"), $v("FlakePos"), 15),
    set_target_anim($o("Cursor"), "HandTouchAnim"),
    any {
        keep_running() <<= apply_force($o("^"), $o("Cursor")),
        sequence {
            move($o("Cursor"), $o("Sparrow"), orxVECTOR{2.3,-2,0}, 15),
            wait_for_state($o("Sparrow"), "Chew"),
        }
    },
    set_target_anim($o("Cursor"), "HandHoverAnim"),
    add_track($o("Text"), "FadeOutTrack"),
    timeout(1),
    create_in_collection($os("TutObjs"), $o("Text"), "GetPointsByBasketText"),
    timeout(2),
    create_in_collection($os("TutObjs"), $o("Flake"), "CheeseFlake"),
    timeout(0.9),
    get_first_node_position($v("FlakePos"),$o("Flake")),
    move($o("Cursor"), $v("FlakePos"), 15),
    set_target_anim($o("Cursor"), "HandTouchAnim"),
    any {
        keep_running() <<= apply_force($o("^"), $o("Cursor")),
        sequence {
            add($v("ThrowPoint"), $o("Sparrow"), orxVECTOR{14,-9,0}),
            move($o("Cursor"), $v("ThrowPoint"), 12),
            set_controller($o("Sparrow"), "SparrowEatConverger"),
            wait_for_state($o("Sparrow"), "WaitForFood"),
            wait_for_animation($o("Sparrow"), "StayMouthOpen"),
            add($v("ReleasePoint"), $v("ThrowPoint"), orxVECTOR{-3,0,0}),
            move($o("Cursor"), $v("ReleasePoint"), 30),
        }
    },
    set_target_anim($o("Cursor"), "HandHoverAnim"),
    get_position($v("CursorPos"), $o("Cursor")),
    get_marker($v("SparrowMouth"), $o("Sparrow"), "MouthMarker"),
    set_velocity_for_target($o("^"), $v("CursorPos"),$v("SparrowMouth")),
    ensure_basket($o("^"), $v("SparrowMouth")),
    wait_for_state($o("Sparrow"), "Chew"),
    set_controller($o("Sparrow"), "SparrowController"),
    add_track($o("Text"), "FadeOutTrack"),
    timeout(1),
    destroy($o("Cursor")),
    destroy($o("Curtain")),
    timeout(0.2),
    create_in_collection($os("TutObjs"), $o("Text"), "TryFeedingYourselfText"),
    create_in_collection($os("TutObjs"), $o("Flake"), "CheeseFlake"),
    wait_for_state($o("Sparrow"), "Chew"),
    add_track($o("Text"), "FadeOutTrack"),
    timeout(1),
};

auto teach_feeding_level_behavior = sequence {
    create($o("Sparrow"), "SimpleStayAroundSparrow"),
    skippable <<= teach_feeding_behavior,
    set_controller($o("Sparrow"), "Level1SparrowController"),
    create($o("Text"), "FeedAllTheFlakesText"),
    create($o(""), "Lvl1EatCountObserver", false),
    any {
        count_eats($o("^"), $i("PiecesEaten")),
        sequence {
            for_range($i("i"), 0, $config("NumOfFlakes")) <<= sequence {
                create($o("Flake"), "CheeseFlake", false),
                timeout(0.2),
            },
            timeout(0.5),
            add_track($o("Text"), "FadeOutTrack"),
            create($o(""), "StartGameText", false),
        },
    },
    any {
        count_eats($o("^"), $i("PiecesEaten")),
        consecutive_baskets_challenge($o("^"), $b("Challenge1")),
        keep_running() <<= sequence {
            $b("Challenge2") = true,
            $f("Threshold") = $config("Challenge2TimeThreshold"),
            timeout($f("Threshold")),
            $b("Challenge2") = false,
        },
        wait_for_success <<= world_empty($o("^")),
        level_countdown(),
    },
    get_node_count($i("RemainingPieces"), $o("^")),
    if_($i("RemainingPieces")) <<= quantified_print($s("Failure"), "FailureString", $i("RemainingPieces")),
    show_end_dialog(),
};
REGISTER_BEHAVIOR(teach_feeding_level_behavior, "teach_feeding_level_behavior")

auto teach_large_cheese_level_behavior = sequence {
    create($o("Sparrow"), "SimpleStayAroundSparrow"),
    skippable <<= cheese_demonstration,
    not_($b("CheeseNotCreated"), $o("Cheese")),
    if_($b("CheeseNotCreated")) <<=  create($o("Cheese"), "Cheese"),
    create($o("Text"), "FeedTheEntireCheeseText"),
    timeout(2.5),
    add_track($o("Text"), "FadeOutTrack"),
    create($o(""), "StartGameText", false),
    set_controller($o("Sparrow"), "Level2SparrowController"),
    $i("EatThreshold") = $config("PieceFeedThreshold"),
    create($o(""), "Lvl2EatCountObserver", false),
    any {
        count_eats($o("^"), $i("PiecesEaten")),
        level_countdown(),
        sequence {
            wait_for_success <<= world_empty($o("^")),
            $b("Challenge1") = true,
        },
        keep_running() <<= sequence {
            $b("Challenge2") = true,
            $f("Threshold") = $config("Challenge2TimeThreshold"),
            timeout($f("Threshold")),
            less_than($b("NotEnoughEaten"), $i("PiecesEaten"), $i("EatThreshold")),
            not_($b("Challenge2"), $b("NotEnoughEaten")),
        },
    },
    less_than($b("NotEnoughEaten"), $i("PiecesEaten"), $i("EatThreshold")),
    if_($b("NotEnoughEaten")) <<= quantified_print($s("Failure"), "FailureString"
                                        ,$i("PiecesEaten"), $i("EatThreshold")),
    show_end_dialog(),
};
REGISTER_BEHAVIOR(teach_large_cheese_level_behavior, "teach_large_cheese_level_behavior")

auto teach_happiness = collection_scope($os("Sparrows")) <<= sequence {
    for_range($i("i"), 0, 2) <<= sequence {
        create_in_collection($os("TutObjs"), $o("Sparrow"), "TeachHappinessSparrow", false),
        append_object($os("Sparrows"), $o("Sparrow")),
    },
    timeout(2.5),
    add($v("HappinessPos"),$o("Sparrow"), orxVECTOR{1.7,-5.5,0}),
    create_arrow($o("Arrow"),$v("HappinessPos"), orxVECTOR{-1,1,0}),
    append_object($os("TutObjs"), $o("Arrow")),
    create_in_collection($os("TutObjs"), $o("Text"), "HappinessDescriptionText"),
    timeout(2.5),
    add_track($o("Text"), "FadeOutTrack"),
    add_track($o("Arrow"), "FadeOutTrack"),
    timeout(1),
    create_in_collection($os("TutObjs"), $o("Text"), "FeedToMakeHappyText"),
    for_range($i("i"), 0, $config("NumOfInitialFlakes")) <<= sequence {
        create_in_collection($os("TutObjs"), $o("Flake"), "CheeseFlake", false),
        timeout(0.2),
    },
    timeout(4),
    add_track($o("Text"), "FadeOutTrack"),
    wait_for_success <<= collection_empty($os("Sparrows")),
};

auto teach_boredom = sequence {
    create_in_collection($os("TutObjs"), $o("Sparrow"), "TeachBoredomSparrow"),
    timeout(1.5),
    add($v("HappinessPos"),$o("Sparrow"), orxVECTOR{1.7,-5.5,0}),
    create_arrow($o("Arrow"),$v("HappinessPos"), orxVECTOR{-1,1,0}),
    append_object($os("TutObjs"), $o("Arrow")),
    create_in_collection($os("TutObjs"), $o("Text"), "BirdsGetBoredText"),
    wait_for_state($o("Sparrow"), "FlyAway"),
    add_track($o("Text"), "FadeOutTrack"),
    add_track($o("Arrow"), "FadeOutTrack"),
    timeout(1.5),
};

struct count_happy_birds_behavior: event_behavior<sparrow_leave_event>{
    in_out_variable<orxU32> count;
    orxFLOAT min_happiness;
    count_happy_birds_behavior(orxOBJECT * ed, in_out_variable<orxU32> count, orxFLOAT min_happiness)
        :behavior_base(ed), count(count), min_happiness(min_happiness){}
    void handle_event(Sparrow * s){
        if(s->happiness >= min_happiness) count = orxU32(count)+1;
    }
};
BEHAVIOR_LEAF(count_happy_birds, count_happy_birds_behavior)

auto teach_happiness_boredom_level_behavior = sequence {
    skippable <<= sequence {
        teach_happiness,
        teach_boredom,
    },
    create($o("Text"), "MakeNBirdsHappyText"),
    timeout(2.5),
    create($o(""), "StartGameText", false),
    add_track($o("Text"), "FadeOutTrack"),
    $i("HappyBirds") = 0,
    create($o(""), "HappyBirdCountObserver", false),
    any {
        spawn_sparrows_randomly($o("^")),
        spawn_cheese_when_finished(),
        level_countdown(),
        count_happy_birds($o("^"), $i("HappyBirds"), $config("VeryHappyThreshold")),
        consecutive_baskets_challenge($o("^"), $b("Challenge2")),
        unhappy_bird_left("Challenge1"),
    },
    $i("Threshold") = $config("HappyBirdThreshold"),
    less_than($b("NotEnoughHappy"), $i("HappyBirds"), $i("Threshold")),
    if_($b("NotEnoughHappy")) <<= quantified_print($s("Failure"), "FailureString"
                                        ,$i("HappyBirds"), $i("Threshold")),
    show_end_dialog(),
};
REGISTER_BEHAVIOR(teach_happiness_boredom_level_behavior, "teach_happiness_boredom_level_behavior")
