/*
 * Cell.h
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#ifndef CELL_H_
#define CELL_H_

#include <physicalElements/Node.h>

struct Cell {
	// TODO implement a per-cell lock
	// TODO maintain a list of non-empty cells
	NodeCellList nodes;
	Cell();
};

#endif /* CELL_H_ */
