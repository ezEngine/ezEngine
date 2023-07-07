#pragma once

#include <Core/World/World.h>
#include <GameEngine/Volumes/VolumeSampler.h>
#include <RendererCore/Pipeline/Declarations.h>

class EZ_GAMEENGINE_DLL ezPostProcessingComponentManager : public ezComponentManager<class ezPostProcessingComponent, ezBlockStorageType::Compact>
{
public:
  ezPostProcessingComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

private:
  void UpdateComponents(const UpdateContext& context);
};

struct ezPostProcessingValueMapping
{
  ezHashedString m_sRenderPassName;
  ezHashedString m_sPropertyName;
  ezHashedString m_sVolumeValueName;
  ezVariant m_DefaultValue;
  ezTime m_InterpolationDuration;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPostProcessingValueMapping);

class EZ_GAMEENGINE_DLL ezPostProcessingComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPostProcessingComponent, ezComponent, ezPostProcessingComponentManager);

public:
  ezPostProcessingComponent();
  ezPostProcessingComponent(ezPostProcessingComponent&& other);
  ~ezPostProcessingComponent();
  ezPostProcessingComponent& operator=(ezPostProcessingComponent&& other);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

private:
  ezUInt32 Mappings_GetCount() const { return m_Mappings.GetCount(); }                                // [ property ]
  const ezPostProcessingValueMapping& Mappings_GetMapping(ezUInt32 i) const { return m_Mappings[i]; } // [ property ]
  void Mappings_SetMapping(ezUInt32 i, const ezPostProcessingValueMapping& mapping);                  // [ property ]
  void Mappings_Insert(ezUInt32 uiIndex, const ezPostProcessingValueMapping& mapping);                // [ property ]
  void Mappings_Remove(ezUInt32 uiIndex);                                                             // [ property ]

  void RegisterSamplerValues();
  void SampleAndSetViewProperties();

  ezEnum<ezCameraUsageHint> m_UsageHint = ezCameraUsageHint::MainView;              // [ property ]
  ezEnum<ezCameraUsageHint> m_AlternativeUsageHint = ezCameraUsageHint::EditorView; // [ property ]
  ezDynamicArray<ezPostProcessingValueMapping> m_Mappings;
  ezUniquePtr<ezVolumeSampler> m_pSampler;
};
