#pragma once

#include <ParticlePlugin/Basics.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <Core/World/ComponentManager.h>
#include <RendererCore/Components/RenderComponent.h>

class ezParticleRenderData;
struct ezUpdateLocalBoundsMessage;
struct ezExtractRenderDataMessage;
class ezParticleSystemInstance;
class ezParticleComponent;

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleComponentManager : public ezComponentManagerSimple<ezParticleComponent, true>
{
public:
  ezParticleComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleComponent, ezRenderComponent, ezParticleComponentManager);

public:
  ezParticleComponent();
  ~ezParticleComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetParticleEffect(const ezParticleEffectResourceHandle& hEffect);
  EZ_FORCE_INLINE const ezParticleEffectResourceHandle& GetParticleEffect() const { return m_hEffect; }

  void SetParticleEffectFile(const char* szFile);
  const char* GetParticleEffectFile() const;


  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds) override;

protected:
  ezParticleEffectResourceHandle m_hEffect;

  virtual void OnAfterAttachedToObject() override;
  virtual void OnBeforeDetachedFromObject() override;

public:

  ezParticleEffectInstance* m_pParticleEffect;
};
