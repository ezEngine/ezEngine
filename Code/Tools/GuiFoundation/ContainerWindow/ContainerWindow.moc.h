#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMainWindow>

class ezDocumentManager;
class ezDocument;
class ezQtApplicationPanel;
struct ezDocumentTypeDescriptor;
class QLabel;

class EZ_GUIFOUNDATION_DLL ezContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  ezContainerWindow();
  ~ezContainerWindow();

  static const ezDynamicArray<ezContainerWindow*>& GetAllContainerWindows() { return s_AllContainerWindows; }

  void MoveDocumentWindowToContainer(ezQtDocumentWindow* pDocWindow);
  void MoveApplicationPanelToContainer(ezQtApplicationPanel* pPanel);

  static ezResult EnsureVisibleAnyContainer(ezDocument* pDocument);

private:
  friend class ezQtDocumentWindow;
  friend class ezQtApplicationPanel;

  ezResult EnsureVisible(ezQtDocumentWindow* pDocWindow);
  ezResult EnsureVisible(ezDocument* pDocument);
  ezResult EnsureVisible(ezQtApplicationPanel* pPanel);

  bool m_bWindowLayoutRestored;
  void ScheduleRestoreWindowLayout();
  ezInt32 m_iWindowLayoutRestoreScheduled;

private slots:
  void SlotDocumentTabCloseRequested(int index);
  void SlotRestoreLayout();
  void SlotTabsContextMenuRequested(const QPoint& pos);
  void SlotDocumentTabCurrentChanged(int index);
  void SlotUpdateWindowDecoration(void* pDocWindow);

private:
  QTabWidget* GetTabWidget() const;

  void SaveWindowLayout();
  void RestoreWindowLayout();

  void UpdateWindowTitle();

  void RemoveDocumentWindowFromContainer(ezQtDocumentWindow* pDocWindow);
  void RemoveApplicationPanelFromContainer(ezQtApplicationPanel* pPanel);

  void UpdateWindowDecoration(ezQtDocumentWindow* pDocWindow);

  void SetupDocumentTabArea();

  const char* GetUniqueName() const { return "ezEditor"; /* todo */ }

  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void ProjectEventHandler(const ezToolsProject::Event& e);
  void UIServicesEventHandler(const ezUIServices::Event& e);

  void closeEvent(QCloseEvent* e);

  /// \brief Called whenever the tab order changes to tell each document window at what position it is.
  void ReassignWindowIndex();

private:
  QLabel* m_pStatusBarLabel;
  ezDynamicArray<ezQtDocumentWindow*> m_DocumentWindows;
  ezDynamicArray<ezQtApplicationPanel*> m_ApplicationPanels;

  static ezDynamicArray<ezContainerWindow*> s_AllContainerWindows;
};




