#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgSetColor;
using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

class EZ_RENDERERCORE_DLL ezLensFlareRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLensFlareRenderData, ezRenderData);

public:
  void FillBatchIdAndSortingKey();

  ezTexture2DResourceHandle m_hTexture;
  ezFloat16Vec4 m_Color;

  float m_fSize;
  float m_fMaxScreenSize;
  float m_fAspectRatio;
  float m_fShiftToCenter;
  float m_fOcclusionSampleRadius;
  float m_fOcclusionSampleSpread;
  float m_fOcclusionDepthOffset;

  bool m_bInverseTonemap;
  bool m_bGreyscaleTexture;
  bool m_bApplyFog;
};

struct ezLensFlareElement
{
  ezTexture2DResourceHandle m_hTexture;
  ezColor m_Color = ezColor::White;
  float m_fSize = 10000.0f;
  float m_fMaxScreenSize = 1.0f;
  float m_fAspectRatio = 1.0f;
  float m_fShiftToCenter = 0.0f;
  bool m_bGreyscaleTexture = false;
  bool m_bModulateByLightColor = true;
  bool m_bInverseTonemap = false;

  void SetTextureFile(const char* szFile); // [ property ]
  const char* GetTextureFile() const;      // [ property ]

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezLensFlareElement);

using ezLensFlareComponentManager = ezComponentManager<class ezLensFlareComponent, ezBlockStorageType::Compact>;

class EZ_RENDERERCORE_DLL ezLensFlareComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezLensFlareComponent, ezRenderComponent, ezLensFlareComponentManager);

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
  // ezLensFlareComponent

public:
  ezLensFlareComponent();
  ~ezLensFlareComponent();

  void SetOcclusionSampleRadius(float fRadius);                               // [ property ]
  float GetOcclusionSampleRadius() const { return m_fOcclusionSampleRadius; } // [ property ]

  void SetLinkToLightShape(bool bLink);                            // [ property ]
  bool GetLinkToLightShape() const { return m_bLinkToLightShape; } // [ property ]

  ezSmallArray<ezLensFlareElement, 1> m_Elements; // [ property ]

  float m_fIntensity = 1.0f; // [ property ]

private:
  void FindLightComponent();

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  float m_fOcclusionSampleRadius = 0.1f;
  float m_fOcclusionSampleSpread = 0.5f;
  float m_fOcclusionDepthOffset = 0.0f;
  bool m_bLinkToLightShape = true;
  bool m_bApplyFog = true;

  bool m_bDirectionalLight = false;
  ezComponentHandle m_hLightComponent;
};
