/*
 * SparrowIndicator.cpp
 *
 *  Created on: May 16, 2015
 *      Author: eba
 */
#include <cmath>

#include <orx_utilities.h>
#include <gameObjects/HappinessIndicator.h>

using namespace std;

void HappinessIndicator::setHappiness(double happiness) {
    happiness = max(0.0,min(1.0,happiness));
    SetHSV(GetOrxObject(), {orxFLOAT(happiness/3),1,1});

    orxConfig_PushSection("SparrowIndicator");
    vector<string> happinessLevels = getConfigList("AnimationList");
    orxConfig_PopSection();
    size_t idx = happiness * happinessLevels.size();
    idx = min(idx, happinessLevels.size()-1);
    SetAnim(happinessLevels[idx].c_str(), true);

}

void HappinessIndicator::setPhase(double phase) {
    orxCOLOR color;
    GetColor(color);
    orxColor_SetAlpha(&color, 1-phase);
    SetColor(color);
}

void HappinessIndicator::ExtOnCreate() {
    setHappiness(orxConfig_GetFloat("InitHappiness"));
    setPhase(orxConfig_GetFloat("InitPhase"));
}
