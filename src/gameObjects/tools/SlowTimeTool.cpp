/*
 * SlowTimeTool.cpp
 *
 *  Created on: Sep 11, 2015
 *      Author: enis
 */

#include <orx_utilities.h>
#include "SlowTimeTool.h"
#include <gestures.h>
#include <pancarDemo.h>
#include <cmath>
#include "../screens/GameLevel.h"

const float minTimeMult = 0.33;
const float coeffMax = 0.6;
float graynessCoeff = 0;
const float coeffIncreaseRate = 0.5;
const float coeffDecreaseRate = 2.5;

void SetClockMultiplier(orxFLOAT mul) {
    auto mainClock = orxClock_FindFirst(orx2F(-1.0f), orxCLOCK_TYPE_CORE);
    orxClock_SetModifier(mainClock, orxCLOCK_MOD_TYPE_MULTIPLY, mul);
}

void SlowTimeTool::setCoeff(float new_coeff) {
    if(slownessCoeff == 0 && new_coeff != 0) {
        orxViewport_AddShader(viewport, "SlowTimeShader");
        orxViewport_RemoveShader(viewport, "PassthroughShader");
        AddSound("SlowTimeSoundObject");
    }
    if(slownessCoeff == coeffMax && new_coeff != coeffMax) {
        AddSound("SlowTimeSoundObject");
    }
    if(new_coeff == 0 && slownessCoeff!= 0) {
        orxViewport_AddShader(viewport, "PassthroughShader");
        orxViewport_RemoveShader(viewport, "SlowTimeShader");
    }
    slownessCoeff = new_coeff;
    SetClockMultiplier(minTimeMult + (1-minTimeMult)*(coeffMax-new_coeff)/coeffMax);
    graynessCoeff = new_coeff;
}

void SlowTimeTool::SetContext(ExtendedMonomorphic& context) {
    viewport = dynamic_cast<GameLevel &>(context).GetScreenViewport();
    setCoeff(0);
}

void SlowTimeTool::Update(const orxCLOCK_INFO& _rstInfo) {
    Tool::Update(_rstInfo);
    if(selected) {
        setCoeff(std::min(coeffMax, slownessCoeff + coeffIncreaseRate*_rstInfo.fDT));
    } else {
        setCoeff(std::max(0.f, slownessCoeff - coeffDecreaseRate*_rstInfo.fDT));
    }
}

orxSTATUS orxFASTCALL slowdownCoeffParamHandler(const orxEVENT *_pstEvent) {
    if(_pstEvent->eID == orxSHADER_EVENT_SET_PARAM) {
        auto payload = (orxSHADER_EVENT_PAYLOAD *)_pstEvent->pstPayload;
        if(strcmp(payload->zParamName, "slowdownCoeff") == 0) {
            payload->fValue = graynessCoeff;
        }
    }
    return orxSTATUS_SUCCESS;
}

void SlowTimeTool::OnDelete() {
    SetClockMultiplier(1);
}

void SlowTimeTool::OnPause(bool pause) {
    if(pause) {
        graynessCoeff = 0;
        SetClockMultiplier(1);
    }
}
