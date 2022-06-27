#pragma once

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_PluginSelectionDlg.h>
#include <QDialog>

class EZ_EDITORFRAMEWORK_DLL ezQtPluginSelectionDlg : public QDialog, public Ui_PluginSelectionDlg
{
public:
  Q_OBJECT

public:
  ezQtPluginSelectionDlg(ezPluginBundleSet* pPluginSet, QWidget* parent = nullptr);
  ~ezQtPluginSelectionDlg();


private Q_SLOTS:
  void on_Buttons_clicked(QAbstractButton* pButton);

private:
  ezPluginBundleSet m_LocalPluginSet;
  ezPluginBundleSet* m_pPluginSet = nullptr;
};
