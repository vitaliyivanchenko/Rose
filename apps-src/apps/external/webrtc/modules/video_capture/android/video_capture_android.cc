/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <utility>
#include <android/log.h>

#include "webrtc/base/arraysize.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/refcount.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/modules/video_capture/android/device_info_android.h"
#include "webrtc/modules/video_capture/android/video_capture_android.h"
#include "webrtc/system_wrappers/include/trace.h"

#define TAG "VideoCaptureAndroid"
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

using namespace webrtc;
using namespace videocapturemodule;

// VideoCaptureAndroid::JavaVideoCapturer implementation
VideoCaptureAndroid::JavaVideoCapturer::JavaVideoCapturer(NativeRegistration* native_reg, std::unique_ptr<GlobalRef> audio_manager)
    : audio_manager_(std::move(audio_manager))
      , init_(native_reg->GetMethodId("init", "()Z"))
      , dispose_(native_reg->GetMethodId("dispose", "()V"))
      , start_capture_(native_reg->GetMethodId("startCapture", "()Z")) 
{
	ALOGD("JavaVideoCapturer::ctor%s", GetThreadInfo().c_str());
}

VideoCaptureAndroid::JavaVideoCapturer::~JavaVideoCapturer() 
{
	ALOGD("JavaVideoCapturer::dtor%s", GetThreadInfo().c_str());
}

bool VideoCaptureAndroid::JavaVideoCapturer::Init() 
{
	return audio_manager_->CallBooleanMethod(init_);
}

void VideoCaptureAndroid::JavaVideoCapturer::Close() {
	audio_manager_->CallVoidMethod(dispose_);
}

bool VideoCaptureAndroid::JavaVideoCapturer::StartCapture() 
{
	return audio_manager_->CallBooleanMethod(start_capture_);
}

rtc::scoped_refptr<VideoCaptureModule> VideoCaptureImpl::Create(
     const char* deviceUniqueIdUTF8) {
  return VideoCaptureAndroid::Create(deviceUniqueIdUTF8);
}

VideoCaptureAndroid::VideoCaptureAndroid()
    : VideoCaptureImpl()
	, is_capturing_(false)
	, initialized_(false)
{
  capability_.width = kDefaultWidth;
  capability_.height = kDefaultHeight;
  capability_.maxFPS = kDefaultFrameRate;
}

VideoCaptureAndroid::~VideoCaptureAndroid() {
  if (is_capturing_) {
    StopCapture();
  }
}

rtc::scoped_refptr<VideoCaptureModule> VideoCaptureAndroid::Create(
    const char* deviceUniqueIdUTF8) {
  if (!deviceUniqueIdUTF8[0]) {
    return NULL;
  }

  rtc::scoped_refptr<VideoCaptureAndroid> capture_module(
      new rtc::RefCountedObject<VideoCaptureAndroid>());

  const int32_t name_length = strlen(deviceUniqueIdUTF8);
  if (name_length > kVideoCaptureUniqueNameLength)
    return nullptr;

  capture_module->_deviceUniqueId = new char[name_length + 1];
  strncpy(capture_module->_deviceUniqueId, deviceUniqueIdUTF8, name_length + 1);
  capture_module->_deviceUniqueId[name_length] = '\0';

  return capture_module;
}

int32_t VideoCaptureAndroid::StartCapture(const VideoCaptureCapability& capability) 
{
	capability_ = capability;

	if (!InitCaptureAndroidJNI() || !j_video_capturer_->StartCapture()) {
		UninitCaptureAndroidJNI();
		ALOGD("StartCapture%s, Fail", GetThreadInfo().c_str());
		return -1;
	}

	is_capturing_ = true;
	return 0;
}

int32_t VideoCaptureAndroid::StopCapture() 
{
	if (!is_capturing_) {
		return 0;
	}
	UninitCaptureAndroidJNI();

	is_capturing_ = false;
	return 0;
}

