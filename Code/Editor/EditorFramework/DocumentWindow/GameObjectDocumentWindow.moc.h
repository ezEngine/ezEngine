#pragma once
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>

class ezGameObjectDocument;
class ezWorldSettingsMsgToEngine;
class ezQtGameObjectViewWidget;
struct ezGameObjectEvent;
struct ezSnapProviderEvent;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT
public:
  ezQtGameObjectDocumentWindow(ezGameObjectDocument* pDocument);
  ~ezQtGameObjectDocumentWindow();

  ezGameObjectDocument* GetGameObjectDocument() const;

protected:
  ezWorldSettingsMsgToEngine GetWorldSettings() const;
  ezGridSettingsMsgToEngine GetGridSettings() const;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void GameObjectEventHandler(const ezGameObjectEvent& e);
  void SnapProviderEventHandler(const ezSnapProviderEvent& e);

  void FocusOnSelectionAllViews();
  void FocusOnSelectionHoveredView();

  void HandleFocusOnSelection(const ezQuerySelectionBBoxResultMsgToEditor* pMsg, ezQtGameObjectViewWidget* pSceneView);
};
