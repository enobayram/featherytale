/*
 * HappinessSidebar.cpp
 *
 *  Created on: May 16, 2015
 *      Author: eba
 */

#include <gameObjects/HappinessSidebar.h>
#include <cmath>

void HappinessSidebar::ExtOnCreate() {
    auto indObj = orxObject_GetOwnedChild(GetOrxObject());
    indicator = (HappinessIndicator *) orxObject_GetUserData(indObj);
}

void HappinessSidebar::setHappiness(float happiness, bool with_fx) {
    if(with_fx) {
        auto delta = std::abs(happiness - this->happiness);
        auto fx = delta > 0.05 ? "HappinessChangeFXLarge" : "HappinessChangeFXSmall";
        indicator->AddFX(fx);
    }
    this->happiness = happiness;
    indicator->setHappiness(happiness);
    orxVECTOR size;
    orxObject_GetSize(GetOrxObject(),&size);
    indicator->SetPosition({0,size.fY/2 - size.fY*happiness,-0.0001});
}
