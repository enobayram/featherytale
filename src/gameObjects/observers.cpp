/*
 * observers.cpp
 *
 *  Created on: Dec 23, 2015
 *      Author: eba
 */

#include <scroll_ext/ExtendedObject.h>
#include <orx_utilities.h>
#include <orxFSM/behavior.hpp>
#include <orxFSM/behavior_combinators.hpp>
#include <orxFSM/simple_behaviors.hpp>
#include <orxFSM/event_behaviors.hpp>
#include <gameObjects/screens/GameLevel.h>
#include <gameObjects/HappinessSidebar.h>
#include <util/event_utils.hpp>

class BehaviorObserver: public behavior_context_mixin<BehaviorObserver> {
    SCROLL_BIND_CLASS("BehaviorObserver")
public:
    void SetContext(ExtendedMonomorphic & context) {
        set("level", context.GetOrxObject());
    }
};

struct wait_for_max_happiness_behavior: event_behavior<happiness_change_event> {
    using behavior_base::behavior_base;
    void handle_event(orxFLOAT new_happiness) {
        if(new_happiness == 1) returned_state = SUCCEEDED;
    }
};
BEHAVIOR_LEAF(wait_for_max_happiness, wait_for_max_happiness_behavior)

orxVECTOR getHappinessIndicatorPosition(GameLevel * level) {
    return level->happinessBar->indicator->GetPosition(true);
}
RETURNING_ACTION_LEAF(get_happiness_indicator_pos, getHappinessIndicatorPosition)

auto happiness_to_points_behavior = repeat <<= sequence {
    wait_for_max_happiness($o("level")),
    for_range($i("i"), 0, $config("Steps")) <<= sequence {
        change_happiness($o("level"), $config("HappinessPerStep")),
        get_happiness_indicator_pos($v("pos"), $o("level")),
        add_score($o("level"), "Happiness", $config("ScorePerStep"), $v("pos")),
        $f("pause") = $config("PausePerStep"),
        timeout($f("pause")),
    },
};
REGISTER_BEHAVIOR(happiness_to_points_behavior, "happiness_to_points_behavior")

class TemperatureObserver: public ExtendedMonomorphic {
    SCROLL_BIND_CLASS("TemperatureObserver")
    weak_scroll_ptr<GameLevel> level;
    orxOBJECT * thermometer;
    orxFLOAT nominal_temp, nominal_range;
public:
    void SetContext(ExtendedMonomorphic & context) {
        level = dynamic_cast<GameLevel *>(&context);
        UpdateThermometer();
    }
    void ExtOnCreate() {
        thermometer = CreateChild("Thermometer");
        nominal_temp = orxConfig_GetFloat("NominalTemperature");
        nominal_range = orxConfig_GetFloat("NominalRange");
    }
    void UpdateThermometer() {
        auto level = this->level.get();
        if(!level) return;
        auto temperature = level->query<temperature_status>(temperature_status::ONFRAME);
        orxFLOAT colorH;
        orxFLOAT deltaT = temperature-nominal_temp;
        if(deltaT < -nominal_range) colorH = 0.5 + (nominal_range-deltaT) / 200;
        else if(deltaT > nominal_range) colorH = 0.08 - (deltaT-nominal_range) / 200;
        else colorH = 0.33 - 0.08 * deltaT/nominal_range;

        colorH = std::max(.0f, std::min(colorH, 0.66f));
        SetHSV(thermometer, {colorH,1,1});
    }
    void Update(const orxCLOCK_INFO & clock) {
        UpdateThermometer();
    }
};

class CounterObserver: public ExtendedMonomorphic {
    SCROLL_BIND_CLASS("CounterObserver")
    GameLevel * level;
    orxOBJECT * text;
    const char * counter;
    orxS32 expected = -1;
    void SetContext(ExtendedMonomorphic & context) {
        level = dynamic_cast<GameLevel *>(&context);
        ConfigSectionGuard guard(GetModelName());
        text = CreateChild(orxConfig_GetString("CountText"), true);
        counter = orxConfig_GetString("Counter");
        if(orxConfig_HasValue("Expected")) expected = orxConfig_GetU32("Expected");
    }
    void Update(const orxCLOCK_INFO &) {
        unsigned int count = level->get<orxU32>(counter);
        char buffer[33];
        if(expected >= 0) snprintf(buffer, sizeof(buffer), "%u/%u", count, (unsigned int)expected);
        else snprintf(buffer, sizeof(buffer), "%u", count);
        orxObject_SetTextString(text, buffer);
    }
};
