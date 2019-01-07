#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>

class ezQtOrbitCamViewWidget;
class ezParticleEffectAssetDocument;
class QComboBox;
class QToolButton;
class ezQtPropertyGridWidget;


class ezQtParticleEffectAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtParticleEffectAssetDocumentWindow(ezAssetDocument* pDocument);
  ~ezQtParticleEffectAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const;
  ezParticleEffectAssetDocument* GetParticleDocument();

private Q_SLOTS:
  void onSystemSelected(int index);
  void onAddSystem(bool);
  void onRemoveSystem(bool);
  void onRenameSystem(bool);

protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();
  void RestoreResource();
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void ParticleEventHandler(const ezParticleEffectAssetEvent& e);
  void UpdateSystemList();
  void SelectSystem(ezDocumentObject* pObject);

  ezParticleEffectAssetDocument* m_pAssetDoc;

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;

  QComboBox* m_pSystemsCombo = nullptr;
  QToolButton* m_pAddSystem = nullptr;
  QToolButton* m_pRemoveSystem = nullptr;
  QToolButton* m_pRenameSystem = nullptr;
  ezQtPropertyGridWidget* m_pPropertyGridSystems = nullptr;
  ezQtPropertyGridWidget* m_pPropertyGridEmitter = nullptr;
  ezQtPropertyGridWidget* m_pPropertyGridInitializer = nullptr;
  ezQtPropertyGridWidget* m_pPropertyGridBehavior = nullptr;
  ezQtPropertyGridWidget* m_pPropertyGridType = nullptr;

  ezString m_sSelectedSystem;
  ezMap<ezString, ezDocumentObject*> m_ParticleSystems;
};
