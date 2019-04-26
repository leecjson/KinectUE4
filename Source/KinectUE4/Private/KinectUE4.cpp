// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#include "KinectUE4.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif

#include <algorithm>
#pragma warning (push)
#pragma warning (disable : 4005)
#include <Kinect.h>
#pragma warning (pop)
#include <Kinect.VisualGestureBuilder.h>

static_assert(FKinectBody::Count == BODY_COUNT, "invalid BODY_COUNT setup");
static_assert(FKinectJoint::TypeCount == JointType_Count, "invalid JointType_Count setup");

#define LOCTEXT_NAMESPACE "FKinectUE4Module"

void FKinectUE4Module::StartupModule() {
  //InstallKinect();
}

void FKinectUE4Module::ShutdownModule() {
  //UninstallKinect();
}

void FKinectUE4Module::StartupKinect(const FString& gdbFilePath) {
  if (bKinectStartup) {
    return;
  }

  TKinectUniqueComPtr<IKinectSensor, TKinectDefaultReferWithClose<struct IKinectSensor>> kinectSensor;
  if (FAILED(GetDefaultKinectSensor(&kinectSensor))) {
    UE_LOG(LogTemp, Error, TEXT("FAILED(GetDefaultKinectSensor(&_kinectSensor))"));
    return;
  }
  if (FAILED(kinectSensor->Open())) {
    UE_LOG(LogTemp, Error, TEXT("FAILED(kinectSensor->Open())"));
    return;
  }

  TKinectComPtr<IBodyFrameSource> bodyFrameSource;
  if (FAILED(kinectSensor->get_BodyFrameSource(&bodyFrameSource))) {
    UE_LOG(LogTemp, Error, TEXT("FAILED(kinectSensor->get_BodyFrameSource(&bodyFrameSource))"));
    return;
  }
  TKinectComPtr<IBodyFrameReader> bodyFrameReader;
  if (FAILED(bodyFrameSource->OpenReader(&bodyFrameReader))) {
    UE_LOG(LogTemp, Error, TEXT("FAILED(bodyFrameSource->OpenReader(&bodyFrameReader))"));
    return;
  }



  /*IColorFrameSource* colorFrameSource = nullptr;
  if (SUCCEEDED(_kinectSensor->get_ColorFrameSource(&colorFrameSource))) {
    if (FAILED(colorFrameSource->OpenReader(&_colorFrameReader))) {
      UE_LOG(LogTemp, Log, TEXT("FAILED colorFrameSource->OpenReader(&_colorFrameReader)"));
    }
    colorFrameSource->Release();
  } else {
    UE_LOG(LogTemp, Log, TEXT("FAILED _kinectSensor->get_ColorFrameSource(&colorFrameSource)"));
  }*/

  TKinectComPtr<IVisualGestureBuilderDatabase> gestureDatabase;
  if (FAILED(CreateVisualGestureBuilderDatabaseInstanceFromFile(*gdbFilePath, &gestureDatabase))) {
    UE_LOG(LogTemp, Error, TEXT("FAILED(CreateVisualGestureBuilderDatabaseInstanceFromFile(\"%s\", &gestureDatabase))"), *gdbFilePath);
    return;
  }
  UINT numOfGestures = 0;
  if (FAILED(gestureDatabase->get_AvailableGesturesCount(&numOfGestures))) {
    UE_LOG(LogTemp, Error, TEXT("FAILED(gestureDatabase->get_AvailableGesturesCount(&numOfGestures))"));
    return;
  }
  if (numOfGestures == 0 || numOfGestures > FKinectGesture::Max) {
    UE_LOG(LogTemp, Error, TEXT("numOfGestures == 0 || numOfGestrues > FKinectGesture::Max"));
    return;
  }
  TUniquePtr<IGesture*, TDefaultDelete<IGesture*[]>> tmp_gestures(new IGesture*[numOfGestures]);
  if (FAILED(gestureDatabase->get_AvailableGestures(numOfGestures, tmp_gestures.Get()))) {
    UE_LOG(LogTemp, Error, TEXT("FAILED(gestureDatabase->get_AvailableGestures(numOfGestures, gestures))"));
    return;
  }
  TKinectComPtr<IGesture> gestures[FKinectGesture::Max];
  for (UINT i = 0; i < numOfGestures; ++i) {
    gestures[i] = (tmp_gestures.Get())[i];
    wchar_t gestureName[260];
    if (FAILED(gestures[i]->get_Name(260, gestureName))) {
      UE_LOG(LogTemp, Error, TEXT("FAILED(gestures[i]->get_Name(260, gestureName))"));
      return;
    }
    UE_LOG(LogTemp, Log, TEXT("Gesture: %s"), gestureName);
  }

  
  


  TKinectComPtr<IVisualGestureBuilderFrameSource> gestureSources[BODY_COUNT];
  TKinectComPtr<IVisualGestureBuilderFrameReader> gestureReaders[BODY_COUNT];
  for (int bodyIndex = 0; bodyIndex < BODY_COUNT; ++bodyIndex) {
    auto& gestureSource = gestureSources[bodyIndex];
    if (FAILED(CreateVisualGestureBuilderFrameSource(kinectSensor.Get(), bodyIndex, &gestureSource))) {
      UE_LOG(LogTemp, Error, TEXT("FAILED(CreateVisualGestureBuilderFrameSource(kinectSensor, bodyIndex, &gestureSource))"));
      return;
    }
    gestureSource->AddGestures(numOfGestures, tmp_gestures.Get());
    gestureSource->OpenReader(&gestureReaders[bodyIndex]);
  }
  
  _kinectSensor = MoveTemp(kinectSensor);
  _bodyFrameReader = MoveTemp(bodyFrameReader);
  _numOfGestures = numOfGestures;
  std::copy_n(std::begin(gestures), numOfGestures, std::begin(_gestures));
  std::copy(std::begin(gestureSources), std::end(gestureSources), std::begin(_gestureSources));
  std::copy(std::begin(gestureReaders), std::end(gestureReaders), std::begin(_gestureReaders));

  for (int i = 0; i < BODY_COUNT; ++i) {
    _bodies[i].gestures.SetNum(numOfGestures, true);
  }

  bKinectStartup = true;
}

