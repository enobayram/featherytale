/*
 * mappedTexture.h
 *
 *  Created on: 04/11/2011
 *      Author: eba
 */

#ifndef MAPPEDTEXTURE_H_
#define MAPPEDTEXTURE_H_
#define _USE_MATH_DEFINES

#include <memory>
#include <vector>

#include <Eigen/Core>

#include "visualElements/forward_declarations.h"
#include "visualElements/containers.h"
#include "visualElements/attributes.h"
#include "physicalElements.h"



using namespace Eigen;

class MappedTexture {
	friend class Drawer;
	NodePinSet NodesSet;
	BondPinSet BondsSet;
	Vector2d p1, p2;
	DiagonalMatrix<double,2> s;
	InvalidList toDelete;
public:
	std::unique_ptr<TriangleCollection> Triangles; // This is a pointer so that I don't have to include triangle.h
	MappedTexture(std::list<Node *>, std::list<Bond *>, Vector2d p1, Vector2d p2);
	virtual ~MappedTexture();
	NodePin & getPin(Node * node);
	BondPin & getPin(Bond * bond);
	Vector2d textureTransform(Vector2d pw){return Vector2d(s*(pw-p1));}
	void updateInternalState();
	Vector2d scale(const Vector2d pw) {return s*pw;}
	Vector2d invScale(const Vector2d pt) {return s.inverse()*pt;}
	void deleteLater(Invalidatable * inv) {toDelete.push_front(*inv);}
	void performDeletions() {
		InvalidList deleteHere; // Some deletions trigger other deletions, which should be performed at the next phase
		deleteHere.swap(toDelete);
		deleteHere.clear_and_dispose(boost::checked_deleter<Invalidatable>());
	}
	std::vector<Node *> getNodes();
	bool empty() { return NodesSet.empty(); }
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

#endif /* MAPPEDTEXTURE_H_ */
