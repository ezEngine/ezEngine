#pragma once

#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgUpdateLocalBounds;

class EZ_RENDERERCORE_DLL ezLightShaftsRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLightShaftsRenderData, ezRenderData);

public:
  ezVec3 m_vDirection;
  float m_fIntensity;
  float m_fMaxBrightness;
  float m_fBrightnessThreshold;
  float m_fDiskMaskRadius;
};

using ezLightShaftsComponentManager = ezSettingsComponentManager<class ezLightShaftsComponent>;

/// \brief Adds a light shaft effect to the scene. Usually, this component is attached to the same game object as a directional light.
class EZ_RENDERERCORE_DLL ezLightShaftsComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezLightShaftsComponent, ezComponent, ezLightShaftsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLightShaftsComponent

public:
  ezLightShaftsComponent();
  ~ezLightShaftsComponent();

  /// \brief Sets the intensity of the light shafts.
  void SetIntensity(float fIntensity);                // [ property ]
  float GetIntensity() const { return m_fIntensity; } // [ property ]

  /// \brief Sets the brightness threshold for the light shafts. Only pixels brighter than this value will contribute to the light shafts.
  void SetBrightnessThreshold(float fBrightnessThreshold);                // [ property ]
  float GetBrightnessThreshold() const { return m_fBrightnessThreshold; } // [ property ]

  /// \brief The maximum brightness is used to prevent excessively bright light shafts, which can cause visual artifacts.
  void SetMaxBrightness(float fMaxBrightness);                // [ property ]
  float GetMaxBrightness() const { return m_fMaxBrightness; } // [ property ]

  /// \brief The disk is used to mask only a small area around the sun as source for the light shafts.
  ///
  /// This is only needed if the sun has no other visual representation in the sky and the brightness threshold is too low to prevent artifacts.
  /// The radius is given in relative screen space, where 0.1 means 10% of the screen height.
  void SetDiskMaskRadius(float fDiskMaskRadius);                // [ property ]
  float GetDiskMaskRadius() const { return m_fDiskMaskRadius; } // [ property ]

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  float m_fIntensity = 1.0f;
  float m_fMaxBrightness = 10.0f;
  float m_fBrightnessThreshold = 0.0f;
  float m_fDiskMaskRadius = 0.1f;
};