void FKinectUE4Module::ShutdownKinect() {
  /*if (_colorFrameReader) {
    _colorFrameReader->Release();
  }*/
  if (!bKinectStartup) {
    return;
  }

  for (UINT i = 0; i < _numOfGestures; ++i) {
    _gestures[i].Reset();
  }
  _numOfGestures = 0;

  for (int i = 0; i < BODY_COUNT; ++i) {
    _gestureSources[i].Reset();
    _gestureReaders[i].Reset();
  }

  _bodyFrameReader.Reset();
  _kinectSensor->Close();
  _kinectSensor.Reset();

  bKinectStartup = false;
}

//void FKinectUE4Module::InstallGestureDatabase(const FString& Path) {
//  if (!_kinectSensor)
//    return;
//
//  IVisualGestureBuilderDatabase* gestureDatabase = nullptr;
//  if (FAILED(CreateVisualGestureBuilderDatabaseInstanceFromFile(*Path, &gestureDatabase))) {
//    UE_LOG(LogTemp, Error, TEXT("FAILED(CreateVisualGestureBuilderDatabaseInstanceFromFile(\"%s\", &gestureDatabase))"), *Path);
//    return;
//  }
//
//  //UINT numGesture = 0;
//  gestureDatabase->get_AvailableGesturesCount(&_numOfGestures);
//
//  _gestures = new IGesture*[_numOfGestures];
//  gestureDatabase->get_AvailableGestures(_numOfGestures, _gestures);
//
//  //IVisualGestureBuilderFrameSource** gestureSources = new IVisualGestureBuilderFrameSource*[BODY_COUNT];
//  ////IVisualGestureBuilderFrameSource* gestureSource = nullptr;
//  //IVisualGestureBuilderFrameReader** gestureReaders = new IVisualGestureBuilderFrameReader*[BODY_COUNT];
//  for (int bodyIndex = 0; bodyIndex < BODY_COUNT; ++bodyIndex) {
//    auto& gestureSource = _gestureSources[bodyIndex];
//    CreateVisualGestureBuilderFrameSource(_kinectSensor, bodyIndex, &gestureSource);
//    gestureSource->AddGestures(_numOfGestures, _gestures);
//    gestureSource->OpenReader(&_gestureReaders[bodyIndex]);
//  }
//
//  /*for (UINT i = 0; i < _numOfGestures; ++i) {
//    if (_gestures[i])
//      gestures[i]->Release();
//  }
//  delete[] gestures;
//*/
//  if (gestureDatabase) {
//    gestureDatabase->Release();
//  }
//
//  for (int i = 0; i < BODY_COUNT; ++i) {
//    _bodies[i].gestures.SetNum(_numOfGestures, true);
//  }
//}
//
//void FKinectUE4Module::UninstallGestureDatabase() {
//  for (UINT i = 0; i < _numOfGestures; ++i) {
//    if (_gestures[i])
//      _gestures[i]->Release();
//  }
//  delete[] _gestures;
//
//  for (int i = 0; i < FKinectBody::Count; ++i) {
//    _gestureReaders[i]->Release();
//    _gestureReaders[i] = nullptr;
//    _gestureSources[i]->Release();
//    _gestureSources[i] = nullptr;
//  }
//}