bool VideoCaptureAndroid::CaptureStarted() { return is_capturing_; }

int32_t VideoCaptureAndroid::CaptureSettings(VideoCaptureCapability& settings)
{
	settings = capability_;
	settings.rawType = kVideoNV12;
	return 0;
}

bool VideoCaptureAndroid::InitCaptureAndroidJNI()
{
	attach_thread_if_needed_.reset(new AttachCurrentThreadIfNeeded); 
	std::unique_ptr<JNIEnvironment> j_environment_ = JVM::GetInstance()->environment();

	ALOGD("InitCaptureAndroidJNI%s", GetThreadInfo().c_str());
	RTC_CHECK(j_environment_);

	JNINativeMethod native_methods[] = {
	  {"nativeCapturerStarted",
       "(JZ)V",
       reinterpret_cast<void*>(&VideoCaptureAndroid::CapturerStarted)},
	  {"nativeCapturerStopped",
       "(J)V",
       reinterpret_cast<void*>(&VideoCaptureAndroid::CapturerStopped)},
	  {"nativeOnByteBufferFrameCaptured",
       "(J[BIIIIJ)V",
       reinterpret_cast<void*>(&VideoCaptureAndroid::OnByteBufferFrameCaptured)},
	  {"nativeOnTextureFrameCaptured",
       "(JIII[FIJ)V",
       reinterpret_cast<void*>(&VideoCaptureAndroid::OnTextureFrameCaptured)},
      {"nativeCreateSurfaceTextureHelper",
       "(JLorg/webrtc/EglBase$Context;)V",
       reinterpret_cast<void*>(&VideoCaptureAndroid::CreateSurfaceTextureHelper)},
	  {"nativeInitializeVideoCapturer",
       "(JLorg/webrtc/VideoCapturer;Lorg/webrtc/VideoCapturer$CapturerObserver;)V",
       reinterpret_cast<void*>(&VideoCaptureAndroid::InitializeVideoCapturer)}
	};

	j_native_registration_ = j_environment_->RegisterNatives(
      "org/webrtc/videoengine/WebRtcVideoCapturer", native_methods,
      arraysize(native_methods));
	ALOGD("j_native_registration_: %p", j_native_registration_.get());
	j_video_capturer_.reset(new JavaVideoCapturer(
      j_native_registration_.get(),
      j_native_registration_->NewObject(
          "<init>", "(Landroid/content/Context;J)V",
          JVM::GetInstance()->context(), PointerTojlong(this))));

	RTC_DCHECK(!initialized_);
	if (!j_video_capturer_->Init()) {
		ALOGE("init failed!");
		return false;
	}
	initialized_ = true;
	return true;
}

void VideoCaptureAndroid::UninitCaptureAndroidJNI()
{
	// this thread must be InitCaptureAndroidJNI's thread.
	// all JNI operator must be executed in the thread.
	ALOGD("UninitCaptureAndroidJNI%s", GetThreadInfo().c_str());
	RTC_DCHECK(thread_checker_.CalledOnValidThread());
	
	if (j_video_capturer_) {
		j_video_capturer_->Close();
		j_video_capturer_.reset();
	}
	if (j_native_registration_) {
		j_native_registration_.reset();
	}
	surface_texture_helper_ = NULL;
	attach_thread_if_needed_.reset(NULL);
	initialized_ = false;

	ALOGD("UninitCaptureAndroidJNI%s, completement!", GetThreadInfo().c_str());
}

void JNICALL VideoCaptureAndroid::CapturerStarted(JNIEnv* env, jobject obj, jlong j_source, jboolean success)
{
	ALOGD("CapturerStarted%s, %s", GetThreadInfo().c_str(), success? "Success": "Fail");
}

void JNICALL VideoCaptureAndroid::CapturerStopped(JNIEnv* env, jobject obj, jlong j_source)
{
	ALOGD("CapturerStopped%s", GetThreadInfo().c_str());
}

