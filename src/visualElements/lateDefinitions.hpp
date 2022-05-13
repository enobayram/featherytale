/*
 * lateDefinitions.h
 *
 *  Created on: Dec 23, 2011
 *      Author: eba
 */

#ifndef LATEDEFINITIONS_H_
#define LATEDEFINITIONS_H_

#include "visualElements/ghosts.h"
#include "visualElements/pinTypes.h"
#include "visualElements/triangle_proxies.h"
#include "visualElements/Triangle.h"
#include "visualElements/mappedTexture.h"

inline Vector2d NodePin::ghostTexVector(int i) {
		double ghostAngle = i*2*M_PI/GHOSTCOUNT;
		return owner->scale(Vector2d(cos(ghostAngle), sin(ghostAngle))*GHOSTLENGTH)+texturePoint;
}


#endif /* LATEDEFINITIONS_H_ */
