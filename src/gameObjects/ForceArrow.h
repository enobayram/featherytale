/*
 * ForceArrow.h
 *
 *  Created on: Jun 27, 2015
 *      Author: eba
 */

#ifndef FORCEARROW_H_
#define FORCEARROW_H_

#include <scroll_ext/ExtendedObject.h>

#include <physicalElements/World.h>

class ForceArrow: public ExtendedMonomorphic {
    ExternalForce::weak_ptr<ExternalForce> force;
public:
    SCROLL_BIND_CLASS("ForceArrow")
    static ForceArrow * Create(ExternalForce *);
    void Update(const orxCLOCK_INFO &);
};

#endif /* FORCEARROW_H_ */
