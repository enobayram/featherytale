/*
 * KnifeTool.cpp
 *
 *  Created on: Aug 6, 2015
 *      Author: enis
 */

#include <iostream>
#include <cmath>

#include <gestures.h>

#include <orx_utilities.h>

#include "KnifeTool.h"
#include <gameObjects/Button.h>
#include <gameObjects/screens/GameLevel.h>
#include <gameObjects/TrailEffect.h>

struct point {
    double x,y;
};

struct line_segment {
    point p1, p2;
};

$generator(segments) {
    const trail & t;
    double tb, te;
    decltype(t.traces.begin()) it_new;
    decltype(t.traces.begin()) it_old;
    segments(const trail & t, double tb, double te):t(t),tb(tb),te(te){}
    $emit(line_segment) {
        if(t.traces.size()>=2) {
            it_old=t.traces.begin();
            it_new=it_old+1;
            do {
                if(it_new->t>tb && it_new->t<te) {
                    $yield(line_segment{{it_old->x, it_old->y},{it_new->x, it_new->y}});
                }
                it_old = it_new; it_new++;
            } while(it_new!=t.traces.end());
        }
    }$stop
};

double cross(const line_segment & seg, const point & p) {
    return (p.x - seg.p1.x) * (seg.p2.y-seg.p1.y) - (p.y-seg.p1.y) * (seg.p2.x - seg.p1.x);
}

bool intersects(const line_segment & seg1, const line_segment & seg2) {
    return (cross(seg1, seg2.p1) * cross(seg1, seg2.p2) < 0) &&
           (cross(seg2, seg1.p1) * cross(seg2,seg1.p2) < 0);
}

point toPoint(const Node & node) {
    return {node.pos.x(), node.pos.y()};
}

struct cut_gesture: public gesture {
    const trail & t;
    World & w;
    double last_time = 0;
    TrailEffect * trailEfffect;
    weak_object_ptr cut_sound_object;
    weak_object_ptr tool_ptr;
    cut_gesture(const trail & t, World & w, orxOBJECT * tool): t(t), w(w), tool_ptr(tool){
        trailEfffect = Create<TrailEffect>();
        trailEfffect->t = &t;
    }
    ~cut_gesture() {trailEfffect->SetLifeTime(0);}

    void updateCutSoundVolume(double dt) {
        auto sound = getSound(cut_sound_object);
        if(sound) {
            orxFLOAT volumeCoeff = exp(-4*dt);
            orxFLOAT newVolume = orxSound_GetVolume(sound)*volumeCoeff;
            orxSound_SetVolume(sound, newVolume);
            if(newVolume<0.01) cut_sound_object = {};
        }
    }

    virtual orxSTATUS time_update(double time) {
        auto segs = segments(t, last_time,time);
        line_segment s;
        updateSwingSound();
        double dt = time-last_time;
        updateCutSoundVolume(dt);
        while(segs(s)) {
            for(Bond & b: w.getBonds()) {
                if(intersects(s, {toPoint(b.node1), toPoint(b.node2)})) {
                    b.deleteLater();
                    if(auto sound = getSound(cut_sound_object)) {
                        orxSound_SetVolume(sound, orxSound_GetVolume(sound) + 0.2);
                        SetPosition(cut_sound_object, traceToOrxVector(t.traces.back()));
                    } else {
                        cut_sound_object = CreateSoundObject("KnifeCutSound", traceToOrxVector(t.traces.back()));
                    }
                }
            }
        }
        last_time = time;
        auto tool = scroll_cast<KnifeTool>(tool_ptr);
        if(!tool) return orxSTATUS_FAILURE;
        return t.active && tool->selected ? orxSTATUS_SUCCESS: orxSTATUS_FAILURE;
    }

    enum {
        READY, SWINGING
    } swingstate = READY;

    void updateSwingSound() {
        const int range = 5;
        if(t.traces.size()>=range) {
            auto & curtrace = t.traces[t.traces.size()-1];
            auto & prevtrace = t.traces[t.traces.size()-range];
            double dx = curtrace.x - prevtrace.x, dy = curtrace.y - prevtrace.y;
            double curspeed = sqrt(dx*dx + dy*dy) / (curtrace.t - prevtrace.t);
            if(swingstate == READY && curspeed > 100) {
                swingstate = SWINGING;
                CreateSoundObject("KnifeSwingSound", traceToOrxVector(curtrace));
            } else if(swingstate == SWINGING && curspeed < 10) {
                swingstate = READY;
            }
        }
    }
};

void KnifeTool::SetSelected(bool selected) {
    Tool::SetSelected(selected);
    if(selected) {
        auto surface = Create<Button>("ToolGestureSurface");
        surface->set_handler([this](trail & t){
            return new cut_gesture{t,*world,GetOrxObject()};
        });
        gesture_surface = surface->GetOrxObject();
        orxObject_SetOwner(gesture_surface, GetOrxObject());
    }
    else {
        orxObject_SetLifeTime(gesture_surface,0);
    }
}

void KnifeTool::SetContext(ExtendedMonomorphic& context) {
    world = context.GetAspect<game_context>()->world.get();
}
