/*
 * attributes.hpp
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#ifndef ATTRIBUTES_HPP_
#define ATTRIBUTES_HPP_

#include "visualElements/attributes.h"
#include "visualElements/mappedTexture.h"

void Invalidatable::invalidate(MappedTexture * owner) { // owner defaults to null meaning "just invalidate"
	if(valid) {
		valid = false;
		invalidating();
		if(owner) owner->deleteLater(this);
	}
}

#endif /* ATTRIBUTES_HPP_ */
