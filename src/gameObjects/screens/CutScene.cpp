/*
 * CutScene.cpp
 *
 *  Created on: Nov 8, 2015
 *      Author: eba
 */

#include "CutScene.h"
#include <game_state.h>
#include <analytics.h>

void CutScene::EndCutscene() {
    design_event({"Watch", "CutScene", GetModelName()});
    ConfigSectionGuard guard(GetModelName());
    if(!orxConfig_HasValue("NextLevel")) {
        SetNextScreen(orxConfig_GetString("MenuScreen"));
    } else {
        auto next_level = orxConfig_GetString("NextLevel");
        if(is_level_playable(next_level)) SetNextScreen(next_level);
        else SetNextScreen(orxConfig_GetString("MenuScreen"));
    }
}

void CutScene::Update(const orxCLOCK_INFO& ci) {
    if(orxInput_IsActive("RestartLevel")) SetNextScreen(GetModelName());
    if(orxInput_IsActive("QuitLevel")) SetNextScreen("OpeningMenuScreen");
}

orxOBJECT * CutScene::NewSlide(const char* slide, orxFLOAT lifetime) {
    if(curSlide) {
        orxVECTOR curPos;
        orxObject_GetPosition(curSlide, &curPos);
        curPos.fZ += lifetime ? 0.1 : 0;
        orxObject_SetPosition(curSlide, &curPos);
        orxObject_SetLifeTime(curSlide, lifetime);
    }
    curSlide = CreateChild(slide);
    return curSlide;
}

void CutScene::ExtOnCreate() {
    SimpleScreen::ExtOnCreate();
    CreateChild("SkipCutSceneButton", true);
    slideViewport = orxViewport_CreateFromConfig("SlideViewport");
}

void CutScene::OnDelete() {
    orxViewport_Delete(slideViewport);
}
