#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>

class ezQtParticleViewWidget;
class ezParticleEffectAssetDocument;

class ezQtParticleEffectAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtParticleEffectAssetDocumentWindow(ezAssetDocument* pDocument);
  ~ezQtParticleEffectAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "ParticleEffectAsset"; }
  ezParticleEffectAssetDocument* GetParticleDocument();

private slots:

protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();
  void RestoreResource();
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void ParticleEventHandler(const ezParticleEffectAssetEvent& e);

  ezParticleEffectAssetDocument* m_pAssetDoc;

  ezEngineViewConfig m_ViewConfig;
  ezQtParticleViewWidget* m_pViewWidget;

};
