/**
 * Copyright (c) 2006-2016 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/
 
 /**
 *This file has been edited by bio1712 for love2d.org
 **/

#include "android.h"

#ifdef LOVE_ANDROID

#include "SDL.h"
#include "jni.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

namespace love
{
namespace android
{

void setImmersive(bool immersive_active)
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "setImmersiveMode", "(Z)V");

	env->CallVoidMethod(activity, method_id, immersive_active);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}

bool getImmersive()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "getImmersiveMode", "()Z");

	jboolean immersive_active = env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return immersive_active;
}

double getScreenScale()
{
	static double result = -1.;

	if (result == -1.)
	{
		JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();
		jclass activity = env->FindClass("org/love2d/android/GameActivity");

		jmethodID getMetrics = env->GetStaticMethodID(activity, "getMetrics", "()Landroid/util/DisplayMetrics;");
		jobject metrics = env->CallStaticObjectMethod(activity, getMetrics);
		jclass metricsClass = env->GetObjectClass(metrics);

		result = env->GetFloatField(metrics, env->GetFieldID(metricsClass, "density", "F"));

		env->DeleteLocalRef(metricsClass);
		env->DeleteLocalRef(metrics);
		env->DeleteLocalRef(activity);
	}

	return result;
}

const char *getSelectedGameFile()
{
	static const char *path = NULL;

	if (path)
	{
		delete path;
		path = NULL;
	}

	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();
	jclass activity = env->FindClass("org/love2d/android/GameActivity");

	jmethodID getGamePath = env->GetStaticMethodID(activity, "getGamePath", "()Ljava/lang/String;");
	jstring gamePath = (jstring) env->CallStaticObjectMethod(activity, getGamePath);
	const char *utf = env->GetStringUTFChars(gamePath, 0);
	if (utf)
	{
		path = SDL_strdup(utf);
		env->ReleaseStringUTFChars(gamePath, utf);
	}

	env->DeleteLocalRef(gamePath);
	env->DeleteLocalRef(activity);

	return path;
}

bool openURL(const std::string &url)
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();
	jclass activity = env->FindClass("org/love2d/android/GameActivity");

	jmethodID openURL= env->GetStaticMethodID(activity, "openURL", "(Ljava/lang/String;)V");
	jstring url_jstring = (jstring) env->NewStringUTF(url.c_str());

	env->CallStaticVoidMethod(activity, openURL, url_jstring);

	env->DeleteLocalRef(url_jstring);
	env->DeleteLocalRef(activity);
	return true;
}

void vibrate(double seconds)
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();
	jclass activity = env->FindClass("org/love2d/android/GameActivity");

	jmethodID vibrate_method = env->GetStaticMethodID(activity, "vibrate", "(D)V");
	env->CallStaticVoidMethod(activity, vibrate_method, seconds);

	env->DeleteLocalRef(activity);
}

/*
 * Helper functions for the filesystem module
 */
void freeGameArchiveMemory(void *ptr)
{
	char *game_love_data = static_cast<char*>(ptr);
	delete[] game_love_data;
}

bool loadGameArchiveToMemory(const char* filename, char **ptr, size_t *size)
{
	SDL_RWops *asset_game_file = SDL_RWFromFile(filename, "rb");
	if (!asset_game_file) {
		SDL_Log("Could not find %s", filename);
		return false;
	}

	Sint64 file_size = asset_game_file->size(asset_game_file);
	if (file_size <= 0) {
		SDL_Log("Could not load game from %s. File has invalid file size: %d.", filename, (int) file_size);
		return false;
	}

	*ptr = new char[file_size];
	if (!*ptr) {
		SDL_Log("Could not allocate memory for in-memory game archive");
		return false;
	}

	size_t bytes_copied = asset_game_file->read(asset_game_file, (void*) *ptr, sizeof(char), (size_t) file_size);
	if (bytes_copied != file_size) {
		SDL_Log("Incomplete copy of in-memory game archive!");
		delete[] *ptr;
		return false;
	}

	*size = (size_t) file_size;
	return true;
}

bool directoryExists(const char *path)
{
	struct stat s;
	int err = stat(path, &s);
	if (err == -1)
	{
		if (errno != ENOENT)
			SDL_Log("Error checking for directory %s errno = %d: %s", path, errno, strerror(errno));
		return false;
	}

	return S_ISDIR(s.st_mode);
}

