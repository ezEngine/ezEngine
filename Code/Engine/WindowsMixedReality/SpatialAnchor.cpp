﻿#includde < WindowsMixedRealityPCH.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialAnchor.h>
#include <WindowsMixedReality/SpatialLocationService.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>

#include <windows.foundation.collections.h>
#include <windows.perception.spatial.h>
#include <wrl/event.h>

ezWindowsSpatialAnchor::ezWindowsSpatialAnchor(const ComPtr<ABI::Windows::Perception::Spatial::ISpatialAnchor>& pSpatialAnchor)
  : m_pSpatialAnchor(pSpatialAnchor)
{
}

ezWindowsSpatialAnchor::~ezWindowsSpatialAnchor() {}

ezResult ezWindowsSpatialAnchor::GetInternalCoordinateSystem(
  ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem>& outCoordinateSystem) const
{
  EZ_HRESULT_TO_FAILURE_LOG(m_pSpatialAnchor->get_CoordinateSystem(outCoordinateSystem.GetAddressOf()));
  return EZ_SUCCESS;
}

ezResult ezWindowsSpatialAnchor::PersistCurrentLocation(const char* szID)
{
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();
  if (pHoloSpace == nullptr)
    return EZ_FAILURE;

  return pHoloSpace->GetSpatialLocationService().SavePersistentAnchor(*this, szID);
}

// static
ezUniquePtr<ezWindowsSpatialAnchor> ezWindowsSpatialAnchor::LoadPersistedLocation(const char* szID)
{
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();
  if (pHoloSpace == nullptr)
    return nullptr;

  return pHoloSpace->GetSpatialLocationService().LoadPersistentAnchor(szID);
}
