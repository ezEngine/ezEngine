#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/ui_PluginSelectionDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class EZ_EDITORFRAMEWORK_DLL ezQtPluginSelectionDlg : public ezQtDialog, public Ui_PluginSelectionDlg
{
public:
  Q_OBJECT

public:
  ezQtPluginSelectionDlg(ezPluginBundleSet* pPluginSet, QWidget* pParent = nullptr);
  ~ezQtPluginSelectionDlg();


private Q_SLOTS:
  void on_Buttons_clicked(QAbstractButton* pButton);

private:
  ezPluginBundleSet m_LocalPluginSet;
  ezPluginBundleSet* m_pPluginSet = nullptr;
};
