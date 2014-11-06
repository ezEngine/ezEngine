#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/DynamicArray.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <QMainWindow>

class EZ_EDITORFRAMEWORK_DLL ezContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  ezContainerWindow();
  ~ezContainerWindow();

  void MoveDocumentWindowToContainer(ezDocumentWindow* pDocWindow);

  void ShowSettingsTab() { SlotSettings(); }

private:
  friend class ezDocumentWindow;

  void EnsureVisible(ezDocumentWindow* pDocWindow);

private slots:
  void SlotDocumentTabCloseRequested(int index);
  void SlotRestoreLayout();
  void SlotSettings();

private:
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
};




