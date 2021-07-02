#pragma once

#include <RendererCore/Declarations.h>
#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;
struct ezRenderWorldRenderEvent;
class ezAbstractObjectNode;

using ezProbeTreeSectorResourceHandle = ezTypedResourceHandle<class ezProbeTreeSectorResource>;

class EZ_RENDERERCORE_DLL ezBakedProbesSettingsComponentManager : public ezSettingsComponentManager<class ezBakedProbesSettingsComponent>
{
public:
  ezBakedProbesSettingsComponentManager(ezWorld* pWorld);
  ~ezBakedProbesSettingsComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  ezMeshResourceHandle m_hDebugSphere;
  ezMaterialResourceHandle m_hDebugMaterial;

private:
  void RenderDebug(const ezWorldModule::UpdateContext& updateContext);
  void OnRenderEvent(const ezRenderWorldRenderEvent& e);
  void CreateDebugResources();
};

class EZ_RENDERERCORE_DLL ezBakedProbesSettingsComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezBakedProbesSettingsComponent, ezSettingsComponent, ezBakedProbesSettingsComponentManager);

public:
  ezBakedProbesSettingsComponent();
  ~ezBakedProbesSettingsComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetShowDebugOverlay(bool bShow);
  bool GetShowDebugOverlay() const { return m_bShowDebugOverlay; }

  void SetShowDebugProbes(bool bShow);
  bool GetShowDebugProbes() const { return m_bShowDebugProbes; }

  void SetUseTestPosition(bool bUse);
  bool GetUseTestPosition() const { return m_bUseTestPosition; }

  void SetTestPosition(const ezVec3& pos);
  const ezVec3& GetTestPosition() const { return m_TestPosition; }

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
  void RenderDebugOverlay();
  void OnObjectCreated(const ezAbstractObjectNode& node);

  bool m_bShowDebugOverlay = false;
  bool m_bShowDebugProbes = false;
  bool m_bUseTestPosition = false;
  ezVec3 m_TestPosition = ezVec3::ZeroVector();

  struct RenderDebugViewTask;
  ezSharedPtr<RenderDebugViewTask> m_pRenderDebugViewTask;

  ezGALTextureHandle m_hDebugViewTexture;

  ezProbeTreeSectorResourceHandle m_hProbeTree;
};
