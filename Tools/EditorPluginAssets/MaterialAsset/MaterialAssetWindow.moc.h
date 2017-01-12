#pragma once

#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class QLabel;
class QScrollArea;
class ezQtImageWidget;
class ezMaterialAssetDocument;
class ezQtMaterialViewWidget;
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
  virtual const char* GetWindowLayoutGroupName() const { return "MaterialAsset"; }

protected:
  virtual void InternalRedraw() override;

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void SelectionEventHandler(const ezSelectionManagerEvent& e);
  void SendRedrawMsg();
  void RestoreResource();
  void UpdateNodeEditorVisibility();
  void OnVseConfigChanged(const char* filename, ezDirectoryWatcherAction action);
  void VisualShaderEventHandler(const ezMaterialVisualShaderEvent& e);

  ezSceneViewConfig m_ViewConfig;
  ezQtMaterialViewWidget* m_pViewWidget;
  ezQtVisualShaderScene* m_pScene;
  ezQtNodeView* m_pNodeView;
  ezQtDocumentPanel* m_pVsePanel;
  QTextEdit* m_pOutputLine;
  bool m_bVisualShaderEnabled;

  static ezInt32 s_iNodeConfigWatchers;
  static ezDirectoryWatcher* s_pNodeConfigWatcher;


};