bool mkdir(const char *path)
{
	int err = ::mkdir(path, 0770);
	if (err == -1)
	{
		SDL_Log("Error: Could not create directory %s", path);
		return false;
	}

	return true;
}

bool createStorageDirectories()
{
	std::string internal_storage_path = SDL_AndroidGetInternalStoragePath();

	std::string save_directory = internal_storage_path + "/save";
	if (!directoryExists(save_directory.c_str()) && !mkdir(save_directory.c_str()))
		return false;

	std::string game_directory = internal_storage_path + "/game";
	if (!directoryExists (game_directory.c_str()) && !mkdir(game_directory.c_str()))
		return false;

	return true;
}

//Ads

void createBanner(const char *adID,const char *position)
{
	std::string ID = (std::string) adID;
	std::string pos = (std::string) position;
	
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();
	jclass clazz (env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "createBanner", "(Ljava/lang/String;Ljava/lang/String;)V");
	jstring ID_jstring = (jstring) env->NewStringUTF(ID.c_str());
	jstring pos_jstring = (jstring) env->NewStringUTF(pos.c_str());

	env->CallVoidMethod(activity, method_id, ID_jstring, pos_jstring);

	env->DeleteLocalRef(ID_jstring);
	env->DeleteLocalRef(pos_jstring);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}

	
void hideBanner()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();
	jclass clazz (env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "hideBanner", "()V");

	env->CallVoidMethod(activity, method_id);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}
	
void showBanner()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();
	jclass clazz (env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "showBanner", "()V");

	env->CallVoidMethod(activity, method_id);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}

void requestInterstitial(const char *adID)
{
	std::string ID = (std::string) adID;
	
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();
	jclass clazz (env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "requestInterstitial", "(Ljava/lang/String;)V");
	jstring ID_jstring = (jstring) env->NewStringUTF(ID.c_str());

	env->CallVoidMethod(activity, method_id, ID_jstring);

	env->DeleteLocalRef(ID_jstring);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}

bool isInterstitialLoaded()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "isInterstitialLoaded", "()Z");

	jboolean adLoaded = env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return adLoaded;
}

void showInterstitial()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();
	jclass clazz (env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "showInterstitial", "()V");

	env->CallVoidMethod(activity, method_id);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}


void requestRewardedAd(const char *adID)
{
	std::string ID = (std::string) adID;
	
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();
	jclass clazz (env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "requestRewardedAd", "(Ljava/lang/String;)V");
	jstring ID_jstring = (jstring) env->NewStringUTF(ID.c_str());

	env->CallVoidMethod(activity, method_id, ID_jstring);

	env->DeleteLocalRef(ID_jstring);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}

bool isRewardedAdLoaded()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "isRewardedAdLoaded", "()Z");

	jboolean adLoaded = env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return adLoaded;
}

void showRewardedAd()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();
	jclass clazz (env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "showRewardedAd", "()V");

	env->CallVoidMethod(activity, method_id);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}

//For callbacks

bool coreInterstitialError()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "coreInterstitialError", "()Z");

	jboolean adHasFailedToLoad = env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return adHasFailedToLoad;
}

bool coreInterstitialClosed()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "coreInterstitialClosed", "()Z");

	jboolean adHasBeenClosed = env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return adHasBeenClosed;
}

bool coreRewardedAdError()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "coreRewardedAdError", "()Z");

	jboolean adHasFailedToLoad = env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return adHasFailedToLoad;
}

bool coreRewardedAdDidStop()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "coreRewardedAdDidStop", "()Z");

	jboolean adHasBeenClosed = env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return adHasBeenClosed;
}

bool coreRewardedAdDidFinish()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "coreRewardedAdDidFinish", "()Z");

	jboolean videoHasFinishedPlaying = env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return videoHasFinishedPlaying;
}

std::string coreGetRewardType()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "coreGetRewardType",  "()Ljava/lang/String;");

	jstring rewardType = (jstring) env->CallObjectMethod(activity, method_id);
	
	const char *strPtr = env->GetStringUTFChars(rewardType, 0);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return (std::string) strPtr;
}

double coreGetRewardQuantity()
{
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	jobject activity = (jobject) SDL_AndroidGetActivity();

	jclass clazz(env->GetObjectClass(activity));
	jmethodID method_id = env->GetMethodID(clazz, "coreGetRewardQuantity",  "()D");

	jdouble rewardQty = (jdouble) env->CallDoubleMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return static_cast<double>(rewardQty);
}

} // android
} // love

#endif // LOVE_ANDROID