bool FKinectUE4Module::AcquireLatestBodyFrame(FKinectBody*& out_bodies, bool bAcquireJoint, bool bAcquireGesture) {
  if (!_bodyFrameReader) {
    return false;
  }

  TKinectComPtr<IBodyFrame> bodyFrame = nullptr;
  TKinectComPtr<IBody> bodies[BODY_COUNT] = { nullptr };
  HRESULT hr = _bodyFrameReader->AcquireLatestFrame(&bodyFrame);
  if (hr == E_PENDING) {
    return false;
  } else if (FAILED(hr)) {
    UE_LOG(LogTemp, Error, TEXT("FAILED(_bodyFrameReader->AcquireLatestFrame(&bodyFrame))"));
    return false;
  }
  IBody* tmp_bodies[BODY_COUNT] = { nullptr };
  if (FAILED(bodyFrame->GetAndRefreshBodyData(BODY_COUNT, tmp_bodies))) {
    UE_LOG(LogTemp, Error, TEXT("FAILED(bodyFrame->GetAndRefreshBodyData(BODY_COUNT, tmp_bodies))"));
    return false;
  }
  for (int i = 0; i < BODY_COUNT; ++i) {
    bodies[i] = tmp_bodies[i];
  }
  for (int i = 0; i < BODY_COUNT; ++i) {
    const TKinectComPtr<IBody>& body = bodies[i];
    if (body) {
      BOOLEAN bTracked = false;
      if (FAILED(body->get_IsTracked(&bTracked))) {
        UE_LOG(LogTemp, Error, TEXT("FAILED(body->get_IsTracked(&bTracked))"));
        return false;
      }
      auto& wrapped_body = _bodies[i];
      wrapped_body.bValid = (bool)bTracked;
      if (bTracked) {
        if (bAcquireJoint) { // Joint
          Joint joints[JointType_Count];
          if (FAILED(body->GetJoints(JointType_Count, joints))) {
            UE_LOG(LogTemp, Error, TEXT("FAILED(body->GetJoints(JointType_Count, joints))"));
            return false;
          }
          for (int j = 0; j < JointType_Count; ++j) {
            const auto& joint = joints[j];
            auto& wrapped_joint = wrapped_body.joints[j];
            wrapped_joint.type = static_cast<decltype(wrapped_joint.type)>(joint.JointType);
            wrapped_joint.trackingState = static_cast<decltype(wrapped_joint.trackingState)>(joint.TrackingState);
            wrapped_joint.location = FVector(joint.Position.Z, -joint.Position.X, joint.Position.Y) * 100.f;
          }
        }
        if (bAcquireGesture) { // Gesture
          for (UINT gestureIdx = 0; gestureIdx < _numOfGestures; ++gestureIdx) {
            wrapped_body.gestures[gestureIdx].Reset();
          }
          UINT64 trackingId = _UI64_MAX;
          if (FAILED(body->get_TrackingId(&trackingId))) {
            UE_LOG(LogTemp, Error, TEXT("FAILED(body->get_TrackingId(&trackingId))"));
            return false;
          }
          UINT64 gestureId = _UI64_MAX;
          if (FAILED(_gestureSources[i]->get_TrackingId(&gestureId))) {
            UE_LOG(LogTemp, Error, TEXT("FAILED(_gestureSources[i]->get_TrackingId(&gestureId))"));
            return false;
          }
          if (trackingId != gestureId) {
            if (FAILED(_gestureSources[i]->put_TrackingId(trackingId))) {
              UE_LOG(LogTemp, Error, TEXT("FAILED(_gestureSources[i]->put_TrackingId(trackingId))"));
              return false;
            }
            UE_LOG(LogTemp, Error, TEXT("Put TrackingId: %llu"), trackingId);
            //continue;
          }
          TKinectComPtr<IVisualGestureBuilderFrame> gestureFrame;
          hr = _gestureReaders[i]->CalculateAndAcquireLatestFrame(&gestureFrame);
          if (hr == E_PENDING) {
            return false;
          } else if (FAILED(hr)) {
            UE_LOG(LogTemp, Error, TEXT("FAILED(_gestureReaders[i]->CalculateAndAcquireLatestFrame(&gestureFrame))"));
            return false;
          }
          BOOLEAN bGestureTracked = false;
          if (FAILED(gestureFrame->get_IsTrackingIdValid(&bGestureTracked))) {
            UE_LOG(LogTemp, Error, TEXT("FAILED(gestureFrame->get_IsTrackingIdValid(&bGestureTracked))"));
            return false;
          }
          if (bGestureTracked) {
            static int oN = 1;
            UE_LOG(LogTemp, Log, TEXT("In ----------------- %d"), oN++);

            for (UINT gestureIdx = 0; gestureIdx < _numOfGestures; ++gestureIdx) {
              auto& wrapped_gesture = wrapped_body.gestures[gestureIdx];
              GestureType gestureType;
              if (FAILED(_gestures[gestureIdx]->get_GestureType(&gestureType))) {
                UE_LOG(LogTemp, Error, TEXT("FAILED(_gestures[gestureIdx]->get_GestureType(&gestureType))"));
                return false;
              }
              wrapped_gesture.type = static_cast<decltype(wrapped_gesture.type)>(gestureType);
              if (gestureType == GestureType::GestureType_Discrete) {
                TKinectComPtr<IDiscreteGestureResult> gestureResult = nullptr;
                if (FAILED(gestureFrame->get_DiscreteGestureResult(_gestures[gestureIdx].Get(), &gestureResult))) {
                  UE_LOG(LogTemp, Error, TEXT("FAILED(gestureFrame->get_DiscreteGestureResult(_gestures[gestureIdx], &gestureResult))"));
                  return false;
                }
                wchar_t gestureName[260];
                if (FAILED(_gestures[gestureIdx]->get_Name(260, gestureName))) {
                  UE_LOG(LogTemp, Error, TEXT("FAILED(_gestures[gestureIdx]->get_Name(260, gestureName))"));
                  return false;
                }
                wrapped_gesture.name = FString(gestureName);
                BOOLEAN detected = false;
                if (FAILED(gestureResult->get_Detected(&detected))) {
                  UE_LOG(LogTemp, Error, TEXT("FAILED(gestureResult->get_Detected(&detected))"));
                  return false;
                }
                wrapped_gesture.bDetected = detected;
                if (detected) {
                  float confidence = 0.0f;
                  if (FAILED(gestureResult->get_Confidence(&confidence))) {
                    UE_LOG(LogTemp, Error, TEXT("FAILED(gestureResult->get_Confidence(&confidence))"));
                    return false;
                  }
                  wrapped_gesture.confidence = confidence;

                  //if (confidence > 0.8f) {
                    UE_LOG(LogTemp, Log, TEXT("Gesture: %s          confidence: %f"), gestureName, confidence);
                  //}
                }
              } else if (gestureType == GestureType::GestureType_Continuous) {

              } else {
                check(false);
                return false;
              }
            }
          }
        }
      }
    } else {
      _bodies[i].bValid = false;
    }
  }
  out_bodies = _bodies;
  return true;
}

