#pragma once

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <Foundation/Basics.h>
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
class ezQtPropertyWidget;

Q_DECLARE_OPAQUE_POINTER(ezQtSceneViewWidget*);

class ezQtSceneDocumentWindowBase : public ezQtGameObjectDocumentWindow, public ezGameObjectGizmoInterface
{
  Q_OBJECT

public:
  ezQtSceneDocumentWindowBase(ezSceneDocument* pDocument);
  ~ezQtSceneDocumentWindowBase();

  ezSceneDocument* GetSceneDocument() const;

  virtual void CreateImageCapture(const char* szOutputPath) override;

public Q_SLOTS:
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

  void GameObjectEventHandler(const ezGameObjectEvent& e);
  void SnapSelectionToPosition(bool bSnapEachObject);
  void SendRedrawMsg();
  void ExtendPropertyGridContextMenu(QMenu& menu, ezQtPropertyWidget* pPropWidget);

protected:
  ezQtQuadViewWidget* m_pQuadViewWidget = nullptr;
};

class ezQtSceneDocumentWindow : public ezQtSceneDocumentWindowBase
{
  Q_OBJECT

public:
  ezQtSceneDocumentWindow(ezSceneDocument* pDocument);
  ~ezQtSceneDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "Scene"; }
};
