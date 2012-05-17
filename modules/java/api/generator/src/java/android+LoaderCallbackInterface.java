package org.opencv.android;

/**
 * Interface for callback object in case of asynchronous initialization of OpenCV
 */
public interface LoaderCallbackInterface
{
	/**
	 * OpenCV initialization finished successfully
	 */
    static final int SUCCESS = 0;
    /**
     * OpenCV Engine service is not installed on the device. App need to notify user about it
     */
    static final int NO_SERVICE = 1;
    /**
     * OpenCV library installation via Google Play service was initialized. Application restart is required
     */
    static final int RESTART_REQUIRED = 2;
    /**
     * Google Play (Android Market) cannot be invoked
     */
    static final int MARKET_ERROR = 3;
    /**
     * OpenCV library installation was canceled by user
     */
    static final int INSTALL_CANCELED = 4;
    /**
     * OpenCV library initialization failed
     */
    static final int INIT_FAILED = 0xff;

    /**
     * Callback method for Async OpenCV initialization
     * @param status Status of initialization. See Initialization status constants
     */
    public void onEngineConnected(int status);
};