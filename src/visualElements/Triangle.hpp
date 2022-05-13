/*
 * Triangle.hpp
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#ifndef TRIANGLE_HPP_
#define TRIANGLE_HPP_

#include "util/debug.hpp"
#include "visualElements/attributes.hpp"
#include "visualElements/Triangle.h"
#include "visualElements/ghosts.hpp"

inline TriangleBase::TriangleBase(TriangleProxy *proxy, MappedTexture *owner):proxy(proxy), owner(owner){
	proxy->this_triangle = this;
}

inline void TriangleBase::invalidating() {
	DebugOut()<<"Invalidating triangle of type " << printtype() << " at:"<<this<<"\n";
	proxy->invalidating();
	invalidateTriangle();
}

template<class information>
inline void TriangleBase::informProxy(information inf) {proxy->informationAvailable(inf);}


#endif /* TRIANGLE_HPP_ */
