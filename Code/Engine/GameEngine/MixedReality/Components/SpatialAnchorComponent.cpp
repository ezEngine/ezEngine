#include <PCH.h>
#include <GameEngine/MixedReality/Components/SpatialAnchorComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialLocationService.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>
#include <Foundation/Profiling/Profiling.h>
#include <WindowsMixedReality/SpatialAnchor.h>

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
#include <windows.perception.spatial.h>
#endif

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezSpatialAnchorComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("PersistentName", GetPersistentAnchorName, SetPersistentAnchorName),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Mixed Reality"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE

ezSpatialAnchorComponent::ezSpatialAnchorComponent() { }
ezSpatialAnchorComponent::~ezSpatialAnchorComponent() { }

void ezSpatialAnchorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_sAnchorName;
}

void ezSpatialAnchorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_sAnchorName;
}

void ezSpatialAnchorComponent::SetPersistentAnchorName(const char* szName)
{
  m_sAnchorName = szName;
}

const char* ezSpatialAnchorComponent::GetPersistentAnchorName() const
{
  return m_sAnchorName;
}

ezResult ezSpatialAnchorComponent::RecreateAnchorAt(const ezTransform& position)
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();
  if (pHoloSpace == nullptr)
    return EZ_FAILURE;

  auto pNewAnchor = pHoloSpace->GetSpatialLocationService().CreateSpatialAnchor(position);

  if (pNewAnchor == nullptr)
    return EZ_FAILURE;

  m_pSpatialAnchor = std::move(pNewAnchor);
  PersistCurrentLocation();
#endif

  return EZ_SUCCESS;
}

void ezSpatialAnchorComponent::PersistCurrentLocation()
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (m_sAnchorName.IsEmpty() || m_pSpatialAnchor == nullptr)
    return;

  if (m_pSpatialAnchor->PersistCurrentLocation(m_sAnchorName).Failed())
  {
    ezLog::Error("Failed to persist spatial anchor '{0}'", m_sAnchorName);
  }
#endif
}

ezResult ezSpatialAnchorComponent::RestorePersistedLocation()
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (m_sAnchorName.IsEmpty())
    return EZ_FAILURE;

  auto pAnchor = ezWindowsSpatialAnchor::LoadPersistedLocation(m_sAnchorName);

  if (pAnchor == nullptr)
  {
    return EZ_FAILURE;
  }

  m_pSpatialAnchor = std::move(pAnchor);
#endif

  return EZ_SUCCESS;
}

void ezSpatialAnchorComponent::Update()
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
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
  ezMat4 mFinalFinal;
  mFinalFinal.SetColumn(0, mFinal.GetColumn(0));
  mFinalFinal.SetColumn(1, -mFinal.GetColumn(2));
  mFinalFinal.SetColumn(2, mFinal.GetColumn(1));
  mFinalFinal.SetColumn(3, mFinal.GetColumn(3));

  ezVec3 vPos = mFinalFinal.GetTranslationVector();
  ezMath::Swap(vPos.y, vPos.z); // Y up in other coordinate system

  ezQuat qRot;
  //qRot.SetIdentity();
  qRot.SetFromMat3(mFinalFinal.GetRotationalPart());

  GetOwner()->SetGlobalTransform(ezTransform(vPos, qRot));
#endif
}

void ezSpatialAnchorComponent::OnSimulationStarted()
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();

  RestorePersistedLocation();

  if (m_pSpatialAnchor == nullptr)
  {
    RecreateAnchorAt(GetOwner()->GetGlobalTransform());
  }
#endif
}

void ezSpatialAnchorComponent::OnDeactivated()
{
  PersistCurrentLocation();
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_MixedReality_Components_SpatialAnchorComponent);

