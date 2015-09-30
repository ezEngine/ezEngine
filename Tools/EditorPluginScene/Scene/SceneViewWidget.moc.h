#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <EditorPluginScene/InputContexts/SelectionContext.h>
#include <EditorPluginScene/InputContexts/CameraMoveContext.h>
#include <EditorPluginScene/InputContexts/CameraPositionContext.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

class QVBoxLayout;
class ezSceneDocumentWindow;

class ezSceneViewWidget : public ezEngineViewWidget
{
  Q_OBJECT
public:
  ezSceneViewWidget(QWidget* pParent, ezSceneDocumentWindow* pDocument, ezCameraMoveContextSettings* pCameraMoveSettings, ezSceneViewConfig* pViewConfig);
  ~ezSceneViewWidget();

  ezSelectionContext* m_pSelectionContext;
  ezCameraMoveContext* m_pCameraMoveContext;
  ezCameraPositionContext* m_pCameraPositionContext;

  virtual void SyncToEngine() override;

protected:
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* e) override;
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;

  ezUuid CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue);
  void MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition);
  void MoveDraggedObjectsToPosition(const ezVec3& vPosition);

  ezHybridArray<ezUuid, 16> m_DraggedObjects;
  ezTime m_LastDragMoveEvent;
};

class ezSceneViewWidgetContainer : public QWidget
{
  Q_OBJECT
public:
  ezSceneViewWidgetContainer(QWidget* pParent, ezSceneDocumentWindow* pDocument, ezCameraMoveContextSettings* pCameraMoveSettings, ezSceneViewConfig* pViewConfig);
  ~ezSceneViewWidgetContainer();

  ezSceneViewWidget* GetViewWidget() const { return m_pViewWidget; }

private:
  ezSceneViewWidget* m_pViewWidget;
  QVBoxLayout* m_pLayout;
};

