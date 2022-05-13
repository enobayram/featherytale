/*
 * PushButton.cpp
 *
 *  Created on: Dec 8, 2015
 *      Author: enis
 */

#include <memory>

#include <analytics.h>
#include <orx_utilities.h>
#include <game_state.h>
#include <gameObjects/PushButton.h>
#include <gameObjects/SmartText.h>
#include <util/orx_pointers.hpp>
#include <util/iteration_utils.hpp>
#include <gestures.h>

bool IsInActiveContext(orxOBJECT * obj) {
    orxOBJECT * input_context = nullptr;
    for(orxOBJECT * owner = GetOwner(obj); owner; owner = GetOwner(owner)) {
        if(GetValue<bool>(orxObject_GetName(owner), "IC")) {
            input_context = owner;
            break;
        }
    }
    if(!input_context) return false;
    bool result = true;
    iterate_object_tree(input_context, [&](orxOBJECT * child) {
        if(child == input_context) return;
        if(GetValue<bool>(orxObject_GetName(child), "IC")) result = false;
    });
    return result;
}

struct PushButtonGesture: gesture {
	weak_object_ptr button;
	trail & t;
	std::unique_ptr<gesture> internal_gesture;
	double executeAfter;
	PushButtonGesture(PushButton * pButton, trail & t, orxFLOAT delay):t(t) {
		button = pButton->GetOrxObject();
		executeAfter = t.traces.back().t + delay;
	}
    orxSTATUS time_update(double time) {
    	if(internal_gesture) return internal_gesture->time_update(time);
    	PushButton * pButton = scroll_cast<PushButton>(button);
    	if(!pButton || pButton->disabled) return orxSTATUS_FAILURE;
    	trace last = t.traces.back();
    	orxVECTOR lastV{float(last.x), float(last.y), 0};
    	orxOBOX obox;
    	orxObject_GetBoundingBox(pButton->GetOrxObject(), &obox);
    	if(orxOBox_2DIsInside(&obox, &lastV)) {
    		if(!t.active) {
    			if(time<executeAfter) return orxSTATUS_SUCCESS;
    			if(!IsInActiveContext(pButton->GetOrxObject())) {
    	            pButton->SetTaggedAnim("Unpressed");
    			    return orxSTATUS_FAILURE;
    			}
    			internal_gesture.reset(pButton->Button::clicked(t));
    			pButton->SetTaggedAnim("Unpressed");
    			if(pButton->GetValue<bool>("PlayActivateSound")) {
    			    CreateSoundObject(pButton->GetValue<const char *>("ActivateSound"), pButton->GetPosition());
    			}
    			if(internal_gesture) internal_gesture->time_update(time);
    			else return orxSTATUS_FAILURE;
    		} else {
        		pButton->SetTaggedAnim("Pressed");
    		}
    	} else {
			pButton->SetTaggedAnim("Unpressed");
        	if(!t.active) return orxSTATUS_FAILURE;
    	}
    	return orxSTATUS_SUCCESS;
    }
};

void PushButton::ExtOnCreate() {
	SetTaggedAnim("Unpressed");
	if(orxConfig_HasValue("ActivationKey")) activation_key = orxConfig_GetString("ActivationKey");
}

gesture* PushButton::clicked(trail & t) {
    if(!IsPaused()) {
        return new PushButtonGesture(this, t, GetValue<orxFLOAT>("PressDelay"));
    } else return nullptr;
}

void PushButton::set_disabled(bool disabled) {
    if(disabled != this->disabled) {
        if(disabled) {
            SetTaggedAnim("Disabled");
        } else {
            SetTaggedAnim("Unpressed");
        }
    }
    Button::set_disabled(disabled);
}

void PushButton::Update(const orxCLOCK_INFO&) {
    if(activation_key) {
        ConfigSectionGuard guard("Runtime/SK"); // Swallowed keys
        if(orxInput_IsActive(activation_key)) {
            if(!orxConfig_GetBool(activation_key) && IsInActiveContext(GetOrxObject())) {
                handler(nullptr);
                orxConfig_SetBool(activation_key, true);
            }
        } else {
            orxConfig_SetBool(activation_key, false);
        }
    }
}

