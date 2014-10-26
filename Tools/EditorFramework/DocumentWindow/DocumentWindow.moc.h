#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>
#include <QWidget>

class ezContainerWindow;

class EZ_EDITORFRAMEWORK_DLL ezDocumentWindow : public QWidget
{
  Q_OBJECT

public:

  struct Event
  {
    enum Type
    {
      BeforeDocumentClosed,
      AfterDocumentClosed,
    };

    Type m_Type;
    ezDocumentWindow* m_pDocument;
  };

  static ezEvent<const Event&> s_Events;

public:
  ezDocumentWindow(const char* szUniqueName, ezContainerWindow* pContainer);
  ~ezDocumentWindow();

  virtual ezString GetDisplayName() const { return GetUniqueName(); }
  virtual ezString GetDisplayNameShort() const { return GetDisplayName(); }

  const char* GetUniqueName() const { return m_sUniqueName; }

  ezContainerWindow* GetContainerWindow() const { return m_pContainer; }

  bool CanClose();
  void CloseDocument();

private:
  virtual void closeEvent(QCloseEvent* e);
  virtual bool InternalCanClose();
  virtual void InternalCloseDocument();

  ezString m_sUniqueName;
  ezContainerWindow* m_pContainer;
};




