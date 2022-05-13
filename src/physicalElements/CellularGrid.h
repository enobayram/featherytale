/*
 * CellularGrid.h
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#ifndef CELLULARGRID_H_
#define CELLULARGRID_H_

#include <algorithm>
#include <tuple>

#include <Eigen/Core>

#include "auxiliary/generator.h"
#include "constants.h"
#include "physicalElements/Cell.h"

using std::min;

class Node;
struct VicinityIterator;
struct CellIterator;

// TODO use a hash table to determine whether two atoms are bonded.
// TODO define a smooth potential function of proximity to walls.

class CellularGrid {
	friend struct CellIterator;
	friend struct VicinityIterator;
	Cell cells[GRIDSIZEY][GRIDSIZEX];
public:
	CellularGrid();
	virtual ~CellularGrid();
	void moveNode(Node & node, const Vector2d & from, const Vector2d & to) {
		node.NodeCellHook::unlink();
		Cell * newcell = getCell(getCellIndices(to));
		newcell->nodes.push_back(node);
	}
	VicinityIterator getVicinityIterator(Node & node, double distance);
	inline CellIterator getCellIterator(Node & node, double distance);
	std::tuple<int,int> getCellIndices(const Vector2d & pos) {
		return std::tuple<int,int>(int(floor(pos.x()/CELLWIDTH+GRIDSIZEX/2)),int(floor(pos.y()/CELLWIDTH+GRIDSIZEY/2)));}
	Cell * getCell(Node & node) {
	    return getCell(node.pos);
	}
	Cell * getCell(const Vector2d & pos) {
        int xind, yind; std::tie(xind,yind) = getCellIndices(pos);
        return getCell(xind,yind);
	}
	void registerNode(Node & node) {
		Cell * cell = getCell(node);
		cell->nodes.push_back(node);
	}
	Cell * getCell(std::tuple<int,int> index) {
		return getCell(std::get<0>(index), std::get<1>(index));
	}
	Cell * getCell(int x, int y) {
	    return &(cells[y][x]);
	}
};

#endif /* CELLULARGRID_H_ */
