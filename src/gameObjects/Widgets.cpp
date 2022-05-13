/*
 * Widgets.cpp
 *
 *  Created on: Nov 9, 2015
 *      Author: eba
 */

#include <vector>
#include <gameObjects/Widgets.h>
#include <game_state.h>
#include <scroll_ext/config_utils.hpp>
#include <util/orx_pointers.hpp>
#include <gameObjects/aspects.h>
#include <gameObjects/PushButton.h>
#include <orx_utilities.h>
#include <gestures.h>
#include <analytics.h>
#include <platform.h>
#include <pancarDemo.h>
#include <monetization.h>

void ReloadStateButtons::ExtOnCreate() {
    ConfigSectionGuard guard("DebugMetaData");
    orxFLOAT y = 0;
    for(auto state_file: ConfigListRange<const char *>("PredefinedStates")) {
        auto button = CreateChild<Button>("ReloadStateButton");
        button->set_handler([=]{
            reload_persistence((std::string("common/debug/")+state_file).c_str());
            pancarDemo::GetInstance().level->SetNextScreen("OpeningMenuScreen");
        });
        button->SetPosition({0,y,0});
        y+=button->GetSize().fY;
        auto text = button->CreateChild("OpeningMenuButtonText");
        orxObject_SetTextString(text, state_file);
    }
}

class SettingsDialog: public ExtendedMonomorphic {
public:
    SCROLL_BIND_CLASS("SettingsDialog")
    void ExtOnCreate() {
        CreateSoundButton(true);
        CreateMusicButton(true);
        auto langButton = CreateChild<Button>("LanguageButton");
        SetLanguageIndex(langButton, getLanguageIndex(), false);
    }

    void ApplyFadeIn(orxOBJECT * obj) {
        SetAlpha(obj, 0);
        orxObject_AddFX(obj, "SettingsDialogButtonFadeInFX");
    }

    void CreateSoundButton(bool withFadeIn) {
        auto button = CreateChild<Button>(isSoundOff() ? "SoundOffButton" : "SoundButton");
        button->set_handler([=]{
            ToggleSound();
            CreateSoundButton(false);
            button->SetLifeTime(0);
        });
        if(withFadeIn) ApplyFadeIn(button->GetOrxObject());
    }
    void CreateMusicButton(bool withFadeIn) {
        auto button = CreateChild<Button>(isMusicOff() ? "MusicOffButton" : "MusicButton");
        button->set_handler([=]{
            ToggleMusic();
            CreateMusicButton(false);
            button->SetLifeTime(0);
        });
        if(withFadeIn) ApplyFadeIn(button->GetOrxObject());
    }
    void ToggleSound() { setSoundOff(!isSoundOff());}
    void ToggleMusic() { setMusicOff(!isMusicOff());}
    void SetLanguageIndex(Button * langButton, int idx, bool save) {
        auto langText = orxObject_GetOwnedChild(langButton->GetOrxObject());

        ConfigSectionGuard guard("Locale");
        auto numLanguages = orxConfig_GetListCounter("LanguageList");
        if(idx >= numLanguages || idx < 0) idx = 0;
        orxObject_SetTextString(langText, orxConfig_GetListString("LanguageList", idx));
        if(save) setLanguageIndex(idx);
        langButton->set_handler([=] {
            SetLanguageIndex(langButton, idx+1, true);
        });
    }
};

class TabbedPane: public ExtendedMonomorphic {
    SCROLL_BIND_CLASS("TabbedPane")
    std::vector<Button *> tabButtons;
    orxOBJECT * content = nullptr;
    size_t current_tab = 0; // To avoid playing the tab sound when tab pressed.
public:
    void ExtOnCreate() {
    	auto tabCount = orxConfig_GetListCounter("Tabs");
    	for(int i=0; i<tabCount; ++i) {
    		auto tabButton = CreateChild<Button>(orxConfig_GetListString("Tabs", i));
    		tabButton->set_handler([=]{
    			SetTab(i);
    		});
    		tabButtons.push_back(tabButton);
    	}
    	SetTab(0);
    }
    void SetTab(size_t idx) {
    	if(content != nullptr) {
    		orxObject_SetLifeTime(content, 0);
    		content = nullptr;
    	}
    	for(size_t i=0; i<tabButtons.size(); ++i) {
    		auto tabButton = tabButtons[i];
    		if(i == idx) {
    			tabButton->AddTrack(tabButton->GetValue<const char *>("SelectTrack"));
    			tabButton->SetTaggedAnim("Selected");
    			content = CreateChild(tabButton->GetValue<const char *>("Content"));
    		} else {
    			tabButton->AddTrack(tabButton->GetValue<const char *>("UnselectTrack"));
    			tabButton->SetTaggedAnim("Unselected");
    		}
    	}
    	if(idx!=current_tab) AddSound(GetValue<const char *>("TabSound"));
    	current_tab = idx;
    }
};

