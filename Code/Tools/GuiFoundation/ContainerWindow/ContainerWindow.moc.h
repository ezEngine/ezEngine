#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMainWindow>

class ezDocumentManagerBase;
class ezDocumentBase;
class ezApplicationPanel;
struct ezDocumentTypeDescriptor;
class QLabel;

class EZ_GUIFOUNDATION_DLL ezContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  ezContainerWindow();
  ~ezContainerWindow();

  static const ezDynamicArray<ezContainerWindow*>& GetAllContainerWindows() { return s_AllContainerWindows; }

  void MoveDocumentWindowToContainer(ezDocumentWindow* pDocWindow);
  void MoveApplicationPanelToContainer(ezApplicationPanel* pPanel);

  static ezResult EnsureVisibleAnyContainer(ezDocumentBase* pDocument);

private:
  friend class ezDocumentWindow;
  friend class ezApplicationPanel;

  ezResult EnsureVisible(ezDocumentWindow* pDocWindow);
  ezResult EnsureVisible(ezDocumentBase* pDocument);
  ezResult EnsureVisible(ezApplicationPanel* pPanel);

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

  void RemoveDocumentWindowFromContainer(ezDocumentWindow* pDocWindow);
  void RemoveApplicationPanelFromContainer(ezApplicationPanel* pPanel);

  void UpdateWindowDecoration(ezDocumentWindow* pDocWindow);

  void SetupDocumentTabArea();

  const char* GetUniqueName() const { return "ezEditor"; /* todo */ }

  void DocumentWindowEventHandler(const ezDocumentWindow::Event& e);
  void ProjectEventHandler(const ezToolsProject::Event& e);
  void UIServicesEventHandler(const ezUIServices::Event& e);

  void closeEvent(QCloseEvent* e);

private:
  QLabel* m_pStatusBarLabel;
  ezDynamicArray<ezDocumentWindow*> m_DocumentWindows;
  ezDynamicArray<ezApplicationPanel*> m_ApplicationPanels;

  static ezDynamicArray<ezContainerWindow*> s_AllContainerWindows;
};




