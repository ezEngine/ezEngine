#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <QMainWindow>

class EZ_EDITORFRAMEWORK_DLL ezContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  ezContainerWindow(const char* szUniqueName);
  ~ezContainerWindow();

  virtual void SaveWindowLayout();
  virtual void RestoreWindowLayout();

  const char* GetUniqueName() const { return m_sUniqueName; }

  virtual ezDocumentWindow* AddDocumentWindow(const char* szUniqueName);
  ezDocumentWindow* GetDocumentWindow(const char* szUniqueName);

protected:
  ezMap<ezString, ezDocumentWindow*> m_DocumentWindows;

  virtual void SetupDocumentTabArea();
  virtual ezDocumentWindow* CreateDocumentWindow(const char* szUniqueName);

private slots:
  void OnDocumentTabCloseRequested(int index);

private:
  virtual void InternalCloseDocumentWindow(ezDocumentWindow* pDocumentWindow);
  virtual void DocumentWindowEventHandler(const ezDocumentWindow::Event& e);
  virtual void closeEvent(QCloseEvent* e);


  ezString m_sUniqueName;
};




