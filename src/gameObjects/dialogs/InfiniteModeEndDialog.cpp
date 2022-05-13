/*
 * InfiniteModeEndDialog.cpp
 *
 *  Created on: Jan 18, 2016
 *      Author: eba
 */

#include <analytics.h>
#include <behaviors/behavior_common.hpp>
#include <gameObjects/screens/GameLevel.h>
#include "gameObjects/aspects.hpp"
#include "InfiniteModeEndDialog.h"
#include <game_state.h>

void InfiniteModeEndDialog::AddButton(orxOBJECT* button, orxFLOAT xPos) {
    orxObject_SetOwner(button, GetOrxObject());
    orxObject_SetParent(button, GetOrxObject());
    orxObject_SetGroupID(button, GetGroupID());
    orxVECTOR pos{xPos, GetValue<orxFLOAT>("ButtonRowY"), -0.1};
    orxObject_SetPosition(button, &pos);
}

void InfiniteModeEndDialog::RemoveButton(orxOBJECT* button) {
    orxObject_SetLifeTime(button, 0);
}

void updateCoinDisplay(orxOBJECT * disp) {
    orxObject_SetTextString(disp, to_string(getInventory("Coin")).c_str());
}

void InfiniteModeEndDialog::DisplayBonusCount(int count, const char * bonus_name) {
    if(displayed_bonuses == 0) {
        coins_owned_display = CreateChild("CoinsOwnedDisplay", true);
    }
    auto graphic_name = ::GetValue<const char *>(bonus_name, "GraphicObject");
    auto graphicObject = CreateChild(graphic_name, true);
    auto pos = GetValue<orxVECTOR>("BonusesTop");
    auto bpr = GetValue<orxU32>("BonusesPerRow");
    auto gap = GetValue<orxFLOAT>("BonusesGap");
    pos.fX += gap * (displayed_bonuses%bpr - (bpr-1)/2.0);
    pos.fY += gap * (displayed_bonuses/bpr);
    orxObject_SetPosition(graphicObject, &pos);

    auto count_text = CreateChild("BonusCountText");
    orxObject_SetTextString(count_text, to_string(count).c_str());
    pos.fY += GetValue<orxFLOAT>("BonusesCountTextY");
    orxObject_SetPosition(count_text, &pos);

    orxU32 coins_earned = count * ::GetValue<orxFLOAT>(bonus_name, "CoinCoefficient");

    auto coin_text = CreateChild("BonusCoinDisplay");
    orxObject_SetTextString(coin_text, to_string(coins_earned).c_str());
    pos.fY += GetValue<orxFLOAT>("BonusesCoinTextY");
    orxObject_SetPosition(coin_text, &pos);
    earnResource("Coin", coins_earned, "EarnInGame", bonus_name);
    updateCoinDisplay(coins_owned_display);

    displayed_bonuses++;
}
ACTION_LEAF(display_bonus_count, &InfiniteModeEndDialog::DisplayBonusCount)

int get_current_record_action() {
    return fetchInfiniteModeData().record;
}
RETURNING_ACTION_LEAF(get_current_record, get_current_record_action)

orxOBJECT * ShowRecordDialog(orxOBJECT * endDialog, int levelScore) {
    auto dialog = CreateChild(endDialog, "NewRecordDialog", true);
    auto record_text = FindOwnedChild(dialog, "NewRecordText");
    orxObject_SetTextString(record_text, to_string(levelScore).c_str());
    auto data = fetchInfiniteModeData();
    data.record = levelScore;
    putInfiniteModeData(data);
    return dialog;
}
RETURNING_ACTION_LEAF(show_record_dialog, ShowRecordDialog)

behavior_constructor show_bonus_item(const char * count_name, const char * bonus_name) {
    return sequence {
        get($i("Count"), $o("Level"), count_name, 0),
        if_($i("Count")) <<= sequence {
            timeout(1),
            display_bonus_count($o("^"), $i("Count"), bonus_name),
        }
    };
}

auto show_new_record_dialog = sequence {
    timeout(1),
    show_record_dialog($o("NewRecordDialog"), $o("^"), $f("LevelScore")),
    any {
        sequence {
            wait_for_free_button_click($o("NewRecordDialog"), "NewRecordFacebookIcon"),
            share_on_facebook($o("Level")),
        },
        wait_for_button_click($o("NewRecordDialog"), "NewRecordOKButton"),
    },
    destroy($o("NewRecordDialog")),
};

void progression_event_action(progression_event_type type, const char * level, int score) {
    progression_event(type, level, score);
}
ACTION_LEAF(progression_event, progression_event_action)

auto infinite_mode_end_dialog_behavior = sequence {
    get_owner($o("Level"), $o("^")),
    get_score($f("LevelScore"), $o("Level")),
    get_name($cp("LevelName"), $o("Level")),
    progression_event(P_COMPLETE, $cp("LevelName"), $f("LevelScore")),
    get_score_display($o("LevelScoreDisplay"), $o("Level")),
    get_position($v("LevelScoreDispPos"), $o("LevelScoreDisplay")),
    create($o("DialogScoreDisplay"), $config("ScoreDisplay")),
    fill_score($o("DialogScoreDisplay"), $v("LevelScoreDispPos"), $f("LevelScore")),
    get_current_record($i("CurrentRecord")),
    less_than($b("NewRecord"), $i("CurrentRecord"), $f("LevelScore")),
    if_($b("NewRecord")) <<= show_new_record_dialog,
    show_bonus_item("EatCount", "SparrowEatBonus"),
    show_bonus_item("DealerBeatCount", "DealerBeatenBonus"),
    show_bonus_item("HenchmanBeatCount", "HenchmanBeatenBonus"),
    show_bonus_item("FatherBeatCount", "FatherBeatenBonus"),
    $f("FacebookButtonX") = $config("FacebookButtonX"),
    any {
        sequence {
            wait_for_button_click($o("^"), "RestartIcon", $config<orxFLOAT>("RestartIconX")),
            set_next_screen($o("Level"), "InfiniteRun"),
        },
        sequence {
            wait_for_button_click($o("^"), "MenuIcon", $config<orxFLOAT>("MenuIconX")),
            set_next_screen($o("Level"), "OpeningMenuScreen"),
        },
        show_facebook_share_button($o("Level"), $o("^"), $f("FacebookButtonX")),
    }
};
REGISTER_BEHAVIOR(infinite_mode_end_dialog_behavior, "InfiniteModeEndDialogBehavior")