void PushButton::OnNewAnim(const orxSTRING _zOldAnim, const orxSTRING _zNewAnim, orxBOOL _bCut) {
    if(_zOldAnim == _zNewAnim) return;
    auto is_from_to = [&](const char * from, const char * to) {
        return strcmp(GetAnimName(from), _zNewAnim) == 0 && strcmp(GetAnimName(to), _zOldAnim) == 0;
    };
    if(is_from_to("Unpressed", "Pressed")) AddSound("ButtonPressSound");
    if(is_from_to("Pressed", "Unpressed")) AddSound("ButtonCancelSound");
}

void CreateLockPad(ExtendedMonomorphic * owner, std::function<void()> unlock_action) {
    ConfigSectionGuard guard(owner->GetModelName());
    const char * unlock_token = orxConfig_GetString("UnlockToken");
    if(isButtonUnlocked(unlock_token)) unlock_action();
    else {
        bool should_unlock = false;
        if(orxConfig_HasValue("UnlockCondition")) {
            auto unlock_condition = orxConfig_GetString("UnlockCondition");
            orxCOMMAND_VAR result;
            orxCommand_Evaluate(unlock_condition, &result);
            orxASSERT(result.eType == orxCOMMAND_VAR_TYPE_BOOL);
            should_unlock = result.bValue;
        }
        if(should_unlock) {
            setButtonUnlocked(unlock_token, true);
            if(orxConfig_HasValue("UnlockTrack")) owner->AddTrack(orxConfig_GetString("UnlockTrack"));
            unlock_action();
        } else {
            auto unlock_button = owner->CreateChild<PushButton>(orxConfig_GetString("LockButton"));
            auto price = orxConfig_GetS32("UnlockCost");
            const char * dialog_section = orxConfig_GetString("Dialog");
            unlock_button->set_handler([=]{
                auto curcoins = getInventory("Coin");
                auto unlock_dialog = owner->CreateChild<ExtendedMonomorphic>(dialog_section, false, false);
                auto accept_button = unlock_dialog->FindOwnedChild<PushButton>("AcceptUnlockButton");
                auto reject_button = unlock_dialog->FindOwnedChild<PushButton>("RejectUnlockButton");
                auto coin_display  = unlock_dialog->FindOwnedChild("UnlockDialogCoinDisplay");
                auto price_display = unlock_dialog->FindOwnedChild("UnlockDialogPriceDisplay");
                auto description_text = unlock_dialog->FindOwnedChild<SmartText>("UnlockDescription");
                reject_button->set_handler([unlock_dialog]{
                    unlock_dialog->SetLifeTime(0);
                });
                orxObject_SetTextString(coin_display, to_string(curcoins).c_str());
                orxObject_SetTextString(price_display, to_string(price).c_str());
                if(curcoins >= price) {
                    accept_button->set_handler([=]{
                        setButtonUnlocked(unlock_token, true, price);
                        resource_event(R_SINK, price, "Coin", "UnlockButton", unlock_token);
                        unlock_dialog->SetLifeTime(0);
                        unlock_button->SetLifeTime(0);
                        unlock_action();
                    });
                } else accept_button->set_disabled(true);
                description_text->SetText(Localize(owner->GetValue<const char *>("UnlockDescription")));
            });
        }
    }
}

class UnlockButton: public PushButton {
    SCROLL_BIND_CLASS("UnlockButton")
public:
    void ExtOnCreate() {
        PushButton::ExtOnCreate();
        set_disabled(true);
        CreateLockPad(this, [=]{set_disabled(false);});
    }
};

class CreationLock: public ExtendedMonomorphic {
    SCROLL_BIND_CLASS("CreationLock")
public:
    void SetContext(ExtendedMonomorphic & context) {
        auto created_object = GetValue<const char *>("CreatedObject");
        auto context_obj = context.GetOrxObject();
        CreateLockPad(this, [=]() {
            ::CreateChild(context_obj, created_object);
            SetGroupID(orxString_GetID("hidden"), true);
            SetLifeTime(0);
        });
    }
};
