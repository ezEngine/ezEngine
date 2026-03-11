#pragma once

#include <Core/World/World.h>
#include <GameEngine/Volumes/VolumeSampler.h>

struct ezVolumeSamplerValue
{
  ezHashedString m_sName;
  ezVariant m_DefaultValue;
  ezTime m_InterpolationDuration;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezVolumeSamplerValue);

using ezVolumeSamplerComponentManager = ezComponentManagerSimple<class ezVolumeSamplerComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact, ezWorldUpdatePhase::Async>;

/// \brief A component that wraps an ezVolumeSampler to sample arbitrary values from volumes at the owner's position (or the main camera's position).
///
/// The sampled values can then be queried via script or C++ code and used for things like gameplay logic, audio modulation, etc.
class EZ_GAMEENGINE_DLL ezVolumeSamplerComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVolumeSamplerComponent, ezComponent, ezVolumeSamplerComponentManager);

public:
  ezVolumeSamplerComponent();
  ezVolumeSamplerComponent(ezVolumeSamplerComponent&& other);
  ~ezVolumeSamplerComponent();
  ezVolumeSamplerComponent& operator=(ezVolumeSamplerComponent&& other);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  /// \brief Sets which volume type to sample from.
  void SetVolumeType(const char* szType); // [ property ]
  const char* GetVolumeType() const;      // [ property ]

  /// \brief If enabled, the sampling position will be the main camera's position. Otherwise, the owner's position is used.
  void SetAttachToMainCamera(bool bAttach);                            // [ property ]
  bool GetAttachToMainCamera() const { return m_bAttachToMainCamera; } // [ property ]

  /// \brief Registers a new value to be sampled from the volumes. This registration is only done at runtime and not serialized.
  void RegisterValue(const ezHashedString& sName, const ezVariant& defaultValue, ezTime interpolationDuration = ezTime::MakeZero()); // [ scriptable ]

  /// \brief Get the latest sampled value for the given name.
  ezVariant GetValue(const ezHashedString& sName) const;                                                   // [ scriptable ]
  float GetFloatValue(const ezHashedString& sName, float fFallbackValue = 0.0f) const;                     // [ scriptable ]
  ezColor GetColorValue(const ezHashedString& sName, const ezColor& fallbackValue = ezColor::White) const; // [ scriptable ]

private:
  ezUInt32 Values_GetCount() const { return m_Values.GetCount(); }                                         // [ property ]
  const ezVolumeSamplerValue& Values_GetMapping(ezUInt32 i) const { return m_Values[i]; }                  // [ property ]
  void Values_SetMapping(ezUInt32 i, const ezVolumeSamplerValue& mapping);                                 // [ property ]
  void Values_Insert(ezUInt32 uiIndex, const ezVolumeSamplerValue& mapping);                               // [ property ]
  void Values_Remove(ezUInt32 uiIndex);                                                                    // [ property ]

  void RegisterSamplerValues(ezArrayPtr<const ezVolumeSamplerValue> values);
  void Update();

  ezDynamicArray<ezVolumeSamplerValue> m_Values;
  ezUniquePtr<ezVolumeSampler> m_pSampler;
  ezSpatialData::Category m_SpatialCategory = ezInvalidSpatialDataCategory;
  bool m_bAttachToMainCamera = false;
};
