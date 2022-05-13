/*
 * Node2.cpp
 *
 *  Created on: Dec 18, 2011
 *      Author: eba
 */

#include "Node.h"
#include "Bond.h"

void Node::addBond(Bond & bond) {
	bonds.push_back(&bond);
	bondedNodes.insert(bond.otherNode(this));
}
