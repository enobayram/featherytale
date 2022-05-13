/*
 * Tool.h
 *
 *  Created on: Sep 21, 2015
 *      Author: eba
 */

#ifndef GAMEOBJECTS_TOOL_H_
#define GAMEOBJECTS_TOOL_H_

#include <scroll_ext/ExtendedObject.h>
#include "../aspects.h"

class Tool: public ExtendedMonomorphic, public click_handler {
protected:
    bool selected = false;
    bool cooling_down = false;
    bool free_tool = false;
    orxOBJECT * halo = nullptr;
    orxOBJECT * icon = nullptr;
    orxOBJECT * token_text = nullptr;
    orxFLOAT remaining_time = 0;
    orxU32 bonus = 0;
    orxOBJECT * bonus_ticks = nullptr;
    enum Appearance{ SELECTED, AVAILABLE, DISABLED, COUNTINGDOWN} appearance;
    void SetAppearance(Appearance);
    void ShowBonusTicks(orxU32 count);
public:
    virtual gesture * clicked(trail &);
    virtual void SetSelected(bool selected);
    virtual void Update(const orxCLOCK_INFO &_rstInfo);
    virtual void ExtOnCreate();
    bool IsAvailable();
    void AddBonus(orxU32 count);
};

#endif /* GAMEOBJECTS_TOOL_H_ */
