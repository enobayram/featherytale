/*
 * CellularGrid.cpp
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#include "CellularGrid.h"
#include "Node.h"
#include "VicinityIterator.h"

CellularGrid::CellularGrid() {

}

CellularGrid::~CellularGrid() {
}

VicinityIterator CellularGrid::getVicinityIterator(Node & node, double distance) {
	return VicinityIterator(*this, node,distance);
}
