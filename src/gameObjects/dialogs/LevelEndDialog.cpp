/*
 * LevelEndDialog.cpp
 *
 *  Created on: Aug 11, 2015
 *      Author: enis
 */

#include <sstream>

#include <orx_utilities.h>
#include <game_state.h>

#include <behaviors/behavior_common.hpp>

#include <analytics.h>

#include "LevelEndDialog.h"
#include <gameObjects/Button.h>
#include "../screens/GameLevel.h"
#include <gameObjects/Score.h>
#include <gameObjects/SmartText.h>

#include <utility>

using namespace std;

void CreateNextLevelButton(LevelEndDialog * dialog, GameLevel * level) {
    orxFLOAT next_level_button_x = dialog->GetValue<orxFLOAT>("NextLevelButtonX");
    ConfigSectionGuard level_guard(level->GetModelName());
    if(orxConfig_HasValue("NextLevel")) {
        auto next_level_name = orxConfig_GetString("NextLevel");
        if(!is_level_dependent_completed(next_level_name)) {
            auto disabled_icon = orxObject_CreateFromConfig("NextButtonDisabled");
            dialog->AddButton(disabled_icon, next_level_button_x);
        } else {
            auto next_level_icon = Create<Button>("NextLevelIcon");
            dialog->AddButton(next_level_icon->GetOrxObject(), next_level_button_x);

            if(is_level_playable(next_level_name)) {
                next_level_icon->set_handler([level, next_level_name]{
                    level->SetNextScreen(next_level_name);
                });
            } else {
                next_level_icon->set_disabled(true);
                auto minhearts = GetValue<orxU32>(next_level_name, "MinHearts");
                auto totalhearts = getTotalHearts();
                auto text = next_level_icon->CreateChild("HeartsRemainingText", true);
                std::stringstream ss; ss << totalhearts << "/" << minhearts;
                orxObject_SetTextString(text, ss.str().c_str());
            }
        }
    }
}


void CreateFacebookButton(LevelEndDialog * dialog, bool disabled) {
    orxFLOAT facebook_button_x = dialog->GetValue<orxFLOAT>("FacebookButtonX");
    auto facebook_icon = Create<Button>("FacebookButton");
    dialog->AddButton(facebook_icon->GetOrxObject(), facebook_button_x);
    facebook_icon->set_disabled(disabled);
}

void get_failure_reason_action(in_out_variable<bool> failed, in_out_variable<std::string> reason, GameLevel * level) {
    if(level->get("Failure").eType != orxCOMMAND_VAR_TYPE_NONE) {
        failed = true;
        reason = level->get<std::string>("Failure");
    } else {
        auto pass_score = GetValue<orxS32>(level->GetModelName(), "Thresholds", 0);
        if(level->getScore() < pass_score) {
            failed = true;
            ConfigSectionGuard guard(level->GetModelName());
            auto format = LocaleAwareString("ScoreFailureExplanation");
            reason = sprint_action(format, pass_score);
        } else failed = false;
    }
}
ACTION_LEAF(get_failure_reason, get_failure_reason_action)

ACTION_LEAF(create_challenge_trophy, &LevelEndDialog::createChallengeTrophy)
ACTION_LEAF(create_success_layout, &LevelEndDialog::createSuccessLayout)
ACTION_LEAF(create_failure_layout, &LevelEndDialog::createFailureLayout)
ACTION_LEAF(create_next_level_button, CreateNextLevelButton)
ACTION_LEAF(create_facebook_button, CreateFacebookButton)

orxOBJECT * get_dialog_score_display_action(LevelEndDialog * dialog) {return dialog->scoreDisplay->GetOrxObject();}
RETURNING_ACTION_LEAF(get_dialog_score_display, get_dialog_score_display_action)

