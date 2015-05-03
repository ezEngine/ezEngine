#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

class QWidget;
class QHBoxLayout;
class QPushButton;

class EZ_EDITORFRAMEWORK_DLL ezDocumentWindow3D : public ezDocumentWindow
{
  Q_OBJECT

public:

  virtual bool HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg);

public:
  ezDocumentWindow3D(ezDocumentBase* pDocument);
  virtual ~ezDocumentWindow3D();

  ezEditorEngineConnection* GetEditorEngineConnection() const { return m_pEngineView; }

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

};




