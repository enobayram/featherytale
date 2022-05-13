/*
 * Node2.h
 *
 *  Created on: Dec 18, 2011
 *      Author: eba
 */

#ifndef NODE_H_
#define NODE_H_

#include <list>

#include <Eigen/Core>
#include <boost/intrusive/list.hpp>
#include <boost/unordered_set.hpp>

#include "ConstList.h"
#include "util/memory.hpp"

using namespace Eigen;
namespace bi = boost::intrusive;
using bi::constant_time_size;
using bi::base_hook;

class Node;
class Bond;
class World;

struct world_list_tag;
struct cell_tag;

typedef hooks<cell_tag>::auto_unlink_hook NodeCellHook;

class Node: public SharedCarrier, public NodeCellHook {
public:
	Vector2d pos;
	Vector2d vel;
	Vector2d force; // The accumulated force during a simulation iteration
	double angle;
	double spin;
	double torque = 0; // The accumulated torque during a simulation iteration
	int ID;
	virtual ~Node(){
	    assert(bonds.empty());
	};
	const std::list<Bond *> & getBonds() {
		return bonds;
	}
	bool isBondedTo(Node & other) {
		return bondedNodes.find(&other)!=bondedNodes.end();
	}
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
protected:
	Node(Vector2d pos, Vector2d vel = Vector2d::Zero(),double angle = 0, double spin = 0)
		:pos(pos), vel(vel), angle(angle),spin(spin),ID(nextID++){};
private:
	Node(Node & node);
	Node & operator=(Node &);
	void addBond(Bond & bond);
	static int nextID;
	friend class World;
	friend class Bond;
	std::list<Bond *> bonds;
	boost::unordered_set<Node *> bondedNodes;
};
typedef bi::list_base_hook<bi::tag<world_list_tag>, bi::link_mode<bi::auto_unlink> > NodeWorldHook;
typedef HookedItem<Node,World, NodeWorldHook> HookedNode;
typedef bi::list<HookedNode,base_hook<NodeWorldHook>, constant_time_size<false> > WorldNodeList;
typedef ConstList<WorldNodeList,World,Node> ConstNodeList;

typedef bi::list<Node, base_hook<NodeCellHook>, constant_time_size<false> > NodeCellList;

#endif /* NODE2_H_ */
