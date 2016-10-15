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

/// \brief Container window that hosts documents and applications panels.
class EZ_GUIFOUNDATION_DLL ezQtContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  /// \brief Constructor.
  ///
  /// \param iUniqueIdentifier
  ///   Identifies the container. Must not exist yet when creating. 0 is used
  ///   for the main container window. Closing that one will close all others and the application.
  explicit ezQtContainerWindow(ezInt32 iUniqueIdentifier);
  ~ezQtContainerWindow();

  static const ezDynamicArray<ezQtContainerWindow*>& GetAllContainerWindows() { return s_AllContainerWindows; }
  /// \brief Creates a new container window with a new unique ID.
  static ezQtContainerWindow* CreateNewContainerWindow();
  /// \brief If the given ID is already used, return matching container. Otherwise create it.
  static ezQtContainerWindow* GetOrCreateContainerWindow(ezInt32 iUniqueIdentifier);

  void MoveDocumentWindowToContainer(ezQtDocumentWindow* pDocWindow);
  void MoveApplicationPanelToContainer(ezQtApplicationPanel* pPanel);

  /// \brief Returns true if the unique ID of the container is 0.
  bool IsMainContainer() const;
  /// \brief Returns the unique ID of the container. 0 for the main one and increasing in number for additional containers.
  ezInt32 GetUniqueIdentifier() const;

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

  ezString GetUniqueName() const;

  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void ProjectEventHandler(const ezToolsProjectEvent& e);
  void UIServicesEventHandler(const ezQtUiServices::Event& e);

  void closeEvent(QCloseEvent* e);

  /// \brief Called whenever the tab order changes to tell each document window at what position it is.
  void ReassignWindowIndex();

private:
  ezInt32 m_iUniqueIdentifier;
  QLabel* m_pStatusBarLabel;
  ezDynamicArray<ezQtDocumentWindow*> m_DocumentWindows;
  ezDynamicArray<ezQtApplicationPanel*> m_ApplicationPanels;

  static ezDynamicArray<ezQtContainerWindow*> s_AllContainerWindows;
  static bool s_bForceClose;
};




