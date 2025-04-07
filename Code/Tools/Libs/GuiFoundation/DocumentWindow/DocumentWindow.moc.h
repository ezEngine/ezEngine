#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

#include <QMainWindow>
#include <ads/DockManager.h>

class ezQtContainerWindow;
class ezDocument;
class ezQtDocumentWindow;
class QLabel;
class QToolButton;

struct ezQtDocumentWindowEvent
{
  enum Type
  {
    WindowClosing,           ///< Sent shortly before the window is being deleted
    WindowClosed,            ///< Sent AFTER the window has been deleted. The pointer is given, but not valid anymore!
    WindowDecorationChanged, ///< Window title or icon has changed
    BeforeRedraw,            ///< Sent shortly before the content of the window is being redrawn
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

  ads::CDockManager* m_pDockManager = nullptr;

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
  void SetTargetFramerate(ezInt16 iTargetFPS);

  void TriggerRedraw();

  virtual void RequestWindowTabContextMenu(const QPoint& globalPos);

  static const ezDynamicArray<ezQtDocumentWindow*>& GetAllDocumentWindows() { return s_AllDocumentWindows; }

  /// \brief Returns the document window for the given document, if there is any. nullptr otherwise.
  static ezQtDocumentWindow* FindWindowByDocument(const ezDocument* pDocument);
  ezQtContainerWindow* GetContainerWindow() const;

  /// \brief Shows the given message for the given duration in the statusbar, then shows the permanent message again.
  void ShowTemporaryStatusBarMsg(const ezFormatString& text, ezTime duration = ezTime::MakeFromSeconds(5));

  /// \brief Sets which text to show permanently in the statusbar. Set an empty string to clear the message.
  void SetPermanentStatusBarMsg(const ezFormatString& text);

  /// \brief For unit tests to take a screenshot of the window (may include multiple views) to do image comparisons.
  virtual void CreateImageCapture(const char* szOutputPath);

  /// \brief In 'safe' mode we want to prevent the documents from using the stored window layout state
  static bool s_bAllowRestoreWindowLayout;

protected:
  virtual void showEvent(QShowEvent* event) override;
  virtual void hideEvent(QHideEvent* event) override;
  virtual bool event(QEvent* event) override;
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

  void FinishWindowCreation();

private Q_SLOTS:
  void SlotRestoreLayout();
  void SlotRedraw();
  void SlotQueuedDelete();
  void OnPermanentGlobalStatusClicked(bool);
  void OnStatusBarMessageChanged(const QString& sNewText);

private:
  void SaveWindowLayout();
  void RestoreWindowLayout(bool bForce);
  void DisableWindowLayoutSaving();

  void ShutdownDocumentWindow();

private:
  friend class ezQtContainerWindow;

  void SetVisibleInContainer(bool bVisible);

  bool m_bWindowRestored = false;
  bool m_bIsVisibleInContainer = false;
  bool m_bRedrawIsTriggered = false;
  bool m_bIsDrawingATM = false;
  bool m_bTriggerRedrawQueued = false;
  bool m_bAllowSaveWindowLayout = true;
  ezInt16 m_iTargetFramerate = 0;
  ezDocument* m_pDocument = nullptr;
  ezQtContainerWindow* m_pContainerWindow = nullptr;
  QLabel* m_pPermanentDocumentStatusText = nullptr;
  QToolButton* m_pPermanentGlobalStatusButton = nullptr;

private:
  void Constructor();
  void DocumentManagerEventHandler(const ezDocumentManager::Event& e);
  void DocumentEventHandler(const ezDocumentEvent& e);
  void UIServicesEventHandler(const ezQtUiServices::Event& e);
  void UIServicesTickEventHandler(const ezQtUiServices::TickEvent& e);

  virtual void InternalDeleteThis() { delete this; }
  virtual bool InternalCanCloseWindow();
  virtual void InternalCloseDocumentWindow();
  virtual void InternalVisibleInContainerChanged(bool bVisible) {}
  virtual void InternalRedraw() {}

  ezString m_sUniqueName;

  static ezDynamicArray<ezQtDocumentWindow*> s_AllDocumentWindows;
};
