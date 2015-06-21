#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <QDockWidget>

class ezContainerWindow;

class EZ_GUIFOUNDATION_DLL ezApplicationPanel : public QDockWidget
{
public:
  Q_OBJECT

public:
  ezApplicationPanel(const char* szPanelName);
  ~ezApplicationPanel();

  void EnsureVisible();

  static const ezDynamicArray<ezApplicationPanel*>& GetAllApplicationPanels() { return s_AllApplicationPanels; }

protected:
  virtual void ToolsProjectEventHandler(const ezToolsProject::Event& e);

private:
  friend class ezContainerWindow;

  static ezDynamicArray<ezApplicationPanel*> s_AllApplicationPanels;

  ezContainerWindow* m_pContainerWindow;
};

