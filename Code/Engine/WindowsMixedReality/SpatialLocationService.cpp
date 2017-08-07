#include <PCH.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialLocationService.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>
#include <WindowsMixedReality/SpatialAnchor.h>

#include <windows.perception.spatial.h>
#include <windows.foundation.collections.h>
#include <wrl/event.h>

using namespace ABI::Windows::Foundation::Numerics;
using namespace ABI::Windows::Perception::Spatial;

ezWindowsSpatialLocationService::ezWindowsSpatialLocationService(const ComPtr<ISpatialLocator>& pSpatialLocator)
  : m_pSpatialLocator(pSpatialLocator)
  , m_currentLocatability(ezSpatialLocatability::Unavailable)
{
  using OnLocatabilityChangedFunc = __FITypedEventHandler_2_Windows__CPerception__CSpatial__CSpatialLocator_IInspectable;
  if (FAILED(m_pSpatialLocator->add_LocatabilityChanged(Callback<OnLocatabilityChangedFunc>(this, &ezWindowsSpatialLocationService::OnLocatabilityChanged).Get(), &m_eventRegistrationLocatabilityChanged)))
  {
    ezLog::Error("Failed to subscribe for locatability changes on spatial locator.");
  }

  LoadSpatialAnchorMap();

  // Update internal state.
  OnLocatabilityChanged(m_pSpatialLocator.Get(), nullptr);
}

ezWindowsSpatialLocationService::~ezWindowsSpatialLocationService()
{
  if (m_pSpatialLocator)
    m_pSpatialLocator->remove_LocatabilityChanged(m_eventRegistrationLocatabilityChanged);
}

HRESULT ezWindowsSpatialLocationService::OnLocatabilityChanged(ISpatialLocator* locator, IInspectable* args)
{
  SpatialLocatability locatability;
  EZ_HRESULT_TO_FAILURE_LOG(locator->get_Locatability(&locatability));

  switch (locatability)
  {
  case SpatialLocatability::SpatialLocatability_Unavailable:
    m_currentLocatability = ezSpatialLocatability::Unavailable;
    ezLog::Warning("Spatial locator unavailable, can't place holograms!");
    break;

  case SpatialLocatability::SpatialLocatability_OrientationOnly:
    m_currentLocatability = ezSpatialLocatability::OrientationOnly;
    ezLog::Debug("Spatial locator is orientation only - the system is preparing to use positional tracking.");
    break;

  case SpatialLocatability::SpatialLocatability_PositionalTrackingActivating:
    m_currentLocatability = ezSpatialLocatability::PositionalTrackingActivating;
    ezLog::Debug("Spatial locator is activating positional tracking.");
    break;

  case SpatialLocatability::SpatialLocatability_PositionalTrackingActive:
    m_currentLocatability = ezSpatialLocatability::PositionalTrackingActive;
    ezLog::Debug("Spatial locator fully functional.");
    break;

  case SpatialLocatability::SpatialLocatability_PositionalTrackingInhibited:
    m_currentLocatability = ezSpatialLocatability::PositionalTrackingInhibited;
    ezLog::Warning("Positional tracking is temporarily inhibited. User action may be required in order to restore positional tracking..");
    break;
  }

  return S_OK;
}

ezUniquePtr<ezWindowsSpatialReferenceFrame> ezWindowsSpatialLocationService::CreateStationaryReferenceFrame_CurrentLocation()
{
  ComPtr<ISpatialStationaryFrameOfReference> pFrame;
  HRESULT result = m_pSpatialLocator->CreateStationaryFrameOfReferenceAtCurrentLocation(pFrame.GetAddressOf());
  if (FAILED(result))
  {
    ezLog::Error("Failed to create stationary spatial reference frame at current position: {0}", ezHRESULTtoString(result));
    return nullptr;
  }

  return EZ_DEFAULT_NEW(ezWindowsSpatialReferenceFrame, pFrame);
}

ezUniquePtr<ezWindowsSpatialReferenceFrame> ezWindowsSpatialLocationService::CreateStationaryReferenceFrame_CurrentHeading(const ezWindowsSpatialReferenceFrame& origin, const ezTransform& offset)
{
  ComPtr<ISpatialCoordinateSystem> pOriginCoords;
  origin.GetInternalCoordinateSystem(pOriginCoords);

  ComPtr<ISpatialStationaryFrameOfReference> pCurFrame;
  m_pSpatialLocator->CreateStationaryFrameOfReferenceAtCurrentLocation(pCurFrame.GetAddressOf());

  ComPtr<ISpatialCoordinateSystem> pCurCoords;
  pCurFrame->get_CoordinateSystem(&pCurCoords);

  ComPtr<__FIReference_1_Windows__CFoundation__CNumerics__CMatrix4x4> pMatCurToOrigin;
  pOriginCoords->TryGetTransformTo(pCurCoords.Get(), &pMatCurToOrigin);

  const ezMat4 mCurToOrigin = ezUwpUtils::ConvertMat4(pMatCurToOrigin.Get());

  Vector3 vCurToDest;
  ezUwpUtils::ConvertVec3(mCurToOrigin * offset.m_vPosition, vCurToDest);

  Quaternion qCurToDest;
  ezUwpUtils::ConvertQuat(offset.m_qRotation, qCurToDest);

  ComPtr<ISpatialStationaryFrameOfReference> pFrame;
  HRESULT result = m_pSpatialLocator->CreateStationaryFrameOfReferenceAtCurrentLocationWithPositionAndOrientation(vCurToDest, qCurToDest, pFrame.GetAddressOf());
  if (FAILED(result))
  {
    ezLog::Error("Failed to create stationary spatial reference frame at current position: {0}", ezHRESULTtoString(result));
    return nullptr;
  }

  return EZ_DEFAULT_NEW(ezWindowsSpatialReferenceFrame, pFrame);
}

