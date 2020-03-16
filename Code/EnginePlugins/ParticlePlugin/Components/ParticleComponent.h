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

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

typedef ezComponentManagerSimple<ezParticleComponent, ezComponentUpdateType::WhenSimulating> ezParticleComponentManager;

class EZ_PARTICLEPLUGIN_DLL ezParticleComponent final : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleComponent, ezRenderComponent, ezParticleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;


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

  void OnMsgSetPlaying(ezMsgSetPlaying& msg); // [ msg handler ]

  void SetParticleEffect(const ezParticleEffectResourceHandle& hEffect);
  EZ_ALWAYS_INLINE const ezParticleEffectResourceHandle& GetParticleEffect() const { return m_hEffectResource; }

  void SetParticleEffectFile(const char* szFile); // [ property ]
  const char* GetParticleEffectFile() const;      // [ property ]

  // Exposed Parameters
  const ezRangeView<const char*, ezUInt32> GetParameters() const;   // [ property ]
  void SetParameter(const char* szKey, const ezVariant& value);     // [ property ]
  void RemoveParameter(const char* szKey);                          // [ property ]
  bool GetParameter(const char* szKey, ezVariant& out_value) const; // [ property ]

  ezUInt64 m_uiRandomSeed = 0;    // [ property ]
  ezString m_sSharedInstanceName; // [ property ]

  bool m_bSpawnAtStart = true;                             // [ property ]
  bool m_bIfContinuousStopRightAway = false;               // [ property ]
  ezEnum<ezOnComponentFinishedAction2> m_OnFinishedAction; // [ property ]
  ezTime m_MinRestartDelay;                                // [ property ]
  ezTime m_RestartDelayRange;                              // [ property ]

  ezParticleEffectController m_EffectController;

protected:
  void Update();

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg);

  virtual void OnDeactivated() override;
  void HandOffToFinisher();

  ezParticleEffectResourceHandle m_hEffectResource;
  ezTime m_RestartTime;

  void CheckBVolumeUpdate();
  ezTime m_LastBVolumeUpdate;

  // Exposed Parameters
  friend class ezParticleEventReaction_Effect;
  bool m_bFloatParamsChanged = false;
  bool m_bColorParamsChanged = false;
  ezHybridArray<ezParticleEffectFloatParam, 2> m_FloatParams;
  ezHybridArray<ezParticleEffectColorParam, 2> m_ColorParams;
};
