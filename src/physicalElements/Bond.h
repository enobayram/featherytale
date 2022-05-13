/*
 * Bond.h
 *
 *  Created on: Dec 18, 2011
 *      Author: eba
 */

#ifndef BOND_H_
#define BOND_H_

#include <list>

#include <boost/intrusive/list.hpp>
#include <boost/smart_ptr.hpp>

#include <Eigen/Core>

#include "ConstList.h"
#include "util/geometry.hpp"
#include "physicalElements/Node.h"

using namespace Eigen;
namespace bi = boost::intrusive;

class Node;
class Bond;
class World;

struct world_list_tag;

class Bond: public SharedCarrier {
	friend class World;
	friend bool shouldDelete<Bond>(const Bond &);
	bool upForDeletion;
public:
	Node & node1;
	Node & node2;
	double node1Angle;
	double node2Angle;
	void deleteLater() {upForDeletion=true;}
	Node * otherNode(Node * node) {return node==&node1 ? &node2 : &node1;} // TODO put an assert here
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
protected:
	Bond(Node & node1, Node & node2):upForDeletion(false),node1(node1), node2(node2){
		Vector2d delta = node2.pos-node1.pos;
		double absoluteAngle = atan2(delta.y(),delta.x());
		node1Angle = normalizeAngle(absoluteAngle - node1.angle);
		node2Angle = normalizeAngle(absoluteAngle - node2.angle);
	}
	virtual ~Bond(){
		for(auto node: {&node1, &node2}) {
			node->bonds.remove(this);
		}
	};
private:
	Bond(Bond & bond);
	Bond & operator=(Bond &);

};

typedef bi::list_base_hook<bi::tag<world_list_tag>, bi::link_mode<bi::auto_unlink> > BondWorldHook;
typedef HookedItem<Bond, World, BondWorldHook> HookedBond;
typedef bi::list<HookedBond, bi::base_hook<BondWorldHook>, bi::constant_time_size<false> > WorldBondList;
typedef ConstList<WorldBondList,World,Bond> ConstBondList;


#endif /* BOND_H_ */
