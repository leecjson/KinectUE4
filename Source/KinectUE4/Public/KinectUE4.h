// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"


//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
//#endif
//
//#pragma warning (push)
//#pragma warning (disable : 4005)
//#include <Kinect.h>
//#pragma warning (pop)

enum class FKinectJointType {
  JSpineBase = 0,
  SpineMid = 1,
  Neck = 2,
  Head = 3,
  ShoulderLeft = 4,
  ElbowLeft = 5,
  WristLeft = 6,
  HandLeft = 7,
  ShoulderRight = 8,
  ElbowRight = 9,
  WristRight = 10,
  HandRight = 11,
  HipLeft = 12,
  KneeLeft = 13,
  AnkleLeft = 14,
  FootLeft = 15,
  HipRight = 16,
  KneeRight = 17,
  AnkleRight = 18,
  FootRight = 19,
  SpineShoulder = 20,
  HandTipLeft = 21,
  ThumbLeft = 22,
  HandTipRight = 23,
  ThumbRight = 24,
  Count = (ThumbRight + 1)
};

enum class FKinectTrackingState {
  NotTracked = 0,
  Inferred = 1,
  Tracked = 2
};

//UENUM(BlueprintType)
enum class FKinectGestureType {
  None = 0,
  Discrete = 1,
  Continuous = 2
};

struct FKinectJoint {
  static constexpr int TypeCount = 25;

  FKinectJointType type;
  FKinectTrackingState trackingState;
  FVector location;
};

struct FKinectGesture {
  static constexpr int Max = 16;

  bool bDetected = false;
  FString name;
  FKinectGestureType type = FKinectGestureType::None;
  float confidence = 0.f;

  void Reset() {
    bDetected = false;
    name.Reset();
    type = FKinectGestureType::None;
    confidence = 0.f;
  }
};

struct FKinectBody {
  static constexpr int Count = 6;

  bool bValid = false;
  FKinectJoint joints[FKinectJoint::TypeCount];
  TArray<FKinectGesture> gestures;
};
//
//template<typename T>
//struct TKinectDefaultUnrefer {};

template <typename T>
struct TKinectDefaultRefer {
  inline static void AddRef(T* ptr) {
    ptr->AddRef();
  }
  inline static void Unref(T* ptr) {
    ptr->Release();
  }
};

template <typename T>
struct TKinectDefaultReferWithClose {
  inline static void AddRef(T* ptr) {
    ptr->AddRef();
  }
  inline static void Unref(T* ptr) {
    ptr->Close();
    ptr->Release();
  }
};

template <typename T, typename TRefer = TKinectDefaultRefer<T>>
struct TKinectComPtr {
  TKinectComPtr() = default;
  TKinectComPtr(T* ptr) :
    _ptr(ptr)
  {
  }

  TKinectComPtr(const TKinectComPtr& rhs) :
    _ptr(rhs._ptr)
  {
    if (_ptr) {
      TRefer::AddRef(_ptr);
    }
  }

  TKinectComPtr(TKinectComPtr&& rhs) :
    _ptr(rhs._ptr)
  {
    rhs._ptr = nullptr;
  }

  TKinectComPtr& operator=(T* ptr) {
    if (_ptr) {
      TRefer::Unref(_ptr);
    }
    _ptr = ptr;
    return *this;
  }

  TKinectComPtr& operator=(const TKinectComPtr& rhs) {
    if (_ptr) {
      TRefer::Unref(_ptr);
    }
    _ptr = rhs._ptr;
    if (_ptr) {
      TRefer::AddRef(_ptr);
    }
    return *this;
  }

  TKinectComPtr& operator=(TKinectComPtr&& rhs) {
    if (_ptr) {
      TRefer::Unref(_ptr);
    }
    _ptr = rhs._ptr;
    rhs._ptr = nullptr;
    return *this;
  }

  ~TKinectComPtr() {
    if (_ptr) {
      TRefer::Unref(_ptr);
    }
  }

  /*T* operator*() const {
    return _ptr;
  }*/

  T* operator->() const {
    return _ptr;
  }

  T* Get() const {
    return _ptr;
  }

  T** operator&() {
    return &_ptr;
  }

  void Reset() {
    if (_ptr) {
      TRefer::Unref(_ptr);
      _ptr = nullptr;
    }
  }

  operator bool() const {
    return _ptr != nullptr;
  }

  T* _ptr = nullptr;
};

template<typename T, typename TRefer = TKinectDefaultRefer<T>>
struct TKinectUniqueComPtr : public TKinectComPtr<T, TRefer> {
  TKinectUniqueComPtr() : TKinectComPtr<T, TRefer>() {}
  TKinectUniqueComPtr(const TKinectUniqueComPtr&) = delete;
  TKinectUniqueComPtr& operator=(const TKinectUniqueComPtr&) = delete;
  TKinectUniqueComPtr& operator=(T* ptr) = delete;
  TKinectUniqueComPtr& operator=(TKinectUniqueComPtr&& rhs) {
    TKinectComPtr<T, TRefer>::operator=(std::forward<TKinectUniqueComPtr>(rhs));
    return *this;
  }
};


class KINECTUE4_API FKinectUE4Module : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
  void StartupKinect(const FString& gdbFilePath);
  void ShutdownKinect();
  /*void InstallGestureDatabase(const FString& Path);
  void UninstallGestureDatabase();*/
  //void AcquireLatestColorFrame();
  bool AcquireLatestBodyFrame(FKinectBody*& out_bodies, bool bAcquireJoint = true, bool bAcquireGesture = false);
  //bool AcquireLatestGestureFrame();

public:
  bool bKinectStartup = false;
  TKinectUniqueComPtr<struct IKinectSensor, TKinectDefaultReferWithClose<struct IKinectSensor>> _kinectSensor;
  TKinectComPtr<struct IBodyFrameReader> _bodyFrameReader;
  //struct IColorFrameReader* _colorFrameReader = nullptr;

  UINT _numOfGestures = 0;
  TKinectComPtr<struct IGesture> _gestures[FKinectGesture::Max];
  TKinectComPtr<struct IVisualGestureBuilderFrameSource> _gestureSources[FKinectBody::Count];
  TKinectComPtr<struct IVisualGestureBuilderFrameReader> _gestureReaders[FKinectBody::Count];

private:
  FKinectBody _bodies[FKinectBody::Count];
};
