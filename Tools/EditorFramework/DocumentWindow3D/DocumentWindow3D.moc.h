#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

class QWidget;
class QHBoxLayout;
class QPushButton;

struct ezObjectPickingResult
{
  ezUuid m_PickedObject;
  ezUuid m_PickedComponent;
  ezUuid m_PickedOther;
  ezUInt32 m_uiPartIndex;
  ezVec3 m_vPickedPosition;
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

  ezEditorEngineConnection* GetEditorEngineConnection() const { return m_pEngineView; }

  void SendMessageToEngine(ezEditorEngineDocumentMsg* pMessage, bool bSuperHighPriority = false) const;

  const ezObjectPickingResult& PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const;

private slots:
  void SlotRestartEngineProcess();

protected:
  ezEditorEngineConnection* m_pEngineView;

  void SyncObjects();

private:
  virtual void InternalRedraw() override;

  void ShowRestartButton(bool bShow);
  void EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e);

  QHBoxLayout* m_pRestartButtonLayout;
  QPushButton* m_pRestartButton;

  /// \todo Broken delegates
  ezDelegate<void(const ezEditorEngineProcessConnection::Event&)> m_DelegateEngineViewProcessEvents;

  mutable ezObjectPickingResult m_LastPickingResult;
};




