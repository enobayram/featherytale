/*
 * ForceArrow.cpp
 *
 *  Created on: Jun 27, 2015
 *      Author: eba
 */

#include <gameObjects/ForceArrow.h>

const orxFLOAT forceZ = -0.2f;

void UpdateForceArrow(ForceArrow & arr, ExternalForce & force) {
    orxASSERT(!force.getNodes().empty())
    auto forceX = force.getForce().x();
    auto forceY = force.getForce().y();
    Vector2d centerOfMass = Vector2d::Zero();
    for(Node * n: force.getNodes()) centerOfMass += n->pos;
    centerOfMass /= force.getNodes().size();
    orxVECTOR forceOrigin{orxFLOAT(centerOfMass.x()), orxFLOAT(centerOfMass.y()),forceZ};
    arr.SetPosition(forceOrigin);
    arr.SetRotation(orxFLOAT(atan2(centerOfMass.y()-forceY,centerOfMass.x()-forceX)+PI/2));

    orxVECTOR objSize;
    auto objHeight = orxObject_GetSize(arr.GetOrxObject(), &objSize)->fY;
    orxFLOAT arrowScale = (orxFLOAT) min((force.getForce()-centerOfMass).norm()/objHeight,0.04);
    orxVECTOR arrowScaleVec{arrowScale,arrowScale,0};
    arr.SetScale(arrowScaleVec);
}

ForceArrow* ForceArrow::Create(ExternalForce* force) {
    ForceArrow * result = ::Create<ForceArrow>();
    result->force = force->getWeakPtr<ExternalForce>();
    UpdateForceArrow(*result, *force);
    result->AddSound("PickCheeseSound");
    return result;
}

void ForceArrow::Update(const orxCLOCK_INFO& clock) {
    ExternalForce * ext_force = force.lock().get();
    if(ext_force && !ext_force->getNodes().empty()) {
        UpdateForceArrow(*this, *ext_force);
    } else  SetLifeTime(0);
}
