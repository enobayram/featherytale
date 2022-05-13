/*
 * mappedTexture.cpp
 *
 *  Created on: 04/11/2011
 *      Author: eba
 */

#include "physicalElements.h"
#include "visualElements/triangle_proxies.h"
#include "visualElements/Triangle.hpp"
#include "visualElements/mappedTexture.h"
#include "visualElements/lateDefinitions.hpp"


using namespace boost::tuples;

//MappedTexture::MappedTexture(MappedTexture && other):NodesSet(std::move(other.NodesSet)),BondsSet(std::move(other.BondsSet)),
//		p1(other.p1),p2(other.p2),s(other.s){
//	 Triangles = std::move(other.Triangles);
//}

MappedTexture::MappedTexture(std::list<Node *> nodeslist, std::list<Bond *> bondslist,
		Vector2d p1, Vector2d p2): p1(p1), p2(p2),
		s((1/(p2.x()-p1.x())),(1/(p2.y()-p1.y()))), Triangles(new TriangleCollection){
	for(Node * node: nodeslist) {
		NodesSet.insert(*new NodePin(this,node));
	}
	for(Bond * bond: bondslist) {
		BondsSet.insert(*new BondPin(this,bond));
	}
	for(NodePin & nodepin: NodesSet) {
		nodepin.discoverBonds();}

	//std::cout<<Nodes[5].bonds.size();

	// Discovering triangles
	for(NodePin &node1: NodesSet) {
		for(BondPin*bond1: node1.bonds) {
			NodePin & node2 = bond1->node1==&node1 ? *bond1->node2 : *bond1->node1;
			for(BondPin* bond2: node2.bonds) {
				NodePin & node3 = bond2->node1==&node2 ? *bond2->node2 : *bond2->node1;
				for(BondPin* bond3: node3.bonds) {
					NodePin & node4 = bond3->node1==&node3 ? *bond3->node2 : *bond3->node1;
					if(&node1==&node4 && &node1>&node2 && &node2>&node3) {
						//N3Triangles.push_back( N3Triangle(&node1,&node2,&node3,bond1,bond2,bond3));
						//N3Triangle * t = &N3Triangles.back();
						N3TriangleProxy * proxy = new N3TriangleProxy(bond1, bond2, bond3);
						TriangleBase * t = Triangles->addTriangle(&node1,&node2,&node3, this, proxy);
						bond1->addTriangle(t);
						bond2->addTriangle(t);
						bond3->addTriangle(t);
					}
				}
			}
		}
	}

	for(BondPin & bond: BondsSet ) { // Discover the outer bonds
		bond.update();
	}

	for(NodePin & node: NodesSet) {
		node.update();
	}

}

MappedTexture::~MappedTexture(){
		NodesSet.clear_and_dispose(boost::checked_delete<NodePin>);
		BondsSet.clear_and_dispose(boost::checked_delete<BondPin>);
}

NodePin::NodePin(MappedTexture * owner, Node * node) :Pin<Node>(node), TriangleNode(owner->textureTransform(node->pos)),
		owner(owner), updateGhosts(false) {
	pinnedOrientation = node->angle;
	bonds.reserve(node->getBonds().size());
}

void NodePin::discoverBonds() {
	for(Bond * const bond: pinnedObject->getBonds()){
		BondPin * newPin = &owner->getPin(bond);
		bonds.push_back(newPin);
		//std::cout<<bonds.size();
	}
}

void MappedTexture::updateInternalState() {
	//TODO N3Triangles that are missing only one bond should stay alive,
	//so that they appear as the interior of the object showing through the crack
	for(BondPin & bond: BondsSet) { // Discover expired bondpins (they expire when their world counterparts are destroyed)
		if(bond.expired()) bond.invalidate(this);
	}

	for(NodePin & node: NodesSet) {
	    if(node.expired()) node.invalidate(this);
	}

	for(BondPin & bond: BondsSet) {
		if(bond.valid)bond.update();
	}

	for(NodePin & node: NodesSet) {
		if(node.valid && node.updateGhosts) {
		    node.update();
		    node.updateGhosts = false;
		}
	}
	performDeletions(); // TODO Delete invalid objects
}

void BondPin::invalidating() {
	DebugOut()<<"Invalidating BondPin at:"<<this<<"\n";
	for(TriangleBase * t: triangles) {
		if(t) t->invalidate(owner);
	}
}


NodePin::~NodePin()  {
	nodeGhosts.clear_and_dispose(boost::checked_deleter<NodeGhost>());
	occupancies.clear_and_dispose(boost::checked_deleter<TextureOccupancy>());
}

//void PhasedDeletable::deleteBy(MappedTexture * owner) {
//	if(!isDeleting) {
//		preDeletion();
//		owner->deleteLater(this);
//		isDeleting=true;
//	}
//}
//
//void TriangleNode::preDeletion() {
//	for(TriangleBase * t, dependents) t->invalidate();
//}

NodePin & MappedTexture::getPin(Node * node) {return * (NodesSet.find(node, Comp<NodePin,Node*>()));}
BondPin & MappedTexture::getPin(Bond * bond) {return * (BondsSet.find(bond, Comp<BondPin,Bond*>()));}

std::vector<Node*> MappedTexture::getNodes() {
    std::vector<Node *> result;
    for(NodePin & p: NodesSet) {
        if(!p.expired()) result.push_back(p.pinnedObject);
    }
    return result;
}
