#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;

typedef ezSettingsComponentManager<class ezBakingSettingsComponent> ezBakingSettingsComponentManager;

class EZ_BAKINGPLUGIN_DLL ezBakingSettingsComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezBakingSettingsComponent, ezSettingsComponent, ezBakingSettingsComponentManager);

public:
  ezBakingSettingsComponent();
  ~ezBakingSettingsComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetShowDebugOverlay(bool bShow);
  bool GetShowDebugOverlay() const { return m_bShowDebugOverlay; }

  void SetShowDebugProbes(bool bShow);
  bool GetShowDebugProbes() const { return m_bShowDebugProbes; }

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
  bool m_bShowDebugOverlay = false;
  bool m_bShowDebugProbes = false;

  struct RenderDebugViewTask;
  ezUniquePtr<RenderDebugViewTask> m_pRenderDebugViewTask;

  ezGALTextureHandle m_hDebugViewTexture;
};
