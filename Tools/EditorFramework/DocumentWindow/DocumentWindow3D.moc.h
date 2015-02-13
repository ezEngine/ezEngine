#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

class QWidget;
class QHBoxLayout;
class QPushButton;

class EZ_EDITORFRAMEWORK_DLL ezDocumentWindow3D : public ezDocumentWindow
{
  Q_OBJECT

public:

  struct Event
  {
    //enum Type
    //{
    //  WindowClosed,
    //  WindowDecorationChanged,
    //};
  };

  //static ezEvent<const Event&> s_Events;

  virtual bool HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg);

public:
  ezDocumentWindow3D(ezDocumentBase* pDocument);
  virtual ~ezDocumentWindow3D();

  ezEditorEngineConnection* GetEditorEngineConnection() const { return m_pEngineView; }

private slots:
  void SlotRestartEngineProcess();

protected:
  ezEditorEngineConnection* m_pEngineView;

private:
  void ShowRestartButton(bool bShow);
  void EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e);

  QHBoxLayout* m_pRestartButtonLayout;
  QPushButton* m_pRestartButton;

};




