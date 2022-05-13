/*
 * platform.h
 *
 *  Created on: Feb 26, 2016
 *      Author: eba
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <string>
#include <vector>

struct platform_info {
    std::string user_id;
    std::string platform_name;
    std::string device_name;
    std::string manufacturer;
    std::string os_version;
    std::string device_language;
};

struct item_info {
    std::string item_name;
    bool consumable;
};

struct item_inventory_info {
    std::string item_name;
    std::string price;
};

struct share_dialog_info {
    std::string image_path;
    std::string description;
};

class Platform {
public:
  Platform(){};
  virtual ~Platform(){};
  virtual platform_info getPlatformInfo() = 0;
  virtual void showAd() = 0;
  virtual bool isAdFree() = 0;
  virtual void purchaseAdFree() = 0;
  virtual void purchaseConsumable(item_info item) = 0;
  virtual void queryInventory(std::vector<item_info> items) = 0;
  virtual void showStorePage() = 0;
  virtual void showShareDialog() = 0;
  virtual void showShareDialog(share_dialog_info info) = 0;
};

void SetPlatform(Platform *);
Platform * GetPlatform();

void SetAdRequestResult(bool shown);

void adFreePurchased();
void consumablePurchased(item_info item);

void reportInventory(std::vector<item_inventory_info>);

void reportFacebookShareResult(bool isShared);

#endif /* PLATFORM_H_ */
