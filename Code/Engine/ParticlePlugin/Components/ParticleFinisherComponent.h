#pragma once

#include <ParticlePlugin/Basics.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <RendererCore/Components/RenderComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>

struct ezExtractRenderDataMessage;

typedef ezComponentManagerSimple<class ezParticleFinisherComponent, ezComponentUpdateType::WhenSimulating> ezParticleFinisherComponentManager;

class EZ_PARTICLEPLUGIN_DLL ezParticleFinisherComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleFinisherComponent, ezRenderComponent, ezParticleFinisherComponentManager);

public:
  ezParticleFinisherComponent();
  ~ezParticleFinisherComponent();

  //////////////////////////////////////////////////////////////////////////
  // Properties
  //

public:
  //ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction;

  //////////////////////////////////////////////////////////////////////////
  // ezParticleFinisherComponent Interface
  //


  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
  //

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent Interface
  // 

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

protected:
  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // Implementation
  //

public:
  ezParticleEffectController m_EffectController;

protected:
  void Update();

};
