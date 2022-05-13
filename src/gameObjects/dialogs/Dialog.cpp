/*
 * Dialog.cpp
 *
 *  Created on: May 24, 2015
 *      Author: eba
 */
#include "Dialog.h"


void Dialog::AddButton(orxOBJECT* button, orxFLOAT xPos) {
    orxObject_SetOwner(button, GetOrxObject());
    orxObject_SetParent(button, GetOrxObject());
    orxObject_SetGroupID(button, GetGroupID());
    orxVECTOR pos{xPos, GetValue<orxFLOAT>("ButtonRowY"), -0.1};
    orxObject_SetPosition(button, &pos);
}

void Dialog::RemoveButton(orxOBJECT* button) {
    orxObject_SetLifeTime(button, 0);
}
