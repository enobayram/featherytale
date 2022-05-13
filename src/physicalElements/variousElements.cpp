/*
 * physicalElements.cpp
 *
 *  Created on: Nov 1, 2011
 *      Author: eba
 */

#include "VariousElements.h"
#include "World.h"

void ExternalForce::setLocation(Vector2d loc){
		//auto lock = world.getLockGuard();
		force = loc;
	};

void ExternalForce::remove(){
    world.removeForce(this);
}
