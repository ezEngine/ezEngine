#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezMaterialAssetDocument;
class ezQtOrbitCamViewWidget;
class ezQtVisualShaderScene;
class ezQtNodeView;
struct ezSelectionManagerEvent;
class ezDirectoryWatcher;
enum class ezDirectoryWatcherAction;
class ezQtDocumentPanel;
class QTextEdit;
struct ezMaterialVisualShaderEvent;

class ezQtMaterialAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtMaterialAssetDocumentWindow(ezMaterialAssetDocument* pDocument);
  ~ezQtMaterialAssetDocumentWindow();

  ezMaterialAssetDocument* GetMaterialDocument();
  virtual const char* GetWindowLayoutGroupName() const override { return "MaterialAsset"; }

protected:
  virtual void InternalRedraw() override;


  virtual void showEvent(QShowEvent* event) override;

private Q_SLOTS:
  void OnOpenShaderClicked(bool);

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void SelectionEventHandler(const ezSelectionManagerEvent& e);
  void SendRedrawMsg();
  void RestoreResource();
  void UpdateNodeEditorVisibility();
  void OnVseConfigChanged(const char* filename, ezDirectoryWatcherAction action);
  void VisualShaderEventHandler(const ezMaterialVisualShaderEvent& e);
  void SetupDirectoryWatcher(bool needIt);

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  ezQtVisualShaderScene* m_pScene = nullptr;
  ezQtNodeView* m_pNodeView = nullptr;
  ezQtDocumentPanel* m_pVsePanel = nullptr;
  QTextEdit* m_pOutputLine = nullptr;
  QPushButton* m_pOpenShaderButton = nullptr;
  bool m_bVisualShaderEnabled;

  static ezInt32 s_iNodeConfigWatchers;
  static ezDirectoryWatcher* s_pNodeConfigWatcher;
};

class ezMaterialModelAction : public ezEnumerationMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialModelAction, ezEnumerationMenuAction);

public:
  ezMaterialModelAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual ezInt64 GetValue() const override;
  virtual void Execute(const ezVariant& value) override;
};

class ezMaterialAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hMaterialModelAction;
};
