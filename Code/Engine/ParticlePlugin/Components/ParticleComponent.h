#pragma once

#include <ParticlePlugin/Basics.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <RendererCore/Components/RenderComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>

class ezParticleRenderData;
struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;
class ezParticleSystemInstance;
class ezParticleComponent;

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

typedef ezComponentManagerSimple<ezParticleComponent, ezComponentUpdateType::WhenSimulating> ezParticleComponentManager;

/// \brief This message makes an ezParticleComponent start or stop it's effect.
struct ezMsgPlayParticleEffect : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPlayParticleEffect, ezMessage);

  /// If true, StartEffect() is called, otherwise StopEffect() is called.
  bool m_bPlay = true;
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

  //////////////////////////////////////////////////////////////////////////
  // Interface

  /// \brief Starts a new particle effect. If one is already running, it will be stopped (but not interrupted) and a new one is started as well.
  ///
  /// Returns false, if no valid particle resource is specified.
  bool StartEffect();
  /// \brief Stops emitting further particles, making any existing particle system stop in a finite amount of time.
  void StopEffect();
  /// \brief Cancels the entire effect immediately, it will pop out of existence.
  void InterruptEffect();
  /// \brief Returns true, if an effect is currently in a state where it might emit new particles
  bool IsEffectActive() const;

  /// Message Handler for ezMsgPlayParticleEffect
  void Play(ezMsgPlayParticleEffect& msg);

  //////////////////////////////////////////////////////////////////////////
  // Properties

  void SetParticleEffect(const ezParticleEffectResourceHandle& hEffect);
  EZ_ALWAYS_INLINE const ezParticleEffectResourceHandle& GetParticleEffect() const { return m_hEffectResource; }

  void SetParticleEffectFile(const char* szFile);
  const char* GetParticleEffectFile() const;

  ezUInt64 m_uiRandomSeed;
  ezString m_sSharedInstanceName;

  bool m_bSpawnAtStart;
  ezEnum<ezOnComponentFinishedAction2> m_OnFinishedAction;
  ezTime m_MinRestartDelay;
  ezTime m_RestartDelayRange;

  //////////////////////////////////////////////////////////////////////////


  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezParticleEffectController m_EffectController;

protected:
  virtual void OnDeactivated() override;
  void HandOffToFinisher();

  ezParticleEffectResourceHandle m_hEffectResource;
  ezTime m_RestartTime;

  void CheckBVolumeUpdate();
  ezTime m_LastBVolumeUpdate;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:

  void SetFloatParam(const char* szKey, float value);
  void SetColorParam(const char* szKey, const ezColor& value);

private:
  ezMap<ezString, float> GetFloatParams() const;
  ezMap<ezString, ezColor> GetColorParams() const;
  void RemoveFloatParam(const char* szKey);
  void RemoveColorParam(const char* szKey);

  struct FloatParam
  {
    EZ_DECLARE_POD_TYPE();
    ezHashedString m_sName;
    float m_Value;
  };

  struct ColorParam
  {
    EZ_DECLARE_POD_TYPE();
    ezHashedString m_sName;
    ezColor m_Value;
  };

  bool m_bFloatParamsChanged = false;
  bool m_bColorParamsChanged = false;
  ezHybridArray<FloatParam, 2> m_FloatParams;
  ezHybridArray<ColorParam, 2> m_ColorParams;
};
