/*
 * attributes.h
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#ifndef ATTRIBUTES_H_
#define ATTRIBUTES_H_

#include <iostream>
#include <list>

#include <boost/array.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/intrusive/set_hook.hpp>
#include <boost/weak_ptr.hpp>

#include <Eigen/Core>

#include "util/memory.hpp"
#include "visualElements/forward_declarations.h"
#include "visualElements/containers.h"

using namespace Eigen;

struct Pinning {
	Vector2d texC;
	Vector2d wldC;
	bool isNode;
};
typedef boost::array<Pinning,3> PinningTriplet;

class Invalidatable: public InvalidHook {
public:
	bool valid;
	Invalidatable(): valid(true){}
	inline void invalidate(MappedTexture * deleter=NULL);
	virtual void invalidating() {}
	virtual ~Invalidatable(){}
};

typedef set_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink> > PinHook;

struct TriangleNode: public Invalidatable //:public PhasedDeletable
{
	const Vector2d texturePoint;
	TriangleNode(const Vector2d & texturePoint): texturePoint(texturePoint){}
	Vector2d getTexturePoint() const {return texturePoint;}
	std::list<TriangleBase *> dependents;
	void addDependent(TriangleBase * dependent) {dependents.push_back(dependent);}
	void removeDependent(TriangleBase * dependent) {dependents.remove(dependent);}
	virtual ~TriangleNode(){}
	virtual void printType() = 0;
	virtual Vector2d getPosDyn() = 0;
	virtual void invalidating() {invalidateNode();}
	virtual void invalidateNode() = 0;
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct TexOccRefuser {
	enum{needsTexOcc = false};
	void AddTexOcc(TextureOccupancy * texOcc) {
		std::cerr<<"This object does not accept texture occupancy\n";
		assert(false);
	}
	void addTexOcc(TriangleNode &, TriangleNode &, TriangleBase &) {} // do nothing
	void removeTexOcc(TriangleBase *){} // do nothing
};

template<class T>
class Pin: public PinHook, public Comparable<Pin<T>, T* >{
	SharedCarrier::weak_ptr<> weakPtr;
public:
	T* pinnedObject;
	Pin(T* pinnedObject):weakPtr(pinnedObject->getWeakPtr()) , pinnedObject(pinnedObject) {}
	T* comparisonValue() const {return pinnedObject;}
	bool expired() {return weakPtr.expired();}
};


#endif /* ATTRIBUTES_H_ */
