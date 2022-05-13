/*
 * ghosts.hpp
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#ifndef GHOSTS_HPP_
#define GHOSTS_HPP_

#include <Eigen/Geometry>
#include "visualElements/ghosts.h"
#include "visualElements/mappedTexture.h"

inline Pinning NodeGhost::getPinning() {
	return getPinning(node,relative,texturePoint);
}
inline Pinning NodeGhost::getPinning(NodePin & node,
		const Vector2d & relative, const Vector2d & texturePoint) {
	return Pinning{texturePoint, Rotation2D<double>(node.pinnedObject->angle)*relative+node.getPos()};
}

inline Vector2d BondGhost::getPos() {
	Vector2d delta = bond.node2->getPos() - bond.node1->getPos();
	Vector2d mid = (bond.node2->getPos() + bond.node1->getPos())/2;
	Vector2d unit = delta/delta.norm();
	Vector2d perp(-unit.y(), unit.x()); // CCW perpendicular
	return mid+BONDGHOSTDISTANCE*perp*(CCW?1:-1);
}

inline Vector2d NodeGhost::getPos() {
		return Rotation2D<double>(node.pinnedObject->angle)*relative+node.pinnedObject->pos;
}

inline NodeGhost::NodeGhost(NodePin & node, const Vector2d & relative): TriangleNode(node.owner->scale(relative)+node.texturePoint),
		relative(relative),  node(node) {}; // FIXME This doesn't take into account pinned orientation!!


#endif /* GHOSTS_HPP_ */
