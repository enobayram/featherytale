/*
 * CountdownTimer.cpp
 *
 *  Created on: Aug 10, 2015
 *      Author: enis
 */

#include <gameObjects/CountdownTimer.h>
#include <orx_utilities.h>

void CountdownTimer::ExtOnCreate() {
    text = FindOwnedChild("CountdownTimerText");
    period = GetValue<orxFLOAT>("StartAt",false);
    running = true; //GetValue("Running", false);
    remaining = period;
    endTickingSoundLength = orxConfig_GetFloat("EndTickingDuration");
    update_display();
}

void CountdownTimer::update_display() {
    orxObject_SetTextString(text, to_string(int(remaining)).c_str());
    SetAlpha(GetOrxObject(), remaining/period);
}

void CountdownTimer::Update(const orxCLOCK_INFO& clock) {
    if(running && remaining > 0) {
        auto prev_remaining = remaining;
        remaining = std::max(0.0f, remaining - clock.fDT);
        update_display();
        if(prev_remaining > endTickingSoundLength &&
           remaining <= endTickingSoundLength &&
           orxConfig_HasValue("EndTickingSound")) {
            AddSound(orxConfig_GetString("EndTickingSound"));
        }
    }
}
