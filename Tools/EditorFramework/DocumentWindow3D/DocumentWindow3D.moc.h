#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <EditorFramework/IPC/SyncObject.h>

class QWidget;
class QHBoxLayout;
class QPushButton;
class ezEngineViewWidget;

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

class EZ_EDITORFRAMEWORK_DLL ezDocumentWindow3D : public ezDocumentWindow
{
  Q_OBJECT

public:

  virtual bool HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg);

public:
  ezDocumentWindow3D(ezDocumentBase* pDocument);
  virtual ~ezDocumentWindow3D();

  ezEditorEngineConnection* GetEditorEngineConnection() const { return m_pEngineConnection; }

  void SendMessageToEngine(ezEditorEngineDocumentMsg* pMessage, bool bSuperHighPriority = false) const;

  const ezObjectPickingResult& PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const;

  void AddSyncObject(ezEditorEngineSyncObject* pSync);
  void RemoveSyncObject(ezEditorEngineSyncObject* pSync);
  ezEditorEngineSyncObject* FindSyncObject(const ezUuid& guid);

  /// \brief Returns the ezEngineViewWidget over which the mouse currently hovers
  ezEngineViewWidget* GetHoveredViewWidget() const;

  /// \brief Returns the ezEngineViewWidget that has the input focus
  ezEngineViewWidget* GetFocusedViewWidget() const;

private slots:
  void SlotRestartEngineProcess();

protected:
  friend class ezEngineViewWidget;
  ezEditorEngineConnection* m_pEngineConnection;
  ezHybridArray<ezEngineViewWidget*, 4> m_ViewWidgets;

  void SyncObjectsToEngine();
  void DestroyAllViews();

private:
  virtual void InternalRedraw() override;

  void ShowRestartButton(bool bShow);
  void EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e);

  QHBoxLayout* m_pRestartButtonLayout;
  QPushButton* m_pRestartButton;

  mutable ezObjectPickingResult m_LastPickingResult;

  ezHashTable<ezUuid, ezEditorEngineSyncObject*> m_AllSyncObjects;
  ezDeque<ezEditorEngineSyncObject*> m_SyncObjects;

  ezHybridArray<ezUuid, 32> m_DeletedObjects;
};




