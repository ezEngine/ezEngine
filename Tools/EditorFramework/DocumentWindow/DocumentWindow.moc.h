#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>
#include <ToolsFoundation/Basics/Status.h>
#include <QMainWindow>
#include <ToolsFoundation/Document/DocumentManager.h>

class ezContainerWindow;
class ezDocumentBase;

class EZ_EDITORFRAMEWORK_DLL ezDocumentWindow : public QMainWindow
{
  Q_OBJECT

public:

  struct Event
  {
    enum Type
    {
      WindowClosing,            ///< Sent shortly before the window is being deleted
      WindowClosed,             ///< Sent AFTER the window has been deleted. The pointer is given, but not valid anymore!
      WindowDecorationChanged,  ///< Window title or icon has changed
    };

    Type m_Type;
    ezDocumentWindow* m_pWindow;
  };

  static ezEvent<const Event&> s_Events;

public:
  ezDocumentWindow(ezDocumentBase* pDocument);
  ezDocumentWindow(const char* szUniqueName);
  virtual ~ezDocumentWindow();

  void EnsureVisible();

  virtual ezString GetTypeIcon() const { return ":/Icons/Icons/ezEditor16.png"; }

  virtual ezString GetDisplayName() const { return GetUniqueName(); }
  virtual ezString GetDisplayNameShort() const;

  const char* GetUniqueName() const { return m_sUniqueName; }
  virtual const char* GetGroupName() const { return "Scene"; }

  ezDocumentBase* GetDocument() const { return m_pDocument; }

  ezStatus SaveDocument();

  bool CanCloseWindow();
  void CloseDocumentWindow();

  void ScheduleRestoreWindowLayout();

  bool IsVisibleInContainer() const { return m_bIsVisibleInContainer; }
  void SetTargetFramerate(ezInt8 iTargetFPS);

  void TriggerRedraw();

  static const ezDynamicArray<ezDocumentWindow*>& GetAllDocumentWindows() { return s_AllDocumentWindows; }

private slots:
  void SlotRestoreLayout();
  void SlotRedraw();

private:
  void SaveWindowLayout();
  void RestoreWindowLayout();

  void ShutdownDocumentWindow();

private:
  friend class ezContainerWindow;

  void SetVisibleInContainer(bool bVisible);

  bool m_bIsVisibleInContainer;
  bool m_bRedrawIsTriggered;
  bool m_bIsDrawingATM;
  bool m_bTriggerRedrawQueued;
  ezInt8 m_iTargetFramerate;
  ezDocumentBase* m_pDocument;
  ezContainerWindow* m_pContainerWindow;

private:
  void Constructor();
  void DocumentManagerEventHandler(const ezDocumentManagerBase::Event& e);
  void DocumentEventHandler(const ezDocumentBase::Event& e);

  virtual void InternalDeleteThis() { delete this; }
  virtual bool InternalCanCloseWindow();
  virtual void InternalCloseDocumentWindow();
  virtual void InternalVisibleInContainerChanged(bool bVisible) { }
  virtual void InternalRedraw() { }

  ezString m_sUniqueName;

  static ezDynamicArray<ezDocumentWindow*> s_AllDocumentWindows;
};