behavior_constructor dialog_behavior = sequence {
    get_owner($o("Level"), $o("^")),
    get_score($f("LevelScore"), $o("Level")),
    get_failure_reason($b("NotPassed"), $s("FailureReason"), $o("Level")),
    not_($b("Passed"), $b("NotPassed")),
    if_($b("Passed")) <<= sequence {
        create_success_layout($o("^"), $o("Level")),
        create_next_level_button($o("^"), $o("Level")),
        get_dialog_score_display($o("DialogScoreDisplay"), $o("^")),
        get_score_display($o("LevelScoreDisplay"), $o("Level")),
        get_position($v("LevelScoreDispPos"), $o("LevelScoreDisplay")),
        fill_score($o("DialogScoreDisplay"), $v("LevelScoreDispPos"), $f("LevelScore")),
        timeout(2),
        capture($b("")) <<= get($b("Ch1Achieved"),$o("Level"), "Challenge1"),
        if_($b("Ch1Achieved")) <<= sequence {
            create_challenge_trophy($o("^"), LevelEndDialog::NEW, 0),
            timeout(0.5),
        },
        capture($b("")) <<= get($b("Ch2Achieved"),$o("Level"), "Challenge2"),
        if_($b("Ch2Achieved")) <<= sequence {
            create_challenge_trophy($o("^"), LevelEndDialog::NEW, 1),
            timeout(0.5),
        },
        if_($b("GoldenHeartEarned")) <<= create($o(""), "GoldenHeartEarnedDisplay", false),
    },
    if_($b("NotPassed")) <<= sequence {
        create_failure_layout($o("^"),$o("Level"), $s("FailureReason")),
        create_next_level_button($o("^"), $o("Level")),
        create_facebook_button($o("^"), true)
    },
};

void LevelEndDialog::Update(const orxCLOCK_INFO & clock) {
    MIXIN::Update(clock);
    if(successLayout) {
        orxFLOAT diff = score_target - displayed_score;
        orxFLOAT newDisplayedScore = min(score_target, displayed_score+(diff+0.1f)*clock.fDT*13);
        UpdateFillers(newDisplayedScore);
    }
}

void LevelEndDialog::SetContext(ExtendedMonomorphic& context) {
    if(orxConfig_HasValue("Thresholds")) {
        for(int i=0; i<3; i++) thresholds[i] = orxConfig_GetListU32("Thresholds", i);
    }

    set_behavior(dialog_behavior);
}

template <int N>
pair<int, float> find_stage(int (&thresholds)[N], orxFLOAT val) {
    if(val < thresholds[0]) return {0, val/thresholds[0]};
    for(int i=1; i<N; i++) {
        if(val < thresholds[i]) return {i, (val-thresholds[i-1])/(thresholds[i]-thresholds[i-1])};
    }
    return {N,0};
}

orxOBJECT* LevelEndDialog::createFiller(int index) {
    auto result = CreateChild("LevelEndFillerContent", true);
    orxVECTOR fillerPos {0, firstFramePos.fY + index * GetValue<float>("FrameYGap"), -0.02};
    orxObject_SetPosition(result, &fillerPos);
    return result;
}

void LevelEndDialog::createTrophy(TrophyType type, int index) {
    auto ypos = GetValue<orxFLOAT>("TrophyYPos");
    orxVECTOR pos {orxFLOAT((index-1) * 80), ypos, -0.1};
    newTrophy(trophies[index], type, pos);
}

void LevelEndDialog::createChallengeTrophy(TrophyType type, int index) {
    auto ypos = GetValue<orxFLOAT>("ChallengeYPos");
    orxVECTOR pos = {(index-0.5f)*80, ypos, -0.1f};
    newTrophy(challenge_trophies[index], type, pos);
}

void LevelEndDialog::newTrophy(orxOBJECT*& storage, TrophyType type, orxVECTOR pos) {
    ConfigSectionGuard guard(GetModelName());
    const char * empty_trophy_section = orxConfig_GetString("TrophyEmpty");

    const char * section = type == EMPTY ? empty_trophy_section              :
                           type == OLD   ? orxConfig_GetString("TrophyPast") :
                           /*type == NEW*/ orxConfig_GetString("TrophyFull") ;
    if(storage) {
        bool was_empty = orxObject_GetName(storage) == empty_trophy_section;
        if(was_empty && (type == NEW)) {
            auto coin_display = CreateChild("CoinsEarnedFromHeartDisplay");
            orxObject_SetPosition(coin_display, &pos);
            auto level = get<orxOBJECT *>("Level");
            auto coins_earned = GetValue<const char *>(orxObject_GetName(level), "CoinsPerHeart");
            orxObject_SetTextString(coin_display, "10");
        }
        orxObject_SetLifeTime(storage, 0);
    }
    storage = CreateChild(section, true);
    orxObject_SetPosition(storage, &pos);
}