ezUniquePtr<ezWindowsSpatialReferenceFrame> ezWindowsSpatialLocationService::CreateStationaryReferenceFrame(const ezWindowsSpatialReferenceFrame& origin, const ezTransform& offset)
{
  ComPtr<ISpatialCoordinateSystem> pOriginCoords;
  origin.GetInternalCoordinateSystem(pOriginCoords);

  ComPtr<ISpatialStationaryFrameOfReference> pCurFrame;
  m_pSpatialLocator->CreateStationaryFrameOfReferenceAtCurrentLocation(pCurFrame.GetAddressOf());

  ComPtr<ISpatialCoordinateSystem> pCurCoords;
  pCurFrame->get_CoordinateSystem(&pCurCoords);

  ComPtr<__FIReference_1_Windows__CFoundation__CNumerics__CMatrix4x4> pMatCurToOrigin;
  pOriginCoords->TryGetTransformTo(pCurCoords.Get(), &pMatCurToOrigin);

  const ezMat4 mCurToOrigin = ezUwpUtils::ConvertMat4(pMatCurToOrigin.Get());

  Vector3 vCurToDest;
  ezUwpUtils::ConvertVec3(mCurToOrigin * offset.m_vPosition, vCurToDest);

  const ezMat3 mRot = mCurToOrigin.GetRotationalPart() * offset.m_qRotation.GetAsMat3();
  ezQuat qRot;
  qRot.SetFromMat3(mRot);

  Quaternion qCurToDest;
  ezUwpUtils::ConvertQuat(qRot, qCurToDest);

  ComPtr<ISpatialStationaryFrameOfReference> pFrame;
  HRESULT result = m_pSpatialLocator->CreateStationaryFrameOfReferenceAtCurrentLocationWithPositionAndOrientation(vCurToDest, qCurToDest, pFrame.GetAddressOf());
  if (FAILED(result))
  {
    ezLog::Error("Failed to create stationary spatial reference frame at current position: {0}", ezHRESULTtoString(result));
    return nullptr;
  }

  return EZ_DEFAULT_NEW(ezWindowsSpatialReferenceFrame, pFrame);
}


ezUniquePtr<ezWindowsSpatialReferenceFrame> ezWindowsSpatialLocationService::CreateStationaryReferenceFrame_Rotated(const ezWindowsSpatialReferenceFrame& origin, ezAngle difference)
{
  const ezVec3 vOriginToDest(0.0f);
  const ezQuat qOriginToDest = ezQuat::IdentityQuaternion();

  ComPtr<ISpatialCoordinateSystem> pOriginCoords;
  origin.GetInternalCoordinateSystem(pOriginCoords);

  ComPtr<ISpatialStationaryFrameOfReference> pCurFrame;
  m_pSpatialLocator->CreateStationaryFrameOfReferenceAtCurrentLocation(pCurFrame.GetAddressOf());

  ComPtr<ISpatialCoordinateSystem> pCurCoords;
  pCurFrame->get_CoordinateSystem(&pCurCoords);

  ComPtr<__FIReference_1_Windows__CFoundation__CNumerics__CMatrix4x4> pMatCurToOrigin;
  pOriginCoords->TryGetTransformTo(pCurCoords.Get(), &pMatCurToOrigin);

  const ezMat4 mCurToOrigin = ezUwpUtils::ConvertMat4(pMatCurToOrigin.Get());

  Vector3 vCurToDest;
  ezUwpUtils::ConvertVec3(mCurToOrigin * vOriginToDest, vCurToDest);

  ezQuat qCurToDest0;
  qCurToDest0.SetFromMat3((mCurToOrigin *  qOriginToDest.GetAsMat4()).GetRotationalPart());

  ezQuat qLocalRot;
  qLocalRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), difference);

  Quaternion qCurToDest;
  ezUwpUtils::ConvertQuat(qCurToDest0 * qLocalRot, qCurToDest);

  ComPtr<ISpatialStationaryFrameOfReference> pFrame;
  HRESULT result = m_pSpatialLocator->CreateStationaryFrameOfReferenceAtCurrentLocationWithPositionAndOrientation(vCurToDest, qCurToDest, pFrame.GetAddressOf());
  if (FAILED(result))
  {
    ezLog::Error("Failed to create stationary spatial reference frame at current position: {0}", ezHRESULTtoString(result));
    return nullptr;
  }

  return EZ_DEFAULT_NEW(ezWindowsSpatialReferenceFrame, pFrame);

}

