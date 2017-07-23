#include <PCH.h>
#include <WindowsMixedReality/SpatialLocationService.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>

#include <windows.perception.spatial.h>
#include <windows.foundation.collections.h>
#include <wrl/event.h>

ezWindowsSpatialLocationService::ezWindowsSpatialLocationService(const ComPtr<ABI::Windows::Perception::Spatial::ISpatialLocator>& pSpatialLocator)
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

HRESULT ezWindowsSpatialLocationService::OnLocatabilityChanged(ABI::Windows::Perception::Spatial::ISpatialLocator* locator, IInspectable* args)
{
  ABI::Windows::Perception::Spatial::SpatialLocatability locatability;
  EZ_HRESULT_TO_FAILURE_LOG(locator->get_Locatability(&locatability));

  switch (locatability)
  {
  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_Unavailable:
    m_currentLocatability = ezSpatialLocatability::Unavailable;
    ezLog::SeriousWarning("Spatial locator unavailable, can't place holograms!");
    break;

  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_OrientationOnly:
    m_currentLocatability = ezSpatialLocatability::OrientationOnly;
    ezLog::Debug("Spatial locator is orientation only - the system is preparing to use positional tracking.");
    break;

  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_PositionalTrackingActivating:
    m_currentLocatability = ezSpatialLocatability::PositionalTrackingActivating;
    ezLog::Debug("Spatial locator is activating positional tracking.");
    break;

  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_PositionalTrackingActive:
    m_currentLocatability = ezSpatialLocatability::PositionalTrackingActive;
    ezLog::Debug("Spatial locator fully functional.");
    break;

  case ABI::Windows::Perception::Spatial::SpatialLocatability::SpatialLocatability_PositionalTrackingInhibited:
    m_currentLocatability = ezSpatialLocatability::PositionalTrackingInhibited;
    ezLog::Warning("Positional tracking is temporarily inhibited. User action may be required in order to restore positional tracking..");
    break;
  }

  return S_OK;
}

ezUniquePtr<ezWindowsSpatialReferenceFrame> ezWindowsSpatialLocationService::CreateStationaryReferenceFrame_CurrentLocation()
{
  ComPtr<ABI::Windows::Perception::Spatial::ISpatialStationaryFrameOfReference> pFrame;
  HRESULT result = m_pSpatialLocator->CreateStationaryFrameOfReferenceAtCurrentLocation(pFrame.GetAddressOf());
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

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialAnchorStatics> pSpatialAnchorStatics;
  ezUwpUtils::RetrieveStatics(RuntimeClass_Windows_Perception_Spatial_SpatialAnchor, pSpatialAnchorStatics);

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem> pCoordinateSystem;
  pReferenceFrame->GetInternalCoordinateSystem(pCoordinateSystem);

  Numerics::Vector3 position;
  ezUwpUtils::ConvertVec3(offset.m_vPosition, position);

  /// \todo Rotation does not seem to work as expected
  Numerics::Quaternion rotation;
  ezUwpUtils::ConvertQuat(offset.m_qRotation, rotation);

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialAnchor> pAnchor;
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
  ComPtr<ABI::Windows::Perception::Spatial::ISpatialAnchorManagerStatics> manager;
  ezUwpUtils::RetrieveStatics(RuntimeClass_Windows_Perception_Spatial_SpatialAnchorManager, manager);

  ComPtr<__FIAsyncOperation_1_Windows__CPerception__CSpatial__CSpatialAnchorStore> storeAsync;
  if (SUCCEEDED(manager->RequestStoreAsync(&storeAsync)))
  {
    storeAsync->put_Completed(Microsoft::WRL::Callback< IAsyncOperationCompletedHandler< ABI::Windows::Perception::Spatial::SpatialAnchorStore* > >(
      [this](IAsyncOperation< ABI::Windows::Perception::Spatial::SpatialAnchorStore* >* pHandler, AsyncStatus status)
    {
      if (SUCCEEDED(pHandler->GetResults(&m_pStore)))
      {
        ezLog::Dev("Successfully retrieved spatial anchor storage.");
      }
      return S_OK;
    }).Get());
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

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialAnchor> pSpatialAnchorResult;
  if (FAILED(anchorMap->Lookup(ezStringHString(szID).GetData().Get(), &pSpatialAnchorResult)))
    return nullptr;

  return EZ_DEFAULT_NEW(ezWindowsSpatialAnchor, pSpatialAnchorResult);
}

