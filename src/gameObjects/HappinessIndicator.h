/*
 * SparrowIndicator.h
 *
 *  Created on: May 16, 2015
 *      Author: eba
 */

#ifndef SPARROWINDICATOR_H_
#define SPARROWINDICATOR_H_

#include <scroll_ext/ExtendedObject.h>

class HappinessIndicator: public ExtendedMonomorphic {
public:
    SCROLL_BIND_CLASS("HappinessIndicator")
    void setPhase(double phase);
    void setHappiness(double happiness);
    void ExtOnCreate();
};

#endif /* SPARROWINDICATOR_H_ */
