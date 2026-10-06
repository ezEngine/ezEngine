#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMainWindow>
#include <QSet>
#include <ToolsFoundation/Project/ToolsProject.h>

class ezDocumentManager;
class ezDocument;
class ezQtApplicationPanel;
struct ezDocumentTypeDescriptor;
class QLabel;

namespace ads
{
  class CDockAreaWidget;
  class CDockManager;
  class CFloatingDockContainer;
  class CDockWidget;
} // namespace ads

/// Container window that hosts documents and applications panels.
class EZ_GUIFOUNDATION_DLL ezQtContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  /// Constructor.
  ezQtContainerWindow();
  ~ezQtContainerWindow();

  static ezQtContainerWindow* GetContainerWindow() { return s_pContainerWindow; }

  /// Adds a tag that is shown in brackets after the window title, e.g. "[unattended]".
  ///
  /// Used to make it visible from the outside in which mode an editor runs, for instance when it is controlled by an
  /// automated tool, so that a user can tell such an instance apart from their own. Adding a tag twice has no effect.
  static void AddWindowTitleTag(ezStringView sTag);
  static void RemoveWindowTitleTag(ezStringView sTag);

  void AddDocumentWindow(ezQtDocumentWindow* pDocWindow);
  void DocumentWindowRenamed(ezQtDocumentWindow* pDocWindow);
  void AddApplicationPanel(ezQtApplicationPanel* pPanel);

  ads::CDockManager* GetDockManager() { return m_pDockManager; }

  static ezResult EnsureVisibleAnyContainer(ezDocument* pDocument);

  void GetDocumentWindows(ezHybridArray<ezQtDocumentWindow*, 16>& ref_windows);

  struct DocumentWindowState
  {
    bool m_bFloating = false;
  };

  /// Saves the current state (floating/docked) of all document windows.
  /// Call before restoring a layout.
  void SaveDocumentWindowStates(ezMap<ads::CDockWidget*, DocumentWindowState>& out_states);

  /// Restores document windows to their previous states after a layout change.
  /// Call after restoring a layout.
  void RestoreDocumentWindowStates(const ezMap<ads::CDockWidget*, DocumentWindowState>& states);

protected:
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

private:
  friend class ezQtDocumentWindow;
  friend class ezQtApplicationPanel;

  ezResult EnsureVisible(ezQtDocumentWindow* pDocWindow);
  ezResult EnsureVisible(ezDocument* pDocument);
  ezResult EnsureVisible(ezQtApplicationPanel* pPanel);

private Q_SLOTS:
  void SlotDocumentTabCloseRequested();
  void SlotTabsContextMenuRequested(const QPoint& pos);
  void SlotUpdateWindowDecoration(void* pDocWindow);
  void SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget);
  void SlotDockWidgetFloatingChanged(bool bFloating);
  void SlotDockWidgetVisibilityChanged(bool bVisible);

private:
  void UpdateWindowTitle();

  void RemoveDocumentWindow(ezQtDocumentWindow* pDocWindow);
  void RemoveApplicationPanel(ezQtApplicationPanel* pPanel);

  void UpdateWindowDecoration(ezQtDocumentWindow* pDocWindow);

  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void ProjectEventHandler(const ezToolsProjectEvent& e);
  void UIServicesEventHandler(const ezQtUiServices::Event& e);

  virtual void closeEvent(QCloseEvent* e) override;

  /// Implements Ctrl+Tab / Ctrl+Shift+Tab switching between the document tabs of one dock area, in most-recently-used order.
  ///
  /// Called for every event of the application. Returns true, if the event was consumed.
  bool HandleDocumentTabCycling(QEvent* e);
  void CycleDocumentTab(bool bBackwards);
  void FinishDocumentTabCycling();
  ads::CDockAreaWidget* FindActiveDocumentArea() const;

private:
  ads::CDockManager* m_pDockManager = nullptr;
  QLabel* m_pStatusBarLabel;
  ezDynamicArray<ezQtDocumentWindow*> m_DocumentWindows;
  ezDynamicArray<ads::CDockWidget*> m_DocumentDocks;

  /// All document docks, the one that was shown most recently comes first.
  ezDynamicArray<ads::CDockWidget*> m_DocumentDocksMRU;
  /// While Ctrl+Tab cycling is in progress: the docks that are cycled through, in MRU order at the start of cycling.
  ezDynamicArray<ads::CDockWidget*> m_TabCycleDocks;
  ezUInt32 m_uiTabCycleIndex = 0;

  ezDynamicArray<ezQtApplicationPanel*> m_ApplicationPanels;
  QSet<QString> m_DockNames;

  static ezQtContainerWindow* s_pContainerWindow;
  static bool s_bForceClose;
  static ezHybridArray<ezString, 4> s_WindowTitleTags;
};
