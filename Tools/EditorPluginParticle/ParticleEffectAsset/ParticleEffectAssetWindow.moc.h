#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class ezQtParticleViewWidget;
class ezParticleEffectAssetDocument;

class ezParticleEffectAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezParticleEffectAssetDocumentWindow(ezAssetDocument* pDocument);
  ~ezParticleEffectAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "ParticleEffectAsset"; }
  ezParticleEffectAssetDocument* GetParticleDocument();

private slots:
  
protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();
  void RestoreResource();
  void UpdatePreview();
  void AssetDocumentEventHandler(const ezAssetDocument::AssetEvent& e);
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void ParticleEventHandler(const ezParticleEffectAssetEvent& e);

  ezParticleEffectAssetDocument* m_pAssetDoc;

  ezSceneViewConfig m_ViewConfig;
  ezQtParticleViewWidget* m_pViewWidget;

};