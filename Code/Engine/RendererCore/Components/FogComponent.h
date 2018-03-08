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

public:
  ezFogComponent();
  ~ezFogComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetColor(ezColor color);
  ezColor GetColor() const;

  void SetDensity(float fDensity);
  float GetDensity() const;

  void SetHeightFalloff(float fHeightFalloff);
  float GetHeightFalloff() const;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:

  ezColor m_Color;
  float m_fDensity;
  float m_fHeightFalloff;
};