void JNICALL VideoCaptureAndroid::OnByteBufferFrameCaptured(JNIEnv* jni, jclass, jlong j_source, jbyteArray j_frame, jint length, jint width, jint height, jint rotation, jlong timestamp)
{
	ALOGD("OnByteBufferFrameCaptured%s, (%dx%d)", GetThreadInfo().c_str(), width, height);
	VideoCaptureAndroid* this_object = reinterpret_cast<VideoCaptureAndroid*>(j_source);
/*
	jbyte* bytes = jni->GetByteArrayElements(j_frame, nullptr);
	source->OnByteBufferFrameCaptured(bytes, length, width, height, rotation, timestamp);
	jni->ReleaseByteArrayElements(j_frame, bytes, JNI_ABORT);
*/
}

void JNICALL VideoCaptureAndroid::OnTextureFrameCaptured(JNIEnv* jni, jobject obj, jlong j_source, jint j_width, jint j_height, jint j_oes_texture_id, jfloatArray j_transform_matrix, jint j_rotation, jlong j_timestamp) 
{
	ALOGD("OnTextureFrameCaptured%s, (%dx%d)", GetThreadInfo().c_str(), j_width, j_height);
	VideoCaptureAndroid* this_object = reinterpret_cast<VideoCaptureAndroid*>(j_source);

	this_object->onTextureFrameCaptured(j_width, j_height, j_rotation, j_timestamp, webrtc_jni::NativeHandleImpl(jni, j_oes_texture_id, j_transform_matrix));
}

void JNICALL VideoCaptureAndroid::CreateSurfaceTextureHelper(JNIEnv* env, jobject obj, jlong j_source, jobject j_egl_context) 
{
	// ALOGD("CreateSurfaceTextureHelper%s, j_source(0x%lx)", GetThreadInfo().c_str(), j_source);
	ALOGD("CreateSurfaceTextureHelper%s", GetThreadInfo().c_str());
	VideoCaptureAndroid* this_object = reinterpret_cast<VideoCaptureAndroid*>(j_source);

	this_object->set_surface_texture_helper(webrtc_jni::SurfaceTextureHelper::create(
          env,
          "Camera SurfaceTextureHelper",
          j_egl_context));
}

void JNICALL VideoCaptureAndroid::InitializeVideoCapturer(JNIEnv* jni, jclass, jlong j_source, jobject j_video_capturer, jobject j_frame_observer) 
{
	ALOGD("InitializeVideoCapturer%s", GetThreadInfo().c_str());
	VideoCaptureAndroid* this_object = reinterpret_cast<VideoCaptureAndroid*>(j_source);

	this_object->initializeVideoCapturer(jni, j_video_capturer, j_frame_observer);
}

void VideoCaptureAndroid::set_surface_texture_helper(rtc::scoped_refptr<webrtc_jni::SurfaceTextureHelper> helper)
{
	ALOGD("set_surface_texture_helper, this: %p, helper: %p", this, helper.get());
	surface_texture_helper_ = helper;
}

void VideoCaptureAndroid::initializeVideoCapturer(JNIEnv* jni, jobject j_video_capturer, jobject j_frame_observer)
{
	ALOGD("initializeVideoCapturer%s", GetThreadInfo().c_str());

	jni->CallVoidMethod(j_video_capturer,
		GetMethodID(jni, FindClass(jni, "org/webrtc/VideoCapturer"), "initialize",
                  "(Lorg/webrtc/SurfaceTextureHelper;Landroid/content/"
                  "Context;Lorg/webrtc/VideoCapturer$CapturerObserver;)V"),
        surface_texture_helper_->GetJavaSurfaceTextureHelper(),
		JVM::GetInstance()->context(), j_frame_observer);

	CHECK_EXCEPTION(jni) << "error during VideoCapturer.initialize()";
}

