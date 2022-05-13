package com.mategames.featherytale;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;

import platform.Platform;

import android.provider.Settings.Secure;
import android.util.Log;

import platform.PlatformModule;
import platform.item_info;
import platform.item_info_vector;
import platform.item_inventory_info;
import platform.item_inventory_info_vector;
import platform.platform_info;
import platform.share_dialog_info;
import util.IabHelper;
import util.IabResult;
import util.Inventory;
import util.Purchase;

import com.facebook.CallbackManager;
import com.facebook.FacebookCallback;
import com.facebook.FacebookException;
import com.facebook.share.Sharer;
import com.facebook.share.model.ShareLinkContent;
import com.facebook.share.model.SharePhotoContent;
import com.facebook.share.widget.ShareDialog;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.InterstitialAd;

import java.util.LinkedList;
import java.util.List;
import java.util.Locale;

public class AndroidPlatform extends Platform {
    private Activity context;
    InterstitialAd mInterstitialAd;
    volatile boolean adFree = true;
    public CallbackManager callbackManager;
    ShareDialog shareDialog;

    public AndroidPlatform(MainActivity context) {
        this.context = context;
        mInterstitialAd = new InterstitialAd(context);
        mInterstitialAd.setAdUnitId(context.getString(R.string.level_start_ad));
        mInterstitialAd.loadAd(buildAdRequest());
        mInterstitialAd.setAdListener(new AdListener() {
            @Override
            public void onAdClosed() {
                mInterstitialAd.loadAd(buildAdRequest());
                PlatformModule.SetAdRequestResult(true);
            }
        });
        callbackManager = CallbackManager.Factory.create();
        shareDialog = new ShareDialog(context);
        shareDialog.registerCallback(callbackManager, new FacebookCallback<Sharer.Result>() {

            @Override
            public void onSuccess(Sharer.Result result) {
                PlatformModule.reportFacebookShareResult(true);
            }

            @Override
            public void onCancel() {
                PlatformModule.reportFacebookShareResult(false);
            }

            @Override
            public void onError(FacebookException error) {

            }
        });

    }

    public String getUserID() {
        return Secure.getString(context.getContentResolver(), Secure.ANDROID_ID);
    }

    public String getPlatformName() {
        return "android";
    }

    public String getDeviceName() {
        return Build.MODEL;
    }

    public String getDeviceManufacturer() {
        return Build.MANUFACTURER;
    }

    public String getOSVersion() {
        return getPlatformName() + " " + Build.VERSION.RELEASE;
    }

    public platform_info getPlatformInfo() {
        platform_info result = new platform_info();
        result.setUser_id(getUserID());
        result.setPlatform_name(getPlatformName());
        result.setDevice_name(getDeviceName());
        result.setManufacturer(getDeviceManufacturer());
        result.setOs_version(getOSVersion());
        result.setDevice_language(Locale.getDefault().getLanguage());
        return result;
    }