//bool FKinectUE4Module::AcquireLatestGestureFrame() {
//  if (!_bodyFrameReader) {
//    return false;
//  }
//
//  IBodyFrame* bodyFrame = nullptr;
//  IBody* bodies[BODY_COUNT] = { 0 };
//
//  auto _acquire = [&]() {
//    HRESULT hr = _bodyFrameReader->AcquireLatestFrame(&bodyFrame);
//    if (hr == E_PENDING) {
//      return false;
//    }
//    else if (FAILED(hr)) {
//      UE_LOG(LogTemp, Error, TEXT("FAILED(_bodyFrameReader->AcquireLatestFrame(&bodyFrame))"));
//      return false;
//    }
//    if (FAILED(bodyFrame->GetAndRefreshBodyData(BODY_COUNT, bodies))) {
//      UE_LOG(LogTemp, Error, TEXT("FAILED(bodyFrame->GetAndRefreshBodyData(BODY_COUNT, bodies))"));
//      return false;
//    }
//    for (int i = 0; i < BODY_COUNT; ++i) {
//      IBody* body = bodies[i];
//      if (body) {
//        BOOLEAN bTracked = false;
//        if (FAILED(body->get_IsTracked(&bTracked))) {
//          UE_LOG(LogTemp, Error, TEXT("FAILED(body->get_IsTracked(&bTracked))"));
//          return false;
//        }
//        if (bTracked) {
//          UINT64 trackingId = _UI64_MAX;
//          if (SUCCEEDED(body->get_TrackingId(&trackingId))) {
//            _gestureSources[i]->put_TrackingId(trackingId);
//          }
//        }
//      }
//    }
//    return true;
//  };
//
//  bool ret = false;
//  if (_acquire()) {
//    ret = true;
//  }
//  for (int i = 0; i < BODY_COUNT; ++i) {
//    auto& body = bodies[i];
//    if (body) {
//      body->Release();
//    }
//  }
//  if (bodyFrame) {
//    bodyFrame->Release();
//  }
//  return ret;
//}

