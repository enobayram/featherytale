/*
 * HappinessSidebar.h
 *
 *  Created on: May 16, 2015
 *      Author: eba
 */

#ifndef HAPPINESSSIDEBAR_H_
#define HAPPINESSSIDEBAR_H_

#include <scroll_ext/ExtendedObject.h>
#include "HappinessIndicator.h"

class HappinessSidebar: public ExtendedMonomorphic {
    float happiness;
public:
    SCROLL_BIND_CLASS("HappinessSidebar")
    void setHappiness(float, bool with_fx = false);
    float getHappiness() {return happiness;}
    void ExtOnCreate();
    HappinessIndicator * indicator;
};

#endif /* HAPPINESSSIDEBAR_H_ */
