/*
 * NodePin.cpp
 *
 *  Created on: Dec 25, 2011
 *      Author: eba
 */
#include <Eigen/Core>

#include "util/memory.hpp"
#include "visualElements/Triangle.hpp"
#include "visualElements/mappedTexture.h"

bool intersects(TexOccList & occupancies, double beginAngle, double endAngle) {
    for(TextureOccupancy &t: occupancies) {
        if(positiveAngle(t.end-t.begin) > positiveAngle(beginAngle-t.begin)) return true;
        if(positiveAngle(endAngle-beginAngle) > positiveAngle(t.begin - beginAngle)) return true;
    }
    return false;
}

void NodePin::addTexOcc(TriangleNode & node1, TriangleNode & node2, TriangleBase & triangle) {
	// TODO There's a lot to do here:
	// 1 When two texture occupancies map to the same address, the old one gets expelled!!! Leaking memory !!8-S!!
	Vector2d pos1 = node1.getTexturePoint(), pos2 = node2.getTexturePoint(), post = texturePoint;
	Vector2d scaledDelta1 = owner->invScale(pos1 - post);
	Vector2d scaledDelta2 = owner->invScale(pos2 - post);
	double angle1 = atan2(scaledDelta1.y(), scaledDelta1.x());
	double angle2 = atan2(scaledDelta2.y(), scaledDelta2.x());
	double angleDiff = normalizeAngle(angle2-angle1);
	double beginAngle, endAngle;
	TriangleNode * beginNode, * endNode;
	if(angleDiff>0) {beginAngle = angle1; endAngle = angle2; beginNode = &node1; endNode = &node2;}
	else {  		 beginAngle = angle2; endAngle = angle1; beginNode = &node2; endNode = &node1;}
	assert(!intersects(occupancies,beginAngle, endAngle));
	occupancies.insert(*new TextureOccupancy(triangle,beginAngle, endAngle, *beginNode, *endNode));
}

void NodePin::removeTexOcc(TriangleBase * triangle) {
	TextureOccupancy * texocc;
	updateGhosts = true;
	for(TextureOccupancy & t: occupancies ) {
		if(&(t.triangle) == triangle) {
			texocc = & t;
			texocc->unlink();
			delete texocc;
			DebugOut()<<"Deleted texocc\n";
			return;
		}
	}
	std::cerr<<"No such texocc!\n";
	//occupancies.erase_and_dispose(*texocc, checked_deleter<TextureOccupancy>());
}

// This function returns the distance of a point to a line, but it can also return
// a negative value, meaning that the point is on the CW side of the line.
double signedDistanceToLine(Vector2d point, Vector2d lineBegin, Vector2d lineEnd) {
    Vector2d vectorToBegin = lineBegin-point;
    Vector2d delta = lineEnd-lineBegin;
    Vector2d unit = delta.normalized();
    Vector2d CWperp(unit.y(),-unit.x()); // perpendicular to unit in the CW direction
    return CWperp.dot(vectorToBegin);
}

Vector2d unitVector(double angle) {
    return {cos(angle), sin(angle)};
}

const double MAXGHOSTANGLE = 0.5;

NodeGhost * NodePin::createNodeGhost(const Vector2d & relPosInWorld) {
    NodeGhost * newGhost;
    nodeGhosts.push_back(*(newGhost = new NodeGhost(*this, relPosInWorld)));
    return newGhost;
}

void NodePin::update()  {
    if(occupancies.empty()) {
        int numOfTriangles = ceil(2*M_PI/MAXGHOSTANGLE);
        double angleStep = 2*M_PI/numOfTriangles;
        NodeGhost * firstGhost = createNodeGhost(unitVector(0)*NEWNODEDISTANCE);
        NodeGhost * prevGhost = firstGhost;
        for(int i=1; i<numOfTriangles; i++) {
            NodeGhost * newGhost = createNodeGhost(unitVector(angleStep*i)*NEWNODEDISTANCE);
            pushTriangle(*newGhost,*prevGhost);
            prevGhost = newGhost;
        }
        pushTriangle(*firstGhost,*prevGhost);
    } else {
        TexOccList::iterator it = occupancies.end();
        TextureOccupancy * prev = &(*(--it));
        for(TextureOccupancy & t: occupancies) { // This loop looks for a gap, tries to fill it and returns (since the iterator is expired)
            if(t.end == prev->begin) { // there is no gap between the triangles
                prev = &t;
                continue;
            }
            double gap = positiveAngle(prev->begin-t.end);
            if(gap <= MAXGHOSTANGLE) { // These two nodes can form a valid triangle
                pushTriangle(prev->beginNode, t.endNode);
                update(); // We've filled a gap, recursively fill the other gaps
                return;
            } else {
                int numOfNewTriangles = ceil(gap/MAXGHOSTANGLE);
                double newAngle = t.end + gap/numOfNewTriangles;
                Vector2d proposedGhost = unitVector(newAngle)*NEWNODEDISTANCE;
                pushTriangle(*createNodeGhost(proposedGhost),t.endNode);
                update();
                return;
            }
        }
    }
}

void nodeTypeUndefined() {
	std::cerr<<"Trying to create a triangle of undefined type" << __FILE__ << ":"<<__LINE__<<"\n";
}

template<class Node1, class Node2>
void NodePin::pushTriangle(Node1 & node1, Node2 & node2) {
	if(NodeGhost * node1full = dynamic_cast<NodeGhost*>(&node1)) {
		if(NodeGhost * node2full = dynamic_cast<NodeGhost*>(&node2)) {
			owner->Triangles->addTriangle(this,node1full, node2full,owner);
		}
		else if(BondGhost * node2full = dynamic_cast<BondGhost*>(&node2)) {
			owner->Triangles->addTriangle(this,node2full, node1full,owner);
		} else nodeTypeUndefined();
	}
	else if(BondGhost * node1full = dynamic_cast<BondGhost*>(&node1)) {
		if(NodeGhost * node2full = dynamic_cast<NodeGhost*>(&node2)) {
			owner->Triangles->addTriangle(this,node1full, node2full,owner);
		}
		else if(BondGhost * node2full = dynamic_cast<BondGhost*>(&node2)) {
			owner->Triangles->addTriangle(this,node1full, node2full,owner);
		} else nodeTypeUndefined();
	} else nodeTypeUndefined();
}

void BondGhost::invalidateNode(){
	DebugOut()<<"invalidating BondGhost at:" << this<<"\n";
	std::list<TriangleBase *> invalidateThese = dependents;
	for(TriangleBase * t: invalidateThese) {
		t->invalidate(bond.owner);
	}
}

void NodePin::invalidateNode() {
    DebugOut() << "Invalidating NodePin: " << this << " Num of Occupancies: " << occupancies.size() << "\n";
    while(!occupancies.empty()) {
        occupancies.begin()->triangle.invalidate(owner);
    }
}

