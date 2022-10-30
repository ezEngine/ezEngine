#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_PluginSelectionWidget.h>
#include <Foundation/Strings/String.h>
#include <QWidget>

struct ezPluginBundleSet;
struct ezPluginBundle;

class EZ_EDITORFRAMEWORK_DLL ezQtPluginSelectionWidget : public QWidget, public Ui_PluginSelectionWidget
{
public:
  Q_OBJECT

public:
  ezQtPluginSelectionWidget(QWidget* parent);
  ~ezQtPluginSelectionWidget();

  void SetPluginSet(ezPluginBundleSet* pPluginSet);
  void SyncStateToSet();

private Q_SLOTS:
  void on_PluginsList_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
  void on_PluginsList_itemChanged(QListWidgetItem* item);
  void on_Template_currentIndexChanged(int index);

private:
  struct State
  {
    ezString m_sID;
    ezPluginBundle* m_pInfo = nullptr;
    bool m_bLoadCopy = false;
    bool m_bSelected = false;
    bool m_bIsDependency = false;
  };

  void UpdateInternalState();
  void ApplyRequired(ezArrayPtr<ezString> required);

  ezHybridArray<State, 8> m_States;
  ezPluginBundleSet* m_pPluginSet = nullptr;
};
