#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Tools/EditorFramework/ui_SettingsTab.h>
#include <Foundation/Configuration/Plugin.h>

class ezQtSettingsTab : public ezQtDocumentWindow, Ui_SettingsTab
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtSettingsTab);

public:
  ezQtSettingsTab();
  ~ezQtSettingsTab();

  virtual ezString GetWindowIcon() const override;

  virtual const char* GetWindowLayoutGroupName() const override { return "Settings"; }
  
private slots:
  void SlotAssetChosen(QString sAssetGuid, QString sAssetPathRelative, QString sAssetPathAbsolute);

private:
  virtual bool InternalCanCloseWindow() override;
  virtual void InternalCloseDocumentWindow() override;
};



