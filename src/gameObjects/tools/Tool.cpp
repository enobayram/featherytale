/*
 * Tool.cpp
 *
 *  Created on: Sep 21, 2015
 *      Author: eba
 */

#include <orx_utilities.h>
#include "Tool.h"
#include <gestures.h>
#include <util/config_processors.h>
#include <game_state.h>
#include <scroll_ext/eventdispatcher.h>
#include <analytics.h>

using namespace std;

gesture* Tool::clicked(trail&) {
	if(selected) {
		if(!IsPaused()) {
		    SetSelected(false);
		    AddSound("ToolCancelSound");
		}
	} else {
		if(IsAvailable() && !IsPaused()) {
		    SetSelected(true);
            AddSound("ToolStartSound");
		}
	}
    return new null_gesture;
}

void Tool::SetSelected(bool selected) {
    this->selected = selected;
    string model_name = GetModelName();
    auto ed = ::GetAspect<event_dispatcher>(orxOBJECT(orxObject_GetOwner(GetOrxObject())));
    if(selected) {
    	if(!free_tool) {
    	    if(bonus > 0) {
    	        --bonus;
    	        ShowBonusTicks(bonus);
    	    }
    	    else {
    	        setInventory(GetModelName(), getInventory(GetModelName())-1);
                resource_event(R_SINK, 1, GetModelName(), "Consume", orxObject_GetName(GetOwner(GetOrxObject())));
    	    }
    	}
        remaining_time = GetValue<orxFLOAT>("ActivePeriod");
        SetAppearance(SELECTED);
        SendSignal(ed, (model_name + "Selected").c_str());
    } else {
        if(free_tool) SetAppearance(AVAILABLE);
        else {
            remaining_time = GetValue<orxFLOAT>("Cooldown");
            cooling_down = true;
            SetAppearance(COUNTINGDOWN);
        }
        SendSignal(ed,(model_name + "Unselected").c_str());
    }
}

void Tool::Update(const orxCLOCK_INFO &_rstInfo)
{
    if(!free_tool) remaining_time = std::max(remaining_time-_rstInfo.fDT,0.0f);
    if(selected) {
        SetAlpha(halo, remaining_time/GetValue<orxFLOAT>("ActivePeriod"));
        if(remaining_time==0) {
            SetSelected(false);
            AddSound("ToolFinishSound");
        }
    } else if(cooling_down) {
        SetAlpha(halo, remaining_time/GetValue<orxFLOAT>("Cooldown"));
        if(remaining_time==0) {
        	cooling_down = false;
        	SetAppearance(IsAvailable()?AVAILABLE:DISABLED);
        }
    }
}

void Tool::ExtOnCreate() {
    free_tool = orxConfig_GetBool("FreeTool");
	icon = CreateChild(orxConfig_GetString("Icon"), true);
	if(!free_tool) token_text = CreateChild(orxConfig_GetString("TokenText"), true);
	SetAppearance(IsAvailable()?AVAILABLE:DISABLED);
	if(bonus) ShowBonusTicks(bonus);
}

void Tool::SetAppearance(Appearance appearance) {
    RemoveShader("GrayscaleShader");
    this->appearance = appearance;
	switch (appearance) {
	case SELECTED:
		if(halo) orxObject_SetLifeTime(halo, 0);
        halo = CreateChild("ToolCountdownHalo", true);
        orxObjectX_SetCurrentAnimByTag(icon, "Selected");
        break;
	case AVAILABLE:
		if(halo) orxObject_SetLifeTime(halo, 0);
		halo = nullptr;
        orxObjectX_SetCurrentAnimByTag(icon, "Unselected");
        break;
	case DISABLED:
		if(halo) orxObject_SetLifeTime(halo, 0);
		halo = nullptr;
        orxObjectX_SetCurrentAnimByTag(icon, "Disabled");
        AddShader("GrayscaleShader");
        break;
	case COUNTINGDOWN:
      	orxObjectX_SetCurrentAnimByTag(icon, "Disabled");
        if(halo) orxObject_SetLifeTime(halo,0);
        halo = CreateChild("ToolCooldownHalo", true);
        AddShader("GrayscaleShader");
        break;
	}
	if(token_text) {
	    auto item_count = getInventory(GetModelName());
	    if(item_count != 0) orxObject_SetTextString(token_text, ("x" + to_string(item_count)).c_str());
	    else orxObject_SetTextString(token_text, "");
	}
}

bool Tool::IsAvailable() {
    if(free_tool) return true;
    if(!cooling_down) {
        if(bonus > 0) return true;
        if(getInventory(GetModelName()) > 0) return true;
    }
    return false;
}

void Tool::ShowBonusTicks(orxU32 count) {
    if(bonus_ticks) orxObject_SetLifeTime(bonus_ticks, 0);
    bonus_ticks = CreateChild("BonusToolTickCollection");
    for(orxU32 i = 0; i<count; i++) {
        auto new_tick = ::CreateChild(bonus_ticks, "BonusToolTick");
        orxObject_SetRotation(new_tick, (0.5 - i) * orxMATH_KF_PI/4);
    }
}

void Tool::AddBonus(orxU32 count) {
    bonus += count;
    ShowBonusTicks(bonus);
    if(appearance == DISABLED && IsAvailable()) SetAppearance(AVAILABLE);
}