    public void showAd() {
        context.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if(mInterstitialAd.isLoaded()) {
                    mInterstitialAd.show();
                } else {
                    PlatformModule.SetAdRequestResult(false);
                }
            }
        });
    }

    public boolean isAdFree() {
        return adFree;
    }

    public void purchaseAdFree() {
        context.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if(mHelper == null) {
                    unableToCarryOutPurchase();
                    return;
                }
                Log.d("Billing", "Start purchasing \"adfree\"");
                mHelper.launchPurchaseFlow(context, "adfree",
                        MainActivity.PURCHASE_REQUEST_CODE,
                        new IabHelper.OnIabPurchaseFinishedListener() {
                            @Override
                            public void onIabPurchaseFinished(IabResult result, Purchase info) {
                                Log.d("Billing", "Purchasing success: " + result);
                                Log.d("Billing", "Purchasing status: " + info);
                                if (result.isSuccess()) {
                                    if (info.getSku().equals("adfree")) {
                                        adFree = true;
                                        PlatformModule.adFreePurchased();
                                    }
                                }
                            }
                        });
            }
        });
    }

    void unableToCarryOutPurchase() {
        // TODO Tell the user that no connection with the play store could be established
    }

    public void purchaseConsumable(item_info item_) {
        final item_info item = new item_info(item_);
        context.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if(mHelper == null) {
                    unableToCarryOutPurchase();
                    return;
                }
                Log.d("Billing", "Start purchasing \"" + item.getItem_name() + "\"");
                mHelper.launchPurchaseFlow(context, item.getItem_name(),
                        MainActivity.PURCHASE_REQUEST_CODE,
                        new IabHelper.OnIabPurchaseFinishedListener() {
                            @Override
                            public void onIabPurchaseFinished(IabResult result, Purchase purchase) {
                                Log.d("Billing", "Purchasing success: " + result);
                                Log.d("Billing", "Purchasing status: " + purchase);
                                if (result.isSuccess()) {
                                    consumePurchase(purchase, item);
                                }
                            }
                        });
            }
        });
    }

    void consumePurchase(Purchase purchase, final item_info item) {
        if (purchase.getSku().equals(item.getItem_name())) {
            mHelper.consumeAsync(purchase, new IabHelper.OnConsumeFinishedListener() {
                @Override
                public void onConsumeFinished(Purchase purchase, IabResult result) {
                    if(result.isSuccess()) {
                        PlatformModule.consumablePurchased(item);
                    }
                }
            });
        }
    }

    volatile IabHelper mHelper;
    volatile LinkedList<item_info> query_items;

    public void reportInventory(Inventory inventory) {
        adFree = inventory.hasPurchase("adfree");

        item_inventory_info_vector inventory_info = new item_inventory_info_vector();
        for(item_info item: query_items) {
            String item_name = item.getItem_name();
            item_inventory_info item_info = new item_inventory_info();
            item_info.setItem_name(item_name);
            item_info.setPrice(inventory.getSkuDetails(item_name).getPrice());
            inventory_info.add(item_info);

            if(item.getConsumable()) {
                Purchase purchase = inventory.getPurchase(item_name);
                if(purchase != null) consumePurchase(purchase, item);
            }
        }
        PlatformModule.reportInventory(inventory_info);

    }

    public synchronized void queryInventory(item_info_vector items) {
        query_items = new LinkedList<>();
        for(int i = 0; i<items.size(); i++) {
            query_items.add(new item_info(items.get(i)));
        }
        if(mHelper != null) {
            context.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    performInventoryQuery(query_items);
                }
            });
        }
    }

    public synchronized void IABSetupSuccessful(IabHelper _Helper) {
        mHelper = _Helper;
        if(query_items != null) {
            performInventoryQuery(query_items);
        }
    }

    void performInventoryQuery(List<item_info> items) {
        List<String> item_names = new LinkedList<>();
        for(item_info item:items) item_names.add(item.getItem_name());
        // IAB is fully set up. Now, let's get an inventory of stuff we own.
        Log.d("Billing", "Setup successful. Querying inventory.");
        mHelper.queryInventoryAsync(true, item_names, new IabHelper.QueryInventoryFinishedListener() {
            @Override
            public void onQueryInventoryFinished(IabResult result, Inventory inv) {

                Log.d("Billing", "Query result: " + result.isSuccess());

                // Have we been disposed of in the meantime? If so, quit.
                if (mHelper == null) return;
                if(result.isSuccess()) {
                    Log.d("Billing", "Is ad free? " + inv.hasPurchase("adfree"));
                    reportInventory(inv);
                }

            }
        });
    }

    public void showStorePage() {
        context.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                context.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=" + context.getPackageName())));
            }
        });
    }

    public void showShareDialog() {
        context.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (ShareDialog.canShow(ShareLinkContent.class)) {
                    ShareLinkContent linkContent = new ShareLinkContent.Builder()
                            .setContentTitle("Feathery Tale")
                            .setContentDescription(
                                    "Feathery Tale'de şirin serçelerin yardımına ihtiyaçları var!")
                            .setContentUrl(Uri.parse("https://play.google.com/store/apps/details?id=com.mategames.featherytale"))
                            .build();

                    shareDialog.show(linkContent);
                }
            }
        });
    }

    public void showShareDialog(share_dialog_info info) {
        final String description = info.getDescription();
        context.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (ShareDialog.canShow(ShareLinkContent.class)) {
                    ShareLinkContent linkContent = new ShareLinkContent.Builder()
                            .setContentTitle("Feathery Tale")
                            .setContentDescription(description)
                            .setContentUrl(Uri.parse("https://play.google.com/store/apps/details?id=com.mategames.featherytale"))
                            .build();

                    shareDialog.show(linkContent);
                }
            }
        });
    }

    AdRequest buildAdRequest() {
        return new AdRequest.Builder().build();
    }
}

