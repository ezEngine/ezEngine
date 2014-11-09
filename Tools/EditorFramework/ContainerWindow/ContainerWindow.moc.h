#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/DynamicArray.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <QMainWindow>

class ezDocumentManagerBase;
struct ezDocumentTypeDescriptor;

class EZ_EDITORFRAMEWORK_DLL ezContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  ezContainerWindow();
  ~ezContainerWindow();

  static const ezDynamicArray<ezContainerWindow*>& GetAllContainerWindows() { return s_AllContainerWindows; }

  void MoveDocumentWindowToContainer(ezDocumentWindow* pDocWindow);

  void ShowSettingsTab() { SlotSettings(); }

private:
  friend class ezDocumentWindow;

  ezResult EnsureVisible(ezDocumentWindow* pDocWindow);
  ezResult EnsureVisible(ezDocumentBase* pDocument);

  static ezResult EnsureVisibleAnyContainer(ezDocumentBase* pDocument);

private slots:
  void SlotDocumentTabCloseRequested(int index);
  void SlotRestoreLayout();
  void SlotSettings();
  void SlotCreateDocument();
  void SlotOpenDocument();
  void SlotTabsContextMenuRequested(const QPoint& pos);
  void SlotCurrentTabSave();
  void SlotCurrentTabSaveAll();
  void SlotCurrentTabClose();
  void SlotCurrentTabOpenFolder();

private:
  QTabWidget* GetTabWidget() const;
  ezString BuildDocumentTypeFileFilter(bool bForCreation) const;
  ezResult FindDocumentTypeFromPath(const char* szPath, bool bForCreation, ezDocumentManagerBase*& out_pTypeManager, ezDocumentTypeDescriptor& out_TypeDesc) const;
  void CreateOrOpenDocument(bool bCreate);

  void SaveWindowLayout();
  void RestoreWindowLayout();

  void RemoveDocumentWindowFromContainer(ezDocumentWindow* pDocWindow);

  void SetupDocumentTabArea();

  const char* GetUniqueName() const { return "ezEditor"; /* todo */ }

  void DocumentWindowEventHandler(const ezDocumentWindow::Event& e);
  void closeEvent(QCloseEvent* e);

private:
  ezDynamicArray<ezDocumentWindow*> m_DocumentWindows;

  QAction* m_pActionSettings;
  QAction* m_pActionCreateDocument;
  QAction* m_pActionOpenDocument;
  QAction* m_pActionCurrentTabSave;
  QAction* m_pActionCurrentTabSaveAll;
  QAction* m_pActionCurrentTabClose;
  QAction* m_pActionCurrentTabOpenFolder;

  static ezDynamicArray<ezContainerWindow*> s_AllContainerWindows;
};




