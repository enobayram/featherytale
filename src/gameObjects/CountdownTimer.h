/*
 * CountdownTimer.h
 *
 *  Created on: Aug 10, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_COUNTDOWNTIMER_H_
#define GAMEOBJECTS_COUNTDOWNTIMER_H_

#include <scroll_ext/ExtendedObject.h>

#include <orxFSM/behavior_header.hpp>
#include <util/orx_pointers.hpp>

class CountdownTimer: public ExtendedMonomorphic {
    orxOBJECT * text;
    orxFLOAT period;
    orxFLOAT remaining;
    void update_display();
    orxBOOL running;
    orxFLOAT endTickingSoundLength;
public:
    SCROLL_BIND_CLASS("CountdownTimer")
    void ExtOnCreate();
    void Update(const orxCLOCK_INFO &);
    void SetRunning(bool running) {this->running = running;}
    bool IsFinished() {return remaining == 0;}
};
ACTION_LEAF(set_running, &CountdownTimer::SetRunning)
ACTION_LEAF(is_timer_finished, &CountdownTimer::IsFinished)

struct countdown_behavior: behavior_t {
    weak_object_ptr timer_ptr;
    countdown_behavior(ExtendedMonomorphic * context, std::string section):
        timer_ptr(context->CreateChild(section.c_str())){}
    behavior_state run(const orxCLOCK_INFO &) {
        if(auto timer = ExtractExtendedObject<CountdownTimer>(timer_ptr)) {
            return timer->IsFinished() ? SUCCEEDED : RUNNING;
        } else return FAILED;
    }
    ~countdown_behavior() {
        if(auto timer = ExtractExtendedObject(timer_ptr)) timer->SetLifeTime(0);
    }
};
BEHAVIOR_LEAF(countdown, countdown_behavior)

#endif /* GAMEOBJECTS_COUNTDOWNTIMER_H_ */
