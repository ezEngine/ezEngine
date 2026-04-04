#pragma once

#include <Foundation/Math/Float16.h>
#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

using ezRectangleLightComponentManager = ezComponentManager<class ezRectangleLightComponent, ezBlockStorageType::Compact>;

/// \brief The render data object for rectangular area lights.
class EZ_RENDERERCORE_DLL ezRectangleLightRenderData : public ezLightRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRectangleLightRenderData, ezLightRenderData);

public:
  ezQuat m_qGlobalRotation;
  float m_fWidth;
  float m_fHeight;
  float m_fRange;
  float m_fFalloff;
  bool m_bTwoSided;
  ezDecalId m_CookieId;
};

/// \brief Adds a rectangular area light to the scene using Linearly Transformed Cosines (LTC).
///
/// Rectangle lights emit light from a flat rectangular surface. They are more expensive then
/// point or spot lights. They are useful for simulating light coming from windows or screens.
class EZ_RENDERERCORE_DLL ezRectangleLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRectangleLightComponent, ezLightComponent, ezRectangleLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent
public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRectangleLightComponent
public:
  ezRectangleLightComponent();
  ~ezRectangleLightComponent();

  void SetWidth(float fWidth);            // [ property ]
  float GetWidth() const;                 // [ property ]

  void SetHeight(float fHeight);          // [ property ]
  float GetHeight() const;                // [ property ]

  void SetRange(float fRange);            // [ property ]
  float GetRange() const;                 // [ property ]

  float GetEffectiveRange() const;

  void SetFalloff(float fFalloff);        // [ property ]
  float GetFalloff() const;               // [ property ]

  void SetTwoSided(bool bTwoSided);       // [ property ]
  bool GetTwoSided() const;               // [ property ]

  void SetCookie(const ezTexture2DResourceHandle& hCookie);                // [ property ]
  const ezTexture2DResourceHandle& GetCookie() const { return m_hCookie; } // [ property ]

  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Cookie, m_hCookie, SetCookie);

  void SetMaterial(const ezMaterialResourceHandle& hMaterial);                // [ property ]
  const ezMaterialResourceHandle& GetMaterial() const { return m_hMaterial; } // [ property ]

  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Material, m_hMaterial, SetMaterial);

  void SetMaterialResolution(ezUInt32 uiResolution);                                                     // [ property ]
  ezUInt32 GetMaterialResolution() const { return m_uiMaterialResolution; }                              // [ property ]

  void SetMaterialUpdateInterval(ezTime updateInterval);                                                 // [ property ]
  ezTime GetMaterialUpdateInterval() const { return ezTime::MakeFromSeconds(m_MaterialUpdateInterval); } // [ property ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  void UpdateCookie();
  void DeleteCookie();

  float m_fWidth = 1.0f;
  float m_fHeight = 1.0f;
  float m_fRange = 5.0f;
  float m_fEffectiveRange = 5.0f;
  float m_fFalloff = 1.0f;
  bool m_bTwoSided = false;

  ezUInt16 m_uiMaterialResolution = 512;
  ezFloat16 m_MaterialUpdateInterval = 0.0f;
  ezMaterialResourceHandle m_hMaterial;

  ezTexture2DResourceHandle m_hCookie;

  ezDecalId m_CookieId;
};
