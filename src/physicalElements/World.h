/*
 * World.h
 *
 *  Created on: Dec 18, 2011
 *      Author: eba
 */

#ifndef WORLD_H_
#define WORLD_H_

#include <iostream>
#include <list>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <vector>

#include <Eigen/Core>

#include "util/memory.hpp"
#include "VariousElements.h"
#include "Node.h"
#include "Bond.h"
#include "CellularGrid.h"

using namespace Eigen;

// TODO implement per-node and per-bond physical parameters
class World {
	ConstNodeList constNodes;
	WorldNodeList &nodes;
	WorldNodeList nodesToDelete;

	ConstBondList constBonds;
	WorldBondList &bonds;

	ForceList forces;

	double time;

public:
	typedef std::chrono::duration<double> seconds;

	CellularGrid grid;

	double timeMultiplier;

	World():nodes(constNodes), bonds(constBonds), time(0), timeMultiplier(0) {
	}

	~World();

	seconds getTime() { return seconds(time);}
	void incrementTime(seconds increment) {
		time += timeMultiplier*increment.count();
	}

	Node * createNode(const Vector2d & pos, const Vector2d & vel = Vector2d::Zero(0)) {
		HookedNode * newnode = new HookedNode(pos,vel);
		nodes.push_back(*newnode);
		grid.registerNode(*newnode);
		return newnode;
	}

	Bond * createBond(Node & node1, Node & node2) {
		HookedBond *newBond = new HookedBond(node1, node2);
		bonds.push_back(*newBond);
		node1.addBond(*newBond);
		node2.addBond(*newBond);
		return newBond;
	}

	void bringBackToField(Vector2d & pos) {
		const double minwalldistance = 0.1;
		if(pos.x()>=MAXX-minwalldistance) pos.x() = MAXX-minwalldistance;
		if(pos.x()<=MINX+minwalldistance) pos.x() = MINX+minwalldistance;
		if(pos.y()>=MAXY-minwalldistance) pos.y() = MAXY-minwalldistance;
		if(pos.y()<=MINY+minwalldistance) pos.y() = MINY+minwalldistance;
	}

	void displace(Node & node, const Vector2d & displacement) {
		setNodePos(node,node.pos+displacement);
	}

    void setNodePos(Node & node, const Vector2d & pos) {
        auto newpos = pos;
        bringBackToField(newpos);
        grid.moveNode(node,node.pos,newpos);
        node.pos = newpos;
    }

	void performDeletions() {
		// TODO Implement delayed deletion through "toDelete" linked list, this is an inefficient placeholder
		bonds.remove_and_dispose_if(shouldDelete<Bond>, boost::checked_delete<HookedBond>);
		nodesToDelete.clear_and_dispose(boost::checked_delete<HookedNode>);
	}

	ExternalForce * createExternalForce(const Vector2d & loc, double r);

	ConstNodeList & getNodes() {
		return constNodes;
	}

	ConstBondList & getBonds() {
		return constBonds;
	}

	const ForceList & getForces() {
		return forces;
	}

	Node * closestNode(const Vector2d & pos);
    Node * closestNode(Node * node);
    Node * closestNode(const Vector2d & pos, double radius);

	Node * nodeAt(const Vector2d & pos);
	std::list<Node *> nodesAt(const Vector2d & pos, double radius);

	void removeForce(ExternalForce * force) {
		if(!force) {
			std::cout<<"A null force pointer has been passed for removal!\n";
			return;
		}
		forces.erase(forces.iterator_to(*force));
		delete force;
	}

	void removeNode(Node * node) {
	    for( Bond *b: node->bonds ) b->deleteLater();
	    for(ExternalForce &force: forces) force.removeNode(node);
	    auto & hookedNode = static_cast<HookedNode &>(*node);
	    nodes.erase(nodes.iterator_to(hookedNode));
	    nodesToDelete.push_front(hookedNode);
	}

	std::vector<ExternalForce *> getForcesOnNode(Node * node) {
		std::vector<ExternalForce *> result;
		for(ExternalForce & force: forces) {
			for(Node * node2: force.getNodes()) {
				if(node == node2) {
					result.push_back(&force);
				}
			}
		}
		return result;
	}

	size_t numberOfNodes() {
	    return nodes.size();
	}

private:
	World(World & world);
	World & operator=(World &);

};

#endif /* WORLD_H_ */
