#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/GUI/SimplePropertyGridWidget.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Tools/EditorFramework/ui_SettingsTab.h>
#include <Foundation/Configuration/Plugin.h>

class ezSettingsTab : public ezQtDocumentWindow, Ui_SettingsTab
{
  Q_OBJECT

public:
  ezSettingsTab();
  ~ezSettingsTab();

  static ezSettingsTab* GetInstance();

  virtual ezString GetWindowIcon() const override;

  virtual const char* GetGroupName() const override { return "Settings"; }
  
private slots:
  void SlotAssetChosen(QString sAssetGuid, QString sAssetPathRelative, QString sAssetPathAbsolute);

private:
  virtual bool InternalCanCloseWindow() override;
  virtual void InternalCloseDocumentWindow() override;
};



