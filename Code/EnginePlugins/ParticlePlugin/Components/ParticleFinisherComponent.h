#pragma once

#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>

struct ezMsgExtractRenderData;

class EZ_PARTICLEPLUGIN_DLL ezParticleFinisherComponentManager final : public ezComponentManager<class ezParticleFinisherComponent, ezBlockStorageType::Compact>
{
  using SUPER = ezComponentManager<class ezParticleFinisherComponent, ezBlockStorageType::Compact>;

public:
  ezParticleFinisherComponentManager(ezWorld* pWorld);

  void UpdateBounds();
};

/// \brief Automatically created by the particle system to finish playing a particle effect.
///
/// This is needed to play a particle effect to the end, when a game object with a particle effect on it gets deleted.
/// This component should never be instantiated manually.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinisherComponent final : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleFinisherComponent, ezRenderComponent, ezParticleFinisherComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezParticleFinisherComponent

public:
  ezParticleFinisherComponent();
  ~ezParticleFinisherComponent();

  ezParticleEffectController m_EffectController;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  void UpdateBounds();
};
