/*
 * monetization.cpp
 *
 *  Created on: Mar 19, 2016
 *      Author: eba
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <memory>

#include <orx.h>

#include <scroll_ext/config_utils.hpp>
#include <monetization.h>
#include <analytics.h>
#include <game_state.h>


enum AdRequestStatus {NO_REQUEST, ONGOING_REQUEST, REQUEST_SUCCESSFUL, REQUEST_FAILED};
std::atomic<AdRequestStatus> ad_request_status;
orxU64 last_attempt;
orxU64 last_display;

struct monetization_state {
    std::atomic<bool> something_new;
    std::mutex mut;
    std::vector<item_info> purchased_items;
    std::vector<item_inventory_info> inventory_info;
    bool facebook_share = false;
};
std::unique_ptr<monetization_state> shared_monetization_state;

void SetAdRequestResult(bool shown) {
    ad_request_status = shown ? REQUEST_SUCCESSFUL : REQUEST_FAILED;
    if(shown) design_event({"Monetization", "AdDisplayed"});
}

void adFreePurchased() {
    // TODO send an analytics event here
}

void consumablePurchased(item_info item) {
    std::lock_guard<std::mutex> guard(shared_monetization_state->mut);
    shared_monetization_state->something_new = true;
    shared_monetization_state->purchased_items.push_back(item);
}

void reportInventory(std::vector<item_inventory_info> inventory) {
    std::lock_guard<std::mutex> guard(shared_monetization_state->mut);
    shared_monetization_state->something_new = true;
    for(auto & item: inventory) shared_monetization_state->inventory_info.push_back(item);
}

void init_monetization() {
    shared_monetization_state.reset(new monetization_state);
    ConfigSectionGuard guard("Monetization");
    ad_request_status = NO_REQUEST;
    last_attempt = 0;
    last_display = orxSystem_GetRealTime();
    std::vector<item_info> items;
    for(auto option: ConfigListRange<const char *>("PurchasableItems")) {
        bool consumable = GetValue<bool>(option, "Consumable");
        items.push_back({option, consumable});
    }
    GetPlatform()->queryInventory(items);
}

void apply_inventory_info() {
    std::vector<item_inventory_info> inventory;
    {
        std::lock_guard<std::mutex> guard(shared_monetization_state->mut);
        std::swap(inventory, shared_monetization_state->inventory_info);
    }

    for(auto & item: inventory) {
        ConfigSectionGuard guard(item.item_name.c_str());
        orxConfig_SetString("Price", item.price.c_str());
    }
}

void apply_purchased_items() {
    std::vector<item_info> purchased_items;
    {
        std::lock_guard<std::mutex> guard(shared_monetization_state->mut);
        std::swap(purchased_items, shared_monetization_state->purchased_items);
    }
    for(auto item: purchased_items) {
        auto amount = GetValue<orxS32>(item.item_name.c_str(), "Amount");
        setInventory("Coin", getInventory("Coin")+amount);
        orxObject_CreateFromConfig("PurchaseCoinSound");
        resource_event(R_SOURCE, amount, "Coin", "BuyCoin", item.item_name.c_str());
    }
}

void apply_facebook_share() {
    bool is_shared = false;
    {
        std::lock_guard<std::mutex> guard(shared_monetization_state->mut);
        std::swap(is_shared, shared_monetization_state->facebook_share);
    }
    if(is_shared) {
        orxFLOAT today = orxSystem_GetRealTime()/(60*60*24);
        auto guard = fetch_command_guard("ShareAward");
        if(today>guard) {
            auto award = GetValue<orxU32>("Monetization", "ShareOnFacebookAward");
            auto period = GetValue<orxU32>("Monetization", "ShareAwardPeriod");
            earnResource("Coin", award, "FacebookShare", "Coin");
            persist_command_guard("ShareAward", today+period);
            orxObject_CreateFromConfig("PurchaseCoinSound");
        }
    }
}

void update_monetization() {
    AdRequestStatus curstatus = ad_request_status;
    if(curstatus == REQUEST_SUCCESSFUL) {
        last_display = orxSystem_GetRealTime();
        ad_request_status = NO_REQUEST;
    }
    if(shared_monetization_state->something_new) {
        shared_monetization_state->something_new = false;
        apply_purchased_items();
        apply_inventory_info();
        apply_facebook_share();
    }
}

void exit_monetization() {
}

bool RequestTransition(const char * level_name) {
    if(GetPlatform()->isAdFree()) return true;
    if(ad_request_status == ONGOING_REQUEST) return false;
    auto level_type = GetValue<const char *>(level_name, "ScreenType");
    if(strcmp(level_type, "Level") == 0) {
        auto curtime = orxSystem_GetRealTime();
        if(curtime == last_attempt) return true;
        if(curtime - last_display < GetValue<orxU32>("Monetization", "MinSecsBetweenDisplays")) return true;
        ad_request_status = ONGOING_REQUEST;
        GetPlatform()->showAd();
        last_attempt = curtime;
        return false;
    } else return true;
}

void purchaseAdFree() {
    GetPlatform()->purchaseAdFree();
}

void purchaseConsumable(const char* item_name) {
    GetPlatform()->purchaseConsumable(item_info{item_name});
}

void reportFacebookShareResult(bool isShared) {
    if(isShared) {
        design_event({"Monetization", "Facebook", "ShareSuccessful"});
        std::lock_guard<std::mutex> guard(shared_monetization_state->mut);
        shared_monetization_state->something_new = true;
        shared_monetization_state->facebook_share = true;
    }
}
