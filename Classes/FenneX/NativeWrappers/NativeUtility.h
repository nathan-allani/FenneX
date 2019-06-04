/****************************************************************************
 Copyright (c) 2013-2016 Auticiel SAS
 
 http://www.fennex.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************///

#ifndef FenneX_NativeUtility_h
#define FenneX_NativeUtility_h

#include "Shorteners.h"
#include "AppDelegate.h"
#include "FenneXMacros.h"

NS_FENNEX_BEGIN

/* Allow to know if we are running tests.
 Some things must be disabled when running tests, for example ApiService should not be automatically runned (so it can be tested without conflicts), and we probably want to avoid saving anything, so that we don't loose user data
 Tests can only be running on iOS for now
 */
bool isRunningTests();

bool isPhone();

/* Return the URL the app was opened with, or an empty string
 You also get notified with "UrlOpened" event when app is opened with an url during run (not at startup)
 */
std::string getOpenUrl();

//Use AppName if you need to actually show it. Use package identifier if you need to save files for example, as it does not contain special characters
std::string getAppName();
std::string getPackageIdentifier();

//Return the version number, for example 1.2.4
std::string getAppVersionNumber();

//Return the version code, for example 12, on Android. Always return -1 on iOS because we don't use CFBundleVersion properly, and there is no actual build number
int getAppVersionCode();

/* Get an unique identifier of the device.
 - on iOS, it uses identifierForVendor, which CAN change if all apps are uninstalled at once then reinstalled. It can also be null at the beginning. If it is, retry later
 - on Android, it uses ANDROID_IT, which CAN change on factory reset and can be different if there are several accounts on the device.
 */
std::string getUniqueIdentifier();

//Get an identifier for the device model. Should be unique
std::string getDeviceModelIdentifier();

//Get a name (supposed to be good to show to user, but it's actually not ...)
std::string getDeviceModelName();

//Get the version for the device, for example 4.4.2
std::string getDeviceVersion();

//Return Android SDK version or major iOS version
int getDeviceSDK();

//Expressed in bytes. A long is not enough, since it only goes up to 2Gb. We specifically want 64bit, and might as well have it unsigned (though that's not supported on Android)
uint64_t getTotalStorageSpace();
uint64_t getAvailableStorageSpace();

//Return the movie folder name, without any "/", so that it can be displayed to client. Note: please do not use it on iOS, as there is no movie folder
std::string getMovieFolderName();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
void copyResourceFileToLocal(const std::string& path);
#endif


//implemented by platform because cocos2d version doesn't return the string identifier
//Android version is a copy of getCurrentLanguageJNI defined in main.cpp
//iOS version is defined in AppController
std::string getLocalLanguage();

void preventIdleTimerSleep(bool prevent);
bool doesPreventIdleTimerSleep();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
//The real goal of those methods (which should be called when starting a scene initialisation and after it's fully initialized and drawn)
//is to mitigate the effect of the Garbage Collector due to Label modifications (avoid running it during scene initialisation)
void startSceneInitialisation();
void runGarbageCollector();
#endif

//will format the date in short format (example : 9/8/2010) according user local
std::string formatDate(time_t date);

enum class DateFormat
{
    NONE, // Won't show anything
    SHORT, // 12.13.52 or 3:30pm
    MEDIUM, // Jan 12, 1952
    LONG, // January 12, 1952 or 3:30:32pm
    FULL, // Tuesday, April 12, 1952 AD or 3:30:42pm PST
};
// Will format the date depending on the DateFormat parameters
std::string formatDateTime(time_t date, DateFormat dayFormat = DateFormat::LONG, DateFormat hourFormat = DateFormat::SHORT);
//Will format a duration in seconds into hours/minutes/seconds displayable to user (example: 1h 5m 20s on iOS, 1:05:20 on Android)
std::string formatDurationShort(int seconds);

//Return a float between 0.0 (muted) and 1.0 (full volume)
float getDeviceVolume();
void setDeviceVolume(float volume);

void setDeviceNotificationVolume(float volume);

//Return the step you should use for the current device
float getVolumeStep();

//Will change the native background color (behind everything else, including video and opengl scene)
//The values should range from 0 to 255 (0,0,0 is black, 255,255,255 is white)
void setBackgroundColor(int r, int g, int b);

/*Will vibrate the device, or play a sound if vibrator is not available
 on iOS, the time is fixed by the iOS, and there may be some conflicts with AudioPlayerRecorder
 on Android, the <uses-permission android:name="android.permission.VIBRATE"/> permission is required
 */
void vibrate(int milliseconds);
bool canVibrate();

//Return a float between 0 and 1 describing the device luminosity
float getDeviceLuminosity();

//Change the device luminosity, must be a float between 0 and 1
void setDeviceLuminosity(float);

//Open system settings app. Should always work on iOS8+ and Android. Not available on iOS < 8
//Return true if the settings app will open
bool openSystemSettings();

//Return if a package is installed on android, return false on iOS
bool isPackageInstalled(const std::string& packageName);

//Return the version code on Android or -1 if it's not installed, return -1 on iOS
int getApplicationVersion(const std::string& packageName);

//On iOS, those notifications will automatically start being thrown after getDeviceVolume has been called for the first time
//On Android, they are always on
static inline void notifyVolumeChanged()
{
	DelayedDispatcher::eventAfterDelay("VolumeChanged", Value(ValueMap({{"Volume", Value(getDeviceVolume())}})), 0.01);
}

inline void notifyMemoryWarning(){
	AppDelegate* delegate = (AppDelegate*)cocos2d::Application::getInstance();
    //warning : maybe this should be async to run on main thread ?
    //No problem so far, converting the #warning to a simple comment
	delegate->applicationDidReceiveMemoryWarning();
}

static inline void notifyUrlOpened(std::string url)
{
    DelayedDispatcher::eventAfterDelay("UrlOpened", Value(ValueMap({{"Url", Value(url.c_str())}})), 0.01);
}

/**
 Allow app to define how String displayed to user by native modules will be translated.
 Here is the list of all string that need to be translated:
 * "TooManyAppsLaunched" => There are too many apps executing. User should quit some apps.
 * "PleaseRetrySoon" => Something failed, user should retry soon.
 * "ImageTooLarge" => Image selected by user cannot be read because it is too large.
 * "NoMailClient" => The device doesn't have any mail client available.
 * "CantCopyMailAttachment" => The attachment couldn't be copied to a public location for the mail client.
 * "CantCopyVideo" => The video cannot be copied in public Movies folder.
 * "CantUseCamera" => Cannot use the camera. It is used by another app, or there is no camera
 * "VideoRecordingMaxDuration" => Max duration for video recording reached (configured to 1 hour). The video was saved.
 * "VideoRecordingMaxFilesize" => Max file size for video recording reached (configured to 1Go). The video was saved.
 * "VideoRecordingError" => Error while trying to record a video.
*/
void setNativeStringConverter(std::function<std::string(const std::string&)> converter);

//Internal-use function for native modules. Provide a key, get a translated string
std::string getNativeString(const std::string& key);

NS_FENNEX_END

#endif
