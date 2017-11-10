#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectGizmoHandler.h>

struct ezEngineViewPreferences;
class QGridLayout;
class ezQtViewWidgetContainer;
class ezQtSceneViewWidget;
class QSettings;
struct ezManipulatorManagerEvent;
class ezPreferences;
class ezQtQuadViewWidget;
struct ezEngineWindowEvent;

Q_DECLARE_OPAQUE_POINTER(ezQtSceneViewWidget*);

class ezQtSceneDocumentWindow : public ezQtGameObjectDocumentWindow, public ezGameObjectGizmoInterface
{
  Q_OBJECT

public:
  ezQtSceneDocumentWindow(ezSceneDocument* pDocument);
  ~ezQtSceneDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "Scene"; }
  ezSceneDocument* GetSceneDocument() const;

public slots:
  void ToggleViews(QWidget* pView);

public:
  /// \name ezGameObjectGizmoInterface implementation
  ///@{
  virtual ezObjectAccessorBase* GetObjectAccessor() override;
  virtual bool CanDuplicateSelection() const override;
  virtual void DuplicateSelection() override;
  ///@}

protected:
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;
  virtual void InternalRedraw() override;

private:
  void GameObjectEventHandler(const ezGameObjectEvent& e);
  void SnapProviderEventHandler(const ezSnapProviderEvent& e);

  void FocusOnSelectionAllViews();
  void FocusOnSelectionHoveredView();
  void SnapSelectionToPosition(bool bSnapEachObject);
  void HandleFocusOnSelection(const ezQuerySelectionBBoxResultMsgToEditor* pMsg, ezQtSceneViewWidget* pSceneView);

  void SendRedrawMsg();

private:
  ezQtQuadViewWidget* m_pQuadViewWidget = nullptr;
};
