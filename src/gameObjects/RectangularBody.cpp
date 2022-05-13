/*
 * RectangularBody.cpp
 *
 *  Created on: Jun 13, 2015
 *      Author: eba
 */

#include <memory>
#include <vector>

#include <gameObjects/RectangularBody.h>

#include <Eigen/StdVector>
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(Vector2d) // This is required so that Array2D<Vector2d> is safe

using namespace std;

extern std::mt19937 gen;

template <class T>
class Array2D {
    std::vector<T> storage;
    int r, c;
public:
    Array2D(int r, int c): storage(r*c), r(r), c(c){}
    T & at(int r_, int c_) {
        return storage[c_+r_*c];
    }
};


unique_ptr<MappedTexture> createRectangle(double pX, double pY, int rx, int ry, double texPerNode, World & world, Vector2d vel=Vector2d::Zero()) {
    double thickness = BONDLENGTH;
    double left = pX - rx*thickness/2;
    double top = pY - ry*thickness/2;
    std::uniform_real_distribution<> deviation(-thickness/5, thickness/5);
    bool tendency = 0;
    Array2D<Vector2d> node_positions(rx,ry);
    Array2D<char> create_permissions(rx, ry); // bool doesn't work here since std::vector<bool> sucks
    for(int j=0; j<ry; j++) {
        for(int i = 0; i<rx; i++) {
            auto pos = Vector2d(left+(i+tendency*0.5)*thickness+deviation(gen),top+j*thickness*sqrt(3)/2+deviation(gen));
            node_positions.at(i,j) = pos;
            auto closest = world.closestNode(pos);
            if(closest == nullptr) create_permissions.at(i,j) = true;
            else {
                auto closestDistSq = (closest->pos - pos).squaredNorm();
                create_permissions.at(i,j) = closestDistSq > BONDLENGTH * BONDLENGTH;
            }
        }
        tendency=!tendency;
    }


    Array2D<Node *> nodes(rx,ry);
    std::list<Node *> nodeslist;
    std::list<Bond *> bondslist;
    tendency = 0;
    for(int j=0; j<ry; j++) {
        for(int i = 0; i<rx; i++) {
            if(!create_permissions.at(i,j)) continue;
            Node * node = world.createNode(node_positions.at(i,j),vel);
            auto create_bond = [&](int x, int y) {
                if(create_permissions.at(x,y)) bondslist.push_back(world.createBond(*node,*nodes.at(x,y)));
            };
            if(i>0) {
                create_bond(i-1,j);
            }
            if(j>0) {
                create_bond(i,j-1);
                int secondBondIndex = i+(tendency?1:-1);
                if(0<=secondBondIndex && secondBondIndex<rx) {
                    create_bond(secondBondIndex,j-1);
                }
            }
            nodes.at(i,j)=node;
            nodeslist.push_back(node);
        }
        tendency=!tendency;
    }
    Vector2d diagonal = Vector2d((rx+3)*thickness,(ry+3)*thickness);
    Vector2d lowerLeftPoint = Vector2d(left-1, top-1);
    Vector2d center = lowerLeftPoint + diagonal/2;
    Vector2d scaledDiagonal = Vector2d{thickness,thickness}/texPerNode;//textureScale*diagonal;
    return unique_ptr<MappedTexture>(new MappedTexture(nodeslist,bondslist,center-scaledDiagonal/2,center+scaledDiagonal/2));
//  drawer.addMappedTexture(mt);
}

void RectangularBody::CreateMappedTexture() {
    auto pos = GetValue<orxVECTOR>("BodyPosition");
    auto size = GetValue<orxVECTOR>("BodySize");
    auto vel = GetValue<orxVECTOR>("Velocity");
    auto texPerNode = GetValue<orxFLOAT>("TexturePerNode");
    auto withRetry = GetValue<bool>("WithRetry");
    mappedTexture = createRectangle(pos.fX,pos.fY, size.fX,size.fY, texPerNode, *world, Vector2d(vel.fX, vel.fY));
    if(withRetry) {
        while(mappedTexture->getNodes().size() != size.fX * size.fY) {
            pos = GetValue<orxVECTOR>("BodyPosition");
            for(Node * node: mappedTexture->getNodes()) world->removeNode(node);
            mappedTexture = createRectangle(pos.fX,pos.fY, size.fX,size.fY, texPerNode, *world, Vector2d(vel.fX, vel.fY));
        }
    }
}
