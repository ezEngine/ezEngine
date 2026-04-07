#pragma once

#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Components/RenderComponent.h>
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
  ezColorGammaUB m_TintColor;
};

using ezLightShaftsComponentManager = ezSettingsComponentManager<class ezLightShaftsComponent>;

/// \brief Adds a light shaft effect to the scene. Usually, this component is attached to the same game object as a directional light.
class EZ_RENDERERCORE_DLL ezLightShaftsComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezLightShaftsComponent, ezRenderComponent, ezLightShaftsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

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

  /// \brief The light shafts pick its color from the sky pixels at the center. The tint color can be used to add an additional color tint to the light shafts.
  void SetTintColor(const ezColorGammaUB& color);                    // [ property ]
  const ezColorGammaUB& GetTintColor() const { return m_TintColor; } // [ property ]

private:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  float m_fIntensity = 1.0f;
  float m_fMaxBrightness = 10.0f;
  float m_fBrightnessThreshold = 0.0f;
  float m_fDiskMaskRadius = 0.1f;

  ezColorGammaUB m_TintColor = ezColor::White;
};
