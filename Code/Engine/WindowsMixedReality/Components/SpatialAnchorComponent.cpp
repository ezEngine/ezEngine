#include <PCH.h>
#include <WindowsMixedReality/Components/SpatialAnchorComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialLocationService.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>
#include <windows.perception.spatial.h>
#include <Foundation/Profiling/Profiling.h>
#include <WindowsMixedReality/SpatialAnchor.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezMrSpatialAnchorComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("PersistentName", GetPersistentAnchorName, SetPersistentAnchorName),
  }
  EZ_END_PROPERTIES
}
EZ_END_COMPONENT_TYPE

ezMrSpatialAnchorComponent::ezMrSpatialAnchorComponent() { }
ezMrSpatialAnchorComponent::~ezMrSpatialAnchorComponent() { }

void ezMrSpatialAnchorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_sAnchorName;
}

void ezMrSpatialAnchorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_sAnchorName;
}

void ezMrSpatialAnchorComponent::SetPersistentAnchorName(const char* szName)
{
  m_sAnchorName = szName;
}

const char* ezMrSpatialAnchorComponent::GetPersistentAnchorName() const
{
  return m_sAnchorName;
}

ezResult ezMrSpatialAnchorComponent::RecreateAnchorAt(const ezTransform& position)
{
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();
  auto pNewAnchor = pHoloSpace->GetSpatialLocationService().CreateSpatialAnchor(position);

  if (pNewAnchor == nullptr)
    return EZ_FAILURE;

  m_pSpatialAnchor = std::move(pNewAnchor);
  PersistCurrentLocation();

  return EZ_SUCCESS;
}

void ezMrSpatialAnchorComponent::PersistCurrentLocation()
{
  if (m_sAnchorName.IsEmpty() || m_pSpatialAnchor == nullptr)
    return;

  if (m_pSpatialAnchor->PersistCurrentLocation(m_sAnchorName).Failed())
  {
    ezLog::Error("Failed to persist spatial anchor '{0}'", m_sAnchorName);
  }
}

ezResult ezMrSpatialAnchorComponent::RestorePersistedLocation()
{
  if (m_sAnchorName.IsEmpty())
    return EZ_FAILURE;

  auto pAnchor = ezWindowsSpatialAnchor::LoadPersistedLocation(m_sAnchorName);

  if (pAnchor == nullptr)
  {
    return EZ_FAILURE;
  }

  m_pSpatialAnchor = std::move(pAnchor);
  return EZ_SUCCESS;
}

void ezMrSpatialAnchorComponent::Update()
{
  if (!IsActiveAndSimulating() || m_pSpatialAnchor == nullptr)
    return;

  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem> pReferenceCoords;
  pHoloSpace->GetDefaultReferenceFrame()->GetInternalCoordinateSystem(pReferenceCoords);

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem> pAnchorCoords;
  m_pSpatialAnchor->GetInternalCoordinateSystem(pAnchorCoords);

  ABI::Windows::Foundation::__FIReference_1_Windows__CFoundation__CNumerics__CMatrix4x4_t* mat = nullptr;
  if (FAILED(pAnchorCoords->TryGetTransformTo(pReferenceCoords.Get(), &mat)) || mat == nullptr)
    return;

  ezMat4 mFinal = ezUwpUtils::ConvertMat4(mat);
  
  ezVec3 vPos = mFinal.GetTranslationVector();
  ezMath::Swap(vPos.y, vPos.z); // Y up in other coordinate system
  
  ezQuat qRot;
  qRot.SetIdentity();

  /// \todo Rotation somehow always returns non-identify when loading a persistent anchor
  //qRot.SetFromMat3(mFinal.GetRotationalPart());

  GetOwner()->SetGlobalTransform(ezTransform(vPos, qRot));
}

void ezMrSpatialAnchorComponent::OnSimulationStarted()
{
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();

  RestorePersistedLocation();

  if (m_pSpatialAnchor == nullptr)
  {
    RecreateAnchorAt(GetOwner()->GetGlobalTransform());
  }
}

void ezMrSpatialAnchorComponent::OnDeactivated()
{
  PersistCurrentLocation();
}

