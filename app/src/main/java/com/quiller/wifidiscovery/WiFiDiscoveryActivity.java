package com.quiller.wifidiscovery;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;

import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;

import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Enumeration;
import java.util.Locale;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class WiFiDiscoveryActivity extends Activity {

  private GvrLayout gvrLayout;
  private long nativeInst;
  private GLSurfaceView glSurfaceView;
  private static float MAX_HOSTS = 128;

  public enum WIFI_STATE {
    NOT_CONNECTED(0), SCANNING(1);
    private int value;

    WIFI_STATE(int num) {
      this.value = num;
    }

    public int getVal() {
      return value;
    }
  }

  // This is done on the GL thread because refreshViewerProfile isn't thread-safe.
  private final Runnable refreshViewerProfileRunnable =
      new Runnable() {
        @Override
        public void run() {
          gvrLayout.getGvrApi().refreshViewerProfile();
        }
      };  
  
  // Load the native library
  static {
    System.loadLibrary("wifidiscovery");
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    // Set the window properties
    setImmersiveSticky();
    getWindow()
        .getDecorView()
        .setOnSystemUiVisibilityChangeListener(
            new View.OnSystemUiVisibilityChangeListener() {
              @Override
              public void onSystemUiVisibilityChange(int visibility) {
                if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
                  setImmersiveSticky();
                }
              }
            });

    // Set the layout
    gvrLayout = new GvrLayout(this);
    nativeInst = createRenderer(
        gvrLayout.getGvrApi().getNativeGvrContext(), getAssets(),
        getClass().getClassLoader(), this.getApplicationContext());  
    
    // Configure the layout's view
    glSurfaceView = new GLSurfaceView(this);
    glSurfaceView.setEGLContextClientVersion(3);
    glSurfaceView.setEGLConfigChooser(8, 8, 8, 0, 0, 0);
    glSurfaceView.setPreserveEGLContextOnPause(true);

    // Set the renderer
    glSurfaceView.setRenderer(new GLSurfaceView.Renderer() {

      public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        nativeOnSurfaceCreated(nativeInst);
      }

      public void onSurfaceChanged(GL10 gl, int w, int h) {}

      public void onDrawFrame(GL10 gl) {
        nativeOnDrawFrame(nativeInst);
      }
    });
    gvrLayout.setPresentationView(glSurfaceView);
    setContentView(gvrLayout);    
    
    // Enable scan line racing
    if (gvrLayout.setAsyncReprojectionEnabled(true)) {
      AndroidCompat.setSustainedPerformanceMode(this, true);
    }
    AndroidCompat.setVrModeEnabled(this, true);    
    
    // Get the device's WiFi address
    WifiManager mgr = (WifiManager)getSystemService(Context.WIFI_SERVICE);
    if(!mgr.isWifiEnabled()) {
      if(mgr.getWifiState() != WifiManager.WIFI_STATE_ENABLING)
        mgr.setWifiEnabled(true);
    }

    WifiInfo info = mgr.getConnectionInfo();
    int ip = info.getIpAddress();

    // Not connected
    if(ip == 0) {
      nativeSetState(nativeInst, WIFI_STATE.NOT_CONNECTED.getVal());

    // Scanning
    } else {

      // Set the state to scanning
      nativeSetState(nativeInst, WIFI_STATE.SCANNING.getVal());

      // Determine the IP address string
      String address = String.format(Locale.US, "%d.%d.%d.%d", (ip & 0xff),
        (ip >> 8 & 0xff), (ip >> 16 & 0xff), (ip >> 24 & 0xff));

      NetworkScanner scanner = new NetworkScanner();
      scanner.execute(address);
    }
  }

  class NetworkScanner extends AsyncTask<String, Void, Void> {

    StringBuilder sb;
    protected void onPreExecute() {
      sb = new StringBuilder(200);
    }

    protected Void doInBackground(String... params) {

      try {
        int numHosts = 0;
        for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
          NetworkInterface intf = en.nextElement();
          for (InterfaceAddress address : intf.getInterfaceAddresses()) {
            String ipAddr = address.getAddress().toString().substring(1);

            // Find the WiFi address
            if(ipAddr.equals(params[0])) {

              // Convert the second, third, fourth octets into a number
              int addrNum = 0;
              String octets[] = ipAddr.split("\\.");
              for(int i=1; i<4; i++) {
                addrNum += Integer.parseInt(octets[i]) << (8*(3-i));
              }

              // Find the network prefix and mask the IP address
              int netPrefix = address.getNetworkPrefixLength();
              int mask = 1 << (32 - netPrefix);
              int base = addrNum & (Integer.MAX_VALUE ^ (mask-1));
              int progressInterval = mask/64;
              int progressCounter = progressInterval;

              // Iterate through IP addresses
              try {
                for (int i=0; i<mask; i++) {

                  // Create IP address from number
                  String addrString = String.format(Locale.US, "%s.%d.%d.%d", octets[0],
                    ((base + i) >> 16 & 0xff), ((base + i) >> 8 & 0xff), ((base + i) & 0xff));
                  InetAddress addr = Inet4Address.getByName(addrString);

                  // Progress update
                  if(i == progressCounter) {
                    publishProgress();
                    progressCounter += progressInterval;
                  }

                  if(addr.isReachable(50) && (numHosts < MAX_HOSTS)) {
                    sb.append(addr.getHostName()).append(":").append(addr.getHostAddress());
                    nativeAddHost(nativeInst, sb.toString());
                    sb.setLength(0);
                    numHosts++;
                  }
                }
              } catch(UnknownHostException uhe) {
              } catch(IOException ie) {}
            }
          }
        }
      }
      catch(SocketException se) {
        sb.append(se.getMessage());
      }
      return null;
    }

    @Override
    protected void onProgressUpdate(Void... progress) {
      nativeProgressUpdate(nativeInst);
    }

    protected void onPostExecute(Void result) {

      // Alert native code that scan has finished
      nativeSetScanComplete(nativeInst);
    }
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      setImmersiveSticky();
    }
  }  
  
  @Override
  protected void onPause() {
    super.onPause();
    nativeOnPause(nativeInst);
    gvrLayout.onPause();
    glSurfaceView.onPause();    
  }

  @Override
  protected void onResume() {
    super.onResume();
    nativeOnResume(nativeInst);
    gvrLayout.onResume();
    glSurfaceView.onResume();    
    glSurfaceView.queueEvent(refreshViewerProfileRunnable);    
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    gvrLayout.shutdown();
    nativeOnDestroy(nativeInst);
  }

  private void setImmersiveSticky() {
    getWindow()
        .getDecorView()
        .setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
  }  
  
  // Native methods
  private native long createRenderer(long gvrContext, AssetManager manager,
    ClassLoader loader, Context context);
  private native void nativeOnSurfaceCreated(long nativeInst);
  private native void nativeProgressUpdate(long nativeInst);
  private native void nativeAddHost(long nativeInst, String result);
  private native void nativeSetScanComplete(long nativeInst);  
  private native void nativeSetState(long nativeInst, int state);
  private native void nativeOnDrawFrame(long nativeInst);
  private native void nativeOnPause(long nativeInst);
  private native void nativeOnResume(long nativeInst);
  private native void nativeOnDestroy(long nativeInst);
}