void VideoCaptureAndroid::onTextureFrameCaptured(int width, int height, int rotation, int64_t timestamp_ns, const webrtc_jni::NativeHandleImpl& handle)
{
	// RTC_DCHECK(camera_thread_checker_.CalledOnValidThread());
	RTC_DCHECK(rotation == 0 || rotation == 90 || rotation == 180 ||
             rotation == 270);

	int64_t camera_time_us = timestamp_ns / rtc::kNumNanosecsPerMicrosec;
	int64_t translated_camera_time_us =
      timestamp_aligner_.TranslateTimestamp(camera_time_us, rtc::TimeMicros());

	int adapted_width;
	int adapted_height;
	int crop_width;
	int crop_height;
	int crop_x;
	int crop_y;

	if (!AdaptFrame(width, height, camera_time_us, &adapted_width,
                  &adapted_height, &crop_width, &crop_height, &crop_x,
                  &crop_y)) {
		ALOGE("Frame conversion failed, AdaptFrame.");
		surface_texture_helper_->ReturnTextureFrame();
		return;
	}

	webrtc_jni::Matrix matrix = handle.sampling_matrix;

	matrix.Crop(crop_width / static_cast<float>(width),
              crop_height / static_cast<float>(height),
              crop_x / static_cast<float>(width),
              crop_y / static_cast<float>(height));

	// Make a local copy, since value of apply_rotation() may change
	// under our feet.
	// bool do_rotate = apply_rotation();
	bool do_rotate = GetApplyRotation();

	if (do_rotate) {
		if (rotation == webrtc::kVideoRotation_90 || rotation == webrtc::kVideoRotation_270) {
			std::swap(adapted_width, adapted_height);
		}
		matrix.Rotate(static_cast<webrtc::VideoRotation>(rotation));
	}

	ALOGD("onTextureFrameCaptured#1, (%dx%d), do_rotate: %s, rotation: %d", adapted_width, adapted_height, do_rotate? "true": "false", rotation);

	VideoFrame converted_frame(
      surface_texture_helper_->CreateTextureFrame(
          adapted_width, adapted_height,
          webrtc_jni::NativeHandleImpl(handle.oes_texture_id, matrix)),
      do_rotate ? webrtc::kVideoRotation_0
                : static_cast<webrtc::VideoRotation>(rotation),
      translated_camera_time_us);

	ALOGD("onTextureFrameCaptured#2, (%dx%d)", converted_frame.width(), converted_frame.height());
	// owner_->DeliverCapturedFrame(videoFrame);

	rtc::scoped_refptr<VideoFrameBuffer> converted_buffer(
        converted_frame.video_frame_buffer()->NativeToI420Buffer());

	if (!converted_buffer) {
		ALOGE("Frame conversion failed, dropping frame.");
		surface_texture_helper_->ReturnTextureFrame();
		return;
		// return VCM_PARAMETER_ERROR;
	}
    converted_frame = VideoFrame(converted_buffer,
                                 converted_frame.timestamp(),
                                 converted_frame.render_time_ms(),
                                 converted_frame.rotation());
	ALOGD("onTextureFrameCaptured#3, (%dx%d)", converted_frame.width(), converted_frame.height());

	DeliverCapturedFrame(converted_frame);
}

bool VideoCaptureAndroid::AdaptFrame(int width,
                                         int height,
                                         int64_t time_us,
                                         int* out_width,
                                         int* out_height,
                                         int* crop_width,
                                         int* crop_height,
                                         int* crop_x,
                                         int* crop_y) {
 /* {
    rtc::CritScope lock(&stats_crit_);
    stats_ = rtc::Optional<Stats>({width, height});
  }

  if (!broadcaster_.frame_wanted()) {
    return false;
  }
*/
  if (!video_adapter_.AdaptFrameResolution(
          width, height, time_us * rtc::kNumNanosecsPerMicrosec,
          crop_width, crop_height, out_width, out_height)) {
    // VideoAdapter dropped the frame.
    return false;
  }

  *crop_x = (width - *crop_width) / 2;
  *crop_y = (height - *crop_height) / 2;
  return true;
}