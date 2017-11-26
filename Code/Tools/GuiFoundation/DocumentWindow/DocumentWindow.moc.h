#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Status.h>
#include <QMainWindow>
#include <ToolsFoundation/Document/DocumentManager.h>

class ezQtContainerWindow;
class ezDocument;
class ezQtDocumentWindow;
class QLabel;

struct ezQtDocumentWindowEvent
{
  enum Type
  {
    WindowClosing,            ///< Sent shortly before the window is being deleted
    WindowClosed,             ///< Sent AFTER the window has been deleted. The pointer is given, but not valid anymore!
    WindowDecorationChanged,  ///< Window title or icon has changed
    BeforeRedraw,             ///< Sent shortly before the content of the window is being redrawn
  };

  Type m_Type;
  ezQtDocumentWindow* m_pWindow;
};

/// \brief Base class for all document windows. Handles the most basic document window management.
class EZ_GUIFOUNDATION_DLL ezQtDocumentWindow : public QMainWindow
{
  Q_OBJECT

public:

  static ezEvent<const ezQtDocumentWindowEvent&> s_Events;

public:
  ezQtDocumentWindow(ezDocument* pDocument);
  ezQtDocumentWindow(const char* szUniqueName);
  virtual ~ezQtDocumentWindow();

  void EnsureVisible();

  virtual ezString GetWindowIcon() const;
  virtual ezString GetDisplayName() const { return GetUniqueName(); }
  virtual ezString GetDisplayNameShort() const;

  const char* GetUniqueName() const { return m_sUniqueName; }

  /// \brief The 'GroupName' is used for serializing window layouts. It should be unique among different window types.
  virtual const char* GetWindowLayoutGroupName() const = 0;

  ezDocument* GetDocument() const { return m_pDocument; }

  ezStatus SaveDocument();

  bool CanCloseWindow();
  void CloseDocumentWindow();

  void ScheduleRestoreWindowLayout();

  bool IsVisibleInContainer() const { return m_bIsVisibleInContainer; }
  void SetTargetFramerate(ezInt16 uiTargetFPS);

  void TriggerRedraw(float fLastFrameTimeMS = 0.0f);

  virtual void RequestWindowTabContextMenu(const QPoint& GlobalPos);

  static const ezDynamicArray<ezQtDocumentWindow*>& GetAllDocumentWindows() { return s_AllDocumentWindows; }

  static ezQtDocumentWindow* FindWindowByDocument(const ezDocument* pDocument);
  ezQtContainerWindow* GetContainerWindow() const;

  void ShowTemporaryStatusBarMsg(const ezFormatString& sText);
  void SetPermanentStatusBarMsg(const ezFormatString& sText);

  /// \brief Sets at which tab order index this window is located
  void SetWindowIndex(ezUInt32 uiIndex) { m_uiWindowIndex = uiIndex; }

  /// \brief Returns at which tab order index this window is located
  ezUInt32 GetWindowIndex() const { return m_uiWindowIndex; }

protected:
  void FinishWindowCreation();

private slots:
  void SlotRestoreLayout();
  void SlotRedraw();
  void SlotQueuedDelete();

private:
  void SaveWindowLayout();
  void RestoreWindowLayout();

  void ShutdownDocumentWindow();

private:
  friend class ezQtContainerWindow;

  void SetVisibleInContainer(bool bVisible);

  bool m_bIsVisibleInContainer;
  bool m_bRedrawIsTriggered;
  bool m_bIsDrawingATM;
  bool m_bTriggerRedrawQueued;
  ezInt16 m_iTargetFramerate;
  ezDocument* m_pDocument;
  ezQtContainerWindow* m_pContainerWindow;
  ezUInt32 m_uiWindowIndex;
  QLabel* m_pPermanentStatusMsg = nullptr;

private:
  void Constructor();
  void DocumentManagerEventHandler(const ezDocumentManager::Event& e);
  void DocumentEventHandler(const ezDocumentEvent& e);
  void UIServicesEventHandler(const ezQtUiServices::Event& e);

  virtual void InternalDeleteThis() { delete this; }
  virtual bool InternalCanCloseWindow();
  virtual void InternalCloseDocumentWindow();
  virtual void InternalVisibleInContainerChanged(bool bVisible) { }
  virtual void InternalRedraw() { }

  ezString m_sUniqueName;

  static ezDynamicArray<ezQtDocumentWindow*> s_AllDocumentWindows;
};


