#include <PCH.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezParticleFinisherComponent, 1, ezComponentMode::Static)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //  EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction, m_OnFinishedAction),
  //}
  //EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
  {
    new ezHiddenAttribute,
  }
  EZ_END_ATTRIBUTES;
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS

}
EZ_END_COMPONENT_TYPE


ezParticleFinisherComponent::ezParticleFinisherComponent()
{
}

ezParticleFinisherComponent::~ezParticleFinisherComponent()
{
}

void ezParticleFinisherComponent::OnDeactivated()
{
  m_EffectController.StopImmediate();
  m_EffectController.Invalidate();

  ezRenderComponent::OnDeactivated();
}

void ezParticleFinisherComponent::SerializeComponent(ezWorldWriter& stream) const
{
  //auto& s = stream.GetStream();
}

void ezParticleFinisherComponent::DeserializeComponent(ezWorldReader& stream)
{
  //auto& s = stream.GetStream();
}

ezResult ezParticleFinisherComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_EffectController.IsAlive())
  {
    ezBoundingBoxSphere volume;
    m_LastBVolumeUpdate = m_EffectController.GetBoundingVolume(volume);

    if (!m_LastBVolumeUpdate.IsZero())
    {
      bounds.ExpandToInclude(volume);
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

void ezParticleFinisherComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
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
  if (m_LastBVolumeUpdate < m_EffectController.GetBoundingVolume(bvol))
  {
    TriggerLocalBoundsUpdate();
  }
}


