/*
 * Copyright (c) 2011, Sony Ericsson Mobile Communications AB.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sony Ericsson Mobile Communications AB nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package com.miadzin.shmup;

import android.app.NativeActivity;
import android.content.res.AssetManager;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.FileOutputStream;
import android.os.Environment;
import android.os.Bundle;
import android.util.Log;

public class TouchpadNAActivity extends NativeActivity {
    public static AssetManager assetManager;

    public boolean OnNativeMotion( int action, int x, int y, int source, int device_id ) {
    	Log.i("TouchpadNAJava", "Received native motion event! (" + x + ", " + y + ")");
    	return true;
    }

    /*
     * Add more callbacks if needed (keys, sensors etc)
     * 
     */

    static {
		System.loadLibrary("openal");
        System.loadLibrary("shmup");
    }

    @Override
    protected void onCreate (Bundle savedInstanceState) {
        String apkFilePath = null;
        ApplicationInfo appInfo = null;
        PackageManager packMgmr = getApplicationContext().getPackageManager();
        try {
            appInfo = packMgmr.getApplicationInfo("com.miadzin.shmup", 0);
        } 
        catch (NameNotFoundException e) {
            e.printStackTrace();
            throw new RuntimeException("Unable to locate assets, aborting...");
        }
        apkFilePath = appInfo.sourceDir;
        
        assetManager = getAssets();

        String sdCardPath = Environment.getExternalStorageDirectory() + "/app-data/com.miadzin.shmup/";
        createAssetManager(assetManager, apkFilePath, sdCardPath);

        try {
        File sdCardDir = new File(sdCardPath + "assets/data/cameraPath");
        if (!sdCardDir.exists()) {
            sdCardDir.mkdirs();
            new File(sdCardDir, ".nomedia").createNewFile();
        }
        } catch (IOException e) {
            Log.e("Shmup: Creating SD Card dir", e.getMessage());
        }

        AssetManager assetManager = getAssets();
        String[] files = null;
        try {
            files = assetManager.list("data/cameraPath");
        } catch (IOException e) {
            Log.e("Shmup: Listing Error", e.getMessage());
        }

        for(String filename : files) {
            final String dirPlusFilename = "data/cameraPath/" + filename;
            try {
              File assetFile = new File(sdCardPath, dirPlusFilename);

              InputStream in = assetManager.open(dirPlusFilename);

              FileOutputStream f = new FileOutputStream(assetFile); 
                byte[] buffer = new byte[1024];
             int len = 0;
            while ((len = in.read(buffer)) > 0) {
                f.write(buffer, 0, len);
            }
            f.close();
            } catch(Exception e) {
                Log.e("Shmup: Moving Error", e.getMessage());
            }       
        }

        super.onCreate(savedInstanceState);
    }
    
    public static native void createAssetManager(AssetManager assetManager, String apkPath, String SDCardPath);
}