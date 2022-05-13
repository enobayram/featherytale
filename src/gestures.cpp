/*
 * gestures.cpp
 *
 *  Created on: Mar 20, 2015
 *      Author: seliz
 */

#include <gestures.h>
#include <gameObjects/ForceArrow.h>

force_gesture::force_gesture(ExternalForce * force, World & world, trail * t): ext_force(force), world(world), t(t) {
    ForceArrow::Create(force);
}

force_gesture::~force_gesture() {
	t->attached_gesture = nullptr;
	world.removeForce(ext_force);
}

gesture::~gesture() {

}

orxSTATUS force_gesture::time_update(double time) {
	if(t->active && !ext_force->getNodes().empty()) {
		trace trc = t->traces.back();
		ext_force->setLocation({trc.x, trc.y});
		return orxSTATUS_SUCCESS;
	}
	else {
		return orxSTATUS_FAILURE;
	}
}
