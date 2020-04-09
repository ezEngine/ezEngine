#include <ParticlePluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezParticleFinisherComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezHiddenAttribute,
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezParticleFinisherComponent::ezParticleFinisherComponent() = default;
ezParticleFinisherComponent::~ezParticleFinisherComponent() = default;

void ezParticleFinisherComponent::OnDeactivated()
{
  m_EffectController.StopImmediate();

  ezRenderComponent::OnDeactivated();
}

ezResult ezParticleFinisherComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_EffectController.IsAlive())
  {
    ezBoundingBoxSphere volume;
    m_uiBVolumeUpdateCounter = m_EffectController.GetBoundingVolume(volume);

    if (m_uiBVolumeUpdateCounter != 0)
    {
      bounds.ExpandToInclude(volume);
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

void ezParticleFinisherComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // do not extract particles during shadow map rendering
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  m_EffectController.SetIsInView();
}

void ezParticleFinisherComponent::Update()
{
  if (m_EffectController.IsAlive())
  {
    CheckBVolumeUpdate();
  }
  else
  {
    GetOwner()->GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
  }
}

void ezParticleFinisherComponent::CheckBVolumeUpdate()
{
  ezBoundingBoxSphere bvol;
  if (m_uiBVolumeUpdateCounter < m_EffectController.GetBoundingVolume(bvol))
  {
    TriggerLocalBoundsUpdate();
  }
}
