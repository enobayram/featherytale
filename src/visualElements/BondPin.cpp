/*
 * BondPin.cpp
 *
 *  Created on: Dec 25, 2011
 *      Author: eba
 */

#include "pinTypes.h"
#include "Triangle.hpp"

BondPin::BondPin(MappedTexture * owner, Bond * bond) :Pin<Bond>(bond), owner(owner), triangles{NULL,NULL}{
    node1 = &owner->getPin(&bond->node1);
    node2 = &owner->getPin(&bond->node2);

    Vector2d delta = owner->invScale(node2->getTexturePoint() - node1->getTexturePoint());
    Vector2d mid = owner->invScale(node2->getTexturePoint() + node1->getTexturePoint())/2;
    Vector2d unit = delta/delta.norm();
    Vector2d perp(-unit.y(), unit.x()); // CCW perpendicular
    ghostLocations[0] = owner->scale(Vector2d(mid-BONDGHOSTDISTANCE*perp));
    ghostLocations[1] = owner->scale(Vector2d(mid+BONDGHOSTDISTANCE*perp));

}

BondPin::~BondPin(){
}

void BondPin::removeTriangle(TriangleBase * remove) {
    for(TriangleBase * & t: triangles) {
        if(t==remove) {
            //bool CCW = isCCWinWorld(*t->otherNode(*this));
            t=NULL;//createTriangle(CCW);
            return;
        }
    }
    std::cerr<<"BONDPIN::REMOVETRIANGLE, TRYING TO REMOVE NON-MEMBER\n";
}

Vector2d triangleCenter(TriangleBase *t) {
    Vector2d result = Vector2d::Zero();
    for(int i=0; i<3; ++i) {
        result += t->getNode(i)->getTexturePoint();
    }
    return result/3;
}

void BondPin::addTriangle(TriangleBase * t){
    bool CCW = isCCWinWorld(*(t->otherNode(*this)));
    triangles[CCW] = t;
    Vector2d bondCenter = (node1->getTexturePoint() + node2->getTexturePoint())/2;
    Vector2d triCenter = triangleCenter(t);
    ghostLocations[CCW] = bondCenter*0.1 + triCenter*0.9;
}

bool BondPin::isCCWinWorld(TriangleNode & node) { // CONSTURCTION, DESTRUCTION ONLY!!
    Vector2d delta = owner->invScale(node2->getTexturePoint()) - owner->invScale(node1->getTexturePoint());
    Vector2d perp(-delta.y(), delta.x());
    return owner->invScale(node.getTexturePoint()-node2->getTexturePoint()).transpose()*perp > 0;
}

void BondPin::update() {
    if(triangles[0] && triangles[1]) return; // nothing to do here
    if(!triangles[0]) {
        Vector2d ghost2 = ghostLocations[0];
        triangles[0]=owner->Triangles->addTriangle(node1, node2,BondGhost(*this,ghost2,false), owner, new BGTriangleProxy());
    }
    if(!triangles[1]) {
        Vector2d ghost1 = ghostLocations[1];
        triangles[1]=owner->Triangles->addTriangle(node1, node2,BondGhost(*this,ghost1,true), owner, new BGTriangleProxy());
    }
}



