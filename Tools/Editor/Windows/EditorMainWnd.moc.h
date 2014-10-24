#pragma once

#include <Foundation/Basics.h>
#include <QMainWindow>
#include <Tools/Editor/ui_EditorMainWnd.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class ezEditorMainWnd : public QMainWindow, public Ui_EditorMainWnd
{
public:
  Q_OBJECT

public:
  ezEditorMainWnd();
  ~ezEditorMainWnd();

  static ezEditorMainWnd* GetInstance() { return s_pWidget; }

  virtual void closeEvent(QCloseEvent* event);

public slots:

private slots:
  virtual void on_ActionConfigurePlugins_triggered();
  virtual void on_ActionProjectCreate_triggered();
  virtual void on_ActionProjectOpen_triggered();
  virtual void on_ActionProjectClose_triggered();
  virtual void on_ActionSceneCreate_triggered();
  virtual void on_ActionSceneOpen_triggered();
  virtual void on_ActionSceneClose_triggered();

private:
  void DocumentManagerEventHandler(const ezDocumentManagerBase::Event& e);

  void UpdateSupportedDocumentTypes();

  // Window Layout
  void SaveWindowLayout();
  void RestoreWindowLayout();

  // Plugins
  void LoadPlugins();
  void UnloadPlugins();


  static ezEditorMainWnd* s_pWidget;
};


