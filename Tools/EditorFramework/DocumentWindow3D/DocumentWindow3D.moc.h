#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <EditorFramework/IPC/IPCObjectMirror.h>

class QWidget;
class QHBoxLayout;
class QPushButton;
class ezQtEngineViewWidget;
class ezAssetDocument;
class ezEditorEngineDocumentMsg;

struct ezObjectPickingResult
{
  ezUuid m_PickedObject;
  ezUuid m_PickedComponent;
  ezUuid m_PickedOther;
  ezUInt32 m_uiPartIndex;
  ezVec3 m_vPickedPosition;
  ezVec3 m_vPickedNormal;
  ezVec3 m_vPickingRayStart;
};

/// \brief Base class for all document windows that need a connection to the engine process, and might want to render 3D content.
///
/// This class has an ezEditorEngineConnection object for sending messages between the editor and the engine process.
/// It also allows to embed ezQtEngineViewWidget objects into the UI, which enable 3D rendering by the engine process.
class EZ_EDITORFRAMEWORK_DLL ezQtEngineDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtEngineDocumentWindow(ezAssetDocument* pDocument);
  virtual ~ezQtEngineDocumentWindow();

  ezEditorEngineConnection* GetEditorEngineConnection() const;
  const ezObjectPickingResult& PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const;

  ezAssetDocument* GetDocument() const;

  /// \brief Returns the ezQtEngineViewWidget over which the mouse currently hovers
  ezQtEngineViewWidget* GetHoveredViewWidget() const;

  /// \brief Returns the ezQtEngineViewWidget that has the input focus
  ezQtEngineViewWidget* GetFocusedViewWidget() const;

  ezQtEngineViewWidget* GetViewWidgetByID(ezUInt32 uiViewID) const;

protected:
  friend class ezQtEngineViewWidget;

  ezHybridArray<ezQtEngineViewWidget*, 4> m_ViewWidgets;

  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg);

  void DestroyAllViews();
  virtual void InternalRedraw() override;

private:
  mutable ezObjectPickingResult m_LastPickingResult;
};
