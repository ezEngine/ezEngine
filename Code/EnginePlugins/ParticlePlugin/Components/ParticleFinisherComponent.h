#pragma once

#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>

struct ezMsgExtractRenderData;

typedef ezComponentManagerSimple<class ezParticleFinisherComponent, ezComponentUpdateType::WhenSimulating> ezParticleFinisherComponentManager;

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
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;


  //////////////////////////////////////////////////////////////////////////
  // ezParticleFinisherComponent

public:
  ezParticleFinisherComponent();
  ~ezParticleFinisherComponent();

  ezParticleEffectController m_EffectController;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  void Update();
  void CheckBVolumeUpdate();
  ezTime m_LastBVolumeUpdate;
};
