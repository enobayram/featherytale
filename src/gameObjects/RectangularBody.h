/*
 * RectangularBody.h
 *
 *  Created on: Jun 13, 2015
 *      Author: eba
 */

#ifndef RECTANGULARBODY_H_
#define RECTANGULARBODY_H_

#include <pancarObject.h>

class RectangularBody: public pancarObject {
public:
    SCROLL_BIND_CLASS("Cheese")
    void CreateMappedTexture();
};

#endif /* RECTANGULARBODY_H_ */
