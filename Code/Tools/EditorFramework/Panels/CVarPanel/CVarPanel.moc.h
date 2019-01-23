#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <Foundation/Containers/Map.h>

class ezQtCVarWidget;

class EZ_EDITORFRAMEWORK_DLL ezQtCVarPanel : public ezQtApplicationPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtCVarPanel);

public:
  ezQtCVarPanel();
  ~ezQtCVarPanel();

protected:
  virtual void ToolsProjectEventHandler(const ezToolsProjectEvent& e) override;

private Q_SLOTS:
  void UpdateUI();
  void BoolChanged(const char* szCVar, bool newValue);
  void FloatChanged(const char* szCVar, float newValue);
  void IntChanged(const char* szCVar, int newValue);
  void StringChanged(const char* szCVar, const char* newValue);

private:
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);

  ezQtCVarWidget* m_pCVarWidget = nullptr;

  ezMap<ezString, ezCVarWidgetData> m_EngineCVarState;

  bool m_bUpdateUI = false;
  bool m_bRebuildUI = false;
};
