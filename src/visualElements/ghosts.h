/*
 * ghosts.h
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#ifndef GHOSTS_H_
#define GHOSTS_H_

#include <Eigen/Core>

#include "visualElements/attributes.h"

using boost::intrusive::list_base_hook;


struct BondGhost: public TexOccRefuser, public TriangleNode { // A ghost attached to a bond
	BondPin & bond;
	// TODO Bond Ghosts should be created in a smarter way:
	// When a N3Triangle is being destroyed, it's center of gravity should become the bondGhosts for the remaining bonds
	// When a new bond is being created, The bondGhost should ask the nodes for their permitted distances
	bool CCW; // The orientation of the triange with respect to the bond unit vector
	BondGhost(BondPin & bond, const Vector2d & texturePoint, bool CCW): TriangleNode(texturePoint),bond(bond),CCW(CCW){};
	inline Vector2d getPos();
	Pinning getPinning() {
		return Pinning{texturePoint, getPos(),false};
	}
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	virtual Vector2d getPosDyn() {return getPos();}
	void printType() {std::cout<<"I am a BondGhost\n";}
	void invalidateNode();
};

typedef list_base_hook<> NodeGhostHook;

struct NodeGhost: public NodeGhostHook, public TexOccRefuser, public TriangleNode { // A ghost attached to only a node
	Vector2d relative;
	NodePin & node;
	inline NodeGhost(NodePin & node, const Vector2d & relative);
	Pinning getPinning();
	static Pinning getPinning(NodePin & node, const Vector2d & relative, const Vector2d & texturePoint);
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	inline Vector2d getPos();
	virtual Vector2d getPosDyn() {
		return getPos();
	}
	void printType() {std::cout<<"I am a NodeGhost\n";}
	void invalidateNode(){
	    assert(!"NodeGhost::invalidateNode has not been implemented");
	}
};
typedef intrusive::list<NodeGhost> NodeGhostList;



#endif /* GHOSTS_H_ */
