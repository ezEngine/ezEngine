#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

ezParticleFinisherComponentManager::ezParticleFinisherComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
{
}

void ezParticleFinisherComponentManager::UpdateBounds()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->UpdateBounds();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

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

ezResult ezParticleFinisherComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_EffectController.IsAlive())
  {
    m_EffectController.GetBoundingVolume(ref_bounds);
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezParticleFinisherComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // do not extract particles during shadow map rendering
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  m_EffectController.ExtractRenderData(msg, GetOwner()->GetGlobalTransform());
}

void ezParticleFinisherComponent::UpdateBounds()
{
  if (m_EffectController.IsAlive())
  {
    // This function is called in the post-transform phase so the global bounds and transform have already been calculated at this point.
    // Therefore we need to manually update the global bounds again to ensure correct bounds for culling and rendering.
    GetOwner()->UpdateLocalBounds();
    GetOwner()->UpdateGlobalBounds();
  }
  else
  {
    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleFinisherComponent);
