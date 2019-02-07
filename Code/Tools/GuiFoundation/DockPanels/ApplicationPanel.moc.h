#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <QDockWidget>

class ezQtContainerWindow;

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
  virtual void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

private:
  friend class ezQtContainerWindow;

  static ezDynamicArray<ezQtApplicationPanel*> s_AllApplicationPanels;

  ezQtContainerWindow* m_pContainerWindow;
};

