#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;
struct ezRenderWorldRenderEvent;
class ezAbstractObjectNode;

class EZ_BAKINGPLUGIN_DLL ezBakingSettingsComponentManager : public ezSettingsComponentManager<class ezBakingSettingsComponent>
{
public:
  ezBakingSettingsComponentManager(ezWorld* pWorld);
  ~ezBakingSettingsComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  ezMeshResourceHandle m_hDebugSphere;
  ezMaterialResourceHandle m_hDebugMaterial;

private:
  void RenderDebug(const ezWorldModule::UpdateContext& updateContext);
  void OnRenderEvent(const ezRenderWorldRenderEvent& e);
  void CreateDebugResources();
};

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
  void RenderDebugOverlay();
  void OnObjectCreated(const ezAbstractObjectNode& node);

  bool m_bShowDebugOverlay = false;
  bool m_bShowDebugProbes = false;

  struct RenderDebugViewTask;
  ezUniquePtr<RenderDebugViewTask> m_pRenderDebugViewTask;

  ezGALTextureHandle m_hDebugViewTexture;
};
