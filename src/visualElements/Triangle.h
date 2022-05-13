/*
 * Triangle.h
 *
 *  Created on: Dec 22, 2011
 *      Author: eba
 */

#ifndef TRIANGLE_H_
#define TRIANGLE_H_

#include <memory>
#include <functional>
#include <iostream>

#include <Eigen/Core>

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/array.hpp>
#include <boost/checked_delete.hpp>
#include <boost/bind/mem_fn.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/find.hpp>

#include "visualElements/forward_declarations.h"
#include "visualElements/attributes.h"
#include "visualElements/ghosts.h"
#include "visualElements/pinTypes.h"
#include "visualElements/triangle_proxies.h"
#include "visualElements/mp_aux.h"

namespace fusion = boost::fusion;

using std::unique_ptr;
using std::forward;
using boost::is_reference;

typedef std::unique_ptr<TriangleProxy> ProxyPtr;

class TriangleBase: public Invalidatable //: public PhasedDeletable
{
    ProxyPtr proxy;
public:
	MappedTexture *owner;
	inline TriangleBase(TriangleProxy *proxy, MappedTexture *owner);
	virtual void invalidateTriangle() = 0;
	inline void invalidating();
	virtual ~TriangleBase(){} // The proxy will be deleted automatically
	template<class information>
	inline void informProxy(information inf);
	virtual TriangleNode * getNode(int node) = 0;
	TriangleNode * otherNode(const BondPin & bond) {
		TriangleNode * n;
		for(int i=0; i<3; i++) {
			n = getNode(i); if(n!=bond.node1 && n!=bond.node2) return n;
		}

		return NULL;
	}
	virtual std::string printtype() = 0;
};


typedef boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink > >triangle_hook;
template<class node1type, class node2type, class node3type>
struct Triangle: public TriangleBase, public triangle_hook {
	fusion::vector<node1type, node2type, node3type> nodes;

	inline Triangle(node1type node1, node2type node2, node3type node3, MappedTexture * owner, TriangleProxy * proxy)
	: TriangleBase(proxy,owner), nodes(node1, node2, node3) {
		node1type & node1Int = fusion::at_c<0>(nodes);
		node2type & node2Int = fusion::at_c<1>(nodes);
		node3type & node3Int = fusion::at_c<2>(nodes);

		node1Int.addTexOcc(node2Int,node3Int,*this);
		node2Int.addTexOcc(node3Int,node1Int,*this);
		node3Int.addTexOcc(node1Int,node2Int,*this);

		if(is_reference<node1type>::value) node1.addDependent(this);
		if(is_reference<node2type>::value) node2.addDependent(this);
		if(is_reference<node3type>::value) node3.addDependent(this);
	}
	template <class T> struct getPinning {
		Pinning operator()(T node) 	{return node.getPinning();}};

	std::string printtype() {return std::string(typeid(node1type).name()) + " " + typeid(node2type).name() + " " + typeid(node3type).name() + " ";}

	PinningTriplet getPinnings() {
		PinningTriplet result;
		result[0] = fusion::at_c<0>(nodes).getPinning();
		result[1] = fusion::at_c<1>(nodes).getPinning();
		result[2] = fusion::at_c<2>(nodes).getPinning();
		return result;
	}

	void invalidateTriangle() {
				if(!boost::is_reference<node1type>::value) fusion::at_c<0>(nodes).invalidate();
				if(!boost::is_reference<node2type>::value) fusion::at_c<1>(nodes).invalidate();
				if(!boost::is_reference<node3type>::value) fusion::at_c<2>(nodes).invalidate();

				if(boost::is_reference<node1type>::value) fusion::at_c<0>(nodes).removeDependent(this);
				if(boost::is_reference<node2type>::value) fusion::at_c<1>(nodes).removeDependent(this);
				if(boost::is_reference<node3type>::value) fusion::at_c<2>(nodes).removeDependent(this);

				DebugOut()<<"Attempting to remove texocc\n";//<<std::flush;
				fusion::at_c<0>(nodes).removeTexOcc(this);
				DebugOut()<<"Attempting to remove texocc\n";//<<std::flush;
				fusion::at_c<1>(nodes).removeTexOcc(this);
				DebugOut()<<"Attempting to remove texocc\n";//<<std::flush;
				fusion::at_c<2>(nodes).removeTexOcc(this);
				DebugOut();//<<std::flush;

				if(!boost::is_reference<node1type>::value) DebugOut()<<"nodeType1 is not a reference\n";
				if(!boost::is_reference<node2type>::value) DebugOut()<<"nodeType2 is not a reference\n";
				if(!boost::is_reference<node3type>::value) DebugOut()<<"nodeType3 is not a reference\n";


	}

