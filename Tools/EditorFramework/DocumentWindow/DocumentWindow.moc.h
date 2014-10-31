#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>
#include <QMainWindow>

class ezContainerWindow;

class EZ_EDITORFRAMEWORK_DLL ezDocumentWindow : public QMainWindow
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
  ezDocumentWindow(const char* szUniqueName);
  ~ezDocumentWindow();

  void EnsureVisible();

  virtual ezString GetDisplayName() const { return GetUniqueName(); }
  virtual ezString GetDisplayNameShort() const { return GetDisplayName(); }

  const char* GetUniqueName() const { return m_sUniqueName; }
  virtual const char* GetGroupName() const { return "Scene"; }

  bool CanClose();
  void CloseDocument();

  void ScheduleRestoreWindowLayout();

private slots:
  void SlotRestoreLayout();

private:
  void SaveWindowLayout();
  void RestoreWindowLayout();

private:
  friend class ezContainerWindow;

  ezContainerWindow* m_pContainerWindow;

private:
  virtual bool InternalCanClose();
  virtual void InternalCloseDocument();

  ezString m_sUniqueName;
};