//void FKinectUE4Module::AcquireLatestColorFrame() {
//  if (!_colorFrameReader) {
//    return;
//  }
//
//  IColorFrame* colorFrame = nullptr;
//  HRESULT hr = _colorFrameReader->AcquireLatestFrame(&colorFrame);
//  if (SUCCEEDED(hr)) {
//    INT64 time = 0;
//    IFrameDescription* frameDescription = nullptr;
//    int width = 0;
//    int height = 0;
//    ColorImageFormat imageFormat = ColorImageFormat_None;
//    UINT bufferSize = 0;
//    RGBQUAD *buffer = nullptr;
//
//    do {
//      hr = colorFrame->get_RelativeTime(&time);
//      if (FAILED(hr)) {
//        break;
//      }
//      hr = colorFrame->get_FrameDescription(&frameDescription);
//      if (FAILED(hr)) {
//        break;
//      }
//      hr = frameDescription->get_Width(&width);
//      if (FAILED(hr)) {
//        break;
//      }
//      hr = frameDescription->get_Height(&height);
//      if (FAILED(hr)) {
//        break;
//      }
//      hr = colorFrame->get_RawColorImageFormat(&imageFormat);
//      if (FAILED(hr)) {
//        break;
//      }
//      
//      if (imageFormat == ColorImageFormat_Bgra)
//      {
//        hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
//      }
//      else if (m_pColorRGBX)
//      {
//        pBuffer = m_pColorRGBX;
//        nBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
//        hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
//      }
//      else
//      {
//        hr = E_FAIL;
//      }
//    } while (false);
//    
//    if (frameDescription) {
//      frameDescription->Release();
//    }
//    colorFrame->Release();
//  }
//}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FKinectUE4Module, KinectUE4)


