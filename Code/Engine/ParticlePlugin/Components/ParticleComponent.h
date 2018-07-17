#pragma once

#include <Core/ResourceManager/ResourceBase.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <RendererCore/Components/RenderComponent.h>

class ezParticleRenderData;
struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;
class ezParticleSystemInstance;
class ezParticleComponent;
struct ezMsgSetPlaying;

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

typedef ezComponentManagerSimple<ezParticleComponent, ezComponentUpdateType::WhenSimulating> ezParticleComponentManager;

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

  /// \brief Starts a new particle effect. If one is already running, it will be stopped (but not interrupted) and a new one is started as
  /// well.
  ///
  /// Returns false, if no valid particle resource is specified.
  bool StartEffect();
  /// \brief Stops emitting further particles, making any existing particle system stop in a finite amount of time.
  void StopEffect();
  /// \brief Cancels the entire effect immediately, it will pop out of existence.
  void InterruptEffect();
  /// \brief Returns true, if an effect is currently in a state where it might emit new particles
  bool IsEffectActive() const;

  void OnSetPlaying(ezMsgSetPlaying& msg);

  //////////////////////////////////////////////////////////////////////////
  // Properties

  void SetParticleEffect(const ezParticleEffectResourceHandle& hEffect);
  EZ_ALWAYS_INLINE const ezParticleEffectResourceHandle& GetParticleEffect() const { return m_hEffectResource; }

  void SetParticleEffectFile(const char* szFile);
  const char* GetParticleEffectFile() const;

  ezUInt64 m_uiRandomSeed;
  ezString m_sSharedInstanceName;

  bool m_bSpawnAtStart = true;
  bool m_bIfContinuousStopRightAway = false;
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
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  friend class ezParticleEventReaction_Effect;

  bool m_bFloatParamsChanged = false;
  bool m_bColorParamsChanged = false;
  ezHybridArray<ezParticleEffectFloatParam, 2> m_FloatParams;
  ezHybridArray<ezParticleEffectColorParam, 2> m_ColorParams;
};