class ScrollPane;

class SliderKnobGesture: public gesture {
	trail & t;
	weak_object_ptr pane;
	weak_object_ptr knob;
public:
	SliderKnobGesture(trail & t, ScrollPane * pane, orxOBJECT * knob);
    orxSTATUS time_update(double time);
    virtual int process_trace(ScrollPane * pane, orxOBJECT * knob);
    orxVECTOR get_last_pos(ScrollPane * pane);
};

class ScrollPaneContentGesture: public SliderKnobGesture {
    float referenceY;
public:
    ScrollPaneContentGesture(trail & t, ScrollPane * pane, orxOBJECT * knob):
        SliderKnobGesture(t,pane,knob), referenceY(get_last_pos(pane).fY) {}
    virtual int process_trace(ScrollPane * pane, orxOBJECT * knob);
};

class ScrollPane: public ExtendedMonomorphic {
	SCROLL_BIND_CLASS("ScrollPane");
	std::vector<orxOBJECT *> contents;
	orxOBJECT * sliderBar;
	orxOBJECT * sliderKnob;
	int cur_idx = -1;
	orxFLOAT itemX;
public:
    int num_of_items;
    int num_of_shown_items;
    orxFLOAT itemSpread;
	void ExtOnCreate() {
		sliderBar = CreateChild(orxConfig_GetString("SliderBar"));
		sliderKnob = CreateChild(orxConfig_GetString("SliderKnob"));
		auto sliderSurface = CreateChild<Button>(orxConfig_GetString("SliderSurface"));
        auto contentSurface = CreateChild<Button>(orxConfig_GetString("ContentSurface"));
		num_of_items = orxConfig_GetListCounter("Contents");
		num_of_shown_items = orxConfig_GetU32("NumOfShownItems");
		contents = std::vector<orxOBJECT *>(num_of_shown_items, nullptr);
		itemX = orxConfig_GetFloat("ItemX");
		itemSpread = orxConfig_GetFloat("ItemSpread");
		::SetPosition(sliderKnob, getKnobPosition(0));
		SetIndex(0);
		sliderSurface->set_handler([=](trail & t) {
			return new SliderKnobGesture(t,this, sliderKnob);
		});
        contentSurface->set_handler([=](trail & t) {
            return new ScrollPaneContentGesture(t,this, sliderKnob);
        });
	}
	int GetIndex() {return cur_idx;}
	void SetIndex(int idx) {
	    if(idx < 0) idx = 0;
	    if(idx > num_of_items-1) idx = num_of_items-1;
		if(idx == cur_idx) return;
		cur_idx = idx;
        ConfigSectionGuard guard(GetModelName());
		for(int i=0; i<num_of_shown_items; ++i) {
		    orxOBJECT *& content = contents[i];
		    if(content != nullptr) orxObject_SetLifeTime(content, 0);
		    int item_index = i+idx;
		    if(item_index < num_of_items) {
		        content = CreateChild(orxConfig_GetListString("Contents", item_index));
		        ::SetPosition(content, {itemX, getItemY(i), -0.01});
		    }
		    else content = nullptr;
		}
		AddSound(orxConfig_GetString("ScrollSound"));
	}

	orxFLOAT interpolate(orxFLOAT center, orxFLOAT length, int total, int idx) {
	    return center - length/2 + (idx+0.5f)*length/total;
	}

	orxFLOAT getItemY(int idx) {
        return interpolate(0, itemSpread, num_of_shown_items, idx);
	}

	orxFLOAT getSlideRange() {
	    return ::GetSize(sliderBar).fY - ::GetSize(sliderKnob).fY;
	}

	orxVECTOR getKnobPosition(int idx) {
		auto slideRange = getSlideRange();
		auto center = ::GetPosition(sliderBar);
		if(num_of_items <= 0) return center;
        auto top = center.fY - slideRange/2;
		idx = std::min(num_of_items-1, idx);
		idx = std::max(0,idx);
		if(idx==0) return {center.fX, top, center.fZ-0.01f};
		return {center.fX, top+idx*slideRange/(num_of_items-1), center.fZ-0.01f};
	}

	int getKnobIndex(const orxVECTOR & pos) {
	    if(num_of_items < 2) return 0;
		auto slideRange = getSlideRange();
		auto center = ::GetPosition(sliderBar);
		auto top = center.fY - slideRange/2;
		auto increment = slideRange/(num_of_items-1);
		int idx = (pos.fY - top)/increment + 0.5;
		if(idx<0) return 0;
		if(idx>=num_of_items) return num_of_items-1;
		return idx;
	}

