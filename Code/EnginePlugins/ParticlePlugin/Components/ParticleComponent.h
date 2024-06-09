#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>

class ezParticleRenderData;
struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;
class ezParticleSystemInstance;
class ezParticleComponent;
struct ezMsgSetPlaying;
struct ezMsgInterruptPlaying;

using ezParticleEffectResourceHandle = ezTypedResourceHandle<class ezParticleEffectResource>;

class EZ_PARTICLEPLUGIN_DLL ezParticleComponentManager final : public ezComponentManager<class ezParticleComponent, ezBlockStorageType::Compact>
{
  using SUPER = ezComponentManager<class ezParticleComponent, ezBlockStorageType::Compact>;

public:
  ezParticleComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);

  void UpdatePfxTransformsAndBounds();
};

/// \brief Plays a particle effect at the location of the game object.
class EZ_PARTICLEPLUGIN_DLL ezParticleComponent final : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleComponent, ezRenderComponent, ezParticleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezParticleComponent

public:
  ezParticleComponent();
  ~ezParticleComponent();

  /// \brief Starts a new particle effect. If one is already running, it will be stopped (but not interrupted) and a new one is started as
  /// well.
  ///
  /// Returns false, if no valid particle resource is specified.
  bool StartEffect(); // [ scriptable ]

  /// \brief Stops emitting further particles, making any existing particle system stop in a finite amount of time.
  void StopEffect(); // [ scriptable ]

  /// \brief Cancels the entire effect immediately, it will pop out of existence.
  void InterruptEffect(); // [ scriptable ]

  /// \brief Returns true, if an effect is currently in a state where it might emit new particles
  bool IsEffectActive() const; // [ scriptable ]

  /// \brief Forwards to StartEffect() or StopEffect().
  void OnMsgSetPlaying(ezMsgSetPlaying& ref_msg); // [ msg handler ]

  /// \brief Forwards to InterruptEffect().
  void OnMsgInterruptPlaying(ezMsgInterruptPlaying& ref_msg); // [ msg handler ]

  /// \brief Replaces the effect to be played.
  void SetParticleEffect(const ezParticleEffectResourceHandle& hEffect);
  EZ_ALWAYS_INLINE const ezParticleEffectResourceHandle& GetParticleEffect() const { return m_hEffectResource; }

  void SetParticleEffectFile(const char* szFile); // [ property ]
  const char* GetParticleEffectFile() const;      // [ property ]

  // Exposed Parameters
  const ezRangeView<const char*, ezUInt32> GetParameters() const;   // [ property ]
  void SetParameter(const char* szKey, const ezVariant& value);     // [ property ]
  void RemoveParameter(const char* szKey);                          // [ property ]
  bool GetParameter(const char* szKey, ezVariant& out_value) const; // [ property ]

  /// \brief If zero, the played effect is randomized each time. Use a fixed seed when the result should be deterministic.
  ezUInt64 m_uiRandomSeed = 0; // [ property ]

  /// \brief If set, the component reuses the simulation state of another particle component with the same name.
  ///
  /// This can be used to reuse similar effects, for example smoke on chimneys doesn't need to be unique.
  /// Each instance renders the effect from its own perspective, but the simulation is only done once.
  /// This only makes sense for infinite, ambient effects.
  ezString m_sSharedInstanceName; // [ property ]

  /// \brief If false, the effect starts in a paused state.
  bool m_bSpawnAtStart = true; // [ property ]

  /// \brief If true, the owner rotation is assumed to be identity. Useful for effects that need to always point in one direction (e.g. up).
  bool m_bIgnoreOwnerRotation = false; // [ property ]

  /// \brief What to do when the effect is finished playing.
  ezEnum<ezOnComponentFinishedAction2> m_OnFinishedAction; // [ property ]

  /// \brief Minimum delay between finishing and restarting.
  ezTime m_MinRestartDelay; // [ property ]

  /// \brief Random additional delay between finishing and restarting.
  ezTime m_RestartDelayRange; // [ property ]

  /// \brief The local direction into which to spawn the effect.
  ezEnum<ezBasisAxis> m_SpawnDirection = ezBasisAxis::PositiveZ; // [ property ]

  /// \brief Allows more fine grain control over the effect execution.
  ezParticleEffectController m_EffectController;

protected:
  void Update();
  ezTransform GetPfxTransform() const;
  void UpdatePfxTransformAndBounds();

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg);

  ezParticleEffectResourceHandle m_hEffectResource;

  ezTime m_RestartTime;

  // Exposed Parameters
  friend class ezParticleEventReaction_Effect;
  bool m_bIfContinuousStopRightAway = false;
  bool m_bFloatParamsChanged = false;
  bool m_bColorParamsChanged = false;
  ezHybridArray<ezParticleEffectFloatParam, 2> m_FloatParams;
  ezHybridArray<ezParticleEffectColorParam, 2> m_ColorParams;
};
