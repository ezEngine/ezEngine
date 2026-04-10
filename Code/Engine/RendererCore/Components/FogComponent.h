#pragma once

#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgUpdateLocalBounds;

/// \brief The render data object for height fog.
class EZ_RENDERERCORE_DLL ezFogRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFogRenderData, ezRenderData);

public:
  ezColor m_Color;
  float m_fDensity;
  float m_fBaseHeight;
  float m_fHeightFalloff;
  float m_fInvSkyDistance;
  float m_fFogStartDistance;
};

using ezFogComponentManager = ezSettingsComponentManager<class ezFogComponent>;

class EZ_RENDERERCORE_DLL ezFogComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFogComponent, ezComponent, ezFogComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezFogComponent

public:
  ezFogComponent();
  ~ezFogComponent();

  void SetColor(ezColor color);                 // [ property ]
  ezColor GetColor() const;                     // [ property ]

  void SetDensity(float fDensity);              // [ property ]
  float GetDensity() const;                     // [ property ]

  void SetHeightFalloff(float fHeightFalloff);  // [ property ]
  float GetHeightFalloff() const;               // [ property ]

  void SetModulateWithSkyColor(bool bModulate); // [ property ]
  bool GetModulateWithSkyColor() const;         // [ property ]

  void SetSkyDistance(float fDistance);         // [ property ]
  float GetSkyDistance() const;                 // [ property ]

  void SetStartDistance(float fDistance);       // [ property ]
  float GetStartDistance() const;               // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezColor m_Color = ezColor(0.2f, 0.2f, 0.3f);
  float m_fDensity = 1.0f;
  float m_fHeightFalloff = 10.0f;
  float m_fSkyDistance = 1000.0f;
  float m_fStartDistance = 0.0f;
  bool m_bModulateWithSkyColor = false;
};