	orxVECTOR getKnobPositionForY(orxFLOAT y) {
	    auto slideRange = getSlideRange();
		auto center = ::GetPosition(sliderBar);
		auto min = center.fY - slideRange/2;
		auto max = center.fY + slideRange/2;
		if(y < min) return {center.fX, min, center.fZ-0.01f};
		if(y > max) return {center.fX, max, center.fZ-0.01f};
		return {center.fX, y, center.fZ-0.01f};
	}
};

SliderKnobGesture::SliderKnobGesture(trail & t, ScrollPane * pane, orxOBJECT * knob):
        t(t), pane(pane->GetOrxObject()), knob(knob){}

orxVECTOR SliderKnobGesture::get_last_pos(ScrollPane * pane) {
    auto last_trace = t.traces.back();
    return toObjectFrame(pane->GetOrxObject(), {(float)last_trace.x,(float)last_trace.y, 0});
}

orxSTATUS SliderKnobGesture::time_update(double time) {
	orxOBJECT * knob = this->knob;
	auto pane = scroll_cast<ScrollPane>(this->pane, false);
	if(!knob || !pane) return orxSTATUS_FAILURE;
	auto idx = process_trace(pane, knob);
	if(!t.active) {
		SetPosition(knob, pane->getKnobPosition(idx));
		return orxSTATUS_FAILURE;
	} else {
		pane->SetIndex(idx);
		return orxSTATUS_SUCCESS;
	}
}

int SliderKnobGesture::process_trace(ScrollPane * pane, orxOBJECT * knob) {
    orxVECTOR pos = get_last_pos(pane);
    SetPosition(knob, pane->getKnobPositionForY(pos.fY));
    return pane->getKnobIndex(pos);
}

int ScrollPaneContentGesture::process_trace(ScrollPane * pane, orxOBJECT * knob) {
    orxVECTOR pos = get_last_pos(pane);
    orxFLOAT increment = pane->itemSpread/pane->num_of_items;
    int index_difference = -(pos.fY-referenceY) / increment;
    int result = pane->GetIndex()+index_difference;
    if(index_difference != 0) {
        referenceY = pos.fY;
        SetPosition(knob, pane->getKnobPosition(result));
    }
    return result;
}


class ShopRowItem: public ExtendedMonomorphic {
    SCROLL_BIND_CLASS("ShopRowItem")
public:
    Button * buyButton;
    void ExtOnCreate() {
        buyButton = CreateChild<Button>(orxConfig_GetString("BuyButton"));
        auto thumbnail = CreateChild(orxConfig_GetString("ItemThumbnail"));
        ::SetPosition(thumbnail, GetValue<orxVECTOR>("ThumbnailPosition", false));
        ::SetScale(thumbnail, GetValue<orxVECTOR>("ThumbnailScale", false));
        auto description = CreateChild(orxConfig_GetString("DescriptionText"));
        orxObject_SetTextString(description, LocaleAwareString("ItemDescription"));
    }
};

class BuyInventoryRowItem: public ShopRowItem {
    SCROLL_BIND_CLASS("BuyInventoryRowItem")
    orxOBJECT * amountDisplay;
    const char * item_name;
    orxS32 price;
public:
    void ExtOnCreate() {
        ShopRowItem::ExtOnCreate();
        item_name = orxConfig_GetString("ItemName");
        amountDisplay = CreateChild(orxConfig_GetString("AmountDisplay"));
        auto priceDisplay = CreateChild(orxConfig_GetString("PriceDisplay"));
        orxObject_SetTextString(priceDisplay, orxConfig_GetString("Price"));
        price = orxConfig_GetS32("Price");
        auto itemType = orxConfig_GetString("ItemType");
        buyButton->set_handler([=]{
        	if(getInventory("Coin") < price) return;
            purchase(item_name, 1, price);
            AddSound(GetValue<const char *>("BuySound"));
            UpdateAmountDisplay();
            resource_event(R_SINK, price, "Coin", itemType, item_name);
            resource_event(R_SOURCE, 1, item_name, "Purchase", "Shop");
        });
        UpdateAmountDisplay();
        UpdateBuyButtonState();
    }
    void UpdateAmountDisplay() {
        char buf[13];
        orxString_NPrint(buf, sizeof(buf), "x%d", getInventory(item_name));
        orxObject_SetTextString(amountDisplay, buf);
    }
    void Update(const orxCLOCK_INFO &_rstInfo) {
    	UpdateBuyButtonState();
    }
    void UpdateBuyButtonState() {
    	if(getInventory("Coin") < price) buyButton->set_disabled(true);
    	else buyButton->set_disabled(false);
    }
};

class BuyCoinRowItem: public ShopRowItem {
    SCROLL_BIND_CLASS("BuyCoinRowItem")
public:
    void ExtOnCreate() {
        ShopRowItem::ExtOnCreate();
        auto item_name = orxConfig_GetString("ItemName");
        buyButton->set_handler([=]{
            purchaseConsumable(item_name);
        });
        auto price_text = CreateChild(orxConfig_GetString("PriceText"));
        orxObject_SetTextString(price_text, ::GetValue<const char *>(item_name, "Price"));
    }
};

class FreeCoinsItem: public ExtendedMonomorphic {
    SCROLL_BIND_CLASS("FreeCoinsItem")
public:
    void ExtOnCreate() {
        auto button = CreateChild(orxConfig_GetString("ActionButton"));
        auto buttonText = orxObject_GetOwnedChild(button);
        orxObject_SetTextString(buttonText, LocaleAwareString("ActionText"));
        auto thumbnail = CreateChild(orxConfig_GetString("ItemThumbnail"));
        ::SetPosition(thumbnail, GetValue<orxVECTOR>("ThumbnailPosition", false));
        auto description = CreateChild(orxConfig_GetString("DescriptionText"));
        orxObject_SetTextString(description, LocaleAwareString("ItemDescription"));
    }
};

class CoinsDisplay: public ExtendedMonomorphic {
    SCROLL_BIND_CLASS("CoinsDisplay");
public:
    void ExtOnCreate() {UpdateDisplay();}
    void Update(const orxCLOCK_INFO &_rstInfo) { UpdateDisplay();}
    void UpdateDisplay() {
        char buf[12];
        int coins = getInventory("Coin");
        orxString_NPrint(buf, sizeof(buf), "%d", coins);
        orxObject_SetTextString(GetOrxObject(), buf);
    }
};

class AdFreePurchaseButton: public PushButton {
    SCROLL_BIND_CLASS("AdFreePurchaseButton");
public:
    AdFreePurchaseButton() {
        set_handler([]{
            purchaseAdFree();
        });
    }
    void Update(const orxCLOCK_INFO &) {
        set_disabled(GetPlatform()->isAdFree());
    }
};

orxSTATUS orxFASTCALL soundFilterHandler(const orxEVENT *_pstEvent);

class AudioFilter;

struct shared_filter_state_t {
    std::mutex mut;
    std::map<const char *, std::vector<AudioFilter *>> filter_map;
};
std::unique_ptr<shared_filter_state_t> shared_filter_state;

REGISTER_INITIALIZER([]{
    shared_filter_state.reset(new shared_filter_state_t);
    orxEvent_AddHandler(orxEVENT_TYPE_SOUND, soundFilterHandler);
})

class AudioFilter: public ExtendedMonomorphic {
    SCROLL_BIND_CLASS("AudioFilter")
    const char * sound_name = nullptr;
    std::vector<float> filter_state;
    float time_const = 1;
    float multiplier = 1;
public:
    void ExtOnCreate() {
        sound_name = orxConfig_GetString("TargetSound");
        time_const = orxConfig_GetFloat("TimeConstant");
        multiplier = orxConfig_GetFloat("Multiplier");
        std::lock_guard<std::mutex> lg(shared_filter_state->mut);
        shared_filter_state->filter_map[sound_name].push_back(this);
    }
    void OnDelete() {
        std::lock_guard<std::mutex> lg(shared_filter_state->mut);
        auto & filters = shared_filter_state->filter_map[sound_name];
        filters.erase(std::remove(filters.begin(), filters.end(), this), filters.end());
    }
    void processStream(orxSOUND_STREAM_INFO & stInfo, orxSOUND_STREAM_PACKET & packet) {
        auto chan_num = stInfo.u32ChannelNumber;
        size_t cur_channel = 0;
        filter_state.resize(chan_num);
        for(size_t i = 0; i<packet.u32SampleNumber; ++i) {
            auto & sample = packet.as16SampleList[i];
            sample = filterSample(cur_channel, stInfo.u32SampleRate, sample);
            cur_channel = (cur_channel+1) % chan_num;
        }
    }
    orxS16 filterSample(orxU32 channel, orxU32 sample_rate, orxS16 sample) {
        auto coeff = std::exp(-time_const * sample_rate);
        auto & state = filter_state[channel];
        state += (sample-state) * coeff;
        auto result = state * multiplier;
        const float maxval = (1 << 15) - 1;
        if(result > maxval) return maxval;
        if(result < -maxval) return -maxval;
        return result;
    }
};


orxSTATUS orxFASTCALL soundFilterHandler(const orxEVENT *_pstEvent) {
    auto payload = (orxSOUND_EVENT_PAYLOAD *) _pstEvent->pstPayload;
    if(_pstEvent->eID == orxSOUND_EVENT_PACKET) {
        auto & stream = payload->stStream;
        std::lock_guard<std::mutex> lg(shared_filter_state->mut);
        for(auto & filter: shared_filter_state->filter_map[stream.zSoundName]) {
            filter->processStream(stream.stInfo, stream.stPacket);
        }
    }
    return orxSTATUS_SUCCESS;
}
