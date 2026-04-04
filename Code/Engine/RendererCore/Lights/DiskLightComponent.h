#pragma once

#include <RendererCore/Lights/LightComponent.h>

using ezDiskLightComponentManager = ezComponentManager<class ezDiskLightComponent, ezBlockStorageType::Compact>;

/// \brief The render data object for disk area lights.
class EZ_RENDERERCORE_DLL ezDiskLightRenderData : public ezLightRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDiskLightRenderData, ezLightRenderData);

public:
  ezQuat m_qGlobalRotation;
  float m_fRadius;
  float m_fRange;
  float m_fFalloff;
  bool m_bTwoSided;
};

/// \brief Adds a disk area light to the scene using Linearly Transformed Cosines (LTC).
///
/// Disk lights emit from a flat circular surface
class EZ_RENDERERCORE_DLL ezDiskLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDiskLightComponent, ezLightComponent, ezDiskLightComponentManager);

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
  // ezDiskLightComponent

public:
  ezDiskLightComponent();
  ~ezDiskLightComponent();

  void SetRadius(float fRadius);            // [ property ]
  float GetRadius() const;                  // [ property ]

  void SetRange(float fRange);              // [ property ]
  float GetRange() const;                   // [ property ]

  float GetEffectiveRange() const;

  void SetFalloff(float fFalloff);          // [ property ]
  float GetFalloff() const;                 // [ property ]

  void SetTwoSided(bool bTwoSided);         // [ property ]
  bool GetTwoSided() const;                 // [ property ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  float m_fRadius = 0.5f;
  float m_fRange = 5.0f;
  float m_fEffectiveRange = 5.0f;
  float m_fFalloff = 1.0f;
  bool m_bTwoSided = false;
};