void LevelEndDialog::UpdateFillers(orxFLOAT new_score) {
    auto new_stage = find_stage(thresholds, new_score);
    auto old_stage = find_stage(thresholds, displayed_score);
    if(new_stage.first != old_stage.first) {
        SetAlpha(curFiller, 1);
        curFiller = createFiller(new_stage.first);
        createTrophy(NEW, old_stage.first);
    }
    SetAlpha(curFiller, max(new_stage.second, 0.0001f));
    displayed_score = new_score;
}

void LevelEndDialog::AddButton(orxOBJECT* button, orxFLOAT xPos) {
    orxObject_SetOwner(button, GetOrxObject());
    orxObject_SetParent(button, GetOrxObject());
    orxObject_SetGroupID(button, GetGroupID());
    orxVECTOR pos{xPos, GetValue<orxFLOAT>("ButtonRowY"), -0.1};
    orxObject_SetPosition(button, &pos);
}

void LevelEndDialog::RemoveButton(orxOBJECT* button) {
    orxObject_SetLifeTime(button, 0);
}

bool hasFullHearts(const level_data & data) {
    for(bool heart: data.score_levels) if(!heart) return false;
    for(bool heart: data.challenges) if(!heart) return false;
    return true;
}

void LevelEndDialog::createSuccessLayout(GameLevel * level) {
    auto level_score = level->getScore();

    bool level_completed = level_score>= thresholds[0];

    if(level_completed) AddSound("EndGameCongratulationsSound");
    progression_event(level_completed ? P_COMPLETE : P_FAIL, level->GetModelName(), level_score);

    auto guard = ConfigSectionGuard(GetModelName());
    auto title_section = orxConfig_GetString(level_completed ? "SuccessText" : "FailureText");
    CreateChild(title_section, true);
    CreateChild(orxConfig_GetString("ChallengesButton"));
    level_data history = fetchLevelData(level->GetModelName());

    for(int i=0; i<3; i++) createTrophy(history.score_levels[i] ? OLD : EMPTY,i);

    for(int i=0; i<2; i++) createChallengeTrophy(history.challenges[i] ? OLD : EMPTY , i);

    for(int i=0; i<3; i++) {
        auto frame = CreateChild("LevelEndFillerFrame",true);
        orxVECTOR framepos = ::GetPosition(frame);
        framepos.fY += i*GetValue<orxFLOAT>("FrameYGap",false);
        orxObject_SetPosition(frame,&framepos);
        if(i==0) {
            frameSize = ::GetSize(frame);
            firstFramePos = ::GetPosition(frame);
        }
    }
    auto coinsPerHeart = level->GetValue<orxU32>("CoinsPerHeart");

    auto earnHeart = [&](bool & storage, const char * trophy_name = nullptr) {
        if(!storage) {
            earnResource("Coin", coinsPerHeart, "NewHeartWon", level->GetModelName());
            if(trophy_name) design_event({"TrophyEarned", level->GetModelName(), trophy_name});
        }
        storage = true;
    };

    bool hadFullHearts = hasFullHearts(history);

    for(int i=0; i<3; i++) {
        if(level_score >= thresholds[i]) earnHeart(history.score_levels[i]);
    }
    if(level->get<bool>("Challenge1")) earnHeart(history.challenges[0], "Challenge1");
    if(level->get<bool>("Challenge2")) earnHeart(history.challenges[1], "Challenge2");

    bool nowFullHearts = hasFullHearts(history);

    if(!hadFullHearts && nowFullHearts) {
        earnResource("GoldenHeart", 1, "NewHeartWon", level->GetModelName());
        set<bool>("GoldenHeartEarned", true);
    }

    putLevelData(level->GetModelName(), history);
    curFiller = createFiller(0);
    scoreDisplay = CreateChild<ScoreDisplay>(orxConfig_GetString("ScoreDisplay"), true);
    scoreDisplay->register_handler<score_arrived>([this](int increment){
        score_target += increment;
    });
    successLayout = true;
}

void LevelEndDialog::createFailureLayout(GameLevel * level, std::string failure_reason) {
    auto guard = ConfigSectionGuard(GetModelName());

    progression_event(P_FAIL, level->GetModelName(), level->getScore());

    CreateChild(orxConfig_GetString("FailureText"), true);
    auto reason_object = CreateChild<SmartText>("FailureReasonText", true);
    reason_object->SetText(failure_reason.c_str());

    CreateChild(orxConfig_GetString("HintTitle"), true);
    auto hint_object = CreateChild<SmartText>("HintText", true);
    hint_object->SetText(Localize(level->GetValue<const char *>("HintString")));
}
