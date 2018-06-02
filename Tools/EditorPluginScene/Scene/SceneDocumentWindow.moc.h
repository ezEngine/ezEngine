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
#include <EditorFramework/EditTools/EditTool.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

struct ezEngineViewPreferences;
class QGridLayout;
class ezQtViewWidgetContainer;
class ezQtSceneViewWidget;
class QSettings;
struct ezManipulatorManagerEvent;
class ezPreferences;
class ezQtQuadViewWidget;
struct ezEngineWindowEvent;
class ezSceneDocument;
class QMenu;

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
  void SnapSelectionToPosition(bool bSnapEachObject);
  void SendRedrawMsg();
  void ExtendPropertyGridContextMenu(QMenu& menu, const ezHybridArray<ezPropertySelection, 8>& items, const ezAbstractProperty* pProp);

private:
  ezQtQuadViewWidget* m_pQuadViewWidget = nullptr;
};
