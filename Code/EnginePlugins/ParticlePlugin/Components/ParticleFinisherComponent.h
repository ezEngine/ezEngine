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
