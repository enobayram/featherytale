/*
 * World.cpp
 *
 *  Created on: Dec 18, 2011
 *      Author: eba
 */

#include <cmath>
#include <algorithm>
#include <stack>
#include <set>
#include <limits>

#include <boost/checked_delete.hpp>

#include "World.h"
#include "constants.h"

int	Node::nextID=0;

World::~World() {
	bonds.clear_and_dispose(boost::checked_deleter<HookedBond>());
	nodes.clear_and_dispose(boost::checked_deleter<HookedNode>());
    nodesToDelete.clear_and_dispose(boost::checked_deleter<HookedNode>());
	forces.clear_and_dispose(boost::checked_deleter<ExternalForce>());
}

Node * World::nodeAt(const Vector2d & pos) {
	for(Node & node: nodes){
		Vector2d dist = pos-node.pos;
		if(dist.squaredNorm()<NODESIZE*NODESIZE) {
			return &node;
		}
	}
	return NULL;
}

Node * World::closestNode(const Vector2d & pos) {
	auto dist = [&pos](Node & node) { return (pos - node.pos).squaredNorm();};
	if(nodes.empty()) return NULL;
	Node * closestNode = &nodes.front();
	double mindist = dist(*closestNode);
	for(auto & node: nodes) {
		if( dist(node) < mindist) {
			mindist = dist(node);
			closestNode = &node;
		}
	}
	return closestNode;
}

Node * World::closestNode(Node * node) {
    auto dist = [&node](Node & other_node) { return (other_node.pos - node->pos).squaredNorm();};
    Node * closestNode = nullptr;
    double mindist = std::numeric_limits<double>::max();
    for(auto & other_node: nodes) {
        if( &other_node != node && dist(other_node) < mindist) {
            mindist = dist(other_node);
            closestNode = &other_node;
        }
    }
    return closestNode;
}

std::list<Node *> World::nodesAt(const Vector2d & pos, double radius) {
	std::list<Node *> result;
	for(Node & node: nodes){
		Vector2d dist = pos-node.pos;
		if(dist.squaredNorm()<radius*radius) {
			result.push_back(&node);
		}
	}
	return result;
}

ExternalForce * World::createExternalForce(const Vector2d & loc, const double r) {
	std::list<Node *> nodes = nodesAt(loc,r);
	if(nodes.size()==0) return NULL;
	std::stack<Node *> stack;
	Node * closest = closestNode(loc);
	stack.push(closest);
	std::vector<Node *> selectedNodes{closest};
	std::set<Node *> consideredNodes{closest};
	while(!stack.empty()) {
		auto node = stack.top(); stack.pop();
		for(auto bond: node->bonds) {
			Node * otherNode = bond->otherNode(node);
			if(!consideredNodes.count(otherNode)) {
				consideredNodes.emplace(otherNode);
				if((loc-otherNode->pos).squaredNorm() < r*r) {
					stack.push(otherNode);
					selectedNodes.push_back(otherNode);
				}
			}
		}
	}

	ExternalForce * newForce = new ExternalForce(selectedNodes,loc,*this);
	forces.push_back(*newForce);
	return newForce;
}

$generator(RadialIterator) {
    CellularGrid & grid;
    const Vector2d center;
    const double radius;
    int i,j=0;
    int maxyindex; // The index of the highest row within "distance" from the node (and in game field)
    int minyindex;
    int maxxindex;
    Cell * cell;
    NodeCellList::iterator nodeIterator;
    RadialIterator(CellularGrid & grid, const Vector2d & center, double radius):
        grid(grid), center(center), radius(radius) {
        std::tie(i, minyindex) = grid.getCellIndices({center.x()-radius, center.y()-radius});
        std::tie(maxxindex, maxyindex) = grid.getCellIndices({center.x()+radius, center.y()+radius});
        i = std::max(i, 0), minyindex = std::max(minyindex, 0);
        maxxindex = std::min(maxxindex, GRIDSIZEX-1), maxyindex = std::min(maxyindex, GRIDSIZEY-1);
    }
    $emit(Node *)
        for(; i<=maxxindex; i++) {
            for(j = minyindex; j <= maxyindex; j++) {
                cell = grid.getCell(i,j);
                for(nodeIterator = cell->nodes.begin();nodeIterator!=cell->nodes.end(); nodeIterator++) {
                    if((nodeIterator->pos - center).squaredNorm() <= radius * radius) {
                        $yield(&(*nodeIterator));
                    }
                }
            }
        }
    $stop
};

Node* World::closestNode(const Vector2d& pos, double radius) {
    auto gen = RadialIterator(grid, pos, radius);
    Node * closest = nullptr;
    double closest_dist_sq = 0;
    for(Node * node; gen(node);) {
        double node_dist_sq = (pos-node->pos).squaredNorm();
        if(!closest || closest_dist_sq > node_dist_sq) {
            closest=node;
            closest_dist_sq = node_dist_sq;
        }
    }
    return closest;
}
