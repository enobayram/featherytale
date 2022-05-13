/*
 * InfiniteModeEndDialog.h
 *
 *  Created on: Jan 18, 2016
 *      Author: eba
 */

#ifndef GAMEOBJECTS_DIALOGS_INFINITEMODEENDDIALOG_H_
#define GAMEOBJECTS_DIALOGS_INFINITEMODEENDDIALOG_H_

#include <scroll_ext/ExtendedObject.h>
#include <orxFSM/behavior_context.hpp>

#include <gameObjects/aspects.h>

class InfiniteModeEndDialog: public button_container, public behavior_context_mixin<InfiniteModeEndDialog> {
    SCROLL_BIND_CLASS("InfiniteModeEndDialog")
    int displayed_bonuses = 0;
    orxOBJECT * coins_owned_display = nullptr;
public:
    void AddButton(orxOBJECT *, orxFLOAT xPos);
    void RemoveButton(orxOBJECT *);
    void DisplayBonusCount(int count, const char * graphic_name);
};

#endif /* GAMEOBJECTS_DIALOGS_INFINITEMODEENDDIALOG_H_ */
