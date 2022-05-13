package com.mategames.featherytale;

import android.content.Intent;
import android.util.Log;
import android.annotation.TargetApi;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnSystemUiVisibilityChangeListener;
import org.orx.lib.OrxActivity;

import platform.PlatformModule;

import util.IabHelper;
import util.IabResult;
import util.Inventory;
import com.facebook.FacebookSdk;
import com.facebook.appevents.AppEventsLogger;

public class MainActivity extends OrxActivity {
    private View mDecorView;
    public IabHelper mHelper;
    public static final int PURCHASE_REQUEST_CODE = 1;

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mHelper != null) mHelper.dispose();
        mHelper = null;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(requestCode == PURCHASE_REQUEST_CODE) mHelper.handleActivityResult(requestCode, resultCode, data);
        else platform.callbackManager.onActivityResult(requestCode, resultCode, data);
    }

    static {
        // load your native module here.
        System.loadLibrary("feathery_tale");
    }

    public AndroidPlatform platform;

    @TargetApi(Build.VERSION_CODES.KITKAT)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        FacebookSdk.sdkInitialize(getApplicationContext());
        AppEventsLogger.activateApp(getApplication());
        
        // call setContentView() here if you need a custom layout.
        // The custom layout needs to include a SurfaceView with @+id/orxSurfaceView.

        mHelper = new IabHelper(this,  getString(R.string.license_key));

        platform = new AndroidPlatform(this);

        mHelper.startSetup(new IabHelper.OnIabSetupFinishedListener() {
            public void onIabSetupFinished(IabResult result) {
                if (!result.isSuccess()) {
                    // Oh noes, there was a problem.
                    Log.d("Billing", "Problem setting up In-app Billing: " + result);
                }

                // Have we been disposed of in the meantime? If so, quit.
                if (mHelper == null) return;
                platform.IABSetupSuccessful(mHelper);
            }
        });

        PlatformModule.SetPlatform(platform);

        mDecorView = getWindow().getDecorView();

        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            mDecorView.setOnSystemUiVisibilityChangeListener(new OnSystemUiVisibilityChangeListener() {
				
				@Override
				public void onSystemUiVisibilityChange(int visibility) {
					if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
						mDecorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
		                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
		                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
		                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
		                        | View.SYSTEM_UI_FLAG_FULLSCREEN
		                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
					}
				}
			});
        }
    }

    @TargetApi(Build.VERSION_CODES.KITKAT)
    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
        	if (hasFocus) {
                mDecorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
            }
        }
    }

}

