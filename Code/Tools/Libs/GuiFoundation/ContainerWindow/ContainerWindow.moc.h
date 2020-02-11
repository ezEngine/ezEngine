#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMainWindow>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <QSet>

class ezDocumentManager;
class ezDocument;
class ezQtApplicationPanel;
struct ezDocumentTypeDescriptor;
class QLabel;

namespace ads
{
  class CDockManager;
  class CFloatingDockContainer;
  class CDockWidget;
}

/// \brief Container window that hosts documents and applications panels.
class EZ_GUIFOUNDATION_DLL ezQtContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  /// \brief Constructor.
  explicit ezQtContainerWindow();
  ~ezQtContainerWindow();

  static ezQtContainerWindow* GetContainerWindow() { return s_pContainerWindow; }

  void AddDocumentWindow(ezQtDocumentWindow* pDocWindow);
  void AddApplicationPanel(ezQtApplicationPanel* pPanel);

  ads::CDockManager* GetDockManager() { return m_DockManager; }

  static ezResult EnsureVisibleAnyContainer(ezDocument* pDocument);

  void GetDocumentWindows(ezHybridArray<ezQtDocumentWindow*, 16>& windows);

  void SaveWindowLayout();
  void SaveDocumentLayouts();
  void RestoreWindowLayout();

  void ScheduleRestoreWindowLayout();

protected:
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

private:
  friend class ezQtDocumentWindow;
  friend class ezQtApplicationPanel;

  ezResult EnsureVisible(ezQtDocumentWindow* pDocWindow);
  ezResult EnsureVisible(ezDocument* pDocument);
  ezResult EnsureVisible(ezQtApplicationPanel* pPanel);

  bool m_bWindowLayoutRestored;
  ezInt32 m_iWindowLayoutRestoreScheduled;

private Q_SLOTS:
  void SlotDocumentTabCloseRequested();
  void SlotRestoreLayout();
  void SlotTabsContextMenuRequested(const QPoint& pos);
  void SlotUpdateWindowDecoration(void* pDocWindow);
  void SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget);

private:
  void UpdateWindowTitle();

  void RemoveDocumentWindow(ezQtDocumentWindow* pDocWindow);
  void RemoveApplicationPanel(ezQtApplicationPanel* pPanel);

  void UpdateWindowDecoration(ezQtDocumentWindow* pDocWindow);

  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void ProjectEventHandler(const ezToolsProjectEvent& e);
  void UIServicesEventHandler(const ezQtUiServices::Event& e);

  void closeEvent(QCloseEvent* e);

private:
  ads::CDockManager* m_DockManager = nullptr;
  QLabel* m_pStatusBarLabel;
  ezDynamicArray<ezQtDocumentWindow*> m_DocumentWindows;
  ezDynamicArray<ads::CDockWidget*> m_DocumentDocks;

  ezDynamicArray<ezQtApplicationPanel*> m_ApplicationPanels;
  QSet<QString> m_DockNames;

  static ezQtContainerWindow* s_pContainerWindow;
  static bool s_bForceClose;
};
