#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/SpatialAnchorComponent.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSpatialAnchorComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("XR"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSpatialAnchorComponent::ezSpatialAnchorComponent() = default;
ezSpatialAnchorComponent::~ezSpatialAnchorComponent()
{
  if (ezXRSpatialAnchorsInterface* pXR = ezSingletonRegistry::GetSingletonInstance<ezXRSpatialAnchorsInterface>())
  {
    if (!m_AnchorID.IsInvalidated())
    {
      pXR->DestroyAnchor(m_AnchorID);
      m_AnchorID = ezXRSpatialAnchorID();
    }
  }
}

void ezSpatialAnchorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();
}

void ezSpatialAnchorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();
  if (uiVersion == 1)
  {
    ezString sAnchorName;
    s >> sAnchorName;
  }
}

ezResult ezSpatialAnchorComponent::RecreateAnchorAt(const ezTransform& position)
{
  if (ezXRSpatialAnchorsInterface* pXR = ezSingletonRegistry::GetSingletonInstance<ezXRSpatialAnchorsInterface>())
  {
    if (!m_AnchorID.IsInvalidated())
    {
      pXR->DestroyAnchor(m_AnchorID);
      m_AnchorID = ezXRSpatialAnchorID();
    }

    m_AnchorID = pXR->CreateAnchor(position);
    return m_AnchorID.IsInvalidated() ? EZ_FAILURE : EZ_SUCCESS;
  }

  return EZ_SUCCESS;
}

void ezSpatialAnchorComponent::Update()
{
  if (IsActiveAndSimulating())
  {
    if (ezXRSpatialAnchorsInterface* pXR = ezSingletonRegistry::GetSingletonInstance<ezXRSpatialAnchorsInterface>())
    {
      if (!m_AnchorID.IsInvalidated())
      {
        ezTransform globalTransform;
        if (pXR->TryGetAnchorTransform(m_AnchorID, globalTransform).Succeeded())
        {
          globalTransform.m_vScale = GetOwner()->GetGlobalScaling();
          GetOwner()->SetGlobalTransform(globalTransform);
        }
      }
      else
      {
        m_AnchorID = pXR->CreateAnchor(GetOwner()->GetGlobalTransform());
      }
    }
  }
}

void ezSpatialAnchorComponent::OnSimulationStarted()
{
  if (ezXRSpatialAnchorsInterface* pXR = ezSingletonRegistry::GetSingletonInstance<ezXRSpatialAnchorsInterface>())
  {
    m_AnchorID = pXR->CreateAnchor(GetOwner()->GetGlobalTransform());
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_SpatialAnchorComponent);
