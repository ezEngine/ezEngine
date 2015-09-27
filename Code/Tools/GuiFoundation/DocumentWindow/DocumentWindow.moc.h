#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>
#include <ToolsFoundation/Basics/Status.h>
#include <QMainWindow>
#include <ToolsFoundation/Document/DocumentManager.h>

class ezContainerWindow;
class ezDocumentBase;

class EZ_GUIFOUNDATION_DLL ezDocumentWindow : public QMainWindow
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

  virtual ezString GetWindowIcon() const;
  virtual ezString GetDisplayName() const { return GetUniqueName(); }
  virtual ezString GetDisplayNameShort() const;

  const char* GetUniqueName() const { return m_sUniqueName; }

  /// \brief The 'GroupName' is used for serializing window layouts. It should be unique among different window types.
  virtual const char* GetGroupName() const = 0;

  ezDocumentBase* GetDocument() const { return m_pDocument; }

  ezStatus SaveDocument();

  bool CanCloseWindow();
  void CloseDocumentWindow();

  void ScheduleRestoreWindowLayout();

  bool IsVisibleInContainer() const { return m_bIsVisibleInContainer; }
  void SetTargetFramerate(ezInt16 uiTargetFPS);

  void TriggerRedraw(float fLastFrameTimeMS = 0.0f);

  virtual void RequestWindowTabContextMenu(const QPoint& GlobalPos);

  static const ezDynamicArray<ezDocumentWindow*>& GetAllDocumentWindows() { return s_AllDocumentWindows; }

  static ezDocumentWindow* FindWindowByDocument(const ezDocumentBase* pDocument);

private slots:
  void SlotRestoreLayout();
  void SlotRedraw();
  void SlotQueuedDelete();

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
  ezInt16 m_iTargetFramerate;
  ezDocumentBase* m_pDocument;
  ezContainerWindow* m_pContainerWindow;

private:
  void Constructor();
  void DocumentManagerEventHandler(const ezDocumentManagerBase::Event& e);
  void DocumentEventHandler(const ezDocumentBase::Event& e);
  void UIServicesEventHandler(const ezUIServices::Event& e);

  virtual void InternalDeleteThis() { delete this; }
  virtual bool InternalCanCloseWindow();
  virtual void InternalCloseDocumentWindow();
  virtual void InternalVisibleInContainerChanged(bool bVisible) { }
  virtual void InternalRedraw() { }

  ezString m_sUniqueName;

  static ezDynamicArray<ezDocumentWindow*> s_AllDocumentWindows;
};




