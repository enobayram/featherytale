/*
 * ScoreMultiplierTool.cpp
 *
 *  Created on: Sep 22, 2015
 *      Author: eba
 */

#include <util/orx_pointers.hpp>
#include <util/random.hpp>
#include <util/event_utils.hpp>
#include "Tool.h"

#include <gameObjects/screens/GameLevel.h>
#include <gameObjects/HappinessSidebar.h>
#include <gameObjects/Score.h>

class ScoreMultiplierTool: public Tool {
    weak_object_ptr level;
public:
    SCROLL_BIND_CLASS("ScoreMultiplierTool")
    void SetSelected(bool selected) {
        Tool::SetSelected(selected);
        orxOBJECT * levelobj = level;
        if(!levelobj) return;
        GameLevel * glevel = scroll_cast<GameLevel>(levelobj);
        if(selected) {
            glevel->setScoreMultiplier(2);
        } else {
            glevel->setScoreMultiplier(1);
        }
    }
    void SetContext(ExtendedMonomorphic & context) {
        level = context.GetOrxObject();
    }
};

class BoostHappinessTool: public Tool {
    weak_object_ptr level;
    SCROLL_BIND_CLASS("BoostHappinessTool")
    orxFLOAT bump_period;
public:
    BoostHappinessTool() {
        bump_period = orxConfig_GetFloat("BumpPeriod");
    }
    void SetContext(ExtendedMonomorphic & context) {
        level = context.GetOrxObject();
    }
    virtual void Update(const orxCLOCK_INFO &_rstInfo) {
        Tool::Update(_rstInfo);
        if(selected && poisson_event_generator(_rstInfo.fDT)(bump_period)) {
            auto level = scroll_cast<GameLevel>(this->level);
            if(!level) return;
            auto particle = level->happinessBar->indicator->CreateChild("HappinessBoostParticle",true);
            orxObject_Detach(particle);
            level->changeHappiness(GetValue<orxFLOAT>("BumpAmount"));
        }
    }
};

class HeaterTool: public Tool {
    weak_object_ptr level;
    event_listener_guard<temperature_status> temperature_listener;
    SCROLL_BIND_CLASS("HeaterTool")
public:
    HeaterTool() {
    }
    void SetContext(ExtendedMonomorphic & context) {
        level = context.GetOrxObject();
    }
    void SetSelected(bool selected) {
        Tool::SetSelected(selected);
        auto level = scroll_cast<GameLevel>(this->level);
        if(!level) return;
        if(selected) {
            auto heating_amount = GetValue<orxFLOAT>("HeatingAmount");
            temperature_listener.register_handler(*level, get_weak_accessor<event_dispatcher>(level->GetOrxObject()),
                [=](temperature_status::location loc) {
                if(loc==temperature_status::ONFRAME) return heating_amount;
                else return 0.0f;
            });
        } else {
            temperature_listener.attempt_deregister();
        }
    }
};
