#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgUpdateLocalBounds;

typedef ezSettingsComponentManager<class ezFogComponent> ezFogComponentManager;

/// \brief The render data object for ambient light.
class EZ_RENDERERCORE_DLL ezFogRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFogRenderData, ezRenderData);

public:
  ezColor m_Color;
  float m_fDensity;
  float m_fHeightFalloff;
};

class EZ_RENDERERCORE_DLL ezFogComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFogComponent, ezSettingsComponent, ezFogComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezFogComponent

public:
  ezFogComponent();
  ~ezFogComponent();

  void SetColor(ezColor color); // [ property ]
  ezColor GetColor() const;     // [ property ]

  void SetDensity(float fDensity); // [ property ]
  float GetDensity() const;        // [ property ]

  void SetHeightFalloff(float fHeightFalloff); // [ property ]
  float GetHeightFalloff() const;              // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezColor m_Color = ezColor(0.2f, 0.2f, 0.3f);
  float m_fDensity = 1.0f;
  float m_fHeightFalloff = 10.0f;
};
