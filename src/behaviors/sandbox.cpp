/*
 * level7-9_behaviors.cpp
 *
 *  Created on: Oct 26, 2015
 *      Author: eba
 */

#include <behaviors/behavior_common.hpp>
#include <orxFSM/orxFSM_header.h>

auto sandbox_level_behavior = sequence {
    any {
        repeat <<= sequence {
            wait_for_free_button_click($o("^"), "CallDealerButton"),
            create($o("Dealer"), "WeakCrow", false),
            wait_for_free_button_click($o("^"), "SendDealerButton"),
            set_controller($o("Dealer"), "CrowExitConverger"),
            wait_for_deletion($o("Dealer")),
        },
        repeat <<= sequence {
            wait_for_free_button_click($o("^"), "CallFatherButton"),
            create($o("Father"), "SBCrowFather", false),
            any {
                repeat <<= sequence {
                    $b("CanLeave") = true,
                    wait_for_free_button_click($o("^"), "CallArmyButton"),
                    $b("CanLeave") = false,
                    murder_attack(),
                },
                sequence {
                    wait_for_free_button_click($o("^"), "SendFatherButton"),
                    wait_for_true($b("CanLeave")),
                },
            },
            set_controller($o("Father"), "CrowExitConverger"),
            wait_for_deletion($o("Father")),
        },
        repeat <<= sequence {
            wait_for_free_button_click($o("^"), "CallSparrowButton"),
            create($o(""),"Sparrow", false),
        },
        repeat <<= sequence {
            wait_for_success(world_empty($o("^"))),
            wait_for_free_button_click($o("^"), "CallCheeseButton"),
            create($o("Cheese"), "Cheese"),
        },
        collection_scope($os("Crows")) <<= any {
            repeat <<= sequence {
                wait_for_free_button_click($o("^"), "CallHenchmanButton"),
                create($o("Crow"), "Crow", false),
                append_object($os("Crows"), $o("Crow")),
            },
            repeat <<= sequence {
                any {
                    wait_for_success <<= collection_empty($os("Crows")),
                    repeat <<= sequence {
                        wait_for_free_button_click($o("^"), "SendHenchmanButton"),
                        get_first($o("Crow"),$os("Crows")),
                        set_controller($o("Crow"), "CrowExitConverger"),
                        remove_object($os("Crows"), $o("Crow")),
                    }
                },
                tick,
            },
        },
        repeat <<= sequence {
            wait_for_free_button_click($o("^"), "CallCheeseRainButton"),
            cheese_rain(),
        },
        repeat <<= sequence {
            wait_for_free_button_click($o("^"), "CallWinterButton"),
            $f("WinterLength") = $config("WinterLength"),
            $f("WinterTemperatureDiff") = $config("WinterTemperatureDiff"),
            winter_event(timeout($f("WinterLength")), $f("WinterTemperatureDiff")),
        }
    }
};
REGISTER_BEHAVIOR(sandbox_level_behavior, "sandbox_level_behavior")

