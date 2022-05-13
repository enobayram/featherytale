/*
 * VicinityIterator.h
 *
 *  Created on: Dec 28, 2011
 *      Author: eba
 */

#ifndef VICINITYITERATOR_H_
#define VICINITYITERATOR_H_


#include "auxiliary/generator.h"
#include "Node.h"
#include "CellularGrid.h"

// This Cell iterator is now iterating under the rectangular grid assumption
// I might switch to a hexagonal grid sometime
// Node that this iterator iterates only over half of the neighbors for efficiency
$generator(CellIterator) {
    // TODO Improve this cell iterator. It iterates in a consistent manner, but it would be better, if it were iterating
    // over those neighbors that have STRICTLY greater memory addresses. This would ease the parallelization of the bond
    // processing, since it needs a fast way of checking if the cell of the other node is safe to lock (no deadlocks)
    CellularGrid & grid;
    Node & node;
    const double distance;
    int xindex, yindex; // These hold the cell indices of the node
    int i,j; // These are for iterating
    int maxyindex; // The index of the highest row within "distance" from the node (and in game field)
    int maxxindex;
    CellIterator(CellularGrid & grid, Node & node, double distance): grid(grid), node(node), distance(distance) {}
    bool isWithinCircle(const Vector2d & point) {
        return (point-node.pos).squaredNorm()<distance*distance;
    }
    typedef std::tuple<int,int> IndexType;
    $emit(IndexType)
        std::tie(xindex,yindex) = grid.getCellIndices(node.pos);
        maxyindex = min(int(floor((node.pos.y()+distance)/CELLWIDTH+GRIDSIZEY/2)), GRIDSIZEY-1);
        // Iterate over upper neighbors
        for(j=yindex+1;j<=maxyindex;j++) {
            $yield(std::make_tuple(xindex,j)); // The one in the middle is always sent
            // iterate towars left
            for(i = xindex-1; isWithinCircle(Vector2d((i+1)*CELLWIDTH+MINX,j*CELLWIDTH+MINY))&&i>=0; i--) {
                $yield(std::make_tuple(i,j));
            }
            // iterate towards right
            for(i = xindex+1; isWithinCircle(Vector2d((i)*CELLWIDTH+MINX,j*CELLWIDTH+MINY))&&i<GRIDSIZEX; i++) {
                $yield(std::make_tuple(i,j));
            }
        }
        maxxindex = min(int(floor((node.pos.x()+distance)/CELLWIDTH+GRIDSIZEX/2)), GRIDSIZEX-1);
        // Iterate over right neighbours
        for(i=xindex+1;i<=maxxindex; i++) {
            $yield(std::make_tuple(i,yindex));
        }
    $stop
    // OK, What I'll do now, is a little dirty...
    bool operator()(Cell * & arg) {
        IndexType index;
        bool result = operator ()(index);
        arg = grid.getCell(index);
        return result;
    }
};

CellIterator CellularGrid::getCellIterator(Node & node, double distance) {
    return CellIterator(*this,node,distance);
}

$generator(VicinityIterator) {
	CellularGrid & grid;
	Node & node;
	const double distance;
	CellIterator cellIterator;
	Cell * cell;
	Node * otherNode;
	NodeCellList::iterator nodeIterator;
	int nodeGridx;
	int nodeGridy;

	bool isWithinCircle(Node * otherNode) {
		Vector2d delta = node.pos-otherNode->pos;
		return delta.squaredNorm()<distance*distance;
	}

	VicinityIterator(CellularGrid & grid, Node & node, double distance):
		grid(grid), node(node), distance(distance), cellIterator(grid,node,distance){}
	$emit(Node *)
		//The treatment is different for the nodes in the same cell, and the nodes in the neighboring cells
		//In the same cell, we iterate over only those nodes that have a higher address (for efficiency)
		//In a neighboring cell, we iterate over all
		cell = grid.getCell(node);
		for(nodeIterator = cell->nodes.begin();nodeIterator!=cell->nodes.end(); nodeIterator++) {
			otherNode = &(*nodeIterator);
			if(otherNode>&node && isWithinCircle(otherNode))
				$yield(otherNode);
		}
		//Now, the neighboring cells
		while(cellIterator(cell)) {
			for(nodeIterator = cell->nodes.begin();nodeIterator!=cell->nodes.end(); nodeIterator++) {
				otherNode = &(*nodeIterator);
				if(isWithinCircle(otherNode))
					$yield(otherNode);
			}
		}
	$stop
};


#endif /* VICINITYITERATOR_H_ */
