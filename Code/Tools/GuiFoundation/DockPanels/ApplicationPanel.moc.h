#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <QDockWidget>

class ezContainerWindow;

/// \brief Base class for all panels that are supposed to be application wide (not tied to some document).
class EZ_GUIFOUNDATION_DLL ezQtApplicationPanel : public QDockWidget
{
public:
  Q_OBJECT

public:
  ezQtApplicationPanel(const char* szPanelName);
  ~ezQtApplicationPanel();

  void EnsureVisible();

  static const ezDynamicArray<ezQtApplicationPanel*>& GetAllApplicationPanels() { return s_AllApplicationPanels; }

protected:
  virtual void ToolsProjectEventHandler(const ezToolsProject::Event& e);

private:
  friend class ezContainerWindow;

  static ezDynamicArray<ezQtApplicationPanel*> s_AllApplicationPanels;

  ezContainerWindow* m_pContainerWindow;
};

