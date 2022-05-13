/*
 * pinTypes.h
 *
 *  Created on: Dec 22, 2011
 *      Author: eba
 */

#ifndef PINTYPES_H_
#define PINTYPES_H_

#include <vector>

#include <boost/weak_ptr.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/slist.hpp>

#include "constants.h"
#include "physicalElements.h"
#include "visualElements/forward_declarations.h"
#include "visualElements/attributes.h"
#include "visualElements/ghosts.h"


using namespace Eigen;

using std::greater;

namespace intrusive = boost::intrusive;
using intrusive::base_hook;
using intrusive::set_base_hook;
using intrusive::optimize_size;
using intrusive::compare;
using intrusive::constant_time_size;


typedef set_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink> > TexOccHook;
struct TextureOccupancy: public TexOccHook, public Comparable<TextureOccupancy, double> {
	TriangleBase & triangle;
	double begin;
	double end;
	TriangleNode & beginNode;
	TriangleNode & endNode;
	double comparisonValue() const {return begin;}
	TextureOccupancy(TriangleBase & triangle, double begin, double end, TriangleNode & beginNode, TriangleNode & endNode)
	: triangle(triangle), begin(begin), end(end), beginNode(beginNode), endNode(endNode){}
};
typedef intrusive::set< TextureOccupancy, constant_time_size<false>, base_hook<TexOccHook>, compare<std::greater<TextureOccupancy> > > TexOccList;

struct NodePin: Pin<Node>, public TriangleNode {
	double pinnedOrientation;
	MappedTexture * owner;
	std::vector<BondPin*> bonds;
	TexOccList occupancies;
	bool updateGhosts;
	NodeGhostList nodeGhosts;

	enum{needsTexOcc=true};
	void addTexOcc(TriangleNode & node1, TriangleNode & node2, TriangleBase & triangle);
	NodePin(MappedTexture * owner, Node * node);
	inline Vector2d ghostTexVector(int i) ;
	void discoverBonds();
	void removeTexOcc(TriangleBase * triangle);
	void update();
	~NodePin();
	Pinning getPinning() {
		return Pinning{texturePoint,getPos(),true};
	}
	inline Vector2d getPos() {return pinnedObject->pos;}
	inline Vector2d getPosDyn() {return getPos();}
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	template<class Node1, class Node2> void pushTriangle(Node1 & node1, Node2 & node2);
	void printType() {std::cout<<"I am a NodePin\n";}
	void invalidateNode();
	NodeGhost * createNodeGhost(const Vector2d & relPosInWorld);
};

struct BondPin: Pin<Bond>, public Invalidatable
{
	NodePin * node1;
	NodePin * node2;
	MappedTexture * owner;
	TriangleBase * triangles[2];
	~BondPin();
	BondPin(MappedTexture * owner, Bond * bond);
	void addTriangle(TriangleBase * t);
	void removeTriangle(TriangleBase * remove);

	bool isCCWinWorld(TriangleNode & node);
	void update();
	void invalidating();
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
protected:
	void preDeletion();
	Vector2d ghostLocations[2];
};



#endif /* PINTYPES_H_ */
