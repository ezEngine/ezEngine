#pragma once

#include <EditorFramework/ui_SettingsTab.h>
#include <Foundation/Configuration/Plugin.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class ezQtSettingsTab : public ezQtDocumentWindow, Ui_SettingsTab
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtSettingsTab);

public:
  ezQtSettingsTab();
  ~ezQtSettingsTab();

  virtual ezString GetWindowIcon() const override;
  virtual ezString GetDisplayNameShort() const override;

  virtual const char* GetWindowLayoutGroupName() const override { return "Settings"; }

private:
  virtual bool InternalCanCloseWindow() override;
  virtual void InternalCloseDocumentWindow() override;
};