ezUniquePtr<ezWindowsSpatialAnchor> ezWindowsSpatialLocationService::CreateSpatialAnchor(const ezTransform& offset, const ezWindowsSpatialReferenceFrame* pReferenceFrame /*= nullptr*/)
{
  if (pReferenceFrame == nullptr)
  {
    pReferenceFrame = ezWindowsHolographicSpace::GetSingleton()->GetDefaultReferenceFrame();
  }

  ComPtr<ISpatialAnchorStatics> pSpatialAnchorStatics;
  ezUwpUtils::RetrieveStatics(RuntimeClass_Windows_Perception_Spatial_SpatialAnchor, pSpatialAnchorStatics);

  ComPtr<ISpatialCoordinateSystem> pCoordinateSystem;
  pReferenceFrame->GetInternalCoordinateSystem(pCoordinateSystem);

  Numerics::Vector3 position;
  ezUwpUtils::ConvertVec3(offset.m_vPosition, position);

  // convert rotation from ez left-handed to MR OpenGL style right handed coordinate system
  ezMat3 mRot = offset.m_qRotation.GetAsMat3();
  ezMat3 mNewRot;
  mNewRot.SetColumn(0, mRot.GetColumn(0));
  mNewRot.SetColumn(1, mRot.GetColumn(2));
  mNewRot.SetColumn(2, -mRot.GetColumn(1));
  ezQuat qRot;
  qRot.SetFromMat3(mNewRot);

  Numerics::Quaternion rotation;
  ezUwpUtils::ConvertQuat(qRot, rotation);

  ComPtr<ISpatialAnchor> pAnchor;
  if (FAILED(pSpatialAnchorStatics->TryCreateWithPositionRelativeTo(pCoordinateSystem.Get(), position, &pAnchor)))
  {
    return nullptr;
  }

  if (pAnchor == nullptr)
  {
    // it appears this can happen (success earlier doesn't mean it didn't fail)
    return nullptr;
  }

  return EZ_DEFAULT_NEW(ezWindowsSpatialAnchor, pAnchor);
}

void ezWindowsSpatialLocationService::LoadSpatialAnchorMap()
{
  ComPtr<ISpatialAnchorManagerStatics> manager;
  ezUwpUtils::RetrieveStatics(RuntimeClass_Windows_Perception_Spatial_SpatialAnchorManager, manager);

  ComPtr<__FIAsyncOperation_1_Windows__CPerception__CSpatial__CSpatialAnchorStore> pAsyncOp;
  if (SUCCEEDED(manager->RequestStoreAsync(&pAsyncOp)))
  {
    ezUwpUtils::ezWinRtPutCompleted<SpatialAnchorStore*, ComPtr<ISpatialAnchorStore>>
      (pAsyncOp, [this](const ComPtr<ISpatialAnchorStore>& pResult)
    {
      ezLog::Dev("Successfully retrieved spatial anchor storage.");
      m_pStore = pResult;
    });
  }
}

ezResult ezWindowsSpatialLocationService::SavePersistentAnchor(ezWindowsSpatialAnchor& anchor, const char* szID)
{
  if (!m_pStore)
    return EZ_FAILURE;

  /// \todo Without this, saving will fail. Idk if that means you can only store a single persistent anchor (atm) ?
  m_pStore->Clear();

  boolean res = false;
  if (FAILED(m_pStore->TrySave(ezStringHString(szID).GetData().Get(), anchor.m_pSpatialAnchor.Get(), &res)))
    return EZ_FAILURE;

  return (res == TRUE) ? EZ_SUCCESS : EZ_FAILURE;
}

ezUniquePtr<ezWindowsSpatialAnchor> ezWindowsSpatialLocationService::LoadPersistentAnchor(const char* szID)
{
  if (!m_pStore)
    return nullptr;

  ComPtr<__FIMapView_2_HSTRING_Windows__CPerception__CSpatial__CSpatialAnchor> anchorMap;
  if (FAILED(m_pStore->GetAllSavedAnchors(&anchorMap)))
    return nullptr;

  ComPtr<ISpatialAnchor> pSpatialAnchorResult;
  if (FAILED(anchorMap->Lookup(ezStringHString(szID).GetData().Get(), &pSpatialAnchorResult)))
    return nullptr;

  return EZ_DEFAULT_NEW(ezWindowsSpatialAnchor, pSpatialAnchorResult);
}