	~Triangle() {
		DebugOut() <<"deleting triangle of type:" << printtype() << "\n";
	}

	virtual TriangleNode * getNode(int node) {
		switch(node) {
		case 0: return &fusion::at_c<0>(nodes);
		case 1: return &fusion::at_c<1>(nodes);
		case 2: return &fusion::at_c<2>(nodes);
		default: return NULL;
		}
	}

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

typedef Triangle<NodePin &, NodePin &, NodePin &> N3triangle; // A triangle composed of 3 nodes
typedef Triangle<NodePin &, NodePin &, BondGhost> BGtriangle; // A triangle composed of a bond and a ghost
typedef Triangle<NodePin &, BondGhost &, BondGhost &> BG2triangle; // A triangle composed of 2 bond ghosts and a node
typedef Triangle<NodePin &, BondGhost &, NodeGhost &> BGNtriangle;  // A triangle composed of a bond ghost a ghost and a node
typedef Triangle<NodePin &, NodeGhost &, NodeGhost &> NG2triangle; // A triangle composed of 2 node ghosts and a node

class TriangleCollection {
	typedef fusion::vector<
			N3triangle,
			BGtriangle,
			BG2triangle,
			BGNtriangle,
			NG2triangle
			> AllowedTypes;

	to_containers<AllowedTypes, triangle_hook>::type containers;

	template<class functional>
	struct applyFunctional {
		functional f;
		applyFunctional(functional f): f(f){}
		template<class T> void operator()(T & in) const {
			std::for_each(in.begin(),in.end(), f);
		}
	};

	struct emptyList {template<class listType> void operator()(listType & l) const {
		typedef typename boost::remove_reference<decltype(l.back())>::type triangleType;
			l.clear_and_dispose(boost::checked_deleter<triangleType>());}
	};

public:
	template< class node1Type, class node2Type, class node3Type>
	typename boost::enable_if< mpl::contains<AllowedTypes,
				    typename ProperTriangle<node1Type,node2Type,node3Type>::type >, TriangleBase * >::type
	addTriangle(node1Type node1, node2Type node2, node3Type node3,
			MappedTexture * owner, TriangleProxy * proxy = new NullProxy()) {
		typedef typename ProperTriangle<node1Type, node2Type, node3Type>::type TriangleType;
		typedef typename to_list<TriangleType, triangle_hook>::type list_type;
		TriangleType * newTriangle = new TriangleType(deref(node1), deref(node2), deref(node3), owner, std::move(proxy));
	    fusion::deref(fusion::find<list_type >(containers)).push_back(*newTriangle);
	    return newTriangle;
	}
	template<class functional>
	void apply(functional f) {
		fusion::for_each(containers, applyFunctional<functional>(f));
	}

	template <class functional>
	struct vertexApplier {
		functional * f;
		vertexApplier(functional &f):f(&f){}
		template<class T> void operator()(T & triangle) const {
			(*f)(triangle.getPinnings());
		}
	};

	template<class functional>
	void applyToVertices(functional f) {
		vertexApplier<functional> applier(f);
		apply(applier);
	}

	struct sizeAccumulator {
		int *size;
		sizeAccumulator(int &size):size(&size){}
		template<class T> void operator()(T & in) const {
			*size+=in.size();
		}
	};

	int getSize(){
		int size=0;
		sizeAccumulator acc(size);
		fusion::for_each(containers, acc);
		return size;
	}

	~TriangleCollection() {
		fusion::for_each(containers, emptyList());
	}

};

#endif /* TRIANGLE_H_ */